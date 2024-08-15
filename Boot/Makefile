CC := clang
TARGET := x86_64-pc-win32-coff
FILES := main.c
ifeq ($(OS), Windows_NT)
INCLUDE := ..\include
else
INCLUDE := ../include
endif

build: a.efi

clean:
ifeq ($(OS), Windows_NT)
	del *.o *.efi
	del elfloader.c elfloader.h
else
	rm *.o *.efi
	rm elfloader.c elfloader.h
endif

a.efi: main.o elfloader.o subroutine.o
	lld-link /subsystem:efi_application /entry:efi_main /out:a.efi $^
ifeq ($(OS), Windows_NT)
	copy a.efi ..\disk\EFI\BOOT\BOOTX64.EFI
else
	cp a.efi ../disk/EFI/BOOT/BOOTX64.EFI
endif

main.o: main.c subroutine.h

elfloader.o: elfloader.c

elfloader.c:
ifeq ($(OS), Windows_NT)
	copy ..\lib\elfloader.c elfloader.c
else
	cp ../lib/elfloader.c elfloader.c
endif


%.o: %.c
	$(CC) -I $(INCLUDE) -target $(TARGET) -mno-red-zone -fno-stack-protector -fshort-wchar -Wall -Wextra -c $<

%.o: %.asm
	$(CC) -target $(TARGET) -Wall -Wextra -c $<
