import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

export default defineConfig({
  site: 'https://computer-communication-security-club.github.io',
  base: '/ctf-wiki',
  integrations: [
    starlight({
      title: 'CTF Wiki',
      defaultLocale: 'root',
      locales: { root: { label: 'Монгол', lang: 'mn' } },
      social: [{ icon: 'github', label: 'GitHub', href: 'https://github.com/computer-communication-security-club/ctf-wiki' }],
      sidebar: [
        { label: 'Эхлэл', items: [
          { label: 'Танилцуулга', slug: 'index' },
          { label: 'Нэр томъёо', slug: 'glossary' },
          { label: 'Хувь нэмэр', slug: 'contribute' },
        ]},
        { label: 'CTF', items: [
          { label: 'Pwn: Stack Overflow', items: [
            { label: 'ret2text', slug: 'pwn/ret2text' },
          ]},
        ]},
      ],
    }),
  ],
});