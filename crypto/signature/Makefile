#
# Makefile for the signature checker
#

# USE PREBUILT pubkey2c:

#HOSTLOADLIBES_pubkey2c = -lcrypto
#hostprogs-y := pubkey2c
#$(obj)/pubkey2c: $(src)/pubkey2c.c

quiet_cmd_pubkey2c = PUBKEY  $@
      cmd_pubkey2c = $(filter-out FORCE,$^) > $@

TT_KERNEL_KEY = $(srctree)/keys/tt_kernel_prod_pubkey.dsa
TT_LOOPFS_KEY = $(srctree)/keys/tt_loopfs_prod_pubkey.dsa

ifneq ($(RELEASE),)

TT_ROOTFS_KEY = $(objtree)/keys/tt_rootfs_prod_pubkey.dsa

quiet_cmd_gendsa = GENDSA  $@
      cmd_gendsa = $(srctree)/scripts/gen_key_pair rootfs production $(srctree)/keys $(objtree)/keys

$(TT_ROOTFS_KEY): FORCE
	$(Q)mkdir -p $(dir $@)
	$(call if_changed,gendsa)
	
else

TT_ROOTFS_KEY = $(srctree)/keys/tt_rootfs_dev_pubkey.dsa
TT_LOOPFS_KEY := $(TT_LOOPFS_KEY) $(srctree)/keys/tt_loopfs_dev_pubkey.dsa

endif

TT_FS_KEYS = $(TT_ROOTFS_KEY) $(TT_KERNEL_KEY) $(TT_LOOPFS_KEY)

# USE PREBUILT pubkey2c:
# $(src)/dsa_pubkey.h: $(obj)/pubkey2c $(TT_FS_KEYS) FORCE

$(src)/dsa_pubkey.h: $(srctree)/../../../tools/bin/pubkey2c $(TT_FS_KEYS) FORCE
	$(call cmd,pubkey2c)

obj-y := dsasig.o

$(obj)/dsasig.o: $(src)/dsa_pubkey.h

clean-files := pubkey2c dsa_pubkey.h

PHONY += FORCE

