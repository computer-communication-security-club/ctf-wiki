---
title: Calling convention
description: Функц дуудахад аргумент, буцах утга, регистрүүд хаана байрладаг тухай дүрэм — x86-64 System V, 32-bit cdecl, syscall, мөн pwn-д яагаад чухал вэ.
---

**Calling convention** (дуудлагын дүрэм) нь функц дуудагч (**caller**) болон дуудагдагч (**callee**) хоёрын
хоорондох **гэрээ** юм. Энэ гэрээ дараах асуултуудад хариулдаг:

- Аргументууд **аль регистрт** эсвэл stack-ийн **хаана** очих вэ?
- Буцах утга хаана ирэх вэ?
- Функц аль регистрийг **эвдэж** болох, алийг нь **хэвээр** үлдээх ёстой вэ?
- Stack-ийг хэн цэвэрлэх, ямар **тэгшлэлт** (alignment)-тэй байх ёстой вэ?

Compiler энэ дүрмийг өөрөө мөрддөг тул C код бичихэд анзаарагддаггүй. Харин pwn-д бид функцийг `call`-аар биш,
stack болон регистрүүдийг **гараар** бэлдэж дуудна. Дүрмийг мэдэхгүй бол `system("/bin/sh")`-д аргумент ч
дамжуулж чадахгүй.

Энэ хуудас Linux-ийн **x86-64 System V ABI**-д (CTF-ийн ихэнх binary) төвлөрнө. Төгсгөлд нь 32-bit болон
syscall-ийн дүрмийг харьцуулна.

## Товч хүснэгт

| | x86-64 (System V) | i386 (cdecl) |
|---|---|---|
| Аргумент | `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`, үлдсэн нь stack | Бүгд stack дээр |
| Буцах утга | `rax` | `eax` |
| Callee хадгалах ёстой | `rbx`, `rbp`, `rsp`, `r12`–`r15` | `ebx`, `esi`, `edi`, `ebp`, `esp` |
| Stack-ийн аргументыг цэвэрлэгч | Caller | Caller |
| Alignment | `call`-ийн өмнө `rsp` 16-д хуваагдана | `call`-ийн өмнө `esp` 16-д хуваагдана |

## Жишээ программ

8 аргументтай функц: эхний 6 нь регистрт багтаж, үлдсэн 2 нь stack руу шилжинэ.

```c title="args.c"
#include <stdio.h>

long sum8(long a, long b, long c, long d, long e, long f, long g, long h) {
    long r = a + b + c + d + e + f + g + h;
    return r;
}

int main(void) {
    long r = sum8(1, 2, 3, 4, 5, 6, 7, 8);
    printf("sum = %ld\n", r);
    return 0;
}
```

```console
$ gcc args.c -o args -no-pie -fno-stack-protector
$ ./args
sum = 36
```

## Аргумент дамжуулах

### Caller тал

`main`-ийн disassembly (тодруулсан мөрүүд аргументуудыг бэлдэж байна):

