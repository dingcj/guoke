#!/bin/bash

echo "Build sdk!"

rm -rf ./GKIPCLinuxV100R001C00SPC030

cat GKIPCLinuxV100R001C00SPC030.tar.bz2* | tar -jvx

cd GKIPCLinuxV100R001C00SPC030

source build/env.sh

make gmp -j8

