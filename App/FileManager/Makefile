CC := clang
TARGET := x86_64-elf

ifeq ($(OS), Windows_NT)
INCLUDE := ..\..\include
else
INCLUDE := ../../include
endif

build: a.elf

clean:
ifeq ($(OS), Windows_NT)
	del *.o *.elf
	del app_x64.s
else
	rm *.o *.elf
	rm app_x64.s
endif

a.elf: main.o app_x64.o
	ld.lld --entry main -z norelro --static -pie -o a.elf $^
ifeq ($(OS), Windows_NT)
	copy a.elf ..\..\disk\app\filemanager.elf
else
	cp a.elf ../../disk/app/filemanager.elf
endif

main.o: main.c
app_x64.o:
ifeq ($(OS), Windows_NT)
	copy ..\app_x64.o app_x64.o
else
	cp ../app_x64.o app_x64.o
endif


%.o: %.c
	$(CC) -I $(INCLUDE) -Wall -Wextra --target=$(TARGET) -fshort-wchar -ffreestanding -mno-red-zone -c -O2 -fpie $<

%.o: %.s
	$(CC) -I $(INCLUDE) -Wall -Wextra --target=$(TARGET) -c -O2 -fpie $<