```ansi {7-14}
[1;31mpwndbg>[0m disassemble main
Dump of assembler code for function [33mmain[0m:
   [34m0x0000000000401195[0m <+0>:	[32mendbr64[0m
   [34m0x0000000000401199[0m <+4>:	[32mpush   [0m[31mrbp[0m
   [34m0x000000000040119a[0m <+5>:	[32mmov    [0m[31mrbp[0m,[31mrsp[0m
   [34m0x000000000040119d[0m <+8>:	[32msub    [0m[31mrsp[0m,[34m0x10[0m
   [34m0x00000000004011a1[0m <+12>:	[32mpush   [0m[34m0x8[0m
   [34m0x00000000004011a3[0m <+14>:	[32mpush   [0m[34m0x7[0m
   [34m0x00000000004011a5[0m <+16>:	[32mmov    [0m[31mr9d[0m,[34m0x6[0m
   [34m0x00000000004011ab[0m <+22>:	[32mmov    [0m[31mr8d[0m,[34m0x5[0m
   [34m0x00000000004011b1[0m <+28>:	[32mmov    [0m[31mecx[0m,[34m0x4[0m
   [34m0x00000000004011b6[0m <+33>:	[32mmov    [0m[31medx[0m,[34m0x3[0m
   [34m0x00000000004011bb[0m <+38>:	[32mmov    [0m[31mesi[0m,[34m0x2[0m
   [34m0x00000000004011c0[0m <+43>:	[32mmov    [0m[31medi[0m,[34m0x1[0m
   [34m0x00000000004011c5[0m <+48>:	[32mcall   [0m[34m0x401136[0m <[33msum8[0m>
   [34m0x00000000004011ca[0m <+53>:	[32madd    [0m[31mrsp[0m,[34m0x10[0m
   [34m0x00000000004011ce[0m <+57>:	[32mmov    [0mQWORD PTR [[31mrbp[0m-[34m0x8[0m],[31mrax[0m
   [34m0x00000000004011d2[0m <+61>:	[32mmov    [0m[31mrax[0m,QWORD PTR [[31mrbp[0m-[34m0x8[0m]
   [34m0x00000000004011d6[0m <+65>:	[32mlea    [0m[31mrdx[0m,[[31mrip[0m+[34m0xe27[0m][90m        # [0m[90m0x402004[0m
   [34m0x00000000004011dd[0m <+72>:	[32mmov    [0m[31mrsi[0m,[31mrax[0m
   [34m0x00000000004011e0[0m <+75>:	[32mmov    [0m[31mrdi[0m,[31mrdx[0m
   [34m0x00000000004011e3[0m <+78>:	[32mmov    [0m[31meax[0m,[34m0x0[0m
   [34m0x00000000004011e8[0m <+83>:	[32mcall   [0m[34m0x401040[0m <[33mprintf@plt[0m>
   [34m0x00000000004011ed[0m <+88>:	[32mmov    [0m[31meax[0m,[34m0x0[0m
   [34m0x00000000004011f2[0m <+93>:	[32mleave[0m
   [34m0x00000000004011f3[0m <+94>:	[32mret[0m
End of assembler dump.
```

- 7, 8 дахь аргумент (`g`, `h`) регистрт багтахгүй тул **stack** руу `push` хийгдэнэ. Дараалал нь
  **баруунаас зүүн** тийш — эхлээд `8`, дараа нь `7` — тиймээс `g` хамгийн бага хаягт, `call`-д хамгийн ойр байрлана.
  ABI-ийн тайлбараар баруунаас зүүн дараалал нь variadic функцэд эхний аргументын байрлалыг үргэлж тогтмол болгодог.
- Эхний 6 нь `edi`…`r9d`-д очно. Compiler 32-bit хэсгийг нь (`edi`) бичдэг: x86-64 дээр 32-bit регистрт бичихэд
  дээд 32 bit нь автоматаар тэглэгддэг тул `rdi = 1` болно.
- `call`-ийн дараах `add rsp,0x10` — stack руу түлхсэн 2 аргументыг **caller** өөрөө цэвэрлэнэ.
- `printf`-ийн дуудлага ч мөн адил: `rdi` = format string-ийн хаяг (`0x402004`), `rsi` = `r`.
  Харин `+78` дахь `mov eax,0x0` нь variadic (`...`) функцийн тусгай дүрэм: `al` нь хэдэн vector (`xmm`) регистр
  ашигласныг заана. Энд float аргумент байхгүй тул `0`.

### Callee тал

pwndbg-ээр `call`-ийн яг өмнө зогсоод, нэг заавар урагшилъя (`stepi`):

```ansi
[1;31mpwndbg>[0m break *main+48
[1;31mpwndbg>[0m run
Breakpoint 1, 0x00000000004011c5 in main ()
[1;31mpwndbg>[0m regs rsp
[31m*[0m[31;1mRSP [0m [33m0x7fffffffd6a0[0m ◂— 7
[1;31mpwndbg>[0m stepi
0x0000000000401136 in sum8 ()
[1;31mpwndbg>[0m regs rdi rsi rdx rcx r8 r9 rsp
 [1mRDI [0m 1
 [1mRSI [0m 2
 [1mRDX [0m 3
 [1mRCX [0m 4
 [1mR8  [0m 5
 [1mR9  [0m 6
[31m*[0m[31;1mRSP [0m [33m0x7fffffffd698[0m —▸ [31m0x4011ca (main+53)[0m ◂— [32madd[0m [31mrsp[0m, [34m0x10[0m
[1;31mpwndbg>[0m telescope $rsp 3
00:0000│ [1mrsp[0m [33m0x7fffffffd698[0m —▸ [31m0x4011ca (main+53)[0m ◂— [32madd[0m [31mrsp[0m, [34m0x10[0m
01:0008│-020 [33m0x7fffffffd6a0[0m ◂— 7
02:0010│-018 [33m0x7fffffffd6a8[0m ◂— 8
```

