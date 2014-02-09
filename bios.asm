; BIOS source for 8086tiny IBM PC emulator. Compiles with NASM.
; Copyright 2013, Adrian Cable (adrian.cable@gmail.com) - http://www.megalith.co.uk/8086tiny
;
; Revision 1.20
;
; This work is licensed under the MIT License. See included LICENSE-8086TINY.

	cpu	8086

; Defines

%define	biosbase 0x000f			; BIOS loads at segment 0xF000

; Here we define macros for some custom instructions that help the emulator talk with the outside
; world. They are described in detail in the hint.html file, which forms part of the emulator
; distribution.

%macro	extended_putchar_al 0
	db	0x0f
	db	0x00
%endmacro

%macro	extended_get_rtc 0
	db	0x0f
	db	0x01
%endmacro

%macro	extended_read_disk 0
	db	0x0f
	db	0x02
%endmacro

%macro	extended_write_disk 0
	db	0x0f
	db	0x03
%endmacro

org	100h				; BIOS loads at offset 0x0100

main:

	jmp	bios_entry

; Here go pointers to the different data tables used for instruction decoding

	dw	rm_mode12_reg1	; Table 0: R/M mode 1/2 "register 1" lookup
	dw	biosbase
	dw	rm_mode12_reg2	; Table 1: R/M mode 1/2 "register 2" lookup
	dw	biosbase
	dw	rm_mode12_disp	; Table 2: R/M mode 1/2 "DISP multiplier" lookup
	dw	biosbase
	dw	rm_mode12_dfseg	; Table 3: R/M mode 1/2 "default segment" lookup
	dw	biosbase
	dw	rm_mode0_reg1	; Table 4: R/M mode 0 "register 1" lookup
	dw	biosbase
	dw	rm_mode0_reg2	; Table 5: R/M mode 0 "register 2" lookup
	dw	biosbase
	dw	rm_mode0_disp	; Table 6: R/M mode 0 "DISP multiplier" lookup
	dw	biosbase
	dw	rm_mode0_dfseg	; Table 7: R/M mode 0 "default segment" lookup
	dw	biosbase
	dw	xlat_ids	; Table 8: Translation of raw opcode index ("Raw ID") to function number ("Xlat'd ID")
	dw	biosbase
	dw	0		; Table 9: (Defunct)
	dw	biosbase
	dw	0		; Table 10: (Defunct)
	dw	biosbase
	dw	0		; Table 11: (Defunct)
	dw	biosbase
	dw	0		; Table 12: (Defunct)
	dw	biosbase
	dw	0		; Table 13: (Defunct)
	dw	biosbase
	dw	ex_data		; Table 14: Translation of Raw ID to Extra Data
	dw	biosbase
	dw	std_szp		; Table 15: Translation of Raw ID to SZP flags set yes/no
	dw	biosbase
	dw	std_ao		; Table 16: Translation of Raw ID to AO flags set yes/no
	dw	biosbase
	dw	std_o0c0	; Table 17: Translation of Raw ID to OF=0 CF=0 set yes/no
	dw	biosbase
	dw	base_size	; Table 18: Translation of Raw ID to base instruction size (bytes)
	dw	biosbase
	dw	i_w_adder	; Table 19: Translation of Raw ID to i_w size adder yes/no
	dw	biosbase
	dw	i_mod_adder	; Table 20: Translation of Raw ID to i_mod size adder yes/no
	dw	biosbase
	dw	jxx_dec_a	; Table 21: Jxx decode table A
	dw	biosbase
	dw	jxx_dec_b	; Table 22: Jxx decode table B
	dw	biosbase
	dw	jxx_dec_c	; Table 23: Jxx decode table C
	dw	biosbase
	dw	jxx_dec_d	; Table 24: Jxx decode table D
	dw	biosbase
	dw	flags_mult	; Table 25: FLAGS multipliers
	dw	biosbase
	dw	daa_rslt_ca00	; Table 26: DAA result, CF = 0, AF = 0
	dw	biosbase
	dw	daa_cf_ca00_ca01_das_cf_ca00	; Table 27: DAA CF out, CF = 0, AF = 0
	dw	biosbase
	dw	daa_af_ca00_ca10_das_af_ca00_ca10	; Table 28: DAA AF out, CF = 0, AF = 0
	dw	biosbase
	dw	daa_rslt_ca01	; Table 29: DAA result, CF = 0, AF = 1
	dw	biosbase
	dw	daa_cf_ca00_ca01_das_cf_ca00	; Table 30: DAA CF out, CF = 0, AF = 1
	dw	biosbase
	dw	daa_af_ca01_cf_ca10_cf_ca11_af_ca11_das_af_ca01_cf_ca10_cf_ca11_af_ca11	; Table 31: DAA AF out, CF = 0, AF = 1
	dw	biosbase
	dw	daa_rslt_ca10	; Table 32: DAA result, CF = 1, AF = 0
	dw	biosbase
	dw	daa_af_ca01_cf_ca10_cf_ca11_af_ca11_das_af_ca01_cf_ca10_cf_ca11_af_ca11	; Table 33: DAA CF out, CF = 1, AF = 0
	dw	biosbase
	dw	daa_af_ca00_ca10_das_af_ca00_ca10	; Table 34: DAA AF out, CF = 1, AF = 0
	dw	biosbase
	dw	daa_rslt_ca11	; Table 35: DAA result, CF = 1, AF = 1
	dw	biosbase
	dw	daa_af_ca01_cf_ca10_cf_ca11_af_ca11_das_af_ca01_cf_ca10_cf_ca11_af_ca11	; Table 36: DAA CF out, CF = 1, AF = 1
	dw	biosbase
	dw	daa_af_ca01_cf_ca10_cf_ca11_af_ca11_das_af_ca01_cf_ca10_cf_ca11_af_ca11	; Table 37: DAA AF out, CF = 1, AF = 1
	dw	biosbase
	dw	das_rslt_ca00	; Table 38: DAS result, CF = 0, AF = 0
	dw	biosbase
	dw	daa_cf_ca00_ca01_das_cf_ca00	; Table 39: DAS CF out, CF = 0, AF = 0
	dw	biosbase
	dw	daa_af_ca00_ca10_das_af_ca00_ca10	; Table 40: DAS AF out, CF = 0, AF = 0
	dw	biosbase
	dw	das_rslt_ca01	; Table 41: DAS result, CF = 0, AF = 1
	dw	biosbase
	dw	das_cf_ca01	; Table 42: DAS CF out, CF = 0, AF = 1
	dw	biosbase
	dw	daa_af_ca01_cf_ca10_cf_ca11_af_ca11_das_af_ca01_cf_ca10_cf_ca11_af_ca11	; Table 43: DAS AF out, CF = 0, AF = 1
	dw	biosbase
	dw	das_rslt_ca10	; Table 44: DAS result, CF = 1, AF = 0
	dw	biosbase
	dw	daa_af_ca01_cf_ca10_cf_ca11_af_ca11_das_af_ca01_cf_ca10_cf_ca11_af_ca11	; Table 45: DAS CF out, CF = 1, AF = 0
	dw	biosbase
	dw	daa_af_ca00_ca10_das_af_ca00_ca10	; Table 46: DAS AF out, CF = 1, AF = 0
	dw	biosbase
	dw	das_rslt_ca11	; Table 47: DAS result, CF = 1, AF = 1
	dw	biosbase
	dw	daa_af_ca01_cf_ca10_cf_ca11_af_ca11_das_af_ca01_cf_ca10_cf_ca11_af_ca11	; Table 48: DAS CF out, CF = 1, AF = 1
	dw	biosbase
	dw	daa_af_ca01_cf_ca10_cf_ca11_af_ca11_das_af_ca01_cf_ca10_cf_ca11_af_ca11	; Table 49: DAS AF out, CF = 1, AF = 1
	dw	biosbase
	dw	parity		; Table 50: Parity flag loop-up table (256 entries)
	dw	biosbase
	dw	i_opcodes	; Table 51: 8-bit opcode lookup table
	dw	biosbase

; These values (BIOS ID string, BIOS date and so forth) go at the very top of memory

biosstr	db	'8086tiny BIOS Revision 1.10!', 0, 0		; Why not?
mem_top	db	0xea, 0, 0x01, 0, 0xf0, '01/11/14', 0, 0xfe, 0

