// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2002,2008-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/debugfs.h>

#include "adreno.h"
#include "kgsl_pwrscale.h"

extern struct dentry *kgsl_debugfs_dir;

static void set_isdb(struct adreno_device *adreno_dev, void *priv)
{
	set_bit(ADRENO_DEVICE_ISDB_ENABLED, &adreno_dev->priv);
}

static int _isdb_set(void *data, u64 val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	/* Once ISDB goes enabled it stays enabled */
	if (test_bit(ADRENO_DEVICE_ISDB_ENABLED, &adreno_dev->priv))
		return 0;

	/*
	 * Bring down the GPU so we can bring it back up with the correct power
	 * and clock settings
	 */
	return  adreno_power_cycle(adreno_dev, set_isdb, NULL);
}

static int _isdb_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	*val = (u64) test_bit(ADRENO_DEVICE_ISDB_ENABLED, &adreno_dev->priv);
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(_isdb_fops, _isdb_get, _isdb_set, "%llu\n");

static int _ctxt_record_size_set(void *data, u64 val)
{
	struct kgsl_device *device = data;

	device->snapshot_ctxt_record_size = val;

	return 0;
}

static int _ctxt_record_size_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;

	*val = device->snapshot_ctxt_record_size;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(_ctxt_record_size_fops, _ctxt_record_size_get,
		_ctxt_record_size_set, "%llu\n");

static int _lm_limit_set(void *data, u64 val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	if (!ADRENO_FEATURE(adreno_dev, ADRENO_LM))
		return 0;

	/* assure value is between 3A and 10A */
	if (val > 10000)
		val = 10000;
	else if (val < 3000)
		val = 3000;

	if (adreno_dev->lm_enabled)
		return adreno_power_cycle_u32(adreno_dev,
			&adreno_dev->lm_limit, val);

	return 0;
}

static int _lm_limit_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	if (!ADRENO_FEATURE(adreno_dev, ADRENO_LM))
		*val = 0;

	*val = (u64) adreno_dev->lm_limit;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(_lm_limit_fops, _lm_limit_get,
		_lm_limit_set, "%llu\n");

static int _lm_threshold_count_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	if (!ADRENO_FEATURE(adreno_dev, ADRENO_LM))
		*val = 0;
	else
		*val = (u64) adreno_dev->lm_threshold_cross;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(_lm_threshold_fops, _lm_threshold_count_get,
	NULL, "%llu\n");

static int _active_count_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	unsigned int i = atomic_read(&device->active_cnt);

	*val = (u64) i;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(_active_count_fops, _active_count_get, NULL, "%llu\n");

static int _coop_reset_set(void *data, u64 val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	if (ADRENO_FEATURE(adreno_dev, ADRENO_COOP_RESET))
		adreno_dev->cooperative_reset = val ? true : false;
	return 0;
}

static int _coop_reset_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	*val = (u64) adreno_dev->cooperative_reset;
	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(_coop_reset_fops, _coop_reset_get,
				_coop_reset_set, "%llu\n");

static void set_gpu_client_pf(struct adreno_device *adreno_dev, void *priv)
{
	adreno_dev->uche_client_pf = *((u32 *)priv);
	adreno_dev->patch_reglist = false;
}

static int _gpu_client_pf_set(void *data, u64 val)
{
	struct kgsl_device *device = data;

	return adreno_power_cycle(ADRENO_DEVICE(device), set_gpu_client_pf, &val);
}

static int _gpu_client_pf_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	*val = (u64) adreno_dev->uche_client_pf;
	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(_gpu_client_pf_fops, _gpu_client_pf_get,
				_gpu_client_pf_set, "%llu\n");

static int _prealloc_atomic_snap_mem_set(void *data, u64 val)
{
	struct kgsl_device *device = data;

	kgsl_mutex_lock(&device->mutex);

	/* Allocate atomic snapshot memory if it's not allocated yet */
	if (!val || device->snapshot_memory_atomic.ptr) {
		kgsl_mutex_unlock(&device->mutex);
		return 0;
	}

	device->snapshot_memory_atomic.size = device->snapshot_memory.size;

	/* Ensure size is visible to other threads before setting address */
	smp_wmb();

	device->snapshot_memory_atomic.ptr = dma_alloc_coherent(&device->pdev->dev,
		device->snapshot_memory_atomic.size, &device->snapshot_memory_atomic.dma_handle,
		GFP_KERNEL);

	if (WARN_ON((!device->snapshot_memory_atomic.ptr))) {
		/* Fallback to slab allocator if DMA allocation fails */
		device->snapshot_memory_atomic.size = (SZ_2M + SZ_1M);

		/* Ensure size is visible to other threads before setting address */
		smp_wmb();

		device->snapshot_memory_atomic.ptr = devm_kzalloc(&device->pdev->dev,
			device->snapshot_memory_atomic.size, GFP_KERNEL);
	}

	if (!device->snapshot_memory_atomic.ptr) {
		kgsl_mutex_unlock(&device->mutex);
		dev_err(device->dev, "Failed to allocate memory for atomic snapshot\n");
		return -ENOMEM;
	}

	kgsl_mutex_unlock(&device->mutex);

	return 0;
}

static int _prealloc_atomic_snap_mem_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;

	*val = device->snapshot_memory_atomic.ptr ? 1 : 0;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(_prealloc_atomic_snapshot_mem_fops, _prealloc_atomic_snap_mem_get,
				_prealloc_atomic_snap_mem_set, "%llu\n");

typedef void (*reg_read_init_t)(struct kgsl_device *device);
typedef void (*reg_read_fill_t)(struct kgsl_device *device, int i,
	unsigned int *vals, int linec);


static void sync_event_print(struct seq_file *s,
		struct kgsl_drawobj_sync_event *sync_event)
{
	switch (sync_event->type) {
	case KGSL_CMD_SYNCPOINT_TYPE_TIMESTAMP: {
		seq_printf(s, "sync: ctx: %u ts: %u",
				sync_event->context->id, sync_event->timestamp);
		break;
	}
	case KGSL_CMD_SYNCPOINT_TYPE_FENCE: {
		int i;
		struct event_fence_info *info = sync_event->priv;

		for (i = 0; info && i < info->num_fences; i++)
			seq_printf(s, "sync: %s",
				info->fences[i].name);
		break;
	}
	case KGSL_CMD_SYNCPOINT_TYPE_TIMELINE: {
		int j;
		struct event_timeline_info *info = sync_event->priv;

		for (j = 0; info && info[j].timeline; j++)
			seq_printf(s, "timeline: %d seqno: %lld",
				info[j].timeline, info[j].seqno);
		break;
	}
	default:
		seq_printf(s, "sync: type: %d", sync_event->type);
		break;
	}
}

