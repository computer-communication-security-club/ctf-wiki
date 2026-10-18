FROM ubuntu:24.04

RUN apt-get update \
 && apt-get install -y --no-install-recommends socat \
 && rm -rf /var/lib/apt/lists/* \
 && useradd -m ctf

WORKDIR /home/ctf
# binary нь зориулалтын libc-ийг (rpath $ORIGIN) ашигладаг тул libc.so.6, loader-ийг хамт хуулна
COPY --chmod=755 heap ld-linux-x86-64.so.2 libc.so.6 ./
COPY --chmod=444 flag.txt .

USER ctf
EXPOSE 1337
CMD ["socat", "TCP-LISTEN:1337,reuseaddr,fork", "EXEC:./heap,stderr"]
