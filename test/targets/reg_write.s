.global main

.section .data

# .asciz encodes the string into ASCII with a null terminator.
# %#x is the printf format for hexadecimal numbers.
hex_format: .asciz "%#x"

.section .text

.macro trap
    # Syscalls are expected to be in rax. The syscall for kill is 62.
    movq $62, %rax
    # The first parameter for a syscall is stored in rdi. Since we previously stored the result of getpid in r12, we move that value to rdi.
    movq %r12, %rdi
    # Put SIGTRAP (5) as the second parameter for the kill syscall
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

    trap
 
    # Print rsi
    # "load effective address quadword" puts the address of hex_format into rdi, where printf expects it to be
    leaq    hex_format(%rip), %rdi
    # printf is a varargs function. With varargs, we need to tell how many vector registers are going to be used, which is 0, in this case.
    movq    $0, %rax
    # "procedure linkage table" is used to call functions defined in shared libraries
    call    printf@plt
    movq    $0, %rdi
    # Call fflush(0) to flush all streams
    call    fflush@plt

    trap

    popq    %rbp
    movq    $0, %rax
    ret