struct flag_entry {
	unsigned long mask;
	const char *str;
};

static void _print_flags(struct seq_file *s, const struct flag_entry *table,
			unsigned long flags)
{
	int i;
	int first = 1;

	for (i = 0; table[i].str; i++) {
		if (flags & table[i].mask) {
			seq_printf(s, "%c%s", first ? '\0' : '|', table[i].str);
			flags &= ~(table[i].mask);
			first = 0;
		}
	}
	if (flags) {
		seq_printf(s, "%c0x%lx", first ? '\0' : '|', flags);
		first = 0;
	}
	if (first)
		seq_puts(s, "None");
}

#define print_flags(_s, _flag, _array...)		\
	({						\
		const struct flag_entry symbols[] =   \
			{ _array, { -1, NULL } };	\
		_print_flags(_s, symbols, _flag);	\
	 })

static void syncobj_print(struct seq_file *s,
			struct kgsl_drawobj_sync *syncobj)
{
	struct kgsl_drawobj_sync_event *event;
	unsigned int i;

	seq_puts(s, " syncobj ");

	for (i = 0; i < syncobj->numsyncs; i++) {
		event = &syncobj->synclist[i];

		if (!kgsl_drawobj_event_pending(syncobj, i))
			continue;

		sync_event_print(s, event);
		seq_puts(s, "\n");
	}
}

static void cmdobj_print(struct seq_file *s,
			struct kgsl_drawobj_cmd *cmdobj)
{
	struct kgsl_drawobj *drawobj = DRAWOBJ(cmdobj);

	if (drawobj->type == CMDOBJ_TYPE)
		seq_puts(s, " cmdobj ");
	else
		seq_puts(s, " markerobj ");

	seq_printf(s, "\t %u ", drawobj->timestamp);

	seq_puts(s, " priv: ");
	print_flags(s, cmdobj->priv,
		{ BIT(CMDOBJ_SKIP), "skip"},
		{ BIT(CMDOBJ_FORCE_PREAMBLE), "force_preamble"},
		{ BIT(CMDOBJ_WFI), "wait_for_idle" });
}

static void drawobj_print(struct seq_file *s,
			struct kgsl_drawobj *drawobj)
{
	if (!kref_get_unless_zero(&drawobj->refcount))
		return;

	if (drawobj->type == SYNCOBJ_TYPE)
		syncobj_print(s, SYNCOBJ(drawobj));
	else if ((drawobj->type == CMDOBJ_TYPE) ||
			(drawobj->type == MARKEROBJ_TYPE))
		cmdobj_print(s, CMDOBJ(drawobj));

	seq_puts(s, " flags: ");
	print_flags(s, drawobj->flags, KGSL_DRAWOBJ_FLAGS);
	kgsl_drawobj_put(drawobj);
	seq_puts(s, "\n");
}

static int ctx_print(struct seq_file *s, void *unused)
{
	struct adreno_context *drawctxt = s->private;
	unsigned int i;
	struct kgsl_event *event;
	unsigned int queued = 0, consumed = 0, retired = 0;

	seq_printf(s, "id: %u type: %s priority: %d process: %s (%d) tid: %d\n",
		   drawctxt->base.id,
		   kgsl_context_type(drawctxt->type),
		   drawctxt->base.priority,
		   drawctxt->base.proc_priv->comm,
		   pid_nr(drawctxt->base.proc_priv->pid),
		   drawctxt->base.tid);

	seq_puts(s, "flags: ");
	print_flags(s, drawctxt->base.flags & ~(KGSL_CONTEXT_PRIORITY_MASK
		| KGSL_CONTEXT_TYPE_MASK), KGSL_CONTEXT_FLAGS);
	seq_puts(s, " priv: ");
	print_flags(s, drawctxt->base.priv,
		{ BIT(KGSL_CONTEXT_PRIV_SUBMITTED), "submitted"},
		{ BIT(KGSL_CONTEXT_PRIV_DETACHED), "detached"},
		{ BIT(KGSL_CONTEXT_PRIV_INVALID), "invalid"},
		{ BIT(KGSL_CONTEXT_PRIV_PAGEFAULT), "pagefault"},
		{ BIT(ADRENO_CONTEXT_FAULT), "fault"},
		{ BIT(ADRENO_CONTEXT_GPU_HANG), "gpu_hang"},
		{ BIT(ADRENO_CONTEXT_GPU_HANG_FT), "gpu_hang_ft"},
		{ BIT(ADRENO_CONTEXT_SKIP_EOF), "skip_end_of_frame" },
		{ BIT(ADRENO_CONTEXT_FORCE_PREAMBLE), "force_preamble"});
	seq_puts(s, "\n");

	seq_puts(s, "timestamps: ");
	kgsl_readtimestamp(drawctxt->base.device, &drawctxt->base,
				KGSL_TIMESTAMP_QUEUED, &queued);
	kgsl_readtimestamp(drawctxt->base.device, &drawctxt->base,
				KGSL_TIMESTAMP_CONSUMED, &consumed);
	kgsl_readtimestamp(drawctxt->base.device, &drawctxt->base,
				KGSL_TIMESTAMP_RETIRED, &retired);
	seq_printf(s, "queued: %u consumed: %u retired: %u global:%u\n",
		   queued, consumed, retired,
		   drawctxt->internal_timestamp);

	seq_puts(s, "drawqueue:\n");

	spin_lock(&drawctxt->lock);
	for (i = drawctxt->drawqueue_head;
		i != drawctxt->drawqueue_tail;
		i = DRAWQUEUE_NEXT(i, ADRENO_CONTEXT_DRAWQUEUE_SIZE))
		drawobj_print(s, drawctxt->drawqueue[i]);
	spin_unlock(&drawctxt->lock);

	seq_puts(s, "events:\n");
	spin_lock(&drawctxt->base.events.lock);
	list_for_each_entry(event, &drawctxt->base.events.events, node)
		seq_printf(s, "\t%d: %pS created: %u\n", event->timestamp,
				event->func, event->created);
	spin_unlock(&drawctxt->base.events.lock);

	return 0;
}

static int ctx_open(struct inode *inode, struct file *file)
{
	int ret;
	struct adreno_context *ctx = inode->i_private;

	if (!_kgsl_context_get(&ctx->base))
		return -ENODEV;

	ret = single_open(file, ctx_print, &ctx->base);
	if (ret)
		kgsl_context_put(&ctx->base);
	return ret;
}

static int ctx_release(struct inode *inode, struct file *file)
{
	struct kgsl_context *context;

	context = ((struct seq_file *)file->private_data)->private;

	kgsl_context_put(context);

	return single_release(inode, file);
}

static const struct file_operations ctx_fops = {
	.open = ctx_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = ctx_release,
};


