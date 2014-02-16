; BIOS source for 8086tiny IBM PC emulator. Compiles with NASM.
; Copyright 2013, Adrian Cable (adrian.cable@gmail.com) - http://www.megalith.co.uk/8086tiny
;
; Revision 1.20
;
; This work is licensed under the MIT License. See included LICENSE-8086TINY.

	cpu	8086

; Defines

%define	biosbase 0x000f			; BIOS loads at segment 0xF000

org	100h				; BIOS loads at offset 0x0100

main:

	jmp	bios_entry

biosstr	db	'8086tiny BIOS Revision 1.10!', 0, 0		; Why not?
mem_top	db	0xea, 0, 0x01, 0, 0xf0, '01/11/14', 0, 0xfe, 0

bios_entry:

	; Set up initial stack to F000:F000

	mov	sp, 0xf000
	mov	ss, sp
	mov	es, sp
	mov	ds, sp

; Then we load in the pointers to our interrupt handlers

	mov	ax, 0
	mov	es, ax
	mov	di, 0
	mov	si, int_table
	mov	cx, [itbl_size]
	rep	movsb

; Set up last 16 bytes of memory, including boot jump, BIOS date, machine ID byte

	mov	ax, 0xffff
	mov	es, ax
	mov	di, 0x0
	mov	si, mem_top
	mov	cx, 16
	rep	movsb

; Set up the BIOS data area

	mov	ax, 0x40
	mov	es, ax
	mov	di, 0
	mov	si, bios_data
	mov	cx, 0x100
	rep	movsb

; Set up some I/O ports, between 0 and FFF. Most of them we set to 0xFF, to indicate no device present

	mov	dx, 0
	mov	al, 0xFF

next_out:

	inc	dx

	cmp	dx, 0x40	; We deal with the PIT channel 0 later
	je	next_out

	out	dx, al

	cmp	dx, 0xFFF
	jl	next_out

	mov	dx, 0x3DA	; CGA refresh port
	mov	al, 0
	out	dx, al

	mov	dx, 0x3BC	; LPT1
	mov	al, 0
	out	dx, al

	mov	dx, 0x40	; PIT channel 0
	mov	al, 0
	out	dx, al

	mov	dx, 0x62	; PPI - needed for memory parity checks
	mov	al, 0
	out	dx, al

; Read boot sector from FDD, and load it into 0:7C00

	mov	ax, 0
	mov	es, ax

	mov	ax, 0x0201
	mov	dx, 0		; Read from FDD
	mov	cx, 1
	mov	bx, 0x7c00
	int	13h

; Jump to boot sector

	jmp	0:0x7c00

; ************************* INT 7h handler - keyboard driver

