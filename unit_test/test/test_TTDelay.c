#include "unity.h"
#include "TTDelay.h"
#include "mock_timers.h"


int led_value;
int output_value;
int delay_test_var;
#define DELAY_TIME  50

#define TTDELAY_RUN_TIMER_EXPECTATIONS      ReadResetCpuLoadTick_ExpectAndReturn(0); \
                                            ReadResetCpuLoadTick_ExpectAndReturn(0); \
                                            ReadResetCpuLoadTick_ExpectAndReturn(0)

#define TTDELAY_RUN_TASK_TIMER_EXPECTATIONS ReadResetCpuLoadTick_ExpectAndReturn(0); \
                                            ReadResetCpuLoadTick_ExpectAndReturn(0)

void setUp(void)
{
    TTDelay_reset();
    led_value       = 0;
    output_value    = 0;
    delay_test_var  = 0;
}

void tearDown(void)
{

}

void led_toggle(void* in, void* out){
    *((int*)out) = 1;
    TTDelay_from_last(DELAY_TIME);
}

void test_create_task_check_func(){
    TTDelay_task_t *task;
    GetSysTick_ExpectAndReturn(0);
    TTDelay_create_task(led_toggle, (void*)0, &led_value, 5);
    task = TTDelay_get_task(0);
    TEST_ASSERT_EQUAL(led_toggle, task->func);
}

void test_create_task_check_input_output_pointer(){
    GetSysTick_ExpectAndReturn(0);
    TEST_ASSERT_EQUAL(TT_OK, TTDelay_create_task(led_toggle, &led_value, &output_value, 5));    
    TEST_ASSERT_EQUAL(&led_value,    TTDelay_get_task_input_param_pointer(0));
    TEST_ASSERT_EQUAL(&output_value, TTDelay_get_task_output_param_pointer(0));
}

void test_create_task_and_run(){
    GetSysTick_ExpectAndReturn(0);
    TTDelay_create_task(led_toggle, (void*)0, &led_value, 5);
    TTDELAY_RUN_TASK_TIMER_EXPECTATIONS;
    TTDelay_run_task(0);
    TEST_ASSERT_EQUAL(1, led_value);
}

void test_create_two_tasks_check_func(){
    TEST_ASSERT_EQUAL(0, TTDelay_get_task_count());
    TTDelay_task_t *task;
    GetSysTick_ExpectAndReturn(0);
    GetSysTick_ExpectAndReturn(0);
    TEST_ASSERT_EQUAL(TT_OK, TTDelay_create_task(led_toggle, &led_value, &led_value, 5));    
    TEST_ASSERT_EQUAL(TT_OK, TTDelay_create_task(led_toggle, &led_value, &led_value, 5));    
    task = TTDelay_get_task(1);
    TEST_ASSERT_EQUAL(led_toggle, task->func);
}

void test_create_task_check_flags(){
    TEST_ASSERT_EQUAL(0, TTDelay_get_task_count());
    GetSysTick_ExpectAndReturn(0);
    TEST_ASSERT_EQUAL(TT_OK, TTDelay_create_task(led_toggle, NULL, (void*)led_value, 5)); 
    TTDelay_task_t *task;  
    task = TTDelay_get_task(0);
    TEST_ASSERT_EQUAL(0, task->uiFlags);
}

void test_create_task_periodic_check_flags(){
    TEST_ASSERT_EQUAL(0, TTDelay_get_task_count());
    TTDelay_task_t *task;
    GetSysTick_ExpectAndReturn(0);
    TEST_ASSERT_EQUAL(TT_OK, TTDelay_create_task_periodic(led_toggle, NULL, (void*)led_value, 5, 100));
    task = TTDelay_get_task(0);
    TEST_ASSERT_EQUAL(TT_TASK_IS_PERIODIC, task->uiFlags);
}

