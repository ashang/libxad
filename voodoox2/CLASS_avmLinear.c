/*
 $Id: CLASS_avmLinear.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class for Linear/Flat archive view mode (AVM).

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
 *  Class Name: avmLinear
 * Description: Handles the flat (aka Linear) view mode.
 *  Superclass: MUIC_Group
 */

#include "vx_include.h"
#include "avmLinear.h"
#include "vx_arc.h"
#include "vx_mem.h"
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_misc.h"
#include "vx_cfg.h"


/* CLASSDATA
//------------------------------------------------------------------------------
	Object *AEList;
	Object *AEListview;
	AHN *ahn;
	ULONG viewmask;
	UBYTE buf[256];
//------------------------------------------------------------------------------
*/

SAVEDS ASM(void) LinearNListKillFunc(
    REG_A1 (struct NList_DestructMessage *NL_DMsg),
    REG_A2 (Object *Obj) )
{
}

SAVEDS ASM(struct ArcEntry *) LinearNListMakeFunc(
    REG_A1 (struct NList_ConstructMessage *NL_CMsg),
    REG_A2 (Object *Obj) )
{

	return NL_CMsg->entry; /* Just pass it back :) */
}

SAVEDS ASM(LONG) LinearNListShowFunc(
    REG_A1(struct NList_DisplayMessage *NL_DMsg),
    REG_A2(Object *Obj) )
{

	ArcView_FillHookArrays(
	    (UBYTE **) NL_DMsg->strings,
	    (UBYTE **) NL_DMsg->preparses, (AE *) NL_DMsg->entry, TRUE);

	return 0;
}

SAVEDS ASM(LONG) LinearNListSortFunc(
    REG_A1 (struct NList_CompareMessage *NL_CMsg),
    REG_A2 (Object *Obj) )
{
	struct ArcEntry *ae1 = (struct ArcEntry *) NL_CMsg->entry1;
	struct ArcEntry *ae2 = (struct ArcEntry *) NL_CMsg->entry2;

	LONG col1     = NL_CMsg->sort_type & MUIV_NList_TitleMark_ColMask;
	LONG sorttype = NL_CMsg->sort_type;
	LONG result   = 0;

	UBYTE *comment1 = "", *comment2 = "";
	UBYTE *ei1 = "", *ei2 = "";
	UBYTE *suffix1 = "", *suffix2 = "";

	if (sorttype == MUIV_NList_SortType_None) {
		return 0;
	}

	switch (col1) {
	case 0: /* Name */
		if (ae1->ae_xfi && ae2->ae_xfi) {
			if (sorttype & MUIV_NList_TitleMark_TypeMask) {
				result = Stricmp(ae2->ae_xfi->xfi_FileName, ae1->ae_xfi->xfi_FileName); /* Title arrow is up */
			} else {
				result = Stricmp(ae1->ae_xfi->xfi_FileName, ae2->ae_xfi->xfi_FileName); /* Title arrow is down */
			}
		}
		break;


	case 1: /* Size */
		/* Note: Directory entries MUST always have ae_Size to -1. */

		if (sorttype & MUIV_NList_TitleMark_TypeMask) {
			result = ae2->ae_Size - ae1->ae_Size;
		} else {
			result = ae1->ae_Size - ae2->ae_Size;
		}
		break;

	case 2: /* Day */
	case 3: /* Date */
	case 4: /* Time */
		if (sorttype & MUIV_NList_TitleMark_TypeMask) {
			result = CompareDates(&ae1->ae_DS, &ae2->ae_DS);
		} else {
			result = CompareDates(&ae2->ae_DS, &ae1->ae_DS);
		}
		break;

	case 5:	/* Protection */
	{
		UBYTE *pv1 = ae1->ae_ProtBuf;
		UBYTE *pv2 = ae2->ae_ProtBuf;

		if (sorttype & MUIV_NList_TitleMark_TypeMask) {
			result = Stricmp(pv1, pv2);
		} else {
			result = Stricmp(pv2, pv1);
		}

		break;
	}

	case 6:	/* Comment */
		if (ae1->ae_xfi && ae1->ae_xfi->xfi_Comment) {
			comment1 = ae1->ae_xfi->xfi_Comment;
		}

		if (ae2->ae_xfi && ae2->ae_xfi->xfi_Comment) {
			comment2 = ae2->ae_xfi->xfi_Comment;
		}

		if (sorttype & MUIV_NList_TitleMark_TypeMask) {
			result = Stricmp(comment1, comment2);
		} else {
			result = Stricmp(comment2, comment1);
		}
		break;

	case 7: /* EntryInfo */
		if (ae1->ae_xfi && ae1->ae_xfi->xfi_EntryInfo) {
			ei1 = ae1->ae_xfi->xfi_EntryInfo;
		}

		if (ae2->ae_xfi && ae2->ae_xfi->xfi_EntryInfo) {
			ei2 = ae2->ae_xfi->xfi_EntryInfo;
		}

		if (sorttype & MUIV_NList_TitleMark_TypeMask) {
			result = Stricmp(ei1, ei2);
		} else {
			result = Stricmp(ei2, ei1);
		}
		break;


	case 8: /* Suffix */
		if (ae1->ae_SuffixPtr) {
			suffix1 = ae1->ae_SuffixPtr;
		}

		if (ae2->ae_SuffixPtr) {
			suffix2 = ae2->ae_SuffixPtr;
		}

		if (sorttype & MUIV_NList_TitleMark_TypeMask) {
			result = Stricmp(suffix1, suffix2);
		} else {
			result = Stricmp(suffix2, suffix1);
		}

		break;
	}

	return result;
}

