#!/bin/sh
# ./run.sh [exploit]   — эмзэг QEMU (edu device) + guest. flag нь HOST дээр (/flag).
cd "$(dirname "$0")" || exit 1
[ -x ./qemu-system-x86_64 ] || { echo "qemu-system-x86_64 алга — эхлээд ./build.sh ажиллуулна."; exit 1; }
[ -f /flag ] || echo "flag{local-test}" > /flag 2>/dev/null || true
initrd=rootfs.cpio.gz
if [ -n "$1" ] && [ -f "$1" ]; then                 # exploit-ийг guest-д /exploit болгон auto-run
  tmp=$(mktemp -d); chmod 755 "$tmp"
  gzip -dc rootfs.cpio.gz | ( cd "$tmp" && cpio -idm --quiet )
  cp "$1" "$tmp/exploit"; chmod 755 "$tmp/exploit"
  ( cd "$tmp" && find . | cpio -o -H newc --quiet | gzip ) > "$tmp/exp.cpio.gz"
  initrd="$tmp/exp.cpio.gz"
fi
exec ./qemu-system-x86_64 -L pc-bios -kernel bzImage -initrd "$initrd" \
  -append "console=ttyS0 quiet" -device edu \
  -nographic -no-reboot -monitor none -m 256M
