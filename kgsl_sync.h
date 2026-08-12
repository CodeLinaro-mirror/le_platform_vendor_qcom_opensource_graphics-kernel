/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2012-2014,2018-2019, 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#ifndef __KGSL_SYNC_H
#define __KGSL_SYNC_H

#include <linux/dma-fence.h>

/**
 * struct kgsl_sync_timeline - A sync timeline associated with a kgsl context
 * @kref: Refcount to keep the struct alive until all its fences are signaled,
	  and as long as the context exists
 * @name: String to describe this timeline
 * @fence_context: Used by the fence driver to identify fences belonging to
 *		   this context
 * @child_list_head: List head for all fences on this timeline
 * @lock: Spinlock to protect this timeline
 * @last_timestamp: Last timestamp when signaling fences
 * @device: kgsl device
 * @context: kgsl context
 */
struct kgsl_sync_timeline {
	struct kref kref;
	char name[32];

	u64 fence_context;

	struct list_head child_list_head;

	spinlock_t lock;
	unsigned int last_timestamp;
	struct kgsl_device *device;
	struct kgsl_context *context;
};

/*
 * Set this flag for a kgsl hardware fence to indicate that the hardware fence refcount for
 * this fence is incremented if the corresponding dma fence is not signaled at the time of
 * creation of this hardware fence. This hardware fence refcount is put back when the
 * corresponding dma fence is signaled.
 */
#define KGSL_FENCE_FLAG_SIGNAL_REFCOUNT 0

/*
 * Set this flag for a kgsl hardware fence to indicate that the hr timer has been
 * triggered at least once. This can later be checked if the timer needs to be canceled
 * when the fence is signaled.
 */
#define KGSL_FENCE_FLAG_TIMER_TRIGGERED 1

#define KGSL_FENCE_TYPE_HW_FENCE BIT(0)
#define KGSL_FENCE_TYPE_SYNX_FENCE BIT(1)

#define KGSL_INPUT_FENCE_TYPE_HW_FENCE KGSL_FENCE_TYPE_HW_FENCE
#define KGSL_INPUT_FENCE_TYPE_SYNX_FENCE KGSL_FENCE_TYPE_SYNX_FENCE

#define KGSL_OUTPUT_FENCE_TYPE_HW_FENCE KGSL_FENCE_TYPE_HW_FENCE
#define KGSL_OUTPUT_FENCE_TYPE_SYNX_FENCE KGSL_FENCE_TYPE_SYNX_FENCE

/**
 * struct kgsl_sync_fence - A struct containing a fence and other data
 *				associated with it
 * @fence: The fence struct
 * @sync_file: Pointer to the sync file
 * @parent: Pointer to the kgsl sync timeline this fence is on
 * @child_list: List of fences on the same timeline
 * @context_id: kgsl context id
 * @timestamp: Context timestamp that this fence is associated with
 */
struct kgsl_sync_fence {
	struct dma_fence fence;
	struct sync_file *sync_file;
	struct kgsl_sync_timeline *parent;
	struct list_head child_list;
	u32 context_id;
	unsigned int timestamp;
	/** @synx_handle: synx handle backing this dma fence */
	u64 synx_handle;
	/** @hw_fence_handle: Hw fence handle backing this dma fence */
	u64 hw_fence_handle;
	/** @hw_fence_list: Global list of hw fences */
	struct list_head hw_fence_list;
	/**
	 * @hw_refcount: Refcount to release the hw fence handle when hardware has signaled
	 * this fence and the sw dma fence has also been signaled
	 */
	struct kref hw_refcount;
	/** @flags: kgsl sync fence specific flags */
	unsigned long flags;
	/** @deadline_work: Work that gets scheduled when deadline_timer expires */
	struct kthread_work deadline_work;
	/** @deadline_timer: A missed deadline detection timer */
	struct hrtimer deadline_timer;
	/** @deadline: Kernel time representing fence deadline */
	ktime_t deadline;
	/** @signaled: Kernel time when the fence was signaled */
	ktime_t signaled;
	/** @fence_type: Type of output fence */
	u32 fence_type;
};

/**
 * struct kgsl_sync_fence_cb - Used for fence callbacks
 * fence_cb: Fence callback struct
 * fence: Pointer to the fence for which the callback is done
 * priv: Private data for the callback
 * func: Pointer to the kgsl function to call. This function should return
 * false if the sync callback is marked for cancellation in a separate thread.
 */
struct kgsl_sync_fence_cb {
	struct dma_fence_cb fence_cb;
	struct dma_fence *fence;
	void *priv;
	bool (*func)(void *priv);
};

