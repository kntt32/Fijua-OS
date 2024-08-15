CC := clang
TARGET := x86_64-elf

build: app_x64.o
	make -C FileManager

app_x64.o: app_x64.s

app_x64.s:
ifeq ($(OS), Windows_NT)
	copy ..\lib\app_x64.s app_x64.s
else
	cp ../lib/app_x64.s app_x64.s
endif

clean:
	make -C FileManager clean

%.o: %.s
	$(CC) -I $(INCLUDE) -Wall -Wextra --target=$(TARGET) -c -O2 -fpie $<
