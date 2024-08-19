ifeq ($(OS), Windows_NT)
OVMFFDPATH := tool\OVMF.fd
else
OVMFFDPATH := tool/OVMF.fd
endif


build:
	make -C Kernel
	make -C Boot
	make -C App

clean:
	make -C Kernel clean
	make -C Boot clean
	make -C App clean
	

run:
ifeq ($(OS), Windows_NT)
	del .\disk\NvVars
else
	rm ./disk/NvVars
endif
	qemu-system-x86_64 --monitor stdio -usb -device usb-mouse -device usb-kbd -bios $(OVMFFDPATH) -m 256M  -hda fat:rw:disk
