
![Unit
Tests](https://github.com/larshei/TTDelay/workflows/Unit%20Tests/badge.svg)

---
# TTDelay Readme

TTDelay stands for Tiny Time Delay and is a very basic scheduler. It may be used to design programs with mutliple „parallel“ program branches that are meant to be run periodically or with a given delay from a given point, for example sensor readouts, LED control, HUD updates or time based state machines. Only a system tick is needed and an option for monitoring the CPU usage for each task is available as well. 
TTDelay is NOT an operating system, as it is not scheduling tasks as to be expected from an operating system and cannot interrupt a running task, meaning a blocking function will block the whole system. However, there are mechanisms to introduce „pauses“ to a program branch, during wich other tasks can be run.

# TTDelay Use Example

Using TTDelay is rather simple. The snippet below is a use example printing out two numbers in an infinite loop with different print periods. You dont need to initialize the system and can create tasks right away. All the magic is done in TTDelay_run(), what will grab the highest priority task currently scheduled and execute it.


    #include <stdio.h>
    #include <TTDelay.h>

    void task_print(void *char, void *intervall) {
        printf(„%c“, *char);
        TTDelay_from_now(*intervall);
    }

    void main {
        int intervall1 = 1000;
        int intervall2 = 100;
        int char1      = ‚A‘;
        int char2      = ‚B‘;

        TTDelay_create_task (task_print,  &intervall1, &char1, 100);
        TTDelay_create_task (task_print,  &intervall2, &char2,  90);

        while (1) {
            TTDelay_run();
            sleep(1); // optional sleep statement
        }
    }


# TTDelay Setup

To use TTDelay you need the TTDelay source and header files:
* TTDelay.c
* TTDelay.h
* TTDelay_config.h

## Minimal Configuration


Minimal setup is required to have TTDelay run on your system. You need to provide a function that returns a timer value, whatever resolution it may have, and a data type of your timer value. You could, for example, use the system tick or set up a specific timer with any resolution you would like. TTDelay expects an overflowing timer, what might be adjusted in future. On top of that, the Library always reserves memory for a fixed number of tasks that you have to specify. Trying to add more tasks will not work, the additional tasks will not be created. The setup of this is done in *TTDelay_config.h*.

    #define TT_TIMER_FUNC       HAL_GetTick()
    #define TT_TIMER_TYPE       uint32_t
    #define TT_TASK_COUNT_MAX   4

If low overhead is required, try to make sure TT_TASK_COUNT_MAX is not higher than needed for your application.

## Advanced Setup

The Library supports Task aging, meaning the priority of a task is increased with every cycle while a task is scheduled but not being run. This mechanism makes sure lower priority tasks are not ignored because higher priority tasks are run all the time. There are some defines to set up the behaviour of aging, as in a limit on how much a task can age and a general threshold that forbids aging above a certain priority level. On top of this, aging might be disabled globally to reduce the systems overhead. See *Running a Task* for more information on this.

# Using TTDelay


The next chapters will describe how to create a task, run a task and describe possibilities to structure your software.

## Task Creation


Creating a task requires just a simple function call. There are two options of creating a task:

    TTDelay_create_task (void* (func)(void*,void*), 
                         void* par1, 
                         void* par2, 
                         uint8_t priority)

    TTDelay_create_task_periodic (void* (func)(void*,void*), 
                                  void* par1, 
                                  void* par2, 
                                  uint8_t priority, 
                                  TT_TIMER_TYPE period)

The parameters are:
* a pointer to a function that is run as the task itself. The function takes 2 void* arguments and returns nothing.
* two void* arguments that can be used to exchange data with the task. In your function, you can cast the pointer to whatever pointer type you need.
* a priority from 0 to 255 setting the priority of the task. If multiple tasks are to be run, TTDelay will run the highest priority task (lowest value) and increase the priority of all tasks that were scheduled but not run.
* The periodic task takes a period as an additional argument.

## Running a Task

A task may be run manually by calling `TTDelay_run_task(int index)`, however this function was meant for unit testing purposes. The first task you create is index 0, for each call of *TTDelay_create_task* or *TTDelay_create_task_periodic* the index is increased by one. This approach allows to manually run any task at any time, but the index changes whenever you change the call order of your task creation functions.

When using `TTDelay_run()`, the system will find the scheduled tasks and run the highest priority task that is scheduled. Aging is applied to all scheduled tasks that were not run. The function will return TT_OK or TT_MORE_TASKS_SCHEDULED, depending on additional tasks being scheduled or not.

If you want to run all scheduled tasks before going to sleep (or whatever your software needs to do besides the tasks given) you may use the following approach, for which disabling aging is recommended, as all tasks will be run anyways.

    while(1) {
        while (TTDelay_run() == TT_MORE_TASKS_SCHEDULED)
            Toggle_Watchdog();
        other_stuff();
    }

To run one scheduled task at a time you may do the following. Keeping aging enabled is recommended in this case.

    while(1) {
        TTDelay_run()
        Toggle_Watchdog();
        other_stuff();
    }

## Program structure

As mentioned before, TTDelay will call a function and let this function finish. Adding a delay inside a function will let TTDelay know, when the task and therefore the associated function is to be run next (the current function execution will finish without interruptions). You have multiple ways of letting TTDelay know when to run a task again and you may change the function that should be run next.

`TTDelay_from_now(TT_DELAY_TYPE delay)` will grab the currents system tick and add *delay* ticks until the next execution of the task

`TTDelay_from_last(TT_DELAY_TYPE delay)` will grab the time the task was meant to be called last and adds *delay* ticks until the next execution of the task

`TTDelay_set_next_function(void (*func ))` will set the function that will be executed when the task is run again

You have three options of defining when you want a task to be run again:

    // this will run the function every 100ms on average
    void foo1 (void* p, void* q) {
        TTDelay_from_last(100);
        // function code
    }

    // this will run the function every 100ms on average
    void foo2 (void* p, void* q) {
        // function code
        TTDelay_from_last(100);
    }

    // The function will be run at least 100ms after starting the current call
    void foo3 (void* p, void* q) {
        TTDelay_from_now(100);
        // function code
    }

    // this will run the function at least 100ms after finishing the current call
    void foo4 (void* p, void* q) {
        // function code
        TTDelay_from_now(100);
    }

If you want to initialize a sensor and then do alternating "start conversion" and "read" commands, you could use different approaches, as shown below:

This is a minimal example using a periodic approach, starting a sensor conversion right after having read the previous conversions result.

    void hdc1080_task(void* hdcHandle, void* not_used) {
        HDC1080_Init((hdc1080_t*)hdcHandle);
        HDC1080_StartMeasurement((hdc1080_t*)hdcHandle);
        TTDelay_set_next_function(hdc1080_start);
    }

    void hdc1080_read_and_start(void* hdcHandle, void* not_used) {
        HDC1080_GetMeasurement((hdc1080_t*)hdcHandle);
        HDC1080_StartMeasurement((hdc1080_t*)hdcHandle);
    }


The same can be achieved with a non-periodic setup and a wait time

    void hdc1080_task(void* hdcHandle, void* not_used) {
        HDC1080_Init((hdc1080_t*)hdcHandle);
        HDC1080_StartMeasurement((hdc1080_t*)hdcHandle);
        TTDelay_set_next_function(hdc1080_start);
    }

    void hdc1080_read_and_start(void* hdcHandle, void* not_used) {
        HDC1080_GetMeasurement((hdc1080_t*)hdcHandle);
        HDC1080_StartMeasurement((hdc1080_t*)hdcHandle);
        TTDelay_from_last(1000);
    }

Or, if you need a wait time after starting the conversion and after reading the result, you need to split up the function

    void hdc1080_task(void* hdcHandle, void* not_used) {
        HDC1080_Init((hdc1080_t*)hdcHandle);
        TTDelay_set_next_function(hdc1080_start);
    }

    void hdc1080_start(void* hdcHandle, void* not_used) {
        HDC1080_GetMeasurement((hdc1080_t*)hdcHandle);
        TTDelay_set_next_function(hdc1080_read);
        TTDelay_from_now(10);
    }

    void hdc1080_read(void* hdcHandle, void* not_used) {
        HDC1080_StartMeasurement((hdc1080_t*)hdcHandle);
        TTDelay_set_next_function(hdc1080_start);
        TTDelay_from_last(950);
    }


Alternatively, you could use a periodic task and skip the TTDelay_from_xxx calls. Then a new conversion is started right after a readout, with no delay in between. As a third alternative, you could set the period to half your readout time and switch between StartMeasurement and GetMeasurement.


To keep your code portable, it is recommended to not use any TTDelay related functionality in your modules. Instead, create e.g. a tasks.c file and wrap your own functions in task functions, as shown above.


# TTDelay CPU Usage Monitor


The system is able to estimate the CPU usage of each task by checking a counter value when a task is called and when it finishs. This is just an estimation because interrupts that are called during a tasks execution will be counted as well and the timers have a limited resolution, so tasks with very low execution time might be a bit off.

## CPU Monitor Setup


Just like the core scheduling system the cpu monitor needs a timer as well. However, the system timer is usually too slow and will not generate enough ticks during a small tasks execution. It is recommended to set up a timer with a significantly higher frequency. To enable the CPU Monitor, define TT_MONITOR_CPU_LOAD in your TTDelay_config.h and provide a function that both returns the current timer value and resets its value to 0. Because the timers value/counter needs to be reset to 0, it is NOT possible to use the main TTDelay sytem timer for this. You also need to specify how often you want to recalculate the cpu usage. Make sure your high frequency timer can not overflow within the interval you specify. The timer is reset frequently and very unlikely to overflow, but you might cause an overflow when adding up the exectution times of your tasks.

    #define TT_MONITOR_CPU_LOAD
    #define TT_READ_RST_TICK_FUNC           read_and_reset_counter_func()
    #define TT_CPU_LOAD_UPDATE_INTERVAL     1000

## CPU monitor Usage

The CPU usage calculation is a task, the tasks function is called *TTDelay_cpu_usage_monitor()* and you dont have to pass any arguments to it (meaning you can set both parameters to NULL on task creation).

    TTDelay_create_task            (TTDelay_cpu_usage_monitor    , NULL                , NULL                    ,  50);

All calculated values will be stored in a struct of type TTDelay_cpu_usage_TypDef that is created globally, you may retrieve a pointer using 

    TTDelay_cpu_usage_TypDef * pCpuUsage = TTDelay_get_cpu_usage_pointer();

The structure returned consists of float values, the first n values are the tasks cpu usage, where n equals TT_TASK_COUNT_MAX, followed by the idle usage and the sys usage. All values are relative and should add up to approx. 1.0. The Sys usage estimates the time spent on TTDelay's internal functionality, like finding tasks and aging. The Idle usage is the time spent between leaving and re-entering TTDelay_run().

    float   task0_usage = pCpuUsage->rTaskUsage[0]; // e.g. 0.035..
    float   task1_usage = pCpuUsage->rTaskUsage[1]; // e.g. 0.006..
    float   task0_usage = pCpuUsage->rTaskUsage[2]; // e.g. 0.023..
    float   idle_usage  = pCpuUsage->rIdleUsage;    // e.g. 0.917..
    float   sys_usage   = pCpuUsage->rTtsysUsage;   // e.g. 0.019..


# Unit Testing

The package includes unit tests that may be run with ceedling. All tests are specified in the file "test_TTDelay.c". When mocking TTDelay, CMock creates code with some syntax errors that have to be removed manually (e.g. missing brackets), probably caused by the usage of function pointers (?!).
