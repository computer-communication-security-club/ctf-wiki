FROM ubuntu:24.04

RUN apt-get update \
 && apt-get install -y --no-install-recommends qemu-system-x86 socat cpio gzip coreutils \
 && rm -rf /var/lib/apt/lists/* \
 && useradd -m ctf

WORKDIR /chal
COPY --chmod=755 start.sh .
COPY --chmod=644 bzImage rootfs.cpio.gz ./

USER ctf
EXPOSE 1337
# холболт бүрт start.sh (QEMU VM) ажиллуулна
CMD ["socat", "TCP-LISTEN:1337,reuseaddr,fork", "EXEC:./start.sh,pty,stderr,setsid,sane"]
