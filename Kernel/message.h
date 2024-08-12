#ifndef INCLUDED_MESSAGE_H
#define INCLUDED_MESSAGE_H

typedef struct {
    uint16 taskId;
    Task_Message message;
} Message_Object;

typedef struct {
    uintn changedFlag;
    Queue queue;
} Message;

void Message_Init(void);

uintn Message_EnQueue(uint16 taskId, Task_Message* message);

void Message_RemoveByTaskId(uint16 taskId);

void Message_Update(void);

#endif