int7:	; Whenever the user presses a key, INT 7 is called by the emulator.
	; ASCII character of the keystroke is at 0040:this_keystroke

	push	ds
	push	es
	push	ax
	push	bx
	push	bp

	push	cs
	pop	ds

	mov	bx, 0x40	; Set segment to BIOS data area segment (0x40)
	mov	es, bx

	; Tail of the BIOS keyboard buffer goes in BP. This is where we add new keystrokes

	mov	bp, [es:kbbuf_tail-bios_data]

	; First, copy zero keystroke to BIOS keyboard buffer - if we have an extended code then we
	; don't translate to a keystroke. If not, then this zero will later be overwritten
	; with the actual ASCII code of the keystroke.

	mov	byte [es:bp], 0

	; Retrieve the keystroke

	mov	al, [es:this_keystroke-bios_data]

	cmp	al, 0x7f ; Linux code for backspace - change to 8
	jne	after_check_bksp

	mov	al, 8
	mov	byte [es:this_keystroke-bios_data], 8

  after_check_bksp:

	cmp	al, 0x01 ; Ctrl+A pressed - this is the sequence for "next key is Alt+"
	jne	i2_not_alt

	mov	byte [es:keyflags1-bios_data], 8 ; Alt flag down
	mov	byte [es:keyflags2-bios_data], 2 ; Alt flag down
	mov	al, 0x38 ; Simulated Alt by Ctrl+A prefix?
	out	0x60, al
	int	9

	mov	byte [es:next_key_alt-bios_data], 1
	jmp	i2_dne

  i2_not_alt:

	cmp	al, 0x06 ; Ctrl+F pressed - this is the sequence for "next key is Fxx"
	jne	i2_not_fn

	mov	byte [es:next_key_fn-bios_data], 1
	jmp	i2_dne

  i2_not_fn:

	cmp	byte [es:notranslate_flg-bios_data], 1 ; If no translation mode is on, just pass through the scan code.
	mov	byte [es:notranslate_flg-bios_data], 0
	je	after_translate

	cmp	al, 0xe0 ; Some OSes return scan codes after 0xE0 for things like cursor moves. So, if we find it, set a flag saying the next code received should not be translated.
	mov	byte [es:notranslate_flg-bios_data], 1
	; je	after_translate
	je	i2_dne	; Don't add the 0xE0 to the keyboard buffer

	mov	byte [es:notranslate_flg-bios_data], 0

	cmp	al, 0x1b ; ESC key pressed. Either this a "real" escape, or it is UNIX cursor keys. In either case, we do nothing now, except set a flag
	jne	i2_escnext

	; If the last key pressed was ESC, then we need to stuff it
	cmp	byte [es:escape_flag-bios_data], 1
	jne	i2_sf

	; Stuff an ESC character
	
	mov	byte [es:bp], 0x1b ; ESC ASCII code
	mov	byte [es:bp+1], 0x01 ; ESC scan code

	; ESC keystroke is in the buffer now
	add	word [es:kbbuf_tail-bios_data], 2
	call	kb_adjust_buf ; Wrap the tail around the head if the buffer gets too large

	mov	al, 0x01
	call	keypress_release

  i2_sf:

	mov	byte [es:escape_flag-bios_data], 1
	jmp	i2_dne

  i2_escnext:

	; Check if the last key was an escape character
	cmp	byte [es:escape_flag-bios_data], 1
	jne	i2_noesc

	; It is, so check if this key is a ] character
	cmp	al, '[' ; [ key pressed
	je	i2_esc

	; It isn't, so stuff an ESC character plus this key
	
	mov	byte [es:bp], 0x1b ; ESC ASCII code
	mov	byte [es:bp+1], 0x01 ; ESC scan code

	; ESC keystroke is in the buffer now
	add	bp, 2
	add	word [es:kbbuf_tail-bios_data], 2
	call	kb_adjust_buf ; Wrap the tail around the head if the buffer gets too large

	mov	al, 0x01
	call	keypress_release

	; Now actually process this key
	mov	byte [es:escape_flag-bios_data], 0
	mov	al, [es:this_keystroke-bios_data]
	jmp	i2_noesc

  i2_esc:

	; Last + this characters are ESC ] - do nothing now, but set escape flag
	mov	byte [es:escape_flag-bios_data], 2
	jmp	i2_dne

  i2_noesc:

	cmp	byte [es:escape_flag-bios_data], 2
	jne	i2_regular_key

	; No shifts or Alt for cursor keys
	mov	byte [es:keyflags1-bios_data], 0
	mov	byte [es:keyflags2-bios_data], 0

	; Last + this characters are ESC ] xxx - cursor key, so translate and stuff it
	cmp	al, 'A'
	je	i2_cur_up
	cmp	al, 'B'
	je	i2_cur_down
	cmp	al, 'D'
	je	i2_cur_left
	cmp	al, 'C'
	je	i2_cur_right

  i2_cur_up:

	mov	al, 0x48 ; Translate UNIX code to cursor key scancode
	jmp	after_translate

  i2_cur_down:

	mov	al, 0x50 ; Translate UNIX code to cursor key scancode
	jmp	after_translate

  i2_cur_left:

	mov	al, 0x4b ; Translate UNIX code to cursor key scancode
	jmp	after_translate

  i2_cur_right:

	mov	al, 0x4d ; Translate UNIX code to cursor key scancode
	jmp	after_translate

  i2_regular_key:

	mov	byte [es:notranslate_flg-bios_data], 0

	mov	bx, a2shift_tbl ; ASCII to shift code table
	xlat

	; Now, AL is 1 if shift is down, 0 otherwise. If shift is down, put a shift down scan code
	; in port 0x60. Then call int 9. Otherwise, put a shift up scan code in, and call int 9.

	push	ax

	; Put shift flags in BIOS, 0040:0017. Add 8 to shift flags if Alt is down.
	mov	ah, [es:next_key_alt-bios_data]
	cpu	186
	shl	ah, 3
	cpu	8086
	add	al, ah
	mov	[es:keyflags1-bios_data], al

	cpu	186
	shr	ah, 2
	cpu	8086
	mov	[es:keyflags2-bios_data], ah

	pop	ax

	test	al, 1
	jz	i2_n

	mov	al, 0x36 ; Right shift down
	out	0x60, al
	int	9

  i2_n:

	mov	al, [es:this_keystroke-bios_data]
	mov	[es:bp], al

	mov	bx, a2scan_tbl ; ASCII to scan code table
	xlat

	cmp	byte [es:next_key_fn-bios_data], 1	; Fxx?
	jne	after_translate

	mov	byte [es:bp], 0	; Fxx key, so zero out ASCII code
	add	al, 0x39

  after_translate:

	mov	byte [es:escape_flag-bios_data], 0
	mov	byte [es:escape_flag_last-bios_data], 0

	; Now, AL contains the scan code of the key. Put it in the buffer
	mov	[es:bp+1], al

	; New keystroke + scancode is in the buffer now. If the key is actually
	; an Alt+ key we use an ASCII code of 0 instead of the real value.

	cmp	byte [es:next_key_alt-bios_data], 1
	jne	skip_ascii_zero

	mov	byte [es:bp], 0