struct Hook LinearNListMakeHook = { { NULL, NULL }, (void *) LinearNListMakeFunc, NULL, NULL };
struct Hook LinearNListKillHook = { { NULL, NULL }, (void *) LinearNListKillFunc, NULL, NULL };
struct Hook LinearNListShowHook = { { NULL, NULL }, (void *) LinearNListShowFunc, NULL, NULL };
struct Hook LinearNListSortHook = { { NULL, NULL }, (void *) LinearNListSortFunc, NULL, NULL };

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;
	
	obj = (Object *) DoSuperNew(cl, obj,
		Child, data.AEListview = NListviewObject,
			MUIA_ShortHelp,				HELP_MAIN_AELISTVIEW,
			MUIA_HorizWeight,			200,
			MUIA_CycleChain,			1,
			MUIA_NListview_NList,		data.AEList = NListObject,
				MUIA_ObjectID,				MAKE_ID('A','R','C','L'),
				MUIA_NList_Input,			TRUE,
				MUIA_NList_MultiSelect, 	MUIV_NList_MultiSelect_Always,
				MUIA_NList_Active, 			MUIV_NList_Active_Top,
				MUIA_NList_Title,			TRUE,
				MUIA_NList_AutoVisible,		TRUE,
				MUIA_NList_ConstructHook2,	&LinearNListMakeHook,
				MUIA_NList_DestructHook2,	&LinearNListKillHook,
				MUIA_NList_DisplayHook2,	&LinearNListShowHook,
				MUIA_NList_CompareHook2,	&LinearNListSortHook,
				MUIA_NList_MinColSortable,	0,
			End,
		End,
		TAG_MORE, (ULONG)inittags(msg)
	);

	if (!obj) {
		GUI_Popup("Error", "Couldn't create NList.mcc group object!\n"
		          "Possible cause: NList.mcc 19.97+ is not installed.", NULL, "OK");

		return 0;
	}

	ArcView_BuildFormatString(
	    data.viewmask = CFG_Get(TAGID_MAIN_VIEWCOLUMNS), data.buf, sizeof(data.buf)-1);

	set(data.AEList, MUIA_NList_Format, data.buf);

	NLIST_SCLICK_METHOD(data.AEList, MUIM_avmLinear_SingleClick)
	NLIST_DCLICK_METHOD(data.AEList, MUIM_avmLinear_DoubleClick)

	DoMethod(data.AEList, MUIM_Notify,
	         MUIA_NList_TitleClick, MUIV_EveryTime, data.AEList,
	         4, MUIM_NList_Sort3, MUIV_TriggerValue,
	         MUIV_NList_SortTypeAdd_2Values, MUIV_NList_Sort3_SortType_Both);

	DoMethod(data.AEList, MUIM_Notify,
	         MUIA_NList_SortType, MUIV_EveryTime, data.AEList,
	         3, MUIM_Set, MUIA_NList_TitleMark, MUIV_TriggerValue);

	DoMethod(data.AEList, MUIM_Notify,
	         MUIA_NList_SortType, MUIV_EveryTime, data.AEList,
	         3, MUIM_Set, MUIA_NList_TitleMark, MUIV_TriggerValue);

	PREPDATA;

	return (ULONG) obj;
}

