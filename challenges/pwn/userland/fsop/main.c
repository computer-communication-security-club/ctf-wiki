// FSOP — UAF бүхий notes цэс. Full RELRO + PIE: GOT зөвхөн уншигдах, hook-ууд байхгүй (glibc 2.34+).
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX 16
static char  *notes[MAX];
static size_t sizes[MAX];

static void out(const char *s) { write(1, s, strlen(s)); }
static long readint(void) { char b[16] = {0}; read(0, b, 15); return atol(b); }

static void add(void) {
    out("idx: ");  long i = readint();   if (i < 0 || i >= MAX) return;
    out("size: "); size_t s = readint(); if (s == 0 || s > 0x500) return;
    notes[i] = malloc(s); sizes[i] = s;
    out("data: "); read(0, notes[i], s);
}
static void edit(void) {
    out("idx: "); long i = readint(); if (i < 0 || i >= MAX || !notes[i]) return;
    out("data: "); read(0, notes[i], sizes[i]);
}
static void show(void) {
    out("idx: "); long i = readint(); if (i < 0 || i >= MAX || !notes[i]) return;
    write(1, notes[i], sizes[i]);
}
static void del(void) {
    out("idx: "); long i = readint(); if (i < 0 || i >= MAX) return;
    free(notes[i]);                     // BUG: notes[i]-г NULL болгодоггүй -> use-after-free
}
int main(void) {
    while (1) {
        out("\n1.add 2.edit 3.show 4.delete 5.exit\n> ");
        switch (readint()) {
        case 1: add();  break;
        case 2: edit(); break;
        case 3: show(); break;
        case 4: del();  break;
        case 5: exit(0);
        default: break;
        }
    }
}
