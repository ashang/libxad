
; $Id: vx_mem_asm.s,v 1.1 2004/01/20 06:24:59 stoecker Exp $
; VX assembly support routines for memory allocation.
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

; This file was created on Sun/27/Feb/2000

		ifd	_PHXASS_
		opt	3
		endc

;***************************************************************************;
; GPROTO extern APTR MEM_AllocVec( REG_D0 ULONG Size );

		xdef	_MEM_AllocVec
		xref	_SysBase
		xref	_MemPool
		xref	_LVOAllocPooled

		cnop	0,4

_MEM_AllocVec	movem.l	d2/a6,-(sp)

		tst.l	d0
		beq.b	.Exit
		addq.l	#4,d0
		move.l	_SysBase,a6
		move.l	d0,d2
		move.l	_MemPool,a0
		jsr	_LVOAllocPooled(a6)
		tst.l	d0
		beq.b	.Exit
		move.l	d0,a0
		move.l	d2,(a0)
		addq.l	#4,d0

.Exit		movem.l	(sp)+,d2/a6
		rts

;***************************************************************************;
; GPROTO extern APTR MEM_FreePooled( REG_A1 APTR Vec );

		xdef	_MEM_FreeVec
		xref	_LVOFreePooled

		cnop	0,4

_MEM_FreeVec	move.l	a6,-(sp)

		move.l	a1,d0
		beq.b	.Exit

		move.l	_SysBase,a6
		move.l	_MemPool,a0
		move.l	-(a1),d0

;		movem.l	d1/a2,-(sp)
;		move.l	a1,a2
;		move.l	d0,d1
;.Loop		move.b	#'x',(a2)+
;		subq.l	#1,d1
;		bne.b	.Loop
;		movem.l	(sp)+,d1/a2,

		jsr	_LVOFreePooled(a6)

.Exit		move.l	(sp)+,a6
		rts

;***************************************************************************;
; GPROTO extern UBYTE *MEM_StrToVec( REG_A0 UBYTE *Str );

		xdef	_MEM_StrToVec

		cnop	0,4

_MEM_StrToVec	movem.l	d2/a2/a6,-(sp)

		move.l	a0,d0
		beq.b	.Exit
		moveq	#-(1+1+4),d0	; (NULL + Size storage)
		move.l	a0,a2
.StrLenLoop	tst.b	(a0)+
		dbeq	d0,.StrLenLoop
		move.l	_SysBase,a6
		not.l	d0
		move.l	_MemPool,a0
		move.l	d0,d2
		jsr	_LVOAllocPooled(a6)
		tst.l	d0
		beq.b	.Exit
		move.l	d0,a0
		move.l	d2,(a0)+
		move.l	a0,d0
.StrCpyLoop	move.b	(a2)+,(a0)+
		bne.b	.StrCpyLoop

.Exit		movem.l	(sp)+,d2/a2/a6
		rts


		end
		
	
