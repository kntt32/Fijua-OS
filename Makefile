ifeq ($(OS), Windows_NT)
OVMFFDPATH := tool\OVMF.fd
else
OVMFFDPATH := tool/OVMF.fd
endif


build: buildboot buildkernel

buildkernel:
	make -C Kernel

buildboot:
	make -C Boot

clean:
	make -C Kernel clean
	make -C Boot clean
	

run:
	qemu-system-x86_64 --monitor stdio -usb -device usb-mouse -device usb-kbd -bios $(OVMFFDPATH) -hda fat:rw:disk
