#ifndef INCLUDED_QUEUE_H
#define INCLUDED_QUEUE_H

typedef struct {
    uintn start;//FIFOのスタートが何バイト目か
    uintn count;//FIFO内のオブジェクトの数
    uintn perSize;//オブジェクト一つあたりのバイト数
    uintn poolPages;//objectPoolのページ数
    uint8* objectPool;//オブジェクトプールのポインタ
} Queue;


Queue* Queue_Init(Queue* this, uintn perSize);
void Queue_DeInit(Queue* this);
Queue* Queue_GetElementByIndex(Queue* this, uintn index, void* element);
Queue* Queue_GetElementPtrByIndex(Queue* this, uintn index, void** element);
Queue* Queue_EnQueue(Queue* this, const void* object);
Queue* Queue_Replace(Queue* this, const void* from, const void* to);//構造体を管理するQueueの場合、この関数は使用すべきでない
Queue* Queue_Check(Queue* this, void* object);
Queue* Queue_Print(Queue* this);
Queue* Queue_DeQueue(Queue* this, void* object);
uintn Queue_IsExist(Queue* this, void* object);


#endif