void
adreno_context_debugfs_init(struct adreno_device *adreno_dev,
			    struct adreno_context *ctx)
{
	unsigned char name[16];

	/*
	 * Get the context here to make sure it still exists for the life of the
	 * file
	 */
	_kgsl_context_get(&ctx->base);

	snprintf(name, sizeof(name), "%d", ctx->base.id);

	ctx->debug_root = debugfs_create_file(name, 0444,
				adreno_dev->ctx_d_debugfs, ctx, &ctx_fops);
}

static int _bcl_sid0_set(void *data, u64 val)
{
	struct kgsl_device *device = data;
	const struct gmu_dev_ops *ops = GMU_DEVICE_OPS(device);

	if (ops && ops->bcl_sid_set)
		return ops->bcl_sid_set(device, 0, val);

	return 0;
}

static int _bcl_sid0_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	const struct gmu_dev_ops *ops = GMU_DEVICE_OPS(device);

	if (ops && ops->bcl_sid_get)
		*val = ops->bcl_sid_get(device, 0);

	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(_sid0_fops, _bcl_sid0_get, _bcl_sid0_set, "%llu\n");

static int _bcl_sid1_set(void *data, u64 val)
{
	struct kgsl_device *device = data;
	const struct gmu_dev_ops *ops = GMU_DEVICE_OPS(device);

	if (ops && ops->bcl_sid_set)
		return ops->bcl_sid_set(device, 1, val);

	return 0;
}

static int _bcl_sid1_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	const struct gmu_dev_ops *ops = GMU_DEVICE_OPS(device);

	if (ops && ops->bcl_sid_get)
		*val = ops->bcl_sid_get(device, 1);

	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(_sid1_fops, _bcl_sid1_get, _bcl_sid1_set, "%llu\n");

static int _bcl_sid2_set(void *data, u64 val)
{
	struct kgsl_device *device = data;
	const struct gmu_dev_ops *ops = GMU_DEVICE_OPS(device);

	if (ops && ops->bcl_sid_set)
		return ops->bcl_sid_set(device, 2, val);

	return 0;
}

static int _bcl_sid2_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	const struct gmu_dev_ops *ops = GMU_DEVICE_OPS(device);

	if (ops && ops->bcl_sid_get)
		*val = ops->bcl_sid_get(device, 2);

	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(_sid2_fops, _bcl_sid2_get, _bcl_sid2_set, "%llu\n");

static int _bcl_throttle_time_us_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	const struct adreno_gpudev *gpudev  = ADRENO_GPU_DEVICE(adreno_dev);

	if (gpudev->power_feature_stats)
		gpudev->power_feature_stats(adreno_dev);

	if (!ADRENO_FEATURE(adreno_dev, ADRENO_BCL))
		*val = 0;
	else
		*val = (u64) adreno_dev->bcl_throttle_time_us;

	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(_bcl_throttle_fops, _bcl_throttle_time_us_get, NULL, "%llu\n");

static int _skipsaverestore_store(void *data, u64 val)
{
	struct adreno_device *adreno_dev = data;

	if (adreno_dev->hwsched_enabled)
		return adreno_power_cycle_bool(adreno_dev,
			&adreno_dev->preempt.skipsaverestore, val);

	adreno_dev->preempt.skipsaverestore = val ? true : false;
	return 0;

}

