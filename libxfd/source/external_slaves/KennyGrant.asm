; Grants Computing decrunchers
; by Kyzer/CSG & Kenny Grant
; This slave code is in the Public Domain.
;
; 1.0:	works OK, could probably do with range checks on relocs and validity
;	checks on compressed data. syskiller GC!2 formats still todo
; 1.1:	now includes data slaves, but not GC!2 exe formats
;
; Kenny Grant compiled Amiga Power coverdisks for issues 20-43 and
; 50-55, he also did some stuff for CU Amiga. Read the story of the AP
; coverdisk at http://dspace.dial.pipex.com/ap2/comments/JN/disks.html
;
; Thanks to Mr.Larmer for writing the previous datafile-only slave, called
; 'GraftGold', however the decrunch code I first extracted from the example
; files exactly matched the graftgold slave, so technically I didn't need
; to refer to his slave...

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
;	dc.l	GC2exe			; first slave
	dc.l	GC2data			; first slave

GC2exe	dc.l	GC2data
	dc.w	2,38
	dc.l	GC2exe_name
	dc.w	XFDPFF_ADDR,0
	dc.l	GC2exe_recog,GC2exe_decrunch,0,0
	dc.l	0,1024	;******** Calculate this right!!

GC2data	dc.l	GC3exe
	dc.w	2,39			; slave v2+ requires master v39+
	dc.l	GC2data_name
	dc.w	XFDPFF_DATA!XFDPFF_RECOGLEN!XFDPFF_USERTARGET,0
	dc.l	GC2data_recog,GC2data_decrunch,GC2data_scan,GC2data_verify
	dc.l	0,18

GC3exe	dc.l	GC3data
	dc.w	2,38
	dc.l	GC3exe_name
	dc.w	XFDPFF_RELOC,0
	dc.l	GC3exe_recog,GC3exe_decrunch,0,0
	dc.l	0,44+944

GC3data	dc.l	0
	dc.w	2,39
	dc.l	GC3data_name
	dc.w	XFDPFF_DATA!XFDPFF_RECOGLEN!XFDPFF_USERTARGET,0
	dc.l	GC3data_recog,GC3data_decrunch,GC3data_scan,GC3data_verify
	dc.l	0,18


;****************************************************************************
;****************************************************************************
;****************************************************************************
;
; GC!2 and GC!3 crunched data format:
; longword ID: GC!\02 or GC!\03
; longword unpacked length
; longword packed length
; remainder is crunched stuff (6 bytes immediately & unconditionally read)
; total for minimum: 18 bytes


GC2_ID	= "GC!\0"+2
GC3_ID	= "GC!\0"+3

GC2data_recog
	move.l	#GC2_ID,d0
	bra.s	__recog
GC3data_recog
	move.l	#GC3_ID,d0
__recog	cmp.l	(a0)+,d0
	bne.s	.fail
	move.l	(a0)+,d0		; d0 = unpacked length
	beq.s	.fail			; fail if unpacked length = 0
	move.l	(a0)+,d1		; d1 = packed length
	beq.s	.fail			; fail if packed length = 0
	cmp.l	d0,d1
	bhi.s	.fail			; fail if packed length > unpacked
	move.l	d0,xfdrr_FinalTargetLen(a1)
	add.l	#52,d0			; safety for GC!2 (see GC2_DECRUNCH)
	move.l	d0,xfdrr_MinTargetLen(a1)
	moveq	#12,d0
	add.l	d0,d1
	move.l	d1,xfdrr_MinSourceLen(a1)
	moveq	#1,d0			; return success
	rts
.fail	moveq	#0,d0
	rts

GC2data_decrunch
	move.l	a2,-(sp)
	lea	GC2_DECRUNCH(pc),a2
	bra.s	__decr
GC3data_decrunch
	move.l	a2,-(sp)
	lea	GC3_DECRUNCH(pc),a2
__decr	move.l	xfdbi_UserTargetBuf(a0),a1
	move.l	xfdbi_SourceBuffer(a0),a0
	addq.l	#4,a0
	jsr	(a2)
	move.l	(sp)+,a2
	rts

GC2data_scan
	moveq   #0,d0
	cmp.l   #GC2_ID,(a0)
	bne.s	.fail
	moveq	#1,d0
