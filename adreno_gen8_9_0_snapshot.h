/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#ifndef __ADRENO_GEN8_9_0_SNAPSHOT_H
#define __ADRENO_GEN8_9_0_SNAPSHOT_H

#include "adreno_gen8_snapshot.h"
#include "adreno_gen8_2_0_snapshot.h"

static const u32 gen8_9_0_debugbus_blocks[] = {
	DEBUGBUS_GMU_GX_GC_US_I_0,
	DEBUGBUS_DBGC_GC_US_I_0,
	DEBUGBUS_RBBM_GC_US_I_0,
	DEBUGBUS_LARC_GC_US_I_0,
	DEBUGBUS_COM_GC_US_I_0,
	DEBUGBUS_HLSQ_GC_US_I_0,
	DEBUGBUS_CGC_GC_US_I_0,
	DEBUGBUS_VSC_GC_US_I_0_0,
	DEBUGBUS_VSC_GC_US_I_0_1,
	DEBUGBUS_UFC_GC_US_I_0,
	DEBUGBUS_UFC_GC_US_I_1,
	DEBUGBUS_CP_GC_US_I_0_0,
	DEBUGBUS_CP_GC_US_I_0_1,
	DEBUGBUS_CP_GC_US_I_0_2,
	DEBUGBUS_PC_BR_US_I_0,
	DEBUGBUS_PC_BV_US_I_0,
	DEBUGBUS_GPC_BR_US_I_0,
	DEBUGBUS_GPC_BV_US_I_0,
	DEBUGBUS_VPC_BR_US_I_0,
	DEBUGBUS_VPC_BV_US_I_0,
	DEBUGBUS_UCHE_WRAPPER_GC_US_I_0,
	DEBUGBUS_UCHE_GC_US_I_0,
	DEBUGBUS_UCHE_GC_US_I_1,
	DEBUGBUS_CP_GC_S_0_I_0,
	DEBUGBUS_PC_BR_S_0_I_0,
	DEBUGBUS_PC_BV_S_0_I_0,
	DEBUGBUS_TESS_GC_S_0_I_0,
	DEBUGBUS_TSEFE_GC_S_0_I_0,
	DEBUGBUS_TSEBE_GC_S_0_I_0,
	DEBUGBUS_RAS_GC_S_0_I_0,
	DEBUGBUS_LRZ_BR_S_0_I_0,
	DEBUGBUS_LRZ_BV_S_0_I_0,
	DEBUGBUS_VFDP_GC_S_0_I_0,
	DEBUGBUS_GPC_BR_S_0_I_0,
	DEBUGBUS_GPC_BV_S_0_I_0,
	DEBUGBUS_VPCFE_BR_S_0_I_0,
	DEBUGBUS_VPCFE_BV_S_0_I_0,
	DEBUGBUS_VPCBE_BR_S_0_I_0,
	DEBUGBUS_VPCBE_BV_S_0_I_0,
	DEBUGBUS_CCHE_GC_S_0_I_0,
	DEBUGBUS_DBGC_GC_S_0_I_0,
	DEBUGBUS_LARC_GC_S_0_I_0,
	DEBUGBUS_RBBM_GC_S_0_I_0,
	DEBUGBUS_CCRE_GC_S_0_I_0,
	DEBUGBUS_CGC_GC_S_0_I_0,
	DEBUGBUS_GMU_GC_S_0_I_0,
	DEBUGBUS_SLICE_GC_S_0_I_0,
	DEBUGBUS_HLSQ_SPTP_STAR_GC_S_0_I_0,
	DEBUGBUS_USP_GC_S_0_I_0,
	DEBUGBUS_USP_GC_S_0_I_1,
	DEBUGBUS_USPTP_GC_S_0_I_0,
	DEBUGBUS_USPTP_GC_S_0_I_1,
	DEBUGBUS_USPTP_GC_S_0_I_2,
	DEBUGBUS_USPTP_GC_S_0_I_3,
	DEBUGBUS_TP_GC_S_0_I_0,
	DEBUGBUS_TP_GC_S_0_I_1,
	DEBUGBUS_TP_GC_S_0_I_2,
	DEBUGBUS_TP_GC_S_0_I_3,
	DEBUGBUS_RB_GC_S_0_I_0,
	DEBUGBUS_RB_GC_S_0_I_1,
	DEBUGBUS_CCU_GC_S_0_I_0,
	DEBUGBUS_CCU_GC_S_0_I_1,
	DEBUGBUS_HLSQ_GC_S_0_I_0,
	DEBUGBUS_HLSQ_GC_S_0_I_1,
	DEBUGBUS_VFD_GC_S_0_I_0,
	DEBUGBUS_VFD_GC_S_0_I_1,
	DEBUGBUS_CP_GC_S_1_I_0,
	DEBUGBUS_PC_BR_S_1_I_0,
	DEBUGBUS_PC_BV_S_1_I_0,
	DEBUGBUS_TESS_GC_S_1_I_0,
	DEBUGBUS_TSEFE_GC_S_1_I_0,
	DEBUGBUS_TSEBE_GC_S_1_I_0,
	DEBUGBUS_RAS_GC_S_1_I_0,
	DEBUGBUS_LRZ_BR_S_1_I_0,
	DEBUGBUS_LRZ_BV_S_1_I_0,
	DEBUGBUS_VFDP_GC_S_1_I_0,
	DEBUGBUS_GPC_BR_S_1_I_0,
	DEBUGBUS_GPC_BV_S_1_I_0,
	DEBUGBUS_VPCFE_BR_S_1_I_0,
	DEBUGBUS_VPCFE_BV_S_1_I_0,
	DEBUGBUS_VPCBE_BR_S_1_I_0,
	DEBUGBUS_VPCBE_BV_S_1_I_0,
	DEBUGBUS_CCHE_GC_S_1_I_0,
	DEBUGBUS_DBGC_GC_S_1_I_0,
	DEBUGBUS_LARC_GC_S_1_I_0,
	DEBUGBUS_RBBM_GC_S_1_I_0,
	DEBUGBUS_CCRE_GC_S_1_I_0,
	DEBUGBUS_CGC_GC_S_1_I_0,
	DEBUGBUS_GMU_GC_S_1_I_0,
	DEBUGBUS_SLICE_GC_S_1_I_0,
	DEBUGBUS_HLSQ_SPTP_STAR_GC_S_1_I_0,
	DEBUGBUS_USP_GC_S_1_I_0,
	DEBUGBUS_USP_GC_S_1_I_1,
	DEBUGBUS_USPTP_GC_S_1_I_0,
	DEBUGBUS_USPTP_GC_S_1_I_1,
	DEBUGBUS_USPTP_GC_S_1_I_2,
	DEBUGBUS_USPTP_GC_S_1_I_3,
	DEBUGBUS_TP_GC_S_1_I_0,
	DEBUGBUS_TP_GC_S_1_I_1,
	DEBUGBUS_TP_GC_S_1_I_2,
	DEBUGBUS_TP_GC_S_1_I_3,
	DEBUGBUS_RB_GC_S_1_I_0,
	DEBUGBUS_RB_GC_S_1_I_1,
	DEBUGBUS_CCU_GC_S_1_I_0,
	DEBUGBUS_CCU_GC_S_1_I_1,
	DEBUGBUS_HLSQ_GC_S_1_I_0,
	DEBUGBUS_HLSQ_GC_S_1_I_1,
	DEBUGBUS_VFD_GC_S_1_I_0,
	DEBUGBUS_VFD_GC_S_1_I_1,
};