- `RDI`…`R9` = `1`…`6` — эхний зургаан аргумент.
- `call`-ийн өмнө `rsp = 0x7fffffffd6a0` байсан бол `sum8`-д орсны дараа `0x7fffffffd698` болсон: `call` нь
  **буцах хаяг** `0x4011ca (main+53)`-ийг stack руу түлхсэн.
- `[rsp+8] = 7`, `[rsp+0x10] = 8` — stack аргументууд return address-ийн **дээр** (их хаяг талд) байрлана.

`sum8` өөрөө аргументуудаа хаанаас уншдагийг харъя:

```ansi {6-11,23,25}
[1;31mpwndbg>[0m disassemble sum8
Dump of assembler code for function [33msum8[0m:
   [34m0x0000000000401136[0m <+0>:	[32mendbr64[0m
   [34m0x000000000040113a[0m <+4>:	[32mpush   [0m[31mrbp[0m
   [34m0x000000000040113b[0m <+5>:	[32mmov    [0m[31mrbp[0m,[31mrsp[0m
   [34m0x000000000040113e[0m <+8>:	[32mmov    [0mQWORD PTR [[31mrbp[0m-[34m0x18[0m],[31mrdi[0m
   [34m0x0000000000401142[0m <+12>:	[32mmov    [0mQWORD PTR [[31mrbp[0m-[34m0x20[0m],[31mrsi[0m
   [34m0x0000000000401146[0m <+16>:	[32mmov    [0mQWORD PTR [[31mrbp[0m-[34m0x28[0m],[31mrdx[0m
   [34m0x000000000040114a[0m <+20>:	[32mmov    [0mQWORD PTR [[31mrbp[0m-[34m0x30[0m],[31mrcx[0m
   [34m0x000000000040114e[0m <+24>:	[32mmov    [0mQWORD PTR [[31mrbp[0m-[34m0x38[0m],[31mr8[0m
   [34m0x0000000000401152[0m <+28>:	[32mmov    [0mQWORD PTR [[31mrbp[0m-[34m0x40[0m],[31mr9[0m
   [34m0x0000000000401156[0m <+32>:	[32mmov    [0m[31mrdx[0m,QWORD PTR [[31mrbp[0m-[34m0x18[0m]
   [34m0x000000000040115a[0m <+36>:	[32mmov    [0m[31mrax[0m,QWORD PTR [[31mrbp[0m-[34m0x20[0m]
   [34m0x000000000040115e[0m <+40>:	[32madd    [0m[31mrdx[0m,[31mrax[0m
   [34m0x0000000000401161[0m <+43>:	[32mmov    [0m[31mrax[0m,QWORD PTR [[31mrbp[0m-[34m0x28[0m]
   [34m0x0000000000401165[0m <+47>:	[32madd    [0m[31mrdx[0m,[31mrax[0m
   [34m0x0000000000401168[0m <+50>:	[32mmov    [0m[31mrax[0m,QWORD PTR [[31mrbp[0m-[34m0x30[0m]
   [34m0x000000000040116c[0m <+54>:	[32madd    [0m[31mrdx[0m,[31mrax[0m
   [34m0x000000000040116f[0m <+57>:	[32mmov    [0m[31mrax[0m,QWORD PTR [[31mrbp[0m-[34m0x38[0m]
   [34m0x0000000000401173[0m <+61>:	[32madd    [0m[31mrdx[0m,[31mrax[0m
   [34m0x0000000000401176[0m <+64>:	[32mmov    [0m[31mrax[0m,QWORD PTR [[31mrbp[0m-[34m0x40[0m]
   [34m0x000000000040117a[0m <+68>:	[32madd    [0m[31mrdx[0m,[31mrax[0m
   [34m0x000000000040117d[0m <+71>:	[32mmov    [0m[31mrax[0m,QWORD PTR [[31mrbp[0m+[34m0x10[0m]
   [34m0x0000000000401181[0m <+75>:	[32madd    [0m[31mrdx[0m,[31mrax[0m
   [34m0x0000000000401184[0m <+78>:	[32mmov    [0m[31mrax[0m,QWORD PTR [[31mrbp[0m+[34m0x18[0m]
   [34m0x0000000000401188[0m <+82>:	[32madd    [0m[31mrax[0m,[31mrdx[0m
   [34m0x000000000040118b[0m <+85>:	[32mmov    [0mQWORD PTR [[31mrbp[0m-[34m0x8[0m],[31mrax[0m
   [34m0x000000000040118f[0m <+89>:	[32mmov    [0m[31mrax[0m,QWORD PTR [[31mrbp[0m-[34m0x8[0m]
   [34m0x0000000000401193[0m <+93>:	[32mpop    [0m[31mrbp[0m
   [34m0x0000000000401194[0m <+94>:	[32mret[0m
End of assembler dump.
```

