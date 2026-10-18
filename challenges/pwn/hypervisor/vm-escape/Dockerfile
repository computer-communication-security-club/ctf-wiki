# --- build stage: эмзэг QEMU-г угсарна ---
FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential meson ninja-build pkg-config python3 python3-venv python3-pip \
    libglib2.0-dev libpixman-1-dev libfdt-dev flex bison git curl ca-certificates xz-utils \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /b
COPY edu.c .
RUN curl -fsSL https://download.qemu.org/qemu-9.1.2.tar.xz | tar xJ \
 && cp edu.c qemu-9.1.2/hw/misc/edu.c \
 && cd qemu-9.1.2 \
 && ./configure --target-list=x86_64-softmmu --disable-gtk --disable-sdl --disable-vnc \
      --disable-docs --disable-tools --disable-werror --disable-slirp --disable-tpm \
 && ninja -C build -j"$(nproc)" qemu-system-x86_64

# --- runtime stage ---
FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    socat libglib2.0-0 libpixman-1-0 \
 && rm -rf /var/lib/apt/lists/* && useradd -m ctf
WORKDIR /chal
COPY --from=build /b/qemu-9.1.2/build/qemu-system-x86_64 ./qemu-system-x86_64
COPY --from=build /b/qemu-9.1.2/pc-bios ./pc-bios
COPY --chmod=755 start.sh .
COPY --chmod=644 bzImage rootfs.cpio.gz ./
COPY flag.txt /flag
RUN chmod 644 /flag
USER ctf
EXPOSE 1337
CMD ["socat", "TCP-LISTEN:1337,reuseaddr,fork", "EXEC:./start.sh,pty,stderr,setsid,sane"]