struct kgsl_device_private;
struct kgsl_drawobj_sync_event;
struct event_fence_info;
struct kgsl_process_private;
struct kgsl_syncsource;

#if defined(CONFIG_SYNC_FILE)
int kgsl_add_fence_event(struct kgsl_device *device,
	u32 context_id, u32 timestamp, void __user *data, int len,
	struct kgsl_device_private *owner);

int kgsl_sync_timeline_create(struct kgsl_context *context);

void kgsl_sync_timeline_detach(struct kgsl_sync_timeline *ktimeline);

void kgsl_sync_timeline_put(struct kgsl_sync_timeline *ktimeline);

void kgsl_sync_timeline_value_str(struct dma_fence *fence, char *str, int size);

struct dma_fence *kgsl_sync_file_get_fence(int fd);

struct kgsl_sync_fence_cb *kgsl_sync_fence_async_wait(int fd, bool (*func)(void *priv), void *priv);

struct kgsl_sync_fence_cb *kgsl_sync_fence_async_wait_fence(struct dma_fence *fence,
	bool (*func)(void *priv), void *priv);

void kgsl_get_fence_info(struct kgsl_drawobj_sync_event *event);

void kgsl_sync_fence_async_cancel(struct kgsl_sync_fence_cb *kcb);

long kgsl_ioctl_syncsource_create(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data);
long kgsl_ioctl_syncsource_destroy(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data);
long kgsl_ioctl_syncsource_create_fence(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data);
long kgsl_ioctl_syncsource_signal_fence(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data);

void kgsl_syncsource_put(struct kgsl_syncsource *syncsource);

void kgsl_syncsource_process_release_syncsources(
		struct kgsl_process_private *private);

bool is_kgsl_fence(struct dma_fence *f);

void kgsl_sync_timeline_signal(struct kgsl_sync_timeline *ktimeline,
		u32 timestamp);

void kgsl_hw_fence_put(struct kgsl_sync_fence *kfence);

void kgsl_get_fence_name(struct dma_fence *f, char *name, u32 max_size);

/*
 * kgsl_populate_hw_fences - Populate hardware fences in a sync event
 * event: Pointer to the sync event
 *
 * This function checks if the fences in the sync event are hardware fences.
 * If so, it will allocate an array to keep track of all hardware fences
 * in this sync event. If there is an unsignaled software fence, then it will
 * set KGSL_SYNCOBJ_SW for this sync event.
 */
void kgsl_populate_hw_fences(struct kgsl_drawobj_sync_event *event);
#else
static inline int kgsl_add_fence_event(struct kgsl_device *device,
	u32 context_id, u32 timestamp, void __user *data, int len,
	struct kgsl_device_private *owner)
{
	return -EINVAL;
}

static inline int kgsl_sync_timeline_create(struct kgsl_context *context)
{
	context->ktimeline = NULL;
	return 0;
}

static inline void kgsl_sync_timeline_detach(struct kgsl_sync_timeline *ktimeline)
{
}

static inline void kgsl_sync_timeline_put(struct kgsl_sync_timeline *ktimeline)
{
}


static inline void kgsl_get_fence_info(struct kgsl_drawobj_sync_event *event)
{
}

struct dma_fence *kgsl_sync_file_get_fence(int fd)
{
}

static inline struct kgsl_sync_fence_cb *kgsl_sync_fence_async_wait(int fd,
	bool (*func)(void *priv), void *priv)
{
	return NULL;
}

static inline struct kgsl_sync_fence_cb *kgsl_sync_fence_async_wait_fence(struct dma_fence *fence,
	bool (*func)(void *priv), void *priv)
{
	return NULL;
}

static inline void
kgsl_sync_fence_async_cancel(struct kgsl_sync_fence_cb *kcb)
{
}

static inline long
kgsl_ioctl_syncsource_create(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data)
{
	return -ENOIOCTLCMD;
}

static inline long
kgsl_ioctl_syncsource_destroy(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data)
{
	return -ENOIOCTLCMD;
}

static inline long
kgsl_ioctl_syncsource_create_fence(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data)
{
	return -ENOIOCTLCMD;
}

static inline long
kgsl_ioctl_syncsource_signal_fence(struct kgsl_device_private *dev_priv,
					unsigned int cmd, void *data)
{
	return -ENOIOCTLCMD;
}

static inline void kgsl_syncsource_put(struct kgsl_syncsource *syncsource)
{

}

static inline void kgsl_syncsource_process_release_syncsources(
		struct kgsl_process_private *private)
{

}

static inline bool is_kgsl_fence(struct dma_fence *f)
{
	return false;
}

