/** **************************************************************
 * @file      TTDelay_config.h
 * @authors   Lars Heinrichs
 * @copyright Lars Heinrichs
 *
 * @brief
 * Configuration file for TTDelay
 ** **************************************************************/

#ifndef _TTDELAY_CFG_H
#define _TTDELAY_CFG_H

#include "test_timers.h"

/* *****************************************************
 *  TIMER SETUP
 * ****************************************************/
// provide a function to read your current time/tick value (used for scheduling)
#define TT_TIMER_FUNC          GetSysTick()
// data type of counter register (so we can detect overflow correctly)
#define TT_TIMER_TYPE          uint32_t

// tiny scheduler reserves memory for TASK_COUNT_MAX tasks 
#define TT_TASK_COUNT_MAX           7

// to make sure low priority tasks dont starve, increase their priorioty whenever \
    they have been scheduled but not run
#define TT_ENABLE_TASK_AGING        1

// maximum allowed priority increase through aging (default: 0xFF)
#define TT_PRIORITY_MAX_CHANGE      0xFF

// lowest allowed priority that can be reached by aging (>= 0)
#define TT_PRIORITY_THRESHOLD       15


// TTDelay can monitor the CPU load caused by different tasks. uncomment this to enable
#define TT_MONITOR_CPU_LOAD
// provide a function to read and to reset the tick count.
// this function is called at the start and end of each task function call.
// make sure the function resets the timer (counter register) and returns 
// the counter register value (from before the reset)
#define TT_READ_RST_TICK_FUNC           ReadResetCpuLoadTick()
#define TT_CPU_LOAD_UPDATE_INTERVAL     1000





#endif // _SIMPLE_SCHEDULER_H
