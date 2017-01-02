; XFD external slave for OBJT format by Kyzer/CSG <kyzer@4u.net>
; This slave code is in the Public Domain.
; OBJT format (C) GNU Design
;
; OBJT is equivalent to an Amiga executable with one code hunk and
; some relocs. This slave converts the format to a regular Amiga executable.
;

; OBJT FORMAT:
; 4 bytes:   "OBJT"
; 4 bytes:   length of code hunk (in longwords)
; 4*x bytes: code hunk
; then until end of file:
; - 2 bytes:   selector (0, 8, 16 or 32)
; - 4 bytes:   number of relocs
; - 4*x bytes: reloc offsets
; - if selector is 0,  all relocation is done.
; - if selector is 8,  a byte relocation is done 
; - if selector is 16, a word (16-bit) relocation is done 
; - if selector is 32, a longword (32-bit) relocation is done 
; - a relocation means to add the address of where the code hunk was loaded
;   to the byte/word/longword already present at the given offset in the
;   code hunk.

	include	dos/doshunks.i
	include	exec/memory.i
	include	libraries/xfdmaster.i
	include	lvo/exec_lib.i

	IFD	TEST
	include	xfdExeHead.a
	ENDC

ForeMan	dc.l	$70FF4E75,XFDF_ID	; id
	dc.w	XFDF_VERSION		; foreman version
	dcb.w	5,0			; reserved stuff
	dc.l	OBJT			; first slave

OBJT	dc.l	0
	dc.w	2,38
	dc.l	OBJT_name
	dc.w	XFDPFF_DATA,0
	dc.l	OBJT_recog,OBJT_decrunch,0,0
	dc.l	0,14

OBJT_recog
	cmp.l	#"OBJT",(a0)
	bne.s	.fail
	moveq	#1,d0
	rts
.fail	moveq	#0,d0
	rts

OBJT_decrunch
	movem.l	d2-d3/a2-a6,-(sp)
	move.l	a0,a4			; a4 = BufferInfo
	move.w	#XFDERR_CORRUPTEDDATA,xfdbi_Error(a4)
	move.l	xfdbi_SourceBuffer(a4),a3
	move.l	4(a3),d2		; d2 = codesize
	move.l	d2,d3			; d3 = decrunched size
	add.l	d3,d3
	add.l	d3,d3
	lea	8(a3),a0		; a0 = relocs section
	add.l	d3,a0

	; todo: insert check in loop so we don't go past the end of the
	; source buffer if there's no selector=0
	moveq	#16,d1			; reloc hunk = 16 bytes + relocs
.relocs	move.w	(a0)+,d0		; get selector
	beq.s	.rdone			; finished if selector=0
	subq.w	#8,d0			; selector=8 ?
	beq.s	.relok
	subq.w	#8,d0			; selector=16 ?
	beq.s	.relok
	cmp.w	#16,d0			; selector=32 ?
	bne.s	.fail			; if none of the above, fail.

.relok	move.l	(a0)+,d0		; d0 = size of relocs (in bytes)
	add.l	d0,d0
	add.l	d0,d0
	add.l	d1,d3			; add size of relocs section
	add.l	d0,d3
	add.l	d0,a0			; skip to next relocs section
	bra.s	.relocs

.fail	moveq	#0,d0
	bra.s	.exit

	; allocate memory
.rdone	moveq	#36,d0			; hunk headers (32) + HUNK_END (4)
	add.l	d3,d0
	move.w	#XFDERR_NOMEMORY,xfdbi_Error(a4)
	move.l	d0,xfdbi_TargetBufLen(a4)
	move.l	d0,xfdbi_TargetBufSaveLen(a4)
	move.l	xfdbi_TargetBufMemType(a4),d1
	move.l	4.w,a6
	jsr	_LVOAllocMem(a6)
	move.l	d0,xfdbi_TargetBuffer(a4)
	beq.s	.fail
	move.l	d0,a1			; a1 = output

	move.l	#HUNK_HEADER,(a1)+	; hunk header
	clr.l	(a1)+			; name
	moveq	#1,d0			; 1 hunk
	move.l	d0,(a1)+
	clr.l	(a1)+			; 0 = first hunk
	clr.l	(a1)+			; 0 = last hunk
	move.l	d2,(a1)+		; hunk sizes
	move.l	#HUNK_CODE,(a1)+	; HUNK CODE
	move.l	d2,(a1)+		; hunk size

	; copy code hunk
	lea	8(a3),a0
1$	move.l	(a0)+,(a1)+
	subq.l	#1,d2
	bne.s	1$


.reloc2	move.w	(a0)+,d0		; get selector
	beq.s	.rdone2
	move.l	#HUNK_RELOC8,d1		; d1 = HUNK_RELOC8
	subq.w	#8,d0			; selector = 8 ?
	beq.s	.relok2
	subq	#1,d1			; d1 = HUNK_RELOC16
	subq.w	#8,d0			; selector = 16 ?
	beq.s	.relok2
	subq	#1,d1			; d1 = HUNK_RELOC32
.relok2	move.l	d1,(a1)+		; hunk type
	move.l	(a0)+,d0
	move.l	d0,(a1)+		; number of relocs
	clr.l	(a1)+			; 0 = "relative to hunk 0"
2$	move.l	(a0)+,(a1)+		; copy relocs hunk
	subq.l	#1,d0
	bne.s	2$
	clr.l	(a1)+			; 0 = no more relocs
	bra.s	.reloc2
.rdone2	move.l	#HUNK_END,(a1)+
	clr.w	xfdbi_Error(a4)
	moveq	#1,d0
.exit	movem.l	(sp)+,d2-d3/a2-a6
	rts

OBJT_name
	dc.b	"OBJT",0
	dc.b	"$VER: OBJT 1.0 (21.03.2003) by <kyzer@4u.net>"
