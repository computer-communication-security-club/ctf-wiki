---
title: GOT ба PLT
description: Dynamic linking дэх GOT (Global Offset Table) ба PLT (Procedure Linkage Table) гэж юу вэ, lazy binding хэрхэн ажилладаг, pwn-д яагаад чухал.
---

Программ `puts`, `printf` зэрэг **libc-ийн функц** дуудахад тэдгээрийн хаяг **compile хийх үед тодорхойгүй** байдаг —
libc нь тусдаа файл бөгөөд ажиллах үед, [ASLR](../protections/#aslr)-ийн улмаас санамсаргүй хаягт ачаалагддаг.
Энэ асуудлыг **GOT** ба **PLT** гэсэн хоёр хүснэгт шийддэг.

:::note[Юуны өмнө]
- [Calling convention](../calling-convention/), [pwndbg](../../tools/gdb/)
:::

## Хоёр хүснэгт

| Хүснэгт | Юу вэ |
|---------|-------|
| **PLT** (Procedure Linkage Table) | Гадаад функц бүрт зориулсан жижиг **дамжуулагч код** (stub). `call puts@plt` эндээс эхэлнэ |
| **GOT** (Global Offset Table) | Функцүүдийн **жинхэнэ хаягийг** хадгалдаг **заагчдын хүснэгт**. PLT stub эндээс хаягийг уншина |

Дуудлагын урсгал:

![GOT/PLT дуудлагын урсгал: программын код call puts@plt хийнэ → PLT stub нь puts@got-оос хаягийг уншиж jmp хийнэ → GOT нь жинхэнэ libc хаяг руу заана. Анх удаа GOT нь resolver руу заадаг бөгөөд dynamic linker жинхэнэ хаягийг олж GOT-д бичнэ, дараагийн удаа шууд явна.](../../../../assets/pwn/got-plt-flow.svg)

## Жишээ

```c title="demo.c"
int main(void) {
    puts("hello");
    puts("world");
    return 0;
}
```

`puts`-ийг дуудахдаа compiler шууд libc руу биш, **PLT** руу `call` хийдэг:

```console
$ objdump -d -M intel demo
...
401148:  call   401040 <puts@plt>      # эхний puts("hello")
...
401157:  call   401040 <puts@plt>      # хоёр дахь puts("world")
```

`puts@plt` (PLT stub) нь ердөө **GOT-оос хаягийг уншаад тийшээ үсэрнэ**:

```console
401040 <puts@plt>:
  401040:  endbr64
  401044:  jmp    QWORD PTR [rip+0x2fb6]    # 404000 <puts@got>
```

`404000` нь `puts`-ийн **GOT entry** (`puts@got`). Энд `puts`-ийн жинхэнэ хаяг хадгалагдана:

```console
$ readelf -r demo
...
000000404000  ...  R_X86_64_JUMP_SLO  0000000000000000  puts@GLIBC_2.2.5 + 0
```

## Lazy binding

GOT-д хаяг **анхнаасаа бичээстэй байдаггүй** — програм эхлэхэд `puts`-ийн жинхэнэ хаяг хараахан мэдэгдээгүй.
Үүний оронд **анх дуудагдах үед** dynamic linker (`ld.so`) хаягийг олж GOT-д бичнэ. Үүнийг **lazy binding** гэнэ.

gdb-ээр `puts@got`-ийн утгыг эхэнд болон эхний дуудлагын дараа харьцуулъя:

```ansi
[1;31mpwndbg>[0m x/gx 0x404000     [90m# puts@got, main эхлэхэд[0m
0x404000:    [34m0x0000000000401030[0m     [90m# PLT дотор буцаж заана (resolver stub)[0m
[1;31mpwndbg>[0m x/gx 0x404000     [90m# эхний puts() дуудлагын дараа[0m
0x404000:    [34m0x00007ffff7c8e860[0m     [90m# libc дахь puts-ийн жинхэнэ хаяг[0m
```

- **Эхэнд:** `puts@got` = `0x401030` — PLT доторх resolver stub руу заана. Энэ stub reloc-ийн дугаарыг `push`
  хийгээд dynamic linker-ийг дуудна.
- **Эхний дуудлагын дараа:** dynamic linker `puts`-ийг олж `0x7ffff7c8e860` (жинхэнэ хаяг) -ийг GOT-д **бичнэ**.
  Дараагийн бүх `puts` дуудлага шууд тийшээ үсэрнэ (resolver дахин ажиллахгүй).

:::note
`-z now` (эсвэл [Full RELRO](../protections/#relro)) -оор compile хийвэл бүх хаягийг **эхлэхэд шийдээд** GOT-ийг
түгждэг — lazy binding болон GOT бичих боломжгүй болно.
:::

## Pwn-д яагаад чухал

| Хэрэглээ | Тайлбар |
|----------|---------|
| **GOT overwrite** | GOT бичигдэх үед (Partial RELRO) `puts@got` зэргийг өөр функцийн (жишээ нь `system`) хаягаар дарж бичвэл дараагийн дуудлага тийшээ очно. [format string](../../userland/format-string/)-д ашигласан |
| **PLT-ээр функц дуудах** | `puts@plt` нь **тогтмол хаягтай** тул leak хийх, функц дуудахад тохиромжтой (жишээ нь `puts(puts@got)`-оор libc leak) |
| **RELRO-гийн зорилго** | [Full RELRO](../protections/#relro) нь GOT-ийг зөвхөн унших болгож GOT overwrite-ийг хаана |

pwndbg-ийн [`got`](../../tools/gdb/#got--got-ийн-төлөв) команд бүх GOT entry болон RELRO-гийн төлөвийг нэг дор харуулна.

## Эх сурвалж

- [ir0nstone — The GOT and PLT](https://ir0nstone.gitbook.io/notes/binexp/stack/the-got-and-plt)
- [CTF Wiki — Relocation (GOT/PLT)](https://ctf-wiki.org/executable/elf/structure/overview/)