void test_create_task_periodic_period_set(){
    TEST_ASSERT_EQUAL(0, TTDelay_get_task_count());
    GetSysTick_ExpectAndReturn(0);
    TEST_ASSERT_EQUAL(TT_OK, TTDelay_create_task_periodic(led_toggle, NULL, (void*)led_value, 5, 100));
    TTDelay_task_t *task;
    task = TTDelay_get_task(0);
    TEST_ASSERT_EQUAL(100, task->uiPeriod);
}

void test_create_task_check_priority(){
    TEST_ASSERT_EQUAL(0, TTDelay_get_task_count());
    GetSysTick_ExpectAndReturn(0);
    TEST_ASSERT_EQUAL(TT_OK, TTDelay_create_task(led_toggle, NULL, (void*)led_value, 5));    
    TTDelay_task_t *task;
    task = TTDelay_get_task(0);
    TEST_ASSERT_EQUAL(5, task->uiCurrentPriority);
    TEST_ASSERT_EQUAL(5, task->uiInitialPriority);
}

void test_create_too_many_tasks(){
    for (int i=0; i < TT_TASK_COUNT_MAX ; i++){
        GetSysTick_ExpectAndReturn(0);
        TEST_ASSERT_EQUAL(TT_OK, TTDelay_create_task(led_toggle, NULL, &led_value, i));
    }
    TEST_ASSERT_EQUAL(TT_ERROR_TOO_MANY_TASKS, TTDelay_create_task(led_toggle, NULL, &led_value, 5));
}

/* *****************************************************************************
 *  THIS SECTION TESTS ADDING DELAY TIMES AND REPEATED EXECUTION IN CASE
 *  AN EXECUTION HAS BEEN MISSED DUE TO LONG BURST TIMES IN SOME FUNCTIONS
 * *****************************************************************************/
void delay_add_from_now(void* in, void* out){
    *((int*)out) = 1;
    TTDelay_from_now(DELAY_TIME);
}

void delay_add_from_last(void* in, void* out){
    *((int*)out) = 2;
    TTDelay_from_last(DELAY_TIME);
}

void delay_periodic_task(void* in, void* out){
    *((int*)out) = 3;
}

void delay_periodic_increase(void* in, void* out){
    *((int*)out) += 1;
}

void delay_increase(void* in, void* out){
    *((int*)out) += 1;
    TTDelay_from_now(DELAY_TIME);
}

// add delay to task and check the remaining time.
void test_add_delay_to_task(){
    ReadResetCpuLoadTick_IgnoreAndReturn(0);
    GetSysTick_ExpectAndReturn(0);
    GetSysTick_ExpectAndReturn(0);
    GetSysTick_ExpectAndReturn(0);
    TTDelay_create_task(         delay_add_from_now,  (void*)0, &delay_test_var, 10);
    TTDelay_create_task(         delay_add_from_last, (void*)0, &delay_test_var, 30 );
    TTDelay_create_task_periodic( delay_periodic_task, (void*)0, &delay_test_var, 50, DELAY_TIME);

    // runs highest priority (from_now)
    GetSysTick_ExpectAndReturn(DELAY_TIME - 1);
    TTDelay_run();
    TEST_ASSERT_EQUAL(1, delay_test_var);
    // runs 2nd highest priority (from_last)
    GetSysTick_ExpectAndReturn(DELAY_TIME - 1);
    TTDelay_run();
    TEST_ASSERT_EQUAL(2, delay_test_var);
    // runs 3rd highest priority (priodic)
    GetSysTick_ExpectAndReturn(DELAY_TIME - 1);
    TTDelay_run();
    TEST_ASSERT_EQUAL(3, delay_test_var);

    // periodic and _from_last should run again (ordered by priority),\
       _from_now should not be scheduled yet!

    GetSysTick_ExpectAndReturn(DELAY_TIME + 1);
    TTDelay_run();
    TEST_ASSERT_EQUAL(2, delay_test_var);

    GetSysTick_ExpectAndReturn(DELAY_TIME + 1);
    TTDelay_run();
    TEST_ASSERT_EQUAL(3, delay_test_var);
    // output value shoudlnt change cause nothing scheduled yet!
    GetSysTick_ExpectAndReturn(DELAY_TIME + 1);
    TTDelay_run();
    TEST_ASSERT_EQUAL(3, delay_test_var);
    
    // _from_now was last called at DELAY_TIME-1.
    // should be called again at (DELAY_TIME-1)+DELAY_TIME
    GetSysTick_ExpectAndReturn(2*DELAY_TIME - 1);
    TTDelay_run();
    TEST_ASSERT_EQUAL(1, delay_test_var);
    // nothing else should be scheduled at this point.
    delay_test_var = 0;
    GetSysTick_ExpectAndReturn(2*DELAY_TIME - 1);
    TTDelay_run();
    TEST_ASSERT_EQUAL(0, delay_test_var);
}

