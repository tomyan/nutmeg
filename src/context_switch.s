.global activate
activate:
    /* save kernel state */

    /* ip = intra-procedure scratch register (r12), which contains the saved/initial
       value for the Current Programme Status Register, taken from the stack */
    mov ip, sp

    /* push all registers that store kernel state onto the stack */
    push {r4,r5,r6,r7,r8,r9,r10,fp,ip,lr}

    ldmfd r0!, {ip,lr}
    msr SPSR, ip

    msr CPSR_c, #0xDF /* System mode */
    mov sp, r0
    pop {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,fp,ip,lr}
    pop {r7}
    msr CPSR_c, #0xD3 /* Supervisor mode */

    movs pc, lr

.global svc_entry
svc_entry:
    /* Save user state */
    msr CPSR_c, #0xDF /* System mode */
    push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,fp,ip,lr}
    mov r0, sp
    msr CPSR_c, #0xD3 /* Supervisor mode */

    mrs ip, SPSR
    stmfd r0!, {ip,lr}

    /* Load kernel state */
    pop {r4,r5,r6,r7,r8,r9,r10,fp,ip,lr}
    mov sp, ip
    bx lr

.global irq_entry
irq_entry:
    /* Move 0xDF (1101,1111) to the Control field of the Current Programme Status Register
     * i.e. enter System Mode (0001,1111) and disable IRQ and FIQ interrupts (1000,0000 and
     * 0100,0000 respectively), thumb mode off (~0010,0000).
     *
     * When System Mode is entered, the current Programme Counter (r15) is copied
     * into the Link Register (r14) and the Current Programme Status Register
     * is copied into the Saved Programme Status Register for the mode.
     * 
     * System mode is a privileged processor mode that shares the User mode registers.
     * Privileged mode tasks can run in this mode, and exceptions no longer overwrite
     * the link register.
     */
    msr CPSR_c, #0xDF

    /* Save r7 so we can use it to store an identifier for the interrupt -
     * going to use the negated bit number for the device. */
    push {r7}

    /* Load address of Primary Interrupt Controller status into r7. */
    ldr r7, PIC
    PIC: .word 0x10140000

    /* Load status of Primary Interrupt Controller into r7. */
    ldr r7, [r7]

    /* Count the number of leading zeros of status into r7. This results in
     * a value relative to the index of the bit identifying the device that
     * raised the interrupt (http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0224i/Chdbeibh.html).
     */
    clz r7, r7

    /* Subtract 31 from the value described above, to result in the negated
     * index of the bit identifying the device that raised the interupt (link
     * above).
     */
    sub r7, r7, #31

    /* Save registers state. */
    push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,fp,ip,lr}

    mov r0, sp

    /* Move 0xD2 (1101,0010) to the Control field of the Current Programme Status Register
     * i.e. Interrupt Mode (0001,0010) and keep the IRQ and FIQ interupts
     * (1000,000 and 0100,0000 repectively) disabled, thumb mode off (~0010,0000).
     */
    msr CPSR_c, #0xD2

    /* Copy the Saved Programme Status Register to the ip register (intra-procedure-call scratch register, r12).
     */
    mrs ip, SPSR

    /* Subtract 4 bytes (32 bits - i.e. one address) from the link register (r14), which contains
     * the address of the next instruction.
     */
    sub lr, lr, #0x4

    /* Store the saved programme status register and new link register to r0 and r1.
     */
    stmfd r0!, {ip,lr}

    /* Move 0xD3 (1101,0011 tot he Control field of the Current Programme Status Register
     * i.e. Supervisor Mode with IRQ and FIQ off and no thumb mode.
     */
    msr CPSR_c, #0xD3 /* Supervisor mode */

    /* Load kernel state */
    pop {r4,r5,r6,r7,r8,r9,r10,fp,ip,lr}

    /* Reinstate the stack-pointer from the scratch register.
     */
    mov sp, ip

    /* Branch to location stored in link register.
     */
    bx lr
    