bios_entry:

	; Set up initial stack to F000:F000

	mov	sp, 0xf000
	mov	ss, sp

	push	cs
	pop	es

	push	ax

	; The emulator requires a few control registers in memory to always be zero for correct
	; instruction decoding (in particular, register look-up operations). These are the
	; emulator's zero segment (ZS) and always-zero flag (XF). Because the emulated memory
	; space is uninitialised, we need to be sure these values are zero before doing anything
	; else. The instructions we need to use to set them must not rely on look-up operations.
	; So e.g. MOV to memory is out but string operations are fine.

	cld

	xor	ax, ax
	mov	di, 24
	stosw			; Set ZS = 0
	mov	di, 49
	stosb			; Set XF = 0

	; Now we can do whatever we want!

	; Set up Hercules graphics support. We start with the adapter in text mode

	push	dx

	mov	dx, 0x3b8
	mov	al, 0
	out	dx, al		; Set Hercules support to text mode

	mov	dx, 0		; The IOCCC version of the emulator also uses I/O port 0 as a graphics mode flag
	out	dx, al

	mov	dx, 0x3b4
	mov	al, 1		; Hercules CRTC "horizontal displayed" register select
	out	dx, al
	mov	dx, 0x3b5
	mov	al, 0x2d	; 0x2D = 45 (* 16) = 720 pixels wide (GRAPHICS_X)
	out	dx, al
	mov	dx, 0x3b4
	mov	al, 6		; Hercules CRTC "vertical displayed" register select
	out	dx, al
	mov	dx, 0x3b5
	mov	al, 0x57	; 0x57 = 87 (* 4) = 348 pixels high (GRAPHICS_Y)
	out	dx, al

	pop	dx

	pop	ax

	; Check cold boot/warm boot. We initialise disk parameters on cold boot only

	cmp	byte [cs:boot_state], 0	; Cold boot?
	jne	boot

	mov	byte [cs:boot_state], 1	; Set flag so next boot will be warm boot

	; First, set up the disk subsystem. Only do this on the very first startup, when
	; the emulator sets up the CX/AX registers with disk information.

	; Compute the cylinder/head/sector count for the HD disk image, if present.
	; Total number of sectors is in CX:AX, or 0 if there is no HD image. First,
	; we put it in DX:CX.

	mov	dx, cx
	mov	cx, ax

	mov	[cs:hd_secs_hi], dx
	mov	[cs:hd_secs_lo], cx

	cmp	cx, 0
	je	maybe_no_hd

	mov	word [cs:num_disks], 2
	jmp	calc_hd

maybe_no_hd:

	cmp	dx, 0
	je	no_hd

	mov	word [cs:num_disks], 2
	jmp	calc_hd

no_hd:

	mov	word [cs:num_disks], 1

calc_hd:

	mov	ax, cx
	mov	word [cs:hd_max_track], 1
	mov	word [cs:hd_max_head], 1

	cmp	dx, 0		; More than 63 total sectors? If so, we have more than 1 track.
	ja	sect_overflow
	cmp	ax, 63
	ja	sect_overflow

	mov	[cs:hd_max_sector], ax
	jmp	calc_heads

sect_overflow:

	mov	cx, 63		; Calculate number of tracks
	div	cx
	mov	[cs:hd_max_track], ax
	mov	word [cs:hd_max_sector], 63

calc_heads:

	mov	dx, 0		; More than 1024 tracks? If so, we have more than 1 head.
	mov	ax, [cs:hd_max_track]
	cmp	ax, 1024
	ja	track_overflow
	
	jmp	calc_end

track_overflow:

	mov	cx, 1024
	div	cx
	mov	[cs:hd_max_head], ax
	mov	word [cs:hd_max_track], 1024

calc_end:

	; Convert number of tracks into maximum track (0-based) and then store in INT 41
	; HD parameter table

	mov	ax, [cs:hd_max_head]
	mov	[cs:int41_max_heads], al
	mov	ax, [cs:hd_max_track]
	mov	[cs:int41_max_cyls], ax
	mov	ax, [cs:hd_max_sector]
	mov	[cs:int41_max_sect], al

	dec	word [cs:hd_max_track]
	dec	word [cs:hd_max_head]
	
; Main BIOS entry point. Zero the flags, and set up registers.

boot:	mov	ax, 0
	push	ax
	popf

	push	cs
	push	cs
	pop	ds
	pop	ss
	mov	sp, 0xf000
	
; Set up the IVT. First we zero out the table

	cld

	mov	ax, 0
	mov	es, ax
	mov	di, 0
	mov	cx, 512
	rep	stosw

; Then we load in the pointers to our interrupt handlers

	mov	di, 0
	mov	si, int_table
	mov	cx, [itbl_size]
	rep	movsb

; Set pointer to INT 41 table for hard disk

	mov	cx, int41
	mov	word [es:4*0x41], cx
	mov	cx, 0xf000
	mov	word [es:4*0x41 + 2], cx

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

; Clear video memory

	mov	ax, 0xb800
	mov	es, ax
	mov	di, 0
	mov	cx, 80*25
	mov	ax, 0x0700
	rep	stosw

; Set up some I/O ports, between 0 and FFF. Most of them we set to 0xFF, to indicate no device present

	mov	dx, 0
	mov	al, 0xFF

next_out:

	inc	dx

	cmp	dx, 0x40	; We deal with the PIT channel 0 later
	je	next_out
	cmp	dx, 0x3B8	; We deal with the Hercules port later, too
	je	next_out

	out	dx, al

	cmp	dx, 0xFFF
	jl	next_out

	mov	dx, 0x3DA	; CGA refresh port
	mov	al, 0
	out	dx, al

	mov	dx, 0x3BA	; Hercules detection port
	mov	al, 0
	out	dx, al

	mov	dx, 0x3B8	; Hercules video mode port
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

	call	vmem_driver_entry	; CGA text mode driver - documented later

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

	; A Hercules graphics adapter flips bit 7 of I/O port 3BA, every now and then, apparently!
	mov	dx, 0x3BA
	in 	al, dx
	xor	al, 0x80
	out	dx, al

	; We now need to convert the data in I/O port 0x3B8 (Hercules settings) to a new format in
	; I/O port 0, which we use in the IOCCC version of the emulator for code compactness.
	; Basically this port will contain 0 if the Hercules graphics is disabled, 2 otherwise. Note,
	; this is not used in 8086tiny.

	mov	dx, 0x3B8
	in	al, dx
	test	al, 2
	jnz	herc_gfx_mode

	; Hercules is in text mode, so set I/O port 0 to 0

	mov	al, 0
	jmp	io_init_continue

herc_gfx_mode:

	mov	al, 2

io_init_continue:

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

	int	0x1c

	iret

; ************************* INT 10h handler - video services

