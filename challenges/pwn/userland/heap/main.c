// Heap notes — UAF (use-after-free) бүхий энгийн цэсний программ.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX 16
static char  *notes[MAX];
static size_t sizes[MAX];

static int readint(void) { char b[16] = {0}; read(0, b, 15); return atoi(b); }

static void add(void) {
    printf("idx: ");  int i = readint();      if (i < 0 || i >= MAX) return;
    printf("size: "); size_t s = readint();   if (s == 0 || s > 0x500) return;
    notes[i] = malloc(s); sizes[i] = s;
    printf("data: "); read(0, notes[i], s);
}
static void edit(void) {
    printf("idx: "); int i = readint(); if (i < 0 || i >= MAX || !notes[i]) return;
    printf("data: "); read(0, notes[i], sizes[i]);   // UAF: freed chunk-д ч бичнэ
}
static void show(void) {
    printf("idx: "); int i = readint(); if (i < 0 || i >= MAX || !notes[i]) return;
    write(1, notes[i], sizes[i]);                     // UAF: freed chunk-ийг ч уншина
}
static void del(void) {
    printf("idx: "); int i = readint(); if (i < 0 || i >= MAX) return;
    free(notes[i]);                                   // BUG: pointer-ийг NULL болгодоггүй
}
int main(void) {
    setvbuf(stdout, 0, _IONBF, 0); setvbuf(stdin, 0, _IONBF, 0);
    while (1) {
        puts("\n1.add 2.edit 3.show 4.delete 5.exit");
        printf("> ");
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
