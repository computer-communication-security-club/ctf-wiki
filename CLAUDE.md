# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A Mongolian-language CTF learning wiki built with Astro + Starlight, deployed to GitHub Pages at
`https://ctf-wiki.sict-ccs.club/` (served at the domain root). There is no application code, test
suite, or linter: the "code" is Markdown pages plus the challenge binaries they walk through.

## Commands

Requires Node.js ≥ 22.22. `astro` is not on PATH; call it through `npx`.

- `npm install`
- `npx astro dev --background` starts the dev server at `http://localhost:4321/`. Manage it with `npx astro dev stop | status | logs`.
- `npm run build`: production build to `dist/`. This is the only check CI runs, so use it to validate changes (broken frontmatter or sidebar slugs fail here).
- `npm run build && npm run preview`: search (Pagefind) only works on a production build.

After changing `expressiveCode`/markdown config in `astro.config.mjs`, the rendered content is cached. Clear it or the change won't show: `npx astro dev stop; rm -rf .astro node_modules/.astro; npx astro dev --background`.

## Architecture

- **Pages**: `src/content/docs/**.md`, loaded by Starlight's `docsLoader`. The URL is the file path (e.g. `pwn/basics/protections.md` becomes `/pwn/basics/protections/`).
- **Sidebar is manual and partial**: the `sidebar` array in `astro.config.mjs` lists only challenge walkthroughs, so a new walkthrough needs an entry there. `pwn/basics/*` and `pwn/tools/*` are deliberately left out and are reached through links from other pages. Sidebar labels and groups are in Mongolian.
- **Experimental topics**: the Kernel and Hypervisor groups sit inside `...(hideExperimental ? [] : [...])`, so they show in `astro dev` but not in the production sidebar. Their pages still build and are reachable by URL; they also set `pagefind: false` and carry a `:::caution[Туршилтын хуудас]`. Publishing one means moving its group out of that spread and removing `pagefind: false` (see how `tcache-poison` was published).
- **Single locale**: `root` locale is Mongolian (`lang: 'mn'`). Starlight UI strings are translated in `src/content/i18n/mn.json`.
- **Terminal colors**: `astro.config.mjs` overrides expressive-code's ANSI palette per theme so `ansi` blocks stay readable (contrast ≥ 4.5:1) in light and dark mode, and removes the window frame for shell/ansi languages. Pages draw stack diagrams and colored output as ` ```ansi ` blocks with raw escape sequences (`[1;31m` … `[0m`).
- **Diagrams**: SVGs in `src/assets/pwn/`, embedded as Markdown images with a long Mongolian alt text that describes the diagram. Image paths are file-relative (from `pwn/basics/got-plt.md`: `../../../../assets/pwn/got-plt-flow.svg`), unlike page links (see below).
- **CI**: `deploy.yml` builds and publishes the site on every push to `main`. `challenges.yml` runs only when `challenges/**` changes: it builds every `Dockerfile` found under `challenges/` and pushes `ghcr.io/<owner>/<top-level dir>-<challenge dir>:latest` (`challenges/pwn/userland/fmtstr/` becomes `pwn-fmtstr`). The image name comes from the directory, not the page slug (`fmtstr` is walked through in `format-string.md`).

## Challenges

They live in `challenges/pwn/<userland|kernel|hypervisor>/<name>/`, and each layout is different:

- **Userland**: `main.c`, the prebuilt binary, `flag.txt`, a deploy-only `Dockerfile` (ubuntu:24.04, socat on port 1337, user `ctf`), and a `Makefile` (`make run` / `make stop`, then `nc localhost 1337`). Copy `ret2text` as the template. The binary name can differ from the directory (`tcache-poison/heap`). Challenges that pin a glibc (`fmtstr`, `tcache-poison`, `fsop`) also commit `libc.so.6` and `ld-linux-x86-64.so.2`. Their binary has RUNPATH `$ORIGIN` and interpreter `./ld-linux-x86-64.so.2`, so it only runs from its own directory, and the Dockerfile must copy all three files.
- **Kernel** (`ret2usr`): `vuln.c` module source and a `Makefile` that builds it against `KDIR` (no `run`/`stop` targets), plus a prebuilt `bzImage` and `rootfs.cpio.gz` (the module and flag live inside the initramfs, so there's no `flag.txt`). `./run.sh [exploit]` boots it in QEMU locally and packs the exploit into the rootfs as `/exploit`. `start.sh` is what socat runs for each connection in Docker.
- **Hypervisor** (`vm-escape`): `edu.c` replaces QEMU 9.1.2's `hw/misc/edu.c`. `./build.sh` downloads QEMU and builds the vulnerable `qemu-system-x86_64` plus `pc-bios/`, both gitignored, and `run.sh [exploit]` needs them. The Dockerfile rebuilds QEMU in a multi-stage build. The flag is `/flag` on the host, not in the guest.

The repo is public, so every `flag.txt` is visible; these are practice flags only.

## Content conventions

- Write prose in Mongolian. Keep technical terms in English (`return address`, `offset`, `payload`, tool names). When a new term comes up, add it to the English → Mongolian table in `src/content/docs/glossary.md`.
- Frontmatter needs `title` and a one-sentence `description`.
- Challenge pages follow one outline: `:::note[Юуны өмнө]` (prerequisites), `## Санаа`, `## Бодлого` (file table linking to GitHub, then `docker run --rm -p 1337:1337 ghcr.io/computer-communication-security-club/pwn-<name>`; pages no longer show a `make run` block), `:::caution[Spoiler]`, `## Бодолт` with numbered `### N.` steps ending in the exploit, `## Хамгаалалт`, `## Эх сурвалж`.
- Exploit scripts live only inside the page's Markdown, never in `challenges/`. They're based on `pwn template` and run as `./exploit.py LOCAL [GDB]` or `./exploit.py HOST=… PORT=…`.
- Addresses and offsets shown on a page must match the committed prebuilt binary, so if you rebuild the binary, update the page.
- Internal links are relative and end in a trailing slash. Because each page is served as a directory, links need one more `../` than the file-relative path would (from `pwn/userland/ret2text.md`, the gdb page is `../../tools/gdb/`; from `pwn/basics/protections.md`, ret2text is `../../userland/ret2text/`). Links to challenge files point at the GitHub repo URLs (`.../tree/main/challenges/...`, `.../raw/main/...` for binaries).
- Use Starlight asides (`:::note`, `:::tip`, `:::caution`, with an optional Mongolian title in `[...]`).
- Commit messages are short English imperative sentences (e.g. "Add calling convention basics page").
