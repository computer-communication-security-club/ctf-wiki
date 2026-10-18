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
| chunk | `malloc` өгдөг нэгж — header + хэрэглэгчийн өгөгдөл |
| tcache | Thread cache — чөлөөлсөн жижиг chunk-уудын нэг чиглэлт жагсаалт |
| use-after-free (UAF) | Чөлөөлсөн санах ойг үргэлжлүүлэн ашиглах алдаа |
| double free | Нэг chunk-ийг хоёр удаа `free` хийх алдаа |
| tcache poisoning | tcache-ийн `fd` заагчийг хуурч дурын хаягт chunk гаргах |
| unsorted bin | Чөлөөлсөн том chunk түр ордог жагсаалт — libc leak-ийн эх сурвалж |
| safe-linking | tcache/fastbin-ийн `fd`-г xor-оор нуудаг хамгаалалт (glibc 2.32+) |
| ret2usr | Kernel exploit-д return address-ийг userland код руу чиглүүлж ring-0-д ажиллуулах |
| commit_creds | Процессын эрхийг (credential) солих kernel функц — privesc-д ашиглана |
| KASLR | Kernel ASLR — kernel-ийн хаягийг санамсаргүй болгоно |
| SMEP | Supervisor Mode Execution Prevention — ring-0-д userland код ажиллуулахыг хориглоно |
| SMAP | Supervisor Mode Access Prevention — ring-0-оос userland өгөгдөлд хандахыг хориглоно |
| KPTI | Kernel Page-Table Isolation — kernel/userland хуудсыг тусгаарлана (Meltdown) |
| ring 0 / ring 3 | CPU эрхийн түвшин — ring 0 (kernel), ring 3 (userland) |
| syscall | Userland-аас kernel рүү ханддаг цорын ганц интерфейс |
| LKM | Loadable Kernel Module (`.ko`) — ачаалж болдог kernel драйвер |
| ioctl | Төхөөрөмжтэй command/өгөгдөл солилцдог syscall |
| hypervisor | Виртуал машинуудыг ажиллуулж, тусгаарладаг давхарга (жишээ QEMU/KVM) |
| VM exit | Guest-ээс hypervisor руу удирдлага шилжих үйл явдал (жишээ MMIO хандалт) |
| VM escape | Guest VM-ээс гарч доорх hypervisor (host) процесс руу халдах |
| MMIO | Memory-Mapped I/O — төхөөрөмжийн register-ийг санах ойн хаягаар унших/бичих |
| PCI BAR | Base Address Register — PCI төхөөрөмжийн MMIO/PMIO мужийн хаяг |
| DMA | Direct Memory Access — төхөөрөмж санах ойг CPU-гүйгээр шууд уншиж/бичих |
