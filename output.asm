.model small
.stack 100h
.data
.code
main proc
    mov ax, 0003h
    int 10h
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
