obj-m += sysfs_temp_hum.o
PWD := $(CURDIR)
OUTDIR := $(PWD)/build

all:
	rm -rf $(OUTDIR)
	mkdir -p $(OUTDIR)
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	find . -maxdepth 1 -type f \( \
		-name '*.o' -o \
		-name '*.ko' -o \
		-name '*.mod' -o \
		-name '*.mod.*' -o \
		-name '*.symvers' -o \
		-name '*.order' -o \
		-name '*.cmd' -o \
		-name '.*.cmd' \
	\) -exec mv -t $(OUTDIR) {} +

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf $(OUTDIR)
