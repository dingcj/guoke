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

if [ $# != 6 ] ; then
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

cd sample
make clean
echo $make_cmd
$make_cmd
cd ../

rm -rf ./rootfs_package
tar zxvf ./rootfs_package.tar.gz

packageAlgDir=./rootfs_package/alg
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
cp ./sample/mime.types ./rootfs_package/etc
cp ./sample/rcS ./rootfs_package/etc/init.d

arm-gcc6.3-linux-uclibceabi-strip $packageAlgDir/boa
arm-gcc6.3-linux-uclibceabi-strip $packageAlgDir/sample_venc
arm-gcc6.3-linux-uclibceabi-strip $packageAlgDir/ctrcgi
arm-gcc6.3-linux-uclibceabi-strip $packageAlgDir/search

current=`date "+%Y%m%d%H%M%S"`

chmod +x ./mkfs.jffs2
mkfs_cmd="./mkfs.jffs2 -d ./rootfs_package -l -e 0x10000 -o rootfs-$current.jffs2"

if [[ ${6} = "eqm" ]]
then
    mkfs_cmd="./mkfs.jffs2 -d ./rootfs_package -l -e 0x10000 -o rootfs-eqm-$current.jffs2"
fi
echo $mkfs_cmd
$mkfs_cmd

mv ./rootfs_package ./out/rootfs_package_$current
mv rootfs-*.jffs2 ./out