- `+8`…`+28`: регистрээр ирсэн 6 аргументыг stack руу хуулна (`-O0` үед compiler бүх хувьсагчийг санах ойд хадгалдаг).
- `[rbp+0x10]`, `[rbp+0x18]`: stack-ээр ирсэн `g`, `h`. `push rbp` хийсний дараа `rbp+8` нь return address,
  `rbp` нь saved rbp тул stack аргументууд `rbp+0x10`-аас эхэлнэ.
- `sub rsp` байхгүй атлаа `[rbp-0x40]` хүртэл бичиж байна. Энэ бол **red zone**: өөр функц дууддаггүй (leaf)
  функц `rsp`-ээс доош 128 байтыг `rsp`-г хөдөлгөлгүйгээр ашиглаж болно.

`sum8`-ийн stack frame:

```ansi
[1m их хаяг[0m
[90m┌──────────────────────────┐[0m
[90m│[0m[36m  h = 8                   [0m[90m│[0m  ← rbp + 0x18      [36m← 8 дахь аргумент[0m
[90m├──────────────────────────┤[0m
[90m│[0m[36m  g = 7                   [0m[90m│[0m  ← rbp + 0x10      [36m← 7 дахь аргумент[0m
[90m├──────────────────────────┤[0m
[90m│[0m[1;31m  return address  (8 B)   [0m[90m│[0m  ← rbp + 8         [1;31m← main+53 (0x4011ca)[0m
[90m├──────────────────────────┤[0m
[90m│[0m[33m  saved rbp       (8 B)   [0m[90m│[0m  ← rbp
[90m├──────────────────────────┤[0m
[90m│[0m[32m  r, a … f (локал)        [0m[90m│[0m  ← rbp - 0x8 … rbp - 0x40
[90m└──────────────────────────┘[0m
[1m бага хаяг[0m
```

### Буцах утга

`finish` — одоогийн функц дуустал ажиллуулна:

```ansi
[1;31mpwndbg>[0m finish
0x00000000004011ca in main ()
[1;31mpwndbg>[0m regs rax
[31m*[0m[31;1mRAX [0m 0x24
```

`rax = 0x24 = 36`. Буцах утгын дүрэм:

- Бүхэл тоо, pointer → `rax` (128-bit бол `rdx:rax`).
- `float`, `double` → `xmm0`. Float **аргументууд** ч мөн тусдаа: `xmm0`–`xmm7`.

## Prologue ба epilogue

Бараг бүх функц ижил эхлэл, төгсгөлтэй:

```asm
push rbp          # caller-ийн rbp-г хадгална
mov  rbp, rsp     # шинэ frame-ийн суурь
sub  rsp, N       # локал хувьсагчдад зай гаргана
...
leave             # = mov rsp, rbp ; pop rbp
ret               # = pop rip
```

`call X` нь `push <дараагийн зааврын хаяг>` + `jmp X`-тэй ижил. `ret` нь эсрэгээр stack-ийн оройн утгыг **ямар ч
шалгалтгүйгээр** `rip` болгодог — stack overflow аюултай байдгийн гол шалтгаан энэ.

`leave` нь `rsp`-г `rbp`-ээс сэргээдэг. Тиймээс saved rbp-г дарж бичвэл дараагийн `leave` дээр stack-ийг өөрийн
сонгосон газар руу шилжүүлж болно — үүнийг **stack pivot** гэдэг.

## Caller-saved ба callee-saved регистр

