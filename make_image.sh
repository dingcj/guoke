#!/bin/bash

function check_ip() {
    local IP=$1
    VALID_CHECK=$(echo $IP|awk -F. '$1<=255&&$2<=255&&$3<=255&&$4<=255{print "yes"}')
    if echo $IP|grep -E "^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$" >/dev/null; then
        if [ $VALID_CHECK == "yes" ]; then
         echo "IP $IP  available!"
            return 0
        else
            echo "IP $IP not available!"
            return 1
        fi
    else
        echo "IP format error!"
        return 1
    fi
}

if [ $# != 7 ] ; then
    echo "Make image packe script param err!"
    exit 1
fi

check_ip $3
if [ $? -eq 1 ]; then
    echo "IP addr is invalid!"
    exit -1
fi

check_ip $4
if [ $? -eq 1 ]; then
    echo "Netmask is invalid!"
    exit -1
fi

check_ip $5
if [ $? -eq 1 ]; then
    echo "Gateway is invalid!"
    exit -1
fi

commit_id=`git rev-parse HEAD`
sed -i "s/^const char \*g_GitCommitId =.*$/const char \*g_GitCommitId = \"$commit_id\";/g" HttpInterface/shareHeader.h
sed -i "s/^const char \*g_SoftWareVersion =.*$/const char \*g_SoftWareVersion = \"$1\";/g" HttpInterface/shareHeader.h
sed -i "s/^const char \*g_HardWareVersion =.*$/const char \*g_HardWareVersion = \"$2\";/g" HttpInterface/shareHeader.h

cd GKIPCLinuxV100R001C00SPC030
source build/env.sh
cd ..

make_cmd=""
if [[ ${6} = "eqm" ]]
then
    make_cmd="make eqm"
fi

if [[ ${6} = "release" ]]
then
    make_cmd="make"
fi

if [[ ${make_cmd} = "" ]]
then
    echo "Please input correct type, eqm or release!"
    exit -1
fi

chip_type=""
if [[ ${7} = "v300" ]]
then
    chip_type="v300"
fi

if [[ ${7} = "v200" ]]
then
    chip_type="v200"
fi

if [[ ${chip_type} = "" ]]
then
    echo "Please input correct chip type, v300 or v200!"
    exit -1
fi
echo $chip_type
rootfs_dirname=rootfs_package_$chip_type
echo $rootfs_dirname

cd sample
make clean
echo $make_cmd
$make_cmd
cd ../

rm -rf ./${rootfs_dirname}
tar zxvf ./${rootfs_dirname}.tar.gz

packageAlgDir=./${rootfs_dirname}/alg
## set ip addr
echo $3 > $packageAlgDir/ip_cfg.txt
echo $4 > $packageAlgDir/netmask_cfg.txt
echo $5 > $packageAlgDir/gateway_cfg.txt

cp ./sample/venc/run.sh $packageAlgDir
cp ./search/run_search.sh $packageAlgDir
cp ./boa-0.94.13/src/boa $packageAlgDir
cp ./ctrlCgi/ctrcgi $packageAlgDir
cp ./sample/venc/sample_venc $packageAlgDir
cp ./search/search $packageAlgDir
cp ./sample/venc/vehicle0415.bin $packageAlgDir
cp ./sample/venc/vehicle0415.param $packageAlgDir
cp ./sample/mime.types ./${rootfs_dirname}/etc
cp ./sample/rcS ./${rootfs_dirname}/etc/init.d

arm-gcc6.3-linux-uclibceabi-strip $packageAlgDir/boa
arm-gcc6.3-linux-uclibceabi-strip $packageAlgDir/sample_venc
arm-gcc6.3-linux-uclibceabi-strip $packageAlgDir/ctrcgi
arm-gcc6.3-linux-uclibceabi-strip $packageAlgDir/search

current=`date "+%Y%m%d%H%M%S"`

chmod +x ./mkfs.jffs2
mkfs_cmd="./mkfs.jffs2 -d ./${rootfs_dirname} -l -e 0x10000 -o rootfs-${chip_type}-$current.jffs2"

if [[ ${6} = "eqm" ]]
then
    mkfs_cmd="./mkfs.jffs2 -d ./${rootfs_dirname} -l -e 0x10000 -o rootfs-eqm-${chip_type}-$current.jffs2"
fi
echo $mkfs_cmd
$mkfs_cmd

mv ./${rootfs_dirname} ./out/${rootfs_dirname}_$current
mv rootfs-*.jffs2 ./out