skip_ascii_zero:

	add	word [es:kbbuf_tail-bios_data], 2
	call	kb_adjust_buf ; Wrap the tail around the head if the buffer gets too large

	; Output key down/key up event (scancode in AL) to keyboard port
	call	keypress_release

	; If scan code is not 0xE0, then also send right shift up
	cmp	al, 0xe0
	je	i2_dne

	mov	al, 0xb6 ; Right shift up
	out	0x60, al
	int	9

  check_alt:

	mov	al, byte [es:next_key_alt-bios_data]
	mov	byte [es:next_key_alt-bios_data], 0
	mov	byte [es:next_key_fn-bios_data], 0

	cmp	al, 1
	je	endalt

	jmp	i2_dne

  endalt:

	mov	al, 0xb8 ; Left Alt up
	out	0x60, al
	int	9

  i2_dne:

	pop	bp
	pop	bx
	pop	ax
	pop	es
	pop	ds
	iret

; ************************* INT 8h handler - timer

int8:	
	; See if there is an ESC waiting from a previous INT 2h. If so, put it in the keyboard buffer
	; (because by now - 1/18.2 secs on - we know it can't be part of an escape key sequence).
	; Also handle CGA refresh register. Also release any keys that are still marked as down.

	push	ax
	push	bx
	push	dx
	push	bp
	push	es

	push	cx
	push	di
	push	ds
	push	si

	mov	bx, 0x40
	mov	es, bx

	; Increment 32-bit BIOS timer tick counter, once every 8 calls

	cmp	byte [cs:int8_call_cnt], 8
	jne	skip_timer_increment

	add	word [es:0x6C], 1
	adc	word [es:0x6E], 0

	mov	byte [cs:int8_call_cnt], 0

skip_timer_increment:

	inc	byte [cs:int8_call_cnt]

	; Hercules is in text mode, so set I/O port 0 to 0

	mov	al, 0
	mov	dx, 0
	out	dx, al

	; See if we have any keys down. If so, release them
	cmp	byte [es:key_now_down-bios_data], 0
	je	i8_no_key_down

	mov	al, [es:key_now_down-bios_data]
	mov	byte [es:key_now_down-bios_data], 0
	add	al, 0x80
	out	0x60, al
	int	9

  i8_no_key_down:

	; See if we have a waiting ESC flag
	cmp	byte [es:escape_flag-bios_data], 1
	jne	i8_end
	
	; Did we have one last two cycles as well?
	cmp	byte [es:escape_flag_last-bios_data], 1
	je	i8_stuff_esc

	inc	byte [es:escape_flag_last-bios_data]
	jmp	i8_end

i8_stuff_esc:

	; Yes, clear the ESC flag and put it in the keyboard buffer
	mov	byte [es:escape_flag-bios_data], 0
	mov	byte [es:escape_flag_last-bios_data], 0

	mov	bp, [es:kbbuf_tail-bios_data]
	mov	byte [es:bp], 0x1b ; ESC ASCII code
	mov	byte [es:bp+1], 0x01 ; ESC scan code

	; ESC keystroke is in the buffer now
	add	word [es:kbbuf_tail-bios_data], 2
	call	kb_adjust_buf ; Wrap the tail around the head if the buffer gets too large

	; Push out ESC keypress/release
	mov	al, 0x01
	call	keypress_release

i8_end:	
	; Now, reset emulated PIC

	; mov	al, 0
	; mov	dx, 0x20
	; out	dx, al

	pop	si
	pop	ds
	pop	di
	pop	cx

	pop	es
	pop	bp
	pop	dx
	pop	bx
	pop	ax

	iret

; ************************* INT 16h handler - keyboard

int16:
	cmp	ah, 0x00 ; Get keystroke (remove from buffer)
	je	kb_getkey
	cmp	ah, 0x01 ; Check for keystroke (do not remove from buffer)
	je	kb_checkkey
	cmp	ah, 0x02 ; Check shift flags
	je	kb_shiftflags
	cmp	ah, 0x12 ; Check shift flags
	je	kb_extshiftflags

	iret

  kb_getkey:
	
	push	es
	push	bx
	push	cx
	push	dx

	sti

	mov	bx, 0x40
	mov	es, bx

    kb_gkblock:

	mov	cx, [es:kbbuf_tail-bios_data]
	mov	bx, [es:kbbuf_head-bios_data]
	mov	dx, [es:bx]

	; Wait until there is a key in the buffer
	cmp	cx, bx
	je	kb_gkblock

	add	word [es:kbbuf_head-bios_data], 2
	call	kb_adjust_buf

	mov	ah, dh
	mov	al, dl

	pop	dx
	pop	cx
	pop	bx
	pop	es	

	iret

  kb_checkkey:

	push	es
	push	bx
	push	cx
	push	dx

	sti

	mov	bx, 0x40
	mov	es, bx

	mov	cx, [es:kbbuf_tail-bios_data]
	mov	bx, [es:kbbuf_head-bios_data]
	mov	dx, [es:bx]

	; Check if there is a key in the buffer. ZF is set if there is none.
	cmp	cx, bx

	mov	ah, dh
	mov	al, dl

	pop	dx
	pop	cx
	pop	bx
	pop	es	
	
	retf	2	; NEED TO FIX THIS!!

    kb_shiftflags:

	push	es
	push	bx

	mov	bx, 0x40
	mov	es, bx

	mov	al, [es:keyflags1-bios_data]

	pop	bx
	pop	es

	iret

    kb_extshiftflags:

	push	es
	push	bx

	mov	bx, 0x40
	mov	es, bx

	mov	al, [es:keyflags1-bios_data]
	mov	ah, al

	pop	bx
	pop	es

	iret

; ************************* INT 1Eh - diskette parameter table

int1e_spt	db 0xff	; 18 sectors per track (1.44MB)

; ************************* ROM configuration table

rom_config	dw 16		; 16 bytes following
		db 0xfe		; Model
		db 'A'		; Submodel
		db 'C'		; BIOS revision
		db 0b00100000   ; Feature 1
		db 0b00000000   ; Feature 2
		db 0b00000000   ; Feature 3
		db 0b00000000   ; Feature 4
		db 0b00000000   ; Feature 5
		db 0, 0, 0, 0, 0, 0

; Internal state variables

cga_refresh_reg	db 0

; Default interrupt handlers

int0:
int1:
int2:
int3:
int4:
int5:
int6:
int9:
inta:
intb:
intc:
intd:
inte:
intf:
int10:
int11:
int12:
int13:
int14:
int15:
int17:
int18:
int19:
int1a:
int1b:
int1c:
int1d:
int1e:

iret

; ************ Function call library ************

; Keyboard adjust buffer head and tail. If either head or the tail are at the end of the buffer, reset them
; back to the start, since it is a circular buffer.

kb_adjust_buf:

	push	ax
	push	bx

	; Check to see if the head is at the end of the buffer (or beyond). If so, bring it back
	; to the start

	mov	ax, [es:kbbuf_end_ptr-bios_data]
	cmp	[es:kbbuf_head-bios_data], ax
	jnge	kb_adjust_tail

	mov	bx, [es:kbbuf_start_ptr-bios_data]
	mov	[es:kbbuf_head-bios_data], bx	

  kb_adjust_tail:

	; Check to see if the tail is at the end of the buffer (or beyond). If so, bring it back
	; to the start

	mov	ax, [es:kbbuf_end_ptr-bios_data]
	cmp	[es:kbbuf_tail-bios_data], ax
	jnge	kb_adjust_done

	mov	bx, [es:kbbuf_start_ptr-bios_data]
	mov	[es:kbbuf_tail-bios_data], bx	

  kb_adjust_done:

	pop	bx
	pop	ax
	ret

; Pushes a key press, followed by a key release, event to I/O port 0x60 and calls
; INT 9.

keypress_release:

	push	ax

	cmp	byte [es:key_now_down-bios_data], 0
	je	kpr_no_key_down

	mov	al, [es:key_now_down-bios_data]
	add	al, 0x80
	out	0x60, al
	int	9

	pop	ax
	push	ax

  kpr_no_key_down:

	mov	[es:key_now_down-bios_data], al
	out	0x60, al
	int	9

	pop	ax

	ret

; ****************************************************************************************
; That's it for the code. Now, the data tables follow.
; ****************************************************************************************

; Standard PC-compatible BIOS data area - to copy to 40:0

bios_data:

com1addr	dw	0
com2addr	dw	0
com3addr	dw	0
com4addr	dw	0
lpt1addr	dw	0
lpt2addr	dw	0
lpt3addr	dw	0
lpt4addr	dw	0
equip		dw	0b0000000100100001
		db	0
memsize		dw	0x280
		db	0
		db	0
keyflags1	db	0
keyflags2	db	0
		db	0
kbbuf_head	dw	kbbuf-bios_data
kbbuf_tail	dw	kbbuf-bios_data
kbbuf: times 32	db	'X'
drivecal	db	0
diskmotor	db	0
motorshutoff	db	0x07
disk_laststatus	db	0
times 7		db	0
vidmode		db	0x03
vid_cols	dw	80
page_size	dw	0x1000
		dw	0
curpos_x	db	0
curpos_y	db	0
times 7		dw	0
cur_v_end	db	7
cur_v_start	db	6
disp_page	db	0
crtport		dw	0x3d4
		db	10
		db	0
times 5		db	0
clk_dtimer	dd	0
clk_rollover	db	0
ctrl_break	db	0
soft_rst_flg	dw	0x1234
		db	0
num_hd		db	0
		db	0
		db	0
		dd	0
		dd	0
kbbuf_start_ptr	dw	0x001e
kbbuf_end_ptr	dw	0x003e
vid_rows	db	25         ; at 40:84
		db	0
		db	0
vidmode_opt	db	0 ; 0x70
		db	0 ; 0x89
		db	0 ; 0x51
		db	0 ; 0x0c
		db	0
		db	0
		db	0
		db	0
		db	0
		db	0
		db	0
		db	0
		db	0
		db	0
		db	0
kb_mode		db	0
kb_led		db	0
		db	0
		db	0
		db	0
		db	0
		db	0
		db	0
		db	0
key_now_down	db	0
next_key_fn	db	0
cursor_visible	db	1
escape_flag_last	db	0
next_key_alt	db	0
escape_flag	db	0
notranslate_flg	db	0
this_keystroke	db	0
		db	0
ending:		times (0xff-($-bios_data)) db	0

; Keyboard scan code tables

a2scan_tbl      db	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x0F, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x39, 0x02, 0x28, 0x04, 0x05, 0x06, 0x08, 0x28, 0x0A, 0x0B, 0x09, 0x0D, 0x33, 0x0C, 0x34, 0x35, 0x0B, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x27, 0x27, 0x33, 0x0D, 0x34, 0x35, 0x03, 0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C, 0x1A, 0x2B, 0x1B, 0x07, 0x0C, 0x29, 0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C, 0x1A, 0x2B, 0x1B, 0x29, 0x0E
a2shift_tbl     db	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0

; Interrupt vector table - to copy to 0:0

int_table	dw int0
          	dw 0xf000
          	dw int1
          	dw 0xf000
          	dw int2
          	dw 0xf000
          	dw int3
          	dw 0xf000
          	dw int4
          	dw 0xf000
          	dw int5
          	dw 0xf000
          	dw int6
          	dw 0xf000
          	dw int7
          	dw 0xf000
          	dw int8
          	dw 0xf000
          	dw int9
          	dw 0xf000
          	dw inta
          	dw 0xf000
          	dw intb
          	dw 0xf000
          	dw intc
          	dw 0xf000
          	dw intd
          	dw 0xf000
          	dw inte
          	dw 0xf000
          	dw intf
          	dw 0xf000
          	dw int10
          	dw 0xf000
          	dw int11
          	dw 0xf000
          	dw int12
          	dw 0xf000
          	dw int13
          	dw 0xf000
          	dw int14
          	dw 0xf000
          	dw int15
          	dw 0xf000
          	dw int16
          	dw 0xf000
          	dw int17
          	dw 0xf000
          	dw int18
          	dw 0xf000
          	dw int19
          	dw 0xf000
          	dw int1a
          	dw 0xf000
          	dw int1b
          	dw 0xf000
          	dw int1c
          	dw 0xf000
          	dw int1d
          	dw 0xf000
          	dw int1e

itbl_size	dw $-int_table

; Int 8 call counter - used for timer slowdown

int8_call_cnt	db	0
