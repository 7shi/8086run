; This file is in the public domain.

org 0x100

mov ah, 9
mov dx, hello
int 0x21

mov ax, 0x4c00
int 0x21

hello db "Hello, 8086run!", 10, "$"
