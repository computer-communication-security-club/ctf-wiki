---
title: pwninit
description: pwn бодлого тохируулах автомат хэрэгсэл — тохирох ld-linux татах, binary-г өгсөн libc/ld рүү patchelf хийх, exploit template үүсгэх.
---

**[pwninit](https://github.com/io12/pwninit)** нь pwn бодлого эхлүүлэхэд **гар ажлыг автоматжуулдаг**. Challenge-ийн
binary (болон өгсөн `libc`)-ийг хавтаст тавиад ажиллуулахад:

1. тухайн `libc`-д **тохирох loader (`ld-linux`)**-ийг татаж,
2. binary-г өгсөн `libc`/`ld` руу **patchelf** хийж (`chall_patched`),
3. exploit-ийн **template** (`solve.py`) үүсгэдэг.

Remote сервер тодорхой `libc` ашигладаг үед offset таарах ёстой — pwninit үүнийг амар болгоно.

```console
$ cargo install pwninit        # эсвэл GitHub-аас release binary татна
```

Binary болон libc-ийг нэг хавтаст тавиад жагсаалгүй ажиллуулна:

```console
$ ls
chall  libc.so.6
$ pwninit
# -> libc.so.6-д тохирох ld-linux-г татна
# -> chall-ийг patchelf хийж chall_patched гаргана
# -> solve.py template үүсгэнэ
$ ls
chall  chall_patched  ld-linux-x86-64.so.2  libc.so.6  solve.py
```

`chall_patched` нь өгсөн `libc.so.6`-г ашиглан ажиллах тул локал offset-ууд remote-той таарна. Энэ wiki-д бид
үүнийг гараар хийсэн:

```console
$ patchelf --set-interpreter ./ld-linux-x86-64.so.2 --set-rpath '$ORIGIN' ./chall
```

pwninit үүнийг (+ loader татах, template бичих) нэг команд болгож хураангуйлна.

:::tip
Үүсгэсэн `solve.py` нь [pwn template](../pwntools/#pwn-template)-тэй төстэй бэлдэц — `exe`, `libc`-ийн замыг
аль хэдийн зөв тохируулсан байдаг.
:::

## Эх сурвалж

- [pwninit (GitHub)](https://github.com/io12/pwninit)
- [patchelf](https://github.com/NixOS/patchelf) — pwninit дотроо ашигладаг
