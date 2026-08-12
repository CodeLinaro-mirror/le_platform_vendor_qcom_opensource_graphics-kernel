// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/net.h>
#include <net/sock.h>

#include "adreno.h"
#include "adreno_qmi.h"
#include "kgsl_snapshot.h"

static const struct qmi_elem_info qdcp_gpudbg_snapshot_req_msg_v01_ei[] = {
	{
		.data_type      = QMI_UNSIGNED_8_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u64),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0x01,
		.offset         = offsetof(struct qdcp_gpudbg_snapshot_req_msg_v01,
					   gpudbg_id),
	},
	{
		.data_type      = QMI_DATA_LEN,
		.elem_len       = 1,
		.elem_size      = sizeof(u8),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0x02,
		.offset         = offsetof(struct qdcp_gpudbg_snapshot_req_msg_v01,
					   block_type_len),
	},
	{
		.data_type      = QMI_SIGNED_4_BYTE_ENUM,
		.elem_len       = QDCP_GPUDBG_BLOCK_TYPES_MAX_V01,
		.elem_size      = sizeof(enum qdcp_gpudbg_block_enum_type_v01),
		.array_type     = VAR_LEN_ARRAY,
		.tlv_type       = 0x02,
		.offset         = offsetof(struct qdcp_gpudbg_snapshot_req_msg_v01,
					   block_type),
	},
	{
		.data_type      = QMI_EOTI,
		.array_type     = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};

static const struct qmi_elem_info qdcp_gpudbg_snapshot_resp_msg_v01_ei[] = {
	{
		.data_type      = QMI_STRUCT,
		.elem_len       = 1,
		.elem_size      = sizeof(struct qmi_response_type_v01),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0x02,
		.offset         = offsetof(struct qdcp_gpudbg_snapshot_resp_msg_v01,
					   resp),
		.ei_array      = qmi_response_type_v01_ei,
	},
	{
		.data_type      = QMI_EOTI,
		.array_type     = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};

static const struct qmi_elem_info qdcp_gpudbg_get_data_req_msg_v01_ei[] = {
	{
		.data_type      = QMI_UNSIGNED_8_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u64),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0x01,
		.offset         = offsetof(struct qdcp_gpudbg_get_data_req_msg_v01,
					   gpudbg_id),
	},
	{
		.data_type      = QMI_UNSIGNED_8_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u64),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0x02,
		.offset         = offsetof(struct qdcp_gpudbg_get_data_req_msg_v01,
					   paddr),
	},
	{
		.data_type      = QMI_UNSIGNED_8_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u64),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0x03,
		.offset         = offsetof(struct qdcp_gpudbg_get_data_req_msg_v01,
					   size),
	},
	{
		.data_type      = QMI_EOTI,
		.array_type     = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};

static const struct qmi_elem_info qdcp_gpudbg_get_data_resp_msg_v01_ei[] = {
	{
		.data_type      = QMI_STRUCT,
		.elem_len       = 1,
		.elem_size      = sizeof(struct qmi_response_type_v01),
		.array_type	= NO_ARRAY,
		.tlv_type       = 0x02,
		.offset         = offsetof(struct qdcp_gpudbg_get_data_resp_msg_v01,
					   resp),
		.ei_array      = qmi_response_type_v01_ei,
	},
	{
		.data_type      = QMI_UNSIGNED_8_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u64),
		.array_type	= NO_ARRAY,
		.tlv_type       = 0x03,
		.offset         = offsetof(struct qdcp_gpudbg_get_data_resp_msg_v01,
					data_size),
	},
	{
		.data_type	= QMI_EOTI,
		.array_type	= NO_ARRAY,
		.tlv_type	= QMI_COMMON_TLV_TYPE,
	},
};

static const struct qmi_elem_info qdcp_gpudbg_block_type_v01_ei[] = {
	{
		.data_type      = QMI_UNSIGNED_4_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u32),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0,
		.offset         = offsetof(struct qdcp_gpudbg_block_type_v01,
					   block_id),
	},
	{
		.data_type      = QMI_UNSIGNED_4_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u32),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0,
		.offset         = offsetof(struct qdcp_gpudbg_block_type_v01,
					   block_size),
	},
	{
		.data_type      = QMI_EOTI,
		.array_type     = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};

static const struct qmi_elem_info qdcp_gpudbg_blocks_get_req_msg_v01_ei[] = {
	{
		.data_type      = QMI_UNSIGNED_8_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u64),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0x01,
		.offset         = offsetof(struct qdcp_gpudbg_blocks_get_req_msg_v01,
					   gpudbg_id),
	},
	{
		.data_type      = QMI_SIGNED_4_BYTE_ENUM,
		.elem_len       = 1,
		.elem_size      = sizeof(enum qdcp_gpudbg_block_enum_type_v01),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0x02,
		.offset         = offsetof(struct qdcp_gpudbg_blocks_get_req_msg_v01,
					   block_type),
	},
	{
		.data_type      = QMI_EOTI,
		.array_type     = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};

static const struct qmi_elem_info qdcp_gpudbg_blocks_get_resp_msg_v01_ei[] = {
	{
		.data_type      = QMI_STRUCT,
		.elem_len       = 1,
		.elem_size      = sizeof(struct qmi_response_type_v01),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0x02,
		.offset         = offsetof(struct qdcp_gpudbg_blocks_get_resp_msg_v01,
					   resp),
		.ei_array       = qmi_response_type_v01_ei,
	},
	{
		.data_type      = QMI_DATA_LEN,
		.elem_len       = 1,
		.elem_size      = sizeof(u32),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0x03,
		.offset         = offsetof(struct qdcp_gpudbg_blocks_get_resp_msg_v01,
					   blocks_len),
	},
	{
		.data_type      = QMI_STRUCT,
		.elem_len       = QDCP_GPUDBG_BLOCKS_MAX_V01,
		.elem_size      = sizeof(struct qdcp_gpudbg_block_type_v01),
		.array_type     = VAR_LEN_ARRAY,
		.tlv_type       = 0x03,
		.offset         = offsetof(struct qdcp_gpudbg_blocks_get_resp_msg_v01,
					   blocks),
		.ei_array      = qdcp_gpudbg_block_type_v01_ei,
	},
	{
		.data_type      = QMI_EOTI,
		.array_type     = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};

static const struct qmi_elem_info qdcp_gpudbg_blocks_set_req_msg_v01_ei[] = {
	{
		.data_type      = QMI_UNSIGNED_8_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u64),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0x01,
		.offset         = offsetof(struct qdcp_gpudbg_blocks_set_req_msg_v01,
					   gpudbg_id),
	},
	{
		.data_type      = QMI_SIGNED_4_BYTE_ENUM,
		.elem_len       = 1,
		.elem_size      = sizeof(enum qdcp_gpudbg_block_enum_type_v01),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0x02,
		.offset         = offsetof(struct qdcp_gpudbg_blocks_set_req_msg_v01,
					   block_type),
	},
	{
		.data_type      = QMI_DATA_LEN,
		.elem_len       = 1,
		.elem_size      = sizeof(u16),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0x03,
		.offset         = offsetof(struct qdcp_gpudbg_blocks_set_req_msg_v01,
					   blocks_len),
	},
	{
		.data_type      = QMI_STRUCT,
		.elem_len       = QDCP_GPUDBG_BLOCKS_MAX_V01,
		.elem_size      = sizeof(struct qdcp_gpudbg_block_type_v01),
		.array_type     = VAR_LEN_ARRAY,
		.tlv_type       = 0x03,
		.offset         = offsetof(struct qdcp_gpudbg_blocks_set_req_msg_v01,
					   blocks),
		.ei_array       = qdcp_gpudbg_block_type_v01_ei,
	},
	{
		.data_type      = QMI_EOTI,
		.array_type     = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};

static const struct qmi_elem_info qdcp_gpudbg_blocks_set_resp_msg_v01_ei[] = {
	{
		.data_type      = QMI_STRUCT,
		.elem_len       = 1,
		.elem_size      = sizeof(struct qmi_response_type_v01),
		.array_type     = NO_ARRAY,
		.tlv_type       = 0x02,
		.offset         = offsetof(struct qdcp_gpudbg_blocks_set_resp_msg_v01,
					   resp),
		.ei_array       = qmi_response_type_v01_ei,
	},
	{
		.data_type      = QMI_EOTI,
		.array_type     = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};

static int adreno_qmi_new_server(struct qmi_handle *qmi, struct qmi_service *service)
{
	struct adreno_device *adreno_dev = container_of(qmi, struct adreno_device, qmi);
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct platform_device *pdev = device->pdev;
	struct sockaddr_qrtr *sq = &adreno_dev->sq;
	int ret;

	if (!adreno_dev->qecp_debugbus_enabled)
		return 0;

	sq->sq_family = AF_QIPCRTR;
	sq->sq_node = service->node;
	sq->sq_port = service->port;
	service->priv = pdev;

	ret = kernel_connect(adreno_dev->qmi.sock, (struct sockaddr *)sq, sizeof(*sq), 0);
	if (ret) {
		dev_err_ratelimited(device->dev,
			"Failed to connect to remote QMI port ret=%d\n", ret);
		return ret;
	}
	adreno_dev->qmi_service_connected = true;
	return 0;
}

static void adreno_qmi_del_server(struct qmi_handle *qmi, struct qmi_service *service)
{
	struct adreno_device *adreno_dev = container_of(qmi, struct adreno_device, qmi);

	if (!adreno_dev->qecp_debugbus_enabled)
		return;

	adreno_dev->qmi_service_connected = false;
	adreno_dev->qecp_data_sent = false;
}

static const struct qmi_ops adreno_qmi_ops = {
	.new_server = adreno_qmi_new_server,
	.del_server = adreno_qmi_del_server,
};

void adreno_qmi_init(struct adreno_device *adreno_dev)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	int ret;

	if (!adreno_dev->qecp_debugbus_enabled)
		return;

	/* Set a limit on setup attempts */
	adreno_dev->qecp_retry_count = 10;

	ret = qmi_handle_init(&adreno_dev->qmi,
			QDCP_GPUDBG_BLOCKS_SET_REQ_MSG_V01_MAX_MSG_LEN,
			&adreno_qmi_ops, NULL);
	if (ret) {
		dev_err_ratelimited(device->dev,
			"Unable to setup QMI handle ret=%d\n", ret);
		adreno_dev->qecp_debugbus_enabled = false;
	}
}

void adreno_qmi_setup_service(struct adreno_device *adreno_dev)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	int ret;

	if (!adreno_dev->qecp_debugbus_enabled)
		return;

	/* Set up the service to QECP server */
	ret = qmi_add_lookup(&adreno_dev->qmi, QDCP_SERVICE_ID, QDCP_VERSION_ID, QDCP_INSTANCE_ID);
	if (ret) {
		dev_err_ratelimited(device->dev, "Failed to add QMI lookup ret=%d\n", ret);
		adreno_dev->qecp_debugbus_enabled = false;
	}
}

