; hcopy: Host to Guest copy
; This file is in the public domain.

	org 0x100

	; fname = args
	mov ch, 0
	mov cl, [0x80]
	sub cx, 1
	jbe error_usage
	mov si, 0x81
	cmp [si], byte " "
	jnz copy
	inc si
copy:
	mov di, fname
	cld
	rep movsb

	; hopen fname
	mov ah, 0xfe
	mov dx, fname
	int 0x13
	jc error_open

	; bx = create fname
	mov ah, 0x3c
	xor cx, cx
	mov dx, fname
	int 0x21
	jc error_create
	mov bx, ax

hcopy:
	; hcopy fname
	mov ah, 0xff
	mov dx, buf
	mov cx, 512
	int 0x13
	test cx, cx
	jz close

	; write bx buf 512
loop:
	mov ah, 0x40
	mov dx, buf
	int 0x21
	jc error_write
	sub cx, ax
	jnbe loop
	jmp hcopy

close:
	; close bx
	mov ah, 0x3e
	int 0x21

	; exit 0
	mov ax, 0x4c00
	int 0x21

error_usage:
	mov ah, 9
	mov dx, err_usage
	int 0x21
	jmp exit1

error_open:
	mov dx, err_open
	jmp showfnexit

error_create:
	mov dx, err_create
	jmp showfnexit
	
error_write:
	mov dx, err_write

showfnexit:
	mov ah, 9
	int 0x21
	mov dx, fname
	mov [di], word 0x240a
	int 0x21
exit1:
	mov ax, 0x4c01
	int 0x21

err_usage db "usage: hcopy filename", 13, 10, "$"
err_open db "can not open in host: $"
err_create db "can not create: $"
err_write db "can not write: $"
crlf db 13, 10, "$"

fname times 132 db 0
buf times 512 db 0
