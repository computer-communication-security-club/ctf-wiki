---
title: Heap — chunk, bins, tcache
description: glibc malloc-ийн дотоод бүтэц — chunk, free хийсэн chunk-уудыг хадгалдаг bins (tcache, fastbin, unsorted, small, large), tcache хэрхэн ажилладаг, safe-linking.
---

Heap pwn эхлэхийн өмнө glibc-ийн `malloc`/`free` санах ойг хэрхэн удирддагийг ойлгох хэрэгтэй. Энэ хуудас
[Heap (tcache poisoning)](../../userland/tcache-poison/) бодлогод хэрэгтэй суурь ойлголтуудыг цуглуулав.

## Chunk

`malloc(n)` нь **chunk** буцаана — жижиг header + хэрэглэгчийн өгөгдөл. 64-bit дээр header нь:

```text
            +------------------+------------------+
chunk  ->   |  prev_size (8B)  |  size (8B)       |   <- header
            +------------------+------------------+
mem    ->   |  хэрэглэгчийн өгөгдөл ...            |   <- malloc энэ хаягийг буцаана
            +-------------------------------------+
```

- **size** — chunk-ийн нийт хэмжээ (header-тэй хамт, 16-д тэгшилсэн). Доод 3 бит нь **flag**:
  `PREV_INUSE` (A), `IS_MMAPPED` (M), `NON_MAIN_ARENA` (N).
- Хэмжээ үргэлж 16-ийн үржвэр тул `malloc(0x50)` → `0x60` chunk (`0x50` өгөгдөл + header зэрэг).
- Chunk **чөлөөтэй** үед нэмэлт талбарууд (`fd`, `bk`) өгөгдлийн хэсэгт хэвтэнэ — bins-ийн холбоос.

## Bins — free хийсэн chunk-ууд хаана очдог вэ

`free` хийсэн chunk-ийг дахин ашиглахаар **bins** гэдэг жагсаалтуудад хадгална. Хэмжээ болон төрлөөс
хамаарч өөр bin рүү очно:

| Bin | Бүтэц | Хэмжээ | Онцлог |
|-----|-------|--------|--------|
| **tcache** | нэг чиглэлт (fd), LIFO | ≤ 0x408 (bin бүр 7 chunk хүртэл) | thread бүрт тусдаа, хамгийн түрүүнд шалгагддаг |
| **fastbin** | нэг чиглэлт (fd), LIFO | ≤ 0x80 орчим | хурдан, consolidate хойшлуулдаг |
| **unsorted bin** | хоёр чиглэлт (fd/bk) | бүх хэмжээ (түр) | free хийсэн том chunk эхлээд энд ирнэ |
| **small bins** | хоёр чиглэлт, FIFO | < 0x400 | хэмжээ тус бүрт нэг |
| **large bins** | хоёр чиглэлт, эрэмбэлсэн | ≥ 0x400 | хэмжээний мужтай |

**`free` хийхэд:** эхлээд хэмжээ нь tcache-д багтах уу гэж хардаг — багтвал tcache руу. Үгүй бол fastbin
(жижиг) эсвэл unsorted bin руу. Хөрш chunk-ууд чөлөөтэй бол **consolidate** (нийлүүлж) том chunk болгоно.

**`malloc` хийхэд** дараалан хайна: **tcache → fastbin → unsorted → small/large → top chunk**. Аль нэгээс
тохирох хэмжээ олдвол тэндээс авна.

:::tip[libc leak яагаад unsorted bin-ээс гардаг вэ]
Том chunk (tcache/fastbin-д багтахгүй) free хийвэл **unsorted bin** руу орж, ганцаараа байвал `fd`/`bk` нь
libc доторх `main_arena`-г заана. Тэр заагчийг унших нь libc-ийн base-ийг олох сонгодог арга.
:::

## tcache (thread cache)

tcache бол орчин үеийн glibc (2.26+)-ийн хамгийн түрүүнд ашиглагддаг кэш. Thread бүрт `tcache_perthread_struct`
байх ба хэмжээ тус бүрт (**bin**) нэг чиглэлт жагсаалт хадгална:

```c
struct tcache_perthread_struct {
    uint16_t       counts[64];    // bin бүрт хэдэн chunk байгаа (max 7)
    tcache_entry  *entries[64];   // bin бүрийн ЖАГСААЛТЫН ТОЛГОЙ
};
```

