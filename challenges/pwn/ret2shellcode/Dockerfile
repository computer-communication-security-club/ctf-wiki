FROM ubuntu:24.04

RUN apt-get update \
 && apt-get install -y --no-install-recommends socat \
 && rm -rf /var/lib/apt/lists/* \
 && useradd -m ctf

WORKDIR /home/ctf
COPY --chmod=755 ret2shellcode .
COPY --chmod=444 flag.txt .

USER ctf
EXPOSE 1337
CMD ["socat", "TCP-LISTEN:1337,reuseaddr,fork", "EXEC:./ret2shellcode,stderr"]
