#!/bin/sh
# ret2usr kernel challenge — QEMU-д ачаална.  ./run.sh [exploit]
#   exploit өгвөл түүнийг rootfs-д /exploit болгон нэмж ачаална.
# Intro хувилбар: KASLR/SMEP/SMAP/KPTI унтраалттай.
cd "$(dirname "$0")" || exit 1
INITRD=rootfs.cpio.gz
if [ -n "$1" ]; then
  tmp=$(mktemp -d); chmod 755 "$tmp"
  gzip -dc rootfs.cpio.gz | ( cd "$tmp" && cpio -idm --quiet )
  cp "$1" "$tmp/exploit" && chmod 755 "$tmp/exploit"
  INITRD=$(mktemp --suffix=.cpio.gz)
  ( cd "$tmp" && find . | cpio -o -H newc --quiet | gzip ) > "$INITRD"
  rm -rf "$tmp"
fi
qemu-system-x86_64 \
  -kernel bzImage -initrd "$INITRD" \
  -append "console=ttyS0 nokaslr nopti quiet oops=panic panic=-1" \
  -cpu qemu64 -m 256M -smp 1 \
  -nographic -no-reboot -monitor none
