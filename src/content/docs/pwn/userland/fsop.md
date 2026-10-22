---
title: FSOP
description: File Stream Oriented Programming — glibc-ийн FILE (_IO_FILE) бүтэц, _IO_list_all, exit → _IO_flush_all → _IO_OVERFLOW зам, vtable шалгалт ба түүнийг тойрох санаа.
pagefind: false
---

**FSOP** (File Stream Oriented Programming) нь glibc-ийн **`FILE` (`_IO_FILE`) бүтцийг дарж бичиж**,
`fflush`, `exit`, `fclose` зэрэг **файл урсгалын үйлдлүүдийг** код гүйцэтгэлд эргүүлэн ашиглах техникийн нэр.
GOT дарах ([tcache poisoning](../tcache-poison/)) болон hook (`__free_hook`) зэрэг замууд **Full RELRO** болон
орчин үеийн glibc-д (2.34+ hook-гүй) хаагдсан тул FSOP нь heap exploit-ийн гол **target** болсон.

:::note[Юуны өмнө]
- **[Heap](../../basics/heap/)**, **[tcache poisoning](../tcache-poison/)** — бичих primitive-ийг эндээс авдаг.
- Энэ хуудас нь онол. Бодит дайралтыг **[House of Apple 2](../house-of-apple-2/)** challenge дээр хийнэ.
:::

## FILE бүтэц ба vtable

glibc-д нээлттэй урсгал бүр нэг **`_IO_FILE_plus`** бүтцээр илэрхийлэгдэнэ. Эхэнд нь `_IO_FILE`
(буферийн заагчууд, `_flags`, `_fileno`, `_lock`, `_wide_data` гэх мэт), төгсгөлд нь **`vtable`** —
урсгалын үйлдлүүдийн **функцийн заагчдын хүснэгт** байрлана:

| Offset | Talbar | Үүрэг |
|--------|--------|-------|
| `0x00` | `_flags` | Урсгалын төлөв; эхний 8 байт тул дараа нь `rdi` болж болзошгүй |
| `0x20` | `_IO_write_base` | Бичих буферийн эхлэл |
| `0x28` | `_IO_write_ptr` | Бичих заагч (`> _base` бол "цэвэрлэх өгөгдөл байна") |
| `0x88` | `_lock` | Урсгалын түгжээ (бичигдэх хүчинтэй хаяг байх ёстой) |
| `0xa0` | `_wide_data` | Өргөн (wide) тэмдэгтийн урсгалын туслах бүтэц |
| `0xd8` | `vtable` | `_IO_jump_t` — `_IO_OVERFLOW`, `_IO_UNDERFLOW` … заагчууд |

`stdin` / `stdout` / `stderr` нь libc дотор **статик** (`_IO_2_1_stdout_` гэх мэт) бөгөөд бүгд
**`_IO_list_all`** гэдэг нэг чиглэлт жагсаалтаар (`_chain`-ээр) холбогдоно.

## exit → _IO_flush_all → _IO_OVERFLOW

Программ `exit()` дуудахад (эсвэл `main` буцахад) libc нь `_IO_cleanup` → **`_IO_flush_all`**-ийг
ажиллуулж, бүх нээлттэй урсгалын буферийг гаргадаг. `_IO_flush_all` нь `_IO_list_all` жагсаалтыг гүйж,
урсгал бүрт бичих өгөгдөл үлдсэн эсэхийг шалгаад тийм бол **`_IO_OVERFLOW(fp)`**-ийг дууддаг:

```c
// _IO_flush_all-ийн гол логик (хялбарчилсан)
for (fp = _IO_list_all; fp; fp = fp->_chain)
    if (fp->_IO_write_ptr > fp->_IO_write_base)   // цэвэрлэх өгөгдөлтэй бол
        _IO_OVERFLOW(fp, EOF);                    // = fp->vtable->__overflow(fp)
```

Хэрэв бид `_IO_list_all` дахь нэг FILE-ийг (жишээ нь `_IO_2_1_stdout_`) бүрэн хянавал:

1. `_IO_write_ptr > _IO_write_base` болгоно → `_IO_OVERFLOW` **заавал дуудагдана**.
2. `vtable`-ийг хянавал `_IO_OVERFLOW` (= `vtable+0x18`) **ямар функц дуудахыг** бид шийднэ.

Энэ нь `exit`-ийг дурын функц дуудлага болгож хувиргах **контрол урсгалын булаалт**.

## vtable шалгалт ба түүнийг тойрох

Хамгийн энгийн FSOP — `vtable`-ийг хуурамч хүснэгт рүү чиглүүлж `__overflow` слотыг `system` болгох.
Гэвч орчин үеийн glibc энэ яг тохиолдлыг **`_IO_vtable_check`**-ээр барьдаг: дуудлагын өмнө `vtable` нь
libc доторх **`__libc_IO_vtables`** мужид (зөвшөөрөгдсөн хүснэгтүүд) байхыг шаарддаг, эс бол `abort`.

**House of Apple 2** энэ шалгалтыг тойрдог гол санаа:

- `vtable`-ийг **хэвийн, зөвшөөрөгдсөн** `_IO_wfile_jumps` руу чиглүүлнэ (шалгалт давна).
- Тэр замаар `_IO_OVERFLOW` → `_IO_wfile_overflow` → `_IO_wdoallocbuf` →
  **`fp->_wide_data->_wide_vtable->__doallocate(fp)`** гэсэн гинж дуудагдана.
- `_wide_data`-г бид бүрэн хянадаг тул сүүлийн заагчийг `system`, `fp` эхний 8 байтыг (`_flags`) `"sh"`
  болговол → `system("sh")`.

:::note[FSOP-ийн гэр бүл]
`_wide_data` руу шилждэг House of Apple 2-оос гадна `_IO_wfile_seekoff` (House of Emma, `_IO_cookie`),
`_IO_str_overflow` зэрэг олон хувилбар бий. Бүгдийн нийтлэг санаа нэг: **зөвшөөрөгдсөн vtable** сонгож
`_IO_vtable_check`-ийг давж, дараа нь хянагдах заагч руу чиглүүлэх.
:::

## Хэрэгтэй зүйлс

| Шаардлага | Яагаад |
|-----------|--------|
| Дурын хаягт **бичих** primitive | `_IO_2_1_stdout_`-г дарахад хэрэгтэй (энд [tcache poisoning](../tcache-poison/)) |
| **libc leak** | `_IO_2_1_stdout_`, `_IO_wfile_jumps`, `system`-ийн хаягийг мэдэх |
| **heap leak** | Хуурамч `_wide_data`-г heap дээр байрлуулж хаягийг нь өгөх (safe-linking-д ч хэрэгтэй) |

Эдгээр гурвыг бүгдийг нь нэг UAF challenge дээр цуглуулж, бодит дайралтыг
**[House of Apple 2](../house-of-apple-2/)** хуудсан дээр хийнэ.

## Эх сурвалж

- [FSOP ба `_IO_FILE` дотоод бүтэц (Angelboy / HITCON)](https://blog.angelboy.tw/2021/09/hitcon-ctf-2021-fsop.html)
- [glibc `_IO_flush_all` (bits/libio)](https://elixir.bootlin.com/glibc/latest/source/libio/genops.c)
- [how2heap (shellphish)](https://github.com/shellphish/how2heap)
