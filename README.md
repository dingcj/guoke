# guoke

setenv bootargs 'mem=44M console=ttyAMA0,115200 root=/dev/mtdblock2 rootfstype=jffs2 rw mtdparts=sfc:768K(boot),3584K(kernel),12032K(rootfs)'
setenv bootcmd 'sf probe 0;sf read 0x41000000 0xC0000 0x380000;bootm 0x41000000'

setenv ipaddr 10.10.10.42;setenv serverip 10.10.10.10;mw.b 0x41000000 0xFF 0x380000;tftp 0x41000000 uImage_gk7205v200;sf probe 0;sf erase 0xC0000 0x380000;sf write 0x41000000 0xC0000 0x380000

setenv ipaddr 10.10.10.42;setenv serverip 10.10.10.10;mw.b 0x41000000 0xFF 0xBC0000;tftp 0x41000000 rootfs-20220430095258.jffs2;sf probe 0;sf erase 0x440000 0xBC0000;sf write 0x41000000 0x440000 0xBC0000