| Төрөл | Регистр | Утга |
|-------|---------|------|
| **Caller-saved** (volatile) | `rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8`–`r11` | Функц дуудсаны дараа өөрчлөгдсөн байж болно. Хэрэгтэй бол caller өөрөө хадгална |
| **Callee-saved** (non-volatile) | `rbx`, `rbp`, `rsp`, `r12`–`r15` | Функц эдгээрийг ашиглавал эхэнд `push`, төгсгөлд `pop` хийж сэргээх ёстой |

pwn-д хэрэгтэй хоёр зүйл:

- Функцүүдийн төгсгөлд `pop rbx; pop rbp; pop r12; ... ; ret` гэсэн дараалал олон тааралддаг — callee-saved
  регистрүүдийг сэргээж буй хэрэг. Эдгээр нь ROP-д бэлэн `pop` **gadget** болж хэрэглэгддэг.
- libc-ийн функц дуудсаны дараа `rdi`, `rsi`, `rdx` зэрэгт ямар утга үлдсэнд найдаж болохгүй.

## Stack alignment

Дүрэм: `call` гүйцэтгэхийн **өмнө** `rsp` 16-д хуваагдах ёстой. `call` 8 байт түлхдэг тул функцийн **эхэнд**
`rsp`-ийн сүүлийн hex орон `8` байна.

Дээрх pwndbg гаралт яг үүнийг харуулж байна: `call`-ийн өмнө `0x7fffffffd6a0` (`0`-ээр төгсөнө), `sum8`-ийн эхэнд
`0x7fffffffd698`.

Exploit-д функцийг `call`-аар биш `ret`-ээр эхлүүлдэг тул энэ тэнцвэр 8 байтаар алдагдаж болно. glibc-ийн `system()`
доторх `movaps` заавар тэгшлэгдээгүй хаяг дээр `SIGSEGV` өгдөг. Шийдэл нь нэмэлт `ret` gadget —
[ret2text](../../userland/ret2text/)-ийн "Stack alignment" алхамд жишээтэй.

## 32-bit (cdecl)

Ижил программыг 32-bit-ээр compile хийвэл (`gcc -m32`, `gcc-multilib` багц шаардлагатай) `main` дотор:

```asm title="gcc -m32 -S -masm=intel args.c (хэсэг)"
push    8
push    7
push    6
push    5
push    4
push    3
push    2
push    1
call    sum8
add     esp, 32
```

- **8 аргумент бүгд stack дээр**, мөн л баруунаас зүүн тийш.
- Caller `add esp, 32` (8 × 4 байт)-аар цэвэрлэнэ.
- Буцах утга `eax`-д ирнэ.

`sum8` дотор аргументууд `[ebp+0x8]`…`[ebp+0x24]`-ээс уншигдана:

```ansi
[1m их хаяг[0m
[90m┌──────────────────────────┐[0m
[90m│[0m[36m  h = 8                   [0m[90m│[0m  ← ebp + 0x24
[90m├──────────────────────────┤[0m
[90m│[0m[90m  …                       [0m[90m│[0m
[90m├──────────────────────────┤[0m
[90m│[0m[36m  b = 2                   [0m[90m│[0m  ← ebp + 0xc
[90m├──────────────────────────┤[0m
[90m│[0m[36m  a = 1                   [0m[90m│[0m  ← ebp + 8         [36m← 1-р аргумент[0m
[90m├──────────────────────────┤[0m
[90m│[0m[1;31m  return address  (4 B)   [0m[90m│[0m  ← ebp + 4
[90m├──────────────────────────┤[0m
[90m│[0m[33m  saved ebp       (4 B)   [0m[90m│[0m  ← ebp
[90m└──────────────────────────┘[0m
[1m бага хаяг[0m
```

### pwn-д ялгаа

**32-bit** дээр аргумент stack дээр байдаг тул overflow-оор return address-ийн ард шууд тавьж болно:

```python
payload  = b'A' * offset
payload += p32(exe.sym.system)   # ret → system()
payload += p32(0xdeadbeef)       # system()-ийн "буцах хаяг"
payload += p32(binsh)            # system()-ийн 1-р аргумент
```

Яагаад дунд нь хуурамч хаяг вэ? `system()` өөрийгөө `call`-аар дуудагдсан гэж үздэг: `[esp]` = буцах хаяг,
`[esp+4]` = 1-р аргумент. Бид `ret`-ээр орсон тул тэр буцах хаягийг өөрсдөө тавих ёстой. `0xdeadbeef`-ийн оронд
`exit`-ийн хаягийг тавьвал shell хаагдсаны дараа программ crash хийлгүй гарна.

