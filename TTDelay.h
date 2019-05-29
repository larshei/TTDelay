/**
 * @file      TTDelay.h
 * @authors   Lars Heinrichs
 * @copyright DSP Systeme GmbH, Düsseldorf
 *
 * @brief [description]
 */

#ifndef _TTDELAY_H
#define _TTDELAY_H

/*******************************************************************************
* Includes
*******************************************************************************/
#include "stdint.h"
#include "TTDelay_config.h"

/*******************************************************************************
* Defines
*******************************************************************************/
#define TT_TASK_EVER_RUN       0x01
#define TT_TASK_IS_PERIODIC    0x02
#define TT_TIMER_OVERFLOW      0x04
#define TT_TASK_ACTIVE         0x08

#ifdef TT_MONITOR_CPU_LOAD
    #define GET_RST_TICK(x)    x = TT_READ_RST_TICK_FUNC
#else
    #define GET_RST_TICK(x)    ;
#endif

/*******************************************************************************
* Types and Typedefs
*******************************************************************************/
typedef struct TTDelay_task_t {
    TT_TIMER_TYPE   timeRunning;
    float           rCpuUsage;
    TT_TIMER_TYPE   uiTimeNextExecute;
    TT_TIMER_TYPE   uiTimeLastExecute;
    TT_TIMER_TYPE   uiPeriod; 
    TT_TIMER_TYPE   uiLongestExecuteDuration; 
    uint8_t         uiNextExecuteOverflow;
    uint8_t         uiInitialPriority;
    uint8_t         uiCurrentPriority;
    uint8_t         uiFlags;
    uint8_t         fDue;
    void            (*func )(void*, void*);
    void *          pvFuncParameterIn;
    void *          pvFuncParameterOut;
} TTDelay_task_t;

typedef struct TTDelay_cpu_usage_TypDef {
    float rTaskUsage[TT_TASK_COUNT_MAX];
    float rIdleUsage;
    float rTtsysUsage;
} TTDelay_cpu_usage_TypDef;

enum {
    TT_OK,
    TT_NOK,
    TT_ERROR_TOO_MANY_TASKS,
    TT_RETURN_COUNT,
    TT_MORE_TASKS_SCHEDULED
};

/*******************************************************************************
* Global Variables
*******************************************************************************/

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
// public functions to be used
int  TTDelay_create_task(void (*func ), void* input_param, void* output_param, uint8_t priority);
int  TTDelay_create_task_periodic(void (*func ), void* input_param, void* output_param, uint8_t priority, TT_TIMER_TYPE uiPeriod);
int  TTDelay_run(void);
void TTDelay_from_last(int delay);
void TTDelay_from_now (int delay);
int  TTDelay_set_next_function(void (*func ));
void TTDelay_cpu_usage_monitor(void* in, void* out); // runs as a seperate task, both arguments can be NULL

//functions for unit testing
void    TTDelay_adjust_priority(void);
void    TTDelay_reset(void);
int     TTDelay_get_task_count(void);
uint8_t TTDelay_get_next_scheduled(void);
TTDelay_task_t* TTDelay_get_task(int index);
void    TTDelay_find_due_tasks(void);
void    TTDelay_run_task(int index);
int     TTDelay_is_due(int index);
void*   TTDelay_get_task_output_param_pointer(int index);
void*   TTDelay_get_task_input_param_pointer (int index);
uint32_t TTDelay_get_next_schedule_time(int index);
float   TTDelay_get_idle_time_percentage(void);
float   TTDelay_get_ttsys_time_percentage(void);
void    TTDelay_set_idle_tick_count(uint32_t uiTickCount);
void    TTDelay_set_ttsys_tick_count(uint32_t uiTickCount);
TTDelay_cpu_usage_TypDef* TTDelay_get_cpu_usage_pointer(void);

#endif // _SIMPLE_SCHEDULER_H
