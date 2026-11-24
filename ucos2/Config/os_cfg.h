/*
*********************************************************************************************************
*                                              uC/OS-II
*                                        实时操作系统内核
*
*                    Copyright 1992-2021 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               本软件遵循开源许可证，由 Silicon Laboratories Inc. 根据 Apache 许可证条款分发，
*                    版本 2.0，详见 www.apache.org/licenses/LICENSE-2.0。
*
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*
*                                 uC/OS-II 配置文件 for V2.9x
*
* 文件名：os_cfg.h
* 版本：V2.93.01
*********************************************************************************************************
*/

#ifndef OS_CFG_H
#define OS_CFG_H


                                       /* ---------------------- 杂项配置 ----------------------- */
#define OS_APP_HOOKS_EN           0u   /* 应用程序定义的钩子函数从 uC/OS-II 钩子中调用 ;1*/
#define OS_ARG_CHK_EN             1u   /* 启用 (1) 或禁用 (0) 参数检查 */
#define OS_CPU_HOOKS_EN           1u   /* uC/OS-II 钩子函数在处理器移植文件中找到 */

#define OS_DEBUG_EN               0u   /* 启用 (1) 调试变量 ;1*/

#define OS_EVENT_MULTI_EN         0u   /* 包含 OSEventPendMulti() 的代码 ;1*/
#define OS_EVENT_NAME_EN          1u   /* 为信号量、互斥量、邮箱和队列启用名称 */

#define OS_LOWEST_PRIO           63u   /* 定义可分配的最低优先级... ;63*/
                                       /* ... 绝不能高于 254！ */

#define OS_MAX_EVENTS            10u   /* 应用程序中事件控制块的最大数量 */
#define OS_MAX_FLAGS              5u   /* 应用程序中事件标志组的最大数量 */
#define OS_MAX_MEM_PART           5u   /* 内存分区的最大数量 */
#define OS_MAX_QS                 4u   /* 应用程序中队列控制块的最大数量 */
#define OS_MAX_TASKS             16u   /* 应用程序中任务的最大数量，必须 >= 2 ;20*/

#define OS_SCHED_LOCK_EN          1u   /* 包含 OSSchedLock() 和 OSSchedUnlock() 的代码 */

#define OS_TICK_STEP_EN           1u   /* 为 uC/OS-View 启用滴答步进功能 */
#define OS_TICKS_PER_SEC        1000u   /* 设置一秒中的滴答数 ;100*/

#define OS_TLS_TBL_SIZE           0u   /* 线程本地存储表的大小 */


                                       /* --------------------- 任务栈大小 ---------------------- */
#define OS_TASK_TMR_STK_SIZE    128u   /* 定时器任务栈大小 (# 个 OS_STK 宽条目) */
#define OS_TASK_STAT_STK_SIZE   128u   /* 统计任务栈大小 (# 个 OS_STK 宽条目) */
#define OS_TASK_IDLE_STK_SIZE   128u   /* 空闲任务栈大小 (# 个 OS_STK 宽条目) */


                                       /* --------------------- 任务管理 ---------------------- */
#define OS_TASK_CHANGE_PRIO_EN    1u   /*     包含 OSTaskChangePrio() 的代码 */
#define OS_TASK_CREATE_EN         1u   /*     包含 OSTaskCreate() 的代码 */
#define OS_TASK_CREATE_EXT_EN     1u   /*     包含 OSTaskCreateExt() 的代码 */
#define OS_TASK_DEL_EN            1u   /*     包含 OSTaskDel() 的代码 */
#define OS_TASK_NAME_EN           1u   /*     启用任务名称 */
#define OS_TASK_PROFILE_EN        1u   /*     在 OS_TCB 中包含用于性能分析的变量 */
#define OS_TASK_QUERY_EN          1u   /*     包含 OSTaskQuery() 的代码 */
#define OS_TASK_REG_TBL_SIZE      1u   /*     任务变量数组的大小 (# 个 INT32U 条目) */
#define OS_TASK_STAT_EN           1u   /*     启用 (1) 或禁用 (0) 统计任务 */
#define OS_TASK_STAT_STK_CHK_EN   1u   /*     从统计任务检查任务栈 */
#define OS_TASK_SUSPEND_EN        1u   /*     包含 OSTaskSuspend() 和 OSTaskResume() 的代码 */
#define OS_TASK_SW_HOOK_EN        1u   /*     包含 OSTaskSwHook() 的代码 */


                                       /* ----------------------- 事件标志 ------------------------ */
#define OS_FLAG_EN                0u   /* 启用 (1) 或禁用 (0) 事件标志的代码生成 ;1*/
#define OS_FLAG_ACCEPT_EN         1u   /*     包含 OSFlagAccept() 的代码 */
#define OS_FLAG_DEL_EN            1u   /*     包含 OSFlagDel() 的代码 */
#define OS_FLAG_NAME_EN           1u   /*     为事件标志组启用名称 */
#define OS_FLAG_QUERY_EN          1u   /*     包含 OSFlagQuery() 的代码 */
#define OS_FLAG_WAIT_CLR_EN       1u   /* 包含等待清除事件标志的代码 */
#define OS_FLAGS_NBITS           16u   /* OS_FLAGS 数据类型的位数 (8、16 或 32) */


                                       /* -------------------- 消息邮箱 --------------------- */
