//Scheduler.cpp
#include "Scheduler.h"


Task taskTable[MAX_TASKS];
uint8_t taskCount = 0;

void registerTask(TaskFn fn, ms_t period, uint8_t priority, bool enabled) { // 태스크 등록 함수, 필요 인수 = Task의 구성요소들 그대로
    if (taskCount >= MAX_TASKS) return;
    taskTable[taskCount++] = { fn, period, millis(), priority, enabled }; // ++ 요긴하게 활용하기 ㅎ
}

void updateTaskPriority(TaskFn fn, uint8_t newPriority) {  // 태스크 동적 변화를 위한 함수, 멤버변수인 priority 접근해서 바꾸기
    for (int i = 0; i < taskCount; i++) {
        if (taskTable[i].fn == fn) {
            taskTable[i].priority = newPriority;
            return;
        }
    }
}

void scheduleLoop() {       //  우선순위 기반 태스크 실행, 이번 코드의 핵심 main loop
    ms_t now = millis();
    Task* best = nullptr;
    for (int i = 0; i < taskCount; i++) {
        Task& t = taskTable[i];
        if (!t.enabled) continue;
        if (now >= t.next && (!best || t.priority < best->priority)) {
            best = &t;
        }
    }
    if (best) {
        best->fn();
        best->next = now + best->period;
    } else {
        delay(1);
    }
}
