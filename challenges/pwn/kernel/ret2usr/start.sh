#!/bin/sh
# Docker сервер дээр холболт бүрт шинэ QEMU VM ачаалж, эрхгүй (uid 1000) shell өгнө.
# Хугацааны хязгааргүй — хэрэглэгч shell дотор бүх зүйлийг хийнэ.
cd "$(dirname "$0")" || exit 1
exec qemu-system-x86_64 \
  -kernel bzImage -initrd rootfs.cpio.gz \
  -append "console=ttyS0 nokaslr nopti quiet oops=panic panic=-1" \
  -cpu qemu64 -m 256M -smp 1 -nographic -no-reboot -monitor none
