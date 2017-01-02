
; $Id: vx_asm.s,v 1.1 2004/01/20 06:24:59 stoecker Exp $
; Amiga MC680x0 assembly support routines for VX
;
; VX - User interface for the XAD decompression library system.
; Copyright (C) 1999 and later by Andrew Bell <mechanismx@lineone.net>
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

; This file was created on Tue/4/Apr/2000

*****************************************************************************

TOLOWER	MACRO		; \1 = char reg [32 bit]
		cmp.l	#$DF,\1
		bhi.b	.\@_exit
		cmp.w	#$C0,\1
		bhs.b	.\@_add
		cmp.w	#'Z',\1
		bhi.b	.\@_exit
		cmp.w	#'A',\1
		blo.b	.\@_exit
.\@_add
		add.w	#'a'-'A',\1
.\@_exit
		ENDM

TOUPPER	MACRO		; \1 = char reg [32 bit]
		cmp.l	#$FE,\1
		bhi.b	.\@_exit
		cmp.w	#$E0,\1
		bhs.b	.\@_sub
		cmp.w	#'z',\1
		bhi.b	.\@_exit
		cmp.w	#'a',\1
		blo.b	.\@_exit
.\@_sub
		add.w	#'A'-'a',\1
.\@_exit
		ENDM

*****************************************************************************

		incdir	AsmInc:
		include	mymacros.i
		include	exec/types.i
		include	lvo/exec_lib.i
		include	lvo/graphics_lib.i
		include	lvo/utility_lib.i

*****************************************************************************

		section	text,code
		mc68020

		ifd	_PHXASS_
			ttl	v_asm.s
			symdebug
			linedebug
			opt	3
		endc

******* AB.LIB/SearchMem ****************************************************
*
*   NAME   
*       SearchMem - Search memory for a binary string.
*
*   SYNOPSIS
*       Offset =  SearchMem( Mem, MemLen, CmpStr, CmpStrLen)
*       D0                   A0   D0      A1      D1
*
*       extern ULONG SearchMem( REG_A0 void *Mem,
*                               REG_D0 ULONG MemLen,
*                               REG_A1 UBYTE *CmpStr,
*                               REG_D1 ULONG CmpStrLen );
*
*   FUNCTION
*       Look for a binary string in memory.
*
*   INPUTS
*       Mem       - Pointer to memory that will be searched.
*       MemLen    - Length of memory to be searched.
*       CmpStr    - Pointer to binary string to find.
*       CmpStrLen - Length of binary string to find
*
*   RESULT
*       ULONG Offset - Offset the binary string was found at (relative)
*                      to the Mem parameter. If string was not found then
*                      0xffffffff is returned.
*
*****************************************************************************
* AB.LIB code module, created: 4/8/1997
*
* This file is copyright © 1997 Andrew Bell.
*
* ULONG SearchMem( REG_A0 void *Mem,
*                  REG_D0 ULONG MemLen,
*                  REG_A1 UBYTE *CmpStr,
*                  REG_D1 ULONG CmpStrLen );
*

		xdef	SearchMem
		xdef	_SearchMem

		cnop	0,4

SearchMem:
_SearchMem:
		PUSH	d1-d4/a2-a4

		move.l	a0,a4
		tst.l	d0		; Check params
		beq.b	SM.Fail
		tst.l	d1
		beq.b	SM.Fail
		move.b	(a1)+,d2	; Get first byte from match string
		subq.l	#1,d1

SM.Loop
		cmp.b	(a0)+,d2	; Compare next byte
		bne.b	SM.NextByte
		move.l	a0,a2
		move.l	a1,a3
		move.l	d1,d3
		beq.b	SM.Matched
		move.l	d0,d4		; << BEQ.B here >>

SM.MatchStr
		cmpm.b	(a0)+,(a1)+	; Match rest of string
		bne.b	SM.NoMatch
		subq.l	#1,d3
		beq.b	SM.Matched
		subq.l	#1,d4
		bne.b	SM.MatchStr
		bra.b	SM.Fail

SM.Matched
		subq.l	#1,a2
		move.l	a2,d0
		sub.l	a4,d0
		bra.b	SM.Fin

SM.NoMatch
		move.l	a3,a1
		move.l	a2,a0

SM.NextByte
		subq.l	#1,d0
		bne.b	SM.Loop

SM.Fail
		moveq.l	#-1,d0

SM.Fin
		PULL	d1-d4/a2-a4
		rts

*****************************************************************************

		xdef	SearchMemNoCase
		xdef	_SearchMemNoCase

		cnop	0,4

SearchMemNoCase:		; Same as SearchMem, but case insensitive.
_SearchMemNoCase:
		PUSH	d1-d6/a2-a4

		move.l	a0,a4
		tst.l	d0		; Check params
		beq.w	SMNC.Fail
		tst.l	d1
		beq.w	SMNC.Fail
		moveq	#0,d5		; Clear for TOLOWER
		moveq	#0,d6
		moveq	#0,d2

		move.b	(a1)+,d2	; Get first byte from match string
		subq.l	#1,d1
		TOLOWER	d2

SMNC.Loop
		move.b	(a0)+,d5
		TOLOWER	d5
		cmp.b	d5,d2		; Compare next byte
		bne.b	SMNC.NextByte
		move.l	a0,a2
		move.l	a1,a3
		move.l	d1,d3
		beq.b	SMNC.Matched
		move.l	d0,d4		; << BEQ.B here >>

SMNC.MatchStr
		move.b	(a0)+,d5
		TOLOWER	d5
		move.b	(a1)+,d6
		TOLOWER	d6
		cmp.b	d5,d6		; Match rest of string
		bne.b	SMNC.NoMatch
		subq.l	#1,d3
		beq.b	SMNC.Matched
		subq.l	#1,d4
		bne.b	SMNC.MatchStr
		bra.b	SMNC.Fail

SMNC.Matched
		subq.l	#1,a2
		move.l	a2,d0
		sub.l	a4,d0
		bra.b	SMNC.Fin

SMNC.NoMatch
		move.l	a3,a1
		move.l	a2,a0

SMNC.NextByte
		subq.l	#1,d0
		bne.w	SMNC.Loop
SMNC.Fail
		moveq.l	#-1,d0

SMNC.Fin
		PULL	d1-d6/a2-a4
		rts

*****************************************************************************

		end
