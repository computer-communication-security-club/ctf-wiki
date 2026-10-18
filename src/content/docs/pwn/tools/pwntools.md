---
title: pwntools
description: pwn template-ээр exploit бэлдэц үүсгэх, process/remote-той харилцах, p64/flat, ELF, cyclic, ROP зэрэг pwntools-ийн үндсэн хэрэгслүүд.
---

**[pwntools](https://github.com/Gallopsled/pwntools)** нь exploit бичихэд зориулсан Python сан.
Программ ажиллуулах, сервер рүү холбогдох, өгөгдөл илгээх/хүлээн авах, хаягийг байт болгох,
binary-аас symbol хайх гэх мэт pwn бодлогод байнга хийдэг бүх зүйлийг нэг дор өгдөг.

Энэ хуудасны жишээнүүд [ret2text](../../userland/ret2text/) бодлогын
[`ret2text`](https://github.com/computer-communication-security-club/ctf-wiki/raw/main/challenges/pwn/userland/ret2text/ret2text)
binary дээр хийгдсэн тул хаягууд таных яг ижил гарна.

```console
$ pip install pwntools
```

Бүх скрипт ингэж эхэлнэ — хэрэгтэй бүх функц нэг мөрөөр орж ирнэ:

```python
from pwn import *
```

## pwn template

Exploit-ийг хоосон файлаас эхлэх хэрэггүй. `pwn template` команд binary-г уншаад бэлэн бэлдэц (template) үүсгэнэ:

```console
$ pwn template ./ret2text --host localhost --port 1337 --quiet > exploit.py
$ chmod +x exploit.py
```

- `--host`, `--port` — challenge серверийн хаяг. Өгөхгүй бол зөвхөн local горимтой template гарна.
- `--quiet` — тайлбар comment-уудыг хасна. Анх удаа ашиглаж байвал `--quiet`-гүйгээр нэг үзээрэй —
  `checksec`-ийн үр дүн болон жишээ кодыг comment хэлбэрээр оруулдаг.

Үүссэн файл:

```python title="exploit.py"
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

io = start()

io.interactive()
```

Гол хэсгүүд:

- `exe = context.binary = ELF(...)` — binary-г ачаалж, `context`-д архитектурыг (amd64, little-endian) тохируулна.
  Ингэснээр `p64`, `flat`, `asm` зэрэг нь зөв форматаар ажиллана.
- `start()` — `LOCAL` өгвөл программыг өөрийн машин дээр, үгүй бол **сервер рүү** холбогдоно.
- `gdbscript` — `GDB` горимд gdb-д автоматаар өгөх командууд. Breakpoint-ээ энд бич.
- `io` — программтай харилцах объект (tube). Exploit-ийн бүх илгээх/хүлээн авах үйлдэл үүгээр явна.

Та зөвхөн `# -- Exploit goes here --` хэсэгт кодоо бичнэ.

### Ажиллуулах горимууд

Template-ийн хүч нь **command-line argument** юм. Скриптийг өөрчлөхгүйгээр горимыг сольж болно:

```console
$ ./exploit.py LOCAL            # өөрийн машин дээр ажиллуулна
$ ./exploit.py LOCAL GDB        # gdb дотор ажиллуулж, gdbscript-ийг гүйцэтгэнэ
$ ./exploit.py LOCAL DEBUG      # илгээсэн/хүлээн авсан бүх байтыг хэвлэнэ
$ ./exploit.py                  # localhost:1337 сервер рүү
$ ./exploit.py HOST=ctf.example.com PORT=31337
```

| Argument | Үйлдэл |
|----------|--------|
| `LOCAL` | Binary-г local-аар ажиллуулна (байхгүй бол remote) |
| `GDB` | gdb-тэй хамт эхлүүлнэ |
| `DEBUG` | `context.log_level = 'debug'` — бүх traffic-ийг харуулна |
| `NOASLR` | Local process-д ASLR унтраана |
| `HOST=...` `PORT=...` | Серверийн хаягийг солино |
| `EXE=...` | Өөр binary ашиглана |

:::caution
`LOCAL`-гүйгээр ажиллуулбал template **remote** горимд орно. `make run`-аар сервер асаагаагүй бол
`Could not connect to localhost on port 1337` алдаа гарна.
:::

:::tip
`GDB` горим gdb-г шинэ terminal цонхонд нээдэг. tmux дотор ажиллаж байвал автоматаар хуваана.
Цонх нээгдэхгүй бол `start()`-ийн өмнө terminal-аа зааж өг:
`context.terminal = ['tmux', 'splitw', '-h']`
:::

:::note[Docker-тэй хавтас]
`pwn template` нь хавтсанд `Dockerfile` байвал libc-г image-аас автоматаар гаргаж авахыг оролддог.
Docker ажиллахгүй үед алдаа өгвөл `--no-auto` нэмээрэй.
:::

## Process ба remote

Template-гүйгээр tube-ийг шууд үүсгэж болно:

```python
io = process('./ret2text')           # local программ
io = remote('localhost', 1337)       # TCP сервер (nc localhost 1337-тай ижил)
```

Хоёулаа **ижил** функцтэй тул local дээр бичсэн exploit серверт өөрчлөлтгүй ажиллана:

| Функц | Үйлдэл |
|-------|--------|
| `io.send(data)` | Өгөгдөл илгээх |
| `io.sendline(data)` | Өгөгдөл + `\n` илгээх |
| `io.sendafter(delim, data)` | `delim` иртэл хүлээгээд илгээх |
| `io.sendlineafter(delim, data)` | `delim` иртэл хүлээгээд мөр илгээх |
| `io.recv(n)` | `n` хүртэл байт унших |
| `io.recvline()` | Нэг мөр унших |
| `io.recvuntil(delim)` | `delim` хүртэл унших |
| `io.recvall()` | Холболт хаагдтал бүгдийг унших |
| `io.interactive()` | Гараас шууд бичих горимд шилжих (shell авсны дараа) |

Бүх өгөгдөл **`bytes`** байна: `'name?'` биш `b'name?'` гэж бич.

:::tip
`sendline`-ийн оронд `sendlineafter` ашиглах нь найдвартай. Программ асуултаа хэвлэж дуусаагүй байхад
өгөгдөл илгээвэл серверт, ялангуяа сүлжээ удаан үед, асуудал үүсч болно.
:::

## Хаягийг байт болгох

x86-64 нь **little-endian**: олон байттай тоог санах ойд бага байтаас нь эхэлж бичдэг.
`p64` нь тоог ийм 8 байт болгоно, `u64` эсрэгээр нь:

```python
>>> p64(0x401196)
b'\x96\x11@\x00\x00\x00\x00\x00'
>>> hex(u64(b'\x96\x11@\x00\x00\x00\x00\x00'))
'0x401196'
>>> p32(0xdeadbeef)
b'\xef\xbe\xad\xde'
```

(`@` нь `0x40` байт — Python хэвлэх боломжтой байтыг тэмдэгтээр харуулдаг.)

Leak хийсэн хаяг ихэвчлэн 6 байт ирдэг (дээд 2 байт нь `\x00`). `u64` яг 8 байт шаарддаг тул нөхнө:

```python
leak = u64(io.recv(6).ljust(8, b'\x00'))
```

### flat

`flat` нь олон хэсгийг нэг payload болгон нийлүүлнэ. Тоог `context`-ийн дагуу автоматаар `p64` хийнэ:

```python
>>> flat(b'A' * 8, 0x401196)
b'AAAAAAAA\x96\x11@\x00\x00\x00\x00\x00'
```

Payload-ийг шалгахад `hexdump` тустай:

```python
>>> print(hexdump(flat(b'A' * 8, 0x401196)))
00000000  41 41 41 41  41 41 41 41  96 11 40 00  00 00 00 00  │AAAA│AAAA│··@·│····│
00000010
```

## ELF

`ELF()` нь binary-г уншаад хамгаалалтыг (`checksec`) хэвлэнэ:

```python
>>> exe = context.binary = ELF('./ret2text')
[*] './ret2text'
    Arch:       amd64-64-little
    RELRO:      Partial RELRO
    Stack:      No canary found
    NX:         NX enabled
    PIE:        No PIE (0x400000)
    SHSTK:      Enabled
    IBT:        Enabled
    Stripped:   No
```

Хаягуудыг gdb-ээр хайх шаардлагагүй:

```python
>>> hex(exe.sym.win)                     # функц, глобал хувьсагч
'0x401196'
>>> hex(exe.got.puts)                    # GOT entry
'0x404000'
>>> hex(next(exe.search(b'/bin/sh')))    # binary доторх string
'0x402022'
```

`exe.sym`, `exe.got`, `exe.plt` нь dictionary тул `exe.sym['win']` гэж бас бичиж болно.

:::note
PIE-тэй binary-д эдгээр нь 0-ээс эхэлсэн **offset** байна. Base хаягийг leak хийсний дараа
`exe.address = base` гэж оноовол бүх хаяг автоматаар шинэчлэгдэнэ.
:::

## cyclic: offset олох

`cyclic(n)` нь аль ч 4 байт нь **давтагддаггүй** pattern үүсгэнэ. Программ унахад return address-ийн
байранд очсон 4 байтаас pattern-ийн хаана байсныг `cyclic_find` олно:

```python
>>> cyclic(32)
b'aaaabaaacaaadaaaeaaafaaagaaahaaa'
>>> cyclic_find(0x61616173)          # эсвэл cyclic_find(b'saaa')
72
```

Terminal болон pwndbg-ээс ч ажиллана (`!pwn cyclic 200` гэх мэт):

```console
$ pwn cyclic 200
$ pwn cyclic -l 0x61616173
72
```

Offset-ийг бүр **автоматаар** олж болно: программыг унагаагаад pwntools core dump-аас `rsp`-г уншина.

```python
io = process(exe.path)
io.sendlineafter(b'name?\n', cyclic(200))
io.wait()                                       # программ унатал хүлээнэ

core = io.corefile
offset = cyclic_find(core.read(core.rsp, 4))    # ret хийх гэж байсан утга
log.info(f'offset = {offset}')                  # offset = 72
```

:::tip
Core dump үүсэхгүй бол `ulimit -c unlimited` ажиллуулаад дахин оролдоорой.
:::

## ROP

`ROP` нь binary-аас gadget хайж, гинж (chain) угсрахад тусална:

```python
>>> rop = ROP(exe)
>>> rop.find_gadget(['ret'])
Gadget(0x40101a, ['ret'], [], 0x8)
>>> rop.find_gadget(['ret'])[0]          # зөвхөн хаяг
4198426
```

Функц дуудах гинжийг гараар бичихгүйгээр `rop.call` хийж, `dump()`-аар шалгана:

```python
>>> rop.raw(rop.find_gadget(['ret'])[0])
>>> rop.call('win')
>>> print(rop.dump())
0x0000:         0x40101a ret
0x0008:         0x401196 win()
>>> payload = flat(b'A' * 72, rop.chain())
```

Аргументтай функцэд (`rop.call('system', [binsh])` гэх мэт) `pop rdi; ret` зэрэг gadget-уудыг өөрөө олж тавьдаг.

## Лог ба debug

```python
log.info('энгийн мэдээлэл')       # [*] энгийн мэдээлэл
log.success(f'leak: {hex(leak)}')  # [+] leak: 0x7ffff7c58750
context.log_level = 'debug'        # DEBUG argument-тай ижил
```

`DEBUG` горимд программтай солилцсон бүх байт харагдана. Payload яг зөв илгээгдэж байгаа эсэхийг
шалгах хамгийн хурдан арга:

```console
$ ./exploit.py LOCAL DEBUG
[+] Starting local process './ret2text': pid 78840
[DEBUG] Received 0x13 bytes:
    b'What is your name?\n'
[DEBUG] Sent 0x59 bytes:
    00000000  41 41 41 41  41 41 41 41  41 41 41 41  41 41 41 41  │AAAA│AAAA│AAAA│AAAA│
    *
    00000040  41 41 41 41  41 41 41 41  1a 10 40 00  00 00 00 00  │AAAA│AAAA│··@·│····│
    00000050  96 11 40 00  00 00 00 00  0a                        │··@·│····│·│
    00000059
[*] Switching to interactive mode
```

`*` нь өмнөх мөртэй ижил мөрүүдийг товчилсон гэсэн үг. Сүүлийн `0a` нь `sendline`-ий нэмсэн `\n`.

## Товч лавлах

| Код | Үйлдэл |
|-----|--------|
| `pwn template ./bin --host H --port P` | Exploit бэлдэц үүсгэх |
| `./exploit.py LOCAL` / `GDB` / `DEBUG` | Горим сонгох |
| `process(path)` / `remote(host, port)` | Tube үүсгэх |
| `io.sendlineafter(delim, data)` | Хүлээгээд мөр илгээх |
| `io.recvuntil(delim)` / `io.recvline()` | Унших |
| `io.interactive()` | Гараар харилцах |
| `p64(x)` / `u64(b)` | Тоо ↔ 8 байт (little-endian) |
| `flat(...)` | Payload угсрах |
| `exe.sym.f` / `exe.got.f` / `exe.plt.f` | Хаяг авах |
| `next(exe.search(b'...'))` | String-ийн хаяг |
| `cyclic(n)` / `cyclic_find(x)` | Offset олох |
| `ROP(exe).find_gadget(['ret'])[0]` | Gadget хайх |
| `rop.call(f, [args])` / `rop.chain()` | ROP гинж |
| `hexdump(data)` | Байтыг харах |
| `log.info()` / `log.success()` | Лог хэвлэх |

## Эх сурвалж

- [pwntools documentation](https://docs.pwntools.com/en/stable/)
- [pwntools-tutorial](https://github.com/Gallopsled/pwntools-tutorial)