int10:

	cmp	ah, 0x00 ; Set video mode
	je	int10_set_vm
	cmp	ah, 0x01 ; Set cursor shape
	je	int10_set_cshape
	cmp	ah, 0x02 ; Set cursor position
	je	int10_set_cursor
	cmp	ah, 0x03 ; Get cursur position
	je	int10_get_cursor
	cmp	ah, 0x06 ; Scroll up window
	je	int10_scrollup
	cmp	ah, 0x07 ; Scroll down window
	je	int10_scrolldown
	cmp	ah, 0x08 ; Get character at cursor
	je	int10_charatcur
	cmp	ah, 0x09 ; Write char and attribute
	je	int10_write_char_attrib
	cmp	ah, 0x0e ; Write character at cursor position
	je	int10_write_char
	cmp	ah, 0x0f ; Get video mode
	je	int10_get_vm
	;cmp	ah, 0x12 ; Feature check (EGA)
	;je	int10_ega_features
	;cmp	ah, 0x1a ; Feature check
	;je	int10_features

	iret

  int10_set_vm:

	push	dx
	push	cx
	push	bx

	cmp	al, 7		; If an app tries to set Hercules text mode 7, actually set mode 3 (we do not support mode 7's video memory buffer at B000:0)
	je	int10_set_vm_3
	cmp	al, 2		; Same for text mode 2 (mono)
	je	int10_set_vm_3

	jmp	int10_set_vm_continue

  int10_set_vm_3:

	mov	al, 3

  int10_set_vm_continue:

	mov	[cs:vidmode], al

	mov	bh, 7		; Black background, white foreground
	call	clear_screen	; ANSI clear screen

	cmp	byte [cs:vidmode], 6
	je	set6
	mov	al, 0x30
	jmp	svmn

  set6:

	mov	al, 0x3f

  svmn:

	; Take Hercules adapter out of graphics mode when resetting video mode via int 10
	push	ax
	mov	dx, 0x3B8
	mov	al, 0
	out	dx, al
	pop	ax

	pop	bx
	pop	cx
	pop	dx
	iret

  int10_set_cshape:

	push	ds
	push	ax
	push	cx

	mov	ax, 0x40
	mov	ds, ax

	mov	byte [cursor_visible-bios_data], 1	; Show cursor

	and	ch, 01100000b
	cmp	ch, 00100000b
	jne	cur_visible

	mov	byte [cursor_visible-bios_data], 0	; Hide cursor

    cur_visible:

	mov	al, 0x1B
	extended_putchar_al
	mov	al, '['
	extended_putchar_al
	mov	al, '?'
	extended_putchar_al
	mov	al, '2'
	extended_putchar_al
	mov	al, '5'
	extended_putchar_al
	mov	al, 'l'
	sub	al, [cursor_visible-bios_data]
	sub	al, [cursor_visible-bios_data]
	sub	al, [cursor_visible-bios_data]
	sub	al, [cursor_visible-bios_data]
	extended_putchar_al

	pop	cx
	pop	ax
	pop	ds
	iret

  int10_set_cursor:

	push	es
	push	ax

	mov	ax, 0x40
	mov	es, ax

	cmp	dh, 24
	jb	skip_set_cur_row_max
	mov	dh, 24
	
    skip_set_cur_row_max:

     	cmp	dl, 79
	jb	skip_set_cur_col_max
	mov	dl, 79
	
    skip_set_cur_col_max:

	mov	al, 0x1B	; ANSI
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, dh		; Row number
	mov	[es:curpos_y-bios_data], al
	inc	al
	call	puts_decimal_al
	mov	al, ';'		; ANSI
	extended_putchar_al
	mov	al, dl		; Column number
	mov	[es:curpos_x-bios_data], al
	inc	al
	call	puts_decimal_al
	mov	al, 'H'		; Set cursor position command
	extended_putchar_al

	pop	ax
	pop	es
	iret

  int10_get_cursor:		; We don't really support this - just a placeholder

	push	es
	push	ax

	mov	ax, 0x40
	mov	es, ax

	mov	cx, 0x0607
	mov	dl, [es:curpos_x-bios_data]
	mov	dh, [es:curpos_y-bios_data]

	pop	ax
	pop	es

	iret

  int10_scrollup:

	push	bx
	push	cx
	push	bp
	push	ax

	mov	bp, bx		; Convert from CGA to ANSI
	mov	cl, 12
	ror	bp, cl
	and	bp, 7
	mov	bl, byte [cs:bp+colour_table]
	add	bl, 10

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, bl		; Background colour
	call	puts_decimal_al
	mov	al, 'm'		; Set cursor position command
	extended_putchar_al

	pop	ax
	pop	bp
	pop	cx
	pop	bx

	cmp	al, 0 ; Clear window
	jne	cls_partial

	cmp	cx, 0 ; Start of screen
	jne	cls_partial

	cmp	dl, 0x4f ; Clearing columns 0-79
	jne	cls_partial

	cmp	dl, 0x18 ; Clearing rows 0-24 (or more)
	jl	cls_partial

	call	clear_screen
	iret

  cls_partial:

	push 	ax
	push	bx

	; mov	bx, 0
	mov	bl, al		; Number of rows to scroll are now in bl

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al

	cmp	ch, 0		; Start row 1? Maybe full screen
	je	cls_maybe_fs
	jmp	cls_not_fs

    cls_maybe_fs:

	cmp	dh, 24		; End row 25? Full screen for sure
	je	cls_fs

    cls_not_fs:

	mov	al, ch		; Start row
	inc	al
	call	puts_decimal_al
	mov	al, ';'		; ANSI
	extended_putchar_al
	mov	al, dh		; End row
	inc	al
	call	puts_decimal_al

    cls_fs:

	mov	al, 'r'		; Set scrolling window
	extended_putchar_al

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al

	cmp	bl, 1
	jne	cls_fs_multiline

	mov	al, 'M'
	jmp	cs_fs_ml_out

cls_fs_multiline:

	mov	al, bl		; Number of rows
	call	puts_decimal_al
	mov	al, 'S'		; Scroll up

cs_fs_ml_out:

	extended_putchar_al

	pop	ax
	pop	bx

	; Update "actual" cursor position with expected value - different ANSI terminals do different things
	; to the cursor position when you scroll

	push	ax
	push	bx
	push 	dx
	push	es

	mov	ax, 0x40
	mov	es, ax

	mov	ah, 2
	mov	bh, 0
	mov	dh, [es:curpos_y-bios_data]
	mov	dl, [es:curpos_x-bios_data]
	int	10h

	pop	es
	pop	dx
	pop	bx
	pop	ax

	;push	es
	;mov	ax, 0x40
	;mov	es, ax
	;sub	byte [es:curpos_y-bios_data], bl
	;cmp	byte [es:curpos_y-bios_data], 0
	;jnl	int10_scroll_up_vmem_update

	;mov	byte [es:curpos_y-bios_data], 0

int10_scroll_up_vmem_update:

	;pop	es

	; Now, we need to update video memory

	push	bx
	push	ax

	push	ds
	push	es
	push	cx
	push	dx
	push	si
	push	di

	push	bx

	mov	bx, 0xb800
	mov	es, bx
	mov	ds, bx

	pop	bx

    cls_vmem_scroll_up_next_line:

	cmp	bl, 0
	je	cls_vmem_scroll_up_done

    cls_vmem_scroll_up_one:

	push	bx
	push	dx

	mov	ax, 0
	mov	al, ch		; Start row number is now in AX
	mov	bx, 80
	mul	bx
	add	al, cl
	adc	ah, 0		; Character number is now in AX
	mov	bx, 2
	mul	bx		; Memory location is now in AX

	pop	dx
	pop	bx

	mov	di, ax
	mov	si, ax
	add	si, 2*80	; In a moment we will copy CX words from DS:SI to ES:DI

	mov	ax, 0
	add	al, dl
	adc	ah, 0
	inc	ax
	sub	al, cl
	sbb	ah, 0		; AX now contains the number of characters from the row to copy

	cmp	ch, dh
	jae	cls_vmem_scroll_up_one_done

	;jne	vmem_scroll_up_copy_next_row

	;push	cx
	;mov	cx, ax
	;mov	ah, 0x47
	;mov	al, 0
	;cld
	;rep	stosw
	;pop	cx

	;jmp	cls_vmem_scroll_up_one_done

vmem_scroll_up_copy_next_row:

	push	cx
	mov	cx, ax		; CX is now the length (in words) of the row to copy
	cld
	rep	movsw		; Scroll the line up
	pop	cx

	inc	ch		; Move onto the next row
	jmp	cls_vmem_scroll_up_one

    cls_vmem_scroll_up_one_done:

	push	cx
	mov	cx, ax		; CX is now the length (in words) of the row to copy
	mov	ah, bh		; Attribute for new line
	mov	al, 0		; Write 0 to video memory for new characters
	cld
	rep	stosw
	pop	cx

	dec	bl		; Scroll whole text block another line
	jmp	cls_vmem_scroll_up_next_line	

    cls_vmem_scroll_up_done:

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, '0'		; Reset attributes
	extended_putchar_al
	mov	al, 'm'
	extended_putchar_al

	;int	8		; Force display update after scroll
	;int	8		; Force display update after scroll
	;int	8		; Force display update after scroll
	;int	8		; Force display update after scroll
	;int	8		; Force display update after scroll
	;int	8		; Force display update after scroll
	;int	8		; Force display update after scroll
	;int	8		; Force display update after scroll

	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	es
	pop	ds

	pop	ax
	pop	bx

	; pop	bx
	; pop	ax
	iret
	
  int10_scrolldown:

	push	bx
	push	cx
	push	bp
	push	ax

	mov	bp, bx		; Convert from CGA to ANSI
	mov	cl, 12
	ror	bp, cl
	and	bp, 7
	mov	bl, byte [cs:bp+colour_table]
	add	bl, 10

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, bl		; Background colour
	call	puts_decimal_al
	mov	al, 'm'		; Set cursor position command
	extended_putchar_al

	pop	ax
	pop	bp
	pop	cx
	pop	bx

	cmp	al, 0 ; Clear window
	jne	cls_partial_down

	cmp	cx, 0 ; Start of screen
	jne	cls_partial_down

	cmp	dl, 0x4f ; Clearing columns 0-79
	jne	cls_partial_down

	cmp	dl, 0x18 ; Clearing rows 0-24 (or more)
	jl	cls_partial_down

	call	clear_screen
	iret

  cls_partial_down:

	push 	ax
	push	bx

	mov	bx, 0
	mov	bl, al		; Number of rows to scroll are now in bl

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al

	cmp	ch, 0		; Start row 1? Maybe full screen
	je	cls_maybe_fs_down
	jmp	cls_not_fs_down

    cls_maybe_fs_down:

	cmp	dh, 24		; End row 25? Full screen for sure
	je	cls_fs_down

    cls_not_fs_down:

	mov	al, ch		; Start row
	inc	al
	call	puts_decimal_al
	mov	al, ';'		; ANSI
	extended_putchar_al
	mov	al, dh		; End row
	inc	al
	call	puts_decimal_al

    cls_fs_down:

	mov	al, 'r'		; Set scrolling window
	extended_putchar_al

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, bl		; Number of rows
	call	puts_decimal_al
	mov	al, 'T'		; Scroll down
	extended_putchar_al

	; Update "actual" cursor position with expected value - different ANSI terminals do different things
	; to the cursor position when you scroll

	push	ax
	push	bx
	push 	dx
	push	es

	mov	ax, 0x40
	mov	es, ax

	mov	ah, 2
	mov	bh, 0
	mov	dh, [es:curpos_y-bios_data]
	mov	dl, [es:curpos_x-bios_data]
	int	10h

	pop	es
	pop	dx
	pop	bx
	pop	ax

	;push	es
	;mov	ax, 0x40
	;mov	es, ax
	;add	byte [es:curpos_y-bios_data], bl
	;cmp	byte [es:curpos_y-bios_data], 25
	;jna	int10_scroll_down_vmem_update

	;mov	byte [es:curpos_y-bios_data], 25

int10_scroll_down_vmem_update:

	;pop	es

	; Now, we need to update video memory

	push	ds
	push	es
	push	cx
	push	dx
	push	si
	push	di

	push	bx

	mov	bx, 0xb800
	mov	es, bx
	mov	ds, bx

	pop	bx

    cls_vmem_scroll_down_next_line:

	cmp	bl, 0
	je	cls_vmem_scroll_down_done

    cls_vmem_scroll_down_one:

	push	bx
	push	dx

	mov	ax, 0
	mov	al, dh		; End row number is now in AX
	mov	bx, 80
	mul	bx
	add	al, cl
	adc	ah, 0		; Character number is now in AX
	mov	bx, 2
	mul	bx		; Memory location (start of final row) is now in AX

	pop	dx
	pop	bx

	mov	di, ax
	mov	si, ax
	sub	si, 2*80	; In a moment we will copy CX words from DS:SI to ES:DI

	mov	ax, 0
	add	al, dl
	adc	ah, 0
	inc	ax
	sub	al, cl
	sbb	ah, 0		; AX now contains the number of characters from the row to copy

	cmp	ch, dh
	jae	cls_vmem_scroll_down_one_done

	push	cx
	mov	cx, ax		; CX is now the length (in words) of the row to copy
	rep	movsw		; Scroll the line down
	pop	cx

	dec	dh		; Move onto the next row
	jmp	cls_vmem_scroll_down_one

    cls_vmem_scroll_down_one_done:

	push	cx
	mov	cx, ax		; CX is now the length (in words) of the row to copy
	mov	ah, bh		; Attribute for new line
	mov	al, 0		; Write 0 to video memory for new characters
	rep	stosw
	pop	cx

	dec	bl		; Scroll whole text block another line
	jmp	cls_vmem_scroll_down_next_line	

    cls_vmem_scroll_down_done:

	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	es
	pop	ds

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, '0'		; Reset attributes
	extended_putchar_al
	mov	al, 'm'
	extended_putchar_al

	pop	bx
	pop	ax
	iret

  int10_charatcur:

	; This returns the character at the cursor. It is completely dysfunctional,
	; and only works at all if the character has previously been written following
	; an int 10/ah = 2 call to set the cursor position. Added just to support
	; GWBASIC.
	
	push	ds
	push	es
	push	bx

	mov	bx, 0x40
	mov	es, bx

	mov	bx, 0xb000
	mov	ds, bx

	mov	bx, 0
	add	bl, [es:curpos_x-bios_data]
	add	bl, [es:curpos_x-bios_data]

	mov	ah, 0
	mov	al, [bx]

	pop	bx
	pop	es
	pop	ds

	iret

  i10_unsup:

	iret

  int10_write_char:

	; First we kind of write the character to "video memory". This is so that
	; we can later retrieve it using the get character at cursor function,
	; which GWBASIC uses.

	push	ds
	push	es
	push	bx

	mov	bx, 0x40
	mov	es, bx

	mov	bx, 0xb000
	mov	ds, bx

	mov	bx, 0
	mov	bl, [es:curpos_x-bios_data]
	shl	bx, 1
	mov	[bx], al

	cmp	al, 0x08
	jne	int10_write_char_inc_x

	dec	byte [es:curpos_x-bios_data]
	cmp	byte [es:curpos_x-bios_data], 0
	jg	int10_write_char_done

	mov	byte [es:curpos_x-bios_data], 0    
	jmp	int10_write_char_done

    int10_write_char_inc_x:

	cmp	al, 0x0A	; New line?
	je	int10_write_char_newline

	cmp	al, 0x0D	; Carriage return?
	jne	int10_write_char_not_cr

	mov	byte [es:curpos_x-bios_data],0
	jmp	int10_write_char_done

    int10_write_char_not_cr:

	inc	byte [es:curpos_x-bios_data]
	cmp	byte [es:curpos_x-bios_data], 80
	jge	int10_write_char_newline
	jmp	int10_write_char_done

    int10_write_char_newline:

	mov	byte [es:curpos_x-bios_data], 0
	inc	byte [es:curpos_y-bios_data]

	cmp	byte [es:curpos_y-bios_data], 25
	jb	int10_write_char_done
	mov	byte [es:curpos_y-bios_data], 24

	push	cx
	push	dx

	mov	bx, 0x0701
	mov	cx, 0
	mov	dx, 0x184f

	pushf
	push	cs
	call	int10_scroll_up_vmem_update

	pop	dx
	pop	cx

    int10_write_char_done:

	pop	bx
	pop	es
	pop	ds

	extended_putchar_al

	iret

  int10_write_char_attrib:

	; First we write the character to a fake "video memory" location. This is so that
	; we can later retrieve it using the get character at cursor function, which
	; GWBASIC uses in this way to see what has been typed.

	push	ds
	push	es
	push	cx
	push	bp
	push	bx
	push	bx

	mov	bx, 0x40
	mov	es, bx

	mov	bx, 0xb000
	mov	ds, bx

	mov	bx, 0
	mov	bl, [es:curpos_x-bios_data]
	shl	bx, 1
	mov	[bx], al

	pop	bx

	push	bx
	push	ax

	mov	bh, bl
	and	bl, 7		; Foreground colour now in bl

	mov	bp, bx		; Convert from CGA to ANSI
	and	bp, 0xff
	mov	bl, byte [cs:bp+colour_table]

	and	bh, 8		; Bright attribute now in bh
cpu	186
	shr	bh, 3
cpu	8086

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, bh		; Bright attribute
	call	puts_decimal_al
	mov	al, ';'
	extended_putchar_al
	mov	al, bl		; Foreground colour
	call	puts_decimal_al
	mov	al, 'm'		; Set cursor position command
	extended_putchar_al

	pop	ax
	pop	bx

	push	bx
	push	ax

	mov	bh, bl
	shr	bl, 1
	shr	bl, 1
	shr	bl, 1
	shr	bl, 1
	and	bl, 7		; Background colour now in bl

	mov	bp, bx		; Convert from CGA to ANSI
	and	bp, 0xff
	mov	bl, byte [cs:bp+colour_table]

	add	bl, 10
	rol	bh, 1
	and	bh, 1		; Bright attribute now in bh (not used right now)

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, bl		; Background colour
	call	puts_decimal_al
	mov	al, 'm'		; Set cursor position command
	extended_putchar_al
	
	pop	ax
	pop	bx

	push	ax

    out_another_char:

	extended_putchar_al
	dec	cx
	cmp	cx, 0
	jne	out_another_char

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, '0'		; Reset attributes
	extended_putchar_al
	mov	al, 'm'
	extended_putchar_al

	cmp	al, 0x08
	jne	int10_write_char_attrib_inc_x

	dec	byte [es:curpos_x-bios_data]
	cmp	byte [es:curpos_x-bios_data], 0
	jg	int10_write_char_attrib_done

	mov	byte [es:curpos_x-bios_data], 0
	jmp	int10_write_char_attrib_done

    int10_write_char_attrib_inc_x:

	cmp	al, 0x0A	; New line?
	je	int10_write_char_attrib_newline

	cmp	al, 0x0D	; Carriage return?
	jne	int10_write_char_attrib_not_cr

	mov	byte [es:curpos_x-bios_data],0
	jmp	int10_write_char_attrib_done

    int10_write_char_attrib_not_cr:

	inc	byte [es:curpos_x-bios_data]
	cmp	byte [es:curpos_x-bios_data], 80
	jge	int10_write_char_attrib_newline
	jmp	int10_write_char_attrib_done

    int10_write_char_attrib_newline:

	mov	byte [es:curpos_x-bios_data], 0
	inc	byte [es:curpos_y-bios_data]

	cmp	byte [es:curpos_y-bios_data], 25
	jb	int10_write_char_attrib_done
	mov	byte [es:curpos_y-bios_data], 24

	push	cx
	push	dx

	mov	bl, 1
	mov	cx, 0
	mov	dx, 0x184f

	pushf
	push	cs
	call	int10_scroll_up_vmem_update

	pop	dx
	pop	cx

    int10_write_char_attrib_done:

	pop	ax
	pop	bx
	pop	bp
	pop	cx
	pop	es
	pop	ds

	iret

  int10_get_vm:

	mov	ah, 80 ; Number of columns
	mov	al, [cs:vidmode]
	mov	bh, 0

	iret

  ;int10_ega_features:

;	cmp	bl, 0x10
;	jne	i10_unsup
;
;	mov	bx, 0
;	mov	cx, 0x0005
;	iret
;
 ; int10_features:
;
;	; Signify we have CGA display
;
;	mov	al, 0x1a
;	mov	bx, 0x0202
;	iret

; ************************* INT 11h - get equipment list

int11:	
	mov	ax, [cs:equip]
	iret

; ************************* INT 12h - return memory size

int12:	
	mov	ax, 0x280 ; 640K conventional memory
	iret

; ************************* INT 13h handler - disk services

int13:
	cmp	ah, 0x00 ; Reset disk
	je	int13_reset_disk
	cmp	ah, 0x01 ; Get last status
	je	int13_last_status

	cmp	dl, 0x80 ; Hard disk being queried?
	jne	i13_diskok

	; Now, need to check an HD is installed
	cmp	word [cs:num_disks], 2
	jge	i13_diskok

	; No HD, so return an error
	mov	ah, 15 ; Report no such drive
	jmp	reach_stack_stc

  i13_diskok:

	cmp	ah, 0x02 ; Read disk
	je	int13_read_disk
	cmp	ah, 0x03 ; Write disk
	je	int13_write_disk
	cmp	ah, 0x04 ; Verify disk
	je	int13_verify
	cmp	ah, 0x05 ; Format track - does nothing here
	je	int13_format
	cmp	ah, 0x08 ; Get drive parameters (hard disk)
	je	int13_getparams
	cmp	ah, 0x10 ; Check if drive ready (hard disk)
	je	int13_hdready
	cmp	ah, 0x15 ; Get disk type
	je	int13_getdisktype
	cmp	ah, 0x16 ; Detect disk change
	je	int13_diskchange

	mov	ah, 1 ; Invalid function
	jmp	reach_stack_stc

	iret

  int13_reset_disk:

	jmp	reach_stack_clc

  int13_last_status:

	mov	ah, [cs:disk_laststatus]
	je	ls_no_error

	stc
	iret

    ls_no_error:

	clc
	iret

  int13_read_disk:

	push	dx

	cmp	dl, 0 ; Floppy 0
	je	i_flop_rd
	cmp	dl, 0x80 ; HD
	je	i_hd_rd

	pop	dx
	mov	ah, 1
	jmp	reach_stack_stc

    i_flop_rd:

	mov	dl, 1		; Floppy disk file handle is stored at j[1] in emulator
	jmp	i_rd

    i_hd_rd:

	mov	dl, 0		; Hard disk file handle is stored at j[0] in emulator

    i_rd: 

	push	si
	push	bp

	; Convert head/cylinder/sector number to byte offset in disk image

	call	chs_to_abs

	; Now, SI:BP contains the absolute sector offset of the block. We then multiply by 512 to get the offset into the disk image

	mov	ah, 0
	cpu	186
	shl	ax, 9
	extended_read_disk
	shr	ax, 9
	cpu	8086
	mov	ah, 0x02	; Put read code back

	cmp	al, 0
	je	rd_error

	; Read was successful. Now, check if we have read the boot sector. If so, we want to update
	; our internal table of sectors/track to match the disk format

	cmp	dx, 1		; FDD?
	jne	rd_noerror
	cmp	cx, 1		; First sector?
	jne	rd_noerror

	push	ax
	mov	al, [es:bx+24]	; Number of SPT in floppy disk BPB
	cmp	al, 0		; If disk is unformatted, do not update the table
	jne	rd_update_spt
	pop	ax

	jmp	rd_noerror

    rd_update_spt:

	mov	[cs:int1e_spt], al
	pop	ax

    rd_noerror:

	clc
	mov	ah, 0 ; No error
	jmp	rd_finish

    rd_error:

	stc
	mov	ah, 4 ; Sector not found

    rd_finish:

	pop	bp
	pop	si
	pop	dx

	mov	[cs:disk_laststatus], ah
	jmp	reach_stack_carry

  int13_write_disk:

	push	dx

	cmp	dl, 0 ; Floppy 0
	je	i_flop_wr
	cmp	dl, 0x80 ; HD
	je	i_hd_wr

	pop	dx
	mov	ah, 1
	jmp	reach_stack_stc

    i_flop_wr:

	mov	dl, 1		; Floppy disk file handle is stored at j[1] in emulator
	jmp	i_wr

    i_hd_wr:

	mov	dl, 0		; Hard disk file handle is stored at j[0] in emulator

    i_wr:

	push	si
	push	bp
	push	cx
	push	di

	; Convert head/cylinder/sector number to byte offset in disk image

	call	chs_to_abs

	; Signal an error if we are trying to write beyond the end of the disk
	
	cmp	dl, 0 ; Hard disk?
	jne	wr_fine ; No - no need for disk sector valid check - NOTE: original submission was JNAE which caused write problems on floppy disk

	; First, we add the number of sectors we are trying to write from the absolute
	; sector number returned by chs_to_abs. We need to have at least this many
	; sectors on the disk, otherwise return a sector not found error.

	mov	cx, bp
	mov	di, si

	mov	ah, 0
	add	cx, ax
	adc	di, 0

	cmp	di, [cs:hd_secs_hi]
	ja	wr_error
	jb	wr_fine
	cmp	cx, [cs:hd_secs_lo]
	ja	wr_error

wr_fine:

	mov	ah, 0
	cpu	186
	shl	ax, 9
	extended_write_disk
	shr	ax, 9
	cpu	8086
	mov	ah, 0x03	; Put write code back

	cmp	al, 0
	je	wr_error

	clc
	mov	ah, 0 ; No error
	jmp	wr_finish

    wr_error:

	stc
	mov	ah, 4 ; Sector not found

    wr_finish:

	pop	di
	pop	cx
	pop	bp
	pop	si
	pop	dx

	mov	[cs:disk_laststatus], ah
	jmp	reach_stack_carry

  int13_verify:

	mov	ah, 0
	jmp	reach_stack_clc

  int13_getparams:

	cmp 	dl, 0
	je	i_gp_fl
	cmp	dl, 0x80
	je	i_gp_hd

	mov	ah, 0x01
	mov	[cs:disk_laststatus], ah
	jmp	reach_stack_stc

    i_gp_fl:

	mov	ax, 0
	mov	bx, 4
	mov	cx, 0x4f12
	mov	dx, 0x0101

	mov	byte [cs:disk_laststatus], 0
	jmp	reach_stack_clc

    i_gp_hd:

	mov	ax, 0
	mov	bx, 0
	mov	dl, 1
	mov	dh, [cs:hd_max_head]
	mov	cx, [cs:hd_max_track]
	ror	ch, 1
	ror	ch, 1
	add	ch, [cs:hd_max_sector]
	xchg	ch, cl

	mov	byte [cs:disk_laststatus], 0
	jmp	reach_stack_clc

  int13_hdready:

	cmp	byte [cs:num_disks], 2	; HD present?
	jne	int13_hdready_nohd
	cmp	dl, 0x80		; Checking first HD?
	jne	int13_hdready_nohd

	mov	ah, 0
	jmp	reach_stack_clc

    int13_hdready_nohd:

	jmp	reach_stack_stc

  int13_format:

	mov	ah, 0
	jmp	reach_stack_clc

  int13_getdisktype:

	cmp	dl, 0 ; Floppy
	je	gdt_flop
	cmp	dl, 0x80 ; HD
	je	gdt_hd

	mov	ah, 15 ; Report no such drive
	mov	[cs:disk_laststatus], ah
	jmp	reach_stack_stc

    gdt_flop:

	mov	ah, 1
	jmp	reach_stack_clc

    gdt_hd:

	mov	ah, 3
	mov	cx, [cs:hd_secs_hi]
	mov	dx, [cs:hd_secs_lo]
	jmp	reach_stack_clc

  int13_diskchange:

	mov	ah, 0 ; Disk not changed
	jmp	reach_stack_clc

; ************************* INT 14h - serial port functions

int14:
	cmp	ah, 0
	je	int14_init

	iret

  int14_init:

	mov	ax, 0

	iret

; ************************* INT 15h - get system configuration

int15:	; Here we do not support any of the functions, and just return
	; a function not supported code - like the original IBM PC/XT does.

	; cmp	ah, 0xc0
	; je	int15_sysconfig
	; cmp	ah, 0x41
	; je	int15_waitevent
	; cmp	ah, 0x4f
	; je	int15_intercept
	; cmp	ah, 0x88
	; je	int15_getextmem

; Otherwise, function not supported

	mov	ah, 0x86

	jmp	reach_stack_stc

;  int15_sysconfig: ; Return address of system configuration table in ROM
;
;	mov	bx, 0xf000
;	mov	es, bx
;	mov	bx, rom_config
;	mov	ah, 0
;
;	jmp	reach_stack_clc
;
;  int15_waitevent: ; Events not supported
;
;	mov	ah, 0x86
;
;	jmp	reach_stack_stc
;
;  int15_intercept: ; Keyboard intercept
;
;	jmp	reach_stack_stc
;
;  int15_getextmem: ; Extended memory not supported
;
;	mov	ah,0x86
;
;	jmp	reach_stack_stc

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

; ************************* INT 17h handler - printer

int17:
	cmp	ah, 0x01
	je	int17_initprint ; Initialise printer

	iret

  int17_initprint:

	mov	ah, 0
	iret

; ************************* INT 19h = reboot

int19:
	jmp	boot

; ************************* INT 1Ah - clock

int1a:
	cmp	ah, 0
	je	int1a_getsystime ; Get ticks since midnight (used for RTC time)
	cmp	ah, 2
	je	int1a_gettime ; Get RTC time (not actually used by DOS)
	cmp	ah, 4
	je	int1a_getdate ; Get RTC date
	cmp	ah, 0x0f
	je	int1a_init    ; Initialise RTC

	iret

  int1a_getsystime:

	push	ax
	push	bx
	push	ds
	push	es

	push	cs
	push	cs
	pop	ds
	pop	es

	mov	bx, timetable

	extended_get_rtc

	mov	ax, 182  ; Clock ticks in 10 seconds
	mul	word [tm_sec]
	mov	bx, 10
	mov	dx, 0
	div	bx ; AX now contains clock ticks in seconds counter
	mov	[tm_sec], ax

	mov	ax, 1092 ; Clock ticks in a minute
	mul	word [tm_min] ; AX now contains clock ticks in minutes counter
	mov	[tm_min], ax
	
	mov	ax, 65520 ; Clock ticks in an hour
	mul	word [tm_hour] ; DX:AX now contains clock ticks in hours counter

	add	ax, [tm_sec] ; Add seconds in to AX
	adc	dx, 0 ; Carry into DX if necessary
	add	ax, [tm_min] ; Add minutes in to AX
	adc	dx, 0 ; Carry into DX if necessary

	push	dx
	push	ax
	pop	dx
	pop	cx

	pop	es
	pop	ds
	pop	bx
	pop	ax

	mov	al, 0
	iret

  int1a_gettime:

	; Return the system time in BCD format. DOS doesn't use this, but we need to return
	; something or the system thinks there is no RTC.

	push	ds
	push	es
	push	ax
	push	bx

	push	cs
	push	cs
	pop	ds
	pop	es

	mov	bx, timetable

	extended_get_rtc

	mov	ax, 0
	mov	cx, [tm_hour]
	call	hex_to_bcd
	mov	bh, al		; Hour in BCD is in BH

	mov	ax, 0
	mov	cx, [tm_min]
	call	hex_to_bcd
	mov	bl, al		; Minute in BCD is in BL

	mov	ax, 0
	mov	cx, [tm_sec]
	call	hex_to_bcd
	mov	dh, al		; Second in BCD is in DH

	mov	dl, 0		; Daylight saving flag = 0 always

	mov	cx, bx		; Hour:minute now in CH:CL

	pop	bx
	pop	ax
	pop	es
	pop	ds

	jmp	reach_stack_clc

  int1a_getdate:

	; Return the system date in BCD format.

	push	ds
	push	es
	push	bx
	push	ax

	push	cs
	push	cs
	pop	ds
	pop	es

	mov	bx, timetable

	extended_get_rtc

	mov	ax, 0x1900
	mov	cx, [tm_year]
	call	hex_to_bcd
	mov	cx, ax
	push	cx

	mov	ax, 1
	mov	cx, [tm_mon]
	call	hex_to_bcd
	mov	dh, al

	mov	ax, 0
	mov	cx, [tm_mday]
	call	hex_to_bcd
	mov	dl, al

	pop	cx
	pop	ax
	pop	bx
	pop	es
	pop	ds

	jmp	reach_stack_clc

  int1a_init:

	jmp	reach_stack_clc

; ************************* INT 1Ch - the other timer interrupt

int1c:

	iret

; ************************* INT 1Eh - diskette parameter table

int1e:

		db 0xdf ; Step rate 2ms, head unload time 240ms
		db 0x02 ; Head load time 4 ms, non-DMA mode 0
		db 0x25 ; Byte delay until motor turned off
		db 0x02 ; 512 bytes per sector
int1e_spt	db 0xff	; 18 sectors per track (1.44MB)
		db 0x1B ; Gap between sectors for 3.5" floppy
		db 0xFF ; Data length (ignored)
		db 0x54 ; Gap length when formatting
		db 0xF6 ; Format filler byte
		db 0x0F ; Head settle time (1 ms)
		db 0x08 ; Motor start time in 1/8 seconds

; ************************* INT 41h - hard disk parameter table

int41:

int41_max_cyls	dw 0
int41_max_heads	db 0
		dw 0
		dw 0
		db 0
		db 11000000b
		db 0
		db 0
		db 0
		dw 0
int41_max_sect	db 0
		db 0

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

num_disks	dw 0	; Number of disks present
hd_secs_hi	dw 0	; Total sectors on HD (high word)
hd_secs_lo	dw 0	; Total sectors on HD (low word)
hd_max_sector	dw 0	; Max sector number on HD
hd_max_track	dw 0	; Max track number on HD
hd_max_head	dw 0	; Max head number on HD
drive_tracks_temp dw 0
drive_sectors_temp dw 0
drive_heads_temp  dw 0
drive_num_temp    dw 0
boot_state	db 0
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
int18:
int1b:
int1d:

iret

; ************ Function call library ************

; Hex to BCD routine. Input is AX in hex (can be 0), and adds CX in hex to it, forming a BCD output in AX.

hex_to_bcd:

	push	bx

	jcxz	h2bfin

  h2bloop:

	inc	ax

	; First process the low nibble of AL
	mov	bh, al
	and	bh, 0x0f
	cmp	bh, 0x0a
	jne	c1
	add	ax, 0x0006

	; Then the high nibble of AL
  c1:
	mov	bh, al
	and	bh, 0xf0
	cmp	bh, 0xa0
	jne	c2
	add	ax, 0x0060

	; Then the low nibble of AH
  c2:	
	mov	bh, ah
	and	bh, 0x0f
	cmp	bh, 0x0a
	jne	c3
	add	ax, 0x0600

  c3:	
	loop	h2bloop
  h2bfin:
	pop	bx
	ret

; Takes a number in AL (from 0 to 99), and outputs the value in decimal using extended_putchar_al.

puts_decimal_al:

	push	ax
	
	aam
	add	ax, 0x3030	; '00'
	
	xchg	ah, al		; First digit is now in AL
	cmp	al, 0x30
	je	pda_2nd		; First digit is zero, so print only 2nd digit

	extended_putchar_al	; Print first digit

  pda_2nd:
	xchg	ah, al		; Second digit is now in AL

	extended_putchar_al	; Print second digit

	pop	ax
	ret

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

; Convert CHS disk position (in CH, CL and DH) to absolute sector number in BP:SI
; Floppy disks have 512 bytes per sector, 9/18 sectors per track, 2 heads. DH is head number (1 or 0), CH bits 5..0 is
; sector number, CL7..6 + CH7..0 is 10-bit cylinder/track number. Hard disks have 512 bytes per sector, but a variable
; number of tracks and heads.

chs_to_abs:

	push	ax	
	push	bx
	push	cx
	push	dx

	mov	[cs:drive_num_temp], dl

	; First, we extract the track number from CH and CL.

	push	cx
	mov	bh, cl
	mov	cl, 6
	shr	bh, cl
	mov	bl, ch

	; Multiply track number (now in BX) by the number of heads

	cmp	byte [cs:drive_num_temp], 1 ; Floppy disk?

	push	dx

	mov	dx, 0
	xchg	ax, bx

	jne	chs_hd

	shl	ax, 1 ; Multiply by 2 (number of heads on FD)
	push	ax
	xor	ax, ax
	mov	al, [cs:int1e_spt]
	mov	[cs:drive_sectors_temp], ax ; Retrieve sectors per track from INT 1E table
	pop	ax

	jmp	chs_continue

chs_hd:

	mov	bp, [cs:hd_max_head]
	inc	bp
	mov	[cs:drive_heads_temp], bp

	mul	word [cs:drive_heads_temp] ; HD, so multiply by computed head count

	mov	bp, [cs:hd_max_sector] ; We previously calculated maximum HD track, so number of tracks is 1 more
	mov	[cs:drive_sectors_temp], bp

chs_continue:

	xchg	ax, bx

	pop	dx

	xchg	dh, dl
	mov	dh, 0
	add	bx, dx

	mov	ax, [cs:drive_sectors_temp]
	mul	bx

	; Now we extract the sector number (from 1 to 63) - for some reason they start from 1

	pop	cx
	mov	ch, 0
	and	cl, 0x3F
	dec	cl

	add	ax, cx
	adc	dx, 0
	mov	bp, ax
	mov	si, dx

	; Now, SI:BP contains offset into disk image file (FD or HD)

	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret

; Clear screen using ANSI codes. Also clear video memory with attribute in BH

clear_screen:

	push	ax

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, 'r'		; Set scrolling window
	extended_putchar_al

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, '0'		; Reset attributes
	extended_putchar_al
	mov	al, 'm'		; Reset attributes
	extended_putchar_al

	push	bx
	push	cx
	push	bp
	push	ax
	push	es

	mov	bp, bx		; Convert from CGA to ANSI
	mov	cl, 12
	ror	bp, cl
	and	bp, 7
	mov	bl, byte [cs:bp+colour_table]
	add	bl, 10

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, bl		; Background colour
	call	puts_decimal_al
	mov	al, 'm'		; Set cursor position command
	extended_putchar_al

	mov	ax, 0x40
	mov	es, ax
	mov	byte [es:curpos_x-bios_data], 0
	mov	byte [es:curpos_y-bios_data], 0

	pop	es
	pop	ax
	pop	bp
	pop	cx
	pop	bx

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, '2'		; Clear screen
	extended_putchar_al
	mov	al, 'J'
	extended_putchar_al

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, '1'		; Cursor row 1
	extended_putchar_al
	mov	al, ';'
	extended_putchar_al
	mov	al, '1'		; Cursor column 1
	extended_putchar_al
	mov	al, 'H'		; Set cursor
	extended_putchar_al

	push	es
	push	di
	push	cx

	cld
	mov	ax, 0xb800
	mov	es, ax
	mov	di, 0
	mov	al, 0
	mov	ah, bh
	mov	cx, 80*25
	rep	stosw

	pop	cx
	pop	di
	pop	es

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

; Reaches up into the stack before the end of an interrupt handler, and sets the carry flag

reach_stack_stc:

	xchg	bp, sp
	or	word [bp+4], 1
	xchg	bp, sp
	iret

; Reaches up into the stack before the end of an interrupt handler, and clears the carry flag

reach_stack_clc:

	xchg	bp, sp
	and	word [bp+4], 0xfffe
	xchg	bp, sp
	iret

; Reaches up into the stack before the end of an interrupt handler, and returns with the current
; setting of the carry flag

reach_stack_carry:

	jc	reach_stack_stc
	jmp	reach_stack_clc

; This is the VMEM driver, to support direct video memory access in 80x25 colour CGA mode.
; It scans through CGA video memory at address B800:0, and if there is anything there (i.e.
; applications are doing direct video memory writes), converts the buffer to a sequence of
; ANSI terminal codes to render the screen output.
;
; Note: this destroys all registers. It is the responsibility of the caller to save/restore
; them.

vmem_driver_entry:

	cmp	byte [cs:in_update], 1
	je	just_finish		; If we are already in the middle of an update, just pass INT 8 on. Needed for re-entrancy.
	
	inc	byte [cs:int8_ctr]
	cmp	byte [cs:int8_ctr], 5	; Only do this once every 5 timer ticks
	je	gmode_test

just_finish:

	ret

gmode_test:

	mov	dx, 0x3b8		; Do not update if in Hercules graphics mode
	in	al, dx
	test	al, 2
	jz	vram_zero_check

	ret

vram_zero_check:			; Check if video memory is blank - if so, do nothing
	
	mov	byte [cs:int8_ctr], 0	
	mov	byte [cs:in_update], 1

	sti

	cld
	mov	bx, 0xb800
	mov	es, bx
	mov	cx, 0x7d0
	mov	ax, 0x0700
	mov	di, 0

	repz	scasw
	cmp	cx, 0
	jne	vram_update		; CX != 0 so something has been written to video RAM

	mov	byte [cs:in_update], 0
	ret

vram_update:

	mov	bx, 0x40
	mov	ds, bx

	mov	byte [cs:int_curpos_x], 0xff
	mov	byte [cs:int_curpos_y], 0xff

	cmp	byte [cursor_visible-bios_data], 0
	je	dont_hide_cursor

	mov	al, 0x1B
	extended_putchar_al
	mov	al, '['
	extended_putchar_al
	mov	al, '?'
	extended_putchar_al
	mov	al, '2'
	extended_putchar_al
	mov	al, '5'
	extended_putchar_al
	mov	al, 'l'
	extended_putchar_al

dont_hide_cursor:

	mov	byte [cs:last_attrib], 0xff

	mov	bx, 0xb800
	mov	ds, bx

	; Set up the initial cursor coordinates. Since the first thing we do is increment the cursor
	; position, this initial position is actually off the screen

	mov	bp, -1		; Row number
	mov	si, 79		; Column number
	mov	di, -2		; Combined offset

disp_loop:

	; Advance to next column

	add	di, 2
	inc	si
	cmp	si, 80
	jne	cont

	; Column is 80, so set to 0 and advance a line

	mov	si, 0
	inc	bp

	; Bottom of the screen reached already? If so, we're done

	cmp	bp, 25
	je	restore_cursor

cont:
	cmp	byte [di], 0		; Ignore null characters in video memory
	je	disp_loop

	mov	ax, bp
	mov	bx, si
	mov	dh, al
	mov	dl, bl

ansi_set_cur_pos:

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, dh		; Row number
	inc	al
	call	puts_decimal_al
	mov	al, ';'		; ANSI
	extended_putchar_al
	mov	al, dl		; Column number
	inc	al
	call	puts_decimal_al
	mov	al, 'H'		; Set cursor position command
	extended_putchar_al

skip_set_cur_pos:

	mov	[cs:int_curpos_y], dh
	mov	[cs:int_curpos_x], dl

	mov	bl, [di+1]
	cmp	bl, [cs:last_attrib]
	je	skip_attrib

	mov	[cs:last_attrib], bl

	push	bx

	mov	bh, bl
	and	bl, 7		; Foreground colour now in bl

	push	bp
	mov	bp, bx		; Convert from CGA to ANSI
	and	bp, 7
	mov	bl, byte [cs:bp+colour_table]
	pop	bp

	and	bh, 8		; Bright attribute now in bh
	cpu	186
	shr	bh, 3
	cpu	8086

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, bh		; Bright attribute
	call	puts_decimal_al
	mov	al, ';'
	extended_putchar_al
	mov	al, bl		; Foreground colour
	call	puts_decimal_al
	mov	al, ';'
	extended_putchar_al

	pop	bx

	cpu	186
	shr	bl, 4
	cpu	8086
	and	bl, 7		; Background colour now in bl

	push	bp
	mov	bp, bx		; Convert from CGA to ANSI
	and	bp, 7
	mov	bl, byte [cs:bp+colour_table]
	pop	bp

	add	bl, 10
	mov	al, bl		; Background colour
	call	puts_decimal_al
	mov	al, 'm'		; Set cursor attribute command
	extended_putchar_al

skip_attrib:

	mov	al, [di]

	cmp	al, 7		; Convert BEL character to 0xFE
	jne	check_space1
	mov	al, 0xFE

check_space1:

	cmp	al, 4		; Convert DIAMOND character to 0xFE
	jne	just_show_it
	mov	al, 0xFE

just_show_it:

	extended_putchar_al

	jmp	disp_loop

restore_cursor:

	mov	bx, 0x40
	mov	ds, bx

	mov	ah, 2
	mov	bh, 0
	mov	dh, [curpos_y-bios_data]
	mov	dl, [curpos_x-bios_data]
	int	10h

	mov	al, 0x1B	; Escape
	extended_putchar_al
	mov	al, '['		; ANSI
	extended_putchar_al
	mov	al, '0'		; Reset attributes
	extended_putchar_al
	mov	al, 'm'
	extended_putchar_al

	cmp	byte [cursor_visible-bios_data], 0
	je	vmem_done

	mov	al, 0x1B
	extended_putchar_al
	mov	al, '['
	extended_putchar_al
	mov	al, '?'
	extended_putchar_al
	mov	al, '2'
	extended_putchar_al
	mov	al, '5'
	extended_putchar_al
	mov	al, 'h'
	extended_putchar_al

vmem_done:

	mov	byte [cs:in_update], 0
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
ending:		times (0xff-($-com1addr)) db	0

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

; Colour table for converting CGA video memory colours to ANSI colours

colour_table	db	30, 34, 32, 36, 31, 35, 33, 37

; Internal variables for VMEM driver

int8_ctr	db	0
in_update	db	0
last_attrib	db	0
int_curpos_x	db	0
int_curpos_y	db	0

; Int 8 call counter - used for timer slowdown

int8_call_cnt	db	0

; Now follow the tables for instruction decode helping

; R/M mode tables

rm_mode0_reg1	db	3, 3, 5, 5, 6, 7, 12, 3
rm_mode0_reg2	db	6, 7, 6, 7, 12, 12, 12, 12
rm_mode0_disp	db	0, 0, 0, 0, 0, 0, 1, 0
rm_mode0_dfseg	db	11, 11, 10, 10, 11, 11, 11, 11

rm_mode12_reg1	db	3, 3, 5, 5, 6, 7, 5, 3
rm_mode12_reg2	db	6, 7, 6, 7, 12, 12, 12, 12
rm_mode12_disp	db	1, 1, 1, 1, 1, 1, 1, 1
rm_mode12_dfseg	db	11, 11, 10, 10, 11, 11, 10, 11

; Opcode decode tables

xlat_ids	db	0, 1, 2, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 47, 17, 17, 18, 19, 20, 19, 21, 22, 21, 22, 23, 53, 24, 25, 26, 25, 25, 26, 25, 26, 27, 28, 27, 28, 27, 29, 27, 29, 48, 30, 31, 32, 53, 33, 34, 35, 36, 37, 37, 38, 39, 40, 19, 41, 42, 43, 44, 53, 53, 45, 46, 46, 46, 46, 46, 46, 52, 52, 12
i_opcodes	db	17, 17, 17, 17, 8, 8, 49, 50, 18, 18, 18, 18, 9, 9, 51, 64, 19, 19, 19, 19, 10, 10, 52, 53, 20, 20, 20, 20, 11, 11, 54, 55, 21, 21, 21, 21, 12, 12, 56, 57, 22, 22, 22, 22, 13, 13, 58, 59, 23, 23, 23, 23, 14, 14, 60, 61, 24, 24, 24, 24, 15, 15, 62, 63, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 93, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 16, 16, 16, 31, 31, 48, 48, 25, 25, 25, 25, 26, 26, 26, 26, 32, 32, 32, 32, 32, 32, 32, 32, 65, 66, 67, 68, 69, 70, 71, 72, 27, 27, 27, 27, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 38, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 94, 94, 39, 39, 73, 74, 40, 40, 0, 0, 41, 41, 75, 76, 77, 78, 28, 28, 28, 28, 79, 80, 81, 82, 47, 47, 47, 47, 47, 47, 47, 47, 29, 29, 29, 29, 42, 42, 43, 43, 30, 30, 30, 30, 44, 44, 45, 45, 83, 0, 46, 46, 84, 85, 7, 7, 86, 87, 88, 89, 90, 91, 6, 6
ex_data  	db	21, 0, 0, 1, 0, 0, 0, 21, 0, 1, 2, 3, 4, 5, 6, 7, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 0, 0, 43, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 1, 0, 0, 1, 1, 0, 3, 0, 8, 8, 9, 10, 10, 11, 11, 8, 27, 9, 39, 10, 2, 11, 0, 36, 0, 0, 0, 0, 0, 0, 255, 0, 16, 22, 0, 255, 48, 2, 255, 255, 40, 11, 1, 2, 40, 80, 81, 92, 93, 94, 95, 0, 21, 1
std_szp  	db	0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0
std_ao   	db	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0
std_o0c0	db	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
base_size	db	2, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 0, 2, 1, 1, 1, 1, 1, 1, 1, 0, 2, 0, 2, 2, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 1, 1, 1, 1, 2, 2, 0, 0, 0, 0, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3
i_w_adder	db	0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
i_mod_adder	db	0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1

flags_mult	db	0, 2, 4, 6, 7, 8, 9, 10, 11

jxx_dec_a	db	48, 40, 43, 40, 44, 41, 49, 49
jxx_dec_b	db	49, 49, 49, 43, 49, 49, 49, 43
jxx_dec_c	db	49, 49, 49, 49, 49, 49, 44, 44
jxx_dec_d	db	49, 49, 49, 49, 49, 49, 48, 48

daa_rslt_ca00	db	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 16, 17, 18, 19, 20, 21, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 32, 33, 34, 35, 36, 37, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 48, 49, 50, 51, 52, 53, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 64, 65, 66, 67, 68, 69, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 80, 81, 82, 83, 84, 85, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 96, 97, 98, 99, 100, 101, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 112, 113, 114, 115, 116, 117, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 128, 129, 130, 131, 132, 133, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 144, 145, 146, 147, 148, 149, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 16, 17, 18, 19, 20, 21, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 32, 33, 34, 35, 36, 37, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 48, 49, 50, 51, 52, 53, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 64, 65, 66, 67, 68, 69, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 80, 81, 82, 83, 84, 85, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 96, 97, 98, 99, 100, 101
daa_cf_ca00_ca01_das_cf_ca00	db	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
daa_af_ca00_ca10_das_af_ca00_ca10	db	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1

daa_rslt_ca01	db	6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101
daa_af_ca01_cf_ca10_cf_ca11_af_ca11_das_af_ca01_cf_ca10_cf_ca11_af_ca11	db	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1

daa_rslt_ca10	db	96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 112, 113, 114, 115, 116, 117, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 128, 129, 130, 131, 132, 133, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 144, 145, 146, 147, 148, 149, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 160, 161, 162, 163, 164, 165, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 176, 177, 178, 179, 180, 181, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 192, 193, 194, 195, 196, 197, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 208, 209, 210, 211, 212, 213, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 224, 225, 226, 227, 228, 229, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 240, 241, 242, 243, 244, 245, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 16, 17, 18, 19, 20, 21, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 32, 33, 34, 35, 36, 37, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 48, 49, 50, 51, 52, 53, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 64, 65, 66, 67, 68, 69, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 80, 81, 82, 83, 84, 85, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 96, 97, 98, 99, 100, 101

daa_rslt_ca11	db	102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101

das_rslt_ca00	db	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 4, 5, 6, 7, 8, 9, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 20, 21, 22, 23, 24, 25, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 36, 37, 38, 39, 40, 41, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 52, 53, 54, 55, 56, 57, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 68, 69, 70, 71, 72, 73, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 84, 85, 86, 87, 88, 89, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 100, 101, 102, 103, 104, 105, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 116, 117, 118, 119, 120, 121, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 132, 133, 134, 135, 136, 137, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 52, 53, 54, 55, 56, 57, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 68, 69, 70, 71, 72, 73, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 84, 85, 86, 87, 88, 89, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 100, 101, 102, 103, 104, 105, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 116, 117, 118, 119, 120, 121, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 132, 133, 134, 135, 136, 137, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 148, 149, 150, 151, 152, 153

das_rslt_ca01	db	154, 155, 156, 157, 158, 159, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153
das_cf_ca01	db	1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1

das_rslt_ca10	db	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 164, 165, 166, 167, 168, 169, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 180, 181, 182, 183, 184, 185, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 196, 197, 198, 199, 200, 201, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 212, 213, 214, 215, 216, 217, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 228, 229, 230, 231, 232, 233, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 244, 245, 246, 247, 248, 249, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 4, 5, 6, 7, 8, 9, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 20, 21, 22, 23, 24, 25, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 36, 37, 38, 39, 40, 41, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 52, 53, 54, 55, 56, 57, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 68, 69, 70, 71, 72, 73, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 84, 85, 86, 87, 88, 89, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 100, 101, 102, 103, 104, 105, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 116, 117, 118, 119, 120, 121, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 132, 133, 134, 135, 136, 137, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 148, 149, 150, 151, 152, 153

das_rslt_ca11	db	154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153

parity		db	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1

; This is the format of the 36-byte tm structure, returned by the emulator's RTC query call

timetable:

tm_sec		equ $
tm_min		equ $+4
tm_hour		equ $+8
tm_mday		equ $+12
tm_mon		equ $+16
tm_year		equ $+20
tm_wday		equ $+24
tm_yday		equ $+28
tm_dst		equ $+32
