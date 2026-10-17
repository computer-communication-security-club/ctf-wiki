---
title: Хамгаалалтууд
description: NX, stack canary, ASLR, PIE, RELRO — орчин үеийн binary-ийн хамгаалалтууд юу хийдэг, checksec-ийг хэрхэн унших, тэдгээрийг юугаар тойрдог.
---

Орчин үеийн compiler болон үйлдлийн систем stack overflow гэх мэт алдааг ашиглахад хэцүү болгох
хэд хэдэн **хамгаалалт** (mitigation) нэмдэг. Хамгаалалт алдааг өөрөө засдаггүй — зөвхөн
ашиглах аргыг хязгаарладаг. Тиймээс pwn бодлого бүрийн эхний алхам бол **аль хамгаалалт идэвхтэйг** шалгах.

| Хамгаалалт | Юуг хориглодог | Хэн нэмдэг |
|------------|----------------|------------|
| [NX](#nx) | Stack дээрх өгөгдлийг код болгон ажиллуулах | Compiler + CPU |
| [Stack canary](#stack-canary) | Return address хүртэл дарж бичих | Compiler |
| [ASLR](#aslr) | Stack, heap, libc-ийн хаягийг таах | Үйлдлийн систем |
| [PIE](#pie) | Программын өөрийн кодын хаягийг таах | Compiler |
| [RELRO](#relro) | GOT хүснэгтийг дарж бичих | Linker |

Энэ хуудасны жишээнүүд доорх программыг өөр өөр флагаар compile хийж үзүүлнэ:

```c title="vuln.c"
#include <stdio.h>

char *gets(char *s);

int main(void) {
    char buf[16];
    printf("main: %p  buf: %p\n", main, buf);
    gets(buf);
    return 0;
}
```

## checksec унших

`checksec` нь binary-д аль хамгаалалт идэвхтэйг харуулна. Ямар ч флаггүй compile хийвэл
(Ubuntu-ийн gcc) **бүх** хамгаалалт асаалттай гарна:

```console
$ gcc vuln.c -o vuln
$ checksec --file=vuln
    Arch:       amd64-64-little
    RELRO:      Full RELRO
    Stack:      Canary found
    NX:         NX enabled
    PIE:        PIE enabled
    SHSTK:      Enabled
    IBT:        Enabled
    Stripped:   No
```

Харин [ret2text](../../ret2text/)-ийн binary-д canary болон PIE-г зориуд унтраасан:

```console
$ checksec --file=ret2text
    RELRO:      Partial RELRO
    Stack:      No canary found
    NX:         NX enabled
    PIE:        No PIE (0x400000)
```

Мөр бүрийг доор тайлбарлана.

## NX

**NX** (No-eXecute, Windows дээр **DEP**) — санах ойн хуудас бүр **унших (r)**, **бичих (w)**,
**ажиллуулах (x)** гэсэн эрхтэй. NX идэвхтэй үед өгөгдөл хадгалдаг хэсэг (stack, heap) бичигдэх боловч
**ажиллагдахгүй**. Нэг хуудас зэрэг бичигдэж, ажиллагдах боломжгүй гэдэг санааг **W^X** гэж бас нэрлэдэг.

pwndbg-ийн `vmmap` командаар эрхийг харна. `[stack]`-ийн мөрийг хар:

```ansi
[1;31mpwndbg>[0m vmmap
             Start                End Perm     Size  Offset File
    0x555555554000     0x555555555000 r--p     1000       0 vuln
[31m    0x555555555000     0x555555556000 r-xp     1000    1000 vuln[0m
    0x555555556000     0x555555557000 r--p     1000    2000 vuln
[35m    0x555555557000     0x555555559000 rw-p     2000    2000 vuln[0m
    ...
[33m    0x7ffffffdd000     0x7ffffffff000 rw-p    22000       0 [stack][0m
```

- `r-xp` — программын код (`.text`): уншиж, ажиллуулж болно, бичиж болохгүй.
- `rw-p` — өгөгдөл ба **stack**: уншиж, бичиж болно, **ажиллуулж болохгүй**.

`-z execstack` флагаар NX-ийг унтраавал stack `rwx` болно:

```ansi
[1;31mpwndbg>[0m vmmap
    ...
[4;33m    0x7ffffffdd000     0x7ffffffff000 rwxp    22000       0 [stack][0m
```

**Юу өөрчлөгдөх вэ:** NX байхгүй бол stack overflow-оор өөрийн машин кодоо (**shellcode**) stack дээр
бичээд return address-ийг түүн рүү чиглүүлж болно ([ret2shellcode](../../ret2shellcode/)). NX байвал CPU тэр хаягт очмогц `SIGSEGV` өгнө.

**Тойрох арга:** шинэ код бичихийн оронд программд **аль хэдийн байгаа** кодыг ашиглах —
[ret2text](../../ret2text/), ROP, ret2libc. Иймээс ret2text-д NX "хамаагүй" байсан.

:::note
`-z execstack`-тэй binary-д pwntools-ийн `checksec` заримдаа `NX unknown - GNU_STACK missing`
гэж буруу харуулдаг. Доод мөрөнд нь `Stack: Executable` байвал stack ажиллагдах боломжтой гэсэн үг.
Баталгаатай шалгах бол `readelf -lW vuln | grep GNU_STACK` — `RWE` бол NX унтраалттай, `RW` бол асаалттай.
:::

## Stack canary

**Canary** нь `buf` болон `saved rbp`-ийн хооронд тавьдаг **санамсаргүй тоо**. Функц `ret` хийхээс өмнө
canary өөрчлөгдсөн эсэхийг шалгана. Overflow return address хүртэл хүрэхийн тулд canary-г заавал
дарах тул өөрчлөлт илэрч, программ шууд зогсоно. (Нэр нь уурхайчдын хорт хий илрүүлэхэд авч ордог
кенар шувуунаас гаралтай.)

Disassembly дээр нэмэгдсэн хэсгүүд (тодруулсан мөрүүд):

```ansi {7-8,21-24}
[1;31mpwndbg>[0m disassemble main
Dump of assembler code for function [33mmain[0m:
   [34m0x0000000000001189[0m <+0>:	[32mendbr64[0m
   [34m0x000000000000118d[0m <+4>:	[32mpush   [0m[31mrbp[0m
   [34m0x000000000000118e[0m <+5>:	[32mmov    [0m[31mrbp[0m,[31mrsp[0m
   [34m0x0000000000001191[0m <+8>:	[32msub    [0m[31mrsp[0m,[34m0x20[0m
   [34m0x0000000000001195[0m <+12>:	[32mmov    [0m[31mrax[0m,QWORD PTR [31mfs[0m:0x28
   [34m0x000000000000119e[0m <+21>:	[32mmov    [0mQWORD PTR [[31mrbp[0m[34m-[0m[34m0x8[0m],[31mrax[0m
   [34m0x00000000000011a2[0m <+25>:	[32mxor    [0m[31meax[0m,[31meax[0m
   [34m0x00000000000011a4[0m <+27>:	[32mlea    [0m[31mrax[0m,[[31mrbp[0m[34m-[0m[34m0x20[0m]
   [34m0x00000000000011a8[0m <+31>:	[32mlea    [0m[31mrsi[0m,[[31mrip[0m+[34m0xffffffffffffffda[0m][90m        # [0m[90m0x1189 <main>[0m
   [34m0x00000000000011af[0m <+38>:	[32mlea    [0m[31mrcx[0m,[[31mrip[0m+[34m0xe4e[0m][90m        # [0m[90m0x2004[0m
   [34m0x00000000000011b6[0m <+45>:	[32mmov    [0m[31mrdx[0m,[31mrax[0m
   [34m0x00000000000011b9[0m <+48>:	[32mmov    [0m[31mrdi[0m,[31mrcx[0m
   [34m0x00000000000011bc[0m <+51>:	[32mmov    [0m[31meax[0m,[34m0x0[0m
   [34m0x00000000000011c1[0m <+56>:	[32mcall   [0m[34m0x1080[0m <[33mprintf@plt[0m>
   [34m0x00000000000011c6[0m <+61>:	[32mlea    [0m[31mrax[0m,[[31mrbp[0m[34m-[0m[34m0x20[0m]
   [34m0x00000000000011ca[0m <+65>:	[32mmov    [0m[31mrdi[0m,[31mrax[0m
   [34m0x00000000000011cd[0m <+68>:	[32mcall   [0m[34m0x1090[0m <[33mgets@plt[0m>
   [34m0x00000000000011d2[0m <+73>:	[32mmov    [0m[31meax[0m,[34m0x0[0m
   [34m0x00000000000011d7[0m <+78>:	[32mmov    [0m[31mrdx[0m,QWORD PTR [[31mrbp[0m[34m-[0m[34m0x8[0m]
   [34m0x00000000000011db[0m <+82>:	[32msub    [0m[31mrdx[0m,QWORD PTR [31mfs[0m:0x28
   [34m0x00000000000011e4[0m <+91>:	[32mje     [0m[34m0x11eb[0m <[33mmain[0m+98>
   [34m0x00000000000011e6[0m <+93>:	[32mcall   [0m[34m0x1070[0m <[33m__stack_chk_fail@plt[0m>
   [34m0x00000000000011eb[0m <+98>:	[32mleave[0m
   [34m0x00000000000011ec[0m <+99>:	[32mret[0m
End of assembler dump.
```

- **Эхэнд** (`+12`, `+21`): `fs:0x28`-аас canary-г уншаад `[rbp-0x8]`-д хадгална.
- **Төгсгөлд** (`+78`…`+93`): `[rbp-0x8]`-ийн утгыг `fs:0x28`-тай харьцуулна. Ижил бол `je`-ээр `leave; ret` руу,
  өөр бол `__stack_chk_fail` дуудаж программыг зогсооно.

Stack-ийн бүтэц ингэж өөрчлөгдөнө:

```ansi
             [90m┌──────────────────┐[0m
             [90m│[0m[1;31m  return address  [0m[90m│[0m
[1;31mrbp + 8[0m      [90m├──────────────────┤[0m
             [90m│[0m[33m  saved rbp       [0m[90m│[0m
[33mrbp[0m          [90m├──────────────────┤[0m
             [90m│[0m[1;36m  canary          [0m[90m│[0m  [1;36m← overflow үүнийг заавал дарна[0m
[1;36mrbp - 0x8[0m    [90m├──────────────────┤[0m
             [90m│[0m[90m  (padding)       [0m[90m│[0m
[90mrbp - 0x10[0m   [90m├──────────────────┤[0m
             [90m│[0m[32m  buf[16]         [0m[90m│[0m
[32mrbp - 0x20[0m   [90m└──────────────────┘[0m
```

Overflow хийвэл:

```console
$ python3 -c "print('A' * 40)" | ./vuln
*** stack smashing detected ***: terminated
Aborted (core dumped)
```

Canary байхгүй үед (`-fno-stack-protector`) ижил оролт return address-ийг дарж `Segmentation fault` өгдөг.
Canary нь overflow-ийг **ret хийхээс өмнө** барьж, `abort` хийснээр хяналтыг бидэнд өгөхгүй.

pwndbg-ийн `canary` команд одоогийн утгыг харуулна:

```ansi
[1;31mpwndbg>[0m canary
[35mAT_RANDOM  = 0x7fffffffdc19[0m[35m # points to the canary value on glibc < 2.44[0m
[35mTLS Canary @ 0x7ffff7f9c768[0m[35m # address where canary is stored[0m
[35mCanary     = 0x12b7753d864c1f00[0m
```

Canary-ийн **хамгийн бага байт үргэлж `00`** байдаг (`...1f00`). Little-endian-д энэ байт санах ойд **эхэлж**
байрладаг тул `puts`, `printf("%s")` зэрэг string функц `buf`-аас хэвлэхдээ canary дээр зогсож, түүнийг
задруулдаггүй.

**Тойрох арга:**
- Canary-г **leak хийх** (format string, `\x00` байтыг дарж бичээд `puts`-ээр хэвлэх) → payload-д яг тэр утгыг буцааж тавих.
- `fork()` хийдэг серверт хүүхэд процесс бүр **ижил** canary-тай тул байт байтаар brute force хийж болно.
- Canary-г огт дайрахгүй байх: overflow биш, шууд тодорхой хаяг руу бичих (format string `%n`, heap).

Флаг: `-fstack-protector-strong` (Ubuntu-д анхнаасаа), `-fstack-protector-all`, унтраах нь `-fno-stack-protector`.

## ASLR

**ASLR** (Address Space Layout Randomization) нь **үйлдлийн системийн** хамгаалалт: программ ажиллах бүрт
stack, heap, shared library (libc) зэргийг **санамсаргүй** хаягт байрлуулна.

```console
$ ./vuln
main: 0x5f50f9141189  buf: 0x7ffc0f30fd00
$ ./vuln
main: 0x59351c607189  buf: 0x7fffe0767640
$ ./vuln
main: 0x5c9243cf3189  buf: 0x7ffe13b88d30
```

`buf`-ийн хаяг (stack) ажиллах бүрт өөр. Тиймээс stack дээр shellcode тавьсан ч **хаана байгааг** мэдэхгүй.

ASLR binary-д биш, **системд** тохируулагддаг:

```console
$ cat /proc/sys/kernel/randomize_va_space
2
```

| Утга | Утга учир |
|------|-----------|
| `0` | Унтраалттай |
| `1` | Stack, libc, vdso санамсаргүй |
| `2` | + heap (анхны тохиргоо) |

:::caution[gdb ASLR-ийг унтраадаг]
gdb анхнаасаа ASLR-ийг **унтраадаг** (`set disable-randomization on`). Иймээс gdb дотор хаяг үргэлж
`0x555555555189`, `0x7fffffffd...` гэх мэт ижил гардаг ч жинхэнэ ажиллуулахад өөр байна.
gdb дээр ажилласан exploit серверт ажиллахгүй байвал эхлээд үүнийг сэжиглээрэй.
:::

**Тойрох арга:** хаягийг **leak хийх**. Санамсаргүй болгодог нь зөвхөн **эхлэлийн хаяг** (base) —
доторх зүйлсийн хоорондох зай тогтмол. Нэг хаяг мэдвэл бусдыг нь тооцоолж болно.

## PIE

**PIE** (Position Independent Executable) нь **программын өөрийн кодыг** ASLR-д хамруулна.
Дээрх гаралтыг дахин хар:

```console
$ gcc vuln.c -o vuln -no-pie       # PIE-гүй
$ ./vuln
main: 0x401176  buf: 0x7ffc04bd0de0
$ ./vuln
main: 0x401176  buf: 0x7ffdf5ec0650
```

- **PIE-гүй:** `main` үргэлж `0x401176` — binary `0x400000`-д тогтмол ачаалагдана.
  Иймээс [ret2text](../../ret2text/)-д `win()`-ийн хаягийг (`0x401196`) шууд ашиглаж чадсан.
- **PIE-тэй:** `main` = `0x5f50f9141189`, `0x59351c607189`… — ажиллах бүрт өөр.

PIE-тэй binary-д `checksec`, `objdump` харуулдаг хаяг (`0x1189`) нь жинхэнэ хаяг биш, binary-ийн эхлэлээс
хэдэн байтын зайтайг заах **offset** юм: жинхэнэ хаяг = **base + offset**.

Анхаар: дээрх гурван хаяг бүгд **`...189`**-өөр төгсдөг. Base үргэлж хуудсын (0x1000) хил дээр байдаг тул
хаягийн **сүүлийн 3 hex орон өөрчлөгддөггүй**.

**Тойрох арга:**
- Кодын нэг хаягийг **leak хийх** → base-ийг тооцох:
  ```python
  exe.address = leak - exe.sym.main   # base олдлоо
  exe.sym.win                         # одоо жинхэнэ хаяг
  ```
- **Partial overwrite:** return address-ийн зөвхөн сүүлийн 1–2 байтыг дарах. Сүүлийн 3 hex орон тогтмол тул
  ойролцоох функц руу leak-гүйгээр үсэрч болно (1 байт бол үргэлж, 2 байт бол 16-аас 1 удаа таарна).

Флаг: Ubuntu-д анхнаасаа идэвхтэй, унтраах нь `-no-pie`.

## RELRO

**RELRO** (RELocation Read-Only) нь **GOT** (Global Offset Table) хүснэгтийг хамгаална. GOT-д `puts`, `printf`
зэрэг libc функцүүдийн хаяг хадгалагддаг бөгөөд программ функц дуудах бүрт эндээс хаягаа уншдаг.
GOT-ийг дарж бичиж чадвал (жишээ нь `puts`-ийн хаягийг `system` болговол) дараагийн `puts` дуудлага
бидний сонгосон функц руу очно.

| checksec | GOT | Тайлбар |
|----------|-----|---------|
| `No RELRO` | Бичигдэнэ | Хамгаалалтгүй |
| `Partial RELRO` | Бичигдэнэ | GOT-ийн зарим хэсгийг (`.got`) хамгаална, функцийн хаягууд (`.got.plt`) бичигдсээр |
| `Full RELRO` | **Зөвхөн унших** | Бүх хаягийг программ эхлэхэд шийдээд GOT-ийг бүхэлд нь түгжинэ |

ret2text-ийн binary `Partial RELRO` — GOT overwrite хийх боломжтой. Ubuntu-д анхны тохиргоо
(PIE-тэй үед) `Full RELRO`.

**Тойрох арга:** Full RELRO үед GOT-ийн оронд өөр бичигдэх зорилт хайна (return address, `__free_hook`
зэрэг hook-ууд хуучин glibc-д гэх мэт). Stack overflow-д RELRO ихэвчлэн хамаагүй — бид GOT биш
return address-ийг дардаг.

Флаг: `-Wl,-z,relro,-z,now` (Full), `-Wl,-z,relro` (Partial), `-Wl,-z,norelro` (байхгүй).

## SHSTK, IBT, Stripped

`checksec`-ийн сүүлийн мөрүүд:

- **SHSTK** (Shadow Stack) ба **IBT** (Indirect Branch Tracking) — Intel **CET** хамгаалалтууд. SHSTK нь
  return address-ийн хуулбарыг тусдаа stack дээр хадгалж `ret` бүрт харьцуулна; IBT нь үсрэлт зөвхөн
  `endbr64` зааварт буух ёстой гэж шаарддаг. Функц бүрийн эхэнд байдаг `endbr64` эндээс гаралтай.
  `Enabled` гэдэг нь binary эдгээрийг **дэмждэг** гэсэн тэмдэглэгээ — CPU, kernel, glibc бүгд дэмжиж,
  тусгайлан асаагаагүй бол хэрэгждэггүй. Ихэнх Linux систем дээр одоогоор идэвхгүй тул CTF-д ихэвчлэн тоохгүй.
- **Stripped** — `Yes` бол symbol table (функцийн нэрс) хасагдсан. `exe.sym.win` ажиллахгүй, gdb-д
  функцийн нэр харагдахгүй тул хаягийг disassembly-аас өөрөө олно. Хамгаалалт биш, зөвхөн reverse хийхэд хэцүү болгоно.

## Товч дүгнэлт

| Хамгаалалт | Унтраах флаг | Тойрох гол арга | Жишээ |
|------------|--------------|-----------------|-------|
| NX | `-z execstack` | Байгаа кодыг ашиглах (ROP) | [ret2text](../../ret2text/) |
| Canary | `-fno-stack-protector` | Canary leak, brute force | |
| ASLR | `randomize_va_space = 0` | Хаяг leak → base тооцох | |
| PIE | `-no-pie` | Кодын хаяг leak, partial overwrite | |
| RELRO | `-Wl,-z,norelro` | Өөр бичигдэх зорилт | |

Бодлого бүрийг `checksec`-ээр эхлүүлж, энэ хүснэгтээс **ямар арга боломжтойг** шийдээрэй.

## Эх сурвалж

- [CTF Wiki — Canary](https://ctf-wiki.org/pwn/linux/user-mode/mitigation/canary/)
- [ir0nstone — Stack Canaries](https://ir0nstone.gitbook.io/notes/binexp/stack/canaries)
- [ir0nstone — PIE](https://ir0nstone.gitbook.io/notes/binexp/stack/pie)
- [ir0nstone — ASLR](https://ir0nstone.gitbook.io/notes/binexp/stack/aslr)
