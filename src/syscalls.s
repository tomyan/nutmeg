.global exit
exit:
    push {r7}
    mov r7, #0x1
    svc 0
    bx lr

.global usleep
usleep:
    push {r7}
    mov r7, #0x2
    svc 0
    bx lr

