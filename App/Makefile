CC := clang
TARGET := x86_64-elf

build: app_x64.o
	make -C FileManager
	make -C NotePad

app_x64.o: app_x64.s

app_x64.s:
ifeq ($(OS), Windows_NT)
	copy ..\lib\app_x64.s app_x64.s
else
	cp ../lib/app_x64.s app_x64.s
endif

clean:
ifeq ($(OS), Windows_NT)
	del app_x64.s *.o
else
	rm app_x64.s *.o
endif
	make -C FileManager clean
	make -C NotePad clean

%.o: %.s
	$(CC) -I $(INCLUDE) -Wall -Wextra --target=$(TARGET) -c -O2 -fpie $<
