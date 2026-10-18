---
title: ropper
description: Binary/библиотекоос ROP gadget хайх хэрэгсэл — pop rdi; ret зэрэг gadget, тэмдэгт мөр, opcode хайх, автомат ROP chain.
---

**[ropper](https://github.com/sashs/Ropper)** нь binary болон библиотекоос **ROP gadget** (жишээ нь
`pop rdi; ret`) хайдаг хэрэгсэл. [ROP](../../basics/calling-convention/) chain барихад хэрэгтэй жижиг кодын
хэсгүүдийг олоход ашиглана.

```console
$ pip install ropper
```

Binary доторх **бүх** gadget:

```console
$ ropper -f ./ret2text
```

`--search`-ээр тодорхой gadget хайна (`;` нь зааглагч, `?` нь нэг тэмдэгтийн wildcard):

```console
$ ropper -f ./ret2text --search 'ret'
0x000000000040101a: ret;

$ ropper -f ./ret2text --search 'pop rbp; ret'
0x000000000040117d: pop rbp; ret;

$ ropper -f ./ret2text --search 'pop r??; ret'    # аль ч регистрийн pop
```

Хаягууд нь [ret2text](../../userland/ret2text/) бодлогын binary дээрх бодит утгууд — pwntools/ROPgadget-ийн
олдогтой яг ижил (gadget нь binary-ийн шинж чанар, хэрэгслээс үл хамаарна).

| Команд | Үйлдэл |
|--------|--------|
| `ropper -f <bin>` | бүх gadget жагсаана |
| `ropper -f <bin> --search '<pattern>'` | gadget хайх (`?` wildcard, `;` зааг) |
| `ropper -f <bin> --string '<str>'` | тэмдэгт мөр хайх |
| `ropper -f <bin> --opcode <hex>` | opcode-оор хайх (жишээ `0f05` = `syscall`) |
| `ropper -f <bin> --chain execve` | автомат ROP chain угсрах |

:::tip
pwntools-ийн [`ROP(exe)`](../pwntools/) класс дотроо gadget хайдаг (ROPgadget ашиглана) тул энгийн
зүйлд pwntools-оор хангалттай. Нарийн/том хайлтад ropper (эсвэл ROPgadget)-ийн CLI тохиромжтой.
:::

## Эх сурвалж

- [ropper (GitHub)](https://github.com/sashs/Ropper)
- [ROPgadget](https://github.com/JonathanSalwan/ROPgadget) — ижил зориулалттай өөр хэрэгсэл
