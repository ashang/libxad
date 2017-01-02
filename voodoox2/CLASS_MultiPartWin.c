/*
 $Id: CLASS_MultiPartWin.c,v 1.2 2004/01/21 18:19:17 andrewbell Exp $
 Custom class for creating the multi-part window.

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
 *  Class Name: MultiPartWin
 * Description: The window that loads multiple portioned archives.
 *  Superclass: MUIC_Window
 */

#include "vx_include.h"
#include "MultiPartWin.h"
#include "vx_mem.h"
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_misc.h"
#include "vx_arc.h"
#include "vx_cfg.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *Info;
	Object *Listview;
	Object *List;
	Object *View;
	Object *Add;
	Object *Rem;
	Object *Cancel;
 
	struct FileRequester *OpenReq;
//------------------------------------------------------------------------------
*/

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;

	/* This requester is used for the "Open..." menu item. */
	data.OpenReq = MUI_AllocAslRequestTags(ASL_FileRequest,
		ASLFR_NegativeText,    "Cancel",
		//ASLFR_InitialLeftEdge, ILeftEdge,
		//ASLFR_InitialTopEdge,  ITopEdge,
		//ASLFR_InitialWidth,    IWidth,
		//ASLFR_InitialHeight,   IHeight,
		ASLFR_InitialDrawer,   CFG_Get(TAGID_MAIN_DEFARCHIVEPATH),
		ASLFR_Flags2,          (FRF_REJECTICONS),
		ASLFR_SleepWindow,     TRUE,
		TAG_DONE
	);

	if (!data.OpenReq) {
		return 0;
	}

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_Title, "Load multipart archive...",
		MUIA_Window_ID,    MAKE_ID('M','U','L','T'),
		WindowContents,    VGroup,
			Child, data.Info = TextObject,
				TextFrame,
				MUIA_ShortHelp,     HELP_MP_INFO,
				MUIA_Background,    MUII_TextBack,
				MUIA_Text_PreParse, "\33c",
				MUIA_Text_Contents,
					"Please add each archive segment to this list,\n"
					"then press the \"View\" button when ready.\n"
					"\n"
					"Note: Most XAD clients require each archive\n"
					"segment to be in the correct order. Use this\n"
					"drag and drop lister to arrange that order.",
				End,
			Child, data.Listview = NListviewObject,
				MUIA_ShortHelp,       HELP_MP_LISTVIEW,
				MUIA_CycleChain,      1,
				MUIA_NListview_NList, data.List = NListObject,
					ReadListFrame,
					MUIA_NList_Input,			TRUE,
					MUIA_NList_Active,			MUIV_List_Active_Top,
					MUIA_NList_Format,			"",
					MUIA_NList_Title,			FALSE,
					MUIA_NList_DragSortable,	TRUE,
					MUIA_NList_ConstructHook,	MUIV_NList_ConstructHook_String,
					MUIA_NList_DestructHook,	MUIV_NList_DestructHook_String,
				End,
			End,
			Child, HGroup,
				Child, data.View   = MyKeyButton("View",   'v', HELP_MP_VIEW),
				Child, data.Add    = MyKeyButton("Add",    'a', HELP_MP_ADD),
				Child, data.Rem    = MyKeyButton("Remove", 'r', HELP_MP_REMOVE),
				Child, data.Cancel = MyKeyButton("Cancel", 'c', HELP_MP_CANCEL),
			End,
			/*
			Child, HGroup,
				Child, data.LoadList   = MyKeyButton("Load List",   'v', HELP_MP_VIEW),
				Child, data.SaveList   = MyKeyButton("Save List",   'a', HELP_MP_ADD),
			End,
			*/
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	CLOSEWIN_METHOD(obj,         MUIM_MultiPartWin_CloseWindow)
	BUTTON_METHOD  (data.Cancel, MUIM_MultiPartWin_Cancel)
	BUTTON_METHOD  (data.View,   MUIM_MultiPartWin_View)
	BUTTON_METHOD  (data.Add,    MUIM_MultiPartWin_Add)
	BUTTON_METHOD  (data.Rem,    MUIM_MultiPartWin_Remove)

	PREPDATA;

	return (ULONG) obj;
}

