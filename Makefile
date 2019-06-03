obj-m += ouichefs.o
ouichefs-objs := super.o inode.o file.o dir.o hash.o rbthash.o fs.o dedup.o 

KERNELDIR ?= ~/sorbonne/tp-pnl/linux-4.19.3/

all:
	make -C $(KERNELDIR) M=$(PWD) modules

debug:
	make -C $(KERNELDIR) M=$(PWD) ccflags-y+="-DDEBUG -g" modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm -rf *~

.PHONY: all clean
