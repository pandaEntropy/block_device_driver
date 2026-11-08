#!/bin/sh

set -euo pipefail

WORKDIR="$HOME"
BUSYBOX_VER="1.36.1"


# Set up a clean initramfs image on every launch
cd "$WORKDIR"
rm -rf initramfs
mkdir -p initramfs/{bin,sbin,etc,proc,sys,dev}
cp -a "busybox-${BUSYBOX_VER}/_install/." initramfs/

cat > initramfs/init <<'EOF'
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys

echo
echo "======================================================"
echo "===   Booted into QEMU   ==="
echo "======================================================"
uname -a
echo
exec /bin/sh
EOF
chmod +x initramfs/init

( cd initramfs && find . | cpio -o -H newc | gzip > ../initramfs.cpio.gz )
echo "initramfs built: $(ls -lh "$WORKDIR/initramfs.cpio.gz")"

# Boot into QEMU
sleep 2
qemu-system-x86_64 \
    -kernel "$WORKDIR/linux/arch/x86/boot/bzImage" \
    -initrd "$WORKDIR/initramfs.cpio.gz" \
    -append "console=ttyS0" \
    -nographic -m 512M
