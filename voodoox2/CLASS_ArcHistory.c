/*
 $Id: CLASS_ArcHistory.c,v 1.3 2004/01/24 23:07:15 andrewbell Exp $
 The archive history custom class widget.

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
 *  Class Name: ArcHistory
 * Description: Controls the history list
 *  Superclass: MUIC_Group
 */

#include "vx_include.h"
#include "ArcHistory.h"
#include "vx_mem.h"
#include "vx_arc.h"
#include "vx_cfg.h"
#include "vx_misc.h"
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_io.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *ArcPop;
	Object *ArcPopStr;
	Object *ArcPopButton;
	Object *ArcHistListView;
	Object *ArcHistList;
	Object *PrevArc;
	Object *NextArc;
	Object *PopASL;
	Object *PopASLButton;
	Object *RecentMenu;
	UBYTE  ArcAsl_PathBuf[256];
	UBYTE  ArcAsl_NameBuf[256];
	UBYTE  ArcAsl_PatternBuf[256];
	UWORD  ArcAsl_LastTopEdge;
	UWORD  ArcAsl_LastLeftEdge;
	UWORD  ArcAsl_LastWidth;
	UWORD  ArcAsl_LastHeight;
//------------------------------------------------------------------------------
*/

/* EXPORT
//------------------------------------------------------------------------------
 
// This saves some typing
#define GetActiveAHN() \
	((AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN))
 
//------------------------------------------------------------------------------
*/

SAVEDS ASM(BOOL) ArcAslPopup_StartFunc(
    REG_A2 (Object *PopASL),
    REG_A1 (struct TagItem *TL) ) {

	AHN *ahn;
	UBYTE *PathStr;
	struct Data *data = (struct Data *) xget(PopASL, MUIA_UserData);

	if (!data || !TL) {
		return FALSE;
	}

	if ((ahn = GetActiveAHN())) {

		if (ahn->ahn_EmbeddedArc) {
			PathStr = ahn->ahn_ParentArcPath;
		} else {
			PathStr = ahn->ahn_Path;
		}

	} else {
		PathStr = (UBYTE *) CFG_Get(TAGID_MAIN_DEFARCHIVEPATH);
	}

	if (!PathStr) {
		PathStr = "";
	}

	strncpy(data->ArcAsl_NameBuf, FilePart(PathStr),
	        sizeof(data->ArcAsl_NameBuf)-1);

	strncpy(data->ArcAsl_PathBuf, PathStr,
	        sizeof(data->ArcAsl_PathBuf)-1);

	*((UBYTE *)PathPart(data->ArcAsl_PathBuf)) = 0;

	#ifdef ENABLE_MUIASLHACK
	/* This implementation is a bit hacky because we're hacking into MUI's taglist. */
	{
		LONG Cnt;
		for (Cnt = -1; Cnt > -5; Cnt--)
		{
			if (TL[Cnt].ti_Tag == ASLFR_InitialTopEdge)
				TL[Cnt].ti_Data = (ULONG) data->ArcAsl_LastTopEdge;
			if (TL[Cnt].ti_Tag == ASLFR_InitialLeftEdge)
				TL[Cnt].ti_Data = (ULONG) data->ArcAsl_LastLeftEdge;
			if (TL[Cnt].ti_Tag == ASLFR_InitialWidth)
				TL[Cnt].ti_Data = data->ArcAsl_LastWidth;
			if (TL[Cnt].ti_Tag == ASLFR_InitialHeight)
				TL[Cnt].ti_Data = data->ArcAsl_LastHeight;
		}
	}
	#endif /* ENABLE_MUIASLHACK */

	TL[0].ti_Tag  = ASLFR_InitialFile;
	TL[0].ti_Data = (ULONG) &data->ArcAsl_NameBuf;
	TL[1].ti_Tag  = ASLFR_InitialDrawer;
	TL[1].ti_Data = (ULONG) &data->ArcAsl_PathBuf;
	TL[2].ti_Tag  = ASLFR_InitialPattern;
	TL[2].ti_Data = (ULONG) &data->ArcAsl_PatternBuf;
	TL[3].ti_Tag  = ASLFR_DoMultiSelect;
	TL[3].ti_Data = (ULONG) TRUE;
	TL[4].ti_Tag  = ASLFR_InitialHeight;
	TL[4].ti_Data = (ULONG) data->ArcAsl_LastHeight;
	TL[5].ti_Tag = TAG_DONE;
	TL[5].ti_Data = 0;

	return TRUE;
}

