.global main

.section .data

# .asciz encodes the string into ASCII with a null terminator.
# %#x is the printf format for hexadecimal numbers.
hex_format: .asciz "%#x"
float_format: .asciz "%.2f"
long_float_format: .asciz "%.2Lf"

.section .text

.macro trap
    # Syscalls are expected to be in rax. The syscall for kill is 62.
    movq $62, %rax
    # The first parameter for a syscall is stored in rdi. 
    # Since we previously stored the result of getpid in r12, we move that value to rdi.
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
    # printf is a varargs function. With varargs, we need to tell how many 
    # vector registers are going to be used, which is 0, in this case.
    movq    $0, %rax
    # Because we are writing to rsi (where the syscall expects it's first parameter to live) 
    # from the debugger's test, printf will send the value we just wrote to the standard out. 
    # Our goal is to check, from within a running process, that we can affect the value of a register.
    # "procedure linkage table" is used to call functions defined in shared libraries
    call    printf@plt
    # Call fflush(0) to flush all streams
    movq    $0, %rdi
    call    fflush@plt

    trap

    # Test MMX
    # From the debugger, we write into mm0.
    # We then move that value onto rsi and call printf, so the value is sent to the standard out.
    movq    %mm0, %rsi
    leaq    hex_format(%rip), %rdi
    movq    $0, %rax
    call    printf@plt
    movq    $0, %rdi
    call    fflush@plt

    trap

    # Test SSE/XMM
    # Because XMM are vector registers, instead of moving the value into rsi, we only need to 
    # tell printf that there is a vector argument by placing 1 in rax
    leaq    float_format(%rip), %rdi
    movq    $1, %rax
    call    printf@plt
    movq    $0, %rdi
    call    fflush@plt

    trap

    # Test x87
    # Push a value to the FPU stack from the debugger, pop that value onto our function stack
    # and call printf with that value.

    # Allocate 16 bytes to the stack, to store the contents of st0.
    # Because the stack grows downwards, we do that by subtracting from the rsp register
    subq    $16, %rsp
    # Pop the value from the top of the stack.
    fstpt   (%rsp)
    leaq    long_float_format(%rip), %rdi
    movq    $0, %rax
    call    printf@plt
    movq    $0, %rdi
    call    fflush@plt
    # Clean up the space required before
    addq    $16, %rsp

    trap


    popq    %rbp
    movq    $0, %rax
    ret