int adreno_qmi_debugbus_get_data(struct adreno_device *adreno_dev,
		struct kgsl_snapshot *snapshot)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct qmi_handle *qmi = &adreno_dev->qmi;
	struct qdcp_gpudbg_get_data_resp_msg_v01 *resp;
	struct qdcp_gpudbg_get_data_req_msg_v01 *req;
	struct qmi_txn txn;
	int ret;

	if (!adreno_dev->qecp_data_sent || !adreno_dev->qecp_debugbus_enabled)
		return -ENOENT;

	if (!adreno_dev->qmi_service_connected)
		return -ENOENT;

	if (!snapshot->qecp_carveout_addr)
		return -ENOMEM;

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->paddr = snapshot->qecp_carveout_addr;
	req->size = snapshot->qecp_carveout_size;

	ret = qmi_txn_init(qmi, &txn, qdcp_gpudbg_get_data_resp_msg_v01_ei, resp);
	if (ret < 0) {
		dev_err_ratelimited(device->dev, "QMI txn init failed ret=%d\n", ret);
		goto out;
	}

	ret = qmi_send_request(qmi, NULL, &txn,
				QMI_QDCP_GPUDBG_GET_DATA_REQ_V01,
				QDCP_GPUDBG_GET_DATA_REQ_MSG_V01_MAX_MSG_LEN,
				qdcp_gpudbg_get_data_req_msg_v01_ei, req);
	if (ret < 0) {
		dev_err_ratelimited(device->dev, "QMI send request failed ret=%d\n", ret);
		qmi_txn_cancel(&txn);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_TRANSACTION_TIMEOUT);
	if (ret < 0) {
		dev_err_ratelimited(device->dev, "QMI response timed out ret=%d\n", ret);
	} else if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		ret = -EINVAL;
		dev_err_ratelimited(device->dev,
			"QMI response error ret=%d err=%d\n", ret, resp->resp.error);
	} else {
		ret = resp->resp.result;
	}

