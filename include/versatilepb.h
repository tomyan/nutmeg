
#define uint32_t unsigned int

#define UART0_BASE_ADDR 0x101f1000
#define UART0 ((volatile uint32_t*)UART0_BASE_ADDR)
#define UART0_DR (*((volatile uint32_t *)(UART0_BASE_ADDR + 0x000)))
#define UART0_IMSC (*((volatile uint32_t *)(UART0_BASE_ADDR + 0x038)))
#define UARTFR 0x06
#define UARTFR_TXFF 0x20

/* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0271d/index.html */
#define TIMER0 ((volatile uint32_t*)0x101E2000)
#define TIMER_VALUE 0x1 /* 0x04 bytes */
#define TIMER_CONTROL 0x2 /* 0x08 bytes */
#define TIMER_INTCLR 0x3 /* 0x0C bytes */
#define TIMER_MIS 0x5 /* 0x14 bytes */
/* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0271d/Babgabfg.html */
#define TIMER_EN 0x80
#define TIMER_PERIODIC 0x40
#define TIMER_INTEN 0x20
#define TIMER_32BIT 0x02
#define TIMER_ONESHOT 0x01

/* http://infocenter.arm.com/help/topic/com.arm.doc.dui0224i/I1042232.html */
#define PIC ((volatile uint32_t*)0x10140000)
#define PIC_TIMER01 0x10
#define PIC_UART0 (1 << 12)
/* http://infocenter.arm.com/help/topic/com.arm.doc.ddi0181e/I1006461.html */
#define VIC_INTENABLE 0x4 /* 0x10 bytes */