void test_run_periodic_first_time_high_timer_count_should_run_once(){
    ReadResetCpuLoadTick_IgnoreAndReturn(0);
    GetSysTick_ExpectAndReturn(0);
    TTDelay_create_task_periodic( delay_periodic_increase, &delay_test_var, &delay_test_var, 50, DELAY_TIME );

    for (int i = 0 ; i < 10 ; i++){
        GetSysTick_ExpectAndReturn(10*DELAY_TIME - 1 + i);
        TTDelay_run();
        TEST_ASSERT_EQUAL(1, delay_test_var);
    }    
    // an additional run should not have anything scheduled
    GetSysTick_ExpectAndReturn(10*DELAY_TIME + 1);
    TTDelay_run();
    TEST_ASSERT_EQUAL(1, delay_test_var);
    // additional run scheduled here!
    GetSysTick_ExpectAndReturn(11*DELAY_TIME + 1);
    TTDelay_run();
    TEST_ASSERT_EQUAL(2, delay_test_var);
}

void test_run_periodic_task_first_time_with_high_timer_count_should_not_repeat(){
    TEST_IGNORE();
}

void test_schedule_with_timer_overflow_inermediate_steps(){
    ReadResetCpuLoadTick_IgnoreAndReturn(0);
    GetSysTick_ExpectAndReturn(0);
    TTDelay_create_task( delay_increase, &delay_test_var, &delay_test_var, 50);
    // run once to assure this worked
    GetSysTick_ExpectAndReturn(0);
    TTDelay_run();
    TEST_ASSERT_EQUAL(1, delay_test_var);
    // run with timer close to overflow
    GetSysTick_ExpectAndReturn(0xFFFFFFF0);
    TTDelay_run(); // delay in function will cause overflow!
    TEST_ASSERT_EQUAL(2, delay_test_var);
    // thuis call should be protected by overflow bit
    GetSysTick_ExpectAndReturn(0xFFFFFFFB);
    TTDelay_run(); 
    TEST_ASSERT_EQUAL(2, delay_test_var);
    // this should reset the overflow, but not run the task
    GetSysTick_ExpectAndReturn(10);
    TTDelay_run(); 
    TEST_ASSERT_EQUAL(2, delay_test_var);
    // next run should certainly be here!
    GetSysTick_ExpectAndReturn(DELAY_TIME);
    TTDelay_run(); 
    TEST_ASSERT_EQUAL(3, delay_test_var);
}

void test_schedule_with_timer_overflow_direct(){
    ReadResetCpuLoadTick_IgnoreAndReturn(0);
    GetSysTick_ExpectAndReturn(0);
    TTDelay_create_task( delay_increase, &delay_test_var, &delay_test_var, 50);
    // run once to assure this worked
    GetSysTick_ExpectAndReturn(0);
    TTDelay_run();
    TEST_ASSERT_EQUAL(1, delay_test_var);
    // run with timer close to overflow
    GetSysTick_ExpectAndReturn(0xFFFFFFF0);
    TTDelay_run(); // delay in function will cause overflow!
    TEST_ASSERT_EQUAL(2, delay_test_var);
    // next run should certainly be here!
    GetSysTick_ExpectAndReturn(DELAY_TIME);
    TTDelay_run(); 
    TEST_ASSERT_EQUAL(3, delay_test_var);
}

