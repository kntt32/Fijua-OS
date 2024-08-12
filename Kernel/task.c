#include <types.h>
#include <kernel.h>
#include <efi.h>
#include "queue.h"
#include "task.h"
#include "memory.h"
#include "x64.h"
#include "graphic.h"
#include "layer.h"
#include "message.h"
#include "console.h"

#include "functions.h"

void* Task_NewTask_Asm_SetStartContext(void* stackptr);
void Task_ContextSwitch(void);

static const uintn Task_DefaultStackPageSize = 5000;//4KiB*1000

static Task task;

//Initialize TaskScheduler
void Task_Init(void) {
    Console_Print("Task_Init: Initializing Task...\n");

    //reset
    task.haltFlag = 0;
    task.kernelStackPtr = NULL;

    task.Queue.runningTaskId = 0;
    Queue_Init(&(task.Queue.app), sizeof(uint16));

    task.Table.count = 0;
    task.Table.listPages = 0;
    task.Table.list = NULL;

    return;
}


//Expand size of task.table.list
static uintn Task_Table_Expand(void) {
    uintn newListPages = task.Table.listPages*2+1;
    Task_Object* newList = Memory_AllocPages(2, newListPages);
    if(newList == NULL) return 1;

    Functions_MemCpy(newList, task.Table.list, sizeof(Task_Object)*task.Table.count);

    Memory_FreePages(2, task.Table.listPages, task.Table.list);
    
    task.Table.list = newList;
    task.Table.listPages = newListPages;

    return 0;
}


//Allocate NewTaskID
static uint16 Task_SeekNewTaskID(void) {
    uint16 nextTaskId = 3;

    uintn i = 0;
    while(1) {
        if(task.Table.count<i) break;

        if(task.Table.list[i].taskId == nextTaskId) {
            nextTaskId++;
            i = 0;
            continue;
        }
        i++;
    }

    return nextTaskId;
}


//Get Index in TaskList
static sintn Task_GetIndexOfTaskList(uint16 taskId) {
    if(taskId == 0 || taskId == 1) return -1;

    for(uintn i=0; i<task.Table.count; i++) {
        if(task.Table.list[i].taskId == taskId) return i;
    }

    return -1;
}


//Add NewTask and return taskID
uint16 Task_New(sintn (*taskEntry)(void), uint16 stdin_taskId, uint16 stdout_taskId) {
    if(taskEntry == NULL) return 0;

    uint16 newTaskId = Task_SeekNewTaskID();
    if(newTaskId == 0) return 0;

    void* stackPtr = Memory_AllocPages(newTaskId, Task_DefaultStackPageSize);
    if(stackPtr == NULL) return 0;

    if(task.Table.listPages < (((task.Table.count+1)*sizeof(Task_Object)+0xfff)>>12)) {
        if(Task_Table_Expand()) {
            Memory_FreeAll(newTaskId);
            return 0;
        }
    }

    task.Table.list[task.Table.count].taskId = newTaskId;
    task.Table.list[task.Table.count].stdin_taskId = stdin_taskId;
    task.Table.list[task.Table.count].stdout_taskId = stdout_taskId;
    task.Table.list[task.Table.count].stackPtr = Task_NewTask_Asm_SetStartContext((void*)(((uintn)stackPtr)+Task_DefaultStackPageSize-1));
    task.Table.list[task.Table.count].taskEntry = taskEntry;
    Queue_Init(&(task.Table.list[task.Table.count].messages), sizeof(Task_Message));

    task.Table.count++;

    Task_EnQueueTask(newTaskId);

    return newTaskId;
}


//Delete Task
void Task_Delete(uint16 taskId) {
    if(taskId == 0) return;

    sintn taskIndex = Task_GetIndexOfTaskList(taskId);
    if(taskIndex == -1) return;

    //stdin_taskId, stdout_taskIdにTask_Message_Quitを送信
    Task_Message message;
    message.type = Task_Message_Quit;
    Task_Messages_EnQueue(task.Table.list[taskIndex].stdin_taskId, &message);
    Task_Messages_EnQueue(task.Table.list[taskIndex].stdout_taskId, &message);
    for(uintn i=0; i<task.Table.count; i++) {
        if(task.Table.list[i].stdin_taskId == taskId) {
            Task_Messages_EnQueue(task.Table.list[i].taskId, &message);
        }
        if(task.Table.list[i].stdout_taskId == taskId) {
            Task_Messages_EnQueue(task.Table.list[i].taskId, &message);
        }
    }

    //メッセージキュー/タスクキューから削除するタスクを消去
    const uint16 taskId_Null = 0;
    Queue_DeInit(&(task.Table.list[taskIndex].messages));
    Message_RemoveByTaskId(taskId);
    Queue_Replace(&(task.Queue.app), &taskId, &taskId_Null);

    for(uintn i=taskIndex; i<task.Table.count; i++) {
        Functions_MemCpy(task.Table.list+i, task.Table.list+i+1, sizeof(Task_Object));
    }
    task.Table.count--;

    Memory_FreeAll(taskId);
    Layer_Window_DeleteAll(taskId);

    Task_Yield();

    return;
}


//NewTask StartPoint
void Task_New_StartPoint() {
    sintn taskIndex = Task_GetIndexOfTaskList(task.Queue.runningTaskId);
    if(taskIndex != -1) {
        task.Table.list[taskIndex].taskEntry();
    }

    //end task
    Task_Delete(task.Queue.runningTaskId);
    while(1) Task_Yield();
}


//EnQueue to TaskQueue
uintn Task_EnQueueTask(uint16 taskId) {
    if(taskId == 0 || taskId == 1 || taskId == 2) {
        return 1;
    }

    sintn taskIndex;
    taskIndex = Task_GetIndexOfTaskList(taskId);
    if(taskIndex == -1) {
        return 2;
    }

    if(!Queue_IsExist(&(task.Queue.app), &taskId)) {
        Queue_EnQueue(&(task.Queue.app), &taskId);
    }
    return 0;
}

void Mouse_CheckState();
void Keyboard_CheckState();

//Yield
void Task_Yield(void) {
    Mouse_CheckState();
    Keyboard_CheckState();
    Layer_Update();
    Message_Update();

    Task_ContextSwitch();
    
    return;
}


//Halt
void Task_Halt(void) {
    task.haltFlag = 1;
    Task_Yield();
    return;
}


//Subroutine of Asm function "Task_ContextSwitch"
void* Task_ContextSwitch_Subroutine(void* currentStackPtr) {
    if(task.Queue.runningTaskId == 0) {
        task.kernelStackPtr = currentStackPtr;
    }else {
        sintn taskIndex = Task_GetIndexOfTaskList(task.Queue.runningTaskId);
        if(!(task.haltFlag) && taskIndex != -1) {
            Task_EnQueueTask(task.Queue.runningTaskId);
        }
        if(taskIndex != -1) {
            task.Table.list[taskIndex].stackPtr = currentStackPtr;
        }
    }
    task.haltFlag = 0;



    //seek nextTaskId
    uint16 nextTaskId = 0;
    sintn nextTaskIndex = 0;
    if(nextTaskId == 0 && task.Queue.app.count != 0) {
        while(1) {
            if(Queue_DeQueue(&(task.Queue.app), &nextTaskId) == NULL) break;
            nextTaskIndex = Task_GetIndexOfTaskList(nextTaskId);
            if(nextTaskIndex == -1) nextTaskId = 0;
            if(nextTaskId != 0 || task.Queue.app.count == 0) break;
        }
    }


    //switch to KernelStackPtr
    if(nextTaskId == 0) {
        task.Queue.runningTaskId = 0;
        return task.kernelStackPtr;
    }
    task.Queue.runningTaskId = nextTaskId;
    return task.Table.list[nextTaskIndex].stackPtr;
}


//実行中のtaskIdを返す
uint16 Task_GetRunningTaskId(void) {
    return task.Queue.runningTaskId;
}


//taskIdのstdinを取得
uint16 Task_GetStdIn(uint16 taskId) {
    sintn taskIndex = Task_GetIndexOfTaskList(taskId);
    if(taskIndex < 0) return 0;

    return task.Table.list[taskIndex].stdin_taskId;
}


//taskIdのstdoutを取得
uint16 Task_GetStdOut(uint16 taskId) {
    sintn taskIndex = Task_GetIndexOfTaskList(taskId);
    if(taskIndex < 0) return 0;

    return task.Table.list[taskIndex].stdout_taskId;
}


//taskIdのstdinを変更
void Task_ChangeStdIn(uint16 taskId, uint16 stdin_taskId) {
    sintn taskIndex = Task_GetIndexOfTaskList(taskId);
    if(taskIndex < 0) return;
    sintn taskIndex_stdin = Task_GetIndexOfTaskList(stdin_taskId);
    if(taskIndex_stdin < 0) return;

    task.Table.list[taskIndex].stdin_taskId = stdin_taskId;

    return;
}


//taskIdのstdoutを変更
void Task_ChangeStdOut(uint16 taskId, uint16 stdout_taskId) {
    sintn taskIndex = Task_GetIndexOfTaskList(taskId);
    if(taskIndex < 0) return;
    sintn taskIndex_stdout = Task_GetIndexOfTaskList(stdout_taskId);
    if(taskIndex_stdout < 0) return;

    task.Table.list[taskIndex].stdout_taskId = stdout_taskId;

    return;
}


//taskIdのメッセージキューにメッセージを追加
uintn Task_Messages_EnQueue(uint16 taskId, const Task_Message* message) {
    if(taskId == 0 || message == NULL) return 1;

    if(taskId == 1) {
        for(uintn i=0; i<task.Table.count; i++) {
            Queue_EnQueue(&(task.Table.list[i].messages), message);

            Task_EnQueueTask(task.Table.list[i].taskId);
        }
    }else {
        sintn taskIndex = Task_GetIndexOfTaskList(taskId);
        if(taskIndex < 0) return 2;
        Queue_EnQueue(&(task.Table.list[taskIndex].messages), message);

        Task_EnQueueTask(taskId);
    }

    return 0;
}


//taskIdのメッセージキューのメッセージを取得して*messageに書き込み
uintn Task_Messages_Check(uint16 taskId, Task_Message* message) {
    if(taskId == 0 || message == NULL) return 1;

    sintn taskIndex = Task_GetIndexOfTaskList(taskId);
    if(taskId < 0) return 2;

    if(Queue_Check(&(task.Table.list[taskIndex].messages), message) == NULL) {
        message->type = Task_Message_Nothing;
        return 0;
    }

    return 0;
}


//taskIdのメッセージキューからメッセージをを取り出して*messageに書き込み
uintn Task_Messages_DeQueue(uint16 taskId, Task_Message* message) {
    if(taskId == 0 || message == NULL) return 1;

    sintn taskIndex = Task_GetIndexOfTaskList(taskId);
    if(taskId < 0) return 2;

    if(Queue_DeQueue(&(task.Table.list[taskIndex].messages), message) == NULL) {
        message->type = Task_Message_Nothing;
        return 0;
    }

    return 0;
}

