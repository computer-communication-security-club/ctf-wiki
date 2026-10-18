---
title: CTF Wiki
description: Монгол хэл дээрх CTF болон мэдээллийн аюулгүй байдлын суралцах материал, өөрсдийн бодлогуудтай.
---

**Computer & Communication Security** клубын Монгол хэл дээрх CTF суралцах wiki. Сэдэв бүрийг жинхэнэ бодлого дээр
алхам алхмаар тайлбарлаж, бүтэн шийдлийг нь хамт өгнө. Бодлогын файлуудыг татаж аваад өөрийн машин дээр туршиж болно.

## Pwn

Доорх дарааллаар уншихыг зөвлөж байна — хуудас бүр өмнөхөө дэвсгэр мэдлэг болгон ашигладаг:

| Хуудас | Сэдэв |
|--------|-------|
| [ret2text](pwn/ret2text/) | Stack overflow-оор программ доторх бэлэн функц (`win`) руу үсрэх |
| [ret2shellcode](pwn/ret2shellcode/) | Өөрийн машин кодыг (shellcode) stack дээр байрлуулж ажиллуулах |
| [Format string](pwn/format-string/) | `printf(buf)` алдаагаар санах ой унших (libc leak), GOT дарж бичих |

Эхлэхийн өмнө суурь мэдлэг, хэрэгслийн хуудсууд тустай:
[Calling convention](pwn/basics/calling-convention/), [GOT ба PLT](pwn/basics/got-plt/), [Хамгаалалтууд](pwn/basics/protections/),
[pwndbg](pwn/tools/gdb/), [pwntools](pwn/tools/pwntools/), [Нэр томъёо](glossary/).

:::tip[Эхлээд өөрөө оролдоорой]
Хуудас бүрт **Spoiler** анхааруулгын дараа шийдэл эхэлнэ. Бодлогыг эхлээд өөрөө бодож үзээд, гацвал шийдлийг уншаарай.
:::
