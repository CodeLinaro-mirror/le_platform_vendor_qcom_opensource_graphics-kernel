// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2012-2019, 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <dt-bindings/soc/qcom,ipcc.h>
#include <linux/file.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/sync_file.h>

#include "kgsl_device.h"
#include "kgsl_gmu_core.h"
#include "kgsl_sync.h"
#include "kgsl_trace.h"

const struct dma_fence_ops kgsl_sync_fence_ops;
static const struct dma_fence_ops kgsl_syncsource_fence_ops;

/* Only allow a single log in a second */
static __maybe_unused DEFINE_RATELIMIT_STATE(_rs, HZ, 1);

/* List of hardware fences that haven't been destroyed */
struct list_head hw_fence_list;
/* Spinlock to protect access to hw_fence_list */
spinlock_t hw_fence_list_lock;

static void _hw_fence_destroy(struct kgsl_sync_fence *kfence);

static __maybe_unused void add_hw_fence(struct kgsl_sync_fence *kfence)
{
	struct dma_fence *fence = &kfence->fence;
	unsigned long flags;

	spin_lock(&hw_fence_list_lock);
	/* This refcount is put back when this hardware fence is signaled by GMU */
	kref_init(&kfence->hw_refcount);

	list_add_tail(&kfence->hw_fence_list, &hw_fence_list);
	dma_fence_get(&kfence->fence);

	spin_unlock(&hw_fence_list_lock);

	/*
	 * If dma fence is not signaled then increment the refcount one more time.
	 * This refcount will be put back when this dma fence gets signaled.
	 */
	spin_lock_irqsave(fence->lock, flags);
	if (!test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags)) {
		kref_get(&kfence->hw_refcount);
		__set_bit(KGSL_FENCE_FLAG_SIGNAL_REFCOUNT, &kfence->flags);
	}
	spin_unlock_irqrestore(fence->lock, flags);
}

static __maybe_unused void destroy_all_hw_fences(void)
{
	struct kgsl_sync_fence *kfence, *next;

	spin_lock(&hw_fence_list_lock);

	list_for_each_entry_safe(kfence, next, &hw_fence_list, hw_fence_list) {
		_hw_fence_destroy(kfence);
		list_del_init(&kfence->hw_fence_list);
	}

	spin_unlock(&hw_fence_list_lock);
}

#if (IS_ENABLED(CONFIG_QCOM_KGSL_SYNX))

#include <synx_api.h>
#include <synx_interop.h>

static struct synx_hw_fence_descriptor {
	/** @hw_fence_session: Session handle for hardware fences */
	struct synx_session *hw_fence_session;
	/** @mem_descriptor: Memory descriptor for hardware fences */
	struct synx_queue_desc mem_descriptor;
	/** @synx_session: Session handle for native synx */
	struct synx_session *synx_session;
	/** @synx_mem_descriptor: Memory descriptor for synx global memory */
	struct synx_queue_desc synx_mem_descriptor;

} kgsl_synx;

int kgsl_hw_fence_soccp_vote(bool pwr_on)
{
	return synx_enable_resources(SYNX_CLIENT_HW_FENCE_GFX_CTX0, SYNX_RESOURCE_SOCCP, pwr_on);
}

static void synx_session_uninitialize(struct synx_session *session)
{
	synx_uninitialize(session);
}

static void kgsl_hw_fence_populate_md(struct kgsl_memdesc *md)
{
	md->physaddr = kgsl_synx.mem_descriptor.dev_addr;
	md->size = kgsl_synx.mem_descriptor.size;
	md->hostptr = kgsl_synx.mem_descriptor.vaddr;
}

static int _hw_fence_register(struct kgsl_device *device, struct kgsl_memdesc *md)
{
	struct synx_initialization_params params;
	int ret;

	params.id = (enum synx_client_id)SYNX_CLIENT_HW_FENCE_GFX_CTX0;
	params.ptr = &kgsl_synx.mem_descriptor;
	kgsl_synx.hw_fence_session = synx_initialize(&params);

	if (IS_ERR_OR_NULL(kgsl_synx.hw_fence_session)) {
		dev_err(device->dev, "HW fences not supported: %d\n",
			PTR_ERR_OR_ZERO(kgsl_synx.hw_fence_session));
		kgsl_synx.hw_fence_session = NULL;
		return -EINVAL;
	}

	/*
	 * We need to set up the memory descriptor with the physical address of the Tx/Rx Queues so
	 * that these buffers can be imported in to GMU VA space
	 */
	kgsl_memdesc_init(device, md, 0);
	kgsl_hw_fence_populate_md(md);

	ret = kgsl_memdesc_sg_dma(md, md->physaddr, md->size);
	if (ret) {
		dev_err(device->dev, "Failed to setup HW fences memdesc: %d\n",
			ret);
		kgsl_synx_deregister(device, md);
	}

	return ret;
}

int kgsl_hw_fence_register(struct kgsl_device *device, struct kgsl_memdesc *md)
{
	int ret = _hw_fence_register(device, md);

	if (!ret) {
		INIT_LIST_HEAD(&hw_fence_list);
		spin_lock_init(&hw_fence_list_lock);
	}

	return ret;
}

static void _kgsl_synx_session_deregister(struct synx_session *session, struct kgsl_memdesc *md)
{
	synx_session_uninitialize(session);
	kgsl_memdesc_free_sgt(md);
	memset(md, 0x0, sizeof(*md));
}

void kgsl_hw_fence_deregister(struct kgsl_device *device, struct kgsl_memdesc *md)
{
	destroy_all_hw_fences();
	_kgsl_synx_session_deregister(kgsl_synx.hw_fence_session, md);
}

static int _get_global_synx_mem_mapping(struct kgsl_device *device)
{
	struct device *dev = GMU_PDEV_DEV(device);
	struct device_node *np;
	struct resource r;
	int ret = 0;

	np = of_parse_phandle(dev->of_node, "synx-memory-region", 0);
	if (!np) {
		dev_err(dev, "synx-memory-region not found\n");
		return -ENOENT;
	}

	ret = of_address_to_resource(np, 0, &r);
	of_node_put(np);
	if (ret)
		return ret;

	kgsl_synx.synx_mem_descriptor.dev_addr = (u64)r.start;
	kgsl_synx.synx_mem_descriptor.size = resource_size(&r);

	return 0;
}

static void kgsl_synx_populate_md(struct kgsl_memdesc *md)
{
	md->physaddr = kgsl_synx.synx_mem_descriptor.dev_addr;
	md->size = kgsl_synx.synx_mem_descriptor.size;
}

static int _synx_register(struct kgsl_device *device, struct kgsl_memdesc *md)
{
	struct synx_initialization_params params;
	int ret = _get_global_synx_mem_mapping(device);

	if (ret)
		return ret;

	params.id = (enum synx_client_id)SYNX_CLIENT_GFX_CTX0;
	params.ptr = NULL;
	kgsl_synx.synx_session = synx_initialize(&params);
	if (IS_ERR_OR_NULL(kgsl_synx.synx_session)) {
		dev_err(device->dev, "SYNX not supported: %d\n",
		PTR_ERR_OR_ZERO(kgsl_synx.synx_session));
		kgsl_synx.synx_session = NULL;
		return -EINVAL;
	}

	kgsl_memdesc_init(device, md, 0);
	kgsl_synx_populate_md(md);

	ret = kgsl_memdesc_sg_dma(md, md->physaddr, md->size);
	if (ret) {
		dev_err(device->dev, "Failed to setup SYNX memdesc: %d\n",
			ret);
		kgsl_synx_deregister(device, md);
	}

	return ret;
}

int kgsl_synx_register(struct kgsl_device *device, struct kgsl_memdesc *synx_md)
{
	int ret = _synx_register(device, synx_md);

	if (!ret) {
		INIT_LIST_HEAD(&hw_fence_list);
		spin_lock_init(&hw_fence_list_lock);
	}

	return ret;
}

void kgsl_synx_deregister(struct kgsl_device *device, struct kgsl_memdesc *synx_md)
{
	destroy_all_hw_fences();

	_kgsl_synx_session_deregister(kgsl_synx.synx_session, synx_md);
}

static int _synx_create(struct kgsl_device *device, struct kgsl_sync_fence *kfence)
{
	u32 handle = 0;
	struct synx_import_params params = {
		.indv.fence = &kfence->fence,
		.type = SYNX_IMPORT_INDV_PARAMS,
		.indv.new_h_synx = &handle,
		.indv.flags = SYNX_IMPORT_GLOBAL_FENCE | SYNX_IMPORT_DMA_FENCE,
	};
	int ret;

	ret = synx_import(kgsl_synx.synx_session, &params);
	if (!ret) {
		kfence->synx_handle = handle;
		return 0;
	}

	if (__ratelimit(&_rs))
		dev_err(device->dev, "Failed to create ctx:%d ts:%d synx handle:%d\n",
			kfence->context_id, kfence->timestamp, ret);

	kfence->synx_handle = 0;

	return ret;
}

static int _hw_fence_create(struct kgsl_device *device, struct kgsl_sync_fence *kfence)
{
	u32 handle = 0;
	struct synx_create_params params = {
		.fence = &kfence->fence,
		.h_synx = &handle,
		.flags = SYNX_CREATE_DMA_FENCE,
	};
	int ret;

	ret = synx_create(kgsl_synx.hw_fence_session, &params);
	if (!ret) {
		kfence->hw_fence_handle = handle;
		return 0;
	}

	kfence->hw_fence_handle = 0;

	if (__ratelimit(&_rs))
		dev_err(device->dev, "Failed to create ctx:%d ts:%d hardware fence:%d\n",
			kfence->context_id, kfence->timestamp, ret);

	return ret;
}

static void _set_output_fence_type(struct kgsl_device *device, struct kgsl_context *context,
	struct kgsl_sync_fence *kfence)
{
	bool hybrid = device->gmu_core.output_fence_type ==
		(KGSL_OUTPUT_FENCE_TYPE_HW_FENCE | KGSL_OUTPUT_FENCE_TYPE_SYNX_FENCE);

	/*
	 * If both synx and hw fence type output fences are supported, then create hybrid output
	 * fences for processes that require hybrid output fences. For all other processes, default
	 * to hw fence type output fences
	 */
	if (hybrid && (!context->proc_priv->hybrid_output_fence))
		kfence->fence_type = KGSL_OUTPUT_FENCE_TYPE_HW_FENCE;
	else
		kfence->fence_type = device->gmu_core.output_fence_type;
}

int kgsl_hw_fence_create(struct kgsl_device *device, struct kgsl_context *context,
	struct kgsl_sync_fence *kfence)
{
	int ret = 0;

	_set_output_fence_type(device, context, kfence);

	/* Return error if fencing got disabled in the meanwhile */
	if (!kfence->fence_type)
		return -EINVAL;

	/*
	 * If the fence type is hybrid, we must first create the hw fence handle, and then create
	 * the synx handle
	 */
	if (kfence->fence_type & KGSL_OUTPUT_FENCE_TYPE_HW_FENCE) {
		ret = _hw_fence_create(device, kfence);
		if (ret)
			goto done;
	}

	if (kfence->fence_type & KGSL_OUTPUT_FENCE_TYPE_SYNX_FENCE)
		ret = _synx_create(device, kfence);

done:
	if (ret)
		_hw_fence_destroy(kfence);
	else
		add_hw_fence(kfence);

	return ret;
}

int kgsl_synx_import(struct kgsl_device *device, struct dma_fence *fence, u32 *hash_index)
{
	u32 handle = 0;
	struct synx_import_params params = {
		.indv.fence = fence,
		.type = SYNX_IMPORT_INDV_PARAMS,
		.indv.new_h_synx = &handle,
		.indv.flags = SYNX_IMPORT_DMA_FENCE | SYNX_IMPORT_GLOBAL_FENCE,
	};
	int ret;

	ret = synx_import(kgsl_synx.synx_session, &params);
	if (ret) {
		dev_err_ratelimited(device->dev,
			"Failed to import synx handle ret:%d for fence ctx:%llu ts:%llu\n",
			ret, fence->context, fence->seqno);
		return ret;
	}

	if (hash_index)
		*hash_index = handle;

	return ret;
}

void kgsl_synx_import_release(struct kgsl_device *device, u32 handle)
{
	int ret = synx_release(kgsl_synx.synx_session, handle);

	if (ret)
		dev_err_ratelimited(device->dev,
			"Failed to import release handle ret:%d handle:%d\n",
			ret, handle);
}

static int _external_hw_fence_import(struct kgsl_device *device,
	struct kgsl_drawobj_sync_input_fence *input_fence)
{
	u32 handle = 0;
	struct synx_import_params params = {
		.indv.fence = input_fence->fence,
		.type = SYNX_IMPORT_INDV_PARAMS,
		.indv.new_h_synx = &handle,
		.indv.flags = SYNX_IMPORT_DMA_FENCE,
	};
	int ret;

	ret = synx_import(kgsl_synx.hw_fence_session, &params);
	if (ret || !handle) {
		dev_err_ratelimited(device->dev,
			"Failed to import fence ctx:%llu ts:%llu ret:%d handle:%d\n",
			input_fence->fence->context, input_fence->fence->seqno, ret, handle);
		return ret;
	}

	/* release reference held by synx_import when importing as a hw fence client */
	ret = synx_release(kgsl_synx.hw_fence_session, handle);
	if (ret) {
		dev_err_ratelimited(device->dev,
			"Failed to release wait fence ret:%d fence ctx:%llu ts:%llu\n",
			ret, input_fence->fence->context, input_fence->fence->seqno);
	} else {
		input_fence->handle = handle;
	}

	return ret;
}

static int _external_synx_fence_import(struct kgsl_device *device,
	struct kgsl_drawobj_sync_input_fence *input_fence)
{
	u32 handle = 0;
	struct synx_import_params params = {
		.indv.fence = input_fence->fence,
		.type = SYNX_IMPORT_INDV_PARAMS,
		.indv.new_h_synx = &handle,
		.indv.flags = SYNX_IMPORT_DMA_FENCE | SYNX_IMPORT_GLOBAL_FENCE,
	};
	int ret;

	ret = synx_import(kgsl_synx.synx_session, &params);
	if (ret || !handle) {
		dev_err_ratelimited(device->dev,
			"Failed to import fence ctx:%llu ts:%llu ret:%d handle:%d\n",
			input_fence->fence->context, input_fence->fence->seqno, ret, handle);
		return ret;
	}

	input_fence->handle = handle;

	return ret;
}

static void _set_input_fence_type(struct kgsl_device *device,
	struct kgsl_drawobj_sync_input_fence *input_fence)
{
	bool hybrid = device->gmu_core.input_fence_type == (KGSL_INPUT_FENCE_TYPE_HW_FENCE |
		KGSL_INPUT_FENCE_TYPE_SYNX_FENCE);

	if (!hybrid) {
		input_fence->fence_type = device->gmu_core.input_fence_type;
		return;
	}

