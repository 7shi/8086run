; hcopy: Host to Guest copy
; This file is in the public domain.

org 0x100

%define argc 0x80
%define argv 0x81
%define fname bss
%define buf bss + 0x100

start:
	mov ch, 0
	mov cl, [argc]
	sub cx, 1
	jnbe short start0
	jmp error_usage
start0:
	mov si, argv
	cmp [si], byte " "
	jnz short checkp
	inc si

checkp:
	; check /s
	xor dx, dx
	cmp [si], word "/s"
	jnz short copyarg
	cmp [si+2], byte " "
	jnz short copyarg
	add si, 3
	sub cx, 3
	jnbe short checkp0
	jmp error_usage
checkp0:
	mov dx, 1

copyarg:
	; fname = args
	mov di, fname
	cld
	rep movsb
	mov [di], byte 0
	test dx, dx
	jnz short send

	; hopen fname
	mov ax, 0xfe00
	mov dx, fname
	int 0x13
	jc short error_hopen

	; bx = create fname
	mov ah, 0x3c
	xor cx, cx
	mov dx, fname
	int 0x21
	jc short error_create
	mov bx, ax

hread:
	; hread buf 512
	mov ah, 0xff
	mov dx, buf
	mov cx, 512
	int 0x13
	test cx, cx
	jz short close

write:
	; write bx buf 512
	mov ah, 0x40
	mov dx, buf
	int 0x21
	jc short error_write
	sub cx, ax
	jnbe short write
	jmp hread

send:
	; bx = open fname
	mov ax, 0x3d00
	mov dx, fname
	int 0x21
	jc short error_open
	mov bx, ax

	; hcreate fname
	mov ax, 0xfe01
	mov dx, fname
	int 0x13
	jc short error_hcreate

read:
	; read bx buf 512
	mov ah, 0x3f
	mov dx, buf
	mov cx, 512
	int 0x21
	jc short error_read
	test ax, ax
	jz short close
	
	; hwrite buf cx
	mov cx, ax
	mov ah, 0xff
	mov dx, buf
	int 0x13
	jmp read

close:
	; close bx
	mov ah, 0x3e
	int 0x21

	; exit 0
	mov ax, 0x4c00
	int 0x21
	jmp close

error_usage:
	mov ah, 9
	mov dx, err_usage
	int 0x21
	jmp exit1

error_hopen:
	mov dx, err_hopen
	jmp showfnexit

error_hcreate:
	mov dx, err_hcreate
	jmp showfnexit

error_open:
	mov dx, err_open
	jmp showfnexit

error_create:
	mov dx, err_create
	jmp showfnexit

error_read:
	mov dx, err_read
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

err_usage   db "usage: hcopy [/s] filename", 10
            db "  default  copy from host", 10
            db "  /s       send to host", 10, "$"
err_hopen   db "can not open in host: $"
err_hcreate db "can not create in host: $"
err_open    db "can not open: $"
err_create  db "can not create: $"
err_read    db "can not read: $"
err_write   db "can not write: $"

bss:
