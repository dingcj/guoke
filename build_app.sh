#!/bin/bash

root_dir=$(dirname "$0")

cd $root_dir
cd GKIPCLinuxV100R001C00SPC030
source build/env.sh
cd ..

cd sample/
make clean
make

