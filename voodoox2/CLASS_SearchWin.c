/*
 $Id: CLASS_SearchWin.c,v 1.2 2004/01/21 18:28:19 andrewbell Exp $
 Custom class for the generic search window.

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
 *  Class Name: SearchWin
 * Description: Handles the search window
 *  Superclass: MUIC_Window
 */

#include "vx_include.h"
#include "SearchWin.h"
#include "CLASS_SearchWin_Defs.h"
#include "vx_arc.h"
#include "vx_virus.h"
#include "vx_mem.h"
#include "vx_io.h"
#include "vx_cfg.h"
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_misc.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *ArcToSearch;
	Object *DoEmbedded;
	Object *Listview;
	Object *List;
	Object *OpenArc;
	Object *ExtFile;
	Object *Exit;
	struct FileRequester *ExtractReq;
	struct Window *WinPtr;
//------------------------------------------------------------------------------
*/

#define LIST_FORMAT_SEARCH "BAR,BAR,"

/***************************************************************************/
/* Search (NList) hooks */
/***************************************************************************/

SAVEDS ASM(void) Search_ListKillFunc(
    REG_A1 (struct NList_DestructMessage *NL_DMsg),
    REG_A2 (Object *Obj) ) {
	register struct SearchResultNode *srn = NL_DMsg->entry;
	if (!srn) {
		return;
	}
	if (srn->srn_ArcPath) {
		MEM_FreeVec(srn->srn_ArcPath);
	}
	if (srn->srn_EntryName) {
		MEM_FreeVec(srn->srn_EntryName);
	}
	MEM_FreeVec(srn);
}

SAVEDS ASM(struct SearchResultNode *) Search_ListMakeFunc(
    REG_A1 (struct NList_ConstructMessage *NL_CMsg),
    REG_A2 (Object *Obj) ) {
	register struct SearchResultNode *srn = NL_CMsg->entry, *newsrn;
	if ((newsrn = MEM_AllocVec(sizeof(struct SearchResultNode)))) {
		newsrn->srn_MatchCnt    = srn->srn_MatchCnt;
		newsrn->srn_EmbeddedArc = srn->srn_EmbeddedArc;
		newsrn->srn_ArcPath     = MEM_StrToVec(srn->srn_ArcPath);
		newsrn->srn_EntryName   = MEM_StrToVec(srn->srn_EntryName);
		if (!newsrn->srn_ArcPath || !newsrn->srn_EntryName) {
			if (newsrn->srn_ArcPath) {
				MEM_FreeVec(newsrn->srn_ArcPath);
			}
			if (newsrn->srn_EntryName) {
				MEM_FreeVec(newsrn->srn_EntryName);
			}
			MEM_FreeVec(newsrn);
			newsrn = NULL;
		}
	}
	return newsrn;
}

SAVEDS ASM(LONG) Search_ListShowFunc(
    REG_A1 (struct NList_DisplayMessage *NL_DMsg),
    REG_A2 (Object *Obj) ) {
	register struct SearchResultNode *srn = NL_DMsg->entry;
	register UBYTE **col = (UBYTE **) NL_DMsg->strings;
	register UBYTE **pp  = (UBYTE **) NL_DMsg->preparses;
	if (srn) {
		if (srn->srn_MatchCnt == 0xffffffff) {
			strcpy(srn->srn_MatchCntStr, "N/A");
		} else {
			sprintf(srn->srn_MatchCntStr, "%lu", srn->srn_MatchCnt);
		}
		/* [0] */ *pp++  = "";
		/* [1] */ *pp++  = "";
		/* [2] */ *pp    = "";
		/* [0] */ *col++ = srn->srn_ArcPath;
		/* [1] */ *col++ = srn->srn_EntryName;
		/* [2] */ *col   = srn->srn_MatchCntStr;
	} else { /* Render titles */
		/* [0] */ *pp++  = "\33b";
		/* [1] */ *pp++  = "\33b";
		/* [2] */ *pp    = "\33b";
		/* [0] */ *col++ = "Archive";
		/* [1] */ *col++ = "File";
		/* [2] */ *col   = "Match count";
	}
	return 0;
}

SAVEDS ASM(LONG) Search_ListSortFunc(
    REG_A1 (struct NList_CompareMessage *NL_CMsg),
    REG_A2 (Object *Obj) ) {
	return 0;
}

struct Hook Search_ListMakeHook = {
	{ NULL, NULL }, (void *) Search_ListMakeFunc, NULL, NULL
};

struct Hook Search_ListKillHook = {
	{ NULL, NULL }, (void *) Search_ListKillFunc, NULL, NULL
};

struct Hook Search_ListShowHook = {
	{ NULL, NULL }, (void *) Search_ListShowFunc, NULL, NULL
};

struct Hook Search_ListSortHook = {
	{ NULL, NULL }, (void *) Search_ListSortFunc, NULL, NULL
};

UBYTE *SearchPages[]       =  { "String search", "Filename search", NULL };
UBYTE *SearchModeEntries[] =
    /* Keep in sync with CYCLE_SEARCHMODE_#? */
    { "Current archive", "Entire archive history list", NULL };

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;
	data.ExtractReq = MUI_AllocAslRequestTags(ASL_FileRequest,
	                  ASLFR_NegativeText,    "Cancel",
	                  //ASLFR_InitialLeftEdge, ILeftEdge,
	                  //ASLFR_InitialTopEdge,  ITopEdge,
	                  //ASLFR_InitialWidth,    IWidth,
	                  //ASLFR_InitialHeight,   IHeight,
	                  ASLFR_InitialDrawer,   "Ram:",
	                  ASLFR_Flags1,          (FRF_DOSAVEMODE),
	                  ASLFR_Flags2,          (FRF_REJECTICONS),
	                  ASLFR_SleepWindow,     TRUE,
	                  TAG_DONE);
	if (!data.ExtractReq) {
		return 0;
	}

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_Title,     "Search window...",
		MUIA_Window_ID,        MAKE_ID('S','E','A','R'),
		WindowContents, VGroup,
			Child, HGroup,
				Child, KeyLabel2("What to search:", 'a'),
				Child, data.ArcToSearch = CycleObject,
					MUIA_CycleChain,    1,
					MUIA_ShortHelp,     HELP_SEARCH_ARCTOSEARCH,
					MUIA_Cycle_Active,  CYCLE_SEARCHMODE_CURRENT,
					MUIA_Font,          MUIV_Font_Button,
					MUIA_Cycle_Entries, SearchModeEntries,
				End,
				Child, ColGroup(2),
					Child, Label1("Search embedded archives?"),
					Child, data.DoEmbedded = MyCheckMark(TRUE, NULL, HELP_SEARCH_SEARCHEMBEDDED_TICK),
				End,
			End,
			Child, data.Listview = NListviewObject,
				MUIA_ShortHelp,			HELP_SEARCH_LISTVIEW,
				MUIA_HorizWeight,		200,
				MUIA_VertWeight,		100,
				MUIA_NListview_NList,	data.List = NListObject,
					MUIA_ObjectID,			 MAKE_ID('S','R','C','H'),
					MUIA_CycleChain,			1,
					MUIA_NList_Input,			TRUE,
					MUIA_NList_MultiSelect,		MUIV_NList_MultiSelect_None,
					MUIA_NList_Format,			LIST_FORMAT_SEARCH,
					MUIA_NList_Active,			MUIV_NList_Active_Top,
					MUIA_NList_Title,			TRUE,
					MUIA_NList_AutoVisible,		TRUE,
					MUIA_NList_ConstructHook2,	&Search_ListMakeHook,
					MUIA_NList_DestructHook2,	&Search_ListKillHook,
					MUIA_NList_DisplayHook2,	&Search_ListShowHook,
					MUIA_NList_CompareHook2,	&Search_ListSortHook,
					MUIA_NList_MinColSortable,	0,
				End,
			End,
			Child, HGroup,
				/*Child, data.OpenArc = MyKeyButton("Open archive",   'o', HELP_SEARCH_OPENARCHIVE),*/
				Child, data.ExtFile = MyKeyButton("Extract file...",'e', HELP_SEARCH_EXTRACTFILE),
			End,
			Child, RegisterGroup(SearchPages),
				MUIA_Register_Frame, TRUE,
				MUIA_VertWeight,     10,
				Child, StringSearchObject,   End, /************ Page 1 ************/
				Child, FileNameSearchObject, End, /************ Page 2 ************/
			End,
			Child, data.Exit = MyKeyButton("Exit", 's', HELP_SEARCH_EXIT),
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		MUI_FreeAslRequest(data.ExtractReq);
		return 0;
	}

	CLOSEWIN_METHOD    (obj,          MUIM_SearchWin_Close)
	/*BUTTON_METHOD      (data.OpenArc, MUIM_SearchWin_OpenArc)*/
	BUTTON_METHOD      (data.ExtFile, MUIM_SearchWin_ExtractFile)
	BUTTON_METHOD      (data.Exit,    MUIM_SearchWin_Close)
	NLIST_DCLICK_METHOD(data.List,    MUIM_SearchWin_DoubleClick)

	PREPDATA;

	return (ULONG) obj;
}

OVERLOAD(OM_DISPOSE)
{
	GETDATA;

	if (data->ExtractReq) {
		MUI_FreeAslRequest(data->ExtractReq);
	}

	return DoSuperMethodA(cl, obj, msg);
}

OVERLOAD(OM_GET)
{
	GETDATA;
	ULONG *store = ((struct opGet *)msg)->opg_Storage;

	switch (((struct opGet *)msg)->opg_AttrID) {
		ATTR(SearchMode): get(data->ArcToSearch, MUIA_Cycle_Active, store); return TRUE;
		ATTR(DoEmbedded): get(data->DoEmbedded, MUIA_Selected, store); return TRUE;
	}

	return DoSuperMethodA(cl, obj, msg);
}

OVERLOAD(MUIM_Setup)
{
	GETDATA;

	if (!DoSuperMethodA(cl, obj, msg)) {
		return FALSE;
	}

	data->WinPtr = (struct Window *) xget(_win(obj), MUIA_Window_Window);

	return TRUE;
}


DECLARE(Open)
{
	set(obj, MUIA_Window_Open, TRUE);

	return 0;
}

DECLARE(Close)
{
	set(obj, MUIA_Window_Open, FALSE);

	return 0;
}

DECLARE(ClearMatches)
{
	GETDATA;
	DoMethod(data->List, MUIM_NList_Clear);

	return 0;
}

DECLARE(RecordMatch) // struct SearchResultNode *srn
{
	GETDATA;
	DoMethod(data->List, MUIM_NList_InsertSingle, msg->srn, MUIV_NList_Insert_Bottom);
	DoMethod(data->List, MUIM_NList_Jump, MUIV_NList_Jump_Bottom);

	return 0;
}

DECLARE(DoubleClick) // LONG sel
{
	GETDATA;
	struct SearchResultNode *srn = NULL;
	struct ArcHistoryNode *embeddedAHN;
	struct ArcHistoryNode *ahn;
	struct ArcEntry *ae;
	Object *arcViewObj = GETARCVIEW;
	Object *arcHistObj = GETARCHISTORY;
	UBYTE *extractName;
	UBYTE *virusName;
	DoMethod(data->List, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, &srn);

	if (!srn) {
		return 0;
	}

	if (!(ahn = ARC_LoadNewArchive(srn->srn_ArcPath, NULL, NULL, FALSE))) {
		return 0;
	}

	if (!(ae = ARC_FindAEInAHN_ViaName(ahn, srn->srn_EntryName))) {
		return 0;
	}

	if (ae->ae_xfi->xfi_Flags & XADFIF_LINK) {
		GUI_PrgError("Sorry, you cannot view link entries!", NULL);
		return 0;
	}

	if (!(extractName = IO_GetTmpLocation(FilePart(ae->ae_FullPath)))) {
		GUI_PrgError("Failed to create temporary file name!", NULL);
		return 0;
	}

	/* Check if the user just clicked on an embedded archive that is
	   already loaded, if so display that entry instead of reloading
	   the archive. */

	if ((embeddedAHN = (struct ArcHistoryNode *) DoMethod(arcHistObj, MUIM_ArcHistory_FindEmbeddedArc, ahn, ae, NULL))) {
		DoMethod(arcViewObj, MUIM_ArcView_ShowArcEntries, embeddedAHN);
		return 0;
	}

	GUI_PrintStatus("Please wait, extracting and examining entry...", 0);

	if (!ARC_ExtractFile(ahn, ae, extractName, TRUE, NULL, "")) {
		return 0;
	}

	if (CFG_Get(TAGID_MAIN_AUTOVIRUSCHECK)) {

		if ((virusName = VIRUS_CheckFile(extractName))) {
			VIRUS_ShowWarning(ahn, ae, virusName);
		}
	}

	if (ARC_IsArc(extractName)) {

		GUI_PrintStatus("Please wait, loading archive from within archive...", 0);
		ARC_LoadNewArchive(extractName, ahn, ae, FALSE);
		DoMethod(arcHistObj, MUIM_ArcHistory_MakeActive, ahn);
		DoMethod(arcViewObj, MUIM_ArcView_ShowArcEntries, ahn);

	} else {

		DoMethod(GETFILETYPESOBJ, MUIM_CfgPageFT_Launch, ae->ae_xfi->xfi_FileName, extractName);
	}

	return 0;
}

DECLARE(ExtractFile)
{
	GETDATA;
	struct SearchResultNode *srn = NULL;
	struct ArcHistoryNode *ahn;
	struct ArcEntry *ae;
	UBYTE pathbuf[256];

	DoMethod(data->List, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, &srn);

	if (!srn) {
		Printf("SearchWin/ExtractFile/No SRN pointer present!\n");
		return 0;
	}

	if (!(ahn = ARC_LoadNewArchive(srn->srn_ArcPath, NULL, NULL, FALSE))) {
		Printf("SearchWin/ExtractFile/ARC_LoadNewArchive failed\n");
		return 0;
	}

	if (!(ae = ARC_FindAEInAHN_ViaName(ahn, srn->srn_EntryName))) {
		Printf("SearchWin/ExtractFile/ARC_FindAEInAHN_ViaName failed\n");
		return 0;
	}

	if (MUI_AslRequestTags(data->ExtractReq,

	  //ASLFR_Window,         data->WinPtr,
        ASLFR_InitialFile,    FilePart(srn->srn_EntryName),
        ASLFR_TitleText,      "Extract file as...",
        ASLFR_RejectIcons,    TRUE,
        ASLFR_DoSaveMode,     TRUE,
        TAG_DONE)) {
		
		strncpy(pathbuf, data->ExtractReq->fr_Drawer, sizeof(pathbuf)-1);

		if (!AddPart(pathbuf, data->ExtractReq->fr_File, sizeof(pathbuf)-1)) {
			return 0;
		}

		if (ARC_ExtractFile(ahn, ae, pathbuf, TRUE, NULL, "")) {
			/* All OK */
		}
	}

	return 0;
}
