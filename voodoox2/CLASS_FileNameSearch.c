/*
 $Id: CLASS_FileNameSearch.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class for file name based searching.

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

/*
 *  Class Name: FileNameSearch
 * Description: This is the page in the search window that controls filename searches.
 *  Superclass: MUIC_Group
 */

#include "vx_include.h"
#include "FileNameSearch.h"
#include "CLASS_SearchWin_Defs.h"
#include "vx_arc.h"
#include "vx_misc.h"
#include "vx_io.h"
#include "vx_main.h"
#include "vx_debug.h"
#include "vx_mem.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *FileNamePat;
	Object *Start;
	UBYTE SearchPattern[256];
//------------------------------------------------------------------------------
*/

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;

	WHERE;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Group_Horiz, FALSE,
		Child, HGroup,
			Child, KeyLabel2("Pattern:", 't'),
			Child, data.FileNamePat = StringObject,
				StringFrame,
				MUIA_ShortHelp,       HELP_SEARCH_FILENAME_PATTERN,
				MUIA_CycleChain,      1,
				MUIA_ControlChar,     'p',
				MUIA_String_MaxLen,   512,
				MUIA_String_Contents, "#?",
			End,
		End,
		Child, data.Start = MyKeyButton("Start filename search",'t', HELP_SEARCH_FILENAME_STARTSEARCH),
		TAG_MORE, (ULONG) inittags(msg)
	);

	if (!obj) {
		return 0;
	}

	BUTTON_METHOD(data.Start, MUIM_FileNameSearch_Start);

	data.SearchPattern[0] = 0;

	if (ParsePatternNoCase(GetToolType(TTID_ARCSEARCHPAT),
	                       data.SearchPattern, sizeof(data.SearchPattern)-1) != 1) {
		data.SearchPattern[0] = 0;
	}

	PREPDATA;
	return (ULONG) obj;
}

ULONG FilenameSearch(

    struct Data *data,
    struct ArcHistoryNode *ahn,
    UBYTE *ppncbuf,
    BOOL   searchembedded,
    UBYTE *arcname,
    BOOL   showprogress ) {
	APTR filebuf = NULL;
	ULONG filelen = 0;
	ULONG aecnt, matchcnt = 0, pcnt = 0;
	UBYTE *tmpname;

	struct ArcHistoryNode *embedded_ahn;
	struct ArcEntry **ae_ptr, *nextae;
	struct PN *pn = ahn->ahn_CurrentPN;

	BOOL abortloop = FALSE;
	LONG xad_err;
	UBYTE *fn;
	extern BOOL ProgressHook_OverwriteAll; /* TODO: Remove this kludge */

	/* IDEA: Search for dir names too? */

	if (!ahn || !ppncbuf) {
		return 0;
	}

	aecnt  = pn->pn_linear_cnt;
	ae_ptr = pn->pn_linear_aes_xad;

	GUI_PrintStatus("Searching %s for filename pattern...", (ULONG) arcname);

	if (showprogress) {
		OpenProgress(aecnt, "Searching archive...");
	}

	ProgressHook_OverwriteAll = FALSE;

	while (aecnt--) {
		nextae = *ae_ptr;

		ARC_ResolveAESize(nextae);
		ARC_ResolveAEProt(nextae);
		ARC_ResolveAEDate(nextae);

		if (showprogress) {
			UpdateProgress(NULL, NULL, ++pcnt);
		}

		if (CheckProgressAbort) {
			GUI_PrintStatus("Search aborted.", 0);
			break;
		}

		if (nextae->ae_IsDir) {
			ae_ptr++;
			continue;
		}

		fn = FilePart(nextae->ae_FullPath);
		ChangeProgressStatus("%s...", &nextae->ae_FullPath);

		if (nextae->ae_Size == 0) {
			ae_ptr++;
			continue;
		}

		if (MatchPatternNoCase(ppncbuf, fn)) {
			struct SearchResultNode tmpsrn;
			memset(&tmpsrn, 0, sizeof(struct SearchResultNode));
			matchcnt++;
			tmpsrn.srn_ArcPath     = (ahn->ahn_EmbeddedArc ?
			                          ahn->ahn_EmbeddedListStr : ahn->ahn_Path);
			tmpsrn.srn_EntryName   = nextae->ae_FullPath;
			tmpsrn.srn_MatchCnt    = ~0;
			tmpsrn.srn_EmbeddedArc = ahn->ahn_EmbeddedArc;
			DoMethod(GETSEARCHWIN, MUIM_SearchWin_RecordMatch, &tmpsrn);
		}

		if (searchembedded && (ARC_IsArcName(fn) || (data->SearchPattern[0] &&
		                       MatchPatternNoCase(data->SearchPattern, FilePart(nextae->ae_FullPath))))) {
			if ((tmpname = IO_GetTmpLocation(FilePart(nextae->ae_FullPath)))) {

				ChangeProgressStatus("Extracting embedded archive %s...",
				                     &nextae->ae_FullPath);

				if (ARC_ExtractFileToBuf(ahn, nextae, (APTR *) &filebuf, &filelen, FALSE, &xad_err) &&
				        ARC_IsArcMem(filebuf, filelen)) {

					ChangeProgressStatus("Entering embedded archive %s...", &nextae->ae_FullPath);

					if (IO_SaveFile(tmpname, filebuf, filelen)) {
						if ((embedded_ahn = ARC_CreateAHN(tmpname, ahn, nextae, NULL))) {
							matchcnt += FilenameSearch(data, embedded_ahn, ppncbuf,
							                           searchembedded, nextae->ae_FullPath, FALSE);
							ARC_FreeAHN(embedded_ahn);
						}
					}

					MEM_FreeVec(filebuf);

				} else if (xad_err == XADERR_BREAK) {

					abortloop = TRUE;
				}

				IO_DeleteTmpFile(tmpname);
			}
		}

		if (abortloop) {
			break;
		}

		ae_ptr++;
	}

	if (showprogress) {
		CloseProgress;
	}

	return matchcnt;
}