/* *****************************************************************************
 *  THIS SECTION TESTS CHECK FUNCTION POINTER CHANGES
 * *****************************************************************************/
// < < < < < < < <  H E L P E R   F U N C T I O N S  > > > > > > > > >
void set_to_10_and_switch(void *in, void* out);

void set_to_5_and_switch(void *in, void* out){
    *((int*)out) = 5;
    TTDelay_set_next_function(set_to_10_and_switch);
    TTDelay_from_now(DELAY_TIME);
}
void set_to_10_and_switch(void *in, void* out){
    *((int*)out) = 10;
    TTDelay_set_next_function(set_to_5_and_switch);
    TTDelay_from_now(DELAY_TIME); 
}
// < < < < < < < < <  T E S T   F U N C T I O N S  > > > > > > > > >
void test_function_pointer_change(){
    ReadResetCpuLoadTick_IgnoreAndReturn(0);
    GetSysTick_ExpectAndReturn(0); 
    TTDelay_create_task(set_to_5_and_switch, &output_value, &output_value, 50);
    TTDelay_task_t* task = TTDelay_get_task(0);

    // execute function ..5.., switch to ..10..
    GetSysTick_ExpectAndReturn(0);
    TTDelay_run();
    TEST_ASSERT_EQUAL(5, output_value);
    TEST_ASSERT_EQUAL(set_to_10_and_switch, task->func);

    // execute function ..10.., switch to ..5..
    GetSysTick_ExpectAndReturn(DELAY_TIME);
    TTDelay_run();
    TEST_ASSERT_EQUAL(10, output_value);
    TEST_ASSERT_EQUAL(set_to_5_and_switch, task->func);
}

/* *****************************************************************************
 *  THIS SECTION TESTS CREATION OF MULTIPLE TASKS THAT ARE SCHEDULED AT 
 *  THE SAME TIME AND SHOULD BE RUN IN ORDER OF THEIR PRIORITY
 * *****************************************************************************/
// < < < < < < < <  H E L P E R   F U N C T I O N S  > > > > > > > > >
void priority_10(void* in, void* out){
    *((int*)out) = 10;
    TTDelay_from_last(DELAY_TIME);
}
void priority_5(void* in, void* out){
     *((int*)out)  = 5;
    TTDelay_from_last(DELAY_TIME);
}
void priority_2(void* in, void* out){
    *((int*)out)  = 2;
    TTDelay_from_last(DELAY_TIME);
}
// helper function to check if priority is within the specified bounds
void check_priority_in_bounds(TTDelay_task_t* task){
    if (task->uiInitialPriority <= TT_PRIORITY_THRESHOLD){
        TEST_ASSERT_EQUAL(task->uiCurrentPriority, task->uiInitialPriority);
    } else if ((task->uiInitialPriority-TT_PRIORITY_MAX_CHANGE) > TT_PRIORITY_THRESHOLD){
        TEST_ASSERT_GREATER_OR_EQUAL(task->uiInitialPriority-TT_PRIORITY_MAX_CHANGE, task->uiCurrentPriority);
    } else {
        TEST_ASSERT_GREATER_OR_EQUAL(TT_PRIORITY_THRESHOLD, task->uiCurrentPriority);
    }
    TEST_ASSERT_GREATER_OR_EQUAL(0, task->uiCurrentPriority);
}

// < < < < < < < < <  T E S T   F U N C T I O N S  > > > > > > > > >
// create 3 tasks. dont change this.
void create_priority_tasks(){
    GetSysTick_ExpectAndReturn(0);
    GetSysTick_ExpectAndReturn(0);
    GetSysTick_ExpectAndReturn(0);
    TEST_ASSERT_EQUAL(TT_OK, TTDelay_create_task(priority_10, NULL, &output_value, 10));
    TEST_ASSERT_EQUAL(TT_OK, TTDelay_create_task(priority_5, NULL, &output_value, 5));
    TEST_ASSERT_EQUAL(TT_OK, TTDelay_create_task(priority_2, NULL, &output_value, 2));
}

// check if task created count is okay
void test_task_count_ok(){
    create_priority_tasks();
    TEST_ASSERT_EQUAL(3, TTDelay_get_task_count());
}

// check if scheduled tasks are found correctly and highest priority is scheduled
void test_find_due_task(){
    create_priority_tasks();
    GetSysTick_ExpectAndReturn(1);
    TTDelay_find_due_tasks();
    TEST_ASSERT_TRUE(TTDelay_is_due(0));
    TEST_ASSERT_TRUE(TTDelay_is_due(1));
    TEST_ASSERT_TRUE(TTDelay_is_due(2));
    // no task with this index, return false!
    TEST_ASSERT_FALSE(TTDelay_is_due(3));
    TEST_ASSERT_FALSE(TTDelay_is_due(-1));
    // next scheduled should be our highest priority task (priority_2())
    TEST_ASSERT_EQUAL(2, TTDelay_get_next_scheduled());
}

// run the highest priority task
void test_run_due_first_task(){
    // creates 3 tasks that are due and figure out which to run next
    ReadResetCpuLoadTick_IgnoreAndReturn(0);
    test_find_due_task();
    TTDelay_task_t *task;
    int next_task_id = TTDelay_get_next_scheduled();
    task = TTDelay_get_task(next_task_id);
    TEST_ASSERT_EQUAL(priority_2, task->func);
    TTDelay_run_task(next_task_id);
    TEST_ASSERT_EQUAL(DELAY_TIME, TTDelay_get_next_schedule_time(next_task_id));
    TEST_ASSERT_EQUAL(2, output_value);
}

// after highest priority was executed: run the lower priority tasks
void test_run_due_next_tasks(){
    TTDelay_task_t *task;
    int next_task_id;
    // creates 3, finds highes priority and runs highest priority
    test_run_due_first_task();

    ReadResetCpuLoadTick_IgnoreAndReturn(0);
    // lets find the next tasks to be run!
    GetSysTick_ExpectAndReturn(2);
    TTDelay_find_due_tasks();
    next_task_id = TTDelay_get_next_scheduled();
    task = TTDelay_get_task(next_task_id);
    TEST_ASSERT_EQUAL(priority_5, task->func);
    TTDelay_run_task(next_task_id);
    TEST_ASSERT_EQUAL(5, output_value);

    GetSysTick_ExpectAndReturn(3);
    TTDelay_find_due_tasks();
    next_task_id = TTDelay_get_next_scheduled();
    task = TTDelay_get_task(next_task_id);
    TEST_ASSERT_EQUAL(priority_10, task->func);
    TTDelay_run_task(next_task_id);
    TEST_ASSERT_EQUAL(10, output_value);
}

// decrease priority of tasks that are scheduled but not run
void test_adjust_priority(){
    TTDelay_task_t *task;
    test_run_due_first_task();
    TTDelay_adjust_priority();
    task = TTDelay_get_task(2);
    TEST_ASSERT_EQUAL(2, task->uiCurrentPriority);
    TEST_ASSERT_EQUAL(2, task->uiInitialPriority);
    task = TTDelay_get_task(1);
    TEST_ASSERT_GREATER_OR_EQUAL(task->uiInitialPriority-1, task->uiCurrentPriority);
    TEST_ASSERT_LESS_OR_EQUAL(task->uiInitialPriority, task->uiCurrentPriority);
    TEST_ASSERT_EQUAL(5, task->uiInitialPriority);
    task = TTDelay_get_task(0);
    TEST_ASSERT_GREATER_OR_EQUAL(task->uiInitialPriority-1, task->uiCurrentPriority);
    TEST_ASSERT_LESS_OR_EQUAL(task->uiInitialPriority, task->uiCurrentPriority);
    TEST_ASSERT_EQUAL(10, task->uiInitialPriority);
}