	/*
	 * If both synx and hw fence type are enabled, then import based on the type of input fence
	 */
	if (kgsl_is_synx_native_fence(input_fence->fence))
		input_fence->fence_type = KGSL_INPUT_FENCE_TYPE_SYNX_FENCE;
	else if (kgsl_is_synx_hw_fence(input_fence->fence))
		input_fence->fence_type = KGSL_INPUT_FENCE_TYPE_HW_FENCE;
}

int kgsl_external_fence_import(struct kgsl_device *device,
	struct kgsl_drawobj_sync_input_fence *input_fence)
{
	int ret = 0;

	/* Import this fence only once */
	if (input_fence->handle)
		return 0;

	_set_input_fence_type(device, input_fence);

	if (input_fence->fence_type  == KGSL_INPUT_FENCE_TYPE_SYNX_FENCE)
		ret = _external_synx_fence_import(device, input_fence);
	else if (input_fence->fence_type == KGSL_INPUT_FENCE_TYPE_HW_FENCE)
		ret = _external_hw_fence_import(device, input_fence);

	return ret;
}

bool kgsl_hw_fence_tx_slot_available(struct kgsl_device *device, u32 pending_hw_fence_count)
{
	void *ptr = kgsl_synx.mem_descriptor.vaddr;
	struct synx_hw_fence_hfi_queue_header *hdr = (struct synx_hw_fence_hfi_queue_header *)
		(ptr + sizeof(struct synx_hw_fence_hfi_queue_table_header));
	u32 queue_size_dwords = hdr->queue_size / sizeof(u32);
	u32 payload_size_dwords = hdr->pkt_size / sizeof(u32);
	u32 free_entries, write_idx = hdr->write_index, read_idx = hdr->read_index;

	free_entries = read_idx <= write_idx ?
		queue_size_dwords - (write_idx - read_idx) :
		read_idx - write_idx;

	free_entries /= payload_size_dwords;

	/* Ensure free_entries does not become negative */
	if (free_entries == 0)
		return false;

	free_entries -= 1;

	if (free_entries > pending_hw_fence_count)
		return true;

	return false;
}

static void _hw_fence_destroy(struct kgsl_sync_fence *kfence)
{
	if (kfence->hw_fence_handle)
		synx_release(kgsl_synx.hw_fence_session, kfence->hw_fence_handle);
	if (kfence->synx_handle)
		synx_release(kgsl_synx.synx_session, kfence->synx_handle);
}

void kgsl_hw_fence_trigger_cpu(struct kgsl_device *device, struct kgsl_sync_fence *kfence)
{
	/* soccp should be powered on */
	WARN_RATELIMIT(!test_bit(GMU_SOCCP_VOTE_ON, &device->gmu_core.flags),
		"signaling hw fence via cpu without soccp powered up\n");

	if (kfence->synx_handle)
		synx_signal(kgsl_synx.synx_session, (u32)kfence->synx_handle,
			SYNX_STATE_SIGNALED_SUCCESS);
	if (kfence->hw_fence_handle)
		synx_signal(kgsl_synx.hw_fence_session, (u32)kfence->hw_fence_handle,
			SYNX_STATE_SIGNALED_SUCCESS);
}

bool kgsl_hw_fence_signaled(struct dma_fence *fence)
{
	return test_bit(SYNX_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags) &&
	test_bit(SYNX_HW_FENCE_FLAG_SIGNALED_BIT, &fence->flags);
}

static bool kgsl_is_input_hw_fence(struct dma_fence *fence)
{
	return test_bit(SYNX_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags) ||
		test_bit(SYNX_NATIVE_FENCE_FLAG_ENABLED_BIT, &fence->flags) || is_kgsl_fence(fence);
}

bool kgsl_is_synx_hw_fence(struct dma_fence *fence)
{
	return test_bit(SYNX_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags);
}

bool kgsl_is_synx_native_fence(struct dma_fence *fence)
{
	return test_bit(SYNX_NATIVE_FENCE_FLAG_ENABLED_BIT, &fence->flags);
}

#elif IS_ENABLED(CONFIG_QTI_HW_FENCE)
#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
#include <msm_hw_fence.h>
#else
#include <linux/soc/qcom/msm_hw_fence.h>
#endif

static struct msm_hw_fence_descriptor {
	/** @handle: Handle for hardware fences */
	void *handle;
	/** @descriptor: Memory descriptor for hardware fences */
	struct msm_hw_fence_mem_addr mem_descriptor;
} kgsl_msm_hw_fence;

int kgsl_hw_fence_register(struct kgsl_device *device, struct kgsl_memdesc *md)
{
	kgsl_msm_hw_fence.handle = msm_hw_fence_register(HW_FENCE_CLIENT_ID_CTX0,
				&kgsl_msm_hw_fence.mem_descriptor);

	if (IS_ERR_OR_NULL(kgsl_msm_hw_fence.handle)) {
		dev_err(device->dev, "HW fences not supported: %d\n",
			PTR_ERR_OR_ZERO(kgsl_msm_hw_fence.handle));
		kgsl_msm_hw_fence.handle = NULL;
		return -EINVAL;
	}

	/*
	 * We need to set up the memory descriptor with the physical address of the Tx/Rx Queues so
	 * that these buffers can be imported in to GMU VA space
	 */
	kgsl_memdesc_init(device, md, 0);
	kgsl_hw_fence_populate_md(md);

	ret = kgsl_memdesc_sg_dma(md, md->physaddr, md->size);
	if (ret) {
		dev_err(device->dev, "Failed to setup HW fences memdesc: %d\n",
			ret);
		kgsl_hw_fence_deregister(device, md);
		return ret;
	};

	INIT_LIST_HEAD(&hw_fence_list);
	spin_lock_init(&hw_fence_list_lock);

	return 0;
}

void kgsl_hw_fence_deregister(struct kgsl_device *device, struct kgsl_memdesc *md)
{
	destroy_all_hw_fences();

	msm_hw_fence_deregister(kgsl_msm_hw_fence.handle);
	kgsl_memdesc_free_sgt(md->sgt);
	memset(md, 0x0, sizeof(*md));
}

static void kgsl_hw_fence_populate_md(struct kgsl_memdesc *md)
{
	md->physaddr = kgsl_msm_hw_fence.mem_descriptor.device_addr;
	md->size = kgsl_msm_hw_fence.mem_descriptor.size;
	md->hostptr = kgsl_msm_hw_fence.mem_descriptor.virtual_addr;
}

int kgsl_hw_fence_create(struct kgsl_device *device, struct kgsl_sync_fence *kfence)
{
	struct msm_hw_fence_create_params params = {
		.fence = &kfence->fence,
		.handle = &kfence->hw_fence_handle,
	};
	int ret;

	ret = msm_hw_fence_create(kgsl_msm_hw_fence.handle, &params);
	if ((ret || IS_ERR_OR_NULL(params.handle))) {
		if (__ratelimit(&_rs))
			dev_err(device->dev, "Failed to create ctx:%d ts:%d hardware fence:%d\n",
				kfence->context_id, kfence->timestamp, ret);
			return -EINVAL;
	}

	add_hw_fence(kfence);

	return 0;
}

int kgsl_external_fence_import(struct kgsl_device *device,
	struct kgsl_drawobj_sync_input_fence *input_fence)
{
	u64 handle = 0;
	int ret;

	/* Import this fence only once */
	if (input_fence->handle)
		return 0;

	ret = msm_hw_fence_wait_update_v2(kgsl_msm_hw_fence.handle, &input_fence->fence,
			&handle, NULL, 1, true);
	if (ret)
		dev_err_ratelimited(device->dev,
			"Failed to import external fence ctx:%llu ts:%llu ret:%d\n",
			ret, input_fence->fence->context, input_fence->fence->seqno);
	else
		input_fence->handle = handle;


