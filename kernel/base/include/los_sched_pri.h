/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
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

#ifndef _LOS_SCHED_PRI_H
#define _LOS_SCHED_PRI_H

#include "los_sortlink_pri.h"
#include "los_sys_pri.h"
#include "los_hwi.h"
#include "hal_timer.h"
#ifdef LOSCFG_SCHED_DEBUG
#include "los_statistics_pri.h"
#endif
#include "los_stackinfo_pri.h"
#include "los_futex_pri.h"
#include "los_pm_pri.h"
#include "los_signal.h"
#ifdef LOSCFG_KERNEL_CPUP
#include "los_cpup_pri.h"
#endif
#ifdef LOSCFG_KERNEL_LITEIPC
#include "hm_liteipc.h"
#endif
#include "los_mp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define OS_SCHED_MINI_PERIOD          (OS_SYS_CLOCK / LOSCFG_BASE_CORE_TICK_PER_SECOND_MINI)
#define OS_TICK_RESPONSE_PRECISION    (UINT32)((OS_SCHED_MINI_PERIOD * 75) / 100)
#define OS_SCHED_MAX_RESPONSE_TIME    OS_SORT_LINK_INVALID_TIME
#define OS_SCHED_TICK_TO_CYCLE(ticks) ((UINT64)ticks * OS_CYCLE_PER_TICK)
#define AFFI_MASK_TO_CPUID(mask)      ((UINT16)((mask) - 1))

extern UINT32 g_taskScheduled;
#define OS_SCHEDULER_ACTIVE (g_taskScheduled & (1U << ArchCurrCpuid()))
#define OS_SCHEDULER_ALL_ACTIVE (g_taskScheduled == LOSCFG_KERNEL_CPU_MASK)

typedef struct TagTaskCB LosTaskCB;
typedef BOOL (*SCHED_TL_FIND_FUNC)(UINTPTR, UINTPTR);

STATIC INLINE UINT64 OsGetCurrSchedTimeCycle(VOID)
{
    return HalClockGetCycles();
}

typedef enum {
    INT_NO_RESCH = 0x0,   /* no needs to schedule */
    INT_PEND_RESCH = 0x1, /* pending schedule flag */
    INT_PEND_TICK = 0x2,  /* pending tick */
} SchedFlag;

#define OS_PRIORITY_QUEUE_NUM 32
typedef struct {
    LOS_DL_LIST priQueList[OS_PRIORITY_QUEUE_NUM];
    UINT32      readyTasks[OS_PRIORITY_QUEUE_NUM];
    UINT32      queueBitmap;
} HPFQueue;

typedef struct {
    HPFQueue queueList[OS_PRIORITY_QUEUE_NUM];
    UINT32   queueBitmap;
} HPFRunqueue;

typedef struct {
    SortLinkAttribute timeoutQueue; /* task timeout queue */
    HPFRunqueue       *hpfRunqueue;
    UINT64            responseTime; /* Response time for current CPU tick interrupts */
    UINT32            responseID;   /* The response ID of the current CPU tick interrupt */
    UINT32            idleTaskID;   /* idle task id */
    UINT32            taskLockCnt;  /* task lock flag */
    UINT32            schedFlag;    /* pending scheduler flag */
} SchedRunqueue;

extern SchedRunqueue g_schedRunqueue[LOSCFG_KERNEL_CORE_NUM];

VOID OsSchedExpireTimeUpdate(VOID);

STATIC INLINE SchedRunqueue *OsSchedRunqueue(VOID)
{
    return &g_schedRunqueue[ArchCurrCpuid()];
}

STATIC INLINE SchedRunqueue *OsSchedRunqueueByID(UINT16 id)
{
    return &g_schedRunqueue[id];
}

STATIC INLINE UINT32 OsSchedLockCountGet(VOID)
{
    return OsSchedRunqueue()->taskLockCnt;
}

STATIC INLINE VOID OsSchedLockSet(UINT32 count)
{
    OsSchedRunqueue()->taskLockCnt = count;
}

STATIC INLINE VOID OsSchedLock(VOID)
{
    OsSchedRunqueue()->taskLockCnt++;
}

STATIC INLINE VOID OsSchedUnlock(VOID)
{
    OsSchedRunqueue()->taskLockCnt--;
}

