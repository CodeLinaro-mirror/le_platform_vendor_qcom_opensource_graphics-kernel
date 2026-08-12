/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _ADRENO_QMI_H_
#define _ADRENO_QMI_H_

#include "adreno_gen8_snapshot.h"

/* Message IDs between KGSL and QECP */
#define QMI_QDCP_GPUDBG_SNAPSHOT_REQ_V01 0x0040
#define QMI_QDCP_GPUDBG_SNAPSHOT_RESP_V01 0x0040
#define QMI_QDCP_GPUDBG_GET_DATA_REQ_V01 0x0041
#define QMI_QDCP_GPUDBG_GET_DATA_RESP_V01 0x0041
#define QMI_QDCP_GPUDBG_BLOCKS_GET_REQ_V01 0x0042
#define QMI_QDCP_GPUDBG_BLOCKS_GET_RESP_V01 0x0042
#define QMI_QDCP_GPUDBG_BLOCKS_SET_REQ_V01 0x0043
#define QMI_QDCP_GPUDBG_BLOCKS_SET_RESP_V01 0x0043
#define QDCP_GPUDBG_BLOCK_TYPES_MAX_V01 2
#define QDCP_GPUDBG_BLOCKS_MAX_V01 256

/* Max message lengths for each mesasge type between KGSL and QECP */
#define QDCP_GPUDBG_SNAPSHOT_REQ_MSG_V01_MAX_MSG_LEN 23
#define QDCP_GPUDBG_SNAPSHOT_RESP_MSG_V01_MAX_MSG_LEN 7
#define QDCP_GPUDBG_GET_DATA_REQ_MSG_V01_MAX_MSG_LEN 33
#define QDCP_GPUDBG_GET_DATA_RESP_MSG_V01_MAX_MSG_LEN 18
#define QDCP_GPUDBG_BLOCKS_GET_REQ_MSG_V01_MAX_MSG_LEN 18
#define QDCP_GPUDBG_BLOCKS_GET_RESP_MSG_V01_MAX_MSG_LEN 2060
#define QDCP_GPUDBG_BLOCKS_SET_REQ_MSG_V01_MAX_MSG_LEN 2071
#define QDCP_GPUDBG_BLOCKS_SET_RESP_MSG_V01_MAX_MSG_LEN 7

#define QDCP_SERVICE_ID 1107
#define QDCP_INSTANCE_ID 32
#define QDCP_VERSION_ID 1

#define QECP_CARVEOUT_SIZE (SZ_1K * 350)
#define QMI_TRANSACTION_TIMEOUT (5 * HZ)

/* Types of debugbus blocks to be sent to QECP */
enum qdcp_gpudbg_block_enum_type_v01 {
	QDCP_GPUDBG_BLOCK_ENUM_TYPE_MIN_VAL_V01 = INT_MIN,
	QDCP_GPUDBG_BLOCK_CX_V01 = 0,
	QDCP_GPUDB_BLOCK_GX_V01 = 1,
	QDCP_GPUDBG_BLOCK_ENUM_TYPE_MAX_VAL_V01 = INT_MAX,
};

/**
 * qdcp_gpudbg_snapshot_req_msg_v01 - Container for the GPU debugbus capture request
 * message between QECP and KGSL
 */
struct qdcp_gpudbg_snapshot_req_msg_v01 {
	/** @gpudbg_id: Unique ID for GPU */
	u64 gpudbg_id;
	/** @block_type_len: Length of the blocktype list */
	u32 block_type_len;
	/** @block_type: List of the block type to capture */
	enum qdcp_gpudbg_block_enum_type_v01 block_type[QDCP_GPUDBG_BLOCK_TYPES_MAX_V01];
};

/**
 * qdcp_gpudbg_snapshot_resp_msg_v01 - Container of the GPU debugbus capture response
 * message between QECP and KGSL
 */
struct qdcp_gpudbg_snapshot_resp_msg_v01 {
	/**
	 * @resp: Response message received from the QECP for message type
	 * qdcp_gpudbg_snapshot_req_msg_v01
	 */
	struct qmi_response_type_v01 resp;
};

/**
 * qdcp_gpudbg_get_data_req_msg_v01 - Container of the GPU debugbus snapshot request
 * message to copy the encrypted data into the buffer provided by KGSL to QECP
 */
struct qdcp_gpudbg_get_data_req_msg_v01 {
	/** @gpudbg_id: Unique ID for GPU */
	u64 gpudbg_id;
	/** @paddr: Physical address of the buffer to copy data into */
	u64 paddr;
	/** @size: Size of the buffer in bytes provided by KGSL to QECP */
	u64 size;
};

/**
 * qdcp_gpudbg_get_data_resp_msg_v01: Container of the GPU debugbus snasphot response
 * message between QECP and KGSL
 */
struct qdcp_gpudbg_get_data_resp_msg_v01 {
	/**
	 * @resp: Response message received from the QECP for message type
	 * qdcp_gpudbg_get_data_req_msg_v01
	 */
	struct qmi_response_type_v01 resp;
	/** @data_size: Number of encrypted bytes copied into the buffer */
	u64 data_size;
};

/**
 * qdcp_gpudbg_block_type_v01: Container of the GPU debugbus ID/size to be passed
 * to the QECP driver
 */
struct qdcp_gpudbg_block_type_v01 {
	/** @block_id: GPU debugbus block ID */
	u32 block_id;
	/** @block_size: GPU debugbus block size in dwords */
	u32 block_size;
};