	return ret;
}

bool kgsl_hw_fence_tx_slot_available(struct kgsl_device *device, u32 pending_hw_fence_count)
{
	void *ptr = kgsl_msm_hw_fence.mem_descriptor.virtual_addr;
	struct msm_hw_fence_hfi_queue_header *hdr = (struct msm_hw_fence_hfi_queue_header *)
		(ptr + sizeof(struct msm_hw_fence_hfi_queue_table_header));
	u32 queue_size_dwords = hdr->queue_size / sizeof(u32);
	u32 payload_size_dwords = hdr->pkt_size / sizeof(u32);
	u32 free_entries, write_idx = hdr->write_index, read_idx = hdr->read_index;

	free_entries = read_idx <= write_idx ?
		queue_size_dwords - (write_idx - read_idx) :
		read_idx - write_idx;

	free_entries /= payload_size_dwords;

	/* Ensure free_entries does not become negative */
	if (free_entries == 0)
		return false;

	free_entries -= 1;

	if (free_entries > pending_hw_fence_count)
		return true;

	return false;
}

static void _hw_fence_destroy(struct kgsl_sync_fence *kfence)
{
	msm_hw_fence_destroy(kgsl_msm_hw_fence.handle, &kfence->fence);
}

#define IPCC_GPU_PHYS_ID 4
void kgsl_hw_fence_trigger_cpu(struct kgsl_device *device, struct kgsl_sync_fence *kfence)
{
	int ret = msm_hw_fence_update_txq(kgsl_msm_hw_fence.handle, kfence->hw_fence_handle,
				0, 0);
	if (ret) {
		dev_err_ratelimited(device->dev,
			"Failed to trigger hw fence via cpu: ctx:%d ts:%d ret:%d\n",
			kfence->context_id, kfence->timestamp, ret);
		return;
	}

	msm_hw_fence_trigger_signal(kgsl_msm_hw_fence.handle, IPCC_GPU_PHYS_ID,
		IPCC_CLIENT_APSS, 0);
}

bool kgsl_hw_fence_signaled(struct dma_fence *fence)
{
	return test_bit(MSM_HW_FENCE_FLAG_SIGNALED_BIT, &fence->flags);
}

static bool kgsl_is_input_hw_fence(struct dma_fence *fence)
{
	return test_bit(MSM_HW_FENCE_FLAG_ENABLED_BIT, &fence->flags);
}

#else
static void _hw_fence_destroy(struct kgsl_sync_fence *kfence)
{
}

#endif

static struct kgsl_sync_fence *kgsl_sync_fence_create(
					struct kgsl_context *context,
					unsigned int timestamp)
{
	struct kgsl_sync_fence *kfence;
	struct kgsl_sync_timeline *ktimeline = context->ktimeline;
	unsigned long flags;

	/* Get a refcount to the timeline. Put when released */
	if (!kref_get_unless_zero(&ktimeline->kref))
		return NULL;

	kfence = kzalloc(sizeof(*kfence), GFP_KERNEL);
	if (kfence == NULL) {
		kgsl_sync_timeline_put(ktimeline);
		return NULL;
	}

	kfence->parent = ktimeline;
	kfence->context_id = context->id;
	kfence->timestamp = timestamp;

	dma_fence_init(&kfence->fence, &kgsl_sync_fence_ops, &ktimeline->lock,
		ktimeline->fence_context, timestamp);

	/*
	 * sync_file_create() takes a refcount to the fence. This refcount is
	 * put when the fence is signaled.
	 */
	kfence->sync_file = sync_file_create(&kfence->fence);

	if (kfence->sync_file == NULL) {
		kgsl_sync_timeline_put(ktimeline);
		dev_err(context->device->dev, "Create sync_file failed\n");
		kfree(kfence);
		return NULL;
	}

	INIT_LIST_HEAD(&kfence->hw_fence_list);

	spin_lock_irqsave(&ktimeline->lock, flags);
	list_add_tail(&kfence->child_list, &ktimeline->child_list_head);
	spin_unlock_irqrestore(&ktimeline->lock, flags);

	return kfence;
}

static void kgsl_sync_fence_release(struct dma_fence *fence)
{
	struct kgsl_sync_fence *kfence = (struct kgsl_sync_fence *)fence;

	kgsl_sync_timeline_put(kfence->parent);
	kfree(kfence);
}

/* Called with ktimeline->lock held */
static bool kgsl_sync_fence_has_signaled(struct dma_fence *fence)
{
	struct kgsl_sync_fence *kfence = (struct kgsl_sync_fence *)fence;
	struct kgsl_sync_timeline *ktimeline = kfence->parent;
	unsigned int ts = kfence->timestamp;

	return (timestamp_cmp(ktimeline->last_timestamp, ts) >= 0);
}

static bool kgsl_enable_signaling(struct dma_fence *fence)
{
	return !kgsl_sync_fence_has_signaled(fence);
}

struct kgsl_sync_fence_event_priv {
	struct kgsl_context *context;
	unsigned int timestamp;
};

/**
 * kgsl_sync_fence_event_cb - Event callback for a fence timestamp event
 * @device - The KGSL device that expired the timestamp
 * @context- Pointer to the context that owns the event
 * @priv: Private data for the callback
 * @result - Result of the event (retired or canceled)
 *
 * Signal a fence following the expiration of a timestamp
 */

static void kgsl_sync_fence_event_cb(struct kgsl_device *device,
		struct kgsl_event_group *group, void *priv, int result)
{
	struct kgsl_sync_fence_event_priv *ev = priv;

	kgsl_sync_timeline_signal(ev->context->ktimeline, ev->timestamp);
	kgsl_context_put(ev->context);
	kfree(ev);
}

static int _add_fence_event(struct kgsl_device *device,
	struct kgsl_context *context, unsigned int timestamp)
{
	struct kgsl_sync_fence_event_priv *event;
	int ret;

	event = kmalloc(sizeof(*event), GFP_KERNEL);
	if (event == NULL)
		return -ENOMEM;

	/*
	 * Increase the refcount for the context to keep it through the
	 * callback
	 */
	if (!_kgsl_context_get(context)) {
		kfree(event);
		return -ENOENT;
	}

	event->context = context;
	event->timestamp = timestamp;

	ret = kgsl_add_event(device, &context->events, timestamp,
		kgsl_sync_fence_event_cb, event);

	if (ret) {
		kgsl_context_put(context);
		kfree(event);
	}

	return ret;
}

/* Only to be used if creating a related event failed */
static void kgsl_sync_cancel(struct kgsl_sync_fence *kfence)
{
	spin_lock(&kfence->parent->lock);
	if (!list_empty(&kfence->child_list)) {
		list_del_init(&kfence->child_list);
		dma_fence_put(&kfence->fence);
	}
	spin_unlock(&kfence->parent->lock);
}

/**
 * kgsl_add_fence_event - Create a new fence event
 * @device - KGSL device to create the event on
 * @timestamp - Timestamp to trigger the event
 * @data - Return fence fd stored in struct kgsl_timestamp_event_fence
 * @len - length of the fence event
 * @owner - driver instance that owns this event
 * @returns 0 on success or error code on error
 *
 * Create a fence and register an event to signal the fence when
 * the timestamp expires
 */

int kgsl_add_fence_event(struct kgsl_device *device,
	u32 context_id, u32 timestamp, void __user *data, int len,
	struct kgsl_device_private *owner)
{
	struct kgsl_timestamp_event_fence priv;
	struct kgsl_context *context;
	struct kgsl_sync_fence *kfence = NULL;
	int ret = -EINVAL;
	unsigned int cur;
	bool retired = false;

