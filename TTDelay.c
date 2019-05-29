


/* ******************************************************************************
 * @file      TTDelay.c
 * @authors   Lars Heinrichs
 * @copyright Lars Heinrichs
 *
 * @brief 
 * routines to check if a function is meant to run or to wait. it is more of
 * a non-blocking delay help than actual scheduling.
 * 
 * @desription
 * TTDelay (tiny time delay) is a way to add very simply non-blocking delays between
 * function calls. Once a function starts being executed, it will be executed
 * until completion. During the functions executions, the application may set
 * when it is to be executed again. Whenever the TTDelay main code is run it 
 * will check wether execution of a function is scheduled or not and skip/execute it.
 * *****************************************************************************/

/*******************************************************************************
* Includes
*******************************************************************************/
#include "TTDelay.h"

/*******************************************************************************
* Defines
********************************************************************************


/*******************************************************************************
* Local Types and Typedefs
*******************************************************************************/
typedef struct {
    uint8_t         current_task_index;
    uint8_t         task_count;
    uint8_t         highest_priority_value;
    uint8_t         highest_priority_index;
    uint8_t         task_scheduled_count;
    TT_TIMER_TYPE   current_time;
    TT_TIMER_TYPE   last_run_time;
    TT_TIMER_TYPE   uiCpuTtsysCycleTickCount;
    TT_TIMER_TYPE   uiCpuIdleCycleTickCount;
    float           rIdleTimePercentage;
    float           rTtsysTimePercentage;
    TTDelay_task_t  task [ TT_TASK_COUNT_MAX ];
}TTDelay_t; 

/*******************************************************************************
* Global Variables
*******************************************************************************/


/*******************************************************************************
* Static Function Prototypes
*******************************************************************************/
void TTDelay_find_due_tasks(void);
void TTDelay_adjust_priority(void);
void TTDelay_run_task(int index);
void TTDelay_time_measure(TT_TIMER_TYPE* puiAddTimeToValue);
void TTDelay_reset_time_running(void);
void TTDelay_calculate_cpu_usage(void);
extern TT_TIMER_TYPE GET_RST_TICK;

/*******************************************************************************
* Static Variables
*******************************************************************************/
// this holds our complete system. initialize everything to 0.
TTDelay_t                   ttSystem    = {0};
TTDelay_cpu_usage_TypDef    ttCpuLoad   = {0};

/*******************************************************************************
* F U N C T I O N S   U S E D   I N   U S E R   P R O G R A M
*******************************************************************************/
/* removes all the tasks */
void TTDelay_reset(void) {
    uint8_t *data = (uint8_t*)&ttSystem;
    for (int i = 0; i < sizeof(TTDelay_t) ; i++,data++){
        *data = 0;
    }
}

/* Create a task for the TTDelay System. */
int TTDelay_create_task(void (*func ), void* input_param, void* output_param, uint8_t priority){
    if (ttSystem.task_count >= TT_TASK_COUNT_MAX)
        return TT_ERROR_TOO_MANY_TASKS;

    TTDelay_task_t *task        = &ttSystem.task[ttSystem.task_count];
    task->func                  = func;
    task->uiFlags               = 0;
    task->uiCurrentPriority     = priority;
    task->uiInitialPriority     = priority;
    task->pvFuncParameterOut    = output_param;
    task->pvFuncParameterIn     = input_param;
    task->uiTimeNextExecute     = TT_TIMER_FUNC;
    
    ttSystem.task_count++;
    return TT_OK;
}

/* same as TTDelay_create_task, but takes an additional uiPeriod argument and sets an additional flag */
int TTDelay_create_task_periodic(void (*func ), void* input_param, void* output_param, uint8_t priority, TT_TIMER_TYPE uiPeriod){
    int error;
    error = TTDelay_create_task(func, input_param, output_param, priority);
    if (error)
        return error;
    ttSystem.task[ttSystem.task_count-1].uiPeriod = uiPeriod;
    ttSystem.task[ttSystem.task_count-1].uiFlags |= TT_TASK_IS_PERIODIC;
    return TT_OK;
}

/* this is the core function of the system that will be called in the users
 * main program loop and execute the tasks.
 * - measure time passed since last call (idle time for TTDelay System) 
 * - check if tasks are due (using TT_TIMER_FUNC from TTDelay_config.h)
 * - if tasks are due: add aging to scheduled tasks that will not be run this time
 * - if tasks are due: run highest priority scheduled (includes time measurement) 
 * - returns TT_OK if all is done and TT_MORE_TASKS_SCHEDULED if more tasks are due */
int TTDelay_run(void) {
    TTDelay_time_measure(&ttSystem.uiCpuIdleCycleTickCount);
    TTDelay_find_due_tasks();
    if(ttSystem.task_scheduled_count){
        #if TT_ENABLE_TASK_AGING
        TTDelay_adjust_priority();
        #endif
        TTDelay_run_task(ttSystem.highest_priority_index);
        if (ttSystem.task_scheduled_count > 1)
            return TT_MORE_TASKS_SCHEDULED;
    }
    return TT_OK;
}

