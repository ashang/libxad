/*
 $Id: CLASS_StringSearch.c,v 1.2 2004/01/21 08:17:43 andrewbell Exp $
 Custom class for the string search GUI.

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
 *  Class Name: StringSearch
 * Description: This is the page in the search window that controls string searches.
 *  Superclass: MUIC_Group
 */

#include "vx_include.h"
#include "StringSearch.h"
#include "CLASS_SearchWin_Defs.h"
#include "vx_arc.h"
#include "vx_io.h"
#include "vx_misc.h"
#include "vx_main.h"
#include "vx_debug.h"
#include "vx_mem.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *String;
	Object *UCeqLC;
	Object *Start;
	UBYTE SearchPattern[256];
*/

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Group_Horiz, FALSE,
		Child, HGroup,
			Child, KeyLabel2("String:", 't'),
			Child, data.String = StringObject,
				StringFrame,
				MUIA_ShortHelp,       HELP_SEARCH_STRING_STRING,
				MUIA_CycleChain,      1,
				MUIA_ControlChar,     's',
				MUIA_String_MaxLen,   512,
				MUIA_String_Contents, "<< enter a search string here >>",
			End,
			Child, ColGroup(2),
				Child, Label1("UPPERCASE = lowercase?"),
				Child, data.UCeqLC = MyCheckMark(TRUE, NULL, HELP_SEARCH_STRING_UCEQLC),
			End,
		End,
		Child, data.Start = MyKeyButton("Start string search",'t', HELP_SEARCH_STARTSTRINGSEARCH),
		TAG_MORE, (ULONG)inittags(msg)
	);

	if (!obj) {
		return 0;
	}

	BUTTON_METHOD(data.Start, MUIM_StringSearch_Start);

	data.SearchPattern[0] = 0;

	if (ParsePatternNoCase(GetToolType(TTID_ARCSEARCHPAT),
	                       data.SearchPattern, sizeof(data.SearchPattern)-1) != 1) {
		data.SearchPattern[0] = 0;
	}

	PREPDATA;

	return (ULONG) obj;
}

extern ASM(ULONG) SearchMem( REG_A0(void *Mem), REG_D0 (ULONG MemLen), REG_A1 (UBYTE *CmpStr), REG_D1 (ULONG CmpStrLen) );
extern ASM(ULONG) SearchMemNoCase( REG_A0 (void *Mem), REG_D0 (ULONG MemLen), REG_A1 (UBYTE *CmpStr), REG_D1 (ULONG CmpStrLen) );

ULONG StringSearch(
    struct Data *data,
    struct ArcHistoryNode *ahn,
    UBYTE *searchstr,
    BOOL   searchembedded,
    UBYTE *arcname,
    BOOL   showprogress,
    BOOL   fastsearch,
    BOOL   uc_eq_lc )
{
	/* Recursive */

	struct ArcHistoryNode *embedded_ahn = NULL;
	struct ArcEntry **ae_ptr, *nextae;
	UBYTE *tmparcname, *filebuf;
	ULONG aecnt = 0, matchcnt = 0, filelen = 0, pcnt = 0;
	struct PN *pn = ahn->ahn_CurrentPN;
	BOOL abortloop = FALSE;
	LONG xad_err;
	UBYTE *tmp[2];

	extern BOOL ProgressHook_OverwriteAll; /* TODO: Remove this kludge */

	if (ahn->ahn_CurrentPN == NULL) {
		DB(dbg("WARNING: StringSearch:ahn->ahn_CurrentPN == NULL\n"))
		return 0;
	}

	if (!ahn->ahn_CurrentPN->pn_ready) {
		DB(dbg("WARNING: StringSreach:Portion node is not ready!\n"))
		return 0;
	}

	if (!ahn || !searchstr) {
		DB(dbg("WARNING: StringSearch/No AHN or searchStr pointer!\n"))
		return 0;
	}

	GUI_PrintStatus("Searching %s...", (ULONG) arcname);

	aecnt  = pn->pn_linear_cnt;
	ae_ptr = pn->pn_linear_aes_xad;

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

		//DB(dbg("StringSearch: examining archive entry '%s'\n", nextae->ae_FullPath))

		tmp[0] = (ahn->ahn_EmbeddedArc ? ahn->ahn_EmbeddedListStr : ahn->ahn_Path);
		tmp[1] = nextae->ae_FullPath;

		ChangeProgressStatus("Extracting %s/%s", &tmp);

		if (nextae->ae_Size == 0) {
			ae_ptr++;
			//DB(dbg("NOTE: Size is 0 for this entry. Ignoring it."))
			continue;
		}

		filebuf = NULL;
		filelen = 0; /* Just in case */

		if (ARC_ExtractFileToBuf(ahn, nextae, (APTR *) &filebuf, &filelen, FALSE, &xad_err)) {

			if (searchembedded &&
			        (ARC_IsArcMem(filebuf, filelen) || (data->SearchPattern[0] &&
			                                            MatchPatternNoCase(data->SearchPattern, FilePart(nextae->ae_FullPath))))) {

				ChangeProgressStatus("Entering embedded archive %s...", &nextae->ae_FullPath);

				if ((tmparcname = IO_GetTmpLocation(FilePart(nextae->ae_FullPath)))) {

					if (IO_SaveFile(tmparcname, filebuf, filelen)) {

						if ((embedded_ahn = ARC_CreateAHN(tmparcname, ahn, nextae, NULL))) {

							//DB(dbg("PERFORMING ACTUAL SEARCH\n"))
							matchcnt += StringSearch(data, embedded_ahn, searchstr,
							                         searchembedded, nextae->ae_FullPath, FALSE, fastsearch, uc_eq_lc);
							ARC_FreeAHN(embedded_ahn);

						} else {

							DB(dbg("WARNING: StringSearch:ARC_CreateAHN() failed.\n"))
						}

					} else {

						DB(dbg("WARNING: StringSearch: IO_SaveFile() failed.\n"))
					}

					IO_DeleteTmpFile(tmparcname);
				} else {

					DB(dbg("WARNING: StringSearch: IO_GetTmpLocation() failed.\n"))
				}

			} else {

				UBYTE *range_ptr = filebuf;
				ULONG sr = 0, range_len = filelen, filematchcnt = 0;
				ChangeProgressStatus("Searching %s...", &nextae->ae_FullPath);
				//DB(dbg("PERFORMING ACTUAL SEARCH\n"))

				for (;;) {
					if (uc_eq_lc) {
						sr = SearchMemNoCase(range_ptr, range_len, searchstr, strlen(searchstr));
					} else {
						sr = SearchMem(range_ptr, range_len, searchstr, strlen(searchstr));
					}

					if (sr == ~0) {
						break;
					} else {
						matchcnt++;
						sr++; /* Move over to next byte... */

						if (fastsearch && matchcnt == 1) {
							break;
						}

						filematchcnt++;
						range_ptr += sr;
						range_len -= sr;
					}
				}

				if (filematchcnt != 0) {
					struct SearchResultNode tmpsrn;

					memset(&tmpsrn, 0, sizeof(struct SearchResultNode));
					tmpsrn.srn_ArcPath = (ahn->ahn_EmbeddedArc ?
					                      ahn->ahn_EmbeddedListStr : ahn->ahn_Path);
					tmpsrn.srn_EntryName = nextae->ae_FullPath;
					tmpsrn.srn_MatchCnt = fastsearch ? ~0 : filematchcnt;
					tmpsrn.srn_EmbeddedArc = ahn->ahn_EmbeddedArc;

					DoMethod(GETSEARCHWIN, MUIM_SearchWin_RecordMatch, &tmpsrn);
				}
			}

			MEM_FreeVec(filebuf);

		} else {

			DB(dbg("WARNING: ARC_ExtractFileToBuf() failed\n"))
		}

		if (xad_err == XADERR_BREAK) {

			abortloop = TRUE;
		}

		if (abortloop) {
			break;
		}

		ae_ptr++;

	}	// while() ends

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
	ULONG searchembedded = TRUE, uc_eq_lc = TRUE;
	BOOL fastsearch = FALSE, aborted = FALSE;
	Object *sw = GETSEARCHWIN;
	UBYTE *searchstr = NULL;
	LONG id, i;
	INITAH;

	get(sw,           MUIA_SearchWin_DoEmbedded, &searchembedded);
	get(data->UCeqLC, MUIA_Selected,             &uc_eq_lc);
	get(data->String, MUIA_String_Contents,      &searchstr);

	if (!xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN)) {
		GUI_PrgError("No archive loaded!", NULL);
		return 0;
	}

	if (!searchstr) {
		GUI_PrgError("Couldn't get search string!", NULL);
		return 0;
	}

	if (strlen(searchstr) == 0) {
		GUI_PrgError("Please provide a search string!", NULL);
		return 0;
	}

	DoMethod(sw, MUIM_SearchWin_ClearMatches);
	get(sw, MUIA_SearchWin_SearchMode, &searchmode);
	GUI_PrintStatus("Please wait...", 0);
	set(_win(obj), MUIA_Window_Sleep, TRUE);
	set(data->Start, MUIA_Disabled, TRUE);

	switch(searchmode) {
	case CYCLE_SEARCHMODE_CURRENT:
		if ((ahn = GetActiveAHN())) {
			matchcnt = StringSearch(data, ahn, searchstr, searchembedded,
			                        ahn->ahn_Path, TRUE, fastsearch, uc_eq_lc );
		}
		break;

	case CYCLE_SEARCHMODE_ALL: 
		OpenProgress(xget(ah, MUIA_ArcHistory_Count), "Searching all archives in history list...");
		for (id = -1, i = 0, matchcnt = 0;;) {
			if (UpdateProgress(NULL, NULL, i++)) {
				GUI_PrintStatus("Search aborted.", 0);
				aborted = TRUE;
				break;
			}
			if ((id = DoMethod(ah, MUIM_ArcHistory_GetNext, id, &ahn)) == -1) {
				break;
			}
			matchcnt += StringSearch(data, ahn, searchstr, searchembedded,
			                         ahn->ahn_Path, FALSE, fastsearch, uc_eq_lc);
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