**64-bit** дээр аргумент **регистрт** байх ёстой. Stack-ээс регистр рүү утга оруулах gadget хэрэгтэй:

```python
payload  = b'A' * offset
payload += p64(pop_rdi)          # pop rdi; ret
payload += p64(binsh)            # → rdi
payload += p64(ret)              # stack alignment
payload += p64(exe.sym.system)
```

pwntools-ийн `ROP` класс эдгээр gadget-ийг өөрөө олж тавьдаг ([pwntools үндэс](../../tools/pwntools/)).

## Syscall

Kernel рүү хандах (**syscall**) дүрэм нь энгийн функцийн дүрмээс арай өөр:

| | x86-64 | i386 |
|---|---|---|
| Заавар | `syscall` | `int 0x80` |
| Syscall-ийн дугаар | `rax` | `eax` |
| Аргумент | `rdi`, `rsi`, `rdx`, **`r10`**, `r8`, `r9` | `ebx`, `ecx`, `edx`, `esi`, `edi`, `ebp` |
| Буцах утга | `rax` | `eax` |
| Эвдэгддэг | `rcx`, `r11` | — |

- 4 дэх аргумент `rcx` биш **`r10`**: `syscall` заавар буцах хаягийг `rcx`-д, `rflags`-ийг `r11`-д хадгалдаг тул.
- Алдаа гарвал `rax`-д сөрөг утга (`-errno`) буцна.
- Нэг syscall-ийн дугаар архитектураас хамаарна: `execve` нь x86-64 дээр **59**, i386 дээр **11**.

Жишээ — shellcode-ийн хамгийн түгээмэл хэлбэр:

```asm
# execve("/bin/sh", NULL, NULL)
mov  rax, 59             # execve-ийн дугаар
lea  rdi, [rip+binsh]    # 1: path
xor  esi, esi            # 2: argv = NULL
xor  edx, edx            # 3: envp = NULL
syscall
binsh: .string "/bin/sh"
```

pwntools-д `constants.SYS_execve`, `shellcraft.sh()` зэрэг нь дугаар болон регистрийг автоматаар зөв тавина.

:::note[Windows]
Windows x64 өөр дүрэмтэй: аргументууд `rcx`, `rdx`, `r8`, `r9`-д очих ба caller stack дээр 32 байтын
"shadow space" гаргах ёстой. CTF-ийн pwn бодлого ихэвчлэн Linux дээр байдаг тул энэ wiki System V-г ашиглана.
:::

## Товч дүгнэлт

- x86-64: аргумент `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9` → stack. Буцах утга `rax`.
- i386: бүх аргумент stack дээр, `[esp+4]`-өөс эхэлнэ (`[esp]` = буцах хаяг).
- `call`-ийн өмнө `rsp` 16-д хуваагдана — `system()` унавал эхлээд alignment-ийг шалга.
- Syscall: дугаар `rax`-д, 4 дэх аргумент `r10`-д.
- 64-bit exploit-д аргумент тавихын тулд `pop rdi; ret` гэх мэт gadget хэрэгтэй.

## Эх сурвалж

- [System V x86-64 psABI](https://gitlab.com/x86-psABIs/x86-64-ABI)
- [System V AMD64 psABI 0.99.6 (PDF)](https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf) — Figure 3.4 (регистрийн хэрэглээ), §3.2.3, Appendix A.2.1 (kernel)
- [OSDev — System V ABI](https://wiki.osdev.org/System_V_ABI)
- [Intel — SYSCALL](https://www.felixcloutier.com/x86/syscall) — `rcx`, `r11`-ийг хэрхэн эвддэг
- [glibc — `sysdeps/unix/sysv/linux/x86_64/sysdep.h`](https://sourceware.org/git/?p=glibc.git;a=blob;f=sysdeps/unix/sysv/linux/x86_64/sysdep.h) — `DOARGS_4`: `movq %rcx, %r10`
- [`man 2 syscall`](https://man7.org/linux/man-pages/man2/syscall.2.html) — архитектур бүрийн syscall-ийн дүрэм
- [Linux syscall table (x86-64)](https://filippo.io/linux-syscall-table/)
