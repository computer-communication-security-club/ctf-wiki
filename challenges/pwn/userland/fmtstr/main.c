#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void vuln(void) {
    char buf[128];
    puts("What is your name?");
    read(0, buf, sizeof(buf) - 1);
    printf(buf);        // format string vulnerability: хэрэглэгчийн оролт шууд format болж байна
    puts("Bye!");
    exit(0);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    vuln();
    return 0;
}