	priv.fence_fd = -1;

	if (len != sizeof(priv))
		return -EINVAL;

	context = kgsl_context_get_owner(owner, context_id);

	if (context == NULL)
		return -EINVAL;

	if (test_bit(KGSL_CONTEXT_PRIV_INVALID, &context->priv))
		goto out;

	kfence = kgsl_sync_fence_create(context, timestamp);
	if (kfence == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	priv.fence_fd = get_unused_fd_flags(0);
	if (priv.fence_fd < 0) {
		dev_crit_ratelimited(device->dev,
					"Unable to get a file descriptor: %d\n",
					priv.fence_fd);
		ret = priv.fence_fd;
		goto out;
	}

	/*
	 * If the timestamp hasn't expired yet create an event to trigger it.
	 * Otherwise, just signal the fence - there is no reason to go through
	 * the effort of creating a fence we don't need.
	 */

	kgsl_readtimestamp(device, context, KGSL_TIMESTAMP_RETIRED, &cur);

	retired = timestamp_cmp(cur, timestamp) >= 0;

	if (retired) {
		ret = 0;
		kgsl_sync_timeline_signal(context->ktimeline, cur);
	} else {
		ret = _add_fence_event(device, context, timestamp);
		if (ret)
			goto out;
	}

	if (copy_to_user(data, &priv, sizeof(priv))) {
		ret = -EFAULT;
		goto out;
	}

	device->ftbl->setup_fence(device, kfence);

	fd_install(priv.fence_fd, kfence->sync_file->file);

out:
	kgsl_context_put(context);
	if (ret) {
		if (priv.fence_fd >= 0)
			put_unused_fd(priv.fence_fd);

		if (kfence) {
			kgsl_sync_cancel(kfence);
			/*
			 * Put the refcount of sync file. This will release
			 * kfence->fence as well.
			 */
			fput(kfence->sync_file->file);
		}
	}
	return ret;
}

void kgsl_sync_timeline_value_str(struct dma_fence *fence, char *str, int size)
{
	struct kgsl_sync_fence *kfence = (struct kgsl_sync_fence *)fence;
	struct kgsl_sync_timeline *ktimeline = kfence->parent;
	struct kgsl_context *context = NULL;
	unsigned long flags;
	int ret = 0;

	unsigned int timestamp_retired;
	unsigned int timestamp_queued;

	if (fence->ops != &kgsl_sync_fence_ops)
		return;

	if (!kref_get_unless_zero(&ktimeline->kref))
		return;
	if (!ktimeline->device)
		goto put_timeline;

	spin_lock_irqsave(&ktimeline->lock, flags);
	ret = _kgsl_context_get(ktimeline->context);
	context = ret ? ktimeline->context : NULL;
	spin_unlock_irqrestore(&ktimeline->lock, flags);

	/* Get the last signaled timestamp if the context is not valid */
	timestamp_queued = ktimeline->last_timestamp;
	timestamp_retired = timestamp_queued;
	if (context) {
		kgsl_readtimestamp(ktimeline->device, context,
			KGSL_TIMESTAMP_RETIRED, &timestamp_retired);

		kgsl_readtimestamp(ktimeline->device, context,
			KGSL_TIMESTAMP_QUEUED, &timestamp_queued);

		kgsl_context_put(context);
	}

	snprintf(str, size, "%u queued:%u retired:%u",
		ktimeline->last_timestamp,
		timestamp_queued, timestamp_retired);

put_timeline:
	kgsl_sync_timeline_put(ktimeline);
}

static void kgsl_sync_fence_value_str(struct dma_fence *fence,
					char *str, int size)
{
	struct kgsl_sync_fence *kfence = (struct kgsl_sync_fence *)fence;

	snprintf(str, size, "%u", kfence->timestamp);
}

static const char *kgsl_sync_fence_driver_name(struct dma_fence *fence)
{
	return "kgsl-timeline";
}

static const char *kgsl_sync_timeline_name(struct dma_fence *fence)
{
	struct kgsl_sync_fence *kfence = (struct kgsl_sync_fence *)fence;
	struct kgsl_sync_timeline *ktimeline = kfence->parent;

	return ktimeline->name;
}

int kgsl_sync_timeline_create(struct kgsl_context *context)
{
	struct kgsl_sync_timeline *ktimeline;

	/* Put context at detach time */
	if (!_kgsl_context_get(context))
		return -ENOENT;

	ktimeline = kzalloc(sizeof(*ktimeline), GFP_KERNEL);
	if (ktimeline == NULL) {
		kgsl_context_put(context);
		return -ENOMEM;
	}

	kref_init(&ktimeline->kref);
	snprintf(ktimeline->name, sizeof(ktimeline->name),
		"%s_%u-%.15s(%d)-%.15s(%d)",
		context->device->name, context->id,
		current->group_leader->comm, current->group_leader->pid,
		current->comm, current->pid);

	ktimeline->fence_context = dma_fence_context_alloc(1);
	ktimeline->last_timestamp = 0;
	INIT_LIST_HEAD(&ktimeline->child_list_head);
	spin_lock_init(&ktimeline->lock);
	ktimeline->device = context->device;

	/*
	 * The context pointer is valid till detach time, where we put the
	 * refcount on the context
	 */
	ktimeline->context = context;

	context->ktimeline = ktimeline;

	return 0;
}

void kgsl_sync_timeline_signal(struct kgsl_sync_timeline *ktimeline,
					unsigned int timestamp)
{
	unsigned long flags;
	struct kgsl_sync_fence *kfence, *next;
	struct list_head hw_fence_put_list;

	INIT_LIST_HEAD(&hw_fence_put_list);

	if (!kref_get_unless_zero(&ktimeline->kref))
		return;

	spin_lock_irqsave(&ktimeline->lock, flags);
	if (timestamp_cmp(timestamp, ktimeline->last_timestamp) > 0)
		ktimeline->last_timestamp = timestamp;

	list_for_each_entry_safe(kfence, next, &ktimeline->child_list_head,
				child_list) {
		if (dma_fence_is_signaled_locked(&kfence->fence)) {
			kfence->signaled = ktime_get();
			/* Now that the fence is signaled, cancel the timer if needed */
			if (__test_and_clear_bit(KGSL_FENCE_FLAG_TIMER_TRIGGERED, &kfence->flags))
				hrtimer_cancel(&kfence->deadline_timer);

			if (__test_and_clear_bit(KGSL_FENCE_FLAG_SIGNAL_REFCOUNT, &kfence->flags)) {
				list_move_tail(&kfence->child_list, &hw_fence_put_list);
			} else {
				list_del_init(&kfence->child_list);
				dma_fence_put(&kfence->fence);
			}
		}
	}

	spin_unlock_irqrestore(&ktimeline->lock, flags);

	/*
	 * kgsl_hw_fence_put() should not be invoked while holding the ktimeline lock
	 * (aka the fence lock) as hw fence destroy APIs may also need to lock and unlock
	 * the same lock.
	 */
	list_for_each_entry_safe(kfence, next, &hw_fence_put_list, child_list) {
		list_del_init(&kfence->child_list);
		kgsl_hw_fence_put(kfence);
		dma_fence_put(&kfence->fence);
	}

	kgsl_sync_timeline_put(ktimeline);
}

void kgsl_sync_timeline_detach(struct kgsl_sync_timeline *ktimeline)
{
	unsigned long flags;
	struct kgsl_context *context = ktimeline->context;

	/* Set context pointer to NULL and drop our refcount on the context */
	spin_lock_irqsave(&ktimeline->lock, flags);
	ktimeline->context = NULL;
	spin_unlock_irqrestore(&ktimeline->lock, flags);
	kgsl_context_put(context);
}

static void kgsl_sync_timeline_destroy(struct kref *kref)
{
	struct kgsl_sync_timeline *ktimeline =
		container_of(kref, struct kgsl_sync_timeline, kref);

	kfree(ktimeline);
}

void kgsl_sync_timeline_put(struct kgsl_sync_timeline *ktimeline)
{
	if (ktimeline)
		kref_put(&ktimeline->kref, kgsl_sync_timeline_destroy);
}

#if (KERNEL_VERSION(6, 4, 0) <= LINUX_VERSION_CODE)
#define BOOT_COMPLETE_COOLDOWN_WINDOW_US    16000

static void kgsl_handle_missed_deadline(struct kgsl_sync_fence *kfence, ktime_t now)
{
	struct kgsl_sync_timeline *ktimeline = kfence->parent;
	struct kgsl_device *device = ktimeline->device;

	trace_kgsl_fence_deadline_info(kfence->context_id, kfence->timestamp, 0, 0, 0);

	/* Account for bootup latency and ignore notification within short period */
	if (!device->bootcomplete_ktime ||
		ktime_us_delta(now, device->bootcomplete_ktime) < BOOT_COMPLETE_COOLDOWN_WINDOW_US)
		return;

	dma_fence_get(&kfence->fence);

	/* In case the work was not queued as it was already pending, put the fence back */
	if (!kthread_queue_work(device->deadline_worker, &kfence->deadline_work))
		dma_fence_put(&kfence->fence);
}

static void kgsl_sync_fence_process_deadline(struct kgsl_sync_fence *kfence, ktime_t deadline)
{
	struct kgsl_sync_timeline *ktimeline = kfence->parent;
	struct kgsl_device *device = ktimeline->device;
	ktime_t signaled, now;
	unsigned long flags;

	spin_lock_irqsave(&ktimeline->lock, flags);

	/* Ignore repeated deadlines for the same fence */
	if (kfence->deadline) {
		spin_unlock_irqrestore(&ktimeline->lock, flags);
		return;
	}

	/* Update the fence */
	signaled = kfence->signaled;
	kfence->deadline = deadline;
	now = ktime_get();

	trace_kgsl_fence_deadline_info(kfence->context_id, kfence->timestamp, deadline,
			(s64)(ktime_to_us(deadline) - ktime_to_us(now)), kfence->signaled);

	/*
	 * Schedule work to notify missed deadline if the fence is signaled after the deadline, or
	 * if the fence is unsignaled even after the deadline.
	 */
	if ((signaled && ktime_before(deadline, signaled)) ||
			(!signaled && ktime_before(deadline, now))) {
		bool ret;

		/* Get a reference to the fence and release it when scheduled work is done */
		dma_fence_get(&kfence->fence);
		ret = kthread_queue_work(device->deadline_worker, &kfence->deadline_work);
		spin_unlock_irqrestore(&ktimeline->lock, flags);
		/* In case the work was not queued as it was already pending, put the fence back */
		if (!ret)
			dma_fence_put(&kfence->fence);
		return;
	}

	/* Check unsignaled fences for missed deadlines */
	if (!signaled && ktime_after(deadline, now)) {
		hrtimer_start(&kfence->deadline_timer, deadline, HRTIMER_MODE_ABS);
		__set_bit(KGSL_FENCE_FLAG_TIMER_TRIGGERED, &kfence->flags);
	}

	spin_unlock_irqrestore(&ktimeline->lock, flags);
}

static void kgsl_sync_fence_set_deadline(struct dma_fence *fence, ktime_t deadline)
{
	struct kgsl_sync_fence *kfence = (struct kgsl_sync_fence *)fence;
	struct kgsl_sync_timeline *ktimeline = kfence->parent;
	struct kgsl_device *device = ktimeline->device;
	ktime_t now = ktime_get();

	/* Return early is the context is bad */
	if (kgsl_context_is_bad(ktimeline->context))
		return;

	/* If missed deadline detection and response is not supported, return early */
	if ((!device->fence_deadline_boost) || (device->host_based_dcvs) ||
			(IS_ERR_OR_NULL(device->deadline_worker)))
		return;

	if (!deadline) {
		/* A deadline of 0 represents an exclusion of a draw for a context from a frame */
		kgsl_handle_missed_deadline(kfence, now);
		return;
	}

	kgsl_sync_fence_process_deadline(kfence, deadline);
}
#endif

const struct dma_fence_ops kgsl_sync_fence_ops = {
	.get_driver_name = kgsl_sync_fence_driver_name,
	.get_timeline_name = kgsl_sync_timeline_name,
	.enable_signaling = kgsl_enable_signaling,
	.signaled = kgsl_sync_fence_has_signaled,
	.wait = dma_fence_default_wait,
	.release = kgsl_sync_fence_release,

#if (KERNEL_VERSION(6, 16, 0) > LINUX_VERSION_CODE)
	.fence_value_str = kgsl_sync_fence_value_str,
	.timeline_value_str = kgsl_sync_timeline_value_str,
#endif

#if (KERNEL_VERSION(6, 4, 0) <= LINUX_VERSION_CODE)
	.set_deadline = kgsl_sync_fence_set_deadline,
#endif
};

static void kgsl_sync_fence_callback(struct dma_fence *fence,
					 struct dma_fence_cb *cb)
{
	struct kgsl_sync_fence_cb *kcb = (struct kgsl_sync_fence_cb *)cb;