/* schedule the call task 'delay' timer ticks from when it was meant to be 
 * executed. Depending on CPU load this may create some jitter, but on average
 * a function that always uses TTDelay_from_last(100) (here: 100 ms) will be 
 * called 10 times a second  */
void TTDelay_from_last(int delay){
    TTDelay_task_t* task;
    task = &ttSystem.task[ttSystem.current_task_index];

    if (task->uiTimeNextExecute + delay < task->uiTimeNextExecute){
        task->uiNextExecuteOverflow = 1;
    }
    task->uiTimeNextExecute += delay;
    if (!(task->uiFlags & TT_TASK_EVER_RUN)){
        task->uiFlags |= TT_TASK_EVER_RUN;
        if (task->uiFlags & TT_TASK_IS_PERIODIC )
            if (task->uiTimeNextExecute < ttSystem.current_time)
                task->uiTimeNextExecute = ttSystem.current_time + task->uiPeriod;
    }
}

/* call this function again in 'delay' timer ticks. This may be used if the next
 * function/task call needs a certain time span AFTER the current task/function call */
void TTDelay_from_now (int delay){
    TTDelay_task_t* task;
    task = &ttSystem.task[ttSystem.current_task_index];
    task->uiTimeNextExecute = ttSystem.current_time + delay;
    if (task->uiTimeNextExecute < task->uiTimeLastExecute)
        task->uiNextExecuteOverflow = 1;
}

// estimates the CPU usage per task
void TTDelay_cpu_usage_monitor(void* in, void* out){
    TTDelay_calculate_cpu_usage();
    TTDelay_reset_time_running();
    TTDelay_from_last(TT_CPU_LOAD_UPDATE_INTERVAL);
}


/* the user may change the function that will be executed on the next task run */
int TTDelay_set_next_function(void (*func )){
    if (func == (void*)0)
        return TT_NOK;
    ttSystem.task[ttSystem.current_task_index].func = func;
    return TT_OK;    
}


/*******************************************************************************
* I N T E R N A L   F U N C T I O N S
*******************************************************************************/
/* Calls a user function that reads the current timer value AND RESETS the timer */
void TTDelay_time_measure(TT_TIMER_TYPE* puiAddTimeToValue){
    int duration = 0;
    GET_RST_TICK(duration);
    *puiAddTimeToValue += duration;
    return;
}

/* compare the current time and a tasks next execute time to find out what tasks
 * should be run */
void TTDelay_find_due_tasks(void) {
    ttSystem.current_time = TT_TIMER_FUNC;
    ttSystem.highest_priority_value = 255;
    ttSystem.highest_priority_index = TT_TASK_COUNT_MAX + 1;
    ttSystem.task_scheduled_count = 0;
    TTDelay_task_t *task = &ttSystem.task[0];
    uint8_t resetNextExecuteOverflow = 0;
    
    if (ttSystem.current_time < ttSystem.last_run_time){
        resetNextExecuteOverflow = 1;
    }

    for (int i = 0 ; i < ttSystem.task_count ; i++, task++){
        // assume task is not scheduled
        task->fDue = 0;
        // unset overflow flag if time had an overflow as well
        if (resetNextExecuteOverflow){
            task->uiNextExecuteOverflow = 0;
        }
        // add task to "due" list if necessary
        if ((ttSystem.current_time >= task->uiTimeNextExecute)
        & (! task->uiNextExecuteOverflow)) {
            task->fDue = 1;
            ttSystem.task_scheduled_count++;
            // find out if priority of this task is highest (low number -> higher priority)
            if (task->uiCurrentPriority < ttSystem.highest_priority_value){
                ttSystem.highest_priority_value = task->uiCurrentPriority;
                ttSystem.highest_priority_index = i;
            }
        }
    }
}

/* Aging for tasks that are scheduled but not run right now */
void TTDelay_adjust_priority(void) {
    // just one task scheduled? then we have no tasks to adjust
    if (ttSystem.task_scheduled_count == 1)
        return;

    TTDelay_task_t  *task = &ttSystem.task[0];

    for (int i = 0 ; i < ttSystem.task_count ; i++, task++){
        if (task->fDue){
            if (i != ttSystem.highest_priority_index){
                // add aging to the task to make sure low priority tasks are executed at some point\
                maximum aging is set by TT_PRIORITY_MAX_CHANGE and TT_PRIORITY_THRESHOLD sets a\
                hard limit to how low a tasks priority can get through aging.
                if ((task->uiCurrentPriority > TT_PRIORITY_THRESHOLD) \
                    && (task->uiCurrentPriority \
                        > (task->uiInitialPriority - TT_PRIORITY_MAX_CHANGE)))
                    task->uiCurrentPriority--;
            }
        }
    }
}


