/*
 $Id: vx_mem.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Memory managment (also contains leak detection code)

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

#include "vx_include.h"
#include "vx_mem.h"
#include "vx_global.h"

/* If the following define is set, then the memory allocation
   and deallocation routines will be imported from the v_mem_asm
   module. Else the C versions below will be used. */

#define COMPILER_IMPORT_FROM_V_MEM_ASM
#define LOWMEM_HANDLER_PRIORITY 25

LONG  MEM_Flush_SigNum  = -1;
ULONG MEM_Flush_SigFlag = 0;
APTR  MemPool           = NULL;

#ifdef COMPILER_MEMORY_DEBUG
#define MEM_MAXREC 50000 /* 50000 free record slots */

// ATM we use basic linear scanning. Speed could be
// improved by using a binary search or even an AVL
// tree - to find allocated pointers.

struct MemRec {
	APTR mr_Vec;
	ULONG mr_Size;
};
struct MemRec *MRA_vec;

struct MemRec *FindVecRecord( APTR vec ) {

	register ULONG index;

	for (index = 0; index < MEM_MAXREC; index++) {
		if (MRA_vec[index].mr_Vec == vec) {
			return &MRA_vec[index];
		}
	}

	return NULL;
}

#define FILTER_SIZE (4 * 5)

void ShowVecRecords( void ) {

	register ULONG index;

	Printf( "------------------------------------------------------\n"
	        "DEBUG: The following pool vectors have not been freed:\n"
	        "------------------------------------------------------\n"
	        "\n");

	/* Show all vector allocations that weren't correctly freed. Along
	   with a mimi memory dump of the first 20 bytes of the vector. */

	for (index = 0; index < MEM_MAXREC; index++) {

		if (MRA_vec[index].mr_Vec != NULL) {


			UBYTE FltBuf[FILTER_SIZE+2];
			register UBYTE *In = MRA_vec[index].mr_Vec, *out = FltBuf;

			while (out <= FltBuf + FILTER_SIZE) {
				if (isprint(*In++)) {
					*out++ = In[-1];
				} else {
					*out++ = '.';
				}
			}

			*out = 0;

			Printf("Index: %04lu Vec: 0x%08lx Size: %-7lu\n"
			       "Dump: [%.20s] Hex: [%08lx %08lx %08lx %08lx]\n\n",
			       index, MRA_vec[index].mr_Vec, MRA_vec[index].mr_Size, FltBuf,
			       ((ULONG *)MRA_vec[index].mr_Vec)[0], ((ULONG *)MRA_vec[index].mr_Vec)[1],
			       ((ULONG *)MRA_vec[index].mr_Vec)[2], ((ULONG *)MRA_vec[index].mr_Vec)[3]);
		}
	}
}

void AddVecRecord( APTR vec, ULONG size ) {

	register ULONG index;

	/* Find the first empty record slot, and fill it. */

	for (index = 0; index < MEM_MAXREC; index++) {
		if (MRA_vec[index].mr_Vec == NULL) {
			MRA_vec[index].mr_Vec = vec;
			MRA_vec[index].mr_Size = size;

			break;
		}
	}
}

void RemVecRecord( APTR vec ) {
	register struct MemRec *MR;

	/* Find and clear a record slot */

	if (MR = FindVecRecord(vec)) {
		MR->mr_Vec = NULL;
		MR->mr_Size = 0;
	}
}

APTR MEM_AllocVec( ULONG size ) {

	register ULONG *vec;

	size += 4;

	if (vec = AllocPooled(MemPool, size)) {
		*vec++ = size;
		AddVecRecord(vec, size);
	}

	return (APTR) vec;
}

void MEM_FreeVec( APTR vec ) {

	if (!vec) {
		return;
	}

	if (FindVecRecord(vec)) {

		RemVecRecord(vec);
		FreePooled(MemPool, ((UBYTE *) vec) - 4, ((ULONG *) vec)[-1]);

	} else {

		Printf("DEBUG: Bad MEM_FreeVec() call (vec=0x%08lx, size=%ld)\n", vec, ((ULONG *) vec)[-1]);

	}
}

UBYTE *MEM_StrToVec( UBYTE *Str ) {

	register UBYTE *StrBuf;

	if (!Str) {
		return NULL;
	}

	if (StrBuf = MEM_AllocVec(strlen(Str) + 1)) {
		strcpy(StrBuf, Str);
	}

	return StrBuf;
}

#endif /* COMPILER_MEMORY_DEBUG */

SAVEDS ASM(LONG) LowMemHandlerCode( REG_A0 (struct MemHandlerData *mhd) ) {

	/* Low memory handler for exec.library.
	 *
	 * This is the actual low memory handler. It simply signals the main
	 * task to free it's memory resources.
	 *
	 * Remember that this routine is being called on the context of
	 * AllocMem() via another task! Stack usage MUST be within 128 bytes.
	 *
	 * If the requested size is this big, then we'll assume that some
	 * idiot is trying to flush the memory. VX will only flush
	 * its resources in real low memory situations.
	 */

	if (mhd->memh_RequestSize & 0xff000000) {
		return MEM_DID_NOTHING;
	}

	Signal(&VXProcess->pr_Task, MEM_Flush_SigFlag);

	return MEM_ALL_DONE;
}

struct Interrupt LowMemHandlerData = {
	{ NULL, NULL, NT_INTERRUPT, LOWMEM_HANDLER_PRIORITY, "VX's LowMem Handler" },
	NULL, (void *) &LowMemHandlerCode
};

GPROTO BOOL MEM_Init( void ) {

	if (!(MemPool = (APTR) CreatePool(MEMF_CLEAR, POOL_PUDDLESIZE, POOL_THRESHSIZE))) {
		return FALSE;
	}

	/* Note: Memory handlers are only available under exec.library version 39+. */

	AddMemHandler((struct Interrupt *) &LowMemHandlerData);

	MEM_Flush_SigNum = AllocSignal(-1);

	if (MEM_Flush_SigNum == -1) {
		return FALSE;
	} else {
		MEM_Flush_SigFlag = (1 << MEM_Flush_SigNum);
	}

#ifdef COMPILER_MEMORY_DEBUG
	Printf("DEBUG: Memory pool vector tracking is active.\n");

	if (!(MRA_vec = AllocVec(MEM_MAXREC * sizeof(struct MemRec), MEMF_CLEAR))) {
		MEM_Free();
		return FALSE;
	}
#endif /* COMPILER_MEMORY_DEBUG */

	return TRUE;
}

GPROTO void MEM_Free( void ) {

	RemMemHandler((struct Interrupt *) &LowMemHandlerData);

	if (MEM_Flush_SigNum != -1) {
		FreeSignal(MEM_Flush_SigNum);
		MEM_Flush_SigNum = -1;
	}

	if (MemPool) {

#ifdef COMPILER_MEMORY_DEBUG
		ShowVecRecords();
#endif /* COMPILER_MEMORY_DEBUG */

		DeletePool(MemPool);
		MemPool = NULL;
	}

#ifdef COMPILER_MEMORY_DEBUG
	if (MRA_vec) {
		FreeVec(MRA_vec);
	}
#endif /* COMPILER_MEMORY_DEBUG */
}