.fail	rts
GC3data_scan
	moveq   #0,d0
	cmp.l   #GC3_ID,(a0)
	bne.s	.fail
	moveq	#1,d0
.fail	rts


GC2data_verify
GC3data_verify
	moveq   #12,d1
	add.l   8(a0),d1		; d1 = packed length
	cmp.l   d0,d1
	bhi.s	.fail			; fail if packed len > remaining len
	cmp.l	4(a0),d1
	bhi.s	.fail			; fail if packed len > unpacked len
	move.l	d1,d0
	rts
.fail	moveq	#0,d0
	rts




;****************************************************************************
;****************************************************************************
;****************************************************************************
;
;Grants Computing (Kenny Grant) GC!2 syskiller format


GC2exe_recog
	moveq	#0,d0
	rts

GC2exe_decrunch
	moveq	#0,d0
	rts


;Grants Computing (Kenny Grant) GC!3 overlay format
;
;Executable overlay header (a few changes to the code here and there),
;performs a SEEK call with an offset - matches with the first byte after
;the hunk_overlay header the real data starts there.
;
;(1) a longword listing the total number of hunks.
;
;(2) one longword for each hunk representing arguments to AllocMem():
;    - (upper 8 bits >>24)|MEMF_CLEAR -> lower 8 bits of D1
;    - lower 24 bits && $00FFFFFF -> D0
;
;(3) here begins a loop for each hunk in the decrunched file
;    - one longword that states what the hunk is:
;      944 & 932 BYTES VERSION
;      - longword < 0: throw away next longword, goto (3) ; 944 only?
;      - longword = 0: copy verbatim to fill the hunk, goto (4)
;      924 & 936 BYTES VERSION
;      - longword = 0: goto (3)
;      ALL VERSIONS
;      - longword  > 0: (always "GC!"<<8+3) if size!=0 decruncher; goto (4)
;
;(4) if longword >=0 (ie isn't BSS), there may be relocs
;    - standard reloc format (num, hunk, relocs[num], num...)
;    - if first num=0, there are no relocs
;
;Examples:
;944 byte ver: AP38a:AtomSmasher, AP39a:TrickorTreat, AP38a:VectorBattleGround
;936 byte ver: AP34a:SmidgeDemo, AP38a:SuperObliteration
;932 byte ver: AP37a:Monaco
;924 byte ver: AP35:Statix

GC3exe_recog
	cmp.l	#HUNK_HEADER,(a0)	; executable?
	bne.s	.fail
	cmp.w	#2,10(a0)		; 2 hunks?
	bne.s	.fail
	lea	32(a0),a0
	move.l	(a0)+,d0		; a0 = start of code hunk
	asl.l	#2,d0			; d0 = size of code hunk
	cmp.l	#944,d0
	bhi.s	.fail			; code size <= 944 bytes?
	lea	(a0,d0.w),a1		; a1 = end of code hunk
	cmp.l	#HUNK_OVERLAY,4(a1)	; hunk 2 = overlay?
	bne.s	.fail

	cmp.w	#$6000,(a0)		; check bra.w instruction
	bne.s	.fail
	addq.l	#2,a0
	adda.w	(a0),a0			; skip past overlay header

	; check the code itself
	cmp.l	#$4eadffc4,$18(a0)	; check jsr _LVOOutput(a5)
	bne.s	.fail
	cmp.b	#$67,$1E(a0)		; check for 944 byte version
	bne.s	.not944			; which has an extra instruction here
	addq.l	#2,a0			; compensate for extra instruction
.not944	cmp.w	#$243A,$30(a0)		; check for 936 byte version
	bne.s	.not936			; which has 'move.l xxx(pc),d2'
	move.w	$32(a0),d1		; d1 = xxxx
	lea	$32(a0,d1.w),a1		; a1 = xxxx(pc)
	move.l	(a1),d1			; d1 = offset
	bra.s	.cont
.not936	cmp.w	#$243C,$30(a0)		; check for other versions
	bne.s	.fail			; which have 'move.l #offset,d2'
	move.l	$32(a0),d1		; d1 = offset
.cont	add.l	#52,d0			; check offset == (codesize+52)
	cmp.l	d0,d1
	bne.s	.fail

	moveq	#1,d0
	rts
.fail	moveq	#0,d0
	rts


	STRUCTURE myhunk,0
	ULONG	h_len
	APTR	h_addr
	UBYTE	h_pad
	UBYTE	h_memflags
	UBYTE	h_isbss
	UBYTE	h_ispacked
	LABEL	h_SIZEOF

MAX_HUNKS=20	; this is an arbitrary number
hunks=-h_SIZEOF*MAX_HUNKS

GC3exe_decrunch
	movem.l	d2-d7/a2-a6,-(sp)
	link	a5,#hunks		; a5 = hunks
	move.l	a0,a4			; a4 = BufferInfo

	move.w	#XFDERR_CORRUPTEDDATA,xfdbi_Error(a4)
	move.l	xfdbi_SourceBuffer(a4),a3
	move.l	a3,a0
	lea	36(a3),a2		; a2 = start of code (temp)
	bsr	GC3exe_recog		; d1 = offset of data
	lea	(a3,d1.w),a3		; a3 = start of data


	; A0=data_ptr A4=BufferInfo A5=hunks A6=current_hunk
	; D4=relocs_size D5=944style D6=current_hunk D7=number_hunks
	; A1/A2/A3/D0/D1/D2/D3/D4=scratch


	; check which version of code this file is
	; ('944 style' means code=0 for unpacked)
	; ('not 944 style' means code=0 for bss)
	move.b	$fa(a2),d0

	moveq	#1,d5
	cmp.b	#$6a,d0			; 944 byte version (944style)
	beq.s	.styled
	cmp.b	#$22,d0			; 932 byte version (944style)
	beq.s	.styled

	moveq	#0,d5
	cmp.b	#$53,d0			; 936 byte version (not 944style)
	beq.s	.styled
	cmp.b	#$20,d0			; 924 byte version (not 944style)
	bne	.fail
.styled


	; get the number of hunks
	move.l	a3,a0			; a0 = data_ptr
	move.l	(a0)+,d7		; d7 = number of hunks
	cmp.l	#MAX_HUNKS,d7
	bhi	.fail			; fail if too many hunks for us


	; get the sizes/flags of hunks
	lea	hunks(a5),a6		; FOR all hunks
	move.l	d7,d6

01$	move.l	(a0)+,d0		; read len/memflags
	move.l	d0,d1
	andi.l	#$ffffff,d0		; extract length of hunk
	move.l	d0,h_len(a6)		; and store it
	andi.w	#3,d0
	bne	.fail			; hunksize must be multiple of 4
	rol.l	#8,d1			; extract memflags of hunk
	clr.l	h_pad(a6)		; clear isbss/ispacked/memflags
	move.b	d1,h_memflags(a6)	; store memflags

	lea	h_SIZEOF(a6),a6		; NEXT
	subq	#1,d6
	bne.s	01$


	; get types and relocs of hunks
	moveq	#0,d4			; d4 = size of relocs
	lea	hunks(a5),a6		; FOR all hunks
	move.l	d7,d6

11$	move.l	(a0)+,d0		; read typeid
	bne.s	.nzero			; typeid=0 is ambiguous
	tst.l	d5
	bne.s	.israw			; typeid=0 944style is raw data
.isbss	st.b	h_isbss(a6)		; typeid=0 !944style is bss
	bra.s	12$
.nzero	bpl.s	.ispack			; typeid>0 is always packed data
	tst.l	d5
	beq	.fail			; typeid<0 944style is bss
	addq.l	#4,a0			; 944style bss has extra long (why?)
	bra.s	.isbss

.israw	move.l	a0,h_addr(a6)		; remember where data is
	add.l	h_len(a6),a0		; skip to end of data
	bra.s	.relocs			; raw data can have relocs

.ispack	move.l	a0,h_addr(a6)		; remember where data is
	st.b	h_ispacked(a6)
	tst.l	h_len(a6)
	beq.s	.relocs			; no data if hunk is empty
	addq.l	#4,a0			; skip unpacked size
	add.l	(a0)+,a0		; skip to end of packed data
;	bra.s	.relocs			; packed data can have relocs

.relocs	move.l	a0,d1			; d1 = start of relocs
	move.l	(a0)+,d0		; get number of relocs
	beq.s	12$			; finish if relocs=0
	addq.l	#4,d4			; add hunk_reloc id to reloc size
13$	asl.l	#2,d0
	lea	4(a0,d0.w),a0		; skip to more relocs or end
	move.l	(a0)+,d0		; if there are more relocs, go back
	bne.s	13$
	move.l	a0,d0
	sub.l	d1,d0			; d0 = size of relocs (end-start)
	add.l	d0,d4			; add to reloc size

12$	lea	h_SIZEOF(a6),a6		; NEXT hunk
	subq	#1,d6
	bne.s	11$

	; calculate size of output executable
	; = (4*5)			hunk_header,0,n,0,n-1
	; + 4*4*numhunks		sizeinhdr,hunkid,hunksize,hunk_end
	; + relocs

	moveq	#4*5,d0	; 4*5
	move.l	d7,d1
	asl.l	#4,d1
	add.l	d1,d0	; + 4*4*number_of_hunks
	add.l	d4,d0	; + size of relocs

	; sum non-BSS hunk sizes
	lea	hunks(a5),a6		; FOR all hunks
	move.l	d7,d6
21$	tst.b	h_isbss(a6)		; if hunk isn't bss
	bne.s	.izbss
	add.l	h_len(a6),d0		; add its value to the total
.izbss	lea	h_SIZEOF(a6),a6		; NEXT
	subq	#1,d6
	bne.s	21$

	; d0 = calculated size

	; allocate memory
	move.w	#XFDERR_NOMEMORY,xfdbi_Error(a4)
	move.l	d0,xfdbi_TargetBufLen(a4)
	move.l	d0,xfdbi_TargetBufSaveLen(a4)
	move.l	xfdbi_TargetBufMemType(a4),d1
	move.l	4.w,a6
	jsr	_LVOAllocMem(a6)
	move.l	d0,xfdbi_TargetBuffer(a4)
	beq	.fail
	move.l	d0,a1			; a1 = output


	; A0/A3/A4/A6=scratch A1=output A5=hunks A6=current_hunk
	; D0/D1/D2/D3/D4/D5=scratch D6=current_hunk D7=number_hunks

	; write hunk header
	move.l	#HUNK_HEADER,(a1)+	; write header
	clr.l	(a1)+			; write name (null)
	move.l	d7,d0
	move.l	d0,(a1)+		; write num_hunks
	clr.l	(a1)+			; write first hunk (0)
	subq	#1,d0
	move.l	d0,(a1)+		; write last hunk (num_hunks-1)
	lea	hunks(a5),a6
31$	move.l	h_len(a6),d1		; get length
	asr.l	#2,d1			; bytes->longwords

	btst.b	#MEMB_CHIP,h_memflags(a6)
	beq.s	.nochip
	bset	#30,d1			; set 'chip mem' flag if needed
	bra.s	.nofast
.nochip	btst.b	#MEMB_FAST,h_memflags(a6)
	beq.s	.nofast
	bset	#31,d1			; set 'fast mem' flag if needed
.nofast
	move.l	d1,(a1)+		; write hunk length
	lea	h_SIZEOF(a6),a6
	dbra	d0,31$


	; write hunks
	move.l	d7,d6			; FOR all hunks
	lea	hunks(a5),a6

32$	move.l	h_len(a6),d0		; get length
	asr.l	#2,d0			; bytes->longwords
	tst.b	h_isbss(a6)
	beq.s	.nobss
	move.l	#HUNK_BSS,(a1)+		; BSS hunk is just header
	move.l	d0,(a1)+		; and length
	bra.s	.h_end

.nobss	move.l	#HUNK_CODE,(a1)+	; data hunks are header
	move.l	d0,(a1)+		; length, and contents

	move.l	h_addr(a6),a0		; get address of data
	tst.b	h_ispacked(a6)
	beq.s	.raw
	tst.l	h_len(a6)		; skip onwards if no data
	beq.s	.reloc
	bsr.s	GC3_DECRUNCH		; otherwise decrunch from a0 to a1
	addq.l	#4,a0			; skip past crunched data
	add.l	(a0)+,a0		; so a0 points at relocs
	add.l	h_len(a6),a1		; advance output ptr
	bra.s	.reloc

.raw	move.l	(a0)+,(a1)+		; raw is just copy
	subq.l	#1,d0			; use length as num of longs to copy
	bne.s	.raw			; a0 is left pointing at relocs

	; write relocs if need be
.reloc	move.l	(a0)+,d0		; get number of relocs
	beq.s	.h_end			; finish hunk if none
	move.l	#HUNK_RELOC32,(a1)+	; otherwise write reloc id
33$	move.l	d0,(a1)+		; write number of relocs
	move.l	(a0)+,(a1)+		; write reloc hunk
34$	move.l	(a0)+,(a1)+
	subq	#1,d0			; LOOP for number of relocs
	bne.s	34$
	move.l	(a0)+,d0		; if there are more relocs
	bne.s	33$			; go do them
	move.l	#0,(a1)+		; terminate reloc list

.h_end	move.l	#HUNK_END,(a1)+		; write hunk_end id
	lea	h_SIZEOF(a6),a6		; NEXT hunk
	subq	#1,d6
	bne.s	32$

	; success
	clr.w	xfdbi_Error(a4)
	moveq	#1,d0
	bra.s	.done
.fail	moveq	#0,d0
.done	unlk	a5
	movem.l	(sp)+,d2-d7/a2-a6
	rts


;****************************************************************************
;****************************************************************************
;****************************************************************************
;
; GC!3 decruncher: A0=source (after header), A1=dest

GC3_DECRUNCH
	movem.l	d0-d7/a0-a6,-(sp)
	lea	-$620(sp),sp
	move.l	(a0)+,d1		; d1 = unpacked length
	lea	0(a1,d1.l),a1		; a1 = end of destination buffer
	move.l	(a0)+,d2		; d2 = packed length

;	move.l	a0,d0			; this bit of code arranges that
;	add.l	d2,d0			; the 5kb read buffer which is
;	sub.l	DATA_BUFFER(pc),d0	; indexed by a0 is left pointing
;	divu.w	#5120,d0		; at the right address after this
;	clr.w	d0			; routine finishes - of course,
;	swap	d0			; there's no 5kb buffer in the
;	add.l	DATA_BUFFER(pc),d0	; the slave, so just take it
;	move.l	d0,$640(sp)		; out

	lea	$20(sp),a6
	move.w	(a0)+,d0
	move.l	(a0)+,d6
	moveq	#$10,d7
	sub.w	d0,d7
	lsr.l	d7,d6
	move.w	d0,d7
	moveq	#$10,d3
1$	lea	(sp),a2
	bsr.l	12$
	moveq	#$10,d1
	bsr.l	10$
	move.w	d0,d5
	lea	$9E(a6),a2
	lea	-$1E(a2),a5
2$	movea.l	a6,a4
	bsr.s	6$
	btst	#8,d0
	bne.s	5$
	move.w	d0,d4
	lea	$20(a6),a4
	exg	a5,a2
	bsr.s	6$
	exg	a5,a2
	move.w	d0,d1
	move.w	d0,d2
	bne.s	3$
	moveq	#1,d1
	moveq	#$10,d2
3$	bsr.l	10$
	bset	d2,d0
	lea	1(a1,d0.w),a3
4$	move.b	-(a3),-(a1)
	subq.w	#1,d4
	bpl.s	4$
	move.b	-(a3),-(a1)
	move.b	-(a3),d0
5$	move.b	d0,-(a1)
	subq.w	#1,d5
	bpl.s	2$
	moveq	#1,d1
	bsr.s	10$
	bne.s	1$
	lea	$620(sp),sp
	movem.l	(sp)+,d0-d7/a0-a6
	rts

6$	moveq	#0,d1
7$	subq.w	#1,d7
	beq.s	8$
	lsr.l	#1,d6
	bra.s	9$

8$	moveq	#$10,d7
	move.w	d6,d0
	lsr.l	#1,d6
	swap	d6
	move.w	(a0)+,d6
	swap	d6
	lsr.w	#1,d0
9$	addx.w	d1,d1
	move.w	(a4)+,d0
	cmp.w	d1,d0
	bls.s	7$
	add.w	$3E(a4),d1
	add.w	d1,d1
	move.w	0(a2,d1.w),d0
	rts

10$	move.w	d6,d0
	lsr.l	d1,d6
	sub.w	d1,d7
	bgt.s	11$
	add.w	d3,d7
	ror.l	d7,d6
	move.w	(a0)+,d6
	rol.l	d7,d6
11$	move.l	d2,-(sp)
	moveq	#1,d2
	lsl.l	d1,d2
	subq.l	#1,d2
	move.l	d2,d1
	move.l	(sp)+,d2
	and.w	d1,d0
	rts

12$	moveq	#9,d2
	lea	$9E(a6),a4
	lea	-2(a6),a5
	bsr.s	13$
	moveq	#4,d2
	lea	$80(a6),a4
	lea	$1E(a6),a5
13$	movem.l	d2-d5/a1/a2,-(sp)
	moveq	#4,d1
	bsr.s	10$
	moveq	#15,d5
	sub.w	d0,d5
	movea.w	d5,a1
	suba.l	a3,a3
	moveq	#0,d4
	move.w	d0,d5
14$	addq.w	#1,d4
	move.w	d4,d1
	cmp.w	d2,d1
	ble.s	15$
	move.w	d2,d1
15$	bsr.s	10$
	move.w	d0,(a2)+
	adda.w	d0,a3
	subq.w	#1,d5
	bne.s	14$
	move.w	a1,d5
16$	beq.s	17$
	clr.w	(a2)+
	subq.w	#1,d5
	bra.s	16$

17$	move.w	a3,d5
18$	move.w	d2,d1
	bsr.s	10$
	move.w	d0,(a4)+
	subq.w	#1,d5
	bne.s	18$
	movem.l	(sp)+,d2-d5/a1/a2
	movem.l	d2-d7/a2,-(sp)
	clr.w	(a5)+
	moveq	#0,d2
	moveq	#0,d3
	moveq	#-1,d4
	moveq	#15,d7
19$	move.w	(a2)+,d6
	move.w	d3,$40(a5)
	move.w	-2(a5),d0
	add.w	d0,d0
	sub.w	d0,$40(a5)
	add.w	d6,d3
	add.w	d6,d2
	move.w	d2,(a5)+
	lsl.w	#1,d2
	subq.l	#1,d7
	bne.s	19$
	movem.l	(sp)+,d2-d7/a2
	rts



;****************************************************************************
;****************************************************************************
;****************************************************************************
;
; GC!2 decruncher: A0=source (after header), A1=dest

GC2_DECRUNCH
	movem.l	d0-d7/a0-a6,-(sp)
	lea	-$454(sp),sp
;	cmpi.l	#$47432102,(a0)+
;	bne.l	9$

	move.l	(a0)+,d1		; d1 = unpacked length
	lea	0(a1,d1.l),a1		; a1 = end of destination buffer

	; heres a funny bit of code, that
	; seems to 'save' the 52 bytes after
	; the end of the decrunch buffer -
	; does it overwrite it in the routine?
	moveq	#52,d0
	lea	0(sp,d0.w),a2
1$	move.b	(a1)+,-(a2)
	subq.l	#1,d0
	bne.s	1$

	move.l	(a0)+,d2		; d2 = packed length
	adda.l	d2,a0			; a0 = end of source buffer
	lea	$54(sp),a6
	move.w	-(a0),d0
	move.l	-(a0),d6
	moveq	#$10,d7
	sub.w	d0,d7
	lsr.l	d7,d6
	move.w	d0,d7
	moveq	#$10,d3
2$	lea	$34(sp),a2
	bsr.l	16$
	moveq	#$10,d1
	bsr.l	14$
	move.w	d0,d5
	lea	$9E(a6),a2
	lea	-$1E(a2),a5
3$

;	move.l	a6,-(sp)
;	lea	COLOUR1_DATA(pc),a6	; write colour into copperlist
;	move.w	d6,(a6)			; to show decrunching
;	movea.l	(sp)+,a6

	movea.l	a6,a4
	bsr.s	10$
	btst	#8,d0
	bne.s	6$
	move.w	d0,d4
	lea	$20(a6),a4
	exg	a5,a2
	bsr.s	10$
	exg	a5,a2
	move.w	d0,d1
	move.w	d0,d2
	bne.s	4$
	moveq	#1,d1
	moveq	#$10,d2
4$	bsr.s	14$
	bset	d2,d0
	lea	1(a1,d0.w),a3
5$	move.b	-(a3),-(a1)
	subq.w	#1,d4
	bpl.s	5$
	move.b	-(a3),-(a1)
	move.b	-(a3),d0
6$	move.b	d0,-(a1)
	subq.w	#1,d5
	bpl.s	3$
	moveq	#1,d1
	bsr.s	14$
	bne.s	2$
	move.l	-(a0),d0
	divu.w	#$34,d0
	addq.w	#1,d0
7$	movem.l	(a1),d1-d7/a0/a2-a6
	movem.l	d1-d7/a0/a2-a6,-(a1)
	lea	$68(a1),a1
	subq.w	#1,d0
	bne.s	7$

	; and here it is again to restore?
	swap	d0
	moveq	#52,d1
	suba.l	d1,a1
	adda.l	d0,a1
	lea	(sp),a0
8$	move.b	(a0)+,-(a1)
	subq.l	#1,d1
	bne.s	8$

9$	lea	$454(sp),sp
	movem.l	(sp)+,d0-d7/a0-a6
	rts

10$	moveq	#0,d1
11$	subq.w	#1,d7
	beq.s	12$
	lsr.l	#1,d6
	bra.s	13$

12$	moveq	#$10,d7
	move.w	d6,d0
	lsr.l	#1,d6
	swap	d6
	move.w	-(a0),d6
	swap	d6
	lsr.w	#1,d0
13$	addx.w	d1,d1
	move.w	(a4)+,d0
	cmp.w	d1,d0
	bls.s	11$
	add.w	$3E(a4),d1
	add.w	d1,d1
	move.w	0(a2,d1.w),d0
	rts

14$	move.w	d6,d0
	lsr.l	d1,d6
	sub.w	d1,d7
	bgt.s	15$
	add.w	d3,d7
	ror.l	d7,d6
	move.w	-(a0),d6
	rol.l	d7,d6
15$	move.l	d2,-(sp)
	moveq	#1,d2
	lsl.l	d1,d2
	subq.l	#1,d2
	move.l	d2,d1
	move.l	(sp)+,d2
	and.w	d1,d0
	rts

16$	moveq	#9,d2
	lea	$9E(a6),a4
	lea	-2(a6),a5
	bsr.s	17$
	moveq	#4,d2
	lea	$80(a6),a4
	lea	$1E(a6),a5
17$	movem.l	d2-d5/a1/a2,-(sp)
	moveq	#4,d1
	bsr.s	14$
	moveq	#15,d5
	sub.w	d0,d5
	movea.w	d5,a1
	suba.l	a3,a3
	moveq	#0,d4
	move.w	d0,d5
18$	addq.w	#1,d4
	move.w	d4,d1
	cmp.w	d2,d1
	ble.s	19$
	move.w	d2,d1
19$	bsr.s	14$
	move.w	d0,(a2)+
	adda.w	d0,a3
	subq.w	#1,d5
	bne.s	18$
	move.w	a1,d5
20$	beq.s	21$
	clr.w	(a2)+
	subq.w	#1,d5
	bra.s	20$

21$	move.w	a3,d5
22$	move.w	d2,d1
	bsr.s	14$
	move.w	d0,(a4)+
	subq.w	#1,d5
	bne.s	22$
	movem.l	(sp)+,d2-d5/a1/a2
	movem.l	d2-d7/a2,-(sp)
	clr.w	(a5)+
	moveq	#0,d2
	moveq	#0,d3
	moveq	#-1,d4
	moveq	#15,d7
23$	move.w	(a2)+,d6
	move.w	d3,$40(a5)
	move.w	-2(a5),d0
	add.w	d0,d0
	sub.w	d0,$40(a5)
	add.w	d6,d3
	add.w	d6,d2
	move.w	d2,(a5)+
	lsl.w	#1,d2
	subq.l	#1,d7
	bne.s	23$
	movem.l	(sp)+,d2-d7/a2
	rts


GC2exe_name	dc.b	'Grants Computing (GC!2)',0
GC3exe_name	dc.b	'Grants Computing (GC!3)',0
GC2data_name	dc.b	'Grants Computing (GC!2) Data',0
GC3data_name	dc.b	'Grants Computing (GC!3) Data',0
	dc.b	'$VER: KennyGrant 1.2 (06.05.2000) by <kyzer@4u.net>',0