STATIC INLINE BOOL OsSchedUnlockResch(VOID)
{
    SchedRunqueue *rq = OsSchedRunqueue();
    if (rq->taskLockCnt > 0) {
        rq->taskLockCnt--;
        if ((rq->taskLockCnt == 0) && (rq->schedFlag & INT_PEND_RESCH) && OS_SCHEDULER_ACTIVE) {
            return TRUE;
        }
    }

    return FALSE;
}

STATIC INLINE BOOL OsSchedIsLock(VOID)
{
    return (OsSchedRunqueue()->taskLockCnt != 0);
}

/* Check if preemptible with counter flag */
STATIC INLINE BOOL OsPreemptable(VOID)
{
    SchedRunqueue *rq = OsSchedRunqueue();
    /*
     * Unlike OsPreemptableInSched, the int may be not disabled when OsPreemptable
     * is called, needs manually disable interrupt, to prevent current task from
     * being migrated to another core, and get the wrong preemptable status.
     */
    UINT32 intSave = LOS_IntLock();
    BOOL preemptible = (rq->taskLockCnt == 0);
    if (!preemptible) {
        /* Set schedule flag if preemption is disabled */
        rq->schedFlag |= INT_PEND_RESCH;
    }
    LOS_IntRestore(intSave);
    return preemptible;
}

STATIC INLINE BOOL OsPreemptableInSched(VOID)
{
    BOOL preemptible = FALSE;
    SchedRunqueue *rq = OsSchedRunqueue();

#ifdef LOSCFG_KERNEL_SMP
    /*
     * For smp systems, schedule must hold the task spinlock, and this counter
     * will increase by 1 in that case.
     */
    preemptible = (rq->taskLockCnt == 1);

#else
    preemptible = (rq->taskLockCnt == 0);
#endif
    if (!preemptible) {
        /* Set schedule flag if preemption is disabled */
        rq->schedFlag |= INT_PEND_RESCH;
    }

    return preemptible;
}

STATIC INLINE UINT32 OsSchedRunqueueIdleGet(VOID)
{
    return OsSchedRunqueue()->idleTaskID;
}

STATIC INLINE VOID OsSchedRunqueuePendingSet(VOID)
{
    OsSchedRunqueue()->schedFlag |= INT_PEND_RESCH;
}

#define LOS_SCHED_NORMAL  0U
#define LOS_SCHED_FIFO    1U
#define LOS_SCHED_RR      2U
#define LOS_SCHED_IDLE    3U

typedef struct {
    UINT16 policy;
    UINT16 basePrio;
    UINT16 priority;
    UINT32 timeSlice;
} SchedParam;

typedef struct {
    UINT16  policy; /* This field must be present for all scheduling policies and must be the first in the structure */
    UINT16  basePrio;
    UINT16  priority;
    UINT32  initTimeSlice;
    UINT32  priBitmap; /**< Bitmap for recording the change of task priority, the priority can not be greater than 31 */
} SchedHPF;

typedef struct {
    union {
        SchedHPF hpf;
    } Policy;
} SchedPolicy;

typedef struct {
    VOID (*dequeue)(SchedRunqueue *rq, LosTaskCB *taskCB);
    VOID (*enqueue)(SchedRunqueue *rq, LosTaskCB *taskCB);
    VOID (*start)(SchedRunqueue *rq, LosTaskCB *taskCB);
    VOID (*exit)(LosTaskCB *taskCB);
    UINT32 (*wait)(LosTaskCB *runTask, LOS_DL_LIST *list, UINT32 timeout);
    VOID (*wake)(LosTaskCB *taskCB);
    BOOL (*schedParamModify)(LosTaskCB *taskCB, const SchedParam *param);
    UINT32 (*schedParamGet)(const LosTaskCB *taskCB, SchedParam *param);
    UINT32 (*delay)(LosTaskCB *taskCB, UINT64 waitTime);
    VOID (*yield)(LosTaskCB *taskCB);
    UINT32 (*suspend)(LosTaskCB *taskCB);
    UINT32 (*resume)(LosTaskCB *taskCB, BOOL *needSched);
    UINT64 (*deadlineGet)(const LosTaskCB *taskCB);
    VOID (*timeSliceUpdate)(SchedRunqueue *rq, LosTaskCB *taskCB, UINT64 currTime);
    INT32 (*schedParamCompare)(const SchedPolicy *sp1, const SchedPolicy *sp2);
    VOID (*priorityInheritance)(LosTaskCB *owner, const SchedParam *param);
    VOID (*priorityRestore)(LosTaskCB *owner, const LOS_DL_LIST *list, const SchedParam *param);
} SchedOps;