static inline void kgsl_sync_timeline_signal(struct kgsl_sync_timeline *ktimeline,
		u32 timestamp)
{

}

static inline void kgsl_hw_fence_put(struct kgsl_sync_fence *kfence)
{

}

static inline void kgsl_populate_hw_fences(struct kgsl_drawobj_sync_event *event)
{
}

#endif /* CONFIG_SYNC_FILE */

#if IS_ENABLED(CONFIG_QCOM_KGSL_SYNX)
int kgsl_synx_register(struct kgsl_device *device, struct kgsl_memdesc *synx_md);

void kgsl_synx_deregister(struct kgsl_device *device, struct kgsl_memdesc *synx_md);

int kgsl_synx_import(struct kgsl_device *device, struct dma_fence *fence, u32 *hash_index);

void kgsl_synx_import_release(struct kgsl_device *device, u32 handle);

bool kgsl_is_synx_hw_fence(struct dma_fence *fence);

bool kgsl_is_synx_native_fence(struct dma_fence *fence);
#else
static inline int kgsl_synx_register(struct kgsl_device *device, struct kgsl_memdesc *synx_md)
{
	return -EINVAL;
}

static inline void kgsl_synx_deregister(struct kgsl_device *device, struct kgsl_memdesc *synx_md)
{

}

static inline int kgsl_synx_import(struct kgsl_device *device, struct dma_fence *fence,
		u32 *hash_index)
{
	return -EINVAL;
}

static inline void kgsl_synx_import_release(struct kgsl_device *device, u32 handle)
{
}

static inline bool kgsl_is_synx_hw_fence(struct dma_fence *fence)
{
	return false;
}

static inline bool kgsl_is_synx_native_fence(struct dma_fence *fence)
{
	return false;
}
#endif

#if (IS_ENABLED(CONFIG_SYNC_FILE) && (IS_ENABLED(CONFIG_QTI_HW_FENCE) || \
	IS_ENABLED(CONFIG_QCOM_KGSL_SYNX)))
int kgsl_hw_fence_init(struct kgsl_device *device);

void kgsl_hw_fence_close(struct kgsl_device *device);

int kgsl_hw_fence_create(struct kgsl_device *device, struct kgsl_context *context,
	struct kgsl_sync_fence *kfence);

int kgsl_external_fence_import(struct kgsl_device *device,
	struct kgsl_drawobj_sync_input_fence *input_fence);

bool kgsl_hw_fence_tx_slot_available(struct kgsl_device *device, u32 pending_hw_fence_count);

void kgsl_hw_fence_trigger_cpu(struct kgsl_device *device, struct kgsl_sync_fence *kfence);

bool kgsl_hw_fence_signaled(struct dma_fence *fence);

int kgsl_hw_fence_soccp_vote(bool pwr_on);

int kgsl_hw_fence_register(struct kgsl_device *device, struct kgsl_memdesc *md);

void kgsl_hw_fence_deregister(struct kgsl_device *device, struct kgsl_memdesc *md);

#else

static inline int kgsl_hw_fence_soccp_vote(bool pwr_on)
{
	return -EINVAL;
}

static inline int kgsl_hw_fence_init(struct kgsl_device *device)
{
	return -EINVAL;
}

static inline void kgsl_hw_fence_close(struct kgsl_device *device)
{

}

static inline int kgsl_hw_fence_create(struct kgsl_device *device,
		struct kgsl_context *context, struct kgsl_sync_fence *kfence)
{
	return -EINVAL;
}

static inline int kgsl_external_fence_import(struct kgsl_device *device,
	struct kgsl_drawobj_sync_input_fence *input_fence)
{
	return -EINVAL;
}

static inline bool kgsl_hw_fence_tx_slot_available(struct kgsl_device *device,
		u32 pending_hw_fence_count)
{
	return false;
}

static inline void kgsl_hw_fence_trigger_cpu(struct kgsl_device *device,
		struct kgsl_sync_fence *kfence)
{

}

static inline bool kgsl_hw_fence_signaled(struct dma_fence *fence)
{
	return false;
}

static inline bool kgsl_is_input_hw_fence(struct dma_fence *fence)
{
	return false;
}

static inline int kgsl_hw_fence_register(struct kgsl_device *device, struct kgsl_memdesc *md)
{
	return -EINVAL;
}

static inline void kgsl_hw_fence_deregister(struct kgsl_device *device, struct kgsl_memdesc *md)
{
}

#endif /* CONFIG_SYNC_FILE && (CONFIG_QTI_HW_FENCE || CONFIG_QCOM_KGSL_SYNX) */

#endif /* __KGSL_SYNC_H */
