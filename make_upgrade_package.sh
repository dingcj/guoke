#!/bin/bash

if [ $# != 2 ] ; then
    echo "Make upgrade packe script param err!"
    exit 1
fi

rm -rf ./upgrade
mkdir upgrade

sed -i "s/^const char \*g_SoftWareVersion =.*$/const char \*g_SoftWareVersion = \"$1\";/g" HttpInterface/shareHeader.h
sed -i "s/^const char \*g_HardWareVersion =.*$/const char \*g_HardWareVersion = \"$2\";/g" HttpInterface/shareHeader.h

cd GKIPCLinuxV100R001C00SPC030
source build/env.sh
cd ..

cd sample/
make clean
make

cd ../

packageAlgDir=./upgrade

cp ./sample/venc/run.sh $packageAlgDir
cp ./search/run_search.sh $packageAlgDir
cp ./boa-0.94.13/src/boa $packageAlgDir
cp ./ctrlCgi/ctrcgi $packageAlgDir
cp ./sample/venc/sample_venc $packageAlgDir
cp ./search/search $packageAlgDir
cp ./sample/venc/vehicle0415.bin $packageAlgDir
cp ./sample/venc/vehicle0415.param $packageAlgDir
cp ./update.sh $packageAlgDir

arm-gcc6.3-linux-uclibceabi-strip $packageAlgDir/boa
arm-gcc6.3-linux-uclibceabi-strip $packageAlgDir/sample_venc
arm-gcc6.3-linux-uclibceabi-strip $packageAlgDir/search
arm-gcc6.3-linux-uclibceabi-strip $packageAlgDir/ctrcgi

zip -q -r upgrade_$1.zip ./upgrade

rm -rf ./out/upgrade
rm -rf ./out/upgrade_$1.zip

mv ./upgrade ./out
mv ./upgrade_$1.zip ./out
