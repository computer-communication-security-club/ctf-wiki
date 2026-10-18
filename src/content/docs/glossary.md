---
title: Нэр томъёо
---

| English | Монгол |
|---------|--------|
| payload | Халдлагын өгөгдөл |
| offset  | Байтын зай |
| calling convention | Функц дуудах дүрэм |
| caller | Дуудагч функц |
| callee | Дуудагдсан функц |
| stack frame | Функцийн stack дээрх хэсэг |
| shellcode | Payload дотор авчирдаг жижиг машин код (ихэвчлэн shell нээдэг) |
| leak | Санах ойн мэдээллийг (хаяг гэх мэт) гадагш задруулах |
| NOP sled | Shellcode-ийн өмнөх `nop` зааврын дараалал, хаягийн алдааг "уучилдаг" гулгуур |
| format string | `printf`-ийн эхний аргумент — `%` орлуулагчтай мөр |
| specifier | Format мөр доторх `%`-тэй орлуулагч (`%d`, `%p`, `%n` гэх мэт) |
| GOT | Global Offset Table — libc функцүүдийн хаягийг хадгалдаг хүснэгт |
| PLT | Procedure Linkage Table — GOT-оос хаягийг уншиж функц дууддаг код |
| ROP | Return-Oriented Programming — байгаа кодын хэсгүүдийг (gadget) `ret`-ээр холбож ажиллуулах |
| gadget | `ret`-ээр төгсдөг богино кодын хэсэг (жишээ нь `pop rdi; ret`) |
