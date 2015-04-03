#!/bin/bash
if [ -z $1 -o -z $2 ]
then
   echo "You should specify the device and kernel !"
   echo "Usage: sudo ./setup.sh device kernel" 
   echo "Example: sudo ./setup.sh /dev/sdb HD-Elastos"
   exit 0
fi
#convert the boot.txt to boot.scr
mkimage -A arm -O linux -T script -C none -n "U-Boot commands" -d boot.txt boot.scr

#create a file named disk.img
dd if=/dev/zero of=disk.img bs=1M count=60

#repartition the file disk.img
sfdisk -uS --force disk.img << EOF
2048,,b,*
EOF

#copy the spl file into disk.img
dd if=u-boot-sunxi-with-spl.bin of=disk.img bs=1024 seek=8

#create a file named fs.img
dd if=/dev/zero of=fs.img bs=1M count=60

#create an fat32 filesystem on fs.img
mkfs.vfat fs.img -F 32 -n boot

#create a temp directory
mkdir tmp

#copy the kernel and boot.scr into filesystem
mount -t vfat fs.img tmp/
cp boot.scr $2 tmp/
sync
umount tmp/
#copy the filesystem into the disk.img
dd if=fs.img of=disk.img bs=1M count=60 seek=1

#copy the image into the device
echo "wirting the img to the device, Please wait... "
dd if=disk.img of=$1 bs=1M
rm -r fs.img disk.img tmp/ boot.scr 
