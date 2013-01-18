
#include "asm.h"
#include "versatilepb.h"
#include <stddef.h>
#include "dlist.h"

void bwputs(char *s) {
    unsigned int i = 0;
    while(s[i]) {
        while(*(UART0 + UART_FR_OFFSET) & UART_FR_TXFF);
        *UART0 = s[i++];
    }
}

void bwputs1(char *s) {
    unsigned int i = 0;
    while(s[i]) {
        while(*(UART1 + UART_FR_OFFSET) & UART_FR_TXFF);
        *UART1 = s[i++];
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

void pointer_to_hex (const void* input, char* result[11]) {
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
    bwputs("debug: " desc " "); \
    pointer_to_hex(p, paddr); \
    bwputs(*paddr); \
    bwputs("\n");


void first_task(void) {
    bwputs("debug: first task: started\n");
    usleep(2 * 1000 * 1000);
    bwputs("debug: first task: after usleep(2s) - exiting\n");
    exit(0);
}

void second_task(void) {
    bwputs("debug: second task: started\n");
    usleep(1 * 1000 * 1000);
    bwputs("debug: second task: after usleep(1s) - exiting\n");
    exit(0);
}

void idle () {
    while (1);
}

#define SERIAL_INPUT_BUFFER_LENGTH 512
#define SERIAL_OUTPUT_BUFFER_LENGTH 512
struct serial_port_t {
    volatile unsigned int* addr;
    unsigned short unsent_start;
    unsigned short unsent_remaining;
    char output_buffer[SERIAL_OUTPUT_BUFFER_LENGTH];
    unsigned short unread_start;
    unsigned short unread_remaining;
    char input_buffer[SERIAL_INPUT_BUFFER_LENGTH];
};

static __inline__ void serial_port_t_init (struct serial_port_t* self, volatile unsigned int* addr, const unsigned int interrupt_mask) {
    self->addr = addr;
    self->unsent_start = 0;
    self->unsent_remaining = 0;
    self->unread_start = 0;
    self->unread_remaining = 0;

    /* enable interrupts for device */
    *(PIC + PIC_INTENABLE) |= interrupt_mask; /* (1 << 12); */

    /* set the interupt mask with recieve interupt masks */
    *(self->addr + UART_IMSC_OFFSET) |= UART_RXIM;
}

#define min(x, y) ((x) < (y) ? (x) : (y))
#define overflow_mod(x, y) ((x) > (y) ? (x) - (y) : (x))

static __inline__ unsigned short int serial_port_t_read (struct serial_port_t* self, char** dest, const unsigned int length) {
    unsigned int i;
    unsigned short read;
    assert(SERIAL_INPUT_BUFFER_LENGTH >= self->unread_remaining);
    read = min(length, (unsigned short) (SERIAL_INPUT_BUFFER_LENGTH - self->unread_remaining));
    for (i = 0; i < read; i++) {
        *(dest[i]) = self->input_buffer[self->unread_start];
        if ((self->unread_start + 1) > SERIAL_INPUT_BUFFER_LENGTH) {
            self->unread_start = 0;
        }
        else {
            self->unread_start++;
        }
        self->unread_remaining--;
    }
    if (self->unread_remaining < SERIAL_INPUT_BUFFER_LENGTH) {
        *(self->addr + UART_IMSC_OFFSET) |= UART_RXIM;
    }
    return read;
}

static __inline__ unsigned short int serial_port_t_write (struct serial_port_t* self, const char* data, const unsigned int length) {
    unsigned short i;
    unsigned short write;
    assert(SERIAL_OUTPUT_BUFFER_LENGTH >= self->unsent_remaining);
    write = min(length, (unsigned int) (SERIAL_OUTPUT_BUFFER_LENGTH - self->unsent_remaining));
    for (i = 0; i < write; i++) {
        self->output_buffer[ overflow_mod(self->unsent_start + self->unsent_remaining, SERIAL_OUTPUT_BUFFER_LENGTH) ] = data[i];
        self->unsent_remaining++;
    }
    if (self->unsent_remaining > 0) {
        /* set the interupt mask with send interupt masks */
        *(self->addr + UART_IMSC_OFFSET) |= UART_TXIM;  
    }
    return write;
}

static __inline__ void serial_port_t_handle_interrupt (struct serial_port_t* self) {
    while (self->unsent_remaining > 0 && (*(self->addr + UART_FR_OFFSET) & UART_FR_TXFF) == 0) {
        *(self->addr) = self->output_buffer[self->unsent_start];
        if ((self->unsent_start + 1) > SERIAL_OUTPUT_BUFFER_LENGTH) {
            self->unsent_start = 0;
        }
        else {
            self->unsent_start++;
        }
        self->unsent_remaining--;
        if (self->unsent_remaining == 0) {
            /* set the interupt mask with send interupt masks */
            *(self->addr + UART_IMSC_OFFSET) &= ~UART_TXIM;  
        }
    }
    while ((self->unread_remaining < SERIAL_INPUT_BUFFER_LENGTH) && (*(self->addr + UART_FR_OFFSET) & UART_FR_RXFE) == 0) {
        self->input_buffer[overflow_mod(self->unread_start + self->unread_remaining, SERIAL_INPUT_BUFFER_LENGTH)] = *(self->addr);
        self->unread_remaining++;
    }
    if (self->unread_remaining == SERIAL_INPUT_BUFFER_LENGTH) {
        /* No buffer space, so stop listening to events we can't handle. */
        *(self->addr + UART_IMSC_OFFSET) &= ~UART_RXIM;
        
    }
}

#define TASK_STACK_SIZE 256 /* Size of task stacks in words */
#define TASK_EXITED_STATUS 0
#define TASK_RUNNING_STATUS 1
#define TASK_SLEEPING_STATUS 2

union debuggable_double {
    double d;
    struct {
#ifdef LITTLE_ENDIAN
        int j, i;
#else
        int i, j;
#endif
    } n;
};


struct task_t {
    unsigned int stack[TASK_STACK_SIZE];
    unsigned int* current;
    unsigned int status;
    unsigned int sleep_until;
    union debuggable_double timeshare;
    unsigned int timeshare_at;
    dlist_fields(struct task_t)
};

static __inline__ void task_t_init (struct task_t* self, void (*start)(void)) {
    self->status = TASK_RUNNING_STATUS;
    self->timeshare.d = 1.;

    self->timeshare_at = 0;
    /* set current position to the end of the stack, minus what we're about to push */
    self->current = self->stack + TASK_STACK_SIZE - 16;
    /* User mode, interupts on */
    self->current[0] = 0x10;
    self->current[1] = (unsigned int)start;
    dlist_init();
}


#define SCHEDULAR_TASK_LIMIT 2   /* Max number of tasks we can handle */

struct schedular_t {
    struct task_t idle_task;
    dlist_container_fields(active, struct task_t)
    dlist_container_fields(sleeping, struct task_t)
};

static __inline__ void schedular_t_init (struct schedular_t* self) {
    task_t_init(&self->idle_task, &idle);
    dlist_container_init(active);
    dlist_container_init(sleeping);
}

static __inline__ void schedular_t_add_task (struct schedular_t* self, struct task_t* task, unsigned int tick_time) {
    task->timeshare.d = 1.;
    task->timeshare_at = tick_time;
    dlist_push(active, task);
}

/* fast exponential approximation from http://nic.schraudolph.org/pubs/Schraudolph99.pdf
 */

#define LITTLE_ENDIAN

#define M_LN2 0.693147180559945309417232121458176568

static union {
    double d;
    struct {
#ifdef LITTLE_ENDIAN
        int j, i;
#else
        int i, j;
#endif
    } n;
} _eco;

#define EXP_A (1048576 / M_LN2) /* use 1512775 for integer version */
#define EXP_C 60801 /* see text (of paper) for choice of c values */
#define EXP(y) (_eco.n.i = EXP_A * (y) + (1072693248 - EXP_C), _eco.d)

#define DECAY(initial_value, time, halflife) (initial_value * (1. / EXP(time * M_LN2 / halflife)))

#define HALFLIFE (1 * 1000 * 1000)

static __inline__ void task_t_update_timeshare (struct task_t* self, unsigned int tick_time) {
    self->timeshare.d = DECAY(self->timeshare.d, tick_time, HALFLIFE);
}

static __inline__ struct task_t* schedular_t_get_task (struct schedular_t* self, unsigned int tick_time) {
    struct task_t* task;
    struct task_t* next;
    while (dlist_has_first(sleeping) && dlist_first(sleeping)->sleep_until <= tick_time) {
        dlist_shift_existing(sleeping, task);
        task_t_update_timeshare(task, tick_time);
        next = dlist_first(active);
        while (1) {
            if (next == NULL) {
                dlist_push(active, task);
                break;
            }
            task_t_update_timeshare(next, tick_time);
            if (task->timeshare.d <= next->timeshare.d) {
                dlist_insert_before(active, task, next);
                break;
            }
            next = next->next;
        }
    }
    if (dlist_has_first(active)) {
        task = dlist_first(active);
        if (task->timeshare_at != tick_time) {
            task_t_update_timeshare(task, tick_time);
        }
        if (task->next != NULL) {
            if (task->next->timeshare_at != tick_time) {
                task_t_update_timeshare(task->next, tick_time);
            }
            if (task->next->timeshare.n.j < task->timeshare.n.j) {
                task = task->next;
                dlist_extract_not_first(active, task);
                dlist_unshift(active, task);
            }
        }
        return task;
    }
    return &self->idle_task;
}

static __inline__ int schedular_t_activate_task (struct schedular_t* self, struct task_t* task) {
    task->current = activate(task->current);
    /* TODO: replace magic numbers with meaningful constants */
    return task->current[ 2 + 7 ];
}

static __inline__ void schedular_t_task_sleep (
    struct schedular_t* self,
    struct task_t* task
) {
    struct task_t* next;
    dlist_detach_first(active);
    /* TODO consider skip-list to speed insertion of sleeping tasks */
    next = dlist_first(sleeping);
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

static __inline__ void schedular_t_task_exited (struct schedular_t* self, struct task_t* task) {
    task->status = TASK_EXITED_STATUS;
    dlist_shift_existing(active, task);
}

#define TICK_INTERVAL 1000

#define string(s) s, sizeof(s) - 1


int main () {
    struct task_t tasks[SCHEDULAR_TASK_LIMIT];

    /* size_t task_count = 2; */
    struct task_t* current_task;

    unsigned int tick_time = 0;

    int status;
    struct schedular_t schedular;

    struct serial_port_t uart0;
    struct serial_port_t uart1;

    schedular_t_init(&schedular);

    *(PIC + PIC_INTENABLE) = PIC_TIMER01;

    *TIMER0 = TICK_INTERVAL;
    *(TIMER0 + TIMER_CONTROL) = TIMER_EN | TIMER_PERIODIC | TIMER_32BIT | TIMER_INTEN;

    task_t_init(&tasks[0], &first_task);
    task_t_init(&tasks[1], &second_task);

    schedular_t_add_task(&schedular, &tasks[0], tick_time);
    schedular_t_add_task(&schedular, &tasks[1], tick_time);

    bwputs("debug: initialising serial 0\n");

    serial_port_t_init(&uart0, UART0, PIC_UART0);

    serial_port_t_write(&uart0, string("debug: starting...\n"));

    serial_port_t_init(&uart1, UART1, PIC_UART1);

    serial_port_t_write(&uart1, string("serial1 output\n"));

    while (1) {
        current_task = schedular_t_get_task(&schedular, tick_time);
        status = schedular_t_activate_task(&schedular, current_task);
        switch (status) {
            case 0x1: /* exit */
                bwputs("debug: got exit\n");
                /*serial_port_t_write(&uart1, string("debug: got exit\n"));*/
                schedular_t_task_exited(&schedular, current_task);
                break;
            case 0x2: /* usleep */
                bwputs("debug: usleep\n");
                schedular_t_task_sleep(&schedular, current_task); /* tick_time + current_task->current[2]); */
                break;
            case -4: /* Timer 0 or 1 went off */
                if(*(TIMER0 + TIMER_MIS)) { /* Timer0 went off */
                    tick_time += TICK_INTERVAL;
                    *(TIMER0 + TIMER_INTCLR) = 1; /* Clear interrupt */
                }
                task_t_update_timeshare(current_task, tick_time);
                current_task->timeshare.d += 1.;
                break;
            case -12:
                bwputs("debug: serial0\n");
                serial_port_t_handle_interrupt(&uart0);
                break;
            case -13:
                bwputs("debug: serial1\n");
                serial_port_t_handle_interrupt(&uart1);
                break;
            default:
                bwputs("debug: unknown status from interrupt\n");
                /* TODO improve status output */
                while(1);
        }
    }
    return 0;
}


