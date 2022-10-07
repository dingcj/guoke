#!/bin/bash

echo "Build sdk!"

if [ $# != 1 ] ; then
    echo "Please input the chip type!"
    exit 1
fi

chip_type=""

if [[ ${1} = "v300" ]]
then
    echo "select chip is gk7205v300"
    chip_type="gk7205v300"
fi

if [[ ${1} = "v200" ]]
then
    echo "select chip is gk7205v200"
    chip_type="gk7205v200"
fi

if [[ ${chip_type} = "" ]]
then
    echo "Please input correct chip type, v200 or v300."
    exit -1
fi

cfg_file="${chip_type}_def_cfg.mk"
echo $cfg_file

rm -rf ./GKIPCLinuxV100R001C00SPC030

cat GKIPCLinuxV100R001C00SPC030.tar.bz2* | tar -jvx

cd GKIPCLinuxV100R001C00SPC030

source build/env.sh
cp configs/$chip_type/$cfg_file ./cfg.mk

make gmp -j8