// run the first task, then decrease priority of the others\
   many times. check if we stay within bounds (given in TTDelay_config.h)
void test_adjust_priority_many_times(){
    TTDelay_task_t *task;
    test_run_due_first_task();
    for (int i=0 ; i<10 ; i++){
        TTDelay_adjust_priority();
    }
    task = TTDelay_get_task(2);
    TEST_ASSERT_EQUAL(2, task->uiCurrentPriority);
    TEST_ASSERT_EQUAL(2, task->uiInitialPriority);
    task = TTDelay_get_task(1);
    check_priority_in_bounds(task);
    TEST_ASSERT_EQUAL(5, task->uiInitialPriority);
    task = TTDelay_get_task(0);
    check_priority_in_bounds(task);
    TEST_ASSERT_EQUAL(10, task->uiInitialPriority);
}

// full cycle, no task. must not crash
void test_full_run_cycle_no_tasks(){
    ReadResetCpuLoadTick_IgnoreAndReturn(0);
    GetSysTick_ExpectAndReturn(3);
    int status = TTDelay_run();
    TEST_ASSERT_EQUAL(TT_OK, status);
}

// full cycle, 3 tasks
void test_full_run_cycle_three_tasks(){
    ReadResetCpuLoadTick_IgnoreAndReturn(0);
    create_priority_tasks();
    GetSysTick_ExpectAndReturn(1);
    TTDelay_run();
    TEST_ASSERT_EQUAL(2, output_value);
    GetSysTick_ExpectAndReturn(2);
    TTDelay_run();
    TEST_ASSERT_EQUAL(5, output_value);
    GetSysTick_ExpectAndReturn(3);
    TTDelay_run();
    TEST_ASSERT_EQUAL(10, output_value);
}

// full cycle, 3 tasks, high priority task before all tasks were run
void test_full_run_cycle_with_rescheduled_high_priority(){
    ReadResetCpuLoadTick_IgnoreAndReturn(0);
    create_priority_tasks();

    GetSysTick_ExpectAndReturn(0);
    TTDelay_run();
    TEST_ASSERT_EQUAL(2, output_value);

    GetSysTick_ExpectAndReturn(DELAY_TIME - 1);
    TTDelay_run();
    TEST_ASSERT_EQUAL(5, output_value);
    // at this point, the first two tasks should be rescheduled!
    GetSysTick_ExpectAndReturn(DELAY_TIME + 1);
    TTDelay_run();
    TEST_ASSERT_EQUAL(2, output_value);

    GetSysTick_ExpectAndReturn(DELAY_TIME + 2);
    TTDelay_run();
    TEST_ASSERT_EQUAL(5, output_value);

    GetSysTick_ExpectAndReturn(DELAY_TIME + 3);
    TTDelay_run();
    TEST_ASSERT_EQUAL(10, output_value);

}