out:
	kfree(resp);
	kfree(req);

	return ret;
}

int adreno_qmi_trigger_debugbus_capture(struct adreno_device *adreno_dev, u32 type)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct qmi_handle *qmi = &adreno_dev->qmi;
	struct qdcp_gpudbg_snapshot_resp_msg_v01 *resp;
	struct qdcp_gpudbg_snapshot_req_msg_v01 *req;
	struct qmi_txn txn;
	int ret;

	if (!adreno_dev->qecp_data_sent || !adreno_dev->qecp_debugbus_enabled)
		return -ENOENT;

	if (!adreno_dev->qmi_service_connected)
		return -ENOENT;

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	/* GX = 0, CX = 1 */
	req->block_type_len = 1;
	req->block_type[0] = type;

	ret = qmi_txn_init(qmi, &txn, qdcp_gpudbg_snapshot_resp_msg_v01_ei, resp);
	if (ret < 0) {
		dev_err_ratelimited(device->dev, "QMI txn init failed ret=%d\n", ret);
		goto out;
	}

	ret = qmi_send_request(qmi, NULL, &txn,
				QMI_QDCP_GPUDBG_SNAPSHOT_REQ_V01,
				QDCP_GPUDBG_SNAPSHOT_REQ_MSG_V01_MAX_MSG_LEN,
				qdcp_gpudbg_snapshot_req_msg_v01_ei, req);
	if (ret < 0) {
		dev_err_ratelimited(device->dev, "QMI send request failed ret=%d\n", ret);
		qmi_txn_cancel(&txn);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_TRANSACTION_TIMEOUT);
	if (ret < 0) {
		dev_err_ratelimited(device->dev, "QMI response timed out ret=%d\n", ret);
	} else if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		ret = -EINVAL;
		dev_err_ratelimited(device->dev,
			"QMI response error ret=%d err=%d\n", ret, resp->resp.error);
	} else {
		ret = resp->resp.result;
	}

out:
	kfree(resp);
	kfree(req);

	return ret;
}

int adreno_qmi_debugbus_blocks_get(struct adreno_device *adreno_dev, u32 type)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct qmi_handle *qmi = &adreno_dev->qmi;
	struct qdcp_gpudbg_blocks_get_resp_msg_v01 *resp;
	struct qdcp_gpudbg_blocks_get_req_msg_v01 *req;
	struct qmi_txn txn;
	int ret;

	if (!adreno_dev->qmi_service_connected)
		return -ENOENT;

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	/* GX = 0, CX = 1 */
	req->block_type = type;

	ret = qmi_txn_init(qmi, &txn, qdcp_gpudbg_blocks_get_resp_msg_v01_ei, resp);
	if (ret < 0) {
		dev_err_ratelimited(device->dev, "QMI txn init failed ret=%d\n", ret);
		goto out;
	}

	ret = qmi_send_request(qmi, NULL, &txn,
				QMI_QDCP_GPUDBG_BLOCKS_GET_REQ_V01,
				QDCP_GPUDBG_BLOCKS_GET_REQ_MSG_V01_MAX_MSG_LEN,
				qdcp_gpudbg_blocks_get_req_msg_v01_ei, req);
	if (ret < 0) {
		dev_err_ratelimited(device->dev, "QMI send request failed ret=%d\n", ret);
		qmi_txn_cancel(&txn);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_TRANSACTION_TIMEOUT);
	if (ret < 0) {
		dev_err_ratelimited(device->dev, "QMI response timed out ret=%d\n", ret);
	} else if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		ret = -EINVAL;
		dev_err_ratelimited(device->dev,
			"QMI response error ret=%d err=%d\n", ret, resp->resp.error);
	} else {
		ret = resp->blocks_len;
	}

out:
	kfree(resp);
	kfree(req);

	return ret;
}

