//Scheduler.h
#pragma once
#include "SystemState.h"

struct Task {
    TaskFn fn;
    ms_t period;
    ms_t next;
    uint8_t priority;
    bool enabled;
};

// 최대 태스크 수는 필요에 따라
#define MAX_TASKS 8
extern Task taskTable[MAX_TASKS];
extern uint8_t taskCount;

void scheduleLoop();
void registerTask(TaskFn fn, ms_t period, uint8_t priority, bool enabled = true);
void updateTaskPriority(TaskFn fn, uint8_t newPriority);