/**
 * @ingroup los_sched
 * Define a usable task priority.
 *
 * Highest task priority.
 */
#define OS_TASK_PRIORITY_HIGHEST    0

/**
 * @ingroup los_sched
 * Define a usable task priority.
 *
 * Lowest task priority.
 */
#define OS_TASK_PRIORITY_LOWEST     31

/**
 * @ingroup los_sched
 * Flag that indicates the task or task control block status.
 *
 * The task is init.
 */
#define OS_TASK_STATUS_INIT         0x0001U

/**
 * @ingroup los_sched
 * Flag that indicates the task or task control block status.
 *
 * The task is ready.
 */
#define OS_TASK_STATUS_READY        0x0002U

/**
 * @ingroup los_sched
 * Flag that indicates the task or task control block status.
 *
 * The task is running.
 */
#define OS_TASK_STATUS_RUNNING      0x0004U

/**
 * @ingroup los_sched
 * Flag that indicates the task or task control block status.
 *
 * The task is suspended.
 */
#define OS_TASK_STATUS_SUSPENDED    0x0008U

/**
 * @ingroup los_sched
 * Flag that indicates the task or task control block status.
 *
 * The task is blocked.
 */
#define OS_TASK_STATUS_PENDING      0x0010U

/**
 * @ingroup los_sched
 * Flag that indicates the task or task control block status.
 *
 * The task is delayed.
 */
#define OS_TASK_STATUS_DELAY        0x0020U

/**
 * @ingroup los_sched
 * Flag that indicates the task or task control block status.
 *
 * The time for waiting for an event to occur expires.
 */
#define OS_TASK_STATUS_TIMEOUT      0x0040U

/**
 * @ingroup los_sched
 * Flag that indicates the task or task control block status.
 *
 * The task is pend for a period of time.
 */
#define OS_TASK_STATUS_PEND_TIME    0x0080U

/**
 * @ingroup los_sched
 * Flag that indicates the task or task control block status.
 *
 * The task is exit.
 */
#define OS_TASK_STATUS_EXIT         0x0100U

#define OS_TASK_STATUS_BLOCKED     (OS_TASK_STATUS_INIT | OS_TASK_STATUS_PENDING | \
                                    OS_TASK_STATUS_DELAY | OS_TASK_STATUS_PEND_TIME)

/**
 * @ingroup los_task
 * Flag that indicates the task or task control block status.
 *
 * The delayed operation of this task is frozen.
 */
#define OS_TASK_STATUS_FROZEN       0x0200U

#define OS_TCB_NAME_LEN             32

