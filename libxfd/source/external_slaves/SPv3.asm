; SPv3 (SPacker v3 for Atari ST) XFD slave by Kyzer/CSG
; This slave code is in the Public Domain.
; not based on Mr.Larmer's slave ;)
;
; SPv3 crunched data format:
; ULONG ID: "SPv3"
; UWORD 0 (always?)
; UWORD ???? (something to do with decrunch colour?)
; ULONG packed length, includes header
; ULONG unpacked length
;
; Altair (author of Atomik Packer) says "Thanks to Firehawks who have done
; a new release of Atomik they have called SpeedPacker 3"
;(Meow, but it's not identical code)

	IFD	TEST
	include	xfdExeHead.a
	ENDC

	include	libraries/xfdmaster.i

ForeMan	dc.l	$70FF4E75,XFDF_ID	; id
	dc.w	XFDF_VERSION		; foreman version
	dcb.w	5,0			; reserved stuff
	dc.l	SP3data			; first slave

SP3data	dc.l	0
	dc.w	2,39
	dc.l	SP3data_name
	dc.w	XFDPFF_DATA!XFDPFF_RECOGLEN!XFDPFF_USERTARGET,0
	dc.l	SP3data_recog,SP3data_decrunch,SP3data_scan,SP3data_verify
	dc.l	0,18

SP3data_recog
	cmp.l	#"SPv3",(a0)+
	bne.s	.fail
	tst.w	(a0)+
	bne.s	.fail
	addq.l	#2,a0
	move.l	(a0)+,d1		; d1 = packed length
	beq.s	.fail			; fail if unpacked length = 0
	move.l	(a0)+,d0		; d0 = unpacked length
	beq.s	.fail			; fail if unpacked length = 0
	cmp.l	d0,d1
	bhi.s	.fail			; fail if packed length > unpacked
	move.l	d0,xfdrr_FinalTargetLen(a1)
	addq.l	#8,d0			; up to 8 bytes overrun
	move.l	d0,xfdrr_MinTargetLen(a1)
	move.l	d1,xfdrr_MinSourceLen(a1)
	moveq	#1,d0			; return success
	rts
.fail	moveq	#0,d0
	rts

SP3data_decrunch
	movem.l	d2-d7/a2-a6,-(sp)
	move.l	a0,a6
	move.l	xfdbi_SourceBuffer(a6),a0
	move.l	xfdbi_UserTargetBuf(a6),a2
	move.l	a2,a1
	move.l	8(a0),d0
	addq.l	#1,d0
	andi.w	#-2,d0
.copy	move.w	(a0)+,(a1)+
	subq.l	#2,d0
	bne.s	.copy
	move.l	a2,a0	
	bsr.s	unpack
	moveq	#1,d0	; success
	movem.l	(sp)+,d2-d7/a2-a6
	rts

SP3data_scan
	moveq   #0,d0
	cmp.l   #"SPv3",(a0)
	bne.s	.fail
	moveq	#1,d0
.fail	rts

SP3data_verify
	move.l   8(a0),d1		; d1 = packed length
	cmp.l   d0,d1
	bhi.s	.fail			; fail if packed len > remaining len
	cmp.l	12(a0),d1
	bhi.s	.fail			; fail if packed len > unpacked len
	move.l	d1,d0
	rts
.fail	moveq	#0,d0
	rts





* UNPACK source for SPACKERv3	(C)THE FIREHAWKS'92
* -------------------------------------------------
* in	a0: even address start packed block
* out	d0: original length or 0 if not SPv3 packed
* =================================================
* Use AUTO_SP3.PRG for multiblk packed files

sp3_53=0	;DS.L 1			= 4
sp3_54=4	;DS.B 8			= 8
sp3_55=12	;DS.W 2*64		= 256
sp3_56=268	;DS.W 2 / DS.B 1	= 5
sp3_57=273	;DS.B 1	/ DS.B 2*64	= 129
sp3_58=402	;DS.B 512		= 512
sp3_SIZE=914

unpack:	moveq	#0,d0
	;movem.l d0-a6,-(sp)
	;lea	sp3_53(pc),a6
	lea	-sp3_SIZE(sp),sp
	move.l	sp,a6

	movea.l	a0,a1
	cmpi.l	#'SPv3',(a1)+
	bne.s	sp3_02
	tst.w	(a1)
	bne.s	sp3_02
	move.l	(a1)+,d5	; magic number only
	move.l	(a1)+,d0	; d0 = packed length
	move.l	(a1)+,-(sp)	; (sp) = unpacklen [and there's a bug fixed]
	movea.l	a0,a2		; a2 = start of packed buffer
	adda.l	d0,a0		; a0 = end of packed buffer

	move.l	-(a0),-(a1)	; overwrite header with last 16
	move.l	-(a0),-(a1)	; bytes of packed buffer.
	move.l	-(a0),-(a1)
	move.l	-(a0),-(a1)

	adda.l	(sp),a1		; a1 = end of unpacked buffer

	lea	sp3_58-sp3_53(a6),a3
	moveq	#128-1,d0	; copy first 512 bytes of packed data
sp3_01:	move.l	(a2)+,(a3)+	; into sp_58 buffer
	dbf	d0,sp3_01

	suba.l	a2,a3
	move.l	a3,-(sp)
	bsr.s	sp3_03
	bsr	sp3_21
	move.b	-(a0),d0
	adda.l	(sp)+,a0
	move.b	d0,(a0)+
	lea	sp3_58-sp3_53(a6),a2
	bsr	sp3_22
	bsr	sp3_15
sp3_02:
	lea	sp3_SIZE+4(sp),sp
	;movem.l (sp)+,d0-a6
	rts

sp3_03:	;move.w	SR,d1
	;andi.w	#$2000,d1
	;beq.s	sp3_04
	;move.w	$FFFF8240.W,2(a6)
	;btst	#1,$FFFF8260.W
	;bne.s	sp3_04
	;swap	d5
sp3_04:	;clr.w	d5
	move.w	-(a0),d6
	lea	sp3_54-sp3_53(a6),a3
	move.b	d6,(a3)+
	moveq	#1,d3
	moveq	#6,d4
sp3_05:	cmp.b	d6,d3
	bne.s	sp3_06
	addq.w	#2,d3
sp3_06:	move.b	d3,(a3)+
	addq.w	#2,d3
	dbf	d4,sp3_05
	moveq	#$10,d4
	move.b	-(a0),(a3)+
	move.b	d4,(a3)+
	move.b	-(a0),(a3)+
	move.b	d4,(a3)+
	move.b	-(a0),d4
	move.w	d4,(a6)
	lea	sp3_57-sp3_53(a6),a5
	move.b	-(a0),d4
	lea	1(a5,d4.w),a3
sp3_07:	move.b	-(a0),-(a3)
	dbf	d4,sp3_07
	move.b	-(a0),-(a3)
	beq.s	sp3_08
	suba.w	d4,a0
sp3_08:	moveq	#0,d2
	move.b	-(a0),d2
	move.w	d2,d3
	move.b	-(a0),d7
sp3_09:	bsr.s	sp3_10
	bsr.s	sp3_10
	dbf	d2,sp3_09
	rts

sp3_10:	not.w	d4
	add.b	d7,d7
	bne.s	sp3_11
	move.b	-(a0),d7
	addx.b	d7,d7
sp3_11:	bcs.s	sp3_12
	move.w	d2,d0
	subq.w	#1,d3
	sub.w	d3,d0
	add.w	d0,d0
	add.w	d4,d0
	add.w	d0,d0
	neg.w	d0
	move.w	d0,-(a3)
	rts
sp3_12:	moveq	#2,d1
	bsr	sp3_44
	add.w	d0,d0
	beq.s	sp3_13
	move.b	d0,-(a3)
	moveq	#2,d1
	bsr	sp3_44
	add.w	d0,d0
	move.b	d0,-(a3)
	rts
sp3_13:	moveq	#2,d1
	bsr	sp3_44
	move.w	sp3_55-sp3_53(a6),d1
	add.w	d0,d0
	beq.s	sp3_14
	move.w	sp3_55+2-sp3_53(a6),d1
sp3_14:	or.w	d1,d0
	move.w	d0,-(a3)
	rts

sp3_15:	clr.w	d1
	;move.w	SR,d1
	;andi.w	#$2000,d1
	;beq.s	sp3_16
	;move.w	2(a6),$FFFF8240.W
sp3_16:	tst.w	d6
	bpl.s	sp3_20
	movea.l	a1,a2
	movea.l	a1,a3
	adda.l	4(sp),a3

sp3_17:	moveq	#3,d6
sp3_18:	move.w	(a2)+,d0
	moveq	#3,d5
sp3_19:	add.w	d0,d0
	addx.w	d1,d1
	add.w	d0,d0
	addx.w	d2,d2
	add.w	d0,d0
	addx.w	d3,d3
	add.w	d0,d0
	addx.w	d4,d4
	dbf	d5,sp3_19
	dbf	d6,sp3_18
	cmpa.l	a2,a3
	blt.s	sp3_20
	movem.w	d1-d4,-8(a2)
	cmpa.l	a2,a3
	;bne.s	sp3_17	; you don't wanna do that!
	bcc.s	sp3_17
sp3_20:	rts

sp3_21:	move.b	-(a0),-(a1)
sp3_22:	;swap	d5
	;beq.s	sp3_23
	;move.w	d5,$FFFF8240.W
sp3_23:	lea	sp3_56+2-sp3_53(a6),a3
	cmpa.l	a0,a2
	blt.s	sp3_25
	rts

sp3_24:	adda.w	d3,a3
sp3_25:	add.b	d7,d7
	bcc.s	sp3_28
	beq.s	sp3_27
sp3_26:	move.w	(a3),d3
	bmi.s	sp3_24
	bra.s	sp3_29
sp3_27:	move.b	-(a0),d7
	addx.b	d7,d7
	bcs.s	sp3_26
sp3_28:	move.w	-(a3),d3
	bmi.s	sp3_24
sp3_29:	ext.w	d3
	jmp	sp3_30(pc,d3.w)
sp3_30:	bra.s	sp3_30
	bra.s	sp3_41
	bra.s	sp3_41
	bra.s	sp3_41
	bra.s	sp3_41
	bra.s	sp3_41
	bra.s	sp3_37
	bra.s	sp3_36
	bra.s	sp3_32
	bra.s	sp3_33
	bra.s	sp3_31
	bra.s	sp3_34
	bra.s	sp3_21
sp3_31:	move.b	(a5),-(a1)
	bra.s	sp3_22
sp3_32:	bsr.s	sp3_43
	move.b	1(a5,d0.w),-(a1)
	bra.s	sp3_22
sp3_33:	bsr.s	sp3_43
	add.w	(a6),d0
	move.b	1(a5,d0.w),-(a1)
	bra.s	sp3_22
sp3_34:	moveq	#3,d1
	bsr.s	sp3_44
	lsr.w	#1,d0
	bcc.s	sp3_35
	not.w	d0
sp3_35:	move.b	(a1),d1
	add.w	d0,d1
	move.b	d1,-(a1)
	bra.s	sp3_22
sp3_36:	lea	sp3_52-2(pc),a4		;-sp3_53(a6),a4
	bsr.s	sp3_48
	addi.w	#16,d0
	lea	1(a1,d0.w),a3
	move.b	-(a3),-(a1)
	move.b	-(a3),-(a1)
	bra	sp3_22
sp3_37:	moveq	#3,d1
	bsr.s	sp3_44
	tst.w	d0
	beq.s	sp3_38
	addq.w	#5,d0
	bra.s	sp3_40
sp3_38:	move.b	-(a0),d0
	beq.s	sp3_39
	addi.w	#20,d0
	bra.s	sp3_40
sp3_39:	moveq	#13,d1
	bsr.s	sp3_44
	addi.w	#276,d0
sp3_40:	move.w	d0,d3
	add.w	d3,d3
sp3_41:	lea	sp3_52(pc),a4		;-sp3_53(a6),a4
	bsr.s	sp3_48
	lsr.w	#1,d3
	lea	1(a1,d0.w),a3
	move.b	-(a3),-(a1)
sp3_42:	move.b	-(a3),-(a1)
	dbf	d3,sp3_42
	bra	sp3_22
sp3_43:	moveq	#0,d1
	move.b	(a3),d1
sp3_44:	moveq	#0,d0
	cmpi.w	#7,d1
	bpl.s	sp3_47
sp3_45:	add.b	d7,d7
	beq.s	sp3_46
	addx.w	d0,d0
	dbf	d1,sp3_45
	rts
sp3_46:	move.b	-(a0),d7
	addx.b	d7,d7
	addx.w	d0,d0
	dbf	d1,sp3_45
	rts
sp3_47:	move.b	-(a0),d0
	subq.w	#8,d1
	bpl.s	sp3_45
	rts
sp3_48:	moveq	#0,d1
	move.b	(a3),d1
	adda.w	d1,a4
	move.w	(a4),d1
	bsr.s	sp3_44
	tst.b	d6
	beq.s	sp3_51
	move.w	d0,d4
	andi.w	#$FFF0,d4
	andi.w	#$000F,d0
	beq.s	sp3_50
	lsr.w	#1,d0
	beq.s	sp3_49
	roxr.b	#1,d7
	bcc.s	sp3_50
	move.b	d7,(a0)+
	moveq	#-128,d7
	bra.s	sp3_50
sp3_49:	moveq	#2,d1
	bsr.s	sp3_44
	add.w	d0,d0
	or.w	d4,d0
	bra.s	sp3_51
sp3_50:	lea	sp3_54-sp3_53(a6),a3
	or.b	(a3,d0.w),d4
	move.w	d4,d0
sp3_51:	add.w	18(a4),d0
	rts

	DC.W	3
sp3_52:	DC.W	4,5,7,8,9,10,11,12
	DC.W	-16
	DC.W	0,32,96,352,864,1888,3936,8032

;sp3_53:	DS.L	1
;sp3_54:	DS.B	8
;sp3_55:	DS.W	2*64
;sp3_56:	DS.W	2
;	DS.B	1
;sp3_57:	DS.B	1
;	DS.B	2*64
;sp3_58:	DS.B	512

SP3data_name	dc.b	'(SPv3) Data Cruncher',0
		dc.b	'$VER: SPv3 1.1 (25.07.2000) by <kyzer@4u.net>'