	kcb->func(kcb->priv);
}

bool is_kgsl_fence(struct dma_fence *f)
{
	if (f->ops == &kgsl_sync_fence_ops)
		return true;

	return false;
}

static void kgsl_hw_fence_destroy(struct kref *kref)
{
	struct kgsl_sync_fence *kfence = container_of(kref, struct kgsl_sync_fence,
		hw_refcount);

	spin_lock(&hw_fence_list_lock);

	if (!list_empty(&kfence->hw_fence_list)) {
		_hw_fence_destroy(kfence);
		list_del_init(&kfence->hw_fence_list);
		/*
		 * Put back the dma fence refcount which was incremented when this hardware fence
		 * was created and added to the hw_fence_list
		 */
		dma_fence_put(&kfence->fence);
	}

	spin_unlock(&hw_fence_list_lock);
}

void kgsl_hw_fence_put(struct kgsl_sync_fence *kfence)
{
	if (kfence)
		kref_put(&kfence->hw_refcount, kgsl_hw_fence_destroy);
}

static void _kgsl_populate_hw_fence(struct kgsl_drawobj_sync_event *event, struct dma_fence *fence)
{
	struct kgsl_drawobj_sync *syncobj = event->syncobj;
	struct kgsl_drawobj_sync_input_fence *input_fence;
	struct kgsl_device *device = event->device;
	u32 max_hw_fence = device->max_syncobj_hw_fence_count;

	if (test_bit(KGSL_SYNCOBJ_SW, &syncobj->flags))
		return;

	if (!kgsl_is_input_hw_fence(fence)) {
		/*
		 * Ignore software fences that are already signaled. Even one unsignaled sw-only
		 * fence in this sync object means we can't send this sync object to the hardware
		 */
		if (!dma_fence_is_signaled(fence))
			set_bit(KGSL_SYNCOBJ_SW, &syncobj->flags);
		return;
	}

	if (syncobj->num_hw_fence >= max_hw_fence) {
		set_bit(KGSL_SYNCOBJ_SW, &syncobj->flags);
		return;
	}

	input_fence = kmem_cache_zalloc(device->syncobj_hw_fence_cache, GFP_KERNEL);
	if (!input_fence) {
		set_bit(KGSL_SYNCOBJ_SW, &syncobj->flags);
		return;
	}

	syncobj->num_hw_fence++;
	input_fence->fence = fence;
	input_fence->handle = 0;
	INIT_LIST_HEAD(&input_fence->node);
	list_add_tail(&input_fence->node, &syncobj->hw_fence_list);

	/* Make sure all output fences for this process are hybrid fences */
	if (kgsl_is_synx_native_fence(fence))
		syncobj->base.context->proc_priv->hybrid_output_fence = true;
}

static void kgsl_syncsource_fence_value_str(struct dma_fence *fence, char *str, int size)
{
	/*
	 * Each fence is independent of the others on the same timeline.
	 * We use a different context for each of them.
	 */
	snprintf(str, size, "%llu", fence->context);
}

#if (KERNEL_VERSION(6, 16, 0) > LINUX_VERSION_CODE)
static int kgsl_get_fence_value_str(struct dma_fence *fence, char *buf, size_t size)
{
	int len = 0;

	if (!fence || !fence->ops || !buf)
		return 0;

	if (fence->ops->fence_value_str) {
		len += scnprintf(buf + len, size - len, ": ");
		fence->ops->fence_value_str(fence, buf + len, size - len);
	}

	return len;
}
#else
static int kgsl_get_fence_value_str(struct dma_fence *fence, char *buf, size_t size)
{
	int len = 0;

	if (!fence || !fence->ops || !buf)
		return 0;

	if (fence->ops == &kgsl_sync_fence_ops) {
		len += scnprintf(buf + len, size - len, ": ");
		kgsl_sync_fence_value_str(fence, buf + len, size - len);
	}

	if (fence->ops == &kgsl_syncsource_fence_ops) {
		len += scnprintf(buf + len, size - len, ": ");
		kgsl_syncsource_fence_value_str(fence, buf + len, size - len);
	}

	return len;
}
#endif

void kgsl_get_fence_name(struct dma_fence *f, char *name, u32 max_size)
{
	int len = scnprintf(name, max_size, "%s %s", f->ops->get_driver_name(f),
			f->ops->get_timeline_name(f));

	len += kgsl_get_fence_value_str(f, name + len, max_size - len);

}

void kgsl_populate_hw_fences(struct kgsl_drawobj_sync_event *event)
{
	unsigned int num_fences;
	struct dma_fence **fences;
	struct dma_fence_array *array;
	int i;

	array = to_dma_fence_array(event->fence);
	if (array != NULL) {
		num_fences = array->num_fences;
		fences = array->fences;
	} else {
		num_fences = 1;
		fences = &event->fence;
	}

	for (i = 0; i < num_fences; i++)
		_kgsl_populate_hw_fence(event, fences[i]);
}

void kgsl_get_fence_info(struct kgsl_drawobj_sync_event *event)
{
	unsigned int num_fences;
	struct dma_fence *fence, **fences;
	struct dma_fence_array *array;
	struct event_fence_info *info_ptr = event->priv;
	int i;

	if (!info_ptr)
		return;

	fence = event->handle->fence;

	array = to_dma_fence_array(fence);
	if (array != NULL) {
		num_fences = array->num_fences;
		fences = array->fences;
	} else {
		num_fences = 1;
		fences = &fence;
	}

	info_ptr->fences = kcalloc(num_fences, sizeof(struct fence_info),
			GFP_KERNEL);
	if (info_ptr->fences == NULL)
		return;

	info_ptr->num_fences = num_fences;

	for (i = 0; i < num_fences; i++) {
		struct dma_fence *f = fences[i];
		struct fence_info *fi = &info_ptr->fences[i];

		kgsl_get_fence_name(f, fi->name, sizeof(fi->name));
	}
}

struct dma_fence *kgsl_sync_file_get_fence(int fd)
{
	return sync_file_get_fence(fd);
}

struct kgsl_sync_fence_cb *kgsl_sync_fence_async_wait_fence(struct dma_fence *fence,
	bool (*func)(void *priv), void *priv)
{
	struct kgsl_sync_fence_cb *kcb;
	int status;

	/* create the callback */
	kcb = kzalloc(sizeof(*kcb), GFP_KERNEL);
	if (kcb == NULL) {
		return ERR_PTR(-ENOMEM);
	}

	kcb->fence = fence;
	kcb->priv = priv;
	kcb->func = func;

	/* if status then error or signaled */
	status = dma_fence_add_callback(fence, &kcb->fence_cb,
				kgsl_sync_fence_callback);

	if (status) {
		kfree(kcb);
		if (!dma_fence_is_signaled(fence))
			kcb = ERR_PTR(status);
		else
			kcb = NULL;
	}

	return kcb;
}

struct kgsl_sync_fence_cb *kgsl_sync_fence_async_wait(int fd,
	bool (*func)(void *priv), void *priv)
{
	struct kgsl_sync_fence_cb *kcb;
	struct dma_fence *fence = sync_file_get_fence(fd);

	if (fence == NULL)
		return ERR_PTR(-EINVAL);

	kcb = kgsl_sync_fence_async_wait_fence(fence, func, priv);
	if (IS_ERR_OR_NULL(kcb))
		dma_fence_put(fence);

	return kcb;
}

/*
 * Cancel the fence async callback.
 */
void kgsl_sync_fence_async_cancel(struct kgsl_sync_fence_cb *kcb)
{
	if (kcb == NULL)
		return;

	dma_fence_remove_callback(kcb->fence, &kcb->fence_cb);
}

struct kgsl_syncsource {
	struct kref refcount;
	char name[32];
	int id;
	struct kgsl_process_private *private;
	struct list_head child_list_head;
	spinlock_t lock;
};

struct kgsl_syncsource_fence {
	struct dma_fence fence;
	struct kgsl_syncsource *parent;
	struct list_head child_list;
};

long kgsl_ioctl_syncsource_create(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data)
{
	struct kgsl_syncsource *syncsource = NULL;
	struct kgsl_syncsource_create *param = data;
	int ret = -EINVAL;
	int id = 0;
	struct kgsl_process_private *private = dev_priv->process_priv;

	if (!kgsl_process_private_get(private))
		return ret;

	syncsource = kzalloc(sizeof(*syncsource), GFP_KERNEL);
	if (syncsource == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	kref_init(&syncsource->refcount);
	snprintf(syncsource->name, sizeof(syncsource->name),
		"kgsl-syncsource-pid-%d", current->group_leader->pid);
	syncsource->private = private;
	INIT_LIST_HEAD(&syncsource->child_list_head);
	spin_lock_init(&syncsource->lock);

	idr_preload(GFP_KERNEL);
	spin_lock(&private->syncsource_lock);
	id = idr_alloc(&private->syncsource_idr, syncsource, 1, 0, GFP_NOWAIT);
	if (id > 0) {
		syncsource->id = id;
		param->id = id;
		ret = 0;
	} else {
		ret = id;
	}

	spin_unlock(&private->syncsource_lock);
	idr_preload_end();

out:
	if (ret) {
		kgsl_process_private_put(private);
		kfree(syncsource);
	}

	return ret;
}

static struct kgsl_syncsource *
kgsl_syncsource_get(struct kgsl_process_private *private, int id)
{
	int result = 0;
	struct kgsl_syncsource *syncsource = NULL;

	spin_lock(&private->syncsource_lock);

	syncsource = idr_find(&private->syncsource_idr, id);
	if (syncsource)
		result = kref_get_unless_zero(&syncsource->refcount);

	spin_unlock(&private->syncsource_lock);

	return result ? syncsource : NULL;
}

static void kgsl_syncsource_destroy(struct kref *kref)
{
	struct kgsl_syncsource *syncsource = container_of(kref,
						struct kgsl_syncsource,
						refcount);

	struct kgsl_process_private *private = syncsource->private;

	/* Done with process private. Release the refcount */
	kgsl_process_private_put(private);

	kfree(syncsource);
}

void kgsl_syncsource_put(struct kgsl_syncsource *syncsource)
{
	if (syncsource)
		kref_put(&syncsource->refcount, kgsl_syncsource_destroy);
}

static void kgsl_syncsource_cleanup(struct kgsl_process_private *private,
				struct kgsl_syncsource *syncsource)
{
	struct kgsl_syncsource_fence *sfence, *next;
	unsigned long flags;

	/* Signal all fences to release any callbacks */
	spin_lock_irqsave(&syncsource->lock, flags);

	list_for_each_entry_safe(sfence, next, &syncsource->child_list_head,
				child_list) {
		dma_fence_signal_locked(&sfence->fence);
		list_del_init(&sfence->child_list);
	}

	spin_unlock_irqrestore(&syncsource->lock, flags);

	/* put reference from syncsource creation */
	kgsl_syncsource_put(syncsource);
}

long kgsl_ioctl_syncsource_destroy(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data)
{
	struct kgsl_syncsource_destroy *param = data;
	struct kgsl_syncsource *syncsource = NULL;
	struct kgsl_process_private *private = dev_priv->process_priv;

	spin_lock(&private->syncsource_lock);
	syncsource = idr_find(&private->syncsource_idr, param->id);

	if (syncsource == NULL) {
		spin_unlock(&private->syncsource_lock);
		return -EINVAL;
	}

	if (syncsource->id != 0) {
		idr_remove(&private->syncsource_idr, syncsource->id);
		syncsource->id = 0;
	}
	spin_unlock(&private->syncsource_lock);

	kgsl_syncsource_cleanup(private, syncsource);
	return 0;
}

long kgsl_ioctl_syncsource_create_fence(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data)
{
	struct kgsl_syncsource_create_fence *param = data;
	struct kgsl_syncsource *syncsource = NULL;
	int ret = -EINVAL;
	struct kgsl_syncsource_fence *sfence = NULL;
	struct sync_file *sync_file = NULL;
	int fd = -1;
	unsigned long flags;

	/*
	 * Take a refcount that is released when the fence is released
	 * (or if fence can't be added to the syncsource).
	 */
	syncsource = kgsl_syncsource_get(dev_priv->process_priv,
					param->id);
	if (syncsource == NULL)
		goto out;

	sfence = kzalloc(sizeof(*sfence), GFP_KERNEL);
	if (sfence == NULL) {
		ret = -ENOMEM;
		goto out;
	}
	sfence->parent = syncsource;

	/* Use a new fence context for each fence */
	dma_fence_init(&sfence->fence, &kgsl_syncsource_fence_ops,
		&syncsource->lock, dma_fence_context_alloc(1), 1);

	sync_file = sync_file_create(&sfence->fence);

	if (sync_file == NULL) {
		dev_err(dev_priv->device->dev,
			     "Create sync_file failed\n");
		ret = -ENOMEM;
		goto out;
	}

	fd = get_unused_fd_flags(0);
	if (fd < 0) {
		ret = -EBADF;
		goto out;
	}
	ret = 0;

	fd_install(fd, sync_file->file);

	param->fence_fd = fd;

	spin_lock_irqsave(&syncsource->lock, flags);
	list_add_tail(&sfence->child_list, &syncsource->child_list_head);
	spin_unlock_irqrestore(&syncsource->lock, flags);
out:
	/*
	 * We're transferring ownership of the fence to the sync file.
	 * The sync file takes an extra refcount when it is created, so put
	 * our refcount.
	 */
	if (sync_file)
		dma_fence_put(&sfence->fence);

	if (ret) {
		if (sync_file)
			fput(sync_file->file);
		else if (sfence)
			dma_fence_put(&sfence->fence);
		else
			kgsl_syncsource_put(syncsource);
	}

	return ret;
}

static int kgsl_syncsource_signal(struct kgsl_syncsource *syncsource,
					struct dma_fence *fence)
{
	struct kgsl_syncsource_fence *sfence, *next;
	int ret = -EINVAL;
	unsigned long flags;

	spin_lock_irqsave(&syncsource->lock, flags);

	list_for_each_entry_safe(sfence, next, &syncsource->child_list_head,
				child_list) {
		if (fence == &sfence->fence) {
			dma_fence_signal_locked(fence);
			list_del_init(&sfence->child_list);

			ret = 0;
			break;
		}
	}

	spin_unlock_irqrestore(&syncsource->lock, flags);

	return ret;
}

long kgsl_ioctl_syncsource_signal_fence(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data)
{
	int ret = -EINVAL;
	struct kgsl_syncsource_signal_fence *param = data;
	struct kgsl_syncsource *syncsource = NULL;
	struct dma_fence *fence = NULL;

	syncsource = kgsl_syncsource_get(dev_priv->process_priv,
					param->id);
	if (syncsource == NULL)
		goto out;

	fence = sync_file_get_fence(param->fence_fd);
	if (fence == NULL) {
		ret = -EBADF;
		goto out;
	}

	ret = kgsl_syncsource_signal(syncsource, fence);
out:
	if (fence)
		dma_fence_put(fence);
	if (syncsource)
		kgsl_syncsource_put(syncsource);
	return ret;
}

static void kgsl_syncsource_fence_release(struct dma_fence *fence)
{
	struct kgsl_syncsource_fence *sfence =
			(struct kgsl_syncsource_fence *)fence;

	/* Signal if it's not signaled yet */
	kgsl_syncsource_signal(sfence->parent, fence);

	/* Release the refcount on the syncsource */
	kgsl_syncsource_put(sfence->parent);

	kfree(sfence);
}

void kgsl_syncsource_process_release_syncsources(
		struct kgsl_process_private *private)
{
	struct kgsl_syncsource *syncsource;
	int next = 0;

	while (1) {
		spin_lock(&private->syncsource_lock);
		syncsource = idr_get_next(&private->syncsource_idr, &next);

		if (syncsource == NULL) {
			spin_unlock(&private->syncsource_lock);
			break;
		}

		if (syncsource->id != 0) {
			idr_remove(&private->syncsource_idr, syncsource->id);
			syncsource->id = 0;
		}
		spin_unlock(&private->syncsource_lock);

		kgsl_syncsource_cleanup(private, syncsource);
		next = next + 1;
	}
}

static const char *kgsl_syncsource_get_timeline_name(struct dma_fence *fence)
{
	struct kgsl_syncsource_fence *sfence =
			(struct kgsl_syncsource_fence *)fence;
	struct kgsl_syncsource *syncsource = sfence->parent;

	return syncsource->name;
}

static bool kgsl_syncsource_enable_signaling(struct dma_fence *fence)
{
	return true;
}

static const char *kgsl_syncsource_driver_name(struct dma_fence *fence)
{
	return "kgsl-syncsource-timeline";
}

static const struct dma_fence_ops kgsl_syncsource_fence_ops = {
	.get_driver_name = kgsl_syncsource_driver_name,
	.get_timeline_name = kgsl_syncsource_get_timeline_name,
	.enable_signaling = kgsl_syncsource_enable_signaling,
	.wait = dma_fence_default_wait,
	.release = kgsl_syncsource_fence_release,

#if (KERNEL_VERSION(6, 16, 0) > LINUX_VERSION_CODE)
	.fence_value_str = kgsl_syncsource_fence_value_str,
#endif
};