typedef struct TagTaskCB {
    VOID            *stackPointer;      /**< Task stack pointer */
    UINT16          taskStatus;         /**< Task status */

    UINT64          startTime;          /**< The start time of each phase of task */
    UINT64          waitTime;           /**< Task delay time, tick number */
    UINT64          irqStartTime;       /**< Interrupt start time */
    UINT32          irqUsedTime;        /**< Interrupt consumption time */
    INT32           timeSlice;          /**< Task remaining time slice */
    SortLinkList    sortList;           /**< Task sortlink node */
    const SchedOps  *ops;
    SchedPolicy     sp;

    UINT32          stackSize;          /**< Task stack size */
    UINTPTR         topOfStack;         /**< Task stack top */
    UINT32          taskID;             /**< Task ID */
    TSK_ENTRY_FUNC  taskEntry;          /**< Task entrance function */
    VOID            *joinRetval;        /**< pthread adaption */
    VOID            *taskMux;           /**< Task-held mutex */
    VOID            *taskEvent;         /**< Task-held event */
    UINTPTR         args[4];            /**< Parameter, of which the maximum number is 4 */
    CHAR            taskName[OS_TCB_NAME_LEN]; /**< Task name */
    LOS_DL_LIST     pendList;           /**< Task pend node */
    LOS_DL_LIST     threadList;         /**< thread list */
    UINT32          eventMask;          /**< Event mask */
    UINT32          eventMode;          /**< Event mode */
#ifdef LOSCFG_KERNEL_CPUP
    OsCpupBase      taskCpup;           /**< task cpu usage */
#endif
    INT32           errorNo;            /**< Error Num */
    UINT32          signal;             /**< Task signal */
    sig_cb          sig;
#ifdef LOSCFG_KERNEL_SMP
    UINT16          currCpu;            /**< CPU core number of this task is running on */
    UINT16          lastCpu;            /**< CPU core number of this task is running on last time */
    UINT16          cpuAffiMask;        /**< CPU affinity mask, support up to 16 cores */
#ifdef LOSCFG_KERNEL_SMP_TASK_SYNC
    UINT32          syncSignal;         /**< Synchronization for signal handling */
#endif
#ifdef LOSCFG_KERNEL_SMP_LOCKDEP
    LockDep         lockDep;
#endif
#endif
#ifdef LOSCFG_SCHED_DEBUG
    SchedStat       schedStat;          /**< Schedule statistics */
#endif
#ifdef LOSCFG_KERNEL_VM
    UINTPTR         archMmu;
    UINTPTR         userArea;
    UINTPTR         userMapBase;
    UINT32          userMapSize;        /**< user thread stack size ,real size : userMapSize + USER_STACK_MIN_SIZE */
    FutexNode       futex;
#endif
    UINT32          processID;          /**< Which belong process */
    LOS_DL_LIST     joinList;           /**< join list */
    LOS_DL_LIST     lockList;           /**< Hold the lock list */
    UINTPTR         waitID;             /**< Wait for the PID or GID of the child process */
    UINT16          waitFlag;           /**< The type of child process that is waiting, belonging to a group or parent,
                                             a specific child process, or any child process */
#ifdef LOSCFG_KERNEL_LITEIPC
    IpcTaskInfo     *ipcTaskInfo;
#endif
#ifdef LOSCFG_KERNEL_PERF
    UINTPTR         pc;
    UINTPTR         fp;
#endif
} LosTaskCB;

STATIC INLINE BOOL OsTaskIsRunning(const LosTaskCB *taskCB)
{
    return ((taskCB->taskStatus & OS_TASK_STATUS_RUNNING) != 0);
}

STATIC INLINE BOOL OsTaskIsReady(const LosTaskCB *taskCB)
{
    return ((taskCB->taskStatus & OS_TASK_STATUS_READY) != 0);
}

STATIC INLINE BOOL OsTaskIsInactive(const LosTaskCB *taskCB)
{
    return ((taskCB->taskStatus & (OS_TASK_STATUS_INIT | OS_TASK_STATUS_EXIT)) != 0);
}

STATIC INLINE BOOL OsTaskIsPending(const LosTaskCB *taskCB)
{
    return ((taskCB->taskStatus & OS_TASK_STATUS_PENDING) != 0);
}

STATIC INLINE BOOL OsTaskIsSuspended(const LosTaskCB *taskCB)
{
    return ((taskCB->taskStatus & OS_TASK_STATUS_SUSPENDED) != 0);
}

STATIC INLINE BOOL OsTaskIsBlocked(const LosTaskCB *taskCB)
{
    return ((taskCB->taskStatus & (OS_TASK_STATUS_SUSPENDED | OS_TASK_STATUS_PENDING | OS_TASK_STATUS_DELAY)) != 0);
}

STATIC INLINE LosTaskCB *OsCurrTaskGet(VOID)
{
    return (LosTaskCB *)ArchCurrTaskGet();
}

STATIC INLINE VOID OsCurrTaskSet(LosTaskCB *task)
{
    ArchCurrTaskSet(task);
}

STATIC INLINE VOID OsCurrUserTaskSet(UINTPTR thread)
{
    ArchCurrUserTaskSet(thread);
}

STATIC INLINE VOID OsSchedIrqUsedTimeUpdate(VOID)
{
    LosTaskCB *runTask = OsCurrTaskGet();
    runTask->irqUsedTime = OsGetCurrSchedTimeCycle() - runTask->irqStartTime;
}