/*
 * Block   : ['AHB_SECURE']
 * REGION  : UNSLICE
 * pairs   : 1 (Regs:3)
 */
static const u32 gen8_9_0_ahb_secure_cp_cp_pipe_none_registers[] = {
	0x0f000, 0x0f002,
	UINT_MAX, UINT_MAX,
};
static_assert(IS_ALIGNED(sizeof(gen8_9_0_ahb_secure_cp_cp_pipe_none_registers), 8));

/*
 * Block   : ['GMUAO']
 * REGION  : UNSLICE
 * Pipeline: PIPE_NONE
 * pairs   : 60 (Regs:87)
 */
static const u32 gen8_9_0_gmuao_registers[] = {
	0x10001, 0x10001, 0x10003, 0x10003, 0x10401, 0x10401, 0x10403, 0x10403,
	0x10801, 0x10801, 0x10803, 0x10803, 0x10c01, 0x10c01, 0x10c03, 0x10c03,
	0x11001, 0x11001, 0x11003, 0x11003, 0x11401, 0x11401, 0x11403, 0x11403,
	0x11801, 0x11801, 0x11803, 0x11803, 0x11c01, 0x11c01, 0x11c03, 0x11c03,
	0x22400, 0x22401, 0x22408, 0x22409, 0x22410, 0x22411, 0x22418, 0x22419,
	0x22420, 0x22421, 0x23801, 0x23801, 0x23803, 0x23803, 0x23805, 0x23805,
	0x23807, 0x23807, 0x23809, 0x23809, 0x2380b, 0x2380b, 0x2380d, 0x2380d,
	0x2380f, 0x2380f, 0x23811, 0x23811, 0x23813, 0x23813, 0x23815, 0x23815,
	0x23817, 0x23817, 0x23819, 0x23819, 0x2381b, 0x2381b, 0x2381d, 0x2381d,
	0x2381f, 0x23820, 0x23822, 0x23822, 0x23824, 0x23824, 0x23826, 0x23826,
	0x23828, 0x23828, 0x2382a, 0x2382a, 0x2382c, 0x2382c, 0x2382e, 0x2382e,
	0x23830, 0x23830, 0x23832, 0x23832, 0x23834, 0x23834, 0x23836, 0x23836,
	0x23838, 0x23838, 0x2383a, 0x2383a, 0x2383c, 0x2383c, 0x2383e, 0x2383e,
	0x23840, 0x23847, 0x23b00, 0x23b01, 0x23b03, 0x23b03, 0x23b05, 0x23b0e,
	0x23b10, 0x23b13, 0x23b15, 0x23b16, 0x23b28, 0x23b28, 0x23b30, 0x23b30,
	UINT_MAX, UINT_MAX,
};
static_assert(IS_ALIGNED(sizeof(gen8_9_0_gmuao_registers), 8));

