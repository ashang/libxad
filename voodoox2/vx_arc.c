/*
 $Id: vx_arc.c,v 1.3 2004/01/21 17:53:42 andrewbell Exp $
 Holds core archive handling functions.

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
#include "vx_main.h"
#include "vx_arc.h"
#include "vx_mem.h"
#include "vx_cfg.h"
#include "vx_io.h"
#include "vx_misc.h"
#include "vx_debug.h"
#include "vx_global.h"
#include "vx_virus.h"
#include "Classes.h"

static ULONG G_StrSize = 0;

UBYTE *ARC_AllocStr( AHN *ahn, STRPTR str, ULONG *size_ptr ) {
	STRPTR strvec;

	if (!str) {
		str = "";
	}

	if ((strvec = GETAHNMEM(*size_ptr = strlen(str) + 1))) {

		ahn->ahn_MemUsage += *size_ptr;
		G_StrSize += *size_ptr;
		strcpy(strvec, str);
		str = strvec;

		while (*str) {
			if (*str == 0x1b) {
				*str = ' ';			/* Prevents MUI exploit.*/
			} else {
				str++;
			}
		}
	}
	return strvec;
}

GPROTO BOOL ARC_Init( void ) {
	DB(dbg("Init Arc..."))

	if (xvsBase) {
		if (!xvsSelfTest()) {
			GUI_PrgError("xvs.library has failed its self test!\n"
				"Please investigate your xvs.library .", NULL);
			return FALSE;
		}
	}

	return TRUE;
}

GPROTO void ARC_Free( void ) {
	DB(dbg("Free ARC..."))
}

GPROTO void ARC_Flush( void ) {
	DoMethod(GETARCHISTORY, MUIM_ArcHistory_Flush);
}

/*******************************************************************************************/
/* File extract progress hook */
/*******************************************************************************************/

BOOL ProgressHook_OverwriteAll = FALSE; /* This is a bit of a kludge... */
static ULONG G_ProgressCount = 0;

GPROTO SAVEDS ASM(ULONG) ARC_ExtractFileProgressFunc(
    REG_A0 (struct Hook *Hk),
    REG_A1 (struct xadProgressInfo *xpi) ) {

	ULONG Result = 0;
	BOOL Abort = FALSE;

	/* If Hk->h_Data == FALSE then we must not do any progress window operations. */

	switch(xpi->xpi_Mode) {

	case XADPMODE_PROGRESS: {

			ULONG BytesLeft;
			DB(dbg("XADProgressFunc got XADPMODE_PROGRESS"));

			if (Hk->h_Data) { /* Note: Hk->h_Data is actually a BOOL */

				if (xpi->xpi_FileInfo->xfi_Flags & XADFIF_NOUNCRUNCHSIZE) {

					Abort = UpdateProgress("Final size unknown, %lu bytes done so far...",
					                       &xpi->xpi_CurrentSize, 0);

				} else {

					BytesLeft = xpi->xpi_FileInfo->xfi_Size - xpi->xpi_CurrentSize;
					Abort = UpdateProgress("%lu bytes left...", &BytesLeft, xpi->xpi_CurrentSize);
				}
			}
			break;
		}

	case XADPMODE_ASK: {

			ULONG status = xpi->xpi_Status, r;
			UBYTE *new_name;

			DB(dbg("XADProgressFunc got XADPMODE_ASK"));

			if (status & XADPIF_OVERWRITE) {

				DB(dbg(" XADPIF_OVERWRITE is set in xpi->xpi_Status"));

				if (ProgressHook_OverwriteAll) {
					return XADPIF_OK | XADPIF_OVERWRITE;
				}
retry:
				r = GUI_Popup("Overwrite request",
				              "File '%s' already exists\n"
				              "What should I do?",
				              &xpi->xpi_FileName,
				              "Overwrite|Overwrite all|Skip|Rename|Abort");
				DB(dbg("Response = %lu", r))

				switch(r) {
				case 0: /* Abort */
					return 0;
				case 1: /* Overwrite */
					return XADPIF_OK | XADPIF_OVERWRITE;
				case 2: /* Overwrite all */
					ProgressHook_OverwriteAll = TRUE;
					return XADPIF_OK | XADPIF_OVERWRITE;
				case 3: /* Skip */
					return XADPIF_OK | XADPIF_SKIP; /* Skip */
				case 4: /* Rename */
					{
						if ((new_name = GUI_GetString("Rename file...",
						                              "Enter new name for file", NULL, xpi->xpi_FileName))) {

							if ((xpi->xpi_NewName = xadAllocVec(strlen(new_name) + 1, MEMF_ANY))) {
								strcpy(xpi->xpi_NewName, new_name);
							}

							return XADPIF_OK | XADPIF_RENAME;
						} else {
							goto retry;
						}
						break;
					}
				}
			}

			DB(if (status & XADPIF_MAKEDIRECTORY) dbg(" XADPIF_MAKEDIRECTORY is set in xpi->xpi_Status"))
			DB(if (status & XADPIF_ISDIRECTORY)   dbg(" XADPIF_ISDIRECTORY is set in xpi->xpi_Status"))
		}

	case XADPMODE_END: {
			DB(dbg("XADProgressFunc got XADPMODE_END"))
			if (Hk->h_Data) /* Note: Hk->h_Data is a BOOL */
			{
				UpdateProgress("Finished.", NULL, xpi->xpi_FileInfo->xfi_Size);
				G_ProgressCount = 0;
			}
			break;
		}
	}

	if (!(CheckSignal(SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) && !Abort) {
		Result |= XADPIF_OK;
	}

	return Result;
}

struct Hook ARC_ExtractFileProgressHook = {
	{ NULL, NULL }, (void *) ARC_ExtractFileProgressFunc, NULL, NULL
};

/********************************************************************************************
 *
 * AHN (Arc History Node) routines
 *
 * Routines for controlling AHNs.
 *
 ********************************************************************************************
 *
 */

void FreeTextInfos( AHN *ahn ) {
	LONG i;

	if (!ahn) {
		return;
	}

	for (i = 0; i < MAX_INFOTEXTS; i++) {
		if (!ahn->ahn_InfoTexts[i]) {
			continue;
		}

		FREEAHNMEM(ahn->ahn_InfoTexts[i], ahn->ahn_InfoTextsSizes[i]);
		ahn->ahn_InfoTexts[i] = NULL;
	}
}

void CollectTextInfos( AHN *ahn, struct xadArchiveInfo *ai ) {
	struct xadTextInfo *ti = NULL;
	UBYTE *p, *vec;
	LONG i = 0;

	if (!ahn || !ai) {
		return;
	}

	FreeTextInfos(ahn);

	if (ai->xai_DiskInfo) {
		ti = ai->xai_DiskInfo->xdi_TextInfo;
	}

	while (ti) {

		if (ti->xti_Size && ti->xti_Text) {

			if ((p = vec = GETAHNMEM(ahn->ahn_InfoTextsSizes[i] = ti->xti_Size + 4))) {

				ahn->ahn_MemUsage += ahn->ahn_InfoTextsSizes[i];
				memcpy(vec, ti->xti_Text, ti->xti_Size);

				while (p < vec + ti->xti_Size) {
					if((!isprint(*p) || *p == 0x1b) && *p != '\n') {
						*p = '.';
						p++;
					}
				}

				ahn->ahn_InfoTexts[i++] = vec;
			}

			if (i > MAX_INFOTEXTS) {
				break;
			}
		}

		ti = ti->xti_Next;
	}
}

UBYTE *GetFirstXADSplitPath( struct xadSplitFile *xad_sf ) {

	if (!xad_sf) {
		return NULL;
	}

	if (xad_sf->xsf_Type == XAD_INFILENAME) {
		return (UBYTE *) xad_sf->xsf_Data;
	}

	return NULL;
}

GPROTO AHN *ARC_CreateAHN( UBYTE *arcpath, AHN *parent_ahn, AE *embedded_ae, struct xadSplitFile *xad_sf ) {

	BOOL success = FALSE;
	PN *pn1, *pn2, *pn3;
	AHN *ahn;

	DB(dbg("ARC_CreateAHN(arcpath=\"%s\", parent_ahn=0x%08lx, embedded_ae=0x%08lx, xad_sf=0x%08lx)",
	       arcpath, parent_ahn, embedded_ae, xad_sf))

	if ((!arcpath && !xad_sf) || (arcpath && xad_sf)) {
		return NULL;
	}

	if ((ahn = MEM_AllocVec(AHN_SIZE))) {

		if (!(ahn->ahn_Pool = CreatePool(MEMF_CLEAR, AHN_POOLSIZE, AHN_POOLSIZE / 4))) {
			MEM_FreeVec(ahn);
			return NULL;
		}

		ahn->ahn_MemUsage           = AHN_SIZE;
		ahn->ahn_DebugID            = DEBUGID_AHN;
		ahn->ahn_PNListCnt          = 0;
		ahn->ahn_EmbeddedListStr[0] = 0;
		NewList((struct List *)&ahn->ahn_PNList);

		if (arcpath) {
			ahn->ahn_ArcLock = Lock(arcpath, SHARED_LOCK);
			ahn->ahn_Path    = ARC_AllocStr(ahn, arcpath, &ahn->ahn_PathSize);
			ahn->ahn_FIB     = IO_GetFIBVec(arcpath);

		} else if ((arcpath = GetFirstXADSplitPath(xad_sf))) {

			/* If we're a splitted archive, then we'll use the first
			   path as the base path. */

			ahn->ahn_ArcLock = Lock(arcpath, SHARED_LOCK);
			ahn->ahn_Path    = ARC_AllocStr(ahn, (STRPTR)xad_sf->xsf_Data, &ahn->ahn_PathSize);
			ahn->ahn_FIB     = IO_GetFIBVec((STRPTR)xad_sf->xsf_Data);

		} else /* Fail */ {

			ahn->ahn_ArcLock = 0;
			ahn->ahn_Path    = NULL;
			ahn->ahn_FIB     = NULL;

		}

		if (ahn->ahn_ArcLock && ahn->ahn_Path && ahn->ahn_FIB) {

			if (!DateStampToStr(&ahn->ahn_FIB->fib_Date, ahn->ahn_DateBuf, FALSE)) {
				strcpy((UBYTE *)&ahn->ahn_DateBuf, "(unknown date)");
			}

			if (parent_ahn && embedded_ae) {
				ARC_CreateAHN_PrepEmbedded(ahn, parent_ahn, embedded_ae);
			}

			GUI_PrintStatus("Loading %s (%lu bytes)...",
			                (ULONG) arcpath, ahn->ahn_FIB->fib_Size);

			pn1	= ARC_CreateAHN_AddArchivePN(ahn, arcpath, xad_sf);
			pn2	= ARC_CreateAHN_AddDiskPN   (ahn, arcpath, xad_sf);
			pn3	= ARC_CreateAHN_AddADFPN    (ahn, arcpath, xad_sf);

			if (pn1 || pn2 || pn3) {

				ahn->ahn_CurrentPN = (pn1 ? pn1 : (pn2 ? pn2 : pn3));

				if (ahn->ahn_CurrentPN) {
					success = TRUE;
				}

			} else {

				GUI_PrgError("Unknown file format: %s", &arcpath);

			}
		}
	}

	DB(dbg("ARC_CreateAHN(): Archive contains %lu portion%s",
	       ahn->ahn_PNListCnt, ahn->ahn_PNListCnt == 1 ? "" : "s"))

	if (!success) {

		ARC_FreeAHN(ahn);
		ahn = NULL;

	}

	return ahn;
}

#define FREEPNRESOURCES(pn)				\
if (pn->pn_ai)							\
{										\
	xadFreeInfo(pn->pn_ai);				\
	xadFreeObjectA(pn->pn_ai, NULL);	\
	pn->pn_ai = NULL;					\
}

GPROTO void ARC_FreeAHN( AHN *ahn ) {
	register PN *pn;

	DB(dbg("ARC_FreeAHN(ahn=0x%08lx)", ahn))

	if (!ahn) {
		return;
	}

	if (ahn->ahn_FIB) {
		MEM_FreeVec(ahn->ahn_FIB);
	}

	if (ahn->ahn_ArcLock) {
		UnLock(ahn->ahn_ArcLock);
	}

	/*while (pn = (PN *) RemHead(&ahn->ahn_PNList)) ARC_FreePN(ahn, pn);*/

	for (pn = (struct PN *) ahn->ahn_PNList.mlh_Head;
	     pn->pn_node.mln_Succ;
	     pn = (struct PN *) pn->pn_node.mln_Succ) {
		FREEPNRESOURCES(pn)
	}

	if (ahn->ahn_Pool) {
		DeletePool(ahn->ahn_Pool);
	}

	ahn->ahn_DebugID = DEBUGID_AHNBAD;
	MEM_FreeVec(ahn);
}

GPROTO BOOL ARC_UpdateFIB( AHN *ahn ) {
	APTR newfib;

	if (!ahn) {
		return FALSE;
	}

	if (!(newfib = IO_GetFIBVec(ahn->ahn_Path))) {
		return FALSE;
	}

	if (ahn->ahn_FIB) {
		MEM_FreeVec(ahn->ahn_FIB);
	}

	ahn->ahn_FIB = newfib;
	return TRUE;
}

GPROTO BOOL ARC_ChangeComment( AHN *ahn, UBYTE *comment ) {

	if (!ahn || !comment) {
		return FALSE;
	}

	if (!SetComment(ahn->ahn_Path, comment)) {
		return FALSE;
	}

	ARC_UpdateFIB(ahn);

	return TRUE;
}

BOOL CloseIt = FALSE;
ULONG Count = 0;

GPROTO SAVEDS ASM(ULONG) AddEntryProgFunc(
	REG_A0(struct Hook *Hk),
	REG_A1(struct xadProgressInfo *xpi) ) {

	ULONG Result = 0;
	BOOL Abort = FALSE;

	/* If Hk->h_Data == FALSE then we must not do any progress window operations. */

	switch (xpi->xpi_Mode) {
	case XADPMODE_NEWENTRY:
		DB(dbg("XADProgressFunc got XADPMODE_NEWENTRY"));

		if (Hk->h_Data) { /* Note: Hk->h_Data is a BOOL */

			if (!xget(GETPROGRESSWIN, MUIA_Window_Open)) {
				// OK, progress window is not opened. We must open it...
				OpenProgress(100, "XAD is reading the archive's entries...");
				CloseIt = TRUE;
			}

			Abort = xget(GETPROGRESSWIN, MUIA_ProgressWin_AbortStatus);

			// Only update progress every 10 entries, for speed.

			if (Count++ % 10 == 0) {
				ULONG tmp[2];
				tmp[0] = Count;
				tmp[1] = (ULONG) xpi->xpi_FileInfo->xfi_FileName;
				ChangeProgressStatus("XAD has read %lu entries so far - %s...", &tmp);
			}
		}
		break;
	case XADPMODE_END:
		DB(dbg("XADProgressFunc got XADPMODE_END"))

		if (CloseIt) {
			// Close the progress. Note that if the TAGID_MAIN_LEAVEPROGRESSOPEN
			// setting is on, then we override it, this flag is only applicable
			// to extract operations
			CloseProgressOverride;
			CloseIt	= FALSE;
		}

		Count =	0;
		break;
	}

	if (!(CheckSignal(SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) && !Abort) {
		Result |= XADPIF_OK;
	}

	return Result;
}

struct Hook AddEntryProgHook = {
	{ NULL, NULL }, (void *) AddEntryProgFunc, NULL, NULL
};

typedef unsigned long uint32;

GPROTO PN *ARC_CreateAHN_AddArchivePN( AHN *ahn, UBYTE *arcpath, struct xadSplitFile *xad_sf ) {
	BOOL success = FALSE;
	LONG xaderr;
	PN *pn;

	DB(dbg("ARC_CreateAHN_AddArchivePN(ahn=0x%08lx, arcpath=\"%s\", xad_sf=0x%08lx)",
	       ahn, arcpath, xad_sf))

	if ((pn = ARC_AllocPN(ahn))) {
		pn->pn_type = PNTYPE_ARCHIVE;
		AddEntryProgHook.h_Data = (void *) TRUE;

		xaderr = xadGetInfo(pn->pn_ai,

		    /* Progress window for AddEntry will only be shown if the user has XAD v11. Since
		       XADPMODE_END is only issued to the GetInfo progress hook from v11+. That is
			   where the progress window is closed. v10 user will just have upgrade if they
			   want AddEntry progress reports.  >;-) */

			   (xadMasterBase->xmb_LibNode.lib_Version >= 11 ?
			   XAD_PROGRESSHOOK : TAG_IGNORE), (ULONG) &AddEntryProgHook,
			   (arcpath ? XAD_INFILENAME : TAG_IGNORE), (ULONG) arcpath,
			   (xad_sf ? XAD_INSPLITTED  : TAG_IGNORE), (ULONG) xad_sf,
			   XAD_NOEXTERN, CFG_Get(TAGID_XAD_NOEXTXADCLIENTS),
			   TAG_DONE);

		// TODO: This won't be needed under XAD v11, though not all client support the Addtag.

		if (CloseIt) {
			CloseProgressOverride;
			CloseIt = FALSE;
		}

		if (xaderr == XADERR_OK) {

			if (pn->pn_ai->xai_FileInfo) {
				ARC_AddPNToAHN(pn, ahn);
				success = TRUE;

				if (!ahn->ahn_DiskAI) {
					ahn->ahn_DiskAI = pn->pn_ai;  // TODO: remove
				}

				DB(dbg("ARC_CreateAHN_AddArchivePN: New archive portion node added to AHN\n"))

			} else if (pn->pn_ai->xai_DiskInfo) {

				CollectTextInfos(ahn, pn->pn_ai);

			}
		}
	}

	if (!success) {
		ARC_FreePN(ahn, pn);
	}

	DB(if (!success) dbg("ARC_CreateAHN_AddArchivePN: No archive portions were added\n"))

	return success ? pn : NULL;
}

GPROTO PN *ARC_CreateAHN_AddDiskPN( AHN *ahn, UBYTE *arcpath, struct xadSplitFile *xad_sf ) {
	struct xadClient *start_client = NULL;
	PN *pn, *first_pn = NULL;
	LONG xaderr;

	DB(dbg("ARC_CreateAHN_AddDiskPN(ahn=0x%08lx, arcpath=\"%s\",  xad_sf=0x%08lx)",
	       ahn, arcpath, xad_sf))

	for	(;;) {
		if ((pn = ARC_AllocPN(ahn))) {
			struct TagItem tags[5];

			pn->pn_type = PNTYPE_DISK;
			tags[0].ti_Tag  =  arcpath ? XAD_INFILENAME : TAG_IGNORE;
			tags[0].ti_Data = (ULONG) arcpath;
			tags[1].ti_Tag  = xad_sf ? XAD_INSPLITTED : TAG_IGNORE;
			tags[1].ti_Data = (ULONG) xad_sf;
			tags[2].ti_Tag  = XAD_NOEXTERN;
			tags[2].ti_Data = CFG_Get(TAGID_XAD_NOEXTXADCLIENTS);
			tags[3].ti_Tag  = (ahn->ahn_ArcPassword ? XAD_PASSWORD : TAG_IGNORE);
			tags[3].ti_Data = (ULONG) ahn->ahn_ArcPassword;
			tags[4].ti_Tag  = TAG_DONE;
			tags[4].ti_Data = 0;

			xaderr = xadGetDiskInfo(pn->pn_ai,
			                        XAD_INDISKARCHIVE, (ULONG) &tags,
			                        (start_client ? XAD_STARTCLIENT : TAG_IGNORE), (ULONG) start_client,
			                        XAD_NOEXTERN, CFG_Get(TAGID_XAD_NOEXTXADCLIENTS),
			                        XAD_NOEMPTYERROR,TRUE,
			                        TAG_DONE);

			if (xaderr == XADERR_OK) {
				if (!first_pn) {
					first_pn = pn;
				}

				start_client = pn->pn_ai->xai_Client->xc_Next;

				if (!ahn->ahn_DiskAI) {
					ahn->ahn_DiskAI = pn->pn_ai;
				}

				ahn->ahn_IsDiskArc = TRUE;
				ARC_AddPNToAHN(pn, ahn);

				DB(dbg("ARC_CreateAHN_AddDiskPN: New disk portion node added to AHN\n"))

			} else if (xaderr == XADERR_DATAFORMAT) {

				ARC_FreePN(ahn, pn);
				GUI_PrgError("Non-DOS disk encountered!", NULL);
				break;

			} else {

				ARC_FreePN(ahn, pn);
				break;

			}
		}
	}
	DB(if (!first_pn) dbg("ARC_CreateAHN_AddDiskPN: No disk portions were added\n"))
	return first_pn;
}

GPROTO PN *ARC_CreateAHN_AddADFPN( AHN *ahn, UBYTE *arcpath, struct xadSplitFile *xad_sf ) {

	struct xadClient *start_client = NULL;
	PN *pn, *first_pn = NULL;
	LONG xaderr;

	DB(dbg("ARC_CreateAHN_AddADFPN(ahn=0x%08lx, arcpath=\"%s\", xad_sf=0x%08lx)",
	       ahn, arcpath, xad_sf))

	for (;;) {
		if ((pn = ARC_AllocPN(ahn))) {
			pn->pn_type = PNTYPE_DISK;

			xaderr = xadGetDiskInfo(pn->pn_ai,
			                        XAD_INFILENAME,  (ULONG) arcpath,
			                        (xad_sf       ? XAD_INSPLITTED : TAG_IGNORE), (ULONG) xad_sf,
			                        (start_client ? XAD_STARTCLIENT : TAG_IGNORE), (ULONG) start_client,
			                        XAD_NOEXTERN, CFG_Get(TAGID_XAD_NOEXTXADCLIENTS),
			                        XAD_NOEMPTYERROR, TRUE,
			                        TAG_DONE);

			if (xaderr == XADERR_OK) {

				if (!first_pn) {
					first_pn = pn;
				}

				start_client = pn->pn_ai->xai_Client->xc_Next;

				if (!ahn->ahn_DiskAI) {
					ahn->ahn_DiskAI = pn->pn_ai;
				}

				ahn->ahn_IsDiskArc = TRUE;
				ARC_AddPNToAHN(pn, ahn);
				DB(dbg("ARC_CreateAHN_AddADFPN: Added a new ADF portion node to AHN\n"))

			} else if (xaderr == XADERR_DATAFORMAT) {

				ARC_FreePN(ahn, pn);
				GUI_PrgError("NDOS disk encountered!", NULL);
				break;

			} else {

				ARC_FreePN(ahn, pn);
				break;
			}
		}
	}

	DB(if (!first_pn) dbg("ARC_CreateAHN_AddADFPN: No ADF portions were added"))

	return first_pn;
}

LPROTO void ARC_CreateAHN_PrepEmbedded( AHN *ahn, AHN *parent_ahn, AE *embedded_ae ) {

	// This routine prepares an AHN structure, which represents an embedded archive.
	// Such archives have their ahn_EmbeddedArc, ahn_EmbeddedPath and ahn_ParentArcPath
	// fields set.

	UBYTE *first_part, *last_part;

	ahn->ahn_EmbeddedPath  = GETAHNMEM(ahn->ahn_EmbeddedPathSize = 256);
	ahn->ahn_ParentArcPath = GETAHNMEM(ahn->ahn_ParentArcPathSize = 256);
	ahn->ahn_MemUsage     += (ahn->ahn_EmbeddedPathSize + ahn->ahn_ParentArcPathSize);

	DB(dbg("ARC_CreateAHN_PrepEmbedded(...)"))

	if (ahn->ahn_EmbeddedPath && ahn->ahn_ParentArcPath) {

		strncpy(ahn->ahn_EmbeddedPath,
		        embedded_ae->ae_xfi->xfi_FileName, ahn->ahn_EmbeddedPathSize - 1);
		strncpy(ahn->ahn_ParentArcPath, parent_ahn->ahn_Path,
		        ahn->ahn_ParentArcPathSize - 1);

		first_part = (parent_ahn->ahn_EmbeddedArc ?
		              parent_ahn->ahn_EmbeddedListStr : parent_ahn->ahn_Path);
		last_part  = ahn->ahn_EmbeddedPath;

		if ((strlen(first_part) + strlen(last_part)) < (LSLIMIT - 10)) { /* Limiter */

			sprintf(ahn->ahn_EmbeddedListStr, "%s :--> %s", first_part, last_part);

		} else {

			strncpy(ahn->ahn_EmbeddedListStr, first_part, LSLIMIT - 1);
		}

		ahn->ahn_EmbeddedArc = TRUE;

	} else {

		FREEAHNMEM(ahn->ahn_EmbeddedPath, ahn->ahn_EmbeddedPathSize);
		FREEAHNMEM(ahn->ahn_ParentArcPath, ahn->ahn_ParentArcPathSize);
		ahn->ahn_EmbeddedPath  = NULL;
		ahn->ahn_ParentArcPath = NULL;
	}
}

GPROTO PN *ARC_AllocPN( AHN *ahn ) {
	BOOL success = FALSE;
	PN *pn;

	if ((pn = GETAHNMEM(PN_SIZE))) {
		ahn->ahn_MemUsage += PN_SIZE;
		pn->pn_ready       = FALSE;

		if ((pn->pn_ai = xadAllocObjectA(XADOBJ_ARCHIVEINFO, NULL))) {
			success = TRUE;
		}
	}

	if (!success) {
		ARC_FreePN(ahn, pn);
		pn = NULL;
	}

	return pn;
}

GPROTO void ARC_FreePN( AHN *ahn, PN *pn ) {

	if (!pn) {
		return;
	}

	FREEPNRESOURCES(pn)
	ARC_UnprepPN(ahn, pn);
	FREEAHNMEM(pn, PN_SIZE);
}

GPROTO BOOL ARC_PrepPN( AHN *ahn, PN *pn ) {
	ULONG arraysize;
	BOOL success = FALSE;

	ARC_UnprepPN(ahn, pn);
	GUI_PrintStatus("Please wait, building directory tree...", 0);

	if ((pn->pn_currentae = pn->pn_rootae = AETree_Create(ahn, pn->pn_ai, FilePart(ahn->ahn_Path)))) {

		pn->pn_linear_cnt       = AETree_CountFileAEs(ahn, pn->pn_rootae);
		arraysize               = (pn->pn_linear_cnt + 1) * sizeof(AE *);
		pn->pn_linear_aes_xad   = GETAHNMEM(pn->pn_linear_aes_xadSize   = arraysize);
		pn->pn_linear_aes_alpha = GETAHNMEM(pn->pn_linear_aes_alphaSize = arraysize);
		ahn->ahn_MemUsage      += (arraysize + arraysize);
		pn->pn_expsize          = XADGetExpLength(pn->pn_ai);
		pn->pn_filecnt          = XADGetFileCount(pn->pn_ai);

		if (pn->pn_linear_aes_xad && pn->pn_linear_aes_alpha) {
			*AETree_GetFileAEs(ahn, pn->pn_rootae, pn->pn_linear_aes_xad, NULL) = NULL;
			CopyMemQuick(pn->pn_linear_aes_xad, pn->pn_linear_aes_alpha, arraysize);
			pn->pn_linear_aes_alpha_ready = FALSE;
			pn->pn_ready                  = TRUE;
			success                       = TRUE;
		}
	}

	GUI_PrintStatus("", 0);

	if (!success) {
		ARC_UnprepPN(ahn, pn);
	}

	return success;
}

GPROTO void ARC_UnprepPN( AHN *ahn, PN *pn ) {

	if (!pn) {
		return;
	}

	if (pn->pn_rootae) {
		AETree_Delete(ahn, pn->pn_rootae);
		ARC_DeleteAE(ahn, pn->pn_rootae);
	}

	FREEAHNMEM(pn->pn_linear_aes_xad,   pn->pn_linear_aes_xadSize);
	FREEAHNMEM(pn->pn_linear_aes_alpha, pn->pn_linear_aes_alphaSize);

	pn->pn_expsize                = 0;
	pn->pn_filecnt                = 0;
	pn->pn_rootae                 = NULL;
	pn->pn_linear_aes_xad         = NULL;
	pn->pn_linear_aes_alpha       = NULL;
	pn->pn_linear_aes_alpha_ready = FALSE;
	pn->pn_linear_cnt             = 0;
	pn->pn_ready                  = FALSE;
}

GPROTO void	ARC_AddPNToAHN( PN *pn, AHN *ahn ) {

	if (!pn || !ahn) {
		return;
	}

	AddTail((struct List *)&ahn->ahn_PNList, (struct Node *) &pn->pn_node);
	ahn->ahn_PNListCnt += 1;
}

/********************************************************************************************
 *
 * AE routines
 *
 * Routines for controlling ArcEntries.
 *
 ********************************************************************************************
 *
 */

GPROTO AE *ARC_CreateAE( AHN *ahn, AE *parent_ae, UBYTE *name,
                         ULONG xadpathpnt, struct xadFileInfo *xfi, UBYTE *fullpath, LONG xadpos ) {

	/* Create an AE node.
	 *
	 * Parameters
	 * ----------
	 *
	 * ParentAE   - Provide the parent AE that the new ArcEntry will be a
	 *              child of. 
	 * Name       - Name of the AE.
	 * XADPathPnt - The position of the name within the corresponding
	 *              xadFileInfo's path string. 
	 * xadFI      - xadFileInfo structure.
	 * FullPath   - The full path of this entry.
	 * XADPos     - The original XAD entry position. 
	 *
	 */

	register AE *ae;

	if (!fullpath) {
		fullpath = "";
	}

	if (!(ae = GETAHNMEM(AE_SIZE)))	{
		return NULL;
	}

	ahn->ahn_MemUsage += AE_SIZE;
	NewList((struct List *)&ae->ae_ChildAEL);

	ae->ae_DebugID			 = DEBUGID_AE;
	ae->ae_ahn				 = ahn;
	ae->ae_xfi				 = xfi;
	ae->ae_ParentAE			 = parent_ae;
	ae->ae_XADPos			 = xadpos;
	ae->ae_ListerPos		 = 0xffffffff;
	ae->ae_XADPathPoint		 = xadpathpnt;

	/* Don't bother	settings zeros on a clear buffer
	ae->ae_Size				 = 0;
	ae->ae_NodeOpened		 = FALSE;
	ae->ae_DateValid		 = FALSE;
	ae->ae_SizeValid		 = FALSE;
	ae->ae_ChildAEListSorted = FALSE;
	ae->ae_ChildAETreeSorted = FALSE;
	ae->ae_DisplayBuf[0]	 = 0;
	*/

	if (xfi) {

		if (!name) {
			name = "(unnamed)";
		}

		ae->ae_Name     = ARC_AllocStr(ahn, name, &ae->ae_NameSize);
		ae->ae_FullPath = ARC_AllocStr(ahn, fullpath, &ae->ae_FullPathSize);

		if (ae->ae_Name && ae->ae_FullPath) {

			UBYTE *lnkstr;

			if (xfi->xfi_Flags & XADFIF_LINK) {
				lnkstr = "- Link";
			} else {
				lnkstr = "";
			}

			// This is a hacky work-around for directory entries that don't
			// have their XADFIB_DIRECTORY bit set.

			if (!xfi->xfi_Size && xfi->xfi_Next) {

				LONG len = strlen(xfi->xfi_FileName);

				if (xfi->xfi_Next->xfi_FileName[len] == '/' &&
				        Strnicmp(xfi->xfi_Next->xfi_FileName, xfi->xfi_FileName, len) == 0)	{

					ae->ae_IsDir     = TRUE;
					ae->ae_Size      = -1;
					ae->ae_ListerPos = 0;
					sprintf((UBYTE *)&ae->ae_SizeBuf,  "\33I[6:22] %s", lnkstr);
					ae->ae_SizeValid = TRUE;
				}
			}

			/* Success */

		} else {

			ARC_DeleteAE(ahn, ae);
			ae = NULL;

		}
	} else {

		/* If xadFI == NULL, then we're creating a directory type AE (DirAE) on the fly.
		   This is normal for archives, because they don't usually carry directory entries
		   like ADFs do, but instead just files (with paths). Because we don't have an
		   xadFileInfo structure to examine, DirAEs created on the fly have no file
		   attributes (like comments, dates, etc.) associated with them. */

		if (!parent_ae && !name) {
			name = "Root";
		}

		ae->ae_Name			= ARC_AllocStr(ahn, name, &ae->ae_NameSize);
		ae->ae_IsDir		= TRUE;
		ae->ae_Size			= -1;
		strcpy((UBYTE *)&ae->ae_SizeBuf, "\33I[6:22]");
		ae->ae_FullPath		= ARC_AllocStr(ahn, fullpath, &ae->ae_FullPathSize);

		/* Don't bother	settings zeros on a clear buffer

		ae->ae_ListerPos	= 0;
		ae->ae_DS.ds_Days	= 0;
		ae->ae_DS.ds_Minute	= 0;
		ae->ae_DS.ds_Tick	= 0;
		ae->ae_DayPart[0]	= 0;
		ae->ae_DatePart[0]	= 0;
		ae->ae_TimePart[0]	= 0;
		*/

		if (!ae->ae_FullPath || !ae->ae_Name) {
			ARC_DeleteAE(ahn, ae);
			ae = NULL;
		}
	}

	return ae;
}

ASM(void) DatePutFunc( REG_A0(struct Hook *hk), REG_A1(UBYTE ch), REG_A2(struct Locale *locale) ) {

	UBYTE *p = hk->h_Data;
	*p++ = ch;
	hk->h_Data = p;

}

struct Hook DatePutHook = { { NULL,NULL }, (void *) DatePutFunc, NULL, NULL };

GPROTO void ARC_ResolveAEDate( AE *ae ) {

	BOOL success = FALSE;
	LONG xaderr;

	struct DateTime my_dt = {
		{ 0, 0, 0 }, FORMAT_DOS, 0, NULL, NULL, NULL
	};
	
	if (ae->ae_DateValid || !ae->ae_xfi) {
		return;
	}
	
	xaderr = xadConvertDates(
	             XAD_DATEXADDATE,   (ULONG) &ae->ae_xfi->xfi_Date,
	             XAD_GETDATEDATESTAMP, (ULONG) &ae->ae_DS,
	             TAG_DONE);
	
	if (xaderr == XADERR_OK) {
		
		DatePutHook.h_Data = ae->ae_DayPart;
		FormatDate(Locale, "%a", &ae->ae_DS, &DatePutHook);
		
		my_dt.dat_StrDate    = ae->ae_DatePart;
		my_dt.dat_StrTime    = ae->ae_TimePart;
		my_dt.dat_StrDay    = NULL; // TODO: Legal?
		my_dt.dat_Stamp.ds_Days   = ae->ae_DS.ds_Days;
		my_dt.dat_Stamp.ds_Minute = ae->ae_DS.ds_Minute;
		my_dt.dat_Stamp.ds_Tick   = ae->ae_DS.ds_Tick;
		
		if (DateToStr((struct DateTime *) &my_dt)) {
			success = TRUE;
		}
	}

	if (!success) {
		strcpy(ae->ae_DayPart, "(unknown)");
		strcpy(ae->ae_DatePart, "(unknown)");
		strcpy(ae->ae_TimePart, "(unknown)");
	}

	ae->ae_DateValid = TRUE;
}

GPROTO void ARC_ResolveAESize( AE *ae ) {

	UBYTE *lnkstr;
	struct xadFileInfo *xfi;

	xfi = ae->ae_xfi;

	if (ae->ae_SizeValid || !xfi) {
		return;
	}

	if (xfi->xfi_Flags & XADFIF_NOUNCRUNCHSIZE) {

		ae->ae_Size = 0;
		strcpy(ae->ae_SizeBuf, "unknown");

	} else {

		if (xfi->xfi_Flags & XADFIF_LINK) {
			lnkstr = "- Link";
		} else {
			lnkstr = "";
		}

		if (xfi->xfi_Flags & XADFIF_DIRECTORY) {

			ae->ae_IsDir  = TRUE;
			ae->ae_Size   = -1;
			ae->ae_ListerPos = 0;
			sprintf(ae->ae_SizeBuf,  "\33I[6:22] %s", lnkstr);

		} else {

			ae->ae_Size = xfi->xfi_Size;
			sprintf(ae->ae_SizeBuf, "%lu %s", xfi->xfi_Size, lnkstr);
		}
	}

	ae->ae_SizeValid = TRUE;
}

GPROTO void ARC_ResolveAEProt( AE *ae ) {
	UBYTE *s;
	register ULONG Prot;

	if (GLOBALHACK_NewAmigaOSBits) {

		// Bits mode has changed since last time. Force it
		// to be reconstructed.
		ae->ae_ProtValid = FALSE;
	}

	if (ae->ae_ProtValid || !ae->ae_xfi) {
		return;
	}

	/* This routine is based on Dirk's ShowProt() in xadUnFile.c */

	s = ae->ae_ProtBuf;
	Prot = ae->ae_xfi->xfi_Protection;
	ae->ae_ProtValid = TRUE;

	if (!GLOBALHACK_AmigaOSBits) { // Don't show these bits when in AmigaOSBits mode
		if (Prot & FIBF_OTR_READ)    *s++ = 'r'; else *s++ = '-';
		if (Prot & FIBF_OTR_WRITE)   *s++ = 'w'; else *s++ = '-';
		if (Prot & FIBF_OTR_EXECUTE) *s++ = 'e'; else *s++ = '-';
		if (Prot & FIBF_OTR_DELETE)  *s++ = 'd'; else *s++ = '-';
		if (Prot & FIBF_GRP_READ)    *s++ = 'r'; else *s++ = '-';
		if (Prot & FIBF_GRP_WRITE)   *s++ = 'w'; else *s++ = '-';
		if (Prot & FIBF_GRP_EXECUTE) *s++ = 'e'; else *s++ = '-';
		if (Prot & FIBF_GRP_DELETE)  *s++ = 'd'; else *s++ = '-';
	}

	if (Prot & (1<<7))          *s++ = 'h'; else *s++ = '-';
	if (Prot & FIBF_SCRIPT)     *s++ = 's'; else *s++ = '-';
	if (Prot & FIBF_PURE)       *s++ = 'p'; else *s++ = '-';
	if (Prot & FIBF_ARCHIVE)    *s++ = 'a'; else *s++ = '-';
	if (!(Prot & FIBF_READ))    *s++ = 'r'; else *s++ = '-';
	if (!(Prot & FIBF_WRITE))   *s++ = 'w'; else *s++ = '-';
	if (!(Prot & FIBF_EXECUTE)) *s++ = 'e'; else *s++ = '-';
	if (!(Prot & FIBF_DELETE))  *s++ = 'd'; else *s++ = '-';

	*s = 0;
}

GPROTO void ARC_DeleteAE( AHN *ahn, AE *ae ) {

	//if (!ae) return;
	//if (ae->ae_Name)     MEM_FreeVec(ae->ae_Name);
	//if (ae->ae_FullPath) MEM_FreeVec(ae->ae_FullPath);

	FREEAHNMEM(ae, AE_SIZE);
}

GPROTO BOOL ARC_SortAEArrayToXADOrder( AHN *ahn, AE **ae_array, ULONG amt ) {

	/* Sort an array of ArcEntries into XAD original order. Doing this
	 * makes file extraction much faster.
	 */

	BOOL incomplete_list = FALSE, posfound;
	register AE **aea = ae_array;
	register struct Node *n, *newn;
	struct List tmplist;

	DB(dbg("ARC_SortAEArrayToXADOrder(ae_array=0x%08lx, amt=%lu)", ae_array, amt))

	GUI_PrintStatus("Please wait, sorting entries for extraction...", 0);
	NewList(&tmplist);

	while (*aea && !incomplete_list) {

		LONG nextxadpos = (*aea)->ae_XADPos;

		if ((newn = GETAHNMEM(sizeof(struct Node)))) {

			ahn->ahn_MemUsage += sizeof(struct Node);
			newn->ln_Name = (UBYTE *) *aea;

			for (n = tmplist.lh_Head, posfound = FALSE; n->ln_Succ; n = n->ln_Succ) {

				if (nextxadpos <= ((AE *)n->ln_Name)->ae_XADPos) {
					Insert(&tmplist, newn, n->ln_Pred);
					posfound = TRUE;
					break;
				}
			}

			if (!posfound) {
				AddTail(&tmplist, newn);
			}

		} else {

			incomplete_list = TRUE;
		}
		aea++;
	}

	/* Write sorted entries back into the array... */

	if (!incomplete_list) {
		for (aea = ae_array, n = tmplist.lh_Head; n->ln_Succ; n = n->ln_Succ) {
			*aea++ = (AE *) n->ln_Name;
		}
	}

	while ((n = RemHead(&tmplist))) {
		FREEAHNMEM(n, sizeof(struct Node));
	}

	return (BOOL) !incomplete_list;
}

GPROTO AE *ARC_FindAEInAHN_ViaName( AHN *ahn, UBYTE *name ) {

	register AE **ae_ptr, *ae, *tmp_ae;
	AE *rootae, **aearray_vec;
	ULONG amt, aearray_vecSize;

	if (!ahn || !name) {
		return NULL;
	}

	rootae = ahn->ahn_CurrentPN->pn_rootae;
	amt = AETree_CountAllAEs(ahn, rootae, NULL, NULL) + 1;

	if (!(aearray_vec = GETAHNMEM(aearray_vecSize = amt * AE_SIZE))) {
		return NULL;
	}

	ahn->ahn_MemUsage += aearray_vecSize;
	*AETree_GetAllAEs(ahn, rootae, aearray_vec, NULL) = NULL;
	ae_ptr = aearray_vec;

	while ((ae = (*ae_ptr))) {

		if (ae->ae_IsDir) {

			UBYTE pathbuf[256], tmpbuf[256];
			pathbuf[0] = 0;
			tmpbuf[0]  = 0;
			tmp_ae     = ae;

			while (tmp_ae->ae_ParentAE) {

				strncpy(tmpbuf, tmp_ae->ae_Name, sizeof(tmpbuf)-1);
				strncat(tmpbuf, "/", sizeof(tmpbuf)-1);
				strncat(tmpbuf, pathbuf, sizeof(tmpbuf)-1);
				strncpy(pathbuf, tmpbuf, sizeof(pathbuf)-1);
				tmp_ae = tmp_ae->ae_ParentAE;
			}

			if (pathbuf[strlen(pathbuf) - 1] == '/') {
				pathbuf[strlen(pathbuf) - 1] = 0;
			}

			if (Stricmp(pathbuf, name) == 0) {
				return ae;
			}

		} else if (Stricmp(ae->ae_xfi->xfi_FileName, name) == 0) {

			return ae;
		}

		ae_ptr++;
	}

	FREEAHNMEM(aearray_vec, aearray_vecSize);
	return NULL;
}

/********************************************************************************************
 *
 * AETree routines
 *
 * Routines for controlling ArcEntries Trees.
 *
 ********************************************************************************************
 *
 */

GPROTO AE *AETree_Create( AHN *ahn, struct xadArchiveInfo *xai, STRPTR rootname ) {

	struct xadFileInfo *xfi;
	AE *rootae, *ae, *dirae;
	LONG xadpos = 0;
	UBYTE *pstrvec;
	BOOL pass_err = FALSE;
	register UBYTE *pathBeg, *pathPtr, *pathEnd;

	if (!xai || !xai->xai_FileInfo) {
		return NULL;
	}

	if (!(rootae = ARC_CreateAE(ahn, NULL, rootname, 0, NULL, NULL, -1))) {
		return NULL;
	}

	/* NOTES: The first pass can be omitted totally, but this means some entries with their
	 * XADFIF_DIRECTORY set will be missed, depending on the client in use. If this happens
	 * then we lose the attributes associated with that entry, like comments, date, etc.
	 */

	xfi = xai->xai_FileInfo;

	do {
		if (!(xfi->xfi_Flags & XADFIF_DIRECTORY)) {

			dirae = rootae;
			pathBeg = pathPtr = pstrvec = xfi->xfi_FileName;
			pathEnd = FilePart(pstrvec);

			if (*pathPtr == '/') {
				pathPtr++;
				pathBeg++;
			}

			while (pathPtr < pathEnd && !pass_err) {

				while(*pathPtr && *pathPtr != '/') {
					pathPtr++;
				}

				*pathPtr = 0;

				if ((ae = AEDir_FindViaName(ahn, dirae, pathBeg))) {

					dirae = ae;

				} else if ((ae = ARC_CreateAE(ahn, dirae, pathBeg, pathBeg - pstrvec, NULL, pstrvec, -1))) {

					AddTail((struct List *)&dirae->ae_ChildAEL, (struct Node *) &ae->ae_Node);
					dirae = ae;

				} else {

					pass_err = TRUE;
				}

				*pathPtr++ = '/';
				pathBeg = pathPtr;
			}

			if (!pass_err && xfi) { /* Add the xadFileInfo to the tree */

				pathPtr++;
				if ((ae = ARC_CreateAE(ahn, dirae, pathBeg, pathBeg - pstrvec, xfi, pstrvec, xadpos))) {

					AddTail((struct List *)&dirae->ae_ChildAEL, (struct Node *) &ae->ae_Node);

				} else {

					pass_err = TRUE;
				}
			}
		}

		xadpos++;

	} while ((xfi = xfi->xfi_Next) && !pass_err);

	return rootae;
}

GPROTO void AETree_Delete( AHN *ahn, AE *aetree ) {
	/* Recursive */

	register AE *nextae, *tmpae;

	if (!aetree || !aetree->ae_IsDir) {
		return;
	}

	for (nextae = (AE *) aetree->ae_ChildAEL.mlh_Head;
         nextae->ae_Node.mln_Succ;) {

		tmpae = (AE *) nextae->ae_Node.mln_Succ;

		if (aetree->ae_IsDir) {
			AETree_Delete(ahn, nextae);
		}

		ARC_DeleteAE(ahn, nextae);
		nextae = tmpae;
	}
}

GPROTO void AETree_Sort( AHN *ahn, AE *dirae, LONG sortmode ) {

	/* Recursive */

	register AE *nextae;

	if (!dirae || !dirae->ae_IsDir) {
		return;
	}

	for (nextae = (AE *) dirae->ae_ChildAEL.mlh_Head;
	     nextae->ae_Node.mln_Succ;
	     nextae = (AE *) nextae->ae_Node.mln_Succ) {

		if (nextae->ae_IsDir) {

			AETree_Sort(ahn, nextae, sortmode);
			AEDir_Sort(nextae, sortmode);
			nextae->ae_ChildAEListSorted = TRUE;

		}
	}

	AEDir_Sort(dirae, sortmode);
}

GPROTO ULONG AETree_CountAllAEs( AHN *ahn, AE *dirae, ULONG *amtfiles_ptr, ULONG *amtdirs_ptr ) {

	/* Recursive */

	register AE *nextae;
	register ULONG cnt = 0;

	if (!dirae || !dirae->ae_IsDir) {
		return 0;
	}

	for (nextae = (AE *) dirae->ae_ChildAEL.mlh_Head;
	     nextae->ae_Node.mln_Succ;
	     nextae = (AE *) nextae->ae_Node.mln_Succ) {

		cnt++;

		if (nextae->ae_IsDir) {

			if (amtdirs_ptr) {
				*amtdirs_ptr += 1;
			}

			cnt += AETree_CountAllAEs(ahn, nextae, amtfiles_ptr, amtdirs_ptr);

		} else if (amtfiles_ptr) {

			*amtfiles_ptr += 1;
		}
	}
	return cnt;
}

GPROTO ULONG AETree_CountFileAEs( AHN *ahn, AE *dirae ) {
	/* Recursive */

	register AE *nextae;
	register ULONG cnt = 0;

	if (!dirae || !dirae->ae_IsDir) {
		return 0;
	}

	for (nextae	= (AE *) dirae->ae_ChildAEL.mlh_Head;
	     nextae->ae_Node.mln_Succ;
	     nextae = (AE *) nextae->ae_Node.mln_Succ) {

		if (nextae->ae_IsDir) {
			cnt += AETree_CountFileAEs(ahn, nextae);
	
		} else {
			cnt++;
		}
	}

	return cnt;
}

GPROTO AE **AETree_GetFileAEs( AHN *ahn, AE *dirae, AE **aea, ULONG *cnt_ptr ) {

	/* Recursive - make sure CntPtr points to a clear ULONG on first call! */

	register AE *nextae;

	if (!dirae || !dirae->ae_IsDir || !aea) {
		return aea;
	}

	for (nextae = (AE *) dirae->ae_ChildAEL.mlh_Head;
	     nextae->ae_Node.mln_Succ;
	     nextae = (AE *) nextae->ae_Node.mln_Succ) {

		if (nextae->ae_IsDir) {
			aea = AETree_GetFileAEs(ahn, nextae, aea, cnt_ptr);
		} else {
			*aea++ = nextae;

			if (cnt_ptr) {
				(*cnt_ptr)++;
			}
		}
	}
	return aea;
}

GPROTO AE **AETree_GetAllAEs( AHN *ahn, AE *dirae, AE **aea, ULONG *cnt_ptr ) {
	/* Recursive - make sure CntPtr points to a clear ULONG! */

	register AE *nextae;

	if (!dirae || !dirae->ae_IsDir || !aea) {
		return aea;
	}

	for (nextae = (AE *) dirae->ae_ChildAEL.mlh_Head;
	     nextae->ae_Node.mln_Succ;
	     nextae = (AE *) nextae->ae_Node.mln_Succ) {
		
		*aea++ = nextae;
		
		if (cnt_ptr) {
			(*cnt_ptr)++;
		}
		
		if (nextae->ae_IsDir) {
			aea = AETree_GetAllAEs(ahn, nextae, aea, cnt_ptr);
		}
	}

	return aea;
}

/********************************************************************************************
 *
 * AEDir routines
 *
 * Routines for controlling ArcEntries Dirs.
 *
 ********************************************************************************************
 *
 */

LPROTO LONG ARC_AESortFunc( AE **n1, AE **n2 ) {

	if ((CFG_Get(TAGID_MAIN_ARCVIEWMODE) == MUIV_ArcView_SwapViewMode_Linear) &&
	        (*n1)->ae_xfi && (*n2)->ae_xfi) {

		return Stricmp((*n1)->ae_xfi->xfi_FileName, (*n2)->ae_xfi->xfi_FileName);

	} else {

		return Stricmp((*n1)->ae_Name, (*n2)->ae_Name);
	}

}

GPROTO void AEDir_Sort( AE *DirAE, LONG SortMode ) {

	AHN *ahn = DirAE->ae_ahn;
	ULONG dircnt  = AEDir_CountDirAEs(ahn, DirAE);
	ULONG filecnt = AEDir_CountFileAEs(ahn, DirAE);
	register AE **dirarray, **da;
	register AE **filearray, **fa;
	register AE *nextae;
	ULONG dirarray_size, filearray_size;

	if (!DirAE || !DirAE->ae_IsDir || !(dircnt + filecnt)) {
		return;
	}

	da = dirarray  = GETAHNMEM(dirarray_size  = sizeof(AE *) * (dircnt + 1));
	fa = filearray = GETAHNMEM(filearray_size = sizeof(AE *) * (filecnt + 1));
	ahn->ahn_MemUsage += (dirarray_size + filearray_size);

	if (dirarray && filearray) {
		struct MinList *destlist;

		for (nextae = (AE *) DirAE->ae_ChildAEL.mlh_Head;
		     nextae->ae_Node.mln_Succ;
		     nextae = (AE *) nextae->ae_Node.mln_Succ) {

			if (nextae->ae_IsDir) {
				*da++ = nextae;
			} else {
				*fa++ = nextae;
			}

		}

		*da = NULL;
		da = dirarray;
		*fa = NULL;
		fa = filearray;
		qsort(dirarray,  dircnt,  sizeof(APTR), (int (*)(const void *, const void *)) &ARC_AESortFunc);
		qsort(filearray, filecnt, sizeof(APTR), (int (*)(const void *, const void *)) &ARC_AESortFunc);
		destlist = &DirAE->ae_ChildAEL;

		switch (SortMode) {
		case SORTMODE_DIRSABOVE:
			NewList((struct List *)&DirAE->ae_ChildAEL);

			while (*da) {
				AddTail((struct List *)destlist, (struct Node *) *da++);
			}

			while (*fa) {
				AddTail((struct List *)destlist, (struct Node *) *fa++);
			}
			break;

		case SORTMODE_MIXED:
			NewList((struct List *)&DirAE->ae_ChildAEL);

			while (*da) {
				AEDir_AddAE_Sorted(ahn, *da++, DirAE);
			}

			while (*fa) {
				AEDir_AddAE_Sorted(ahn, *fa++, DirAE);
			}
			break;

		case SORTMODE_DIRSBELOW:
			NewList((struct List *)&DirAE->ae_ChildAEL);

			while (*fa) {
				AddTail((struct List *)destlist, (struct Node *) *fa++);
			}

			while (*da) {
				AddTail((struct List *)destlist, (struct Node *) *da++);
			}
			break;

		default: /* Don't touch the list */
			break;
		}
	}

	DirAE->ae_ChildAEListSorted = TRUE;
	FREEAHNMEM(dirarray,  dirarray_size);
	FREEAHNMEM(filearray, filearray_size);
}

GPROTO AE *AEDir_FindViaName( AHN *ahn, AE *dirAE, UBYTE *name ) {

	register AE *nextAE;

	if (!dirAE || !name) {
		return NULL;
	}

	for (nextAE = (AE *) dirAE->ae_ChildAEL.mlh_Head;
	     nextAE->ae_Node.mln_Succ;
	     nextAE = (AE *) nextAE->ae_Node.mln_Succ) {

		if (!Stricmp(name, nextAE->ae_Name)) {
			return nextAE;
		}
	}

	return NULL;
}

GPROTO void	AEDir_AddAE( AHN *ahn, AE *insAE, struct ArcEntry *dirAE ) {

	if (!insAE || !dirAE || !dirAE->ae_IsDir) {
		return;
	}

	if (!insAE->ae_Name) {
		return;
	}

	AddTail((struct List *)&dirAE->ae_ChildAEL, (struct Node *) &insAE->ae_Node);
}

GPROTO void	AEDir_AddAE_Sorted( AHN *ahn, AE *insAE, AE *dirAE ) {

	struct MinList *destlist;
	AE *nextAE;

	if (!insAE || !dirAE || !dirAE->ae_IsDir || !insAE->ae_Name) {
		return;
	}

	destlist = &dirAE->ae_ChildAEL;

	for (nextAE = (AE *) dirAE->ae_ChildAEL.mlh_Head;
	     nextAE->ae_Node.mln_Succ;
	     nextAE = (AE *) nextAE->ae_Node.mln_Succ) {

		if (Stricmp(nextAE->ae_Name, insAE->ae_Name) >= 0) {

			Insert((struct List *) destlist, (struct Node *) &insAE->ae_Node,
			       (struct Node *) nextAE->ae_Node.mln_Pred);

			return;
		}
	}

	AddTail((struct List *)destlist, (struct Node *) &insAE->ae_Node);
}

GPROTO ULONG AEDir_CountAllAEs( AHN *ahn, AE *dirAE ) {

	register ULONG cnt = 0;
	register AE *nextAE;

	if (!dirAE || !dirAE->ae_IsDir) {
		return 0;
	}

	for (nextAE = (AE *) dirAE->ae_ChildAEL.mlh_Head;
	     nextAE->ae_Node.mln_Succ;
	     nextAE = (AE *) nextAE->ae_Node.mln_Succ) {
		cnt++;
	}

	return cnt;
}

GPROTO ULONG AEDir_CountDirAEs( AHN *ahn, AE *dirAE ) {

	register ULONG cnt = 0;
	register AE *nextAE;

	if (!dirAE || !dirAE->ae_IsDir) {
		return 0;
	}

	for (nextAE = (AE *) dirAE->ae_ChildAEL.mlh_Head;
	     nextAE->ae_Node.mln_Succ;
	     nextAE = (AE *) nextAE->ae_Node.mln_Succ) {

		if (nextAE->ae_IsDir) {
			cnt++;
		}
	}

	return cnt;
}

GPROTO ULONG AEDir_CountFileAEs( AHN *ahn, AE *dirAE ) {

	register ULONG cnt = 0;
	register struct ArcEntry *nextAE;

	if (!dirAE || !dirAE->ae_IsDir) {
		return 0;
	}

	for (nextAE = (AE *) dirAE->ae_ChildAEL.mlh_Head;
	     nextAE->ae_Node.mln_Succ;
	     nextAE = (AE *) nextAE->ae_Node.mln_Succ) {

		if (!nextAE->ae_IsDir) {
			cnt++;
		}
	}

	return cnt;
}

/********************************************************************************************
 *
 * Extraction routines
 *
 ********************************************************************************************
 *
 */

GPROTO ULONG ARC_DoExtract( UBYTE *destpath, BOOL extractall, BOOL progesswnd )	{

	LONG viewmode = CFG_Get(TAGID_MAIN_ARCVIEWMODE), pathpoint, xaderr;
	ULONG extotal = 0, excnt = 0, exfilecnt = 0, exdircnt = 0, pcnt = 0;
	AHN *ahn;
	AE **aearray, **aea, *ae, *tmpae = NULL;
	BPTR cdlock, oldcd;
	BOOL abortLoop = FALSE;
	Object *av = GETARCVIEW;

	if (!(ahn = (AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN))) {
		return 0;
	}

	if (!IO_MyCreateDir(destpath)) {
		GUI_PrgDOSError(STR(SID_ERR_CREATEDESTDIR,
		                    "Couldn't create destination directory!"), NULL);
		return 0;
	}

	if (!(cdlock = Lock(destpath, SHARED_LOCK))) {
		return 0;
	}

	oldcd = CurrentDir(cdlock);

	if ((aea = aearray = ARC_GetSelected(ahn, &extotal, &exfilecnt, &exdircnt, extractall))) {
		GUI_PrintStatus(STR(SID_EXTFILESDIRS_FMT,
		                    "Please wait, extracting %lu files and dirs..."), extotal);

		if (progesswnd) {
			OpenProgress(exfilecnt, STR(SID_EXTFILESDIRS,
			                            "Please wait, extracting files and dirs..."));
		}

		DoMethod(av, MUIM_ArcView_SelectNone);
		ProgressHook_OverwriteAll = FALSE;

		while ((ae = *aea) && !abortLoop) {
			struct xadFileInfo *xfi;
			UBYTE *pstrvec;
			ULONG  pstrvec_size;

			if (!(xfi = ae->ae_xfi)) {
				++aea;
				continue;
			}

			if (progesswnd) {
				ChangeProgressStatus(xfi->xfi_FileName, NULL);
			}

			if (extractall || viewmode == MUIV_ArcView_SwapViewMode_Linear ||
			        CFG_Get(TAGID_MAIN_KEEPFULLPATH)) {

				pathpoint = 0;

			} else if (viewmode == MUIV_ArcView_SwapViewMode_Listtree ||
			           viewmode == MUIV_ArcView_SwapViewMode_NListtree ||
					   viewmode == MUIV_ArcView_SwapViewMode_Explorer) {

				pathpoint = ae->ae_XADPathPoint;

				if (DoMethod(av, MUIM_ArcView_GetSelCnt) == 1) {
					DoMethod(av, MUIM_ArcView_GetActiveAE, &tmpae);
					if (tmpae && tmpae->ae_IsDir) {
						pathpoint = tmpae->ae_XADPathPoint;
					}
				}

			} else if (viewmode == MUIV_ArcView_SwapViewMode_FilesAndDirs) {

				pathpoint = ((AE *)ahn->ahn_CurrentPN->
				             pn_currentae->ae_ChildAEL.mlh_Head)->ae_XADPathPoint;

			} else {
				pathpoint = 0;
			}

			/* Note: Deselection code was removed from this point. */

			if ((pstrvec = ARC_AllocStr(ahn, xfi->xfi_FileName, &pstrvec_size))) {
				UBYTE *chchop, ch;

				chchop = FilePart(pstrvec + pathpoint);
				ch = *chchop;
				*chchop = 0;
				IO_MyCreateDir(&pstrvec[pathpoint]);
				*chchop = ch;

				if (xfi->xfi_Flags & XADFIF_LINK) {

					GUI_PrgError(STR(SID_ERR_LINKSNOTSUPP,
						"Sorry, this version of VX doesn't extract links [%.100s]!"),
						&xfi->xfi_FileName
					);

				} else if ((xfi->xfi_Flags & XADFIF_DIRECTORY) || ae->ae_IsDir)	{

					IO_MyCreateDir(&pstrvec[pathpoint]);
					ARC_RestoreAttrs(&pstrvec[pathpoint], xfi);

				} else if (ARC_ExtractFile(ahn, ae, pstrvec + pathpoint, FALSE, &xaderr, destpath)) {

					++excnt;
					++pcnt;

					if (progesswnd) {
						abortLoop = UpdateProgress(xfi->xfi_FileName, NULL, pcnt);
					}

					if (CFG_Get(TAGID_MAIN_AUTOVIRUSCHECK)) {
						UBYTE *virusname;

						if ((virusname = VIRUS_CheckFile(&pstrvec[pathpoint]))) {

							VIRUS_ShowWarning(ahn, ae, virusname);
						}
					}

				} else if (xaderr == XADERR_BREAK) {

					abortLoop =	TRUE;
				}

				FREEAHNMEM(pstrvec, pstrvec_size);
			}

			DoMethod(App, MUIM_Application_InputBuffered);

			if (abortLoop) {
				break;
			}

			aea++;
		}

		if (progesswnd) {
			// Progress may or may not close at this point.
			// CloseProgress will check user's settings.
			CloseProgress;
		}

		ARC_FreeSelected(aearray);
	}

	CurrentDir(oldcd);
	UnLock(cdlock);
	DoMethod(av, MUIM_ArcView_SelectNone);
	GUI_PrintStatus(STR(SID_ERR_EXTFILES, "Extracted %ld files."), excnt);

	return 0;
}

GPROTO BOOL ARC_ExtractFile( AHN *ahn, AE *AE, UBYTE *DestFile,
		BOOL ProgressWnd, LONG *xadrc_ptr, UBYTE *destPath ) {

	// If DestPath == "" then DestFile contains FULL path.
	// Else we're extracting relative to the current directory.

	BOOL ExtractSuccess = FALSE;
	LONG xaderr = XADERR_OK;
	UBYTE *passwd = NULL;
	PN *pn;

	if (xadrc_ptr) {
		*xadrc_ptr = 0;
	}

	if (ProgressWnd) {
		ARC_ExtractFileProgressHook.h_Data = (APTR) TRUE;
	} else {
		ARC_ExtractFileProgressHook.h_Data = (APTR) FALSE;
	}

	DB(dbg("ARC_ExtractFile(ahn=0x%08lx, ae=0x%08lx, destfile=\"%s\", progresswnd=%ld, ...)",
	       ahn, AE, DestFile, ProgressWnd))

	if (!ahn || !AE || !AE->ae_xfi || !DestFile) {
		return FALSE;
	}

	ARC_ResolveAEDate(AE);
	ARC_ResolveAESize(AE);

	if (AE->ae_xfi->xfi_Flags & XADFIF_CRYPTED) {
		passwd = GUI_GetPassword(STR(SID_PASSWORDREQ, "%s requires a password"),
		                         &AE->ae_xfi->xfi_FileName);
		if (!passwd) {
			return FALSE;
		}
	}

	FilterString(DestFile);

	if ((AE->ae_xfi->xfi_Size < MIN_EXTRACT_PROGRESS_SIZE) &&
	        (!AE->ae_xfi->xfi_Flags & XADFIF_NOUNCRUNCHSIZE)) {
		/* Don't open the progress window on small files */
		ProgressWnd = FALSE;
	}

	if (ProgressWnd) {
		OpenProgress((AE->ae_xfi->xfi_Flags & XADFIF_NOUNCRUNCHSIZE)
		             ? 0xffffffff : AE->ae_xfi->xfi_Size, STR(SID_EXTANDVIEW,
		                     "Please wait, extracting and viewing..."));
	}

	pn = ahn->ahn_CurrentPN;

	{	// Update progress report

		UBYTE temp[256], *which;
		if (strlen(destPath) == 0) {

			which = DestFile;

		} else {

			strncpy(temp, destPath, sizeof(temp));
			AddPart(temp, AE->ae_xfi->xfi_FileName, sizeof(temp));
			temp[sizeof(temp) - 1] = 0;
			temp[sizeof(temp) - 2] = '.';
			temp[sizeof(temp) - 3] = '.';
			temp[sizeof(temp) - 4] = '.';
			which = temp;
		}

		DoMethod(GETPROGRESSWIN, MUIM_ProgressWin_PrintReport,
			AE->ae_xfi->xfi_FileName, which, AE->ae_xfi->xfi_Size);
	}

	if (pn->pn_type == PNTYPE_DISK) {

		xaderr = xadDiskFileUnArc(pn->pn_ai,
		                          XAD_ENTRYNUMBER,  AE->ae_xfi->xfi_EntryNumber,
		                          XAD_OUTFILENAME,  (ULONG) DestFile,
		                          XAD_OVERWRITE,    TRUE,
		                          XAD_NOEXTERN,     CFG_Get(TAGID_XAD_NOEXTXADCLIENTS),
		                          (passwd ? XAD_PASSWORD : TAG_IGNORE), (ULONG) passwd,
		                          XAD_PROGRESSHOOK, (ULONG) &ARC_ExtractFileProgressHook,
		                          TAG_DONE);

		if (xaderr == XADERR_OK) {

			ExtractSuccess = TRUE;
		} else {

			DoMethod(GETERRWIN, MUIM_ErrorWin_ReportXADError,
			         STR(SID_ERRXAD_DFU_WITH, "xadDiskFileUnArc() failed on	%s"),
			         &AE->ae_xfi->xfi_FileName, xaderr);
		}

	} else if (pn->pn_type == PNTYPE_ARCHIVE) {

		xaderr = xadFileUnArc(pn->pn_ai,
		                      XAD_ENTRYNUMBER,  AE->ae_xfi->xfi_EntryNumber,
		                      XAD_OUTFILENAME,  (ULONG) DestFile,
		                      XAD_OVERWRITE,    TRUE,
		                      XAD_NOEXTERN,     CFG_Get(TAGID_XAD_NOEXTXADCLIENTS),
		                      (passwd ? XAD_PASSWORD : TAG_IGNORE), (ULONG) passwd,
		                      XAD_PROGRESSHOOK, (ULONG) &ARC_ExtractFileProgressHook,
		                      TAG_DONE);

		if (xaderr == XADERR_OK) {

			ExtractSuccess = TRUE;

		} else {

			DoMethod(GETERRWIN, MUIM_ErrorWin_ReportXADError,
			         STR(SID_ERRXAD_FU_WITH, "xadFileUnArc() failed on %s"),
			         &AE->ae_xfi->xfi_FileName, xaderr);
		}
	}

	if (xadrc_ptr) {
		*xadrc_ptr = xaderr;
	}

	if (ExtractSuccess) {
		ARC_RestoreAttrs(DestFile, AE->ae_xfi);
	}

	if (ProgressWnd) {
		CloseProgress;
	}

	return ExtractSuccess;
}

GPROTO BOOL	ARC_ExtractFileToBuf( AHN *ahn, AE *AE, APTR *Buf_ptr,
                                  ULONG *BufLen_ptr, BOOL ProgressWnd, LONG *xadrc_ptr ) {
	/* Same as ARC_ExtractFile() but instead file is wrote to memory and
	 * not to disk. Don't forget to restore file attributes if you
	 * intend on writing the file to disk. The file buffer returned by
	 * this routine must be freed with a call to MEM_FreeVec() when
	 * you're finished with it.
	 */

	LONG xaderr         = XADERR_OK;
	BOOL ExtractSuccess = FALSE;
	UBYTE *passwd       = NULL;
	APTR FileVec;
	ULONG FileVecLen;
	PN *pn;

	DB(dbg("ARC_ExtractFileToBuf(ahn=0x%08lx, ae=0x%08lx, buf_ptr=0x%08lx, "
	       "buflen_ptr=0x%08lx, progresswnd=%ld)", ahn, AE, Buf_ptr, BufLen_ptr, ProgressWnd))

	if (xadrc_ptr) {
		*xadrc_ptr = 0;
	}

	if (ProgressWnd) {

		ARC_ExtractFileProgressHook.h_Data = (APTR) TRUE;

	} else {

		ARC_ExtractFileProgressHook.h_Data = (APTR) FALSE;
	}

	if (!ahn || !AE || !AE->ae_xfi || !Buf_ptr || !BufLen_ptr) {
		return FALSE;
	}

	ARC_ResolveAEDate(AE);
	ARC_ResolveAESize(AE);

	if (AE->ae_xfi->xfi_Flags & XADFIF_CRYPTED) {

		passwd = GUI_GetPassword(STR(SID_PASSWORDREQ, "%s requires a password"),
		                         &AE->ae_xfi->xfi_FileName);
		if (!passwd) {
			return FALSE;
		}
	}

	FileVecLen = AE->ae_xfi->xfi_Size;

	if ((FileVec = MEM_AllocVec(FileVecLen + 4))) {
#define MIN_EXTRACT_PROGRESS_SIZE 50000

		if (FileVecLen < MIN_EXTRACT_PROGRESS_SIZE) {
			ProgressWnd = FALSE;
		}

		if (ProgressWnd) {
			OpenProgress(FileVecLen, STR(SID_EXTANDVIEW, "Please wait, extracting and viewing..."));
		}

		pn = ahn->ahn_CurrentPN;

		if (pn->pn_type == PNTYPE_DISK) {
			xaderr = xadDiskFileUnArc(pn->pn_ai,
			                          XAD_ENTRYNUMBER,	AE->ae_xfi->xfi_EntryNumber,
			                          XAD_OUTMEMORY,	(ULONG) FileVec,
			                          XAD_OUTSIZE,		FileVecLen,
			                          XAD_NOEXTERN,		CFG_Get(TAGID_XAD_NOEXTXADCLIENTS),
			                          (passwd ? XAD_PASSWORD : TAG_IGNORE), (ULONG) passwd,
			                          XAD_PROGRESSHOOK, (ULONG) &ARC_ExtractFileProgressHook,
			                          TAG_DONE);

			if (xaderr == XADERR_OK) {
				ExtractSuccess = TRUE;
			} else {
				DoMethod(GETERRWIN, MUIM_ErrorWin_ReportXADError,
				         STR(SID_ERRXAD_DFU_WITH, "xadDiskFileUnArc() failed on	%s"),
				         &AE->ae_xfi->xfi_FileName, xaderr);
			}

		} else if (pn->pn_type == PNTYPE_ARCHIVE) {

			xaderr = xadFileUnArc(pn->pn_ai,
			                      XAD_ENTRYNUMBER,   AE->ae_xfi->xfi_EntryNumber,
			                      XAD_OUTMEMORY,     (ULONG) FileVec,
			                      XAD_OUTSIZE,       FileVecLen,
			                      XAD_NOEXTERN,      CFG_Get(TAGID_XAD_NOEXTXADCLIENTS),
			                      (passwd ? XAD_PASSWORD : TAG_IGNORE), (ULONG) passwd,
			                      XAD_PROGRESSHOOK,  (ULONG) &ARC_ExtractFileProgressHook,
			                      TAG_DONE);

			if (xaderr == XADERR_OK) {

				ExtractSuccess = TRUE;

			} else {

				DoMethod(GETERRWIN, MUIM_ErrorWin_ReportXADError,
				         STR(SID_ERRXAD_FU_WITH, "xadFileUnArc() failed on %s"),
				         &AE->ae_xfi->xfi_FileName, xaderr);

			}
		}

		if (ProgressWnd) {
			CloseProgress;
		}
	}

	if (xadrc_ptr) {
		*xadrc_ptr = xaderr;
	}

	if (ExtractSuccess) {

		*Buf_ptr = FileVec;
		*BufLen_ptr = FileVecLen;
		DB(dbg("File -> buffer extraction was a sucess. Buf=%lx Len=%lu", FileVec, FileVecLen))

	} else {

		if (FileVec)
			MEM_FreeVec(FileVec);

		*Buf_ptr    = NULL;
		*BufLen_ptr = 0;
		DB(dbg("File -> buffer extraction FAILED"))

	}

	return ExtractSuccess;
}

/********************************************************************************************
 *
 * Archive identification routines
 *
 ********************************************************************************************
 *
 */

GPROTO UBYTE *ARC_IsArc( UBYTE *FileName ) {

	struct xadArchiveInfo *DiskAI;
	struct FileInfoBlock *FIB;
	UBYTE *ArcName = NULL;
	struct xadClient *xc;
	APTR SegVec;
	BPTR InFile;
	LONG rl, xaderr;

	if (!(SegVec = MEM_AllocVec(ISARC_SEGLEN))) {
		return NULL;
	}

	if ((InFile = Open(FileName, MODE_OLDFILE)) && ((rl = Read(InFile, SegVec, ISARC_SEGLEN)) && rl != -1)) {

		if ((xc = xadRecogFile(ISARC_SEGLEN, SegVec, TAG_DONE))) {

			ArcName = xc->xc_ArchiverName;

		} else {

			if ((FIB = IO_GetFIBVec(FileName)) &&
			        ((FIB->fib_Size == DD_ADF_SIZE) || (FIB->fib_Size == HD_ADF_SIZE))) {

				if ((DiskAI = xadAllocObjectA(XADOBJ_ARCHIVEINFO, NULL))) {

					if ((xaderr = xadGetDiskInfo(DiskAI,
					                             XAD_INFILENAME, (ULONG) FileName,
					                             XAD_NOEXTERN,  CFG_Get(TAGID_XAD_NOEXTXADCLIENTS),
					                             TAG_DONE)) == XADERR_OK) {

						if (FIB->fib_Size == DD_ADF_SIZE) {
							ArcName = "ADF (DD)";
						} else if (FIB->fib_Size == HD_ADF_SIZE) {
							ArcName = "ADF (HD)";
						} else {
							ArcName = NULL;
						}

						xadFreeInfo(DiskAI);

					} else if (xaderr == XADERR_DATAFORMAT) {

						GUI_PrgError(STR(SID_ERR_NDOSDISK, "Non-DOS disk encountered!"), NULL);

					} else {

						DoMethod(GETERRWIN, MUIM_ErrorWin_ReportXADError,
						         STR(SID_ERRXAD_GDI_WITH, "xadGetDiskInfo() failed on %s"),
						         &FileName, xaderr);

					}

					xadFreeInfo(DiskAI);
				}

				MEM_FreeVec(FIB);
			}
		}
	}
	
	if (InFile) {
		Close(InFile);
	}
	
	MEM_FreeVec(SegVec);
	
	return ArcName;
}

GPROTO UBYTE *ARC_IsArcMem( APTR arcbuf, ULONG arclen ) {

	struct xadArchiveInfo *diskai;
	UBYTE *arcname = NULL;
	struct xadClient *xc;
	LONG xaderr;
	
	if ((xc = xadRecogFile(arclen, arcbuf, TAG_DONE))) {
		arcname = xc->xc_ArchiverName;
	} else {

		if (arclen == DD_ADF_SIZE || arclen == HD_ADF_SIZE) {

			if ((diskai = xadAllocObjectA(XADOBJ_ARCHIVEINFO, NULL))) {

				if ((xaderr = xadGetDiskInfo(diskai,
				                             XAD_INMEMORY,  (ULONG) arcbuf,
				                             XAD_INSIZE,    arclen,
				                             XAD_NOEXTERN,  CFG_Get(TAGID_XAD_NOEXTXADCLIENTS),
				                             TAG_DONE)) == XADERR_OK) {

					if (arclen == DD_ADF_SIZE) {
						arcname = "ADF (DD)";
					} else if (arclen == HD_ADF_SIZE) {
						arcname = "ADF (HD)";
					} else {
						arcname = NULL;
					}

					xadFreeInfo(diskai);

				} else if (xaderr == XADERR_DATAFORMAT) {

					GUI_PrgError(STR(SID_ERR_NDOSDISK, "Non-DOS disk encountered!"), NULL);

				} else {

					DoMethod(GETERRWIN, MUIM_ErrorWin_ReportXADError,
					         STR(SID_ERRXAD_GDI_WITH, "xadGetDiskInfo() failed on %s"),
					         NULL, xaderr);
				}

				xadFreeInfo(diskai);
			}
		}
	}

	return arcname;
}


LONG LONGSort( LONG *n1, LONG *n2 ) {
	return (*n1) - (*n2);
}

GPROTO BOOL ARC_IsArcName( UBYTE *fileName ) {
	/* Determine if a file is an archive or not via its name. Note that
	 * this code does only basic name matching so you can't depend on a
	 * result of TRUE being 100% accurate. You will still have to call 
	 * ARC_IsArc() or ARC_IsArcMem() on the archive itself to be sure.
	 */
	 
	/*
	 * These hardcoded strings are temporary until XAD provides an
	 * interface for its client's suffixes.
	 *
	 * If you are adding, you MUST make sure that any new entry
	 * inserted is in alphabetical order, because the array is
	 * scanned using a binary search now, not a linear!
	 *
	 */
	 
	static UBYTE *suffixes[] = {
		/* Some of these have been take from xadclients.guide 1.3 (30.03.2001).
		   Hope you don't mind Stuart. :-) */
		".?q?",
		".a",
		".ace",
		".adf",
		".adz",
		".ampk",
		".arc",
		".arj",
		".ark",
		".boo",
		".btoa",
		".bz",
		".bz2",
		".cab",
		".cmp",
		".cop",
		".cpio",
		".d64",
		".d71",
		".d81",
		".deb",
		".dex",
		".dmp",
		".dms",
		".epf",
		".exe",
		".fms",
		".gdc",
		".gz",
		".ha",
		".hcx",
		".hex",
		".hqx",
		".img",
		".lbr",
		".lha",
		".lhf",
		".lhw",
		".lib",
		".lqr",
		".lzh",
		".lzs",
		".mbx",
		".mdc",
		".mp3",
		".msa",
		".pack",
		".pak",
		".pkd",
		".pma",
		".qq",
		".qqq",
		".rar",
		".rpm",
		".run",
		".se",
		".sfx",
		".ship",
		".shr",
		".sit",
		".sq",
		".sqz",
		".svg",
		".t64",
		".tar",
		".tar.gz",
		".tar.Z",
		".taz",
		".tbz",
		".tbz2",
		".tgz",
		".uu",
		".uue",
		".warp",
		".wrp",
		".x64",
		".xds",
		".xms",
		".xpk",
		".XqX",
		".xx",
		".xxe",
		".z",
		".Z",
		".zap",
		".zip",
		".zom",
		".zoo",
		".lzx",
		NULL
	};

	register UBYTE **sp;
	register ULONG fileNameLen = strlen(fileName);

	if (xadMasterBase->xmb_DefaultName) {
		if (Stricmp(xadMasterBase->xmb_DefaultName, fileName) == 0) {
			return TRUE;
		}
	}
	
	// Linear
	for (sp = suffixes; *sp;) {
		register ULONG suffixLen = strlen(*sp);

		if (fileNameLen >= suffixLen) {
			if (Stricmp(fileName + fileNameLen - suffixLen, *sp) == 0) {
				return TRUE;
			}
		}
		sp++;
	}

#ifdef UNTESTED
	// Binary search
	
	int left, right, pivot;
	
	left = 0; right = sizeof(suffixes);
	
	while (left <= right) {
		pivot = (left + right) / 2;

		register int suffixLen = strlen(suffixes[pivot]);
		
		if (fileNameLen >= suffixLen) {
			fileNameSuffix = fileName + fileNameLen - suffixLen;
		} else {
			fileNameSuffix = "";
		}
		
		register long result = Stricmp(fileNameSuffix, suffixes[pivot]);
		
		if (result == 0) {
			return TRUE;
		} else if (result > 0) {
			left = pivot + 1;   // Search right
		} else {
			right = pivot - 1;  // Search left
		}
	}
#endif

	return FALSE;
}

/********************************************************************************************
 *
 * Archive loading routines
 *
 ********************************************************************************************
 *
 */

GPROTO AHN *ARC_LoadNewArchive(	UBYTE *arcpath,	AHN	*parent_ahn, AE	*embedded_ae, BOOL MultiMode ) {

	/*
	 * Load an archive. This routine basically checks to see if the
	 * archive exists, if it does, it creates a new AHN, which
	 * will always be at the top of the history list. It will also make it
	 * the active entry. It doesn't display the entries in the lister
	 * immediately. This has to be done manually with a call to
	 * DoMethod(av, MUIM_ArcView_ShowArcEntries, NULL). This allows you to
	 * load many archives without the lister constantly updating.
	 *
	 * This routine will also make sure the archive isn't already loaded.
	 *
	 * If the archive path contains a ":-->" symbol, then this routine
	 * will recursively extract/load each embedded archive.
	 *
	 * Don't free the AHN that is returned, since it'll	be
	 * attached to the HistoryList.
	 *
	 * Parameters
	 * ----------
	 * ArcPath    - Path of the archive to load.
	 * ParentAHN  - If this is an embedded archive, pass the parent AHN here.
	 * EmbeddedAE - If this is an embedded archive, pass the ArcEntry of
	 *              the parent AHN that represents the embedded archive.
	 *
	 * Todo
	 * ----
	 * - Adapt this routine to scan directories.
	 *
	 */

	struct FileInfoBlock *fib = NULL;
	AHN *ahn = NULL, *tmp_ahn, *tmp_embedded_ahn;
	Object *ah = GETARCHISTORY;
	LONG index = 0, eindex = 0;
	UBYTE **patharray;
	BPTR testlock;

	DB(dbg("ARC_LoadNewArchive(arcpath=\"%s\", parent_ahn=0x%08lx, "
	       "embedded_ae=0x%08lx, MultiMode=%s)", arcpath, parent_ahn,
	       embedded_ae, MultiMode ? "Yes" : "No"))

	if (!(patharray = ARC_GetArcPathComponents(arcpath))) {
		return NULL;
	}

	/*
	 * Load the archive into the history, but don't
	 * redisplay the history list.
	 */

	if ((testlock = Lock(patharray[0], SHARED_LOCK)) &&
	        (fib = AllocDosObject(DOS_FIB, NULL)) &&
	        Examine(testlock, fib)) {

		if (fib->fib_DirEntryType < 0) {

			tmp_ahn = (AHN *) DoMethod(ah, MUIM_ArcHistory_FindArc, testlock, &index);
			tmp_embedded_ahn = (AHN *) DoMethod(ah, MUIM_ArcHistory_FindEmbeddedArc,
			                                    parent_ahn, embedded_ae, &eindex);
			if (tmp_embedded_ahn) {

				ahn = tmp_embedded_ahn;
				DoMethod(ah, MUIM_ArcHistory_MakeActive, ahn);

			} else if (tmp_ahn) {

				ahn = tmp_ahn;
				DoMethod(ah, MUIM_ArcHistory_MakeActive, ahn);

			} else {

				ahn = NULL;
				DoMethod(ah, MUIM_ArcHistory_Load, patharray[0],
				         parent_ahn, embedded_ae, NULL, MultiMode);
			}
		} else {

			GUI_PrgError("'%s' is a directory, I need a file.", &arcpath);

		}

	} else {

		GUI_PrgDOSError(STR(SID_ERR_LOADINGARC, "Error while loading archive '%s'!"), &arcpath);

	}

	if (testlock) {

		UnLock(testlock);
	}

	if (fib) {
		FreeDosObject(DOS_FIB, fib);
	}

	/*
	 * If embedded archives were specified in the path
	 * this code segment will recursively loaded each
	 * one into the archive history list.
	 */

	if (ahn) {
		AE *ae;
		AHN *parent_ahn = ahn;
		UBYTE **array_ptr = &patharray[1];
		UBYTE *expath = NULL, *tmp[2];
		BOOL looperr = FALSE;

		while (*array_ptr && !looperr) {

			if (!(ae = ARC_FindAEInAHN_ViaName(parent_ahn, *array_ptr))) {

				tmp[0] = parent_ahn->ahn_EmbeddedArc ?
				         parent_ahn->ahn_EmbeddedListStr : ahn->ahn_Path;
				tmp[1] = *array_ptr;
				GUI_PrgError("Archive '%s' doesn't contain an embedded archive called '%s'!", &tmp);
				IO_DeleteTmpFile(expath);
				looperr = TRUE;

			} else if ((expath = IO_GetTmpLocation(*array_ptr))) {

				if (ARC_ExtractFile(parent_ahn, ae, expath, FALSE, NULL, "")) {

					if (!(parent_ahn = ARC_LoadNewArchive(expath, parent_ahn, ae, MultiMode))) {
						GUI_PrgError("Failed to load embedded archive '%s'", array_ptr);
						IO_DeleteTmpFile(expath);
						looperr = TRUE;

					} else {
						ahn = parent_ahn;
					}

				} else {
					IO_DeleteTmpFile(expath);
					looperr = TRUE;
				}
			}

			array_ptr++;
		}
	}

	ARC_FreeArcPathComponents(patharray);

	return ahn;
}

/* TODO: This should be part of the ArcHistory class, as a method. */

GPROTO ULONG ARC_LoadArcsFromWBArgs(
    struct WBArg *WBA, ULONG WBATotal, BOOL ShowProgWin ) {

	Object *ah = GETARCHISTORY, *av = GETARCVIEW;
	register ULONG Cnt = 0, LoadCnt = 0;
	UBYTE TmpPath[256];
	AHN	*ahn;
	DB(dbg("ARC_LoadArcsFromWBArgs(wba=0x%08lx, wba_total=%lu, show_prog_win=%ld)",
	       WBA, WBATotal, ShowProgWin))

	if (WBATotal == 0) {
		return 0;
	}

	set(GETMAINWIN,	MUIA_Window_Activate, TRUE);

	if (WBATotal > 1) {
		OpenProgress(WBATotal, STR(SID_ADDARCSTOLIST, "Adding archives to history list..."));
	}

	DoMethod(ah, MUIM_ArcHistory_SetQuiet, TRUE);

	while (Cnt < WBATotal) {

		if (WBA[Cnt].wa_Name && WBA[Cnt].wa_Lock) {

			if (NameFromLock(WBA[Cnt].wa_Lock, TmpPath, sizeof(TmpPath)-1) &&
			        AddPart(TmpPath, WBA[Cnt].wa_Name, sizeof(TmpPath)-1)) {

				ChangeProgressStatus(TmpPath, NULL);

				if (ARC_LoadNewArchive(TmpPath, NULL, NULL, WBATotal == 1 ? FALSE : TRUE)) {
					LoadCnt++;
				}
			}
		}

		Cnt++;

		if (WBATotal > 1) {
			if (UpdateProgress(NULL, NULL, Cnt)) {
				break;
			}
		}
	}

	DoMethod(ah, MUIM_ArcHistory_SetQuiet, FALSE);

	if ((ahn = (AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN)))	{
		DoMethod(av, MUIM_ArcView_ShowArcEntries, ahn);
	}

	if (CFG_Get(TAGID_MAIN_AUTOSELECTALL)) {
		DoMethod(av, MUIM_ArcView_SelectAll);
	}

	CloseProgress;

	return LoadCnt;
}

GPROTO UBYTE *ARC_ExpandPathComponents(	UBYTE *arcpath ) {
	/*
	 * Expand shorthand embedded archive path pointers ("::"). Basically
	 * this routine takes a string like "Archive.lha::Embedded.lha" and
	 * expands it to "Archive.lha :--> Embedded.lha". The resulting string
	 * must be free with MEM_FreeVec().
	 *
	 * This routine also handles superfluous spaces around the symbols.
	 *
	 * So if you pass
	 *
	 * "Archive.lha :: Embedded.lha" or
	 * "Archive.lha :--> Embedded.lha"
	 *
	 * then "Archive.lha :--> Embedded.lha" will be returned.
	 *
	 * Note: The output string is limited to 512 bytes.
	 *
	 */

	UBYTE tmpbuf[512 + 4];
	register UBYTE *in = arcpath, *out = &tmpbuf[1];

	if (!arcpath) {
		return NULL;
	}

	DB(dbg("ARC_ExpandPathComponents(arcpath=\"%s\")", arcpath))

	tmpbuf[0] = 0;

	while (*in) {

		if (*in == ':') {
			ULONG skiplen = 0;

			if (!strncmp(in, "::", 2)) {
				skiplen = 2;

			} else if (!strncmp(in, ":-->", 4)) {

				skiplen = 4;
			}

			if (skiplen) {
				in += skiplen;

				while (*in && *in == ' ') {
					++in;
				}

				while (out[-1] && out[-1] == ' ') {
					--out;
				}

				strcpy(out, EMBEDDEDARC_SEPARATOR);
				out += strlen(out);

			} else {
				*out++ = *in++;
			}

		} else {
			*out++ = *in++;
		}
	}

	*out = 0;

	return MEM_StrToVec(&tmpbuf[1]);
}

GPROTO UBYTE **ARC_GetArcPathComponents( UBYTE *arcpath	) {
	/*
	 * Break down an archive path string into a NULL terminated array.
	 * A string like: "ram:Archive.lzx :--> 1.lzx :--> 2.lzx :--> 3.lzx"
	 * Will be broken into: "ram:Archive.lzx","1.lzx","2.lzx","3.lzx",NULL
	 *
	 * When you've finished with the array you must free it with
	 * ARC_FreeArcPathComponents().
	 */

	UBYTE **array = NULL;
	UBYTE *ptr, *ap = arcpath;
	ULONG cnt;

	if (!arcpath) {
		return NULL;
	}

	DB(dbg("ARC_GetArcPathComponents(arcpath=\"%s\")", arcpath))

	if ((arcpath = MEM_StrToVec(arcpath))) {

		for (cnt = 1;;cnt++) {
			if (!(ptr = strstr(ap, EMBEDDEDARC_SEPARATOR)))	{
				break;
			}
			ap = ptr + sizeof(EMBEDDEDARC_SEPARATOR) - 1;
		}

		if (cnt && (array = MEM_AllocVec(sizeof(UBYTE **) * (cnt + 1)))) {
			UBYTE **array_ptr = array;
			BOOL looperr = FALSE;

			for (ap = arcpath;;) {

				if ((ptr = strstr(ap, EMBEDDEDARC_SEPARATOR))) {
					*ptr = 0;
				}

				if (!(*array_ptr++ = MEM_StrToVec(ap))) {
					looperr = TRUE;
				}

				if (!ptr || looperr) {
					break;
				}
				ap = ptr + sizeof(EMBEDDEDARC_SEPARATOR) - 1;
			}

			*array_ptr = NULL;

			if (looperr) {
				ARC_FreeArcPathComponents(array);
				array = NULL;
			}
		}

		MEM_FreeVec(arcpath);
	}

	return array;
}

GPROTO void ARC_FreeArcPathComponents( UBYTE **pathArray ) {
	/* Free the array returned by ARC_GetArcPathComponents(). */
	UBYTE **arrayPtr = pathArray;

	if (!pathArray) {
		return;
	}

	while (*arrayPtr) {
		MEM_FreeVec(*arrayPtr++);
	}

	MEM_FreeVec(pathArray);
}

/********************************************************************************************
 *
 * XAD related routines
 *
 ********************************************************************************************
 *
 */

GPROTO ULONG XADGetFileCount( struct xadArchiveInfo	*xadAI ) {
	register struct xadFileInfo *xadFI = xadAI->xai_FileInfo;
	register ULONG fileCnt = 0;

	if (!xadFI) {
		return 0;
	}

	do {
		fileCnt++;
	} while ((xadFI = xadFI->xfi_Next));

	return fileCnt;
}

GPROTO ULONG XADGetExpLength( struct xadArchiveInfo *xadAI ) {
	register struct xadFileInfo *xadFI = xadAI->xai_FileInfo;
	register ULONG ExpSize = 0;

	if (!xadFI) {
		return 0;
	}

	do {
		ExpSize += xadFI->xfi_Size;
	} while ((xadFI = xadFI->xfi_Next));

	return ExpSize;
}


GPROTO void ARC_ChangeViewMode( ULONG NewViewMode ) {

	// TODO: Make this obsolete
	DB(dbg("ARC_ChangeViewMode(NewViewMode=%ld)", NewViewMode))
	CFG_Set(TAGID_MAIN_ARCVIEWMODE, NewViewMode);
	DoMethod(GETARCVIEW, MUIM_ArcView_SwapViewMode, NewViewMode);

}

/********************************************************************************************
 *
 * Misc archive related routines
 *
 ********************************************************************************************
 *
 */

GPROTO void ARC_RestoreAttrs( UBYTE *DestPath, struct xadFileInfo *xadFI ) {

	/* Restore file comments, attributes, dates, etc. from an xadFileInfo structure. */
	LONG xaderr;
	struct DateStamp DS;

	if (!DestPath || !xadFI) {
		return;
	}

	xaderr = xadConvertDates(
	             XAD_DATEXADDATE,      (ULONG) &xadFI->xfi_Date,
	             XAD_GETDATEDATESTAMP, (ULONG) &DS,
	             TAG_DONE);

	if (xaderr == XADERR_OK) {
		SetFileDate(DestPath, &DS);
	}

	if (xadFI->xfi_Comment) {
		SetComment(DestPath, xadFI->xfi_Comment);
	}

	SetProtection(DestPath, xadFI->xfi_Protection);
}

GPROTO AE **ARC_GetSelected( AHN *ahn, ULONG *amtPtr, ULONG *amtFilesPtr,
                             ULONG *amtDirsPtr, BOOL all ) {

	/* Get the currently selected file/dir entries.
	 * Returns the currently selected file/dir entries, as an array of
	 * ArcEntries. If the All flag is TRUE, then all ArcEntries will be
	 * returned. Free the resulting pointer with ARC_FreeFileAEs().
	 */

	/* TODO: Selected entries should be handled by ArcView
	 *       Break this into ARC_GetAllFiles, etc...
	 */

	AE **aeArray = NULL, **aea, *ae = NULL;
	ULONG totalSelCnt = 0;
	Object *arcViewObj = GETARCVIEW;

	if (amtFilesPtr) {
		*amtFilesPtr = 0;
	}

	if (amtDirsPtr) {
		*amtDirsPtr  = 0;
	}

	if (all) {
		AHN *ahn;
		AE *rootAE;

		if (!(ahn = (AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN))) {
			return NULL;
		}

		rootAE = ahn->ahn_CurrentPN->pn_rootae;

		if ((totalSelCnt = AETree_CountAllAEs(ahn, rootAE, amtFilesPtr, amtDirsPtr))) {
			if ((aeArray = MEM_AllocVec(AE_SIZE * (totalSelCnt + 1)))) {
				*AETree_GetAllAEs(ahn, rootAE, aeArray, NULL) = NULL;
			}
		}
	} else {

		LONG id = -1, selCnt;
		DoMethod(arcViewObj, MUIM_ArcView_GetActiveAE, &ae);

		if (!(selCnt = DoMethod(arcViewObj, MUIM_ArcView_GetSelCnt))) {
			return NULL;
		}

		/* TODO: revamp this */

		if (ae && ae->ae_IsDir && selCnt == 1 &&
		        (CFG_Get(TAGID_MAIN_ARCVIEWMODE) == MUIV_ArcView_SwapViewMode_Listtree ||
		         CFG_Get(TAGID_MAIN_ARCVIEWMODE) == MUIV_ArcView_SwapViewMode_NListtree)) {

			/* If the user is in tree mode and has one entry selected,
			   which is a dir, then extract the contents of that dir. */

			totalSelCnt = AETree_CountAllAEs(ahn, ae, amtFilesPtr, amtDirsPtr);

			if ((aea = aeArray = MEM_AllocVec(AE_SIZE * (totalSelCnt + 1)))) {
				*(aea = AETree_GetAllAEs(ahn, ae, aea, NULL)) = NULL;
			}

		} else {

			/* Else, just extract each selected entry... */

			for (;;) {

				if ((id = (LONG) DoMethod(arcViewObj, MUIM_ArcView_NextSelected, id)) == -1) {
					break;
				}

				if (!(ae = (AE *) DoMethod(arcViewObj, MUIM_ArcView_GetEntry, id, NULL))) {
					break;
				}

				if (ae->ae_IsDir) {

					if (amtDirsPtr) {
						*amtDirsPtr += 1;
					}

					totalSelCnt += AETree_CountAllAEs(ahn, ae, amtFilesPtr, amtDirsPtr);

				} else {

					if (amtFilesPtr) {
						*amtFilesPtr += 1;
					}

					totalSelCnt++;
				}
			}

			if (totalSelCnt && (aea = aeArray = MEM_AllocVec(AE_SIZE * (totalSelCnt + 1)))) {

				for (id = -1;;) {

					if ((id = (LONG) DoMethod(arcViewObj, MUIM_ArcView_NextSelected, id)) == -1) {
						break;
					}

					if (!(ae = (AE *) DoMethod(arcViewObj, MUIM_ArcView_GetEntry, id, NULL))) {
						break;
					}

					if (ae->ae_IsDir) {
						aea = AETree_GetAllAEs(ahn, ae, aea, NULL);
					} else {
						*aea++ = ae;
					}
				}

				*aea = NULL;
			}
		}
	}

	if (aeArray) {
		ARC_SortAEArrayToXADOrder(ahn, aeArray, totalSelCnt);
	}

	if (amtPtr) {
		*amtPtr = totalSelCnt;
	}

	return aeArray;
}

GPROTO void ARC_FreeSelected( AE **ae_array ) {

	MEM_FreeVec(ae_array);

}