STATIC INLINE VOID OsSchedIrqStartTime(VOID)
{
    LosTaskCB *runTask = OsCurrTaskGet();
    runTask->irqStartTime = OsGetCurrSchedTimeCycle();
}

#ifdef LOSCFG_KERNEL_SMP
STATIC INLINE VOID IdleRunqueueFind(UINT16 *idleCpuid)
{
    SchedRunqueue *idleRq = OsSchedRunqueueByID(0);
    UINT32 nodeNum = OsGetSortLinkNodeNum(&idleRq->timeoutQueue);
    UINT16 cpuid = 1;
    do {
        SchedRunqueue *rq = OsSchedRunqueueByID(cpuid);
        UINT32 temp = OsGetSortLinkNodeNum(&rq->timeoutQueue);
        if (nodeNum > temp) {
            *idleCpuid = cpuid;
            nodeNum = temp;
        }
        cpuid++;
    } while (cpuid < LOSCFG_KERNEL_CORE_NUM);
}
#endif

STATIC INLINE VOID OsSchedTimeoutQueueAdd(LosTaskCB *taskCB, UINT64 responseTime)
{
#ifdef LOSCFG_KERNEL_SMP
    UINT16 cpuid = AFFI_MASK_TO_CPUID(taskCB->cpuAffiMask);
    if (cpuid >= LOSCFG_KERNEL_CORE_NUM) {
        cpuid = 0;
        IdleRunqueueFind(&cpuid);
    }
#else
    UINT16 cpuid = 0;
#endif

    SchedRunqueue *rq = OsSchedRunqueueByID(cpuid);
    OsAdd2SortLink(&rq->timeoutQueue, &taskCB->sortList, responseTime, cpuid);
#ifdef LOSCFG_KERNEL_SMP
    if ((cpuid != ArchCurrCpuid()) && (responseTime < rq->responseTime)) {
        rq->schedFlag |= INT_PEND_TICK;
        LOS_MpSchedule(CPUID_TO_AFFI_MASK(cpuid));
    }
#endif
}

STATIC INLINE VOID OsSchedTimeoutQueueDelete(LosTaskCB *taskCB)
{
    SortLinkList *node = &taskCB->sortList;
#ifdef LOSCFG_KERNEL_SMP
    SchedRunqueue *rq = OsSchedRunqueueByID(node->cpuid);
#else
    SchedRunqueue *rq = OsSchedRunqueueByID(0);
#endif
    UINT64 oldResponseTime = GET_SORTLIST_VALUE(node);
    OsDeleteFromSortLink(&rq->timeoutQueue, node);
    if (oldResponseTime <= rq->responseTime) {
        rq->responseTime = OS_SCHED_MAX_RESPONSE_TIME;
    }
}

STATIC INLINE UINT32 OsSchedTimeoutQueueAdjust(LosTaskCB *taskCB, UINT64 responseTime)
{
    UINT32 ret;
    SortLinkList *node = &taskCB->sortList;
#ifdef LOSCFG_KERNEL_SMP
    UINT16 cpuid = node->cpuid;
#else
    UINT16 cpuid = 0;
#endif
    SchedRunqueue *rq = OsSchedRunqueueByID(cpuid);
    ret = OsSortLinkAdjustNodeResponseTime(&rq->timeoutQueue, node, responseTime);
    if (ret == LOS_OK) {
        rq->schedFlag |= INT_PEND_TICK;
    }
    return ret;
}

STATIC INLINE VOID SchedTaskFreeze(LosTaskCB *taskCB)
{
    UINT64 responseTime;

    if (!OsIsPmMode()) {
        return;
    }

    if (!(taskCB->taskStatus & (OS_TASK_STATUS_PEND_TIME | OS_TASK_STATUS_DELAY))) {
        return;
    }

    responseTime = GET_SORTLIST_VALUE(&taskCB->sortList);
    OsSchedTimeoutQueueDelete(taskCB);
    SET_SORTLIST_VALUE(&taskCB->sortList, responseTime);
    taskCB->taskStatus |= OS_TASK_STATUS_FROZEN;
    return;
}