/* ************************************************** */
void test_cpu_usage_calculation(){
    create_priority_tasks(); // create 3 tasks
    ReadResetCpuLoadTick_IgnoreAndReturn(0);
    uint32_t uiTotalTime = 0;
    TTDelay_cpu_usage_TypDef* cpuUsage = TTDelay_get_cpu_usage_pointer();

    GetSysTick_ExpectAndReturn(0);
    // create a 4th task (task index: 3)
    int status = TTDelay_create_task_periodic(TTDelay_cpu_usage_monitor, NULL, NULL, 50, 1000);
    TEST_ASSERT_EQUAL(TT_OK, status);

    TTDelay_get_task(0)->timeRunning = 100;
    TTDelay_get_task(1)->timeRunning = 200;
    TTDelay_get_task(2)->timeRunning = 400;
    TTDelay_get_task(3)->timeRunning = 50;

    TTDelay_set_idle_tick_count(200);
    TTDelay_set_ttsys_tick_count(50);

    TTDelay_run_task(3); // run task index 3 (ttdelay_cpu_usage_monitor)

    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.1   , cpuUsage->rTaskUsage[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.2   , cpuUsage->rTaskUsage[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.4   , cpuUsage->rTaskUsage[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.05  , cpuUsage->rTaskUsage[3]);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.2   , cpuUsage->rIdleUsage);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.05  , cpuUsage->rTtsysUsage);
}

void heavy_compute_thread_1(void *in, void* out){
    *(int*)out = *(int*)in + 1;
    TTDelay_from_last(10000);
}

void heavy_compute_thread_3(void *in, void* out){
    *(int*)out = *(int*)in + 3;
    TTDelay_from_last(10000);
}

void test_cpu_usage_two_heavy_computing_tasks(){
    TEST_IGNORE_MESSAGE("Lost track on how this one worked");
    int a = 0;
    GetSysTick_ExpectAndReturn(0);               
    TTDelay_create_task(heavy_compute_thread_1, &a, &a, 10);
    GetSysTick_ExpectAndReturn(0);
    TTDelay_create_task(heavy_compute_thread_3, &a, &a, 11);
    GetSysTick_ExpectAndReturn(0);
    TTDelay_create_task(TTDelay_cpu_usage_monitor, NULL, NULL, 12);

    // we are estimating the cpu time needed here with fixed values 
    // (because we dont have the hardware timer available)

    // first call: we just came from IDLE mode. lets say no time passed:
    ReadResetCpuLoadTick_ExpectAndReturn(10);
    GetSysTick_ExpectAndReturn(10);                     // finding due tasks
    ReadResetCpuLoadTick_ExpectAndReturn(10); // start to run the task
    ReadResetCpuLoadTick_ExpectAndReturn(100); // finish to run the task
    TTDelay_run();   // should run one "compute" (higher priority)
    TEST_ASSERT_EQUAL(1, a);

    ReadResetCpuLoadTick_ExpectAndReturn(10);
    GetSysTick_ExpectAndReturn(10);                     // finding due tasks
    ReadResetCpuLoadTick_ExpectAndReturn(10); // start to run the task
    ReadResetCpuLoadTick_ExpectAndReturn(300); // finish to run the task
    TTDelay_run();   // should run the other "compute" task
    TEST_ASSERT_EQUAL(4, a);

    ReadResetCpuLoadTick_ExpectAndReturn(10);
    GetSysTick_ExpectAndReturn(1001);                     // finding due tasks
    ReadResetCpuLoadTick_ExpectAndReturn(1001); // start to run the task
    // this will not be part of the calculation as this is measured AFTER the calculation task ran
    ReadResetCpuLoadTick_ExpectAndReturn(1040); // finish to run the task 
    TTDelay_run();   // should run cpu load calculation

    // TEST_ASSERT_FLOAT_WITHIN(0.002, 0.244, TTDelay_get_task(0)->rCpuUsage);     // 1000 / 4100
    TEST_ASSERT_FLOAT_WITHIN(0.002, 0.732, TTDelay_get_task(1)->rCpuUsage);     // 3000 / 4100
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.000, TTDelay_get_task(2)->rCpuUsage);     //    0 / 4100
    TEST_ASSERT_FLOAT_WITHIN(0.002, 0.007, TTDelay_get_idle_time_percentage()); //   30 / 4100
    TEST_ASSERT_FLOAT_WITHIN(0.002, 0.017, TTDelay_get_ttsys_time_percentage());//   70 / 4100
}