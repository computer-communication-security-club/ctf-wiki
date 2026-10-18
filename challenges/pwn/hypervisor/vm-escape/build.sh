#!/bin/sh
# Эмзэг edu төхөөрөмж бүхий QEMU-г эх кодоос угсарна -> ./qemu-system-x86_64
# Deps (Ubuntu): build-essential meson ninja-build pkg-config python3 libglib2.0-dev libpixman-1-dev flex bison curl xz-utils
set -e
ver=9.1.2
here=$(cd "$(dirname "$0")" && pwd)
work=$(mktemp -d)
echo "[*] QEMU $ver татаж байна..."
curl -fsSL "https://download.qemu.org/qemu-$ver.tar.xz" | tar xJ -C "$work"
cp "$here/edu.c" "$work/qemu-$ver/hw/misc/edu.c"
cd "$work/qemu-$ver"
./configure --target-list=x86_64-softmmu --disable-gtk --disable-sdl --disable-vnc \
  --disable-docs --disable-tools --disable-werror --disable-slirp --disable-tpm
ninja -C build -j"$(nproc)" qemu-system-x86_64
cp build/qemu-system-x86_64 "$here/qemu-system-x86_64"
rm -rf "$here/pc-bios" && cp -rL pc-bios "$here/pc-bios"   # firmware blobs (bios, vgabios...)
rm -rf "$work"
echo "[+] угсарлаа: $here/qemu-system-x86_64 (+ pc-bios/)"
