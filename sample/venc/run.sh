#!/bin/sh

for i in `seq 10`                  
do                                 
    echo Good Morning ,this is  $i  shell program.
    cd /alg
    rm -rf /tmp/upgrade.zip
    rm -rf /tmp/upgrade
    workdir=`pwd`
    chmod +x ./sample_venc
    ./sample_venc
    if [ $? = 123 ]; then
        cd /tmp
        rm -rf upgrade
        unzip ./upgrade.zip
        cd /tmp/upgrade
        chmod +x ./update.sh
        ./update.sh
        cd $workdir
    fi
done

#reboot
