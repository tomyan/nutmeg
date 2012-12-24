
#include "asm.h"
#include "versatilepb.h"
#include <stddef.h>
#include "dlist.h"

void bwputs(char *s) {
    while(*s) {
        while(*(UART0 + UARTFR) & UARTFR_TXFF);
        *UART0 = *s;
        s++;
    }
}

#define ASSERT_TO_STRING_INNER(x) #x
#define ASSERT_TO_STRING(x) ASSERT_TO_STRING_INNER(x)

/* TODO: provide flag to turn assertions off */
#define assert(condition) \
    if (!(condition)) { \
        bwputs("kernel assertion failed: " #condition ", at " __FILE__ " line " ASSERT_TO_STRING(__LINE__) "\n"); \
        while (1); \
    }

static void pointer_to_hex (const void* input, char* result[11]) {
    unsigned int temp;
    unsigned int i;
    (*result)[0] = '0';
    (*result)[1] = 'x';
    for (i = 0; i < 8; i++) {
        temp = ((const unsigned int)input & (0xF << (4 * (7 - i)))) >> (4 * (7 - i));
        assert(temp < 16);
        if (temp < 10) {
            (*result)[i + 2] = '0' + temp;
        }
        else {
            (*result)[i + 2] = 'a' + temp - 10;
        }
    }
    (*result)[10] = '\0';
}
#define debugp_decl() \
    char* paddr[11];

#define debugp(desc, p) \
    bwputs("DEBUG: " desc " "); \
    pointer_to_hex(p, paddr); \
    bwputs(*paddr); \
    bwputs("\n"); \


void first_task(void) {
    bwputs("first task: started\n");
    usleep(10 * 1000 * 1000);
    bwputs("first task: after usleep(10s) - exiting\n");
    exit(0);
}

void second_task(void) {
    bwputs("second task: started\n");
    usleep(5 * 1000 * 1000);
    bwputs("second task: after usleep(5s) - exiting\n");
    exit(0);
}

void idle (void) {
    while (1);
}

#define TASK_STACK_SIZE 256 /* Size of task stacks in words */
#define TASK_EXITED_STATUS 0
#define TASK_RUNNING_STATUS 1
#define TASK_SLEEPING_STATUS 2

struct task_t {
    unsigned int stack[TASK_STACK_SIZE];
    unsigned int *current;
    unsigned int status;
    unsigned int sleep_until;
    dlist_fields(struct task_t)
};

void task_t_init (struct task_t* self, void (*start)(void)) {
    self->status = TASK_RUNNING_STATUS;
    /* set current position to the end of the stack, minus what we're about to push */
    self->current = self->stack + TASK_STACK_SIZE - 16;
    /* User mode, interupts on */
    self->current[0] = 0x10;
    self->current[1] = (unsigned int)start;
    dlist_init();
}

static __inline__ int task_t_activate (struct task_t* self) {
    self->current = activate(self->current);
    /* TODO: replace magic numbers with meaningful constants */
    return self->current[ 2 + 7 ];
}

#define SCHEDULAR_TASK_LIMIT 2   /* Max number of tasks we can handle */

struct schedular_t {
    struct task_t idle_task;
    dlist_container_fields(active, struct task_t)
    dlist_container_fields(sleeping, struct task_t)
};

static __inline__ void schedular_t_init (struct schedular_t* self) {
    task_t_init(&self->idle_task, &idle);
    dlist_container_init(active)
    dlist_container_init(sleeping)
}

static __inline__ void schedular_t_add_task (struct schedular_t* self, struct task_t* task) {
    dlist_push(active, task)
}

static __inline__ struct task_t* schedular_t_get_task (struct schedular_t* self, unsigned int tick_time) {
    struct task_t* task;
    while (dlist_has_first(sleeping) && dlist_first(sleeping)->sleep_until <= tick_time) {
        dlist_shift_existing(sleeping, task);
        dlist_push(active, task);
    }
    if (dlist_has_first(active)) {
        dlist_shift_existing(active, task);
        return task;
    }
    return &self->idle_task;
}

static __inline__ void schedular_t_task_sleep (
    struct schedular_t* self,
    struct task_t* task
) {
    /* TODO consider skip-list to speed insertion of sleeping tasks */
    struct task_t* next = dlist_first(sleeping);
    task->sleep_until = task->current[2];
    task->status = TASK_SLEEPING_STATUS;
    while (next != NULL) {
        if (task->sleep_until < next->sleep_until) {
            dlist_insert_before(sleeping, task, next);
            return;
        }
        next = next->next;
    }
    dlist_push(sleeping, task);
}

#define TICK_INTERVAL 1000000

int main () {
    struct task_t tasks[SCHEDULAR_TASK_LIMIT];

    /* size_t task_count = 2; */
    struct task_t* current_task;

    unsigned int tick_time = 0;

    int status;
    struct schedular_t schedular;

    debugp_decl();

    schedular_t_init(&schedular);

    *(PIC + VIC_INTENABLE) = PIC_TIMER01;

    *TIMER0 = TICK_INTERVAL;
    *(TIMER0 + TIMER_CONTROL) = TIMER_EN | TIMER_PERIODIC | TIMER_32BIT | TIMER_INTEN;

    task_t_init(&tasks[0], &first_task);
    task_t_init(&tasks[1], &second_task);

    schedular_t_add_task(&schedular, &tasks[0]);
    schedular_t_add_task(&schedular, &tasks[1]);

    bwputs("starting...\n");

    while (1) {
        current_task = schedular_t_get_task(&schedular, tick_time);
        status = task_t_activate(current_task);
        switch (status) {
            case 0x1: /* exit */
                bwputs("got exit\n");
                current_task->status = TASK_EXITED_STATUS;
                continue;
            case 0x2: /* usleep */
                schedular_t_task_sleep(&schedular, current_task); /* tick_time + current_task->current[2]); */
                continue;
            case -4: /* Timer 0 or 1 went off */
                if(*(TIMER0 + TIMER_MIS)) { /* Timer0 went off */
                    tick_time += TICK_INTERVAL;
                    *(TIMER0 + TIMER_INTCLR) = 1; /* Clear interrupt */
                    /* bwputs("tock\n"); */
                }
        }
    }
    return 0;
}