SAVEDS ASM(void) ArcAslPopup_StopFunc(
    REG_A2 (Object *PopASL),
    REG_A1 (struct FileRequester *FR) ) {

	struct Data *data = (struct Data *) xget(PopASL, MUIA_UserData);

	if (!data || !FR) {
		return;
	}

	strncpy(data->ArcAsl_PatternBuf, FR->fr_Pattern,
	        sizeof(data->ArcAsl_PatternBuf)-1);

	data->ArcAsl_LastLeftEdge = FR->fr_LeftEdge;
	data->ArcAsl_LastTopEdge  = FR->fr_TopEdge;
	data->ArcAsl_LastWidth    = FR->fr_Width;
	data->ArcAsl_LastHeight   = FR->fr_Height;

	ARC_LoadArcsFromWBArgs(FR->fr_ArgList, FR->fr_NumArgs, TRUE);
}

SAVEDS ASM(void) ArcAslPopup_AppendEntryFunc(
    REG_A2 (Object *TrigObj),
    REG_A1 (ULONG *Param)) {

	UBYTE *expanded_pathstr, *pathstr;
	struct Data *data;

	if (!(data = (struct Data *) xget(TrigObj, MUIA_UserData))) {
		return;
	}

	if (!(pathstr = (UBYTE *) xget(data->ArcPopStr, MUIA_String_Contents))) {
		return;
	}

	/* If the user typed in some shorthand embedded archive pointers
	   (i.e "::"), then expanded them out to " :--> ". */

	if ((expanded_pathstr = ARC_ExpandPathComponents(pathstr))) {

		set(data->ArcPopStr, MUIA_String_Contents, expanded_pathstr);
		ARC_LoadNewArchive(expanded_pathstr, NULL, NULL, FALSE);
		MEM_FreeVec(expanded_pathstr);

	} else {

		ARC_LoadNewArchive(pathstr, NULL, NULL, FALSE);
	}

	DoMethod(GETARCVIEW, MUIM_ArcView_ShowArcEntries, NULL);
}

struct Hook ArcAslPopup_StartHook =
	{ { NULL, NULL }, (void *) ArcAslPopup_StartFunc, NULL, NULL };

struct Hook ArcAslPopup_StopHook =
	{ { NULL, NULL },(void *) ArcAslPopup_StopFunc,  NULL, NULL };

struct Hook ArcAslPopup_AppendEntryHook = {
	{ NULL, NULL },(void *) ArcAslPopup_AppendEntryFunc, NULL, NULL };

//----------------------------------------------------------------------------
// History list & popup hooks
//----------------------------------------------------------------------------

SAVEDS ASM(void) ArcHist_ListKillFunc(
    REG_A2 (APTR Pool),
    REG_A1 (AHN *ahn) ) {

	DB(ULONG tmp = (ULONG) ahn)
	DB(dbg("ARC_ArcHist_ListKillFunc: About to free AHN %lx...", &tmp))

	ARC_FreeAHN(ahn);
}

SAVEDS ASM(AHN *) ArcHist_ListMakeFunc(
    REG_A2 (APTR Pool),
    REG_A1 (AHN *ahn) ) {

	return ahn;
}

SAVEDS ASM(LONG) ArcHist_ListShowFunc(
    REG_A2 (UBYTE **ColumnArray),
    REG_A1 (AHN *ahn) ) {

	UBYTE *vec;

	if (!ahn) {
		return 0;
	}

	if (ahn->ahn_EmbeddedArc) {

		ColumnArray[0] = ahn->ahn_EmbeddedListStr;

	} else {

		ColumnArray[0] = ahn->ahn_Path;
	}

	ColumnArray[1] = ahn->ahn_SizeStr;

	if (!ahn->ahn_SizeStr[0]) {

		if ((vec = FormatULONGToVec(ahn->ahn_FIB->fib_Size))) {

			strncpy(ahn->ahn_SizeStr, vec, sizeof(ahn->ahn_SizeStr)-1);
			MEM_FreeVec(vec);
		}
	}

	return 0;
}

