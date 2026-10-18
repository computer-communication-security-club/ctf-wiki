# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A Mongolian-language CTF learning wiki built with Astro + Starlight, deployed to GitHub Pages at
`https://ctf-wiki.sict-ccs.club/`. There is no application code, test
suite, or linter: the "code" is Markdown pages plus the challenge binaries they walk through.

## Commands

Requires Node.js ≥ 22.22.

- `npm install`
- `astro dev --background` starts the dev server at `http://localhost:4321/`. Manage it with `astro dev stop | status | logs`.
- `npm run build`: production build to `dist/`. This is the only check CI runs, so use it to validate changes (broken frontmatter or sidebar slugs fail here).
- `npm run build && npm run preview`: search (Pagefind) only works on a production build.
- `cd challenges/pwn/<name> && make run` / `make stop`: run a challenge locally on `nc localhost 1337` (Docker).

After changing `expressiveCode`/markdown config in `astro.config.mjs`, the rendered content is cached. Clear it or the change won't show: `astro dev stop; rm -rf .astro node_modules/.astro; astro dev --background`.

## Architecture

- **Pages**: `src/content/docs/**.md`, loaded by Starlight's `docsLoader`. The URL is the file path (e.g. `pwn/basics/protections.md` becomes `/ctf-wiki/pwn/basics/protections/`).
- **Sidebar is manual**: every new page needs an entry in the `sidebar` array in `astro.config.mjs`, or it won't show up in navigation. Sidebar labels and groups are in Mongolian.
- **Single locale**: `root` locale is Mongolian (`lang: 'mn'`). Starlight UI strings are translated in `src/content/i18n/mn.json`.
- **Terminal colors**: `astro.config.mjs` overrides expressive-code's ANSI palette per theme so `ansi` blocks stay readable (contrast ≥ 4.5:1) in light and dark mode, and removes the window frame for shell/ansi languages. Pages draw stack diagrams and colored output as ` ```ansi ` blocks with raw escape sequences (`[1;31m` … `[0m`).
- **Challenges**: `challenges/<category>/<name>/` holds `main.c`, the prebuilt binary, `flag.txt`, a deploy-only `Dockerfile` (socat on port 1337, ubuntu:24.04, user `ctf`), and a `Makefile`. Copy `ret2text` as the template. On push to `main`, `.github/workflows/challenges.yml` builds every `challenges/*/*/Dockerfile` and pushes `ghcr.io/<owner>/<category>-<name>:latest`. `deploy.yml` builds and publishes the site.

## Content conventions

- Write prose in Mongolian. Keep technical terms in English (`return address`, `offset`, `payload`, tool names). When a new term comes up, add it to the English → Mongolian table in `src/content/docs/glossary.md`.
- Frontmatter needs `title` and a one-sentence `description`.
- Exploit scripts live only inside the page's Markdown, never in `challenges/`.
- Addresses and offsets shown on a page must match the committed prebuilt binary, so if you rebuild the binary, update the page.
- Internal links are relative and end in a trailing slash. Because each page is served as a directory, links need one more `../` than the file-relative path would (from `pwn/ret2text.md`, the gdb page is `../tools/gdb/`; from `pwn/basics/protections.md`, ret2text is `../../ret2text/`). Links to challenge files point at the GitHub repo URLs (`.../tree/main/challenges/...`, `.../raw/main/...` for binaries).
- Use Starlight asides (`:::note`, `:::tip`, `:::caution`, with an optional Mongolian title in `[...]`).
- Commit messages are short English imperative sentences (e.g. "Add calling convention basics page").