/*
 * Block   : ['GMUCX', 'GMUCX_RAM']
 * REGION  : UNSLICE
 * Pipeline: PIPE_NONE
 * pairs   : 147 (Regs:741)
 */
static const u32 gen8_9_0_gmucx_registers[] = {
	0x1f400, 0x1f40b, 0x1f40f, 0x1f411, 0x1f500, 0x1f500, 0x1f507, 0x1f507,
	0x1f509, 0x1f50b, 0x1f700, 0x1f701, 0x1f704, 0x1f706, 0x1f708, 0x1f709,
	0x1f70c, 0x1f70d, 0x1f710, 0x1f711, 0x1f713, 0x1f716, 0x1f718, 0x1f71d,
	0x1f720, 0x1f725, 0x1f729, 0x1f729, 0x1f730, 0x1f747, 0x1f750, 0x1f75a,
	0x1f75c, 0x1f75c, 0x1f780, 0x1f781, 0x1f784, 0x1f78b, 0x1f790, 0x1f797,
	0x1f7a0, 0x1f7a7, 0x1f7b0, 0x1f7ba, 0x1f7e0, 0x1f7e1, 0x1f7e4, 0x1f7e5,
	0x1f7e8, 0x1f7e9, 0x1f7ec, 0x1f7ed, 0x1f800, 0x1f804, 0x1f807, 0x1f808,
	0x1f80b, 0x1f80c, 0x1f80f, 0x1f80f, 0x1f811, 0x1f811, 0x1f813, 0x1f817,
	0x1f819, 0x1f81c, 0x1f824, 0x1f830, 0x1f840, 0x1f842, 0x1f848, 0x1f848,
	0x1f84c, 0x1f84c, 0x1f850, 0x1f850, 0x1f858, 0x1f859, 0x1f868, 0x1f869,
	0x1f878, 0x1f883, 0x1f930, 0x1f931, 0x1f934, 0x1f935, 0x1f938, 0x1f939,
	0x1f93c, 0x1f93d, 0x1f940, 0x1f941, 0x1f943, 0x1f943, 0x1f948, 0x1f94a,
	0x1f94f, 0x1f951, 0x1f954, 0x1f955, 0x1f95d, 0x1f95d, 0x1f962, 0x1f96e,
	0x1f970, 0x1f973, 0x1f97c, 0x1f97e, 0x1f980, 0x1f981, 0x1f984, 0x1f986,
	0x1f992, 0x1f993, 0x1f996, 0x1f99e, 0x1f9c0, 0x1f9cf, 0x1f9f0, 0x1f9f1,
	0x1f9f8, 0x1f9fa, 0x1f9fc, 0x1f9fc, 0x1fa00, 0x1fa03, 0x1fc00, 0x1fc01,
	0x1fc04, 0x1fc0d, 0x1fc10, 0x1fc10, 0x1fc14, 0x1fc14, 0x1fc18, 0x1fc19,
	0x1fc20, 0x1fc20, 0x1fc24, 0x1fc26, 0x1fc30, 0x1fc33, 0x1fc38, 0x1fc3b,
	0x1fc40, 0x1fc49, 0x1fc50, 0x1fc59, 0x1fc60, 0x1fc7f, 0x1fca0, 0x1fcef,
	0x1fd80, 0x1fd81, 0x1fd83, 0x1fd84, 0x1fd88, 0x1fda7, 0x1fdb0, 0x1fdb3,
	0x1fdb8, 0x1fdbb, 0x1fdc0, 0x1fdc3, 0x1fdc8, 0x1fdcb, 0x1fdd0, 0x1fdd3,
	0x1fdd8, 0x1fddb, 0x1fde0, 0x1fde3, 0x1fe00, 0x1fe01, 0x1fe03, 0x1fe05,
	0x1fe08, 0x1fe0a, 0x1fe10, 0x1fe12, 0x1fe40, 0x1fe44, 0x1fe70, 0x1fe71,
	0x1fe74, 0x1fe77, 0x1fe88, 0x1fe8b, 0x1fe9c, 0x1fe9c, 0x1fec0, 0x1fec4,
	0x1fef0, 0x1fef1, 0x1fef4, 0x1fef7, 0x1ff08, 0x1ff0b, 0x1ff1c, 0x1ff1c,
	0x1ff20, 0x1ff21, 0x20000, 0x20007, 0x20010, 0x20015, 0x20018, 0x2001a,
	0x2001c, 0x20022, 0x20024, 0x20025, 0x2002a, 0x2002c, 0x20030, 0x20031,
	0x20034, 0x20036, 0x20080, 0x2008a, 0x20120, 0x20120, 0x20128, 0x20128,
	0x20200, 0x20204, 0x20208, 0x20209, 0x20210, 0x20212, 0x20214, 0x20214,
	0x20216, 0x20216, 0x20218, 0x20218, 0x2021a, 0x2021a, 0x20220, 0x20222,
	0x20224, 0x20226, 0x20228, 0x20229, 0x2022c, 0x2022d, 0x20230, 0x2024f,
	0x20260, 0x20261, 0x20268, 0x20269, 0x20270, 0x20272, 0x20278, 0x20279,
	0x2027c, 0x2028c, 0x20300, 0x20301, 0x20304, 0x20305, 0x20308, 0x2030c,
	0x20310, 0x20314, 0x20318, 0x2031a, 0x20320, 0x20322, 0x20324, 0x20326,
	0x20328, 0x2032a, 0x2032c, 0x2032e, 0x20330, 0x20333, 0x20338, 0x20338,
	0x20340, 0x2034f, 0x20380, 0x20385, 0x20400, 0x20400, 0x20404, 0x20404,
	0x20410, 0x20414, 0x20420, 0x20422, 0x20480, 0x2048a,
	 UINT_MAX, UINT_MAX,
};
static_assert(IS_ALIGNED(sizeof(gen8_9_0_gmucx_registers), 8));