Bin-ийн **индекс** нь chunk-ийн хэмжээгээр тодорхойлогдоно (`idx = (size - 0x20) / 0x10`): `0x20→0`, `0x30→1`,
… `0x60→4` гэх мэт. chunk **чөлөөтэй** үед эхний 8 байт нь `fd` (дараагийн chunk), дараагийн 8 байт нь `key`.

**`free(ptr)` юу хийдэг вэ:**

1. Хэмжээнээс bin индекс `i`-г бодно. tcache-д багтах (`counts[i] < 7`) бол:
2. `chunk->fd = entries[i]`  — одоогийн толгойг заана (safe-linking-оор нуугдана, доор).
3. `chunk->key = tcache`     — double-free илрүүлэгч.
4. `entries[i] = chunk`      — chunk ШИНЭ ТОЛГОЙ болно.
5. `counts[i]++`.

**`malloc(n)` юу хийдэг вэ:**

1. Хэмжээнээс bin индекс `i`. `entries[i] != NULL` бол:
2. `chunk = entries[i]`              — толгойг авна.
3. `entries[i] = chunk->fd`          — толгойг дараагийнх руу шилжүүлнэ (fd-г задалж).
4. `chunk->key = 0`                  — key-г цэвэрлэнэ.
5. `counts[i]--`, `chunk`-ийг буцаана.

Товчоор: **`free` → толгойд нэмнэ, `malloc` → толгойгоос авна** (LIFO). Жишээ — `0x60` bin:

![tcache 0x60 bin LIFO: ① free(A) → толгой нь A, A.fd=NULL; ② free(B) → толгой нь B, B.fd=A; ③ malloc нь толгой B-г буцаана, толгой A болно; ④ malloc нь A-г буцаана, жагсаалт хоосон. Хамгийн сүүлд free хийсэн нь хамгийн түрүүнд эргэж гарна.](../../../../assets/pwn/tcache-lifo.svg)

```text
эхлэл:            entries[4] -> NULL                         counts=0
free(A):          entries[4] -> [A] -> NULL                  counts=1   (A.fd=NULL)
free(B):          entries[4] -> [B] -> [A] -> NULL           counts=2   (B.fd=A)
malloc(0x50) -> B entries[4] -> [A] -> NULL                  counts=1   (B буцна)
malloc(0x50) -> A entries[4] -> NULL                         counts=0   (A буцна)
```

Хамгийн сүүлд free хийсэн нь (B) хамгийн түрүүнд эргэж гарч байгааг анзаараарай. Bin бүр **7** chunk хүрэхэд
дүүрч, дараагийн `free` нь fastbin/unsorted руу шилжинэ.

### Safe-linking (glibc 2.32+)

`fd` заагчийг шууд хадгалахгүй, **xor-оор нуудаг**:

```text
stored_fd = (chunk_хаяг >> 12)  XOR  дараагийн_chunk
```

Ингэснээр heap дээр санамсаргүйгээр унших зүйлээс fd задрахгүй. tcache-ийн fd-г хуурахын тулд (**poisoning**)
chunk байгаа **page-ийн хаягийг** (`>>12`) мэдэх шаардлагатай болгодог.

### Double-free хамгаалалт (key)

tcache-д байгаа chunk бүрт `key` талбар (chunk+8) тавьдаг. Нэг chunk-ийг дахин `free` хийхэд `key` таарвал
**double-free** гэж илрүүлж таслана. (`malloc` үед `key`-г NULL болгодог.)

## Цаашид

- [Heap (tcache poisoning)](../../userland/tcache-poison/) — энэ ойлголтууд дээр суурилсан бодлого: UAF → libc/heap
  leak → tcache poisoning → GOT дарж `system`.
- Нэр томъёо: [glossary](../../../glossary/) (chunk, tcache, UAF, double free, safe-linking, unsorted bin).

## Эх сурвалж

- [how2heap (shellphish)](https://github.com/shellphish/how2heap)
- [glibc malloc internals (sploitfun)](https://sploitfun.wordpress.com/2015/02/10/understanding-glibc-malloc/)
- [Safe-linking (Check Point)](https://research.checkpoint.com/2020/safe-linking-eliminating-a-20-year-old-malloc-exploit-primitive/)