#define OS_MBOX_EN                0u   /* 启用 (1) 或禁用 (0) 邮箱的代码生成 ;1*/
#define OS_MBOX_ACCEPT_EN         1u   /*     包含 OSMboxAccept() 的代码 */
#define OS_MBOX_DEL_EN            1u   /*     包含 OSMboxDel() 的代码 */
#define OS_MBOX_PEND_ABORT_EN     1u   /*     包含 OSMboxPendAbort() 的代码 */
#define OS_MBOX_POST_EN           1u   /*     包含 OSMboxPost() 的代码 */
#define OS_MBOX_POST_OPT_EN       1u   /*     包含 OSMboxPostOpt() 的代码 */
#define OS_MBOX_QUERY_EN          1u   /*     包含 OSMboxQuery() 的代码 */


                                       /* --------------------- 内存管理 -------------------- */
#define OS_MEM_EN                 0u   /* 启用 (1) 或禁用 (0) 内存管理器的代码生成 ;1*/
#define OS_MEM_NAME_EN            1u   /*     启用内存分区名称 */
#define OS_MEM_QUERY_EN           1u   /*     包含 OSMemQuery() 的代码 */


                                       /* ---------------- 互斥信号量 --------------- */
#define OS_MUTEX_EN               1u   /* 启用 (1) 或禁用 (0) 互斥量的代码生成 */
#define OS_MUTEX_ACCEPT_EN        1u   /*     包含 OSMutexAccept() 的代码 */
#define OS_MUTEX_DEL_EN           1u   /*     包含 OSMutexDel() 的代码 */
#define OS_MUTEX_QUERY_EN         1u   /*     包含 OSMutexQuery() 的代码 */


                                       /* ---------------------- 消息队列 ---------------------- */
#define OS_Q_EN                   1u   /* 启用 (1) 或禁用 (0) 队列的代码生成 */
#define OS_Q_ACCEPT_EN            1u   /*     包含 OSQAccept() 的代码 */
#define OS_Q_DEL_EN               1u   /*     包含 OSQDel() 的代码 */
#define OS_Q_FLUSH_EN             1u   /*     包含 OSQFlush() 的代码 */
#define OS_Q_PEND_ABORT_EN        1u   /*     包含 OSQPendAbort() 的代码 */
#define OS_Q_POST_EN              1u   /*     包含 OSQPost() 的代码 */
#define OS_Q_POST_FRONT_EN        1u   /*     包含 OSQPostFront() 的代码 */
#define OS_Q_POST_OPT_EN          1u   /*     包含 OSQPostOpt() 的代码 */
#define OS_Q_QUERY_EN             1u   /*     包含 OSQQuery() 的代码 */


                                       /* ------------------------ 信号量 ------------------------ */
#define OS_SEM_EN                 1u   /* 启用 (1) 或禁用 (0) 信号量的代码生成 */
#define OS_SEM_ACCEPT_EN          1u   /*    包含 OSSemAccept() 的代码 */
#define OS_SEM_DEL_EN             1u   /*    包含 OSSemDel() 的代码 */
#define OS_SEM_PEND_ABORT_EN      1u   /*    包含 OSSemPendAbort() 的代码 */
#define OS_SEM_QUERY_EN           1u   /*    包含 OSSemQuery() 的代码 */
#define OS_SEM_SET_EN             1u   /*    包含 OSSemSet() 的代码 */


                                       /* --------------------- 时间管理 ---------------------- */
#define OS_TIME_DLY_HMSM_EN       1u   /*     包含 OSTimeDlyHMSM() 的代码 */
#define OS_TIME_DLY_RESUME_EN     1u   /*     包含 OSTimeDlyResume() 的代码 */
#define OS_TIME_GET_SET_EN        1u   /*     包含 OSTimeGet() 和 OSTimeSet() 的代码 */
#define OS_TIME_TICK_HOOK_EN      1u   /*     包含 OSTimeTickHook() 的代码 */


                                       /* --------------------- 定时器管理 --------------------- */
#define OS_TMR_EN                 0u   /* 启用 (1) 或禁用 (0) 定时器的代码生成 */
#define OS_TMR_CFG_MAX           16u   /*     定时器的最大数量 */
#define OS_TMR_CFG_NAME_EN        1u   /*     确定定时器名称 */
#define OS_TMR_CFG_WHEEL_SIZE     7u   /*     定时器轮的大小 (# 个辐条) */
#define OS_TMR_CFG_TICKS_PER_SEC 10u   /*     定时器管理任务的运行频率 (Hz) */


                                       /* ---------------------- 跟踪记录器 ---------------------- */
#define OS_TRACE_EN               0u   /* 启用 (1) 或禁用 (0) uC/OS-II 跟踪仪表 */
#define OS_TRACE_API_ENTER_EN     0u   /* 启用 (1) 或禁用 (0) uC/OS-II 跟踪 API 进入仪表 */
#define OS_TRACE_API_EXIT_EN      0u   /* 启用 (1) 或禁用 (0) uC/OS-II 跟踪 API 退出仪表 */

#endif
