#include <types.h>
#include "queue.h"
#include "memory.h"
#include "console.h"
#include "functions.h"

#define Queue_TaskId (2)

//Queueを初期化
Queue* Queue_Init(Queue* this, uintn perSize) {
    if(this == NULL || perSize == 0) return NULL;

    this->start = 0;
    this->count = 0;
    this->perSize = perSize;
    this->poolPages = 0;
    this->objectPool = NULL;

    return this;
}


//Queueを解放
void Queue_DeInit(Queue* this) {
    if(this == NULL) return;

    Memory_FreePages(Queue_TaskId, this->poolPages, this->objectPool);

    this->start = 0;
    this->count = 0;
    this->perSize = 1;
    this->poolPages = 0;
    this->objectPool = NULL;

    return;
}


//Queueのindex番目の要素を取得して*elementに戻す
Queue* Queue_GetElementByIndex(Queue* this, uintn index, void* element) {
    if(this == NULL || this->count <= index || element == NULL) return NULL;

    uint8* elementptr = NULL;
    Queue_GetElementPtrByIndex(this, index, (void**)&elementptr);
    if(elementptr == NULL) return NULL;

    uint8* element_uint8Ptr = (uint8*)element;

    for(uintn i=0; i<this->perSize; i++) {
        element_uint8Ptr[i] = elementptr[i];
    }

    return this;
}


//Queueのindex番目の要素のポインタを取得して*elementPtrに戻す
Queue* Queue_GetElementPtrByIndex(Queue* this, uintn index, void** elementPtr) {
    if(this == NULL || this->count <= index || elementPtr == NULL) return NULL;

    uintn realIndex = this->start + index*this->perSize;
    uintn maxIndex = (this->poolPages<<12) - ((this->poolPages<<12)%this->perSize);
    if(maxIndex <= realIndex+this->perSize) realIndex -= maxIndex;

    *elementPtr = this->objectPool + realIndex;

    return this;
}


//QueueのobjectPoolを拡張
static Queue* Queue_Expand(Queue* this) {
    if(this == NULL) return NULL;
    uintn newPoolPages = this->poolPages*2 + 1;
    uint8* newObjectPool = Memory_AllocPages(Queue_TaskId, newPoolPages);
    if(newObjectPool == NULL) return NULL;

    for(uintn i=0; i<this->count; i++) {
        Queue_GetElementByIndex(this, i, newObjectPool + this->perSize*i);
    }
    Memory_FreePages(Queue_TaskId, this->poolPages, this->objectPool);
    this->start = 0;
    this->poolPages = newPoolPages;
    this->objectPool = newObjectPool;
    return this;
}


//Queueに追加
Queue* Queue_EnQueue(Queue* this, const void* object) {
    if(this == NULL || object == NULL) return NULL;
    if((this->poolPages << 12) <= (this->count+1)*this->perSize) {
        if(Queue_Expand(this) == NULL) return NULL;
    }
    uint8* object_uint8Ptr = (uint8*)object;
    uint8* move2_uint8Ptr = NULL;
    this->count ++;
    if(Queue_GetElementPtrByIndex(this, this->count-1, (void**)&move2_uint8Ptr) == NULL) {
        this->count --;
        return NULL;
    }
    for(uintn i=0; i<this->perSize; i++) {
        move2_uint8Ptr[i] = object_uint8Ptr[i];
    }
    return this;
}


//Queueの*fromを*toに置き換え
Queue* Queue_Replace(Queue* this, const void* from, const void* to) {
    if(this == NULL || from == NULL || to == NULL) return NULL;

    const uint8* from_uint8Ptr = (const uint8*)from;
    const uint8* to_uint8Ptr = (const uint8*)to;

    for(uintn i=0; i<this->count; i++) {
        //対象の要素がfromと等しいか判定
        uintn isEqualFlag = 1;
        uint8* objectPool_target_uint8Ptr = NULL;
        if(Queue_GetElementPtrByIndex(this, i, (void**)&objectPool_target_uint8Ptr) == NULL) return NULL;
        for(uintn k=0; k<this->perSize; k++) {
            if(objectPool_target_uint8Ptr[k] != from_uint8Ptr[k]) {
                isEqualFlag = 0;
                break;
            }
        }

        //等しいなら置き換える
        if(isEqualFlag) {
            for(uintn k=0; k<this->perSize; k++) {
                objectPool_target_uint8Ptr[k] = to_uint8Ptr[k];
            }
        }
    }

    return this;
}


//Queueをチェック
Queue* Queue_Check(Queue* this, void* object) {
    if(this == NULL || object == NULL) return NULL;

    if(Queue_GetElementByIndex(this, 0, object) == NULL) return NULL;

    return this;
}


//Queueを表示
Queue* Queue_Print(Queue* this) {
    if(this->count == 0) return NULL;
    for(uintn i=0; i<this->count; i++) {
        ascii strBuff[18];
        uint8* ptr;
        Queue_GetElementPtrByIndex(this, i, (void**)&ptr);
        SPrintIntX(*ptr, 17, strBuff);
        strBuff[17] = ':';
        Console_Print(strBuff);
    }
    Console_Print("\n");
    return this;
}


//Queueから取り出す
Queue* Queue_DeQueue(Queue* this, void* object) {
    if(this == NULL || this->count == 0) return NULL;

    if(Queue_Check(this, object) == NULL) return NULL;

    Queue_GetElementByIndex(this, 0, object);

    uint64 secondQueuePtr;
    Queue_GetElementPtrByIndex(this, 1, (void**)&secondQueuePtr);

    this->count --;
    this->start = secondQueuePtr - (uint64)(this->objectPool);

    return this;
}


//Queueに存在しているか判定
uintn Queue_IsExist(Queue* this, void* object) {
    if(this == NULL || object == NULL) return 0;

    const uint8* object_uint8Ptr = (const uint8*)object;

    for(uintn i=0; i<this->count; i++) {
        uintn isEqualFlag = 1;

        //対象の要素がfromと等しいか判定
        uint8* objectPool_target_uint8Ptr = NULL;
        if(Queue_GetElementPtrByIndex(this, i, (void**)&objectPool_target_uint8Ptr) == NULL) return 0;
        for(uintn k=0; k<this->perSize; k++) {
            if(objectPool_target_uint8Ptr[k] != object_uint8Ptr[k]) {
                isEqualFlag = 0;
                break;
            }
        }

        if(isEqualFlag) return 1;
    }

    return 0;
}

