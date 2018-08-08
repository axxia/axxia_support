#!/bin/sh

#Script to configure drivers for use inside SNR simulation
#Run this after booting simulation to Linux and mounting /host directory

export RDK_INSTALL=/host/workspace_old/jlogan/snr/del14/rdk
export LD_LIBRARY_PATH=$RDK_INSTALL/rdk_user/user_modules/cpk-ae-lib:$RDK_INSTALL/rdk_user/user_modules/ies-api/lib:$RDK_INSTALL/rdk_user/user_modules/qat/lib:$RDK_INSTALL/rdk_user/dpdk-17.08/x86_64-native-linuxapp-gcc/lib:$LD_LIBRARY_PATH

modprobe uio

pushd $RDK_INSTALL/rdk_klm/cpk/
insmod ice_sw.ko
popd

pushd $RDK_INSTALL/rdk_klm/cpk-ae
insmod ice_sw_ae.ko
popd

pushd $RDK_INSTALL/rdk_klm/ies
insmod ies.ko
popd

pushd $RDK_INSTALL/rdk_klm/hqm
insmod hqm.ko
popd


cp $RDK_INSTALL/rdk_klm/qat/qat/fw/qat_c4xxx*.bin /lib/firmware
cp $RDK_INSTALL/rdk_user/user_modules/qat/adf_ctl/conf_files/c4xxx_dev0.conf.haps80 /etc/c4xxx_dev0.conf

modprobe authenc
modprobe dh_generic


pushd $RDK_INSTALL/rdk_klm/qat/qat/drivers/crypto/qat/qat_common
insmod intel_qat.ko
popd


pushd $RDK_INSTALL/rdk_klm/qat/qat/drivers/crypto/qat/qat_c4xxx
insmod qat_c4xxx.ko
popd

pushd $RDK_INSTALL/rdk_user/user_modules/qat/bin
./adf_ctl down
./adf_ctl up
popd

pushd $RDK_INSTALL/rdk_klm/qat/usdm
insmod usdm_drv.ko
popd


pushd $RDK_INSTALL/rdk_klm/qat/inline
insmod ipsec_inline.ko type=1
popd





mkdir -p /mnt/hugepages
mount -t hugetlbfs nodev /mnt/hugepages
echo 256 > /proc/sys/vm/nr_hugepages