/*
 * Block   : ['GDPM_LKG_GMXC']
 * REGION  : UNSLICE
 * Pipeline: PIPE_NONE
 * pairs   : 11 (Regs:131)
 */
static const u32 gen8_9_0_gdpm_lkg_gmxc_registers[] = {
	0x21400, 0x21400, 0x21408, 0x21409, 0x2140c, 0x2140d, 0x21440, 0x21441,
	0x21444, 0x2144b, 0x21480, 0x21488, 0x214a0, 0x214a7, 0x214c0, 0x214c0,
	0x214e0, 0x214e0, 0x21800, 0x21810, 0x21840, 0x2188f,
	UINT_MAX, UINT_MAX,
};
static_assert(IS_ALIGNED(sizeof(gen8_9_0_gdpm_lkg_gmxc_registers), 8));

/*
 * Block   : ['GDPM_LKG']
 * REGION  : UNSLICE
 * Pipeline: PIPE_NONE
 * pairs   : 11 (Regs:131)
 */
static const u32 gen8_9_0_gdpm_lkg_registers[] = {
	0x21c00, 0x21c00, 0x21c08, 0x21c09, 0x21c0c, 0x21c0d, 0x21c40, 0x21c41,
	0x21c44, 0x21c4b, 0x21c80, 0x21c88, 0x21ca0, 0x21ca7, 0x21cc0, 0x21cc0,
	0x21ce0, 0x21ce0, 0x22000, 0x22010, 0x22040, 0x2208f,
	UINT_MAX, UINT_MAX,
};
static_assert(IS_ALIGNED(sizeof(gen8_9_0_gdpm_lkg_registers), 8));

