/*
 $Id: vx_arc.h,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Defines for archive handling.

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

#ifndef VX_ARC_H
#define VX_ARC_H 1

/* These 4 bytes id strings are useful when you're reading direct memory dumps
   during debugging. They help you figure out what type of structure you're
   looking at. If you're really anal about memory usage, you could wrap the
   xxx_DebugID fields (found in various structs) with #ifdefs/#endif to prevent
   them from being included and allocated in release builds. The ArcEntry
   structs can be allocated thousands of times, so for an archive with 1000
   entries, you'd save 4K. */

#define DEBUGID_AHN     MAKE_ID('A','H','N','!')
#define DEBUGID_AHNBAD  MAKE_ID('B','A','D','!')
#define DEBUGID_AE      MAKE_ID('!','A','E','!')

#define DD_ADF_SIZE 901120
#define HD_ADF_SIZE 1802240


/*********************************************************************
 *
 * PortionNode (aka PN)
 *
 * Each archive is represented in "portions" or segments. This allows
 * us to take advantage of XAD's multi-FS features, etc. Every AHN has
 * one or more portions.
 *
 *********************************************************************
 *
 */

struct PN /* Portion Node */ {
	struct MinNode         pn_node;
	ULONG                  pn_type;
	struct xadArchiveInfo *pn_ai;

	BOOL                   pn_align;
	BOOL                   pn_ready;

	/* The following fields are only valid if "pn->pn_ready" is TRUE */

	ULONG                  pn_expsize;
	ULONG                  pn_filecnt;
	struct ArcEntry       *pn_rootae;             /* Free with ARC_AETree_Delete() */
	struct ArcEntry       *pn_currentae;          /* Current dir that we're in */
	ULONG                  pn_linear_cnt;
	struct ArcEntry      **pn_linear_aes_xad;     /* Free with MEM_FreeVec() */
	ULONG                  pn_linear_aes_xadSize;
	struct ArcEntry      **pn_linear_aes_alpha;   /* Free with MEM_FreeVec() */
	ULONG                  pn_linear_aes_alphaSize;
	BOOL                   pn_linear_aes_alpha_ready;
	BOOL                   pn_align2;
};

enum /* Defines for pn->pn_ready */
{
    PNTYPE_ARCHIVE = 0, /* GetInfo returned by xadGetInfo() */
    PNTYPE_DISK         /* GetInfo returned by xadGetDiskInfo() */
};

/*********************************************************************
 *
 * ArcHistoryNode (aka AHN)
 * ------------------------
 * This is the structure created by ARC_CreateAHN(). It represents a
 * loaded archive. It must be freed with a call to ARC_FreeAHN() when
 * it's not required anymore.
 *
 * Notes
 * -----
 * ahn_EmbeddedSingle will always be FALSE if the user has has
 * configured VX not to enter into UNIX archives (i.e. TGZs/RPMs/etc)
 * immediately.
 *
 *********************************************************************
 *
 */

#define MAX_INFOTEXTS 10

struct ArcHistoryNode {
	ULONG                 ahn_DebugID;
	APTR                  ahn_Pool;                  /* Free with DeletePool() */

	ULONG                 ahn_MemUsage;

	UBYTE                *ahn_Path;                  /* Free with MEM_FreeVec() */
	ULONG                 ahn_PathSize;
	BPTR                  ahn_ArcLock;
	struct FileInfoBlock *ahn_FIB;                   /* Free with MEM_FreeVec() */
	UBYTE                 ahn_DateBuf[130];
	UBYTE                *ahn_ArcPassword;           /* Free with MEM_FreeVec() */
	BOOL                  ahn_EmbeddedArc;

	/* This embedded archive is really a Singly Compressed Archive? (i.e. #?.TGZ) */

	BOOL                  ahn_EmbeddedSCA;

	UBYTE                *ahn_EmbeddedPath;          /* Free with MEM_FreeVec() */
	ULONG                 ahn_EmbeddedPathSize;
	UBYTE                *ahn_ParentArcPath;         /* Free with MEM_FreeVec() */
	ULONG                 ahn_ParentArcPathSize;
	UBYTE                 ahn_EmbeddedListStr[1024];
	LONG                  ahn_PNListCnt;
	struct MinList        ahn_PNList;
	struct PN            *ahn_CurrentPN;
	UBYTE                *ahn_InfoTexts[MAX_INFOTEXTS + 1];
	ULONG                 ahn_InfoTextsSizes[MAX_INFOTEXTS + 1];
	UBYTE                 ahn_SizeStr[32];

	struct xadArchiveInfo *ahn_DiskAI;
	BOOL                   ahn_IsDiskArc;
};

enum
{
    SORTMODE_DIRSABOVE = 0,
    SORTMODE_MIXED,
    SORTMODE_DIRSBELOW
};

/*********************************************************************
 *
 * ArcEntry (aka AE)
 * -----------------
 * Each file in the archive tree is represented by an AE structure.
 *
 * Notes
 * -----
 * In order to make tree building faster, some of the following
 * fields do not get initialized until they're needed. These
 * fields are ae_DS, ae_ProtVec, 
 *
 *********************************************************************
 *
 */

#ifndef LEN_DATSTRING
#define LEN_DATSTRING 16
#endif

struct ArcEntry {
	struct MinNode         ae_Node;
	ULONG                  ae_DebugID;
	struct ArcHistoryNode *ae_ahn;

	BOOL                ae_IsDir;
	BOOL                ae_NodeOpened;
	BOOL                ae_DateValid;
	BOOL                ae_SizeValid;
	BOOL                ae_ProtValid;
	BOOL                ae_ChildAEListSorted;
	BOOL                ae_ChildAETreeSorted;
	BOOL                ae_padding1;                  /* Keeps next field aligned to 32 bits */

	LONG                ae_XADPos;
	ULONG               ae_XADPathPoint;
	LONG                ae_Size;
	UBYTE              *ae_Name;                      /* Free with MEM_FreeVec() */
	UBYTE              *ae_FullPath;                  /* Free with MEM_FreeVec() */
	ULONG               ae_NameSize;
	ULONG               ae_FullPathSize;
	struct xadFileInfo *ae_xfi;
	UBYTE               ae_SizeBuf[12];               /* 12 characters = 100GB max */
	UBYTE               ae_ProtBuf[20];
	ULONG               ae_ListerPos;
	UBYTE              *ae_SuffixPtr;                 /* Points to the suffix of ae_Name */

	struct MUI_NListtree_TreeNode *ae_TreeNode;       /* For use by NListtree display only */
	struct DateStamp    ae_DS;
	struct ArcEntry    *ae_ParentAE;
	struct MinList      ae_ChildAEL;

	UBYTE               ae_DayPart[LEN_DATSTRING+1];  /* Keep these at the end. */
	UBYTE               ae_DatePart[LEN_DATSTRING+1];
	UBYTE               ae_TimePart[LEN_DATSTRING+1];

	/* Next entry is for use by the Tree display hooks only so we can
	   preparse white onto dir names. I'd like to make it totally obsolete. */

	UBYTE               ae_DisplayBuf[32];
};

#define EMBEDDEDARC_SEPARATOR      " :--> "
#define LSLIMIT                    sizeof(ahn->ahn_EmbeddedListStr)
#define ISARC_SEGLEN               (32 * 1024)
#define MIN_EXTRACT_PROGRESS_SIZE  50000
#define GETAHNMEM(size)            AllocPooled(ahn->ahn_Pool, size)
#define FREEAHNMEM(mem, size)                 \
	if (mem)                                  \
	{                                         \
		FreePooled(ahn->ahn_Pool, mem, size); \
		ahn->ahn_MemUsage -= (size);          \
	}

typedef struct ArcHistoryNode AHN;
typedef struct PN PN;
typedef struct ArcEntry AE;

#define AE_SIZE  sizeof(AE)
#define AHN_SIZE sizeof(AHN)
#define PN_SIZE  sizeof(PN)

#ifndef VX_ARC_PROTOS_H
#include "vx_arc_protos.h"
#endif

#define AHN_POOLSIZE (32 * 1024)

#endif /* VX_ARC_H */