SAVEDS ASM(void) ArcHist_ObjStrFunc(
    REG_A2 (Object *PopupList),
    REG_A1 (Object *StrObj) ) {
	AHN *ahn = NULL;

	struct Data *data = NULL;

	get(PopupList, MUIA_UserData, &data);

	if (!data) {
		return;
	}

	DoMethod(data->ArcHistListView, MUIM_NList_GetEntry,
	         MUIV_NList_GetEntry_Active, &ahn);

	if (!ahn) {
		return;
	}

	DoMethod(GETARCVIEW, MUIM_ArcView_ShowArcEntries, ahn);
}

SAVEDS ASM(BOOL) ArcHist_StrObjFunc(
    REG_A2 (Object *PopupList),
    REG_A1 (Object *StrObj) ) {

	UBYTE *PathStr = NULL;
	register ULONG i;
	AHN *ahn = NULL;
	struct Data *data = NULL;

	get(PopupList, MUIA_UserData, &data);

	if (!data) {
		return FALSE;
	}

	get(data->ArcPopStr, MUIA_String_Contents, &PathStr);

	if (!PathStr) {
		return FALSE;
	}

	for (i = 0;; i++) {
		DoMethod(data->ArcHistListView, MUIM_NList_GetEntry, i, &ahn);

		if (!ahn) {

			set(data->ArcHistListView, MUIA_NList_Active,
				MUIV_NList_Active_Off);
			break;

		} else if (ahn->ahn_EmbeddedArc &&
				Stricmp(ahn->ahn_EmbeddedListStr, PathStr) == 0) {

			set(data->ArcHistListView, MUIA_NList_Active, i);
			break;

		} else if (Stricmp(ahn->ahn_Path, PathStr) == 0) {

			set(data->ArcHistListView, MUIA_NList_Active, i);
			break;

		}
	}

	return TRUE;
}

struct Hook ArcHist_MakeHook = {
	{ NULL, NULL } , (void *) ArcHist_ListMakeFunc, NULL, NULL
};


struct Hook ArcHist_KillHook = {
	{ NULL, NULL }, (void *) ArcHist_ListKillFunc, NULL, NULL
};


struct Hook ArcHist_ShowHook = {
	{ NULL, NULL }, (void *) ArcHist_ListShowFunc, NULL, NULL
};

struct Hook ArcHist_ObjStrHook = {
	{  NULL, NULL  }, (void *) ArcHist_ObjStrFunc,   NULL, NULL
};

struct Hook ArcHist_StrObjHook = {
	{ NULL, NULL }, (void *) ArcHist_StrObjFunc, NULL, NULL
};

//----------------------------------------------------------------------------