/*
 * Block   : ['CPR_GMXC']
 * REGION  : UNSLICE
 * Pipeline: PIPE_NONE
 * pairs   : 26 (Regs:588)
 */
static const u32 gen8_9_0_cpr_gmxc_registers[] = {
	0x22800, 0x22805, 0x22808, 0x2280c, 0x22814, 0x22814, 0x2281c, 0x2281c,
	0x22820, 0x22838, 0x22840, 0x22840, 0x22848, 0x22848, 0x22850, 0x22850,
	0x22880, 0x2289a, 0x22980, 0x229b0, 0x229c0, 0x229c3, 0x229c5, 0x229c8,
	0x229e0, 0x229f1, 0x229fb, 0x229ff, 0x22a02, 0x22a07, 0x22a09, 0x22a0b,
	0x22a10, 0x22b4f, 0x23000, 0x23014, 0x23031, 0x23040, 0x23480, 0x234a2,
	0x234ac, 0x234c8, 0x234d1, 0x234d6, 0x2358d, 0x2358d, 0x23590, 0x23590,
	0x235a0, 0x235a0, 0x235b0, 0x235b0,
	UINT_MAX, UINT_MAX,
};
static_assert(IS_ALIGNED(sizeof(gen8_9_0_cpr_gmxc_registers), 8));

/*
 * Block   : ['CPR']
 * REGION  : UNSLICE
 * Pipeline: PIPE_NONE
 * pairs   : 26 (Regs:604)
 */
static const u32 gen8_9_0_cpr_registers[] = {
	0x26800, 0x26805, 0x26808, 0x2680d, 0x26814, 0x26815, 0x2681c, 0x2681c,
	0x26820, 0x26839, 0x26840, 0x26841, 0x26848, 0x26849, 0x26850, 0x26851,
	0x26880, 0x268a4, 0x26980, 0x269b0, 0x269c0, 0x269c3, 0x269c5, 0x269c8,
	0x269e0, 0x269f1, 0x269fb, 0x269ff, 0x26a02, 0x26a07, 0x26a09, 0x26a0b,
	0x26a10, 0x26b4f, 0x27000, 0x27014, 0x27031, 0x27040, 0x27480, 0x274a2,
	0x274ac, 0x274c8, 0x274d1, 0x274d6, 0x2758d, 0x2758d, 0x27590, 0x27590,
	0x275a0, 0x275a0, 0x275b0, 0x275b0,
	UINT_MAX, UINT_MAX,
};
static_assert(IS_ALIGNED(sizeof(gen8_9_0_cpr_registers), 8));

