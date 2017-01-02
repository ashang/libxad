/*
 $Id: CLASS_avmFilesAndDirs.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class for Files And Dirs archive view mode (AVM).

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
 *  Class Name: ArcView
 * Description: Controls the archive view
 *  Superclass: MUIC_Group
 */

#include "vx_include.h"
#include "avmFilesAndDirs.h"
#include "vx_arc.h"
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_misc.h"
#include "vx_cfg.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	LONG VM;
	Object *VMObj;
	Object *AEList;
	Object *AEListview;
	Object *NarrowParent;
	Object *Root;
	Object *Parent;
	Object *ArcLocation;
	Object *ExplorerObj;
	ULONG viewmask;
	AHN *ahn;
	UBYTE buf[256]; // General purpose buffer for any temporary str operations.
//------------------------------------------------------------------------------
*/

#define LIST_FORMAT_ARCENTRIES    "BAR MIW=10,BAR,,,BAR,BAR,MIW=10"

SAVEDS ASM(void) ListKillFunc(
    REG_A1 (struct NList_DestructMessage *NL_DMsg),
    REG_A2 (Object *Obj) ) {}

SAVEDS ASM(struct ArcEntry *) ListMakeFunc(
    REG_A1 (struct NList_ConstructMessage *NL_CMsg),
    REG_A2 (Object *Obj) ) {
	return NL_CMsg->entry; /* Just pass it back :) */
}

SAVEDS ASM(LONG) ListShowFunc(
    REG_A1 (struct NList_DisplayMessage *NL_DMsg),
    REG_A2 (Object *Obj) ) {
	ArcView_FillHookArrays(
	    (UBYTE **) NL_DMsg->strings,
	    (UBYTE **) NL_DMsg->preparses,
	    (AE *) NL_DMsg->entry, FALSE);

	return 0;
}

SAVEDS ASM(LONG) ListSortFunc(
    REG_A1 (struct NList_CompareMessage *NL_CMsg),
    REG_A2 (Object *Obj) ) {
	struct ArcEntry *AE1 = (struct ArcEntry *) NL_CMsg->entry1;
	struct ArcEntry *AE2 = (struct ArcEntry *) NL_CMsg->entry2;

	LONG Col1     = NL_CMsg->sort_type & MUIV_NList_TitleMark_ColMask;
	LONG SortType = NL_CMsg->sort_type;
	LONG Result   = 0;

	UBYTE *Comment1 = "", *Comment2 = "";
	UBYTE *ei1 = "", *ei2 = "";
	UBYTE *suffix1 = "", *suffix2 = "";

	if (SortType == MUIV_NList_SortType_None) {
		return 0;
	}

	switch (Col1) {

	case 0: /* Name */
		if (SortType & MUIV_NList_TitleMark_TypeMask) {
			Result = Stricmp(AE2->ae_Name, AE1->ae_Name); /* Title arrow is up */
		} else {
			Result = Stricmp(AE1->ae_Name, AE2->ae_Name); /* Title arrow is down */
		}
		break;


	case 1: /* Size */
		/* Note: Directory entries MUST always have ae_Size to -1. */

		if (SortType & MUIV_NList_TitleMark_TypeMask) {
			Result = AE2->ae_Size - AE1->ae_Size;
		} else {
			Result = AE1->ae_Size - AE2->ae_Size;
		}
		break;


	case 2: /* Day */
	case 3: /* Date */
	case 4: /* Time */
		if (SortType & MUIV_NList_TitleMark_TypeMask) {
			Result = CompareDates(&AE1->ae_DS, &AE2->ae_DS);
		} else {
			Result = CompareDates(&AE2->ae_DS, &AE1->ae_DS);
		}
		break;


	case 5:	/* Protection */
	{
		UBYTE *pv1 = AE1->ae_ProtBuf;
		UBYTE *pv2 = AE2->ae_ProtBuf;

		if (SortType & MUIV_NList_TitleMark_TypeMask) {
			Result = Stricmp(pv1, pv2);
		} else {
			Result = Stricmp(pv2, pv1);
		}

		break;
	}

	case 6:	/* Comment */
		if (AE1->ae_xfi && AE1->ae_xfi->xfi_Comment) {
			Comment1 = AE1->ae_xfi->xfi_Comment;
		}

		if (AE2->ae_xfi && AE2->ae_xfi->xfi_Comment) {
			Comment2 = AE2->ae_xfi->xfi_Comment;
		}

		if (SortType & MUIV_NList_TitleMark_TypeMask) {
			Result = Stricmp(Comment1, Comment2);
		} else {
			Result = Stricmp(Comment2, Comment1);
		}

		break;


	case 7: /* EntryInfo */
		if (AE1->ae_xfi && AE1->ae_xfi->xfi_EntryInfo) {
			ei1 = AE1->ae_xfi->xfi_EntryInfo;
		}

		if (AE2->ae_xfi && AE2->ae_xfi->xfi_EntryInfo) {
			ei2 = AE2->ae_xfi->xfi_EntryInfo;
		}

		if (SortType & MUIV_NList_TitleMark_TypeMask) {
			Result = Stricmp(ei1, ei2);
		} else {
			Result = Stricmp(ei2, ei1);
		}

		break;


	case 8: /* Suffix */
		if (AE1->ae_SuffixPtr) {
			suffix1 = AE1->ae_SuffixPtr;
		}

		if (AE2->ae_SuffixPtr) {
			suffix2 = AE2->ae_SuffixPtr;
		}

		if (SortType & MUIV_NList_TitleMark_TypeMask) {
			Result = Stricmp(suffix1, suffix2);
		} else {
			Result = Stricmp(suffix2, suffix1);
		}

		break;
		
	}

	return Result;
}

struct Hook ListMakeHook = {
	{NULL, NULL }, (void *) ListMakeFunc, NULL, NULL
};

struct Hook ListKillHook = {
	{ NULL, NULL }, (void *) ListKillFunc, NULL, NULL
};

struct Hook ListShowHook = {
	{ NULL, NULL }, (void *) ListShowFunc, NULL, NULL
};

struct Hook ListSortHook = {
	{ NULL, NULL } , (void *) ListSortFunc, NULL, NULL
};

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;

	obj = (Object *) DoSuperNew(cl, obj,
		Child, VGroup,
			Child, HGroup,
				MUIA_Group_SameWidth,	FALSE,
				MUIA_Group_HorizSpacing,0,
				Child, data.NarrowParent = RectangleObject,
					ButtonFrame,
					MUIA_CycleChain,    	1,
					MUIA_ShortHelp,			HELP_MAIN_NARROWPARENT,
					MUIA_ControlChar,		"p",
					MUIA_InputMode,			MUIV_InputMode_RelVerify,
					MUIA_Background,		MUII_ButtonBack,
					MUIA_FixWidth,      	5,
					MUIA_VertWeight,    	100,
				End,
				Child, data.AEListview = NListviewObject,
					MUIA_ShortHelp,				HELP_MAIN_AELISTVIEW,
					MUIA_HorizWeight,			200,
					MUIA_CycleChain,			1,
					MUIA_NListview_NList,		data.AEList = NListObject,
						MUIA_ObjectID,				MAKE_ID('A','R','C','L'),
						MUIA_NList_Input,			TRUE,
						MUIA_NList_MultiSelect,		MUIV_NList_MultiSelect_Always,
						MUIA_NList_Active,			MUIV_NList_Active_Top,
						MUIA_NList_Title,			TRUE,
						MUIA_NList_AutoVisible,		TRUE,
						MUIA_NList_ConstructHook2,	&ListMakeHook,
						MUIA_NList_DestructHook2,	&ListKillHook,
						MUIA_NList_DisplayHook2,	&ListShowHook,
						MUIA_NList_CompareHook2,	&ListSortHook,
						MUIA_NList_MinColSortable,	0,
					End,
				End,
			End,
			Child, HGroup,
				Child, data.Root = MyMiniKeyButton("Root",'r',   HELP_MAIN_ROOT),
				Child, data.Parent = MyMiniKeyButton("Parent",'p', HELP_MAIN_PARENT),
				Child, data.ArcLocation = TextObject,
					TextFrame,
					MUIA_HorizWeight,   200,
					MUIA_ShortHelp,     HELP_MAIN_ARCLOCATION,
					MUIA_Text_PreParse, "\33l",
					MUIA_Background,    MUII_TextBack,
				End,
			End,
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		GUI_Popup("Error",
		          "Couldn't create NList.mcc group object!\n"
		          "Possible cause: NList.mcc 19.97+ is not installed.", NULL, "OK");

		return 0;
	}

	ArcView_BuildFormatString(
	    data.viewmask = CFG_Get(TAGID_MAIN_VIEWCOLUMNS), data.buf, sizeof(data.buf)-1);

	set(data.AEList, MUIA_NList_Format, data.buf);

	DoMethod(data.AEListview, MUIM_Notify, MUIA_NList_TitleClick, MUIV_EveryTime, data.AEListview,
	         4, MUIM_NList_Sort3, MUIV_TriggerValue,
	         MUIV_NList_SortTypeAdd_2Values, MUIV_NList_Sort3_SortType_Both);

	DoMethod(data.AEListview, MUIM_Notify, MUIA_NList_SortType, MUIV_EveryTime, data.AEListview,
	         3, MUIM_Set, MUIA_NList_TitleMark, MUIV_TriggerValue);

	DoMethod(data.AEListview, MUIM_Notify, MUIA_NList_SortType, MUIV_EveryTime, data.AEListview,
	         3, MUIM_Set, MUIA_NList_TitleMark, MUIV_TriggerValue);

	NLIST_SCLICK_METHOD(data.AEList, MUIM_avmFilesAndDirs_SingleClick)
	NLIST_DCLICK_METHOD(data.AEList, MUIM_avmFilesAndDirs_DoubleClick)

	BUTTON_METHOD(data.NarrowParent, MUIM_avmFilesAndDirs_Parent)
	BUTTON_METHOD(data.Root,         MUIM_avmFilesAndDirs_Root)
	BUTTON_METHOD(data.Parent,       MUIM_avmFilesAndDirs_Parent)

	PREPDATA;
	return (ULONG) obj;
}

OVERLOAD(OM_DISPOSE)
{
	GETDATA;

	if (data->VMObj) {
		MUI_DisposeObject(data->VMObj);
	}

	return DoSuperMethodA(cl, obj, msg);
}

OVERLOAD(MUIM_Setup)
{
	if (!DoSuperMethodA(cl, obj, msg)) {
		return FALSE;
	}

	set
		(_win(obj), MUIA_Window_DefaultObject, DATAREF->AEListview);

	return TRUE;
}


OVERLOAD(OM_SET)
{
	GETDATA;

	struct TagItem *tags = inittags(msg), *tag;

	while ((tag = NextTagItem(&tags))) {
		switch(tag->ti_Tag) {
			ATTR(ExplorerObj): data->ExplorerObj = (Object *) tag->ti_Data;
			break;
		}
	}

	return DoSuperMethodA(cl, obj, msg);
}

/******************************************************************************/

DECLARE(GetActiveAE) // struct ArcEntry **ae_ptr
{
	DoMethod(DATAREF->AEList, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, msg->ae_ptr);

	return (ULONG) *(msg->ae_ptr);
}

DECLARE(GetEntry) // LONG pos, APTR *tn_ptr
{
	struct ArcEntry *ae = NULL;

	DoMethod(DATAREF->AEList, MUIM_NList_GetEntry, msg->pos, &ae);

	if (msg->tn_ptr) {
		*(msg->tn_ptr) = NULL;
	}

	return (ULONG) ae;
}

DECLARE(GetSelCnt)
{
	ULONG selcnt = 0;

	DoMethod(DATAREF->AEList, MUIM_NList_Select, MUIV_NList_Select_All, MUIV_NList_Select_Ask, &selcnt);

	return selcnt;
}

DECLARE(Clear)
{
	return DoMethod(DATAREF->AEList, MUIM_NList_Clear);
}

DECLARE(Quiet) // LONG state
{
	set(DATAREF->AEList, MUIA_NList_Quiet, msg->state);

	return 0;
}

DECLARE(Select) // LONG pos, APTR tn
{
	return DoMethod(DATAREF->AEList, MUIM_NList_Select, msg->pos, MUIV_NList_Select_On, NULL);
}

DECLARE(SelectAll)
{
	return DoMethod(DATAREF->AEList, MUIM_NList_Select, MUIV_NList_Select_All, MUIV_NList_Select_On, NULL);
}

DECLARE(SelectNone)
{
	return DoMethod(DATAREF->AEList, MUIM_NList_Select, MUIV_NList_Select_All, MUIV_NList_Select_Off, NULL);
}

DECLARE(NextSelected) // LONG lastpos
{
	/* Pass -1 to start off. Will return -1 when no more entries are selected. */

	GETDATA;
	LONG lastpos = msg->lastpos;

	if (lastpos == -1) {
		lastpos = MUIV_NList_NextSelected_Start;
	}

	DoMethod(data->AEList, MUIM_NList_NextSelected, &lastpos);

	if (lastpos == MUIV_NList_NextSelected_End) {
		lastpos = -1;
	}

	return (ULONG) lastpos;
}

DECLARE(GotoFirst) // ULONG listerpos
{
	set(DATAREF->AEList, MUIA_NList_First, msg->listerpos);

	return 0;
}

DECLARE(GetFirst)
{
	return xget(DATAREF->AEList, MUIA_NList_First); /* listerpos */
}

DECLARE(Jump) // ULONG pos
{
	return DoMethod(DATAREF->AEList, MUIM_NList_Jump, msg->pos);
}

DECLARE(CountEntries)
{
	return xget(DATAREF->AEList, MUIA_NList_Entries);
}

DECLARE(ShowArcEntries) // AHN *ahn
{
	GETDATA;
	Object *lv_obj = data->AEList;
	struct ArcEntry *ae;
	ULONG listerpos = 0;
	AHN *ahn = msg->ahn;
	struct PN *pn;

	if (!ahn) {
		return 0;
	}

	pn = ahn->ahn_CurrentPN;
	ae = pn->pn_currentae;

	if (!ae->ae_ChildAEListSorted) {
		GUI_PrintStatus("Sorting entries...", 0);
		AEDir_Sort(ae, SORTMODE_DIRSABOVE);
		ae->ae_ChildAEListSorted = TRUE;
	}

	set(lv_obj, MUIA_NList_Quiet, TRUE);
	DoMethod(lv_obj, MUIM_NList_Clear);

	for (ae = (struct ArcEntry *) ae->ae_ChildAEL.mlh_Head;
	     ae->ae_Node.mln_Succ;
	     ae = (struct ArcEntry *) ae->ae_Node.mln_Succ) {

		DoMethod(lv_obj, MUIM_NList_InsertSingle, ae, MUIV_NList_Insert_Bottom);
	}

	if (listerpos != ~0UL) {
		set(lv_obj, MUIA_NList_First, listerpos);
	}

	set(lv_obj, MUIA_NList_Quiet, FALSE);

	if (pn->pn_currentae && pn->pn_currentae->ae_FullPath) {

		set(data->ArcLocation, MUIA_Text_Contents, pn->pn_currentae->ae_FullPath);

	} else {

		set(data->ArcLocation, MUIA_Text_Contents, "");
	}

	data->ahn = ahn;

	return 0;
}

DECLARE(SetColumns) // ULONG viewmask
{
	GETDATA;

	ArcView_BuildFormatString(data->viewmask = msg->viewmask, data->buf, sizeof(data->buf));
	set(data->AEList, MUIA_NList_Format, data->buf);

	return 0;
}

DECLARE(ShowColumns) // ULONG viewmask
{
	GETDATA;

	ArcView_BuildFormatString(data->viewmask |= msg->viewmask, data->buf, sizeof(data->buf));
	set(data->AEList, MUIA_NList_Format, data->buf);

	return 0;
}

DECLARE(HideColumns) // ULONG viewmask
{
	GETDATA;

	ArcView_BuildFormatString(data->viewmask &= ~msg->viewmask, data->buf, sizeof(data->buf));
	set(data->AEList, MUIA_NList_Format, data->buf);

	return 0;
}

DECLARE(Disabled) // ULONG yes
{
	GETDATA;

	set(data->AEList,       MUIA_Disabled, msg->yes);
	set(data->NarrowParent, MUIA_Disabled, msg->yes);
	set(data->Root,         MUIA_Disabled, msg->yes);
	set(data->Parent,       MUIA_Disabled, msg->yes);

	return 0;
}

/******************************************************************************/

DECLARE(Parent)
{
	GETDATA;

	AHN *ahn = data->ahn;
	struct PN *pn = ahn->ahn_CurrentPN;

	if (!pn || !pn->pn_currentae) {
		return 0;
	}

	pn->pn_currentae->ae_ListerPos = DoMethod(obj, MUIM_avmFilesAndDirs_GetFirst);

	if (ahn->ahn_EmbeddedArc && !pn->pn_currentae->ae_ParentAE) {

		DoMethod(GETARCVIEW, MUIM_ArcView_GotoParentArc, ahn);

	} else if (pn->pn_currentae->ae_ParentAE) {

		pn->pn_currentae = pn->pn_currentae->ae_ParentAE;
		DoMethod(obj, MUIM_avmFilesAndDirs_ShowArcEntries, ahn);

		if (data->ExplorerObj) {
			DoMethod(data->ExplorerObj, MUIM_avmExplorer_TreeActive, pn->pn_currentae->ae_TreeNode);
		}
	}

	return 0;
}

DECLARE(Root)
{
	GETDATA;
	AHN *ahn = data->ahn;
	struct PN *pn = ahn->ahn_CurrentPN;

	if (!pn || !pn->pn_currentae) {
		return 0;
	}

	pn->pn_currentae->ae_ListerPos = DoMethod(GETARCVIEW, MUIM_ArcView_GetFirst);

	if (pn->pn_currentae != pn->pn_rootae) {

		pn->pn_currentae = pn->pn_rootae;
		DoMethod(obj, MUIM_avmFilesAndDirs_ShowArcEntries, ahn);

		if (data->ExplorerObj) {
			DoMethod(data->ExplorerObj, MUIM_avmExplorer_TreeActive, pn->pn_rootae->ae_TreeNode);
		}
	}

	return 0;
}

DECLARE(SingleClick) // LONG pos
{
	DoMethod(GETARCVIEW, MUIM_ArcView_DoSingleClick);

	return 0;
}

DECLARE(DoubleClick) // LONG pos
{
	GETDATA;

	struct ArcEntry *ae;
	AHN *ahn = data->ahn;
	struct PN *pn;

	DoMethod(obj, MUIM_avmFilesAndDirs_GetActiveAE, &ae);

	if (!ae) {
		return 0;
	}

	pn = ahn->ahn_CurrentPN;

	if (ae->ae_ParentAE) {

		ae->ae_ParentAE->ae_ListerPos = ~0UL;
		get(data->AEList, MUIA_NList_First, &ae->ae_ParentAE->ae_ListerPos);
	}

	if (ae->ae_IsDir) {

		if (data->ExplorerObj) {
			DoMethod(data->ExplorerObj, MUIM_avmExplorer_TreeActive, ae->ae_TreeNode);
		}

		pn->pn_currentae = ae;
		DoMethod(obj, MUIM_avmFilesAndDirs_ShowArcEntries, ahn);

		return 0;
	}

	DoMethod(GETARCVIEW, MUIM_ArcView_DoDoubleClick, ahn, ae);

	return 0;
}