OVERLOAD(OM_NEW)
{
	struct TagItem *tags = inittags(msg), *tag;

	DEFTMPDATA;
	CLRTMPDATA;

	while ((tag = NextTagItem(&tags))) {
		switch(tag->ti_Tag) {
			ATTR(RecentMenu): data.RecentMenu = (Object *) tag->ti_Data;
			break;
		}
	}

	obj = (Object *) DoSuperNew(cl, obj,
		Child, HGroup,
			Child, KeyLabel("Archive:", 'A'),
			Child, data.ArcPop = PopobjectObject,
				MUIA_ShortHelp,        HELP_MAIN_ARCPOP,
				MUIA_Popstring_String,
				data.ArcPopStr = StringObject,
					StringFrame,
					MUIA_HorizWeight,     70,
					MUIA_ShortHelp,       HELP_MAIN_ARCPOPSTR,
					MUIA_ControlChar,     'a',
					MUIA_String_MaxLen,   512,
					MUIA_String_Contents, "",
					MUIA_CycleChain,      1,
				End,
				MUIA_Popstring_Button,    	data.ArcPopButton = PopButton(MUII_PopUp),
					MUIA_Popobject_StrObjHook,	&ArcHist_StrObjHook,
					MUIA_Popobject_ObjStrHook,	&ArcHist_ObjStrHook,
					MUIA_Popobject_Object,		data.ArcHistListView = NListviewObject,
						MUIA_NListview_NList,	data.ArcHistList = NListObject,
						InputListFrame,
						MUIA_CycleChain,			1,
						MUIA_MaxHeight,     	    150,
						MUIA_NList_Title,           FALSE,
						MUIA_NList_Format,          "W=90 BAR,W=10",
						MUIA_NList_AutoVisible, 	TRUE,
						MUIA_NList_ConstructHook,	&ArcHist_MakeHook,
						MUIA_NList_DestructHook,	&ArcHist_KillHook,
						MUIA_NList_DisplayHook,		&ArcHist_ShowHook,
					End,
				End,
			End,
			Child, HGroup,
				MUIA_Group_HorizSpacing, 0,
				Child, data.PrevArc = ImageButton(MUII_ArrowLeft,  '-', HELP_MAIN_PREVARC),
				Child, data.NextArc = ImageButton(MUII_ArrowRight, '+', HELP_MAIN_NEXTARC),
			End,
			Child, data.PopASL = PopaslObject,
				MUIA_ShortHelp,        HELP_MAIN_POPASL,
				MUIA_Popstring_Button, data.PopASLButton = PopButton(MUII_PopFile),
				MUIA_Popasl_StartHook, &ArcAslPopup_StartHook,
				MUIA_Popasl_StopHook,  &ArcAslPopup_StopHook,
				MUIA_Popasl_Type,      ASL_FileRequest,
				ASLFR_RejectIcons,     TRUE,
				ASLFR_DoPatterns,      TRUE,
				ASLFR_InitialDrawer,   CFG_Get(TAGID_MAIN_DEFARCHIVEPATH),
				ASLFR_TitleText,       "Please select one or more archives...",
			End,
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	DoMethod(data.ArcPopStr, MUIM_Notify,
	         MUIA_String_Acknowledge, MUIV_EveryTime, data.ArcPopStr,
	         3, MUIM_CallHook, &ArcAslPopup_AppendEntryHook, MUIV_TriggerValue);

	DoMethod(data.ArcHistList, MUIM_Notify, MUIA_NList_DoubleClick, TRUE, data.ArcHistList,
	         2, MUIM_Popstring_Close, TRUE);

	set(data.ArcPopButton, MUIA_CycleChain, 1);
	set(data.PopASLButton, MUIA_CycleChain, 1);

	BUTTON_METHOD(data.PrevArc, MUIM_ArcHistory_PrevArc)
	BUTTON_METHOD(data.NextArc, MUIM_ArcHistory_NextArc)
	NLIST_DCLICK_METHOD(data.ArcHistList, MUIM_ArcHistory_ListSingleClick)
	NLIST_SCLICK_METHOD(data.ArcHistList, MUIM_ArcHistory_ListDoubleClick)

	strncpy(data.ArcAsl_PatternBuf, "#?", sizeof(data.ArcAsl_PatternBuf)-1);

	GUI_GetInitialAslDims(
	    &data.ArcAsl_LastLeftEdge, &data.ArcAsl_LastTopEdge,
	    &data.ArcAsl_LastWidth,    &data.ArcAsl_LastHeight);

	set(data.ArcPop,          MUIA_UserData, INST_DATA(cl, obj));
	set(data.ArcPopStr,       MUIA_UserData, INST_DATA(cl, obj));
	set(data.ArcHistList,     MUIA_UserData, INST_DATA(cl, obj));
	set(data.ArcHistListView, MUIA_UserData, INST_DATA(cl, obj));
	set(data.PopASL,          MUIA_UserData, INST_DATA(cl, obj));
	set(data.ArcPopButton,    MUIA_UserData, INST_DATA(cl, obj));

	PREPDATA;

	return (ULONG) obj;
}

OVERLOAD(MUIM_Setup)
{
	if (!DoSuperMethodA(cl, obj, msg)) {
		return FALSE;
	}

	set(_win(obj), MUIA_Window_ActiveObject, DATAREF->ArcPopStr);

	return TRUE;
}

OVERLOAD(OM_SET)
{
	GETDATA;

	struct TagItem *tags = inittags(msg), *tag;

	while((tag = NextTagItem(&tags))) {

		switch(tag->ti_Tag) {

			ATTR(RecentMenu): data->RecentMenu = (Object *) tag->ti_Data;
			break;
		}
	}

	return DoSuperMethodA(cl, obj, msg);
}

OVERLOAD(OM_GET)
{
	GETDATA;

	ULONG *store = ((struct opGet *)msg)->opg_Storage;

	switch (((struct opGet *)msg)->opg_AttrID) {
		// Amount of loaded archives // was GetCnt
		ATTR(Count):
			get(data->ArcHistList, MUIA_NList_Entries, store);
		break;

		// Return the active AHN
		ATTR(ActiveAHN):
			DoMethod(data->ArcHistList, MUIM_NList_GetEntry,
			         MUIV_NList_GetEntry_Active, store);
		break;
	}

	return DoSuperMethodA(cl, obj, msg);
}


DECLARE(GetNext) // LONG index, AHN **ahn_ptr
{
	GETDATA; /* If index == -1 then begin */
	AHN *ahn;

	DB(dbg("ArcHistory/GetNext called (UNTESTED!!!!)"));

	DoMethod(data->ArcHistList, MUIM_NList_GetEntry,
	         msg->index == -1 ? ++(msg->index) : msg->index, &ahn);

	*msg->ahn_ptr = ahn;

	/* Will return -1 when finished. */

	return ahn ? (ULONG) 1 + msg->index : (ULONG) -1;
}


LONG GetIndex( struct Data *data, AHN *findahn ) {
	AHN *ahn;
	LONG i;

	if (!findahn) {
		return -1;
	}

	for (i = 0;; i++) {
		DoMethod(data->ArcHistList, MUIM_NList_GetEntry, i, &ahn);

		if (!ahn) {
			break;
		} else if (ahn == findahn) {
			return i;
		}
	}

	return -1; /* Return -1 if AHN doesn't exist. */
}

DECLARE(Flush)
{
	/* Tries to flush any resource that isn't essential... */

	GETDATA;
	PN *pn, *cur_pn;
	AHN *ahn, *cur_ahn;
	ULONG i;

	if (!(cur_ahn = (AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN))) {
		return 0;
	}

	cur_pn = cur_ahn->ahn_CurrentPN;

	for (i = 0;;) {
		DoMethod(data->ArcHistList, MUIM_NList_GetEntry, i, &ahn);

		if (!ahn) {
			break;
		}

		for (pn = (PN *) ahn->ahn_PNList.mlh_Head;
		     pn->pn_node.mln_Succ;
		     pn = (PN *) pn->pn_node.mln_Succ) {

			if (pn != cur_pn) {
				ARC_UnprepPN(ahn, pn);
			}
		}
	}

	return 0;
}

DECLARE(ListSingleClick)
{
	return 0;
}

DECLARE(ListDoubleClick)
{

	DoMethod(GETARCVIEW, MUIM_ArcView_ShowArcEntries,
	         xget(obj, MUIA_ArcHistory_ActiveAHN));

	// TODO: hide popup?

	return 0;
}

DECLARE(PrevArc)
{
	GETDATA;
	
	set(data->ArcHistListView, MUIA_NList_Active, MUIV_NList_Active_Up);

	return 0;
}

DECLARE(NextArc)
{
	GETDATA;

	set(data->ArcHistListView, MUIA_NList_Active, MUIV_NList_Active_Down);

	return 0;
}

static AE *GetAEViaXFI( PN *pn, struct xadFileInfo *xfi ) // Move this to v_arc.c
{
	AE **ae_ptr;

	if (!pn || !xfi) {
		return NULL;
	}

	for (ae_ptr = pn->pn_linear_aes_xad; *ae_ptr; ae_ptr++) {

		if ((*ae_ptr)->ae_xfi == xfi) {
			return *ae_ptr;
		}
	}

	return NULL;
}


DECLARE(Load) // STRPTR arcpath, AHN *parent_ahn, struct ArcEntry *embedded_ae, struct xadSplitFile *xad_sf, LONG MultiMode
{
	GETDATA;
	AHN *ahn;

	// TODO: add extra arg so user can control if arcview should be updated
	// TODO: rename embedded_ae to parent_ae

	DB(dbg("ArcHistory::Load(arcpath=%s, parent_ahn=%lx, embedded_ae=%lx,"
	       " xad_sf=%lx, MultiMode=%s)\n", msg->arcpath, (LONG) msg->parent_ahn,
	       (LONG) msg->embedded_ae, (LONG) msg->xad_sf, msg->MultiMode ? "Yes" : "No"))

	ahn = ARC_CreateAHN(msg->arcpath, msg->parent_ahn, msg->embedded_ae, msg->xad_sf);

	if (!ahn) {
		return FALSE;
	}

	if (data->RecentMenu) {
		DoMethod(data->RecentMenu, MUIM_RecentMenu_Add, msg->arcpath);
	}

	DoMethod(data->ArcHistList, MUIM_NList_InsertSingle,
	         ahn, MUIV_NList_Insert_Top);

	nnset(data->ArcHistList, MUIA_NList_Active, MUIV_NList_Active_Top);

	/* This is the magic that makes VX automatically enter into
	   UNIX archives (TAR.GZ, etc). */

	if (CFG_Get(TAGID_MAIN_AUTOENTERTARS) &&
	        !msg->MultiMode && (ahn->ahn_CurrentPN && ahn->ahn_CurrentPN->pn_ai &&
	                            ahn->ahn_CurrentPN->pn_ai->xai_FileInfo->xfi_Next == NULL)) {

		ULONG arcpathlen = strlen(msg->arcpath), i;
		UBYTE *subname = ahn->ahn_CurrentPN->pn_ai->xai_FileInfo->xfi_FileName;
		ULONG subnamelen = strlen(subname);

		static UBYTE *list[] =
		    {
		        ".rpm",    ".cpio",
		        ".tar.gz", ".tar",
		        ".tgz",    ".tar",
		        ".bz2",    ".tar",
		        NULL,      NULL,
		    };

		for (i = 0; list[i]; i += 2) {
			ULONG suffixlen = strlen(list[i]);
			ULONG subsuffixlen = strlen(list[i+1]);

			if ((arcpathlen >= suffixlen &&
			        !Stricmp(&msg->arcpath[arcpathlen-suffixlen], list[i])) &&
			        (subnamelen >= subsuffixlen &&
			         !Stricmp(&subname[subnamelen-subsuffixlen], list[i+1]))) {

				if (ARC_PrepPN(ahn, ahn->ahn_CurrentPN)) {
					DoMethod(GETARCVIEW, MUIM_ArcView_DoDoubleClick, ahn,
					         GetAEViaXFI(ahn->ahn_CurrentPN,
					                     ahn->ahn_CurrentPN->pn_ai->xai_FileInfo));

					break;
				}
			}
		}
	}

	return TRUE;
}

DECLARE(SetQuiet) // LONG state
{
	GETDATA;
	set(data->ArcHistList, MUIA_NList_Quiet, msg->state);
	return 0;
}

DECLARE(SetArcPath) // STRPTR path
{
	GETDATA;
	set(data->ArcPopStr, MUIA_String_Contents, msg->path);
	return 0;
}

DECLARE(RedrawAll)
{
	return DoMethod(DATAREF->ArcHistList, MUIM_NList_Redraw, MUIV_NList_Redraw_All);
}

DECLARE(RemoveAHN) // AHN *ahn
{
	GETDATA;
	LONG i;
	AHN *ahn = msg->ahn;

	// If no AHN was given, then get the current one.

	if (!ahn) {

		DoMethod(data->ArcHistList, MUIM_NList_GetEntry,
		         MUIV_NList_GetEntry_Active, &ahn);

		if (!ahn) {
			return 0;
		}
	}

	if ((i = GetIndex(data, ahn)) == -1) {
		return 0;
	}

	//set(data->ArcHistListView, MUIA_NList_Active, MUIV_NList_Active_Down);

	//
	// If this AHN is currently being displayed in the ArcView then
	// wipe the ArcView.
	//

	//	if (((AHN *)xget(GETARCVIEW, MUIA_ArcView_AHN)) == ahn)
	//{
	//	DoMethod(GETARCVIEW, MUIM_ArcView_ShowArcEntries, NULL);
	//}

	set(data->ArcHistList, MUIA_NList_Quiet, TRUE);
	DoMethod(data->ArcHistList, MUIM_NList_Remove, i);
	set(data->ArcHistList, MUIA_NList_Quiet, FALSE);

	// No more opened archives? Then blank the display...

	if (xget(data->ArcHistList, MUIA_NList_Entries) == 0) {

		DoMethod(GETARCVIEW, MUIM_ArcView_ShowArcEntries, NULL);

	} else {

		DoMethod(GETARCVIEW, MUIM_ArcView_ShowArcEntries,
		         xget(obj, MUIA_ArcHistory_ActiveAHN));
	}

	return 0;
}

DECLARE(CloseAll) // AHN *skip_ahn
{
	GETDATA;
	AHN *ahn;
	ULONG closecnt, i;

	for (closecnt = 0, i = 0 ;; i++) {
		DoMethod(data->ArcHistList, MUIM_NList_GetEntry, i, &ahn);

		if (!ahn) {
			break;
		}

		if (ahn == msg->skip_ahn) {
			continue;
		}

		DoMethod(data->ArcHistList, MUIM_NList_Remove, --i);

		closecnt++;
	}

	if (!xget(data->ArcHistList, MUIA_NList_Entries)) {
		DoMethod(GETARCVIEW, MUIM_ArcView_ShowArcEntries, NULL);
	}

	return closecnt;
}

DECLARE(FindArc) // BPTR NewArcLock, LONG *IndexPtr
{
	// Try to determine if the an archive is already in the history
	// list, via its lock. NULL will be returned if it's not, else
	// a pointer to the already existing ArcHistoryNode. You can also
	// retrieve its list index position with the IndexPtr parameter.
	// IndexPtr may be set to NULL if you don't want it.

	GETDATA;
	AHN *ahn = NULL;
	LONG i;

	DB(dbg("ArcHistory::FindArc(lock=0x%08lx, index_ptr=0x%08lx)",
	       msg->NewArcLock, msg->IndexPtr))

	if (msg->IndexPtr) {
		*(msg->IndexPtr) = 0;
	}

	for (i = 0;; i++) {
		DoMethod(data->ArcHistList, MUIM_NList_GetEntry, i, &ahn);

		if (!ahn) {
			break;
		}

		if (SameLock(ahn->ahn_ArcLock, msg->NewArcLock) == LOCK_SAME) {
			if (msg->IndexPtr) {
				*(msg->IndexPtr) = i;
			}

			return (ULONG) ahn;
		}
	}

	return 0;
}

DECLARE(FindEmbeddedArc) // AHN *Parentahn, struct ArcEntry *EmbeddedAE, LONG *IndexPtr
{
	GETDATA;
	AHN *ahn = NULL;
	LONG i;

	DB(dbg("ArcHistory::FindEmbeddedArc(parent_ahn=0x%08lx, "
	       "embedded_ae=0x%08lx, index_ptr=0x%08lx)",
	       msg->Parentahn, msg->EmbeddedAE, msg->IndexPtr))

	if (!msg->Parentahn || !msg->EmbeddedAE) {
		return 0;
	}

	if (msg->IndexPtr) {
		*(msg->IndexPtr) = 0;
	}

	for (i = 0;; i++) {
		DoMethod(data->ArcHistList, MUIM_NList_GetEntry, i, &ahn);

		if (!ahn) {

			break;

		} else if (ahn->ahn_EmbeddedArc &&
		           IO_SameFile(ahn->ahn_ParentArcPath, msg->Parentahn->ahn_Path) &&
		           (Stricmp(ahn->ahn_EmbeddedPath, msg->EmbeddedAE->ae_FullPath) == 0)) {

			if (msg->IndexPtr) {
				*(msg->IndexPtr) = i;
			}

			return (ULONG) ahn;
		}
	}
	return 0;
}

DECLARE(MakeActive) // AHN *findahn
{
	GETDATA;
	AHN *ahn;
	LONG i;

	for (i = 0;; i++) {
		DoMethod(data->ArcHistList, MUIM_NList_GetEntry, i, &ahn);

		if (!ahn) {
			break;
		}

		if (ahn == msg->findahn) {

			set(data->ArcHistList, MUIA_NList_Active, i);
			break;
		}
	}

	return 0;
}
