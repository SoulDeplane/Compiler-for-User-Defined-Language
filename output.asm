.model small
.stack 100h
.data
i dw ?
a dw ?
STR_2 db "Smaller$"
STR_1 db "Greater$"
STR_0 db "Hello Nova$"
.code
main proc
    mov ax, 0003h
    int 10h
    mov ax, 5
    mov [a], ax
    mov ax, [a]
    call print_int
    mov dx, offset STR_0
    mov ah, 09h
    int 21h
    mov ax, 1
    mov [i], ax
FOR_0:
    mov ax, [i]
    cmp ax, 5
    jg END_FOR_0
    mov ax, [i]
    call print_int
    inc word ptr [i]
    jmp FOR_0
END_FOR_0:
    mov ax, [a]
    push ax
    mov ax, 3
    pop bx
    cmp ax, 0
    je IF_FALSE_1
    mov dx, offset STR_1
    mov ah, 09h
    int 21h
    jmp IF_END_1
IF_FALSE_1:
    mov dx, offset STR_2
    mov ah, 09h
    int 21h
IF_END_1:
    mov ah, 4Ch
    int 21h
main endp
print_int proc
    mov bx, 10
    xor cx, cx
L1:
    xor dx, dx
    div bx
    push dx
    inc cx
    test ax, ax
    jnz L1
L2:
    pop dx
    add dl, '0'
    mov ah, 02h
    int 21h
    loop L2
    ret
print_int endp
end main
