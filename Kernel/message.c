#include <types.h>
#include "queue.h"
#include "task.h"
#include "message.h"
#include "functions.h"
#include "x64.h"

#include "console.h"

static Message message;

void Message_Init(void) {
    message.changedFlag = 0;
    Queue_Init(&(message.queue), sizeof(Message_Object));

    return;
}


//システム全体のメッセージキューにメッセージを追加 taskId:1で全タスクに通知
uintn Message_EnQueue(uint16 taskId, Task_Message* task_message) {
    if(taskId == 0) return 1;

    Message_Object messageObject;
    messageObject.taskId = taskId;
    Functions_MemCpy(&(messageObject.message), task_message, sizeof(Task_Message));
    
    Queue_EnQueue(&(message.queue), &messageObject);

    message.changedFlag = 1;

    return 0;
}


//taskIdへのメッセージを全クリア
void Message_RemoveByTaskId(uint16 taskId) {
    if(taskId == 0) return;

    Message_Object* tempObject;
    for(uintn i=0; i<message.queue.count; i++) {
        Queue_GetElementPtrByIndex(&(message.queue), i, (void**)&tempObject);
        if(tempObject->taskId == taskId) tempObject->taskId = 0;
    }

    return;
}


//システム全体のMessageQueueのメッセージを各タスクに送信
void Message_Update(void) {
    if(!message.changedFlag) return;

    message.changedFlag = 0;

    Message_Object tempObject;
    for(uintn i=message.queue.count-1; 0<=i; i++) {
        if(Queue_DeQueue(&(message.queue), (void*)&tempObject) == NULL) break;
        if((uintn)Task_Message_ENUMCOUNT <= (uintn)tempObject.message.type) continue;
        Task_Messages_EnQueue(tempObject.taskId, &(tempObject.message));
    }

    return;
}

