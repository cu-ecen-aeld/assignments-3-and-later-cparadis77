#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.
#
#
#
# Modification from CParadis:
#
#   Usage: manual-linux.sh [outdir]
#     outdir    is the absolute path of the location on the filesystem where the output files should be placed
#
set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

# Debug only
#echo "Debug cparadis: $0, $1"
#echo "Debug cparadis: $@"        # List all the arguments supplied to the Bash script
#echo "Debug cparadis: FINDER_APP_DIR is: $FINDER_APP_DIR"

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi

if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # Debug only
    #pwd

    # TODO: Add your kernel build steps here
    #echo "Debug CParadis: building kernel...build clean..."
    make ARCH=arm64 CROSS_COMPILE=$CROSS_COMPILE mrproper

    #echo "Debug CParadis: building kernel...build defconfig..."
    make ARCH=arm64 CROSS_COMPILE=$CROSS_COMPILE defconfig

    #echo "Debug CParadis: building kernel...build vmlinux..."
    make -j4 ARCH=arm64 CROSS_COMPILE=$CROSS_COMPILE all

    #echo "Debug CParadis: building kernel...build modules and devicetree..."
    make ARCH=arm64 CROSS_COMPILE=$CROSS_COMPILE modules
    make ARCH=arm64 CROSS_COMPILE=$CROSS_COMPILE dtbs

    #echo "Debug CParadis: building kernel...build completed!!!"
fi

if [ ! -e ${OUTDIR}/Image ]; then
    echo "Adding the Image in outdir"
    cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image
fi

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
#echo "Debug CParadis: Create necessary base directories..."
mkdir -p ${OUTDIR}/rootfs

# Debug only
#pwd

cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var

mkdir -p usr/bin usr/lib usr/sbin

mkdir -p var/log

mkdir -p home/conf

# Debug only
#echo "Debug CParadis: Create necessary base directories...completed!!!"
#ls -al



cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}

    # TODO:  Configure busybox
    #echo "Debug CParadis: building busybox...build clean..."
    make distclean
    make defconfig
    #echo "Debug CParadis: building busybox...build configured..."

else
    cd busybox
fi

# TODO: Make and install busybox
#echo "Debug CParadis: building busybox...build busybox..."
make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE install
#echo "Debug CParadis: building busybox...build completed!!!"

# Debug only
#pwd
#ls -al

#echo "Debug CParadis: Handle dependencies for busybox...start..."
cd ${OUTDIR}/rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"


# Debug only
#pwd
#ls -al

# TODO: Add library dependencies to rootfs
#cp ~/arm-cross-compiler/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 ~/test/build_kernel/rootfs/lib/ld-linux-aarch64.so.1
#cp ~/arm-cross-compiler/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libm.so.6 ~/test/build_kernel/rootfs/lib64/libm.so.6
#cp ~/arm-cross-compiler/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 ~/test/build_kernel/rootfs/lib64/libresolv.so.2
#cp ~/arm-cross-compiler/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libc.so.6 ~/test/build_kernel/rootfs/lib64/libc.so.6

cp $FINDER_APP_DIR/../assignments/assignment3/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/ld-linux-aarch64.so.1
cp $FINDER_APP_DIR/../assignments/assignment3/libm.so.6 ${OUTDIR}/rootfs/lib64/libm.so.6
cp $FINDER_APP_DIR/../assignments/assignment3/libresolv.so.2 ${OUTDIR}/rootfs/lib64/libresolv.so.2
cp $FINDER_APP_DIR/../assignments/assignment3/libc.so.6 ${OUTDIR}/rootfs/lib64/libc.so.6

#echo "Debug CParadis: Handle dependencies for busybox...completed!!!"

# Debug only
#pwd
#ls -al

# TODO: Make device nodes
#echo "Debug CParadis: Make device nodes...start..."

sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

#echo "Debug CParadis: Make device nodes...completed!!!"

# Debug only
#pwd
#ls -al

# TODO: Clean and build the writer utility
#echo "Debug CParadis: Clean and build the writer utility...start..."

cd $FINDER_APP_DIR

# Debug only
#pwd
#ls -al

make CROSS_COMPILE=$CROSS_COMPILE clean
make CROSS_COMPILE=$CROSS_COMPILE all

#echo "Debug CParadis: Clean and build the writer utility...completed!!!"

# Debug only
#pwd
#ls -al

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs

cd ${OUTDIR}/rootfs

# Debug only
#pwd
#ls -al

cp $FINDER_APP_DIR/writer ${OUTDIR}/rootfs/home/writer

cp $FINDER_APP_DIR/finder.sh ${OUTDIR}/rootfs/home/finder.sh
cp $FINDER_APP_DIR/conf/username.txt ${OUTDIR}/rootfs/home/conf/username.txt
cp $FINDER_APP_DIR/finder-test.sh ${OUTDIR}/rootfs/home/finder-test.sh
cp $FINDER_APP_DIR/autorun-qemu.sh ${OUTDIR}/rootfs/home/autorun-qemu.sh

# Debug only
#pwd
#ls -al

# TODO: Chown the root directory
#echo "Debug CParadis: Chown the root directory...start..."

find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio

#echo "Debug CParadis: Chown the root directory...completed!!!"

# Debug only
#pwd
#ls -al

# TODO: Create initramfs.cpio.gz
#echo "Debug CParadis: Create initramfs.cpio.gz...start..."

gzip -f ${OUTDIR}/initramfs.cpio

#echo "Debug CParadis: Create initramfs.cpio.gz...completed!!"