int adreno_qmi_debugbus_blocks_set(struct adreno_device *adreno_dev,
		struct debugbus_info *info, size_t len, u32 type)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct qmi_handle *qmi = &adreno_dev->qmi;
	struct qdcp_gpudbg_blocks_set_resp_msg_v01 *resp;
	struct qdcp_gpudbg_blocks_set_req_msg_v01 *req;
	struct qmi_txn txn;
	int ret, i;

	if (!adreno_dev->qmi_service_connected)
		return -ENOENT;

	if (len >= UINT_MAX)
		return -EINVAL;

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	/* GX = 0, CX = 1 */
	req->block_type = type;
	req->blocks_len = len;

	for (i = 0; i < len; i++) {
		req->blocks[i].block_id = info[i].debugbus_id;
		req->blocks[i].block_size = info[i].debugbus_size;
	}

	ret = qmi_txn_init(qmi, &txn, qdcp_gpudbg_blocks_set_resp_msg_v01_ei, resp);
	if (ret < 0) {
		dev_err_ratelimited(device->dev, "QMI txn init failed ret=%d\n", ret);
		goto out;
	}

	ret = qmi_send_request(qmi, NULL, &txn,
				QMI_QDCP_GPUDBG_BLOCKS_SET_REQ_V01,
				QDCP_GPUDBG_BLOCKS_SET_REQ_MSG_V01_MAX_MSG_LEN,
				qdcp_gpudbg_blocks_set_req_msg_v01_ei, req);
	if (ret < 0) {
		dev_err_ratelimited(device->dev, "QMI send request failed ret=%d\n", ret);
		qmi_txn_cancel(&txn);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_TRANSACTION_TIMEOUT);
	if (ret < 0) {
		dev_err_ratelimited(device->dev, "QMI response timed out ret=%d\n", ret);
	} else if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		dev_err_ratelimited(device->dev,
			"QMI response error ret=%d err=%d\n", ret, resp->resp.error);
		ret = -EINVAL;
	} else {
		ret = resp->resp.result;
	}

out:
	kfree(resp);
	kfree(req);

	return ret;
}

static size_t adreno_qmi_snapshot_qecp_status(struct kgsl_device *device, u8 *buf,
		size_t remain, void *priv)
{
	struct kgsl_snapshot_debug *header = (struct kgsl_snapshot_debug *)buf;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	u32 *data = (u32 *)(buf + sizeof(*header));

	if (remain < DEBUG_SECTION_SZ(1)) {
		SNAPSHOT_ERR_NOMEM(device, "QECP STATUS");
		return 0;
	}

	/* Dump the QECP debugbus status */
	header->type = SNAPSHOT_DEBUG_QECP;
	header->size = 1;
	/* Indicate whether QECP is enabled */
	*data = adreno_dev->qecp_debugbus_enabled;

	return DEBUG_SECTION_SZ(1);
}

/*
 * Our current snapshot has the following structure for every target. The first
 * 48 bytes store the main snapshot header. 350KB starting from that point is
 * shared with QECP to store the encrypted debugbus. The rest of the memory
 * region is available for the remaining snaphot data. Also debugbus is collected
 * one of two ways, encrypted using QECP or the AHB method in KGSL. Since either
 * one of the method is used we are certain to have enough memory for at least
 * one of the method.
 */
static void adreno_qmi_allocate_qecp_carveout(struct adreno_device *adreno_dev,
			struct kgsl_snapshot *snapshot)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct kgsl_snapshot_section_header *header =
		(struct kgsl_snapshot_section_header *)snapshot->ptr;

	header->magic = SNAPSHOT_SECTION_MAGIC;
	header->id = KGSL_SNAPSHOT_SECTION_QECP;

	snapshot->qecp_carveout_addr = snapshot_phy_addr(device) + snapshot->size + sizeof(*header);
	snapshot->qecp_carveout_size = QECP_CARVEOUT_SIZE;

	header->size = snapshot->qecp_carveout_size + sizeof(*header);
	snapshot->ptr += header->size;
	snapshot->remain -= header->size;
	snapshot->size += header->size;
}

void adreno_qmi_qecp_carveout(struct adreno_device *adreno_dev, struct kgsl_snapshot *snapshot)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);

	/* Skip if QECP debugbus is not enabled or the debugbus fuse is not blown */
	if (!adreno_dev->qecp_debugbus_enabled ||
		!(device->debug_bus_bin && !device->debugbus_en && !device->gpu_niden_en))
		return;

	kgsl_snapshot_add_section(device, KGSL_SNAPSHOT_SECTION_DEBUG,
		snapshot, adreno_qmi_snapshot_qecp_status, NULL);

	adreno_qmi_allocate_qecp_carveout(adreno_dev, snapshot);
}

