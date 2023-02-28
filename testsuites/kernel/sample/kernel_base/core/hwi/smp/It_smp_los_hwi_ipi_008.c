/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "It_smp_hwi.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */
#if (LOSCFG_KERNEL_SMP == YES)
static UINT32 g_targetCpuid;

static VOID SwtmrF01(void)
{
    LOS_AtomicDec(&g_testCount);
}
static VOID HwiF01(void)
{
    UINT32 ret, id;

    id = ArchCurrCpuid() % 3; // 3, Get the current CPU
    switch (id) {
        case 0:
            LOS_SwtmrStart(g_swTmrID);
            break;
        case 1:
            LOS_SwtmrDelete(g_swTmrID);
            break;
        case 2: // 2, when id is 2
            LOS_SwtmrStop(g_swTmrID);
            break;
        default:
            break;
    }
    LOS_AtomicInc(&g_testCount);
    TestDumpCpuid();
    return;
}

static VOID TaskF01(void)
{
    HalIrqUnmask(HWI_NUM_INT11);
    return;
}

static UINT32 Testcase(void)
{
    TSK_INIT_PARAM_S testTask;
    UINT32 ret, testid;
    UINT32 i;

    g_testCount = 0;
    g_testPeriod = 10; // 10, Timeout interval of a periodic software timer.
    g_targetCpuid = 0;

    HalIrqUnmask(HWI_NUM_INT11);

    ret = LOS_HwiCreate(HWI_NUM_INT11, 1, 0, (HWI_PROC_FUNC)HwiF01, 0);
    ICUNIT_ASSERT_EQUAL(ret, LOS_OK, ret);

    for (i = 0; i < LOSCFG_KERNEL_CORE_NUM; i++) {
        TEST_TASK_PARAM_INIT_AFFI(testTask, "it_hwi_001_task", TaskF01, TASK_PRIO_TEST - 1,
            CPUID_TO_AFFI_MASK((ArchCurrCpuid() + i + 1) %
            LOSCFG_KERNEL_CORE_NUM)); // let current cpu 's task create at the last
        ret = LOS_TaskCreate(&testid, &testTask);
        ICUNIT_ASSERT_EQUAL(ret, LOS_OK, ret);
    }
    g_targetCpuid = (1 << LOSCFG_KERNEL_CORE_NUM) - 1; // all cpu

    LOS_TaskDelay(1);
    for (i = 0; i < LOOP; i++) {
        ret = LOS_SwtmrCreate(g_testPeriod, LOS_SWTMR_MODE_PERIOD, (SWTMR_PROC_FUNC)SwtmrF01, &g_swTmrID, 0);
        ICUNIT_ASSERT_EQUAL(ret, LOS_OK, ret);

        HalIrqSendIpi(g_targetCpuid, HWI_NUM_INT11);

        LOS_TaskDelay(1);

        ICUNIT_GOTO_EQUAL(g_testCount, LOSCFG_KERNEL_CORE_NUM * (i + 1), g_testCount, EXIT);
        LOS_SwtmrDelete(g_swTmrID);
    }
    TEST_HwiDelete(HWI_NUM_INT11);
    return LOS_OK;
EXIT:
    TEST_HwiDelete(HWI_NUM_INT11);
    LOS_SwtmrDelete(g_swTmrID);
    return LOS_OK;
}

VOID ItSmpLosHwiIpi008(VOID)
{
    TEST_ADD_CASE("ItSmpLosHwiIpi008", Testcase, TEST_LOS, TEST_HWI, TEST_LEVEL1, TEST_FUNCTION);
}
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */
