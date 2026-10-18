#!/bin/sh
# Docker сервер дээр холболт бүрт ажиллана. Хэрэглэгчээс exploit-ийг base64-ээр
# авч rootfs-д /exploit болгон нэмээд QEMU ачаална.
cd "$(dirname "$0")" || exit 1
echo "[*] Static exploit-ээ base64 болгон нэг мөрөөр илгээнэ үү (gcc -static; base64 -w0):"
read b64 || exit 1
tmp=$(mktemp -d) || exit 1; chmod 755 "$tmp"
gzip -dc rootfs.cpio.gz | ( cd "$tmp" && cpio -idm --quiet )
printf '%s' "$b64" | base64 -d > "$tmp/exploit" 2>/dev/null
chmod 755 "$tmp/exploit"
( cd "$tmp" && find . | cpio -o -H newc --quiet | gzip ) > "$tmp/rootfs.cpio.gz"
echo "[*] Ачаалж байна..."
timeout 120 qemu-system-x86_64 \
  -kernel bzImage -initrd "$tmp/rootfs.cpio.gz" \
  -append "console=ttyS0 nokaslr nopti quiet oops=panic panic=-1" \
  -cpu qemu64 -m 256M -smp 1 -nographic -no-reboot -monitor none
rm -rf "$tmp"
