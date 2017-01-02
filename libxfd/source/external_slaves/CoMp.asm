; CoMp XFD slave by Kyzer/CSG
; This slave code is in the Public Domain.
; (Resident De/Compressor (C) 1994 Simon_Gibbs@Standard.Embassy.Com)
; Huffman-only compression!
;
; CoMp crunched data format:
; ULONG ID: "CoMp"
; ULONG (("H" or "h") << 24) | (unpacked length)
;

	IFD	TEST
	include	xfdExeHead.a
	ENDC

	include	libraries/xfdmaster.i

ForeMan	dc.l	$70FF4E75,XFDF_ID	; id
	dc.w	XFDF_VERSION		; foreman version
	dcb.w	5,0			; reserved stuff
	dc.l	CoMpdata		; first slave

CoMpdata
	dc.l	0
	dc.w	2,39
	dc.l	CoMpdata_name
	dc.w	XFDPFF_DATA!XFDPFF_RECOGLEN!XFDPFF_USERTARGET,0
	dc.l	CoMpdata_recog,CoMpdata_decrunch,CoMpdata_scan,CoMpdata_verify
	dc.l	0,18

CoMpdata_recog
	cmp.l	#"CoMp",(a0)+
	bne.s	.fail
	move.l	(a0)+,d0		; d0 = unpacked length and "H"<<24
	rol.l	#8,d0
	cmp.b	#"h",d0
	beq.s	.ok1
	cmp.b	#"H",d0
	bne.s	.fail
.ok1	asr.l	#8,d0
	beq.s	.fail			; fail if unpacked length = 0
	move.l	d0,xfdrr_FinalTargetLen(a1)
	add.l	#(6*256*2)+4,d0
	move.l	d0,xfdrr_MinTargetLen(a1)
	clr.l	xfdrr_MinSourceLen(a1)
	moveq	#1,d0			; return success
	rts
.fail	moveq	#0,d0
	rts

CoMpdata_decrunch
	movem.l	d2-d7/a2-a6,-(sp)
	move.l	a0,a6
	move.l	xfdbi_SourceBuffer(a6),a0	; a0 = input ptr
	move.l	xfdbi_UserTargetBuf(a6),a1	; a1 = output ptr
	move.l	xfdbi_TargetBufSaveLen(a6),d7	; d7 = output length


	; go to end of decrunched file and align to 4 bytes
	; this becomes our decrunch table pointer
	move.l	a1,d0
	add.l	d7,d0
	addq.l	#3,d0
	andi.w	#-4,d0
	move.l	d0,a2				; A2 = decrunch table

	move.b	4(a0),d0	; D0 = table on-disk format ("H" or "h")
	addq.l	#8,a0
	moveq	#0,d1
	move.b	(a0)+,d1	; D1 = number of leaf-nodes (byte entries)
	moveq	#0,d2
	move.b	(a0)+,d2	; D2 = number of branch-nodes (W/L entries)
	moveq	#1,d6
	add.l	d1,d6
	add.l	d2,d6		; D6 = total table entries + 1 (root entry)


	; build decrunch table (each entry 6 bytes)
	move.l	a2,a3
.tab1	move.b	(a0)+,(a3)+	; $AA
	clr.b	(a3)+		; $00
	clr.l	(a3)+		; $00000000
	dbra	d1,.tab1	; = $AA0000000000

	cmp.b	#"h",d0
	beq.s	.smltab
.bigtab	clr.w	(a3)+		; $00, $00
	move.b	(a0)+,(a3)+	; $AA
	move.b	(a0)+,(a3)+	; $BB
	move.b	(a0)+,(a3)+	; $CC
	move.b	(a0)+,(a3)+	; $DD
	dbra	d2,.bigtab	; = $0000AABBCCDD
	bra.s	.tabok
.smltab	clr.w	(a3)+		; $00, $00
	clr.b	(a3)+		; $00
	move.b	(a0)+,(a3)+	; $AA
	clr.b	(a3)+		; $00
	move.b	(a0)+,(a3)+	; $BB
	dbra	d2,.smltab	; = $000000AA00BB
.tabok

	; A0 = src ptr
	; A1 = dest ptr
	; A2 = table ptr
	; D6 = tree-root index
	; D7 = output bytes to go

	; D0 = current node
	; D1 = bit/byte counter
	; D2 = current byte/bit

	move.w	d6,d0		; current node = root node
	moveq	#8-1,d1		; reset number of bits left in D2
	move.b	(a0)+,d2	; fill up D2

.loop	add.l	d0,d0		; d0 *= 6
	move.l	d0,a5
	add.l	d0,d0
	add.l	a5,d0
	move.l	2(a2,d0.w),d3	; d3 = left/right branch indicies of this node
	bne.s	.branch

	; if both have nil indicies, it's a leaf
	move.b	(a2,d0.w),(a1)+	; write out the literal
	move.w	d6,d0		; back to the root node

	subq.l	#1,d7		; one less byte
	bne.s	.loop		; if any more left, go back again
	bra.s	.done

.branch	asl.b	#1,d2		; clock out the next bit
	bcs.b	2$		; set = LSW branch, clear = MSW branch
	swap	d3
2$	move.w	d3,d0		; set new node index

	dbra	d1,.loop	; if still bits left, back to loop
	moveq	#8-1,d1		; reset number of bits left in D2
	move.b	(a0)+,d2	; fill up D2
	bra.s	.loop

.done	moveq	#1,d0	; success
	movem.l	(sp)+,d2-d7/a2-a6
	rts

CoMpdata_scan
	moveq   #0,d0
	cmp.l   #"CoMp",(a0)
	bne.s	.fail
	moveq	#1,d0
.fail	rts

CoMpdata_verify
	move.l	4(a0),d0
	rol.l	#8,d0
	cmp.b	#"h",d0
	beq.s	.ok1
	cmp.b	#"H",d0
	bne.s	.fail
.ok1	asr.l	#8,d0
	tst.l	d0
	beq.s	.fail
	rts
.fail	moveq	#0,d0
	rts


CoMpdata_name	dc.b	'(CoMp) Data Cruncher',0
		dc.b	'$VER: CoMp 1.1 (14.10.2000) by <kyzer@4u.net>'
