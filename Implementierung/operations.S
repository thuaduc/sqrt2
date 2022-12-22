    .intel_syntax noprefix
    .global binaryConverter
    .align 16

binaryConverter:
    mov r10, 33
    mov r9, rdx
    mov r11, rcx
    xor rcx, rcx
    mov eax, dword ptr [rsi +4*r8 - 4]

.loop:
    dec r10
    jz .endloop

    mov edx, eax
    shr edx, 31
    shl ecx, 1
    or ecx, edx
    cmp ecx, 10
    jnl .loop2

    shl eax, 1
    jmp .loop

.loop2:
    sub ecx, 10
    shl eax, 1
    or eax, 1
    jmp .loop

.endloop:
    mov dword ptr [r9 + 4*r8 - 4], eax
    dec r8
    jz .end
    mov r10, 33
    mov eax, dword ptr [rsi +4*r8 - 4]
    jmp .loop
.end:
    mov byte ptr [rdi + r11], cl
    ret