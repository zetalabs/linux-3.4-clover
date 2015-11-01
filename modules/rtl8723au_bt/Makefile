
################################################################################
#
# Common Variables that already set:
#     LICHEE_KDIR
#     LICHEE_MOD_DIR
#     CROSS_COMPILE
#     ARCH
#
#################################################################################
PWD=$(shell pwd)
obj-m+=rtk_btusb.o

install: build
	cp rtk_btusb.ko $(LICHEE_MOD_DIR)/
	cp rtl8723a_fw $(LICHEE_MOD_DIR)/rtl8723a_fw.ko
	cp rtl8723a_config.bin $(LICHEE_MOD_DIR)/rtl8723a_config.bin.ko

build:
	@echo $(LICHEE_KDIR)
	$(MAKE) -C $(LICHEE_KDIR) M=$(PWD)

clean:
	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~