/**
 * qdcp_gpudbg_blocks_get_req_msg_v01: Container of the GPU message to get the number
 * of debugbus blocks which are going to be dumped by QECP
 */
struct qdcp_gpudbg_blocks_get_req_msg_v01 {
	/** @gpudbg_id: Unique ID for GPU */
	u64 gpudbg_id;
	/** block_type: GPU debugbus block type CX/GX */
	enum qdcp_gpudbg_block_enum_type_v01 block_type;
};

/**
 * qdcp_gpudbg_blocks_get_resp_msg_v01: Container of the response message from QECP
 * which contains the number of blocks being captured by QECP
 */
struct qdcp_gpudbg_blocks_get_resp_msg_v01 {
	/**
	 * @resp: Response message received by QECP for the message type
	 * qdcp_gpudbg_blocks_get_req_msg_v01
	 */
	struct qmi_response_type_v01 resp;
	/** @blocks_len: Number of debugbus blocks being captured for specific type CX/GX */
	u32 blocks_len;
	/** @blocks: List of the debugbus block type CX/GX */
	struct qdcp_gpudbg_block_type_v01 blocks[QDCP_GPUDBG_BLOCKS_MAX_V01];
};

/**
 * qdcp_gpudbg_blocks_set_req_msg_v01: Container of the GPU message to set the list
 * of debugbus blocks which are going to be dumped by QECP
 */
struct qdcp_gpudbg_blocks_set_req_msg_v01 {
	/** @gpudbg_id: Unique ID for GPU */
	u64 gpudbg_id;
	/** block_type: GPU debugbus block type CX/GX */
	enum qdcp_gpudbg_block_enum_type_v01 block_type;
	/** @blocks_len: Number of debugbus blocks being captured for specific type CX/GX */
	u32 blocks_len;
	/** @blocks: List of the debugbus block type CX/GX */
	struct qdcp_gpudbg_block_type_v01 blocks[QDCP_GPUDBG_BLOCKS_MAX_V01];
};

/**
 * qdcp_gpudbg_blocks_set_resp_msg_v01: Container of the response message from QECP
 * which contains the list of debugbus blocks to be set by QECP
 */
struct qdcp_gpudbg_blocks_set_resp_msg_v01 {
	/**
	 * @resp: Response message received from the QECP for message type
	 * qdcp_gpudbg_set_data_req_msg_v01
	 */
	struct qmi_response_type_v01 resp;
};

/**
 * adreno_qmi_init - Initialize the QMI handle that is used for QECP debugbus
 * @adreno_dev: pointer to the adreno device
 *
 * Initializes the QMI handle at probe time.
 */
void adreno_qmi_init(struct adreno_device *adreno_dev);

/**
 * adreno_qmi_setup_service - Register the QMI service that kgsl uses for QECP debugbus
 * @adreno_dev: pointer to the adreno device
 *
 * Register the QMI service at first boot.
 */
void adreno_qmi_setup_service(struct adreno_device *adreno_dev);

/**
 * adreno_qmi_debugbus_blocks_set - Send the list of debugbus blocks to QECP to capture
 * @adreno_dev: pointer to the adreno device
 * @info: Pointer to the list of debubgbus block with sizes
 * @len: Length of the list
 * @type: Type of list, CX or GX
 *
 * Sends a list of the debugbus blocks with sizes of each block to QECP. It uses this list
 * to capture data during a snapshot.
 */
int adreno_qmi_debugbus_blocks_set(struct adreno_device *adreno_dev,
		struct debugbus_info *info, size_t len, u32 type);

/**
 * adreno_qmi_debugbus_blocks_get - Get the list of debugbus blocks from QECP
 * @adreno_dev: pointer to the adreno device
 * @type: Type of list, CX or GX
 *
 * Gets the number of debugbus blocks which have been sent to QECP. This can be used to verify
 * if correct data is recorded on QECP side.
 */
int adreno_qmi_debugbus_blocks_get(struct adreno_device *adreno_dev, u32 type);

/**
 * adreno_qmi_debugbus_get_data - Copies the encrypted debugbus data into the buffer
 * provided by KGSL
 * @adreno_dev: pointer to the adreno device
 * @snapshot: Pointer to the KGSL snapshot
 *
 * Copies the encrypted data captured by QECP into the buffer provided by KGSL
 */
int adreno_qmi_debugbus_get_data(struct adreno_device *adreno_dev,
		struct kgsl_snapshot *snapshot);

/**
 * adreno_qmi_trigger_debugbus_capture - Trigger the GPU debugbus capture
 * @adreno_dev: pointer to the adreno device
 * @type: Type of block, CX or GX
 *
 * Send a message to QECP to start capturing the debugbus data
 */
int adreno_qmi_trigger_debugbus_capture(struct adreno_device *adreno_dev, u32 type);

/**
 * adreno_qmi_qecp_carveout - Setup the QECP carveout in the GPU snapshot
 * @adreno_dev: pointer to the adreno device
 * @snapshot: pointer to the kgsl_snapshot struct
 *
 * Add the SNAPSHOT_DEBUG_QECP snapshot section and allocate the QECP carveout used for encrypted
 * debugbus data
 */
void adreno_qmi_qecp_carveout(struct adreno_device *adreno_dev, struct kgsl_snapshot *snapshot);
#endif
