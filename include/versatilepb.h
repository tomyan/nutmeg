
/* UART reference: http://infocenter.arm.com/help/topic/com.arm.doc.ddi0183f/DDI0183.pdf */
#define uint32_t unsigned int

#define UART0_BASE_ADDR 0x101f1000
#define UART0 ((volatile uint32_t*)UART0_BASE_ADDR)

#define UART1_BASE_ADDR 0x101f2000
#define UART1 ((volatile uint32_t*)UART1_BASE_ADDR)

#define UART0_DR (*((volatile uint32_t *)(UART0_BASE_ADDR + 0x000)))
#define UART0_IMSCA (((volatile uint32_t *)(UART0_BASE_ADDR + 0x038)))

/* flag register offset */
#define UART_FR_OFFSET 0x06 /* 0x18 = 24 bytes */

/* Transmit FIFO Empty */
#define UART_FR_TXFE (1 << 7)
/* Recieve FIFO Full */
#define UART_FR_RXFF (1 << 6)
/* Transmit FIFO Full */
#define UART_FR_TXFF (1 << 5) 
/* Recieve FIFO Empty */
#define UART_FR_RXFE (1 << 4)

/* Recieve FIFO Empty */
#define UART_FR_BUSY (1 << 3)

/* interupt mask set/clear register offset */
#define UART_IMSC_OFFSET 0xE

/* bit to set in order to trigger interupts for given events on the device */
#define UART_RXIM (1 << 4)
#define UART_TXIM (1 << 5)

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

/* base address of the primary interrupt controller */
#define PIC ((volatile uint32_t*)0x10140000)

/* offset for read/write interrupt interrupt enable register */
#define PIC_INTENABLE 0x4 /* 0x10 = 16 bytes */

/* bits to set in order to enable interupts from particular devices */
#define PIC_TIMER01 (1 << 4)
#define PIC_UART0 (1 << 12)
#define PIC_UART1 (1 << 13)


