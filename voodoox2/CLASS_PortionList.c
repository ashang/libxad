/*
 $Id: CLASS_PortionList.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class for the archive portion list.

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
 *  Class Name: Portion list.
 * Description: Handles the portion list.
 *  Superclass: MUIC_NListview
 */

#include "PortionList.h"
#include "vx_misc.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object	*NListObj;
//------------------------------------------------------------------------------
*/

void UpdateList( Object *list, AHN *ahn )
{
	struct PN *pn;
	LONG i;

	if (!ahn) return;

	DoMethod(list, MUIM_NList_Clear);

	for (pn = (struct PN *) ahn->ahn_PNList.mlh_Head, i = 0;
	     pn->pn_node.mln_Succ;
	     pn = (struct PN *) pn->pn_node.mln_Succ, i++) {
			
		UBYTE tmpBuf[256], *typeStr, *clientName;

		if (pn->pn_type == PNTYPE_ARCHIVE) {
			typeStr = "Archive";
		} else if (pn->pn_type == PNTYPE_DISK) {
			typeStr = "Disk";
		} else {
			typeStr = "Unknown";
		}

		if (pn->pn_ai->xai_Client) {
			clientName = pn->pn_ai->xai_Client->xc_ArchiverName;
		} else {
			clientName = NULL;
		}

		if (!clientName) {
			clientName = "Unknown";
		}

		sprintf(tmpBuf, "Portion %03ld - %-10s - %s", i + 1, typeStr, clientName);

		DoMethod(list, MUIM_NList_InsertSingle,
			         &tmpBuf, MUIV_NList_Insert_Bottom);
	}
}

OVERLOAD(OM_NEW)
{
	struct TagItem *tags = inittags(msg), *tag;
	UBYTE **nameArray = NULL;
	AHN *ahn = NULL;
	DEFTMPDATA;
	CLRTMPDATA;

	while ((tag = NextTagItem(&tags))) {
		switch(tag->ti_Tag) {
			ATTR(NameArray): nameArray = (UBYTE **) tag->ti_Data;
			break;
			ATTR(AHN):       ahn       = (AHN *)    tag->ti_Data;
			break;
		}
	}

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Weight,			1,
		MUIA_CycleChain,		1,
		MUIA_ShortHelp,			"This archive contains multiple portions,"
		                        " select which portion you want to view here.",
		MUIA_NListview_NList,	data.NListObj = NListObject,
			MUIA_Font,                MUIV_NList_Font_Fixed,
			MUIA_NList_Input,         TRUE,
			MUIA_NList_MultiSelect,	  MUIV_NList_MultiSelect_None,
			MUIA_NList_Title,		  FALSE,
			MUIA_NList_AutoVisible,   TRUE,
			MUIA_NList_ConstructHook, MUIV_NList_ConstructHook_String,
			MUIA_NList_DestructHook,  MUIV_NList_DestructHook_String,
		End,
		TAG_MORE, (ULONG)inittags(msg)
	);

	if (!obj) {
		return 0;
	}

	UpdateList(data.NListObj, ahn);

	DoMethod(data.NListObj, MUIM_Notify,
	         MUIA_NList_Active, MUIV_EveryTime, obj,
	         2, MUIM_PortionList_SingleClick, MUIV_TriggerValue);

	PREPDATA;

	return (ULONG) obj;
}

DECLARE(SingleClick) // ULONG sel
{
	DoMethod(GETARCVIEW, MUIM_ArcView_SwitchPortionNum, msg->sel);

	return 0;
}

OVERLOAD(OM_SET)
{
	GETDATA;
	struct TagItem *tags = inittags(msg), *tag;

	while ((tag = NextTagItem(&tags))) {
		switch (tag->ti_Tag) {
			ATTR(AHN): UpdateList(data->NListObj, (AHN *)tag->ti_Data); break;
		}
	}

	return DoSuperMethodA(cl, obj, msg);
}