/* execute the task function and track the time needed until completion */
void TTDelay_run_task(int index){
    // time management
    int iExecuteTime            = 0;
    TTDelay_task_t* task        = &ttSystem.task[index];    
    task->uiTimeLastExecute     = ttSystem.current_time;
    ttSystem.current_task_index = index;
    TTDelay_time_measure(&ttSystem.uiCpuTtsysCycleTickCount);

    // run task
    task->func(task->pvFuncParameterIn, task->pvFuncParameterOut);
    // reset task to default
    task->uiCurrentPriority     = task->uiInitialPriority;
    if (task->uiFlags & TT_TASK_IS_PERIODIC){
        TTDelay_from_last(task->uiPeriod);
    }

    // time management
    ttSystem.last_run_time      = ttSystem.current_time;
    TTDelay_time_measure(&iExecuteTime);
    task->timeRunning           += iExecuteTime;
    if (iExecuteTime > task->uiLongestExecuteDuration)
        task->uiLongestExecuteDuration = iExecuteTime;
}


int TTDelay_get_remaining_idle_time(void) {

}

TT_TIMER_TYPE TTDelay_get_next_schedule_time(int index){
    return ttSystem.task[index].uiTimeNextExecute;
}


int TTDelay_get_task_count(void) {
    return ttSystem.task_count;
}

void TTDelay_set_idle_tick_count(TT_TIMER_TYPE uiTickCount){
    ttSystem.uiCpuIdleCycleTickCount = uiTickCount;
}

void TTDelay_set_ttsys_tick_count(TT_TIMER_TYPE uiTickCount){
    ttSystem.uiCpuTtsysCycleTickCount = uiTickCount;
}

float TTDelay_get_idle_time_percentage(void) {
    return ttCpuLoad.rIdleUsage; 
}

float TTDelay_get_ttsys_time_percentage(void) {
    return ttCpuLoad.rTtsysUsage; 
}

TTDelay_task_t* TTDelay_get_task(int index){
    if ((index >= 0) && (index < ttSystem.task_count))
        return &ttSystem.task[index];
    return (TTDelay_task_t*)0;
}

int TTDelay_is_due(int index){
    // if task exists, return due status
    if ((index >= 0) && (index < ttSystem.task_count))
        return ttSystem.task[index].fDue;
    return 0;
}

uint8_t TTDelay_get_next_scheduled(void) {
    return ttSystem.highest_priority_index;
}

void* TTDelay_get_task_input_param_pointer(int index){
    if ((index >= 0) && (index < ttSystem.task_count)){
        return ttSystem.task[index].pvFuncParameterIn;
    }
    return (void*)0;
}

void* TTDelay_get_task_output_param_pointer(int index){
    if ((index >= 0) && (index < ttSystem.task_count))
        return ttSystem.task[index].pvFuncParameterOut;
    return (void*)0;
}

int TTDelay_get_task_scheduled_count(void) {
    return ttSystem.task_scheduled_count;
}

void TTDelay_calculate_cpu_usage(void) {
    TTDelay_task_t* task = (TTDelay_task_t*)ttSystem.task;
    TT_TIMER_TYPE uiTotalTime = ttSystem.uiCpuIdleCycleTickCount + ttSystem.uiCpuTtsysCycleTickCount;
    for (int i = 0 ; i < ttSystem.task_count ; i++, task++){
        uiTotalTime += task->timeRunning;
    }
    // overflow detection
    if (    (uiTotalTime < ttSystem.uiCpuIdleCycleTickCount) \
        ||  (uiTotalTime < ttSystem.uiCpuTtsysCycleTickCount))
        return;

    task = (TTDelay_task_t*)ttSystem.task;
    for (int i = 0 ; i < ttSystem.task_count ; i++, task++){
        ttCpuLoad.rTaskUsage[i] = (float)task->timeRunning / uiTotalTime;
    }
    ttCpuLoad.rIdleUsage  = (float)ttSystem.uiCpuIdleCycleTickCount  / uiTotalTime;
    ttCpuLoad.rTtsysUsage = (float)ttSystem.uiCpuTtsysCycleTickCount / uiTotalTime;
}

void TTDelay_reset_time_running(void) {
    TTDelay_task_t* task = (TTDelay_task_t*)ttSystem.task;
    for (int i = 0 ; i < ttSystem.task_count ; i++, task++){
        task->timeRunning = 0;
    }
    ttSystem.uiCpuIdleCycleTickCount  = 0;
    ttSystem.uiCpuTtsysCycleTickCount = 0;
}

TTDelay_cpu_usage_TypDef* TTDelay_get_cpu_usage_pointer(void) {
    return &ttCpuLoad;
}
