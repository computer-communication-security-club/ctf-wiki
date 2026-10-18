# CTF Wiki

Монгол хэл дээрх CTF болон мэдээллийн аюулгүй байдлын суралцах материал.
[Astro Starlight](https://starlight.astro.build) дээр бүтээсэн.

🌐 **Сайт:** https://ctf-wiki.sict-ccs.club/

## Бүтэц

```
.
├── challenges/                 # Бодлогын файлууд (binary, Dockerfile)
│   └── pwn/
│       ├── userland/           # Процессын санах ой (ret2text, fmtstr...)
│       │   └── ret2text/
│       │       ├── main.c      # Эх код
│       │       ├── ret2text    # Бэлэн binary
│       │       ├── flag.txt
│       │       ├── Dockerfile  # Зөвхөн deploy (socat, порт 1337)
│       │       └── Makefile    # make run / make stop
│       └── kernel/             # Kernel pwn (QEMU: bzImage, rootfs, run.sh)
│           └── ret2usr/
├── src/
│   ├── content/
│   │   ├── docs/               # Wiki хуудсууд (.md)
│   │   │   └── pwn/userland/ret2text.md
│   │   └── i18n/mn.json        # Сайтын UI-ийн Монгол орчуулга
│   └── content.config.ts
└── astro.config.mjs            # Sidebar, домэйн (site)
```

## Локалд ажиллуулах

**Node.js 22.22 буюу түүнээс дээш** хувилбар шаардлагатай.

```sh
npm install
npm run dev        # http://localhost:4321/
```

| Команд | Үйлдэл |
|--------|--------|
| `npm run dev` | Dev server асаана |
| `npm run build` | `./dist/` руу production build хийнэ |
| `npm run preview` | Build хийсэн сайтыг локалд үзнэ |

> Хайлт (search) зөвхөн production build дээр ажиллана. Шалгахдаа `npm run build && npm run preview` ашигла.

## Шинэ хуудас нэмэх

1. `src/content/docs/<ангилал>/<сэдэв>.md` файл үүсгэ:

   ```md
   ---
   title: ret2text
   description: Нэг өгүүлбэр тайлбар
   ---
   ```

2. `astro.config.mjs`-ийн `sidebar` хэсэгт нэг мөр нэм:

   ```js
   { label: 'ret2text', slug: 'pwn/userland/ret2text' },
   ```

3. Шинэ нэр томъёо гарвал `src/content/docs/glossary.md`-д нэм.

Exploit скрипт зөвхөн хуудасны markdown дотор байна. `challenges/` хавтсанд скрипт оруулахгүй.

## Шинэ бодлого нэмэх

`challenges/<ангилал>/<нэр>/` хавтас үүсгээд дараах файлуудыг оруул:

| Файл | Тайлбар |
|------|---------|
| `main.c` | Эх код |
| `<нэр>` | Бэлэн binary — хуудсан дээрх хаягууд үүнтэй таарах ёстой |
| `flag.txt` | Flag |
| `Dockerfile` | Зөвхөн deploy: binary-г `socat`-аар порт `1337` дээр ажиллуулна |
| `Makefile` | `make run`, `make stop` |

`ret2text`-ийн файлуудыг загвар болгон хуул.

```sh
cd challenges/pwn/userland/ret2text
make run           # localhost:1337
nc localhost 1337
make stop
```

> Repo нээлттэй тул `flag.txt` хүн бүрт харагдана. Дасгалд зориулсан бодлогод л ашигла.