DECLARE(Start)
{
	GETDATA;
	struct ArcHistoryNode *ahn = NULL;
	ULONG matchcnt = 0, searchmode = CYCLE_SEARCHMODE_CURRENT;
	ULONG searchembedded = TRUE;
	UBYTE ppncbuf[256], *pat = NULL;
	BOOL aborted = FALSE;
	LONG id;
	INITAH;

	get(data->FileNamePat, MUIA_String_Contents, &pat);

	if (!pat) {
		return 0;
	}

	if (GetActiveAHN() == NULL) {
		GUI_PrgError("No archive loaded!", NULL);
		return 0;
	}

	if (!pat) {
		GUI_PrgError("Couldn't get pattern string!", NULL);
		return 0;
	}

	if (strlen(pat) == 0) {
		GUI_PrgError("Please provide a pattern string!", NULL);
		return 0;
	}

	if (ParsePatternNoCase(pat, ppncbuf, sizeof(ppncbuf)-1) != 1) {
		GUI_PrgDOSError("Failed to parse AmigaDOS pattern '%s' - Please make sure it's valid!", &pat);
		return 0;
	}

	DoMethod(_win(obj), MUIM_SearchWin_ClearMatches);
	get(_win(obj), MUIA_SearchWin_SearchMode, &searchmode);

	GUI_PrintStatus("Please wait...", 0);

	set(_win(obj), MUIA_Window_Sleep, TRUE);
	set(data->Start, MUIA_Disabled, TRUE);

	switch(searchmode) {

	case CYCLE_SEARCHMODE_CURRENT:
		if ((ahn = GetActiveAHN())) {
			matchcnt = FilenameSearch(data, ahn, (UBYTE *) &ppncbuf,
			                          searchembedded, ahn->ahn_Path, TRUE);
		}
		break;

	case CYCLE_SEARCHMODE_ALL: 
		OpenProgress(xget(ah, MUIA_ArcHistory_Count), "Searching all archives in history list...");

		for (id = -1, matchcnt = 0;;) {

			if (UpdateProgress(NULL, NULL, id)) {
				GUI_PrintStatus("Search aborted.", 0);
				aborted = TRUE;

				break;
			}

			if ((id = DoMethod(ah, MUIM_ArcHistory_GetNext, id, &ahn)) == -1) {
				break;
			}

			matchcnt = FilenameSearch(data, ahn, (UBYTE *) &ppncbuf,
			                          searchembedded, ahn->ahn_Path, FALSE);
		}

		CloseProgress;
		break;
	}


	set(data->Start, MUIA_Disabled, FALSE);
	set(_win(obj), MUIA_Window_Sleep, FALSE);

	if (!aborted) {
		GUI_PrintStatus("Found %lu matches.", matchcnt);
	}

	return 0;
}

