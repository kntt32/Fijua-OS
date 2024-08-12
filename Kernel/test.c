#include <types.h>
#include "test.h"
#include "queue.h"
#include "task.h"
#include "graphic.h"

#define slow for(uintn k=0; k<10000; k++) {}

//task test2
sintn Test_Tester3(void) {
    Graphic_Color color;
    color.red = 0;
    color.green = 0;
    color.blue = 0;

    while(1) {
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(400, 500, 100, 100, color);
            slow;
            color.red++;
        }
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(400, 500, 100, 100, color);
            slow;
            color.green++;
        }
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(400, 500, 100, 100, color);
            slow;
            color.blue++;
        }

        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(400, 500, 100, 100, color);
            slow;
            color.red--;
        }
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(400, 500, 100, 100, color);
            slow;
            color.green--;
        }
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(400, 500, 100, 100, color);
            slow;
            color.blue--;
        }
        return 0;
    }
}

//task test2
sintn Test_Tester2(void) {
    Graphic_Color color;
    color.red = 0;
    color.green = 0;
    color.blue = 0;

    for(uintn t=0; t<10; t++) {
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(200, 500, 100, 100, color);
            slow;
            color.red++;
            Task_Yield();
        }
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(200, 500, 100, 100, color);
            slow;
            color.green++;
            Task_Yield();
        }
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(200, 500, 100, 100, color);
            slow;
            color.blue++;
            Task_Yield();
        }
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(200, 500, 100, 100, color);
            slow;
            color.red--;
            Task_Yield();
        }
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(200, 500, 100, 100, color);
            slow;
            color.green--;
            Task_Yield();
        }
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(200, 500, 100, 100, color);
            slow;
            color.blue--;
            Task_Yield();
        }
    }
    return 0;
}

//task test
sintn Test_Tester1(void) {
    Graphic_Color color;
    color.red = 0;
    color.green = 0;
    color.blue = 0;

    for(sintn t=0; t<5; t++) {
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(0, 500, 100, 100, color);
            slow;
            color.red++;
            Task_Yield();
        }
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(0, 500, 100, 100, color);
            slow;
            color.green++;
            Task_Yield();
        }
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(0, 500, 100, 100, color);
            slow;
            color.blue++;
            Task_Yield();
        }
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(0, 500, 100, 100, color);
            slow;
            color.red--;
            Task_Yield();
        }
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(0, 500, 100, 100, color);
            slow;
            color.green--;
            Task_Yield();
        }
        for(uintn i=0; i<0xff; i++) {
            Graphic_DrawSquare(0, 500, 100, 100, color);
            slow;
            color.blue--;
            Task_Yield();
        }
    }
    return 0;
}