OVERLOAD(OM_DISPOSE)
{
	GETDATA;

	if (data->OpenReq) {
		MUI_FreeAslRequest(data->OpenReq);
	}

	return DoSuperMethodA(cl, obj, msg);
}

DECLARE(CloseWindow)
{
	set(obj, MUIA_Window_Open, FALSE);

	return 0;
}

DECLARE(Cancel)
{
	set(obj, MUIA_Window_Open, FALSE);

	return 0;
}

// TODO: Add options to save and load lists
// #?.xadmp. header=XADMULTIPARTLIST

DECLARE(View)
{

	GETDATA;
	UBYTE *NextSegPath = NULL;
	long i = 0;
	BOOL LoopErr = FALSE;
	struct xadSplitFile *FirstxadSF = NULL, *xadSF,
		*LastxadSF  = NULL, *TmpxadSF;

	while (!LoopErr) {
		DoMethod(data->List, MUIM_NList_GetEntry, i++, &NextSegPath);

		if (!NextSegPath) {
			break;
		}

		if ((xadSF = MEM_AllocVec(sizeof(struct xadSplitFile)))) {
			if (!FirstxadSF) {
				FirstxadSF = xadSF;
			}

			xadSF->xsf_Next = NULL;
			xadSF->xsf_Type = XAD_INFILENAME;
			xadSF->xsf_Size = 0;

			if ((xadSF->xsf_Data = (ULONG) MEM_StrToVec(NextSegPath))) {
				if (LastxadSF) {
					LastxadSF->xsf_Next = xadSF;
				}

			} else {

				LoopErr = TRUE;
			}

			LastxadSF = xadSF;

		} else {

			LoopErr = TRUE;
		}
	}

	if (!LoopErr) {
		/* TODO: Make sure the multi-part archive is not already loaded. */

		if (DoMethod(GETARCHISTORY, MUIM_ArcHistory_Load, NULL, NULL, NULL, FirstxadSF, FALSE)) {

			DoMethod(data->List, MUIM_NList_Clear);
			set(obj, MUIA_Window_Open, FALSE);
			DoMethod(GETARCVIEW, MUIM_ArcView_ShowArcEntries, NULL);
		}
	}

	if ((xadSF = FirstxadSF)) {
		do {

			TmpxadSF = xadSF->xsf_Next;
			MEM_FreeVec((APTR)xadSF->xsf_Data);
			MEM_FreeVec(xadSF);

		} while ((xadSF = TmpxadSF));
	}

	return 0;
}

DECLARE(Add)
{
	GETDATA;
	UBYTE PathBuf[256];
	struct Window *WndPtr = NULL;
	get(obj, MUIA_Window_Window, &WndPtr);

	if (MUI_AslRequestTags(data->OpenReq,
	                       ASLFR_Window,        WndPtr,
	                       ASLFR_DoMultiSelect, TRUE,
	                       ASLFR_RejectIcons,   TRUE,
	                       ASLFR_DoPatterns,    TRUE,
	                       ASLFR_TitleText,     "Please select one or more archive segments...",
	                       TAG_DONE)) {

		struct WBArg *WBA = data->OpenReq->fr_ArgList;
		ULONG cnt = data->OpenReq->fr_NumArgs;
		set(data->Listview, MUIA_NList_Quiet, TRUE);

		while(cnt--) {
			strncpy(PathBuf, data->OpenReq->fr_Drawer, sizeof(PathBuf)-1);

			if (AddPart(PathBuf, WBA->wa_Name, sizeof(PathBuf)-1)) {
				/* TODO: Make sure entry doesn't already exist in list. */
				DoMethod(data->Listview, MUIM_NList_InsertSingle, PathBuf, MUIV_NList_Insert_Bottom);
			}

			WBA++;
		}

		set(data->Listview, MUIA_NList_Quiet, FALSE);
	}

	return 0;
}

DECLARE(Remove)
{
	GETDATA;
	DoMethod(data->Listview, MUIM_NList_Remove, MUIV_NList_Remove_Active);

	return 0 ;
}
