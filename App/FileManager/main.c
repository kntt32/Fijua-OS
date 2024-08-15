#include <types.h>
#include "../../Kernel/graphic.h"
#include "../../Kernel/queue.h"
#include "../../Kernel/task.h"
#include "../../Kernel/file.h"
#include <app_syscall_x64.h>

sintn main() {
    App_Syscall_StdOut("Hello, World!\n", sizeof("Hello, World!\n"));

    return 0;
}
