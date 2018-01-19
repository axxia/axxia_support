################################################################################
#
# GNUmakefile
#
# Image makefile for Axxia systems.  Along with CROSS_COMPILE and
# tools in the PATH, the following are required.
#
#        ARCH : The architecture.
# DESCRIPTION : A descriptive string for the image.
#     ADDRESS : The load and entry address.
#         DTB : The device tree.
#     VMLINUX : The Linux ELF image.
#     RAMDISK : A disk image, ext2 for example.
#
################################################################################

ifdef ARCH
ASSERT_ARCH = 
else
ASSERT_ARCH = \
@echo "ARCH is not defined!" ; exit 1
endif

ifdef DESCRIPTION
ASSERT_DESCRIPTION = 
else
ASSERT_DESCRIPTION = \
@echo "DESCRIPTION is not defined!" ; exit 1
endif

ifdef ADDRESS
ASSERT_ADDRESS = 
else
ASSERT_ADDRESS = \
@echo "ADDRESS is not defined!" ; exit 1
endif

ifdef DTB
ASSERT_DTB = 
else
ASSERT_DTB = \
@echo "DTB is not defined!" ; exit 1
endif

ifdef VMLINUX
ASSERT_VMLINUX = 
else
ASSERT_VMLINUX = \
@echo "VMLINUX is not defined!" ; exit 1
endif

ifdef RAMDISK
ASSERT_RAMDISK = 
else
ASSERT_RAMDISK = \
@echo "RAMDISK is not defined!" ; exit 1
endif

%.its: %.its.in
	@$(ASSERT_ARCH)
	@$(ASSERT_ADDRESS)
	@$(ASSERT_DESCRIPTION)
	@sed -e 's/ARCH/$(ARCH)/' -e 's/LOAD/$(ADDRESS)/' \
		-e 's/ENTRY/$(ADDRESS)/' \
		-e 's/DESCRIPTION/"$(DESCRIPTION)"/' $< >$@

all: clean linux.fit multi.fit ramdisk.fit complete.fit

linux.fit: linux.its vmlinux.bin.gz
	mkimage -f linux.its $@

ramdisk.fit: ramdisk.its ramdisk.gz
	@$(ASSERT_RAMDISK)
	mkimage -f ramdisk.its $@

multi.fit: multi.its vmlinux.bin.gz
	@$(ASSERT_DTB)
	cp $(DTB) linux.dtb
	mkimage -f multi.its $@
	rm -f linux.dtb

complete.fit: complete.its vmlinux.bin.gz ramdisk.gz
	@$(ASSERT_DTB)
	cp $(DTB) linux.dtb
	mkimage -f complete.its $@
	rm -f linux.dtb

vmlinux.bin.gz: vmlinux.bin
	gzip -f -c -9 $< >$@

vmlinux.bin: $(VMLINUX)
	@$(ASSERT_VMLINUX)
	${CROSS_COMPILE}objcopy -O binary -R .note -R .comment -S $< $@

ramdisk.gz: $(RAMDISK)
	@$(ASSERT_RAMDISK)
	gzip -f -c -9 $< >$@

clean:
	@rm -f vmlinux.bin vmlinux.bin.gz ramdisk.gz linux.dtb *.its *.img *.fit
