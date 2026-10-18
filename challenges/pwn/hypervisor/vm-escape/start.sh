#!/bin/sh
# Сервер дээр холболт бүрт шинэ guest VM (root shell) өгнө. Escape хийвэл /flag (host) уншина.
cd "$(dirname "$0")" || exit 1
exec ./qemu-system-x86_64 -L pc-bios -kernel bzImage -initrd rootfs.cpio.gz \
  -append "console=ttyS0 quiet" -device edu \
  -nographic -no-reboot -monitor none -m 256M
