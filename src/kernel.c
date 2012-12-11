
#include "versatilepb.h"

void bwputs(char *s) {
    while(*s) {
        while(*(UART0 + UARTFR) & UARTFR_TXFF);
        *UART0 = *s;
        s++;
    }
}

int main () {
    bwputs("hello\n");

    while (1);
    return;
}