STATIC INLINE VOID SchedTaskUnfreeze(LosTaskCB *taskCB)
{
    UINT64 currTime, responseTime;

    if (!(taskCB->taskStatus & OS_TASK_STATUS_FROZEN)) {
        return;
    }

    taskCB->taskStatus &= ~OS_TASK_STATUS_FROZEN;
    currTime = OsGetCurrSchedTimeCycle();
    responseTime = GET_SORTLIST_VALUE(&taskCB->sortList);
    if (responseTime > currTime) {
        OsSchedTimeoutQueueAdd(taskCB, responseTime);
        return;
    }

    SET_SORTLIST_VALUE(&taskCB->sortList, OS_SORT_LINK_INVALID_TIME);
    if (taskCB->taskStatus & OS_TASK_STATUS_PENDING) {
        LOS_ListDelete(&taskCB->pendList);
    }
    taskCB->taskStatus &= ~OS_TASK_STATUS_BLOCKED;
    return;
}

/*
 * Schedule flag, one bit represents one core.
 * This flag is used to prevent kernel scheduling before OSStartToRun.
 */
#define OS_SCHEDULER_SET(cpuid) do {     \
    g_taskScheduled |= (1U << (cpuid));  \
} while (0);

#define OS_SCHEDULER_CLR(cpuid) do {     \
    g_taskScheduled &= ~(1U << (cpuid)); \
} while (0);

STATIC INLINE LosTaskCB *HPFRunqueueTopTaskGet(HPFRunqueue *rq)
{
    LosTaskCB *newTask = NULL;
    UINT32 baseBitmap = rq->queueBitmap;
#ifdef LOSCFG_KERNEL_SMP
    UINT32 cpuid = ArchCurrCpuid();
#endif

    while (baseBitmap) {
        UINT32 basePrio = CLZ(baseBitmap);
        HPFQueue *queueList = &rq->queueList[basePrio];
        UINT32 bitmap = queueList->queueBitmap;
        while (bitmap) {
            UINT32 priority = CLZ(bitmap);
            LOS_DL_LIST_FOR_EACH_ENTRY(newTask, &queueList->priQueList[priority], LosTaskCB, pendList) {
#ifdef LOSCFG_KERNEL_SMP
                if (newTask->cpuAffiMask & (1U << cpuid)) {
#endif
                    return newTask;
#ifdef LOSCFG_KERNEL_SMP
                }
#endif
            }
            bitmap &= ~(1U << (OS_PRIORITY_QUEUE_NUM - priority - 1));
        }
        baseBitmap &= ~(1U << (OS_PRIORITY_QUEUE_NUM - basePrio - 1));
    }

    return NULL;
}

VOID HPFSchedPolicyInit(SchedRunqueue *rq);
VOID HPFTaskSchedParamInit(LosTaskCB *taskCB, UINT16 policy,
                           const SchedParam *parentParam, const TSK_INIT_PARAM_S *param);
VOID HPFProcessDefaultSchedParamGet(SchedParam *param);

VOID IdleTaskSchedParamInit(LosTaskCB *taskCB);

INT32 OsSchedParamCompare(const LosTaskCB *task1, const LosTaskCB *task2);
VOID OsSchedPriorityInheritance(LosTaskCB *owner, const SchedParam *param);
UINT32 OsSchedParamInit(LosTaskCB *taskCB, UINT16 policy,
                        const SchedParam *parentParam, const TSK_INIT_PARAM_S *param);
VOID OsSchedProcessDefaultSchedParamGet(UINT16 policy, SchedParam *param);

VOID OsSchedResponseTimeReset(UINT64 responseTime);
VOID OsSchedToUserReleaseLock(VOID);
VOID OsSchedTick(VOID);
UINT32 OsSchedInit(VOID);
VOID OsSchedStart(VOID);

VOID OsSchedRunqueueIdleInit(UINT32 idleTaskID);
VOID OsSchedRunqueueInit(VOID);

/*
 * This function simply picks the next task and switches to it.
 * Current task needs to already be in the right state or the right
 * queues it needs to be in.
 */
VOID OsSchedResched(VOID);
VOID OsSchedIrqEndCheckNeedSched(VOID);

/*
* This function inserts the runTask to the lock pending list based on the
* task priority.
*/
LOS_DL_LIST *OsSchedLockPendFindPos(const LosTaskCB *runTask, LOS_DL_LIST *lockList);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _LOS_SCHED_PRI_H */
