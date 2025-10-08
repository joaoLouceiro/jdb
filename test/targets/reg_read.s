.global main

.section .data
my_double: .double 64.125

.section .text

# For more details on what the hell is happening here, check @reg_write.s
.macro trap
    movq $62, %rax
    movq %r12, %rdi
    movq $5, %rsi
    syscall
.endm

main:
    push    %rbp
    movq    %rsp, %rbp

    # Get PID
    movq    $39, %rax
    syscall
    movq    %rax, %r12

    # Write to a simple GPR
    movq    $0xcafecafe, %r13
    trap

    # Write to a sub GPR
    # movb instructs to move to a subregister
    movb    $42, %r13b
    trap

    # Write to an MMX
    # We can't write directly to mm0, so we first write to a GRP and then move the value
    movq    $0xba5eba11, %r13
    movq    %r13, %mm0
    trap

    # Write to an SSE
    movsd   my_double(%rip), %xmm0
    trap

    # Write to a x87
    # Because these share the memory addresses with MMX, we need to first Empty MMX State.
    # fldl loads a floating point value onto the FPU stack.
    emms
    fldl    my_double(%rip)
    trap

    popq    %rbp
    movq    $0, %rax
