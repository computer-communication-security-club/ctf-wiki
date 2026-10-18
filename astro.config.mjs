import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

// ```ansi блокуудын өнгө: theme-ийн анхны terminal өнгөнүүдийн зарим нь (light дээр шар/цэнхэр,
// dark дээр саарал) дэвсгэртэйгээ хэт ойр тул уншигдахгүй байсан. Бүгд ≥4.5:1 contrast-тай.
const ansiColors = {
  dark: {
    Black: '#7f8796', Red: '#ff7373', Green: '#4ade80', Yellow: '#facc15',
    Blue: '#82aaff', Magenta: '#d68cf0', Cyan: '#22d3ee', White: '#d6deeb',
    BrightBlack: '#8f97a8', BrightRed: '#ff8f8f', BrightGreen: '#86efac', BrightYellow: '#fde047',
    BrightBlue: '#a5c3ff', BrightMagenta: '#e5a8f7', BrightCyan: '#67e8f9', BrightWhite: '#ffffff',
  },
  light: {
    Black: '#403f53', Red: '#c41a1a', Green: '#16794a', Yellow: '#8a5a00',
    Blue: '#1d5bbf', Magenta: '#a3237a', Cyan: '#00707a', White: '#5f636f',
    BrightBlack: '#666b78', BrightRed: '#b3261e', BrightGreen: '#1b6e3c', BrightYellow: '#7a5200',
    BrightBlue: '#2450a8', BrightMagenta: '#8f2a8f', BrightCyan: '#0f6670', BrightWhite: '#2b2b3a',
  },
};

export default defineConfig({
  site: 'https://ctf-wiki.sict-ccs.club',
  integrations: [
    starlight({
      title: 'CTF Wiki',
      favicon: '/favicon.ico',
      expressiveCode: {
        // terminal блокуудын дээрх 3 цэгтэй title bar-ыг арилгана
        defaultProps: {
          overridesByLang: {
            'ansi,console,sh,bash,shell,shellsession,powershell': { frame: 'code' },
          },
        },
        customizeTheme(theme) {
          for (const [name, color] of Object.entries(ansiColors[theme.type])) {
            theme.colors[`terminal.ansi${name}`] = color;
          }
        },
      },
      defaultLocale: 'root',
      locales: { root: { label: 'Монгол', lang: 'mn' } },
      social: [{ icon: 'github', label: 'GitHub', href: 'https://github.com/computer-communication-security-club/ctf-wiki' }],
      sidebar: [
        { label: 'CTF', items: [
          { label: 'Pwn', items: [
            // pwn/basics/*, pwn/tools/* хуудсууд sidebar-т байхгүй ч URL-аар нээгдэнэ
            { label: 'Userland', items: [
              { label: 'Stack Overflow', items: [
                { label: 'ret2text', slug: 'pwn/userland/ret2text' },
                { label: 'ret2shellcode', slug: 'pwn/userland/ret2shellcode' },
              ]},
              { label: 'Format string', items: [
                { label: 'Format string', slug: 'pwn/userland/format-string' },
              ]},
            ]},
          ]},
        ]},
      ],
    }),
  ],
});