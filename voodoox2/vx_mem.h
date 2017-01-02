/*
 $Id: vx_mem.h,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Memory module definitions.

 VX - User interface for the XAD decompression library system.
 Copyright (C) 1999 and later by Andrew Bell <mechanismx@lineone.net>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef VX_MEM_H
#define VX_MEM_H 1

#ifndef VX_INCLUDE_H
#include "vx_include.h"
#endif

#ifndef VX_COMPILERDEFS_H
#include "vx_compilerDefs.h"
#endif

/* If COMPILER_MEMORY_DEBUG is set, then memory debugging will be
   activated. Any memory vectors that weren't correctly freed will be
   displayed in the Shell window when the program quits. Don't activate
   debugging in release code! It slows the program waaaay down! */

#undef COMPILER_MEMORY_DEBUG

#define POOL_PUDDLESIZE  (8 * 1024)
#define POOL_THRESHSIZE  (POOL_PUDDLESIZE / 4)

#ifdef COMPILER_MEMORY_DEBUG

	APTR MEM_AllocVec( ULONG Size );               /* Protos for the C memory debugging routines */
	void MEM_FreeVec( APTR Vec );
	UBYTE *MEM_StrToVec( UBYTE *Str );

#else

	/* TODO: The makefile should be responsible for defining COMPILER_MEMORY_DEBUG
	         and linking in the required asm module, but it doesn't. Fix that!!!! */

	extern ASM(APTR)   MEM_AllocVec( REG_D0 (ULONG Size) );  /* Protos for the asm memory routines */
	extern ASM(void)   MEM_FreeVec(  REG_A1 (APTR Vec)   );
	extern ASM(UBYTE) *MEM_StrToVec( REG_A0 (UBYTE *Str) );

#endif /* COMPILER_MEMORY_DEBUG */

#ifndef VX_MEM_PROTOS_H
#include "vx_mem_protos.h"
#endif

#endif /* VX_MEM_H */