static int _skipsaverestore_show(void *data, u64 *val)
{
	struct adreno_device *adreno_dev = data;

	*val = (u64) adreno_dev->preempt.skipsaverestore;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(skipsaverestore_fops, _skipsaverestore_show, _skipsaverestore_store,
	"%llu\n");

static int _usesgmem_store(void *data, u64 val)
{
	struct adreno_device *adreno_dev = data;

	if (adreno_dev->hwsched_enabled)
		return adreno_power_cycle_bool(adreno_dev,
			&adreno_dev->preempt.usesgmem, val);

	adreno_dev->preempt.usesgmem = val ? true : false;
	return 0;

}

static int _usesgmem_show(void *data, u64 *val)
{
	struct adreno_device *adreno_dev = data;

	*val = (u64) adreno_dev->preempt.usesgmem;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(usesgmem_fops, _usesgmem_show, _usesgmem_store, "%llu\n");

static int _preempt_level_store(void *data, u64 val)
{
	struct adreno_device *adreno_dev = data;

	if (adreno_dev->hwsched_enabled)
		return adreno_power_cycle_u32(adreno_dev,
			&adreno_dev->preempt.preempt_level,
			min_t(u64, val, 2));

	adreno_dev->preempt.preempt_level = min_t(u64, val, 2);
	return 0;

}

static int _preempt_level_show(void *data, u64 *val)
{
	struct adreno_device *adreno_dev = data;

	*val = (u64) adreno_dev->preempt.preempt_level;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(preempt_level_fops, _preempt_level_show, _preempt_level_store, "%llu\n");

static int sp_profiling_en_show(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_sp_profiling *sp_profiling = &adreno_dev->sp_profiling;

	*val = (u64)sp_profiling->enabled;
	return 0;
}

static int sp_profiling_en_store(void *data, u64 val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_sp_profiling *sp_profiling = &adreno_dev->sp_profiling;

	if (val == sp_profiling->enabled)
		return 0;

	return adreno_power_cycle_bool(adreno_dev, &sp_profiling->enabled, val);
}

DEFINE_DEBUGFS_ATTRIBUTE(sp_profiling_en_fops, sp_profiling_en_show, sp_profiling_en_store,
				"%llu\n");

static int sp_profiling_buf_sz_show(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_sp_profiling *sp_profiling = &adreno_dev->sp_profiling;

	*val = (u64)sp_profiling->buf_sz_bytes;
	return 0;
}

static int sp_profiling_buf_sz_store(void *data, u64 val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_sp_profiling *sp_profiling = &adreno_dev->sp_profiling;

	/* Can only be updated when SP profiling is disabled */
	if (sp_profiling->enabled)
		return -EINVAL;

	/*
	 * DBGC_GBIF_DBG_BUFF_SIZE requires the buffer to be sized in 16-byte chunks.
	 * However, in practice, it's useful to dump larger buffers. As such, use page
	 * alignment to get a reasonably-sized buffer.
	 */
	val = ALIGN(val, PAGE_SIZE);

	/* Limit the buffer size to prevent it from using too much global memory */
	if (!val || val > SZ_128M)
		return -EINVAL;

	sp_profiling->buf_sz_bytes = (u32)val;

	/* Make sure other CPUs can see the updated setting */
	smp_wmb();

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(sp_profiling_buf_sz_fops, sp_profiling_buf_sz_show,
				sp_profiling_buf_sz_store, "0x%08llx\n");

static int sp_profiling_bus_sel_a_show(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_sp_profiling *sp_profiling = &adreno_dev->sp_profiling;

	*val = (u64)sp_profiling->bus_sel_a;
	return 0;
}

static int sp_profiling_bus_sel_a_store(void *data, u64 val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_sp_profiling *sp_profiling = &adreno_dev->sp_profiling;

	/* Can only be updated when SP profiling is disabled */
	if (sp_profiling->enabled)
		return -EINVAL;

	sp_profiling->bus_sel_a = (u32)val;

	/* Make sure other CPUs can see the updated setting */
	smp_wmb();

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(sp_profiling_bus_sel_a_fops, sp_profiling_bus_sel_a_show,
				sp_profiling_bus_sel_a_store, "0x%08llx\n");

static int sp_profiling_bus_sel_b_show(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_sp_profiling *sp_profiling = &adreno_dev->sp_profiling;

	*val = (u64)sp_profiling->bus_sel_b;
	return 0;
}

static int sp_profiling_bus_sel_b_store(void *data, u64 val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_sp_profiling *sp_profiling = &adreno_dev->sp_profiling;

	/* Can only be updated when SP profiling is disabled */
	if (sp_profiling->enabled)
		return -EINVAL;

	sp_profiling->bus_sel_b = (u32)val;

	/* Make sure other CPUs can see the updated setting */
	smp_wmb();

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(sp_profiling_bus_sel_b_fops, sp_profiling_bus_sel_b_show,
				sp_profiling_bus_sel_b_store, "0x%08llx\n");

static int sp_profiling_bus_sel_c_show(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_sp_profiling *sp_profiling = &adreno_dev->sp_profiling;

	*val = (u64)sp_profiling->bus_sel_c;
	return 0;
}

static int sp_profiling_bus_sel_c_store(void *data, u64 val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_sp_profiling *sp_profiling = &adreno_dev->sp_profiling;

	/* Can only be updated when SP profiling is disabled */
	if (sp_profiling->enabled)
		return -EINVAL;

	sp_profiling->bus_sel_c = (u32)val;

	/* Make sure other CPUs can see the updated setting */
	smp_wmb();

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(sp_profiling_bus_sel_c_fops, sp_profiling_bus_sel_c_show,
				sp_profiling_bus_sel_c_store, "0x%08llx\n");

static int sp_profiling_bus_sel_d_show(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_sp_profiling *sp_profiling = &adreno_dev->sp_profiling;

	*val = (u64)sp_profiling->bus_sel_d;
	return 0;
}

static int sp_profiling_bus_sel_d_store(void *data, u64 val)
{
	struct kgsl_device *device = data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_sp_profiling *sp_profiling = &adreno_dev->sp_profiling;

	/* Can only be updated when SP profiling is disabled */
	if (sp_profiling->enabled)
		return -EINVAL;

	sp_profiling->bus_sel_d = (u32)val;

	/* Make sure other CPUs can see the updated setting */
	smp_wmb();

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(sp_profiling_bus_sel_d_fops, sp_profiling_bus_sel_d_show,
				sp_profiling_bus_sel_d_store, "0x%08llx\n");

static ssize_t sp_profiling_buf_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
	struct kgsl_device *device = (struct kgsl_device *)file->private_data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_sp_profiling *sp_profiling = &adreno_dev->sp_profiling;
	ssize_t ret;

	kgsl_mutex_lock(&device->mutex);
	if ((!sp_profiling->enabled) || (IS_ERR_OR_NULL(sp_profiling->dbg_buf))) {
		ret = -EINVAL;
		goto unlock;
	}

	ret = simple_read_from_buffer(buf, len, ppos, sp_profiling->dbg_buf->hostptr,
		sp_profiling->dbg_buf->size);

unlock:
	kgsl_mutex_unlock(&device->mutex);
	return ret;
}

static const struct file_operations sp_profiling_buf_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = sp_profiling_buf_read,
	.llseek = noop_llseek,
};

static void init_sp_profiling_config(struct adreno_device *adreno_dev)
{
	struct adreno_sp_profiling *sp_profiling = &adreno_dev->sp_profiling;

	sp_profiling->enabled = false;
	sp_profiling->active = false;
	sp_profiling->buf_sz_bytes = SZ_16M;
	sp_profiling->bus_sel_a = 0x00a60077;
	sp_profiling->bus_sel_b = 0x00a60077;
	sp_profiling->bus_sel_c = 0x00a60077;
	sp_profiling->bus_sel_d = 0x00a60077;
	sp_profiling->dbg_buf = NULL;
}

static int _warmboot_show(void *data, u64 *val)
{
	struct adreno_device *adreno_dev = data;
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct gmu_core_device *gmu = &device->gmu_core;

	*val = (u64)gmu->warmboot_enabled;
	return 0;
}

/*
 * When warmboot feature is enabled from debugfs, the first slumber exit will be a cold boot
 * and all hfi messages will be recorded, so that warmboot can happen on subsequent slumber
 * exit. When warmboot feature is disabled from debugfs, every slumber exit will be a coldboot.
 */
static int _warmboot_store(void *data, u64 val)
{
	struct adreno_device *adreno_dev = data;
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct gmu_core_device *gmu_core = &device->gmu_core;

	if (gmu_core->warmboot_enabled == val)
		return 0;

	return adreno_power_cycle_bool(adreno_dev, &gmu_core->warmboot_enabled, val);
}

DEFINE_DEBUGFS_ATTRIBUTE(warmboot_fops, _warmboot_show, _warmboot_store, "%llu\n");

static ssize_t gmu_proto_buf_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
	struct kgsl_device *device = (struct kgsl_device *)file->private_data;
	struct gmu_core_device *gmu = &device->gmu_core;
	struct kgsl_memdesc *md = READ_ONCE(gmu->pwr_proto_trace.md);
	void *hostptr = NULL;

	if (IS_ERR_OR_NULL(md))
		return -EINVAL;

	hostptr = READ_ONCE(md->hostptr);

	if (!hostptr)
		return -EINVAL;

	/* We do not need a synchronization/device mutex here since resizing is not allowed */
	return simple_read_from_buffer(buf, len, ppos, hostptr, md->size);
}

static const struct file_operations gmu_pwr_proto_trace_buf_out_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = gmu_proto_buf_read,
	.llseek = noop_llseek,
};

static ssize_t gmu_pwr_limits_buf_read(struct file *file, char __user *buf, size_t len,
					loff_t *ppos)
{
	struct kgsl_device *device = (struct kgsl_device *)file->private_data;
	struct gmu_core_device *gmu = &device->gmu_core;
	struct kgsl_memdesc *md = READ_ONCE(gmu->pwr_limits_trace.md);
	void *hostptr = NULL;

	if (IS_ERR_OR_NULL(md))
		return -EINVAL;

	hostptr = READ_ONCE(md->hostptr);

	if (!hostptr)
		return -EINVAL;

	/* We do not need a synchronization/device mutex here since resizing is not allowed */
	return simple_read_from_buffer(buf, len, ppos, hostptr, md->size);
}

static const struct file_operations gmu_pwr_limits_trace_buf_out_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = gmu_pwr_limits_buf_read,
	.llseek = noop_llseek,
};

static int _gmu_pwr_proto_trace_buf_size_show(void *data, u64 *val)
{
	struct adreno_device *adreno_dev = data;
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct gmu_core_device *gmu = &device->gmu_core;

	*val = ((u64)gmu->gmu_pwr_proto_trace_buf_size / SZ_1M);
	return 0;
}

static int _gmu_pwr_proto_trace_buf_size_store(void *data, u64 val)
{
	struct adreno_device *adreno_dev = data;
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct gmu_core_device *gmu = &device->gmu_core;

	/* Do not overwrite a value once buffer is allocated */
	if (gmu->gmu_pwr_proto_trace_buf_size)
		return 0;

	if (val > (SZ_256M / SZ_1M)) {
		dev_err_ratelimited(device->dev, "Max allocation allowed is 256MB\n");
		return 0;
	}

	return adreno_power_cycle_u32(adreno_dev, &gmu->gmu_pwr_proto_trace_buf_size,
			(val * SZ_1M));
}

DEFINE_DEBUGFS_ATTRIBUTE(gmu_pwr_proto_trace_buf_size_fops, _gmu_pwr_proto_trace_buf_size_show,
				_gmu_pwr_proto_trace_buf_size_store, "%llu\n");

static int _gmu_pwr_limits_trace_buf_size_show(void *data, u64 *val)
{
	struct adreno_device *adreno_dev = data;
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct gmu_core_device *gmu = &device->gmu_core;

	*val = ((u64)gmu->gmu_pwr_limits_trace_buf_size / SZ_1M);
	return 0;
}

static int _gmu_pwr_limits_trace_buf_size_store(void *data, u64 val)
{
	struct adreno_device *adreno_dev = data;
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct gmu_core_device *gmu = &device->gmu_core;

	/* Do not overwrite a value once buffer is allocated */
	if (gmu->gmu_pwr_limits_trace_buf_size)
		return 0;

	if (val > (SZ_256M / SZ_1M)) {
		dev_err(device->dev, "Max allocation allowed is 256MB\n");
		return 0;
	}

	return adreno_power_cycle_u32(adreno_dev, &gmu->gmu_pwr_limits_trace_buf_size,
			(val * SZ_1M));
}

DEFINE_DEBUGFS_ATTRIBUTE(gmu_pwr_limits_trace_buf_size_fops, _gmu_pwr_limits_trace_buf_size_show,
				_gmu_pwr_limits_trace_buf_size_store, "%llu\n");

static int _gmu_pwr_trace_trigger_get(void *data, u64 *val)
{
	struct adreno_device *adreno_dev = data;
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	const struct gmu_dev_ops *ops = GMU_DEVICE_OPS(device);

	if (ops && ops->gmu_pwr_trace_trigger_get)
		*val = ops->gmu_pwr_trace_trigger_get(device);

	return 0;
}

static int _gmu_pwr_trace_trigger_set(void *data, u64 val)
{
	struct adreno_device *adreno_dev = data;
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	const struct gmu_dev_ops *ops = GMU_DEVICE_OPS(device);

	if (ops && ops->gmu_pwr_trace_trigger_set)
		return ops->gmu_pwr_trace_trigger_set(device, (u32)val);

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(gmu_pwr_trace_trigger_fops, _gmu_pwr_trace_trigger_get,
				_gmu_pwr_trace_trigger_set, "0x%08llx\n");

static void spel_config_set(struct adreno_device *adreno_dev, void *priv)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct gmu_core_device *gmu = &device->gmu_core;
	struct kgsl_gmu_spel *spel = &gmu->spel;

	memcpy(spel->config, priv, GMU_PWR_BUDGET_DWORDS * sizeof(u32));
}

static ssize_t spel_config_store(struct file *filep,
		const char __user *user_buf, size_t len, loff_t *off)
{
	struct seq_file *seq = (struct seq_file *) filep->private_data;
	struct adreno_device *adreno_dev = seq->private;
	char *buf;
	ssize_t size;
	u32 user_config[GMU_PWR_BUDGET_DWORDS];
	int ret;

	if ((len >= PAGE_SIZE) || (len == 0))
		return -EINVAL;

	buf = kzalloc(len + 1, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	if (copy_from_user(buf, user_buf, len)) {
		ret = -EFAULT;
		goto out;
	}

	/* For sanity and parsing, ensure it is null terminated */
	buf[len] = '\0';

	size = sscanf(buf, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
		&user_config[0], &user_config[1], &user_config[2], &user_config[3],
		&user_config[4], &user_config[5], &user_config[6]);
	if (size != GMU_PWR_BUDGET_DWORDS) {
		ret = -EINVAL;
		goto out;
	}

	ret = adreno_power_cycle(adreno_dev, spel_config_set, user_config);
	if (!ret)
		ret = len;

out:
	kfree(buf);
	return ret;
}

static int spel_config_show(struct seq_file *s, void *unused)
{
	struct adreno_device *adreno_dev = s->private;
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct gmu_core_device *gmu = &device->gmu_core;
	struct kgsl_gmu_spel *spel = &gmu->spel;
	u32 raw;

	raw = spel->config[0];
	seq_printf(s, "RAW[0] = 0x%08x\n", raw);
	seq_printf(s, "    forced_telemetry_window   = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_FORCED_TELEMETRY_WINDOW, raw));
	seq_printf(s, "    min_perf_level            = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_MIN_PERF_LEVEL, raw));
	seq_printf(s, "    short_dce_margin_scale_en = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_SHORT_DCE_MARGIN_SCALE_EN, raw));
	seq_printf(s, "    local_pmic_loss_calc_en   = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_LOCAL_PMIC_LOSS_CALC_EN, raw));
	seq_printf(s, "    cdyn_hist_en              = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_CDYN_HIST_EN, raw));
	seq_printf(s, "    long_desired_pwr_en       = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_LONG_DESIRED_PWR_EN, raw));
	seq_printf(s, "    short_desired_pwr_en      = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_SHORT_DESIRED_PWR_EN, raw));
	seq_printf(s, "    telemetry_en              = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_TELEMETRY_EN, raw));
	seq_printf(s, "    long_pwr_budget_enforce   = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_LONG_PWR_BUDGET_ENFORCE, raw));
	seq_printf(s, "    long_en                   = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_LONG_EN, raw));
	seq_printf(s, "    short_en                  = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_SHORT_EN, raw));
	seq_printf(s, "    lkg_en                    = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_LKG_EN, raw));
	seq_printf(s, "    dyn_en                    = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_DYN_EN, raw));
	seq_puts(s, "\n");

	raw = spel->config[1];
	seq_printf(s, "RAW[1] = 0x%08x\n", raw);
	seq_printf(s, "    dce_cdyn_bx_scale         = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_DCE_CDYN_BX_SCALE, raw));
	seq_printf(s, "    dce_cdyn_mx_scale         = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_DCE_CDYN_MX_SCALE, raw));
	seq_printf(s, "    max_budget_cdyn_gfx_short = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_MAX_BUDGET_CDYN_GFX_SHORT, raw));
	seq_printf(s, "    max_budget_cdyn_gfx       = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_MAX_BUDGET_CDYN_GFX, raw));
	seq_printf(s, "    des_pwr_alpha             = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_DES_PWR_ALPHA, raw));
	seq_puts(s, "\n");

	raw = spel->config[2];
	seq_printf(s, "RAW[2] = 0x%08x\n", raw);
	seq_printf(s, "    reserved_2                = 0x%lx\n",
		FIELD_GET(GMU_PWR_BUDGET_RESERVED_2, raw));
	seq_printf(s, "    cdyn_accum_config_0       = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_CDYN_ACCUM_CONFIG_0, raw));
	seq_printf(s, "    cdyn_hist_alpha           = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_CDYN_HIST_ALPHA, raw));
	seq_printf(s, "    cdyn_scale_factor_en      = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_CDYN_SCALE_FACTOR_EN, raw));
	seq_printf(s, "    num_samples               = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_NUM_SAMPLES, raw));
	seq_puts(s, "\n");

	raw = spel->config[3];
	seq_printf(s, "RAW[3] = 0x%08x\n", raw);
	seq_printf(s, "    reserved_3                = 0x%lx\n",
		FIELD_GET(GMU_PWR_BUDGET_RESERVED_3, raw));
	seq_puts(s, "\n");

	raw = spel->config[4];
	seq_printf(s, "RAW[4] = 0x%08x\n", raw);
	seq_printf(s, "    forced_long_budget        = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_FORCED_LONG_BUDGET, raw));
	seq_puts(s, "\n");

	raw = spel->config[5];
	seq_printf(s, "RAW[5] = 0x%08x\n", raw);
	seq_printf(s, "    forced_long_time_const    = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_FORCED_LONG_TIME_CONST, raw));
	seq_puts(s, "\n");

	raw = spel->config[6];
	seq_printf(s, "RAW[6] = 0x%08x\n", raw);
	seq_printf(s, "    forced_short_budget       = %lu\n",
		FIELD_GET(GMU_PWR_BUDGET_FORCED_SHORT_BUDGET, raw));

	return 0;
}

static int spel_config_open(struct inode *inode, struct file *file)
{
	return single_open(file, spel_config_show, inode->i_private);
}

static const struct file_operations spel_config_fops = {
	.owner = THIS_MODULE,
	.open = spel_config_open,
	.read = seq_read,
	.write = spel_config_store,
	.llseek = seq_lseek,
	.release = single_release,
};

static int _ifpc_hyst_store(void *data, u64 val)
{
	struct adreno_device *adreno_dev = data;
	u32 hyst;

	if (!gmu_core_dev_ifpc_isenabled(KGSL_DEVICE(adreno_dev)))
		return -EINVAL;

	/* IFPC hysteresis timer is 16 bits */
	hyst = max_t(u32, (u32) (FIELD_GET(GENMASK(15, 0), val)),
		     adreno_dev->ifpc_hyst_floor);

	if (hyst == adreno_dev->ifpc_hyst)
		return 0;

	return adreno_power_cycle_u32(adreno_dev,
			&adreno_dev->ifpc_hyst, hyst);
}

static int _ifpc_hyst_show(void *data, u64 *val)
{
	struct adreno_device *adreno_dev = data;

	*val = (u64) adreno_dev->ifpc_hyst;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(ifpc_hyst_fops, _ifpc_hyst_show, _ifpc_hyst_store, "%llu\n");

static void set_minbw_data(struct adreno_device *adreno_dev, void *priv)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	const struct gmu_dev_ops *ops = GMU_DEVICE_OPS(device);

	if (ops && ops->minbw_idle_level_set)
		ops->minbw_idle_level_set(device, *((u32 *)priv));
}

static int _minbw_data_store(void *data, u64 val)
{
	struct adreno_device *adreno_dev = data;
	u32 minbw_val;

	/* Only 24 bits are allowed by GMU for this feature */
	if (val & 0xffffffffff000000)
		return -EINVAL;

	/* We cannot use minBW if IFPC or minBW is disabled */
	if (!ADRENO_FEATURE(adreno_dev, ADRENO_IFPC) ||
		(!ADRENO_FEATURE(adreno_dev, ADRENO_GMU_MINBW)))
		return 0;

	minbw_val = (u32)val;

	return adreno_power_cycle(adreno_dev, set_minbw_data, &minbw_val);
}

static int _minbw_data_show(void *data, u64 *val)
{
	struct adreno_device *adreno_dev = data;

	*val = (u64)adreno_dev->minbw_data;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(minbw_fops, _minbw_data_show, _minbw_data_store, "%llu\n");

static int _gmu_fp_store(void *data, u64 val)
{
	struct adreno_device *adreno_dev = data;
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);

	/* Max allowed GMU fault settings are 9 bits */
	val = FIELD_GET(GENMASK(GMU_FAULT_MAX, 0), val);

	if (val == device->gmu_core.gf_panic)
		return 0;

	kgsl_mutex_lock(&device->mutex);
	device->gmu_core.gf_panic = val;
	kgsl_mutex_unlock(&device->mutex);

	return 0;
}

static int _gmu_fp_show(void *data, u64 *val)
{
	struct adreno_device *adreno_dev = data;

	*val = (u64) KGSL_DEVICE(adreno_dev)->gmu_core.gf_panic;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(gmu_fp_fops, _gmu_fp_show, _gmu_fp_store, "%llu\n");

static int _gmuclk_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	int cur_freq = gmu_core_get_active_frequency(device);

	if (cur_freq < 0)
		return -EINVAL;

	*val = (u64) cur_freq;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(_gmuclk_fops, _gmuclk_get, NULL, "%llu\n");

static int _num_pwrlevels_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	int num_pwrlevels = gmu_core_get_num_pwrlevels(device);

	if (num_pwrlevels < 0)
		return -EINVAL;

	*val = (u64) num_pwrlevels;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(_num_pwrlevels_fops, _num_pwrlevels_get, NULL, "%llu\n");

static int _min_pwrlevel_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	int min_pwrlevel = gmu_core_get_min_pwrlevel(device);

	if (min_pwrlevel < 0)
		return -EINVAL;

	*val = (u64) min_pwrlevel;
	return 0;
}

static int _min_pwrlevel_set(void *data, u64 val)
{
	struct kgsl_device *device = data;

	return gmu_core_set_min_pwrlevel(device, val);
}

DEFINE_DEBUGFS_ATTRIBUTE(_min_pwrlevel_fops, _min_pwrlevel_get, _min_pwrlevel_set, "%llu\n");

static int _max_pwrlevel_get(void *data, u64 *val)
{
	struct kgsl_device *device = data;
	int max_pwrlevel = gmu_core_get_max_pwrlevel(device);

	if (max_pwrlevel < 0)
		return -EINVAL;

	*val = (u64) max_pwrlevel;
	return 0;
}

static int _max_pwrlevel_set(void *data, u64 val)
{
	struct kgsl_device *device = data;

	return gmu_core_set_max_pwrlevel(device, val);
}

DEFINE_DEBUGFS_ATTRIBUTE(_max_pwrlevel_fops, _max_pwrlevel_get, _max_pwrlevel_set, "%llu\n");

static int gmu_available_frequencies_show(struct seq_file *s, void *unused)
{
	struct kgsl_device *device = s->private;

	return gmu_core_list_frequencies(device, s);
}

DEFINE_SHOW_ATTRIBUTE(gmu_available_frequencies);

static void _toggle_host_based_dcvs(struct adreno_device *adreno_dev, void *priv)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	bool val = *((bool *)priv);

	if (val) {
		/* Enable host based DCVS */
		device->pwrscale.devfreq_enabled = true;
		device->pwrctrl.bus_control = true;
		kgsl_pwrscale_close(device);
		device->host_based_dcvs = val;
		kgsl_pwrscale_init(device, device->pdev);
		kgsl_pwrscale_tz_enable(device);
		adreno_dev->dcvs_profile_enabled = false;
	} else {
		/* Disable host based DCVS */
		kgsl_pwrscale_tz_disable(device, false);
		kgsl_pwrscale_close(device);
		device->host_based_dcvs = val;
		kgsl_pwrscale_init(device, device->pdev);
		device->pwrscale.devfreq_enabled = false;
		device->pwrctrl.bus_control = false;
		if (ADRENO_FEATURE(adreno_dev, ADRENO_DCVS_PROFILE))
			adreno_dev->dcvs_profile_enabled = true;
	}
}

static int _host_based_dcvs_show(void *data, u64 *val)
{
	struct adreno_device *adreno_dev = data;
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);

	*val = (u64)device->host_based_dcvs;
	return 0;
}

static int _host_based_dcvs_store(void *data, u64 val)
{
	struct adreno_device *adreno_dev = data;
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	bool host_based_dcvs;

	if ((val == device->host_based_dcvs) || (val > 1))
		return 0;

	host_based_dcvs = (bool)val;
	return adreno_power_cycle(adreno_dev, _toggle_host_based_dcvs, &host_based_dcvs);
}

DEFINE_DEBUGFS_ATTRIBUTE(host_based_dcvs_fops, _host_based_dcvs_show,
				_host_based_dcvs_store, "%llu\n");

static int _gpu_voltage_show(struct seq_file *s, void *unused)
{
	struct kgsl_device *device = s->private;
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	int i;

	for (i = 0; i < pwr->num_pwrlevels; i++)
		seq_printf(s, "pwrlevel %d: voltage %u\n", i,
			pwr->pwrlevels[i].voltage_level);

	return 0;
}

static int _gpu_voltage_open(struct inode *inode, struct file *file)
{
	return single_open(file, _gpu_voltage_show, inode->i_private);
}

static ssize_t _gpu_voltage_write(struct file *file, const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct kgsl_device *device = s->private;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	u32 pwrlevel, voltage_level;
	int ret, i;
	char buf[32];

	if (!adreno_is_gen8(adreno_dev))
		return -EINVAL;

	if (count >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	buf[count] = 0;

	ret = sscanf(buf, "%u %u", &pwrlevel, &voltage_level);
	if (ret != 2)
		return -EINVAL;

	if (pwrlevel >= pwr->num_pwrlevels)
		return -EINVAL;

	/* Clamp voltage_level to supported range of gpu frequency */
	if (voltage_level > pwr->pwrlevels[0].voltage_level)
		voltage_level = pwr->pwrlevels[0].voltage_level;
	else if (voltage_level < pwr->pwrlevels[pwr->num_pwrlevels - 1].voltage_level)
		voltage_level = pwr->pwrlevels[pwr->num_pwrlevels - 1].voltage_level;
	else {
		/*
		 * Find the first existing voltage_level level that is greater than or
		 * equal to the requested voltage_level.
		 */
		for (i = pwr->num_pwrlevels - 1; i >= 0; i--) {
			if (voltage_level <= pwr->pwrlevels[i].voltage_level) {
				voltage_level = pwr->pwrlevels[i].voltage_level;
				break;
			}
		}
	}

	kgsl_mutex_lock(&device->mutex);
	pwr->pwrlevels[pwrlevel].voltage_level = voltage_level;
	pwr->update_dcvs_table = true;
	gmu_core_mark_for_coldboot(device);
	kgsl_mutex_unlock(&device->mutex);
	return count;
}

static const struct file_operations gpu_voltage_fops = {
	.owner = THIS_MODULE,
	.open = _gpu_voltage_open,
	.read = seq_read,
	.write = _gpu_voltage_write,
	.llseek = seq_lseek,
	.release = single_release,
};

void adreno_debugfs_init(struct adreno_device *adreno_dev)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct dentry *snapshot_dir;

	if (IS_ERR_OR_NULL(device->d_debugfs))
		return;

	debugfs_create_file("active_cnt", 0444, device->d_debugfs, device,
			    &_active_count_fops);
	adreno_dev->ctx_d_debugfs = debugfs_create_dir("ctx",
							device->d_debugfs);
	snapshot_dir = debugfs_lookup("snapshot", kgsl_debugfs_dir);

	if (!IS_ERR_OR_NULL(snapshot_dir))
		debugfs_create_file("coop_reset", 0644, snapshot_dir, device,
					&_coop_reset_fops);

	if (ADRENO_FEATURE(adreno_dev, ADRENO_LM)) {
		debugfs_create_file("lm_limit", 0644, device->d_debugfs, device,
			&_lm_limit_fops);
		debugfs_create_file("lm_threshold_count", 0444,
			device->d_debugfs, device, &_lm_threshold_fops);
	}

	if (adreno_is_a5xx(adreno_dev))
		debugfs_create_file("isdb", 0644, device->d_debugfs,
			device, &_isdb_fops);

	if (gmu_core_isenabled(device)) {
		debugfs_create_file("ifpc_hyst", 0644, device->d_debugfs,
			device, &ifpc_hyst_fops);

		debugfs_create_file("minbw", 0644, device->d_debugfs,
			device, &minbw_fops);

		debugfs_create_file("gmu_fault_policy", 0644, device->d_debugfs,
			device, &gmu_fp_fops);
	}

	if (ADRENO_FEATURE(adreno_dev, ADRENO_GMU_WARMBOOT))
		debugfs_create_file("warmboot", 0644, device->d_debugfs,
			device, &warmboot_fops);

	debugfs_create_file("ctxt_record_size", 0644, snapshot_dir,
		device, &_ctxt_record_size_fops);
	debugfs_create_file("gpu_client_pf", 0644, snapshot_dir,
		device, &_gpu_client_pf_fops);
	debugfs_create_bool("dump_all_ibs", 0644, snapshot_dir,
		&device->dump_all_ibs);
	debugfs_create_file("prealloc_atomic_snapshot_mem", 0644, snapshot_dir,
		device, &_prealloc_atomic_snapshot_mem_fops);

	adreno_dev->bcl_debugfs_dir = debugfs_create_dir("bcl", device->d_debugfs);
	if (!IS_ERR_OR_NULL(adreno_dev->bcl_debugfs_dir)) {
		debugfs_create_file("sid0", 0644, adreno_dev->bcl_debugfs_dir, device, &_sid0_fops);
		debugfs_create_file("sid1", 0644, adreno_dev->bcl_debugfs_dir, device, &_sid1_fops);
		debugfs_create_file("sid2", 0644, adreno_dev->bcl_debugfs_dir, device, &_sid2_fops);
		debugfs_create_file("bcl_throttle_time_us", 0444, adreno_dev->bcl_debugfs_dir,
						device, &_bcl_throttle_fops);
	}

	adreno_dev->preemption_debugfs_dir = debugfs_create_dir("preemption", device->d_debugfs);
	if (!IS_ERR_OR_NULL(adreno_dev->preemption_debugfs_dir)) {
		debugfs_create_file("preempt_level", 0644, adreno_dev->preemption_debugfs_dir,
			device, &preempt_level_fops);
		debugfs_create_file("usesgmem", 0644, adreno_dev->preemption_debugfs_dir, device,
			&usesgmem_fops);
		debugfs_create_file("skipsaverestore", 0644, adreno_dev->preemption_debugfs_dir,
			device, &skipsaverestore_fops);
	}

	init_sp_profiling_config(adreno_dev);
	adreno_dev->sp_profiling_dir = debugfs_create_dir("sp_profiling", device->d_debugfs);
	if (!IS_ERR_OR_NULL(adreno_dev->sp_profiling_dir)) {
		debugfs_create_file("enable", 0644, adreno_dev->sp_profiling_dir, device,
			&sp_profiling_en_fops);
		debugfs_create_file("buf_sz_bytes", 0644, adreno_dev->sp_profiling_dir, device,
			&sp_profiling_buf_sz_fops);
		debugfs_create_file("bus_sel_a", 0644, adreno_dev->sp_profiling_dir, device,
			&sp_profiling_bus_sel_a_fops);
		debugfs_create_file("bus_sel_b", 0644, adreno_dev->sp_profiling_dir, device,
			&sp_profiling_bus_sel_b_fops);
		debugfs_create_file("bus_sel_c", 0644, adreno_dev->sp_profiling_dir, device,
			&sp_profiling_bus_sel_c_fops);
		debugfs_create_file("bus_sel_d", 0644, adreno_dev->sp_profiling_dir, device,
			&sp_profiling_bus_sel_d_fops);
		debugfs_create_file("dbg_buf", 0444, adreno_dev->sp_profiling_dir, device,
			&sp_profiling_buf_fops);
	}

	device->gmu_core.gmu_debugfs_dir = debugfs_create_dir("gmu", device->d_debugfs);
	if (!IS_ERR_OR_NULL(device->gmu_core.gmu_debugfs_dir)) {
		debugfs_create_file("gmuclk", 0444, device->gmu_core.gmu_debugfs_dir, device,
			&_gmuclk_fops);
		debugfs_create_file("num_pwrlevels", 0444, device->gmu_core.gmu_debugfs_dir, device,
			&_num_pwrlevels_fops);
		debugfs_create_file("min_pwrlevel", 0644, device->gmu_core.gmu_debugfs_dir, device,
			&_min_pwrlevel_fops);
		debugfs_create_file("max_pwrlevel", 0644, device->gmu_core.gmu_debugfs_dir, device,
			&_max_pwrlevel_fops);
		debugfs_create_file("gmu_available_frequencies", 0444,
			device->gmu_core.gmu_debugfs_dir, device, &gmu_available_frequencies_fops);
		debugfs_create_file("gmu_pwr_proto_trace_buf_size", 0644,
			device->gmu_core.gmu_debugfs_dir, device,
			&gmu_pwr_proto_trace_buf_size_fops);
		debugfs_create_file("gmu_pwr_proto_trace_buf_out", 0444,
			device->gmu_core.gmu_debugfs_dir, device,
			&gmu_pwr_proto_trace_buf_out_fops);
		debugfs_create_file("gmu_pwr_limits_trace_buf_size", 0644,
			device->gmu_core.gmu_debugfs_dir, device,
			&gmu_pwr_limits_trace_buf_size_fops);
		debugfs_create_file("gmu_pwr_limits_trace_buf_out", 0444,
			device->gmu_core.gmu_debugfs_dir, device,
			&gmu_pwr_limits_trace_buf_out_fops);
		debugfs_create_file("gmu_pwr_trace_trigger", 0644, device->gmu_core.gmu_debugfs_dir,
			device, &gmu_pwr_trace_trigger_fops);

		if (ADRENO_FEATURE(adreno_dev, ADRENO_GMU_SPEL))
			debugfs_create_file("spel_config", 0644, device->gmu_core.gmu_debugfs_dir,
				device, &spel_config_fops);
	}

	if (ADRENO_FEATURE(adreno_dev, ADRENO_GMU_BASED_DCVS))
		debugfs_create_file("host_based_dcvs", 0644, device->d_debugfs,
				device, &host_based_dcvs_fops);

	debugfs_create_file("gpu_voltage", 0644, device->d_debugfs,
			device, &gpu_voltage_fops);
}