OVERLOAD(MUIM_Setup)
{
	if (!DoSuperMethodA(cl, obj, msg)) {
		return FALSE;
	}

	set(_win(obj), MUIA_Window_DefaultObject, DATAREF->AEListview);

	return TRUE;
}

DECLARE(GetActiveAE) // struct ArcEntry **ae_ptr
{
	DoMethod(DATAREF->AEList, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, msg->ae_ptr);

	return (ULONG) *(msg->ae_ptr);
}

DECLARE(GetEntry) // LONG pos, APTR *tn_ptr
{
	AE *ae = NULL;
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
	DoMethod(DATAREF->AEList, MUIM_NList_Select, MUIV_NList_Select_All, MUIV_NList_Select_Off, NULL);

	return 0;
}

DECLARE(NextSelected) // LONG lastpos
{
	/* Pass -1 to start off. Will return -1 when no more entries are selected. */

	LONG lastpos = msg->lastpos;

	if (lastpos == -1) {
		lastpos = MUIV_NList_NextSelected_Start;
	}

	DoMethod(DATAREF->AEList, MUIM_NList_NextSelected, &lastpos);

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
	return /* listerpos */ xget(DATAREF->AEList, MUIA_NList_First);
}

DECLARE(Jump) // ULONG pos
{
	DoMethod(DATAREF->AEList, MUIM_NList_Jump, msg->pos);
	return 0;
}

DECLARE(CountEntries)
{

	return xget(DATAREF->AEList, MUIA_NList_Entries);
}

DECLARE(ShowArcEntries) // AHN *ahn
{
	GETDATA;

	register Object *lv_obj = data->AEList;
	register struct ArcEntry **aea;
	PN *pn = msg->ahn->ahn_CurrentPN;

	WHERE;

	aea = pn->pn_linear_aes_alpha;

	if (!pn->pn_linear_aes_alpha_ready) {
		GUI_PrintStatus("Sorting %lu entries...", pn->pn_linear_cnt);

		/* TODO: Check/abstract this... */

		qsort(aea, pn->pn_linear_cnt, sizeof(APTR),
		      (int (*)(const void *, const void *)) &ARC_AESortFunc);

		pn->pn_linear_aes_alpha_ready = TRUE;
	}

	GUI_PrintStatus("Displaying entries...", 0);

	set(lv_obj, MUIA_NList_Quiet, TRUE);
	DoMethod(lv_obj, MUIM_NList_Clear);

	while (*aea) {
		DoMethod(lv_obj, MUIM_NList_Insert, aea, 1, MUIV_NList_Insert_Bottom);
		aea++;
	}

	set(lv_obj, MUIA_NList_Quiet, FALSE);
	data->ahn = msg->ahn;

	return 0;
}

DECLARE(SetColumns) // ULONG viewmask
{
	GETDATA;
	ArcView_BuildFormatString(
	    data->viewmask = msg->viewmask, data->buf, sizeof(data->buf)-1);
	set(data->AEList, MUIA_NList_Format, data->buf);
	return 0;
}

DECLARE(ShowColumns) // ULONG viewmask
{
	GETDATA;
	ArcView_BuildFormatString(
	    data->viewmask |= msg->viewmask, data->buf, sizeof(data->buf)-1);
	set(data->AEList, MUIA_NList_Format, data->buf);
	return 0;
}

DECLARE(HideColumns) // ULONG viewmask
{
	GETDATA;

	ArcView_BuildFormatString(
	    data->viewmask &= ~msg->viewmask, data->buf, sizeof(data->buf)-1);
	set(data->AEList, MUIA_NList_Format, data->buf);

	return 0;
}

DECLARE(Disabled) // ULONG yes
{
	set(DATAREF->AEList, MUIA_Disabled, msg->yes);

	return 0;
}

DECLARE(SingleClick)
{
	DoMethod(GETARCVIEW, MUIM_ArcView_DoSingleClick);

	return 0;
}

DECLARE(DoubleClick)
{
	GETDATA;

	AHN *ahn = data->ahn;
	AE *ae;

	DoMethod(obj, MUIM_avmLinear_GetActiveAE, &ae);

	if (!ae) {
		return 0;
	}

	if (ae->ae_IsDir) {
		return 0;
	}

	DoMethod(GETARCVIEW, MUIM_ArcView_DoDoubleClick, ahn, ae);

	return 0;
}

