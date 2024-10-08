#ifndef INCLUDED_TASK_H
#define INCLUDED_TASK_H

#define Task_TaskId_NULL (0)

#define Task_Object_Tasklevel_app (0)


typedef struct {
    enum {
        Task_Message_Nothing,
        Task_Message_IPCMessage,
        Task_Message_CloseWindow,
        Task_Message_Quit,
        Task_Message_KeyPushed,
        Task_Message_MouseLayerEvent,
        Task_Message_Private_LayerEvent,
        Task_Message_ENUMCOUNT
    } type;

    union {
        struct {

        } Nothing;

        struct {
            uint64 u64;
            ascii str[32];
        } IPCMessage;

        struct {
            uintn layerId;
        } CloseWindow;
        
        struct {
            ascii asciiCode;
            uint16 scanCode;
        } KeyPushed;

        struct {
            uintn layerId;
            sintn x;
            sintn y;
            uintn leftButton;
            uintn rightButton;
        } MouseLayerEvent;

        struct {
            void* layer;
        } Private_LayerEvent;
    } data;
} Task_Message;


typedef struct {
    uint16 taskId;//0,1は無効、2はカーネル用に予約済み
    uint16 stdio_taskId;//StdIoに使うTaskId
    void* stackPtr;
    sintn (*taskEntry)(ascii arg[32]);
    Queue messages;

    ascii arg[32];
} Task_Object;


typedef struct {
    uint8 haltFlag;

    void* kernelStackPtr;

    struct {
        uint16 runningTaskId;
        
        Queue app;
    } Queue;

    struct {
        uintn count;
        uintn listPages;
        
        Task_Object* list;
    } Table;
} Task;


void Task_Init(void);

uint16 Task_New(void* taskEntry, uint16 stdio_taskId, const ascii arg[32]);
void Task_Delete(uint16 taskId);
uintn Task_EnQueueTask(uint16 taskId);

void Task_Yield(void);
void Task_Halt(void);

void Task_ChangeContext(void);

uint16 Task_GetRunningTaskId(void);
uint16 Task_GetStdIo(uint16 taskId);
void Task_ChangeStdIo(uint16 taskId, uint16 stdio_taskId);

uintn Task_Messages_EnQueue(uint16 taskId, const Task_Message* message);
uintn Task_Messages_Check(uint16 taskId, Task_Message* message);
uintn Task_Messages_DeQueue(uint16 taskId, Task_Message* message);

#endif
