CC := clang
TARGET := x86_64-elf

ifeq ($(OS), Windows_NT)
INCLUDE := ..\include
else
INCLUDE := ../include
endif

build: a.elf

clean:
ifeq ($(OS), Windows_NT)
	del *.o *.elf
	del elfloader.c app_x64.s
else
	rm *.o *.elf
	rm elfloader.c app_x64.s
endif

a.elf: main.o font.o bootwrapper.o x64.o console.o timer.o functions.o queue.o memory.o graphic.o task.o  test.o layer.o mouse.o keyboard.o syscall.o message.o terminal.o shell.o app_x64.o file.o taskbar.o elfloader.o clipboard.o
	ld.lld --entry entrypoint -z norelro --static --image-base 0x200000 -o a.elf $^
ifeq ($(OS), Windows_NT)
	copy a.elf ..\disk\kernel.elf
else
	cp a.elf ../disk/kernel.elf
endif

main.o: main.c functions.h graphic.h console.h timer.h memory.h queue.h task.h test.h layer.h mouse.h keyboard.h syscall.h terminal.h shell.h message.h file.h x64.h
functions.o: functions.c functions.h x64.h queue.h task.h console.h terminal.h shell.h
queue.o: queue.c queue.h memory.h console.h functions.h
font.o: font.c graphic.h font.h font_bitmap
console.o: console.c graphic.h console.h font.h layer.h
bootwrapper.o: bootwrapper.s
x64.o: x64.s
keyboard.o: keyboard.c keyboard.h functions.h x64.h queue.h task.h message.h console.h graphic.h layer.h
timer.o: timer.c timer.h x64.h console.h mouse.h keyboard.h
memory.o: memory.c memory.h functions.h x64.h console.h
graphic.o: graphic.c graphic.h graphic_mouse_bitmap
task.o: task.c queue.h task.h memory.h x64.h graphic.h layer.h message.h console.h
layer.o: layer.c memory.h graphic.h layer.h mouse.h queue.h task.h font.h message.h
mouse.o: mouse.c mouse.h graphic.h layer.h functions.h console.h x64.h
syscall.o: syscall.c syscall.h queue.h task.h x64.h console.h layer.h graphic.h font.h message.h functions.h file.h memory.h terminal.h clipboard.h
message.o: message.c queue.h message.h task.h functions.h
file.o: file.c file.h console.h x64.h functions.h
taskbar.o: taskbar.c taskbar.h graphic.h queue.h task.h terminal.h file.h layer.h x64.h console.h functions.h
clipboard.o: clipboard.c clipboard.h memory.h

terminal.o: terminal.c graphic.h queue.h task.h terminal.h file.h
shell.o: shell.c graphic.h queue.h task.h shell.h file.h
app_x64.o: app_x64.s

elfloader.o: elfloader.c

test.o: test.c test.h queue.h task.h graphic.h functions.h console.h

elfloader.c:
ifeq ($(OS), Windows_NT)
	copy ..\lib\elfloader.c elfloader.c
else
	cp ../lib/elfloader.c elfloader.c
endif

app_x64.s:
ifeq ($(OS), Windows_NT)
	copy ..\lib\app_x64.s app_x64.s
else
	cp ../lib/app_x64.s app_x64.s
endif

%.o: %.c
	$(CC) -I $(INCLUDE) -Wall -Wextra --target=$(TARGET) -fshort-wchar -ffreestanding -mno-red-zone -c -O2 $<

%.o: %.s
	$(CC) -I $(INCLUDE) -Wall -Wextra --target=$(TARGET) -c -O2 $<