/*
 * Before dumping the CP Mempool over the CP_*_MEM_POOL_DBG_ADDR/DATA
 * indexed register pair it must be stabilized.
 * for p in [CP_PIPE_BR, CP_PIPE_BV]:
 *   Program CP_APERTURE_CNTL_* with pipeID={p} sliceID={MAX_UINT}
 *   Program CP_CHICKEN_DBG_PIPE[crashStabilizeMVC] bit = 1.
 *   Dump CP_MEM_POOL_DBG_ADDR_PIPE for pipe=p
 *   Program CP_CHICKEN_DBG_PIPE[crashStabilizeMVC] bit = 0.
 *
 * same thing for CP_SLICE_MEM_POOL_DBG_ADDR_PIPE
 * for p in [CP_PIPE_BR, CP_PIPE_BV]:
 *   for s in [0,1,2]:
 *     Program CP_APERTURE_CNTL_* with pipeID={p} sliceID={s}
 *     Program CP_CHICKEN_DBG_PIPE[crashStabilizeMVC] bit = 1.
 *     Program CP_SLICE_CHICKEN_DBG[crashStabilizeMVC] bit = 1.
 *     Dump CP_SLICE_MEM_POOL_DBG_ADDR_PIPE for pipe=p, sliceID=s
 *     Program CP_CHICKEN_DBG_PIPE[crashStabilizeMVC] bit = 0.
 *     Program CP_SLICE_CHICKEN_DBG[crashStabilizeMVC] bit = 0.
 */

static struct gen8_reg_list gen8_9_0_ahb_registers[] = {
	{ UNSLICE, gen8_2_0_gbif_registers },
	{ UNSLICE, gen8_2_0_ahb_precd_gpu_registers },
	{ SLICE, gen8_2_0_ahb_precd_gpu_slice_registers },
	{ UNSLICE, gen8_2_0_ahb_secure_gpu_registers },
	{ UNSLICE, gen8_9_0_ahb_secure_cp_cp_pipe_none_registers },
};

static struct gen8_reg_list gen8_9_0_gmu_registers[] = {
	{ UNSLICE, gen8_2_0_gmugx_registers },
	{ SLICE, gen8_2_0_gmugx_slice_registers },
	{ UNSLICE, gen8_9_0_gmuao_registers },
	{ UNSLICE, gen8_9_0_gmucx_registers },
};

/*
 * Block   : ['GPU_CC_GPU_CC_REG']
 * REGION  : UNSLICE
 * Pipeline: PIPE_NONE
 * pairs   : 26 (Regs:134)
 */
static const u32 gen8_9_0_gpu_cc_gpu_cc_reg_registers[] = {
	0x25000, 0x25002, 0x25400, 0x25404, 0x25800, 0x25804, 0x25c00, 0x25c04,
	0x26000, 0x26004, 0x26400, 0x26406, 0x26415, 0x2641d, 0x2641f, 0x2643f,
	0x26442, 0x26443, 0x26478, 0x2647a, 0x26489, 0x2648c, 0x2649c, 0x2649e,
	0x264a0, 0x264a1, 0x264c5, 0x264c7, 0x264e8, 0x264e9, 0x264f9, 0x264fd,
	0x2650c, 0x2650c, 0x2651c, 0x2651e, 0x26540, 0x2654b, 0x26554, 0x26556,
	0x26558, 0x2655c, 0x2655e, 0x2655f, 0x26563, 0x26563, 0x2656d, 0x26573,
	0x26576, 0x26576, 0x26578, 0x2657a,
	UINT_MAX, UINT_MAX,
};
static_assert(IS_ALIGNED(sizeof(gen8_9_0_gpu_cc_gpu_cc_reg_registers), 8));

static const u32 *gen8_9_0_external_core_regs[] = {
	gen8_2_0_gpu_cc_ahb2phy_broadcast_swman_registers,
	gen8_9_0_gdpm_lkg_gmxc_registers,
	gen8_9_0_gdpm_lkg_registers,
	gen8_9_0_cpr_registers,
	gen8_9_0_cpr_gmxc_registers,
	gen8_2_0_gpu_cc_ahb2phy_swman_registers,
	gen8_9_0_gpu_cc_gpu_cc_reg_registers,
	gen8_2_0_gpu_cc_pll0_cm_pll_taycan_common_registers,
};

#endif /* __ADRENO_GEN8_9_0_SNAPSHOT_H */
