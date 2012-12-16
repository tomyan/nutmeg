
#include "asm.h"
#include "versatilepb.h"
#include <stddef.h>

void bwputs(char *s) {
    while(*s) {
        while(*(UART0 + UARTFR) & UARTFR_TXFF);
        *UART0 = *s;
        s++;
    }
}

void first(void) {
	bwputs("In user mode 1\n");
	bwputs("In user mode 2\n");
    usleep(14);
	bwputs("In user mode 3\n");
    exit(0);
}

void task(void) {
	bwputs("In other task\n");
	exit(0);
}

struct task_t {
    
};

#define STACK_SIZE 256 /* Size of task stacks in words */
#define TASK_LIMIT 2   /* Max number of tasks we can handle */

unsigned int *init_task(unsigned int *stack, void (*start)(void)) {
	stack += STACK_SIZE - 16; /* End of stack, minus what we're about to push */
	stack[0] = 0x10; /* User mode, interrupts on */
	stack[1] = (unsigned int)start;
	return stack;
}

int main () {
    unsigned int stacks[TASK_LIMIT][STACK_SIZE];
    unsigned int *tasks[TASK_LIMIT];

    size_t task_count = 2;
    size_t current_task = 0;

    unsigned int ticker = 0;

    *TIMER0 = 1000000;
    *(TIMER0 + TIMER_CONTROL) = TIMER_EN | TIMER_ONESHOT | TIMER_32BIT;

    tasks[0] = init_task(stacks[0], &first);
    tasks[1] = init_task(stacks[1], &task);

    bwputs("starting...\n");

    while(1) {
        if(!*(TIMER0+TIMER_VALUE)) {
            bwputs("tick\n");
            *TIMER0 = 1000000;
            *(TIMER0 + TIMER_CONTROL) = TIMER_EN | TIMER_ONESHOT | TIMER_32BIT;
        }
        if (tasks[current_task] == NULL) {
            current_task++;
            if(current_task >= task_count) current_task = 0;
            continue;
        }
        tasks[current_task] = activate(tasks[current_task]);
        switch(tasks[current_task][2+7]) {
            case 0x1: /* exit */
                bwputs("got exit\n");
                tasks[current_task++] = NULL;
                if(current_task >= task_count) current_task = 0;
                continue;
            case 0x2: /* usleep */
                bwputs("got usleep\n");
                break;
			case -4: /* Timer 0 or 1 went off */
				if(*(TIMER0 + TIMER_MIS)) { /* Timer0 went off */
					*(TIMER0 + TIMER_INTCLR) = 1; /* Clear interrupt */
                    bwputs("tock\n");
				}

        }
        current_task++;
        if(current_task >= task_count) current_task = 0;

    }

    return;
}


