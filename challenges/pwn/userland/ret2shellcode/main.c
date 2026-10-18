#include <stdio.h>
#include <stdlib.h>

char *gets(char *s); // шинэ glibc-ийн header-аас gets() хасагдсан

void vuln(void) {
    char buf[256];
    printf("buf: %p\n", buf);
    puts("What is your name?");
    gets(buf);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    vuln();
    puts("Bye!");
    return 0;
}
