/*
 $Id: CLASS_avmNListtree.c,v 1.2 2004/01/25 04:39:16 andrewbell Exp $
 Custom class for NListtree archive view mode (AVM).

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
 *  Class Name: avmNListtree
 * Description: Handles the Tree view mode (using NListtree)
 *  Superclass: MUIC_Group
 */

#include "vx_include.h"
#include "avmNListtree.h"
#include "vx_arc.h"
#include "vx_debug.h"
#include "vx_misc.h"
#include "vx_main.h"
#include "vx_cfg.h"
#include <mui/NListtree_mcc.h>


/* CLASSDATA
//------------------------------------------------------------------------------
	Object *Listtree;
	Object *Parent;
	Object *LinkGroup;
	Object *OpenAll;
	Object *CloseAll;
	Object *ExplorerObj;
 
	AHN *ahn;
	struct	MUIS_NListtree_TreeNode	*last_tn;
 
	ULONG viewmask;
	UBYTE buf[256];
 
	// Keep 16bit BOOLs at end to avoid alignment issues...
 
	BOOL DirsOnly;
//------------------------------------------------------------------------------
*/

/* EXPORT
//------------------------------------------------------------------------------
extern void MCC_GetNListtreeVerRev( ULONG *verptr, ULONG *revptr );
//------------------------------------------------------------------------------
*/

void MCC_GetNListtreeVerRev( ULONG *verptr, ULONG *revptr ) {

	Object *objTree;

	if ((objTree = NListtreeObject, InputListFrame, End)) {

		if (verptr) {
			get(objTree, MUIA_Version, verptr);
		}

		if (revptr) {
			get(objTree, MUIA_Revision, revptr);
		}

		MUI_DisposeObject(objTree);

	} else {

		if (verptr) {
			*verptr = 0;
		}

		if (revptr) {
			*revptr = 0;
		}
	}
}

//#define LIST_FORMAT_ARCENTRIES          "BAR MIW=10,BAR,,,BAR,BAR,MIW=10"
//#define LIST_FORMAT_ARCENTRIES_DIRSONLY ""

SAVEDS ASM(struct ArcEntry *) NListTreeMakeFunc(
    REG_A0 (struct Hook *hook),
    REG_A1 (struct MUIP_NListtree_ConstructMessage *cm),
    REG_A2 (Object *obj) )
{
	return cm->UserData;
}

SAVEDS ASM(LONG) NListTreeKillFunc(
    REG_A0 (struct Hook *hook),
    REG_A1 (struct MUIP_NListtree_DestructMessage *dm),
    REG_A2 (Object *obj) )
{
	return 0;
}

SAVEDS ASM(LONG) NListTreeShowFunc(
    REG_A0 (struct Hook *hook),
    REG_A1 (struct MUIP_NListtree_DisplayMessage *dm),
    REG_A2 (Object *obj) )
{
	register struct MUI_NListtree_TreeNode *treeNode;
	register struct ArcEntry *ae;

	if ((treeNode = dm->TreeNode)) {
		ae = treeNode->tn_User;
	} else {
		ae = NULL;
	}

	ArcView_FillHookArrays((UBYTE **) dm->Array, NULL, ae, FALSE);

	if (ae && ae->ae_IsDir && ae->ae_Name) {
		register UBYTE **col = dm->Array;

		col[0] = ae->ae_DisplayBuf;
		strcpy(ae->ae_DisplayBuf, "\338");
		strncat(ae->ae_DisplayBuf, ae->ae_Name, sizeof(ae->ae_DisplayBuf)-1);
	}

	return 0;

#ifdef IGNORE

	register struct MUI_NListtree_TreeNode *treeNode = dm->TreeNode;
	register UBYTE **col = dm->Array;

	if (!col) {
		return 0;
	}

	if (treeNode) {
		register UBYTE *comment, *name;
		register struct ArcEntry *ae = treeNode->tn_User;

		if (ae->ae_IsDir && ae->ae_Name) {
			strcpy(ae->ae_DisplayBuf, "\338");
			strncat(ae->ae_DisplayBuf, ae->ae_Name, sizeof(ae->ae_DisplayBuf)-1);
			name = (UBYTE *) &ae->ae_DisplayBuf;

		} else {
			name = ae->ae_Name;
		}

		if (!name) {
			name = "(no name)";
		}

		if (ae->ae_xfi && ae->ae_xfi->xfi_Comment) {
			comment = ae->ae_xfi->xfi_Comment;
		} else {
			comment = "";
		}

		ARC_ResolveAEDate(ae);
		ARC_ResolveAESize(ae);
		ARC_ResolveAEProt(ae);

		/* [0] */ *col++ = name;
		/* [1] */ *col++ = ae->ae_SizeBuf;
		/* [2] */ *col++ = ae->ae_DayPart;
		/* [3] */ *col++ = ae->ae_DatePart;
		/* [4] */ *col++ = ae->ae_TimePart;
		/* [5] */ *col++ = ae->ae_ProtBuf;
		/* [6] */ *col   = comment;

	} else {

		/* [0] */ *col++ = "\33bName";
		/* [1] */ *col++ = "\33bSize";
		/* [2] */ *col++ = "\33bDay";
		/* [3] */ *col++ = "\33bDate";
		/* [4] */ *col++ = "\33bTime";
		/* [5] */ *col++ = "\33bBits";
		/* [6] */ *col   = "\33bComment";
	}

	return 1;

#endif
}


SAVEDS ASM(LONG) NListTreeDirsOnlyShowFunc(
    REG_A0 (struct Hook *hook),
    REG_A1 (struct MUIP_NListtree_DisplayMessage *dm),
    REG_A2 (Object *obj) )
{
	register struct MUI_NListtree_TreeNode *treeNode = dm->TreeNode;
	register struct ArcEntry *ae;
	register UBYTE **col = dm->Array;

	if (!treeNode) {
		*col = "\33bName";
		return 1;
	}

	ae = treeNode->tn_User;

	if (!ae->ae_IsDir || !ae->ae_Name) {
		return 0;
	}

	/* IDEA: Use a DisplayBuf_Valid flag to to gain some speed... */

	strcpy(ae->ae_DisplayBuf, "\338");
	strncat(ae->ae_DisplayBuf, ae->ae_Name, sizeof(ae->ae_DisplayBuf)-1);
	*col = ae->ae_DisplayBuf;

	return 1;
}

SAVEDS ASM(LONG) NListTreeOpenFunc(
    REG_A0 (struct Hook *hook),
    REG_A1 (struct MUIP_NListtree_OpenMessage *om),
    REG_A2 (Object *obj) )
{
	struct MUI_NListtree_TreeNode *treeNode = om->TreeNode;

	if (treeNode) {
		struct ArcEntry *ae = treeNode->tn_User;
		ae->ae_NodeOpened = TRUE;
	}

	return 0;
}

SAVEDS ASM(LONG) NListTreeCloseFunc(
    REG_A0 (struct Hook *hook),
    REG_A1 (struct MUIP_NListtree_CloseMessage *cm),
    REG_A2 (Object *obj) )
{
	struct MUI_NListtree_TreeNode *treeNode = cm->TreeNode;

	if (treeNode) {
		struct ArcEntry *ae = treeNode->tn_User;
		ae->ae_NodeOpened = FALSE;
	}

	return 0;
}

SAVEDS ASM(LONG) NListTreeSortFunc(
    REG_A0 (struct Hook *hook),
    REG_A1 (struct MUIP_NListtree_CompareMessage *cm),
    REG_A2 (Object *obj) )
{
	return 0;
}

struct Hook NListTreeMakeHook  = {
	{ NULL, NULL }, (void *) NListTreeMakeFunc, NULL, NULL
};

struct Hook NListTreeKillHook  = {
	{ NULL, NULL }, (void *) NListTreeKillFunc, NULL, NULL
};

struct Hook NListTreeShowHook  = {
	{ NULL, NULL }, (void *) NListTreeShowFunc, NULL, NULL
};

struct Hook NListTreeDirsOnlyShowHook = {
	{ NULL, NULL }, (void *) NListTreeDirsOnlyShowFunc, NULL, NULL
};

struct Hook NListTreeOpenHook  = {
	{ NULL, NULL }, (void *) NListTreeOpenFunc, NULL, NULL
};

struct Hook NListTreeCloseHook =         {
	{ NULL, NULL }, (void *) NListTreeCloseFunc, NULL, NULL
};

SAVEDS ASM(BOOL) MultiTestFunc(
    REG_A0 (struct Hook *hk),
    REG_A1 (struct MUIP_NListtree_MultiTestMessage *mts),
    REG_A2 (Object *obj) )
{
	if (mts->TreeNode->tn_Flags & TNF_LIST) {
		return FALSE; /* Not selectable */

	} else {

		return TRUE; /* Selectable */
	}
}

struct Hook MultiTestHook = {
	{ NULL, NULL }, (void *) MultiTestFunc, NULL, NULL
};

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;

	struct TagItem *tags = inittags(msg), *tag;
	CLRTMPDATA;

	while ((tag = NextTagItem(&tags))) {
		switch(tag->ti_Tag) {
			ATTR(DirsOnly):    data.DirsOnly    = (BOOL)     tag->ti_Data;
			break;
			ATTR(ExplorerObj): data.ExplorerObj = (Object *) tag->ti_Data;
			break;
		}
	}

	obj = (Object *) DoSuperNew(cl, obj,
		Child, data.LinkGroup = VGroup,
			Child, NListviewObject,
				MUIA_CycleChain,		1,
				MUIA_NListview_NList,	data.Listtree = NListtreeObject,
					InputListFrame,
					MUIA_NListtree_MultiSelect,   (data.DirsOnly ? MUIV_NListtree_MultiSelect_None : MUIV_NListtree_MultiSelect_Always),
					//MUIA_NListtree_Format,        (data.DirsOnly ? LIST_FORMAT_ARCENTRIES_DIRSONLY : LIST_FORMAT_ARCENTRIES),
					MUIA_NListtree_DisplayHook,   (data.DirsOnly ? &NListTreeDirsOnlyShowHook      : &NListTreeShowHook),
					MUIA_NListtree_ConstructHook, &NListTreeMakeHook,
					MUIA_NListtree_DestructHook,  &NListTreeKillHook,
					MUIA_NListtree_OpenHook,      &NListTreeOpenHook,
					MUIA_NListtree_CloseHook,     &NListTreeCloseHook,
					MUIA_NListtree_Title,         TRUE,
					MUIA_NListtree_EmptyNodes,    TRUE,
					MUIA_NListtree_DragDropSort,  FALSE,
					MUIA_NListtree_MultiTestHook, &MultiTestHook,
				End,
			End,
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		GUI_Popup("Error",
		          "Couldn't create NListtree.mcc group object!\n"
		          "Possible cause: NListtree.mcc 18.03+ is not installed.", NULL, "OK");

		return 0;
	}

	if (data.DirsOnly) {

		set(data.Listtree, MUIA_NListtree_Format, "");

	} else {

		ArcView_BuildFormatString(
		    data.viewmask = CFG_Get(TAGID_MAIN_VIEWCOLUMNS), data.buf, sizeof(data.buf)-1);

		set(data.Listtree, MUIA_NListtree_Format, data.buf);
	}

	DoMethod(data.LinkGroup, MUIM_Group_InitChange);

	if (data.DirsOnly) {

		DoMethod(data.LinkGroup, OM_ADDMEMBER,
			HGroup,
				Child, data.OpenAll     = MyKeyButton("Expand",'o',  HELP_MAIN_AETREE_OPENALL),
				Child, data.CloseAll    = MyKeyButton("Collapse",'c', HELP_MAIN_AETREE_CLOSEALL),
			End
		);

	} else {

		DoMethod(data.LinkGroup, OM_ADDMEMBER,
			HGroup,
				Child, data.Parent      = MyKeyButton("Parent archive",'p', HELP_MAIN_PARENT),
				Child, RectangleObject, End,
				Child, data.OpenAll     = MyKeyButton("Expand",'o',  HELP_MAIN_AETREE_OPENALL),
				Child, data.CloseAll    = MyKeyButton("Collapse",'c', HELP_MAIN_AETREE_CLOSEALL),
			End
		);
	}

	DoMethod(data.LinkGroup, MUIM_Group_ExitChange);

	if (data.Parent) {
		BUTTON_METHOD(data.Parent, MUIM_avmNListtree_Parent)
	}

	BUTTON_METHOD(data.OpenAll,  MUIM_avmNListtree_OpenAll)
	BUTTON_METHOD(data.CloseAll, MUIM_avmNListtree_CloseAll)

	NLISTTREE_DCLICK_METHOD(data.Listtree, MUIM_avmNListtree_DoubleClick)
	NLISTTREE_SCLICK_METHOD(data.Listtree, MUIM_avmNListtree_SingleClick)

	PREPDATA;

	return (ULONG) obj;
}

OVERLOAD(OM_DISPOSE)
{

	return DoSuperMethodA(cl, obj, msg);
}

OVERLOAD(MUIM_Setup)
{

	if (!DoSuperMethodA(cl, obj, msg)) {
		return FALSE;
	}

	set(_win(obj), MUIA_Window_DefaultObject, DATAREF->Listtree);

	return TRUE;
}

OVERLOAD(OM_SET)
{
	GETDATA;

	struct TagItem *tags = inittags(msg), *tag;

	while((tag = NextTagItem(&tags))) {

		switch(tag->ti_Tag) {
			ATTR(ExplorerObj): data->ExplorerObj = (Object *) tag->ti_Data;
			break;
		}
	}

	return DoSuperMethodA(cl, obj, msg);
}

DECLARE(GetActiveAE) // struct ArcEntry **ae_ptr
{
	GETDATA;
	struct ArcEntry *ae;
	struct MUI_NListtree_TreeNode *tn = NULL;

	get(data->Listtree, MUIA_NListtree_Active, &tn);

	if (tn == NULL || tn == MUIV_NListtree_Active_Off) {

		ae = NULL;
	} else {

		ae = tn->tn_User;
	}

	if (msg->ae_ptr) {
		*(msg->ae_ptr) = ae;
	}

	return (ULONG) ae;
}

DECLARE(GetEntry) // LONG pos, APTR *tn_ptr
{
	GETDATA;
	struct ArcEntry *ae;
	struct MUI_NListtree_TreeNode *tn = NULL;

	get(data->Listtree, MUIA_NListtree_Active, &tn);

	if (tn == NULL || tn == MUIV_NListtree_Active_Off) {

		ae = NULL;
	} else {

		ae = tn->tn_User;
	}

	if (msg->tn_ptr) {
		*(msg->tn_ptr) = tn;
	}

	return (ULONG) ae;
}

DECLARE(GetSelCnt)
{
	GETDATA;
	ULONG selcnt = 0;

	DoMethod(data->Listtree, MUIM_NListtree_Select,
	         MUIV_NListtree_Select_All, MUIV_NListtree_Select_Ask, 0, &selcnt);

	return selcnt;
}

DECLARE(Clear)
{
	GETDATA;

	DoMethod(data->Listtree, MUIM_NListtree_Remove,
	         MUIV_NListtree_Remove_ListNode_Root, MUIV_NListtree_Remove_TreeNode_All, 0);

	return 0;
}

DECLARE(Quiet) // LONG state
{
	GETDATA;

	set(data->Listtree, MUIA_NListtree_Quiet, msg->state);

	return 0;
}

DECLARE(Select) // LONG pos, APTR tn
{
	GETDATA;

	DoMethod(data->Listtree, MUIM_NListtree_Select, msg->tn, MUIV_NListtree_Select_On, 0, NULL);

	return 0;
}

DECLARE(SetActive) // APTR tn
{
	GETDATA;

	set(data->Listtree, MUIA_NListtree_Active, msg->tn);

	return 0;
}

DECLARE(SelectAll)
{
	GETDATA;

	DoMethod(obj, MUIM_avmNListtree_OpenAll);
	DoMethod(data->Listtree, MUIM_NListtree_Select,
	         MUIV_NListtree_Select_All, MUIV_NListtree_Select_On, 0, NULL);

	return 0;
}

DECLARE(SelectNone)
{
	GETDATA;

	DoMethod(data->Listtree, MUIM_NListtree_Select,
	         MUIV_NListtree_Select_All, MUIV_NListtree_Select_Off, 0, NULL);

	return 0;
}

DECLARE(NextSelected) // LONG lastpos
{
	/* Pass -1 to start off. Will return -1 when no more entries are selected. */

	GETDATA;
	LONG lastpos = msg->lastpos;

	if (lastpos == -1) {
		data->last_tn = (APTR) MUIV_NListtree_NextSelected_Start;
	}

	DoMethod(data->Listtree, MUIM_NListtree_NextSelected, &data->last_tn);

	if (data->last_tn == (APTR) MUIV_NListtree_NextSelected_End) {

		data->last_tn = NULL;
		lastpos = -1;

	} else {

		lastpos = DoMethod(data->Listtree, MUIM_NListtree_GetNr, data->last_tn, 0);
	}

	return (ULONG) lastpos;
}

DECLARE(GotoFirst) // ULONG listerpos
{
	/* Not supported by NListtree */

	return 0;
}

DECLARE(GetFirst)
{
	/* Not supported by NListtree */

	return (ULONG) -1;
}

DECLARE(Jump) // ULONG pos
{
	/* Not supported by NListtree */
	return 0;
}

DECLARE(CountEntries)
{
	GETDATA;

	return DoMethod(data->Listtree, MUIM_NListtree_GetNr,
	                NULL, MUIV_NListtree_GetNr_Flag_CountAll);
}

struct MUI_NListtree_TreeNode *ShowNListTree(
	    BOOL DirsOnly,
	    Object *tree_obj,
	    AHN *ahn, struct ArcEntry *Listtree,
	    struct MUI_NListtree_TreeNode *lasttreenode )
{

	/* Recursive! */

	register struct ArcEntry *nextae;
	register struct MUI_NListtree_TreeNode *treeNode = NULL;

	if (!Listtree || !Listtree->ae_IsDir) {
		return NULL;
	}

	if (!lasttreenode) {
		/* If there is no last treenode, then we create the root of the list... */

		Listtree->ae_TreeNode = lasttreenode = (struct MUI_NListtree_TreeNode *)
			DoMethod(tree_obj, MUIM_NListtree_Insert,
				Listtree->ae_Name, Listtree, MUIV_NListtree_Insert_ListNode_Root,
				MUIV_NListtree_Insert_PrevNode_Tail,
				TNF_LIST | (DirsOnly ? TNF_OPEN : 0));
	}

	for (	nextae = (struct ArcEntry *) Listtree->ae_ChildAEL.mlh_Head;
	        nextae->ae_Node.mln_Succ;
	        nextae = (struct ArcEntry *) nextae->ae_Node.mln_Succ) {

		if (nextae->ae_IsDir) {

			nextae->ae_TreeNode = treeNode = (struct MUI_NListtree_TreeNode *)
				DoMethod(tree_obj, MUIM_NListtree_Insert,
					nextae->ae_Name, nextae, lasttreenode,
					MUIV_NListtree_Insert_PrevNode_Tail,
					TNF_LIST | (DirsOnly || nextae->ae_NodeOpened ? TNF_OPEN : 0)
				);

			treeNode = ShowNListTree(DirsOnly, tree_obj, ahn, nextae, treeNode);

		} else if (!DirsOnly) {

			nextae->ae_TreeNode = treeNode = (struct MUI_NListtree_TreeNode *)
	            DoMethod(tree_obj, MUIM_NListtree_Insert,
		            nextae->ae_Name, nextae, lasttreenode,
					MUIV_NListtree_Insert_PrevNode_Tail, 0
				);
		}
	}

	return treeNode;
}

DECLARE(ShowArcEntries) // AHN *ahn
{
	GETDATA;
	Object *Listtree = data->Listtree;
	AHN *ahn = msg->ahn;
	struct PN *pn;

#ifdef DEBUG

	if (ahn == NULL) {
		dbg("WARNING: ShowArcEntries gave avmNListtree a NULL pointer, that shouldn't happen!\n");
	}

	if (ahn->ahn_DebugID != DEBUGID_AHN) {
		dbg("WARNING: ShowArcEntries gave avmNListtree an invalid AHN pointer, that shouldn't happen!\n");
	}
#endif

	pn = ahn->ahn_CurrentPN;

	if (!pn->pn_currentae->ae_ChildAEListSorted) {
		// TODO: offload this and all other to the ARC module

		GUI_PrintStatus("Sorting entries...", 0);
		AETree_Sort(ahn, pn->pn_currentae, SORTMODE_DIRSABOVE);
		pn->pn_currentae->ae_ChildAEListSorted = TRUE;
	}

	GUI_PrintStatus("Displaying tree...", 0);
	set(Listtree, MUIA_NListtree_Quiet, TRUE);
	DoMethod(Listtree, MUIM_NListtree_Clear, NULL, 0);
	ShowNListTree(data->DirsOnly, Listtree, ahn, pn->pn_rootae, NULL);
	set(Listtree, MUIA_NListtree_Quiet, FALSE);
	data->ahn = ahn;

	return 0;
}

DECLARE(SetColumns) // ULONG viewmask
{
	GETDATA;

	ArcView_BuildFormatString(
	    data->viewmask = msg->viewmask, data->buf, sizeof(data->buf)-1);

	set(data->Listtree, MUIA_NListtree_Format, data->buf);

	return 0;
}

DECLARE(ShowColumns) // ULONG viewmask
{
	GETDATA;

	ArcView_BuildFormatString(
	    data->viewmask |= msg->viewmask, data->buf, sizeof(data->buf)-1);

	set(data->Listtree, MUIA_NListtree_Format, data->buf);

	return 0;
}

DECLARE(HideColumns) // ULONG viewmask
{
	GETDATA;

	ArcView_BuildFormatString(
	    data->viewmask &= ~msg->viewmask, data->buf, sizeof(data->buf)-1);

	set(data->Listtree, MUIA_NListtree_Format, data->buf);

	return 0;
}

DECLARE(Disabled) // ULONG yes
{
	GETDATA;

	set(data->Listtree, MUIA_Disabled, msg->yes);
	set(data->Parent, MUIA_Disabled, msg->yes);
	set(data->OpenAll, MUIA_Disabled, msg->yes);
	set(data->CloseAll, MUIA_Disabled, msg->yes);

	return 0;
}

DECLARE(Parent)
{
	GETDATA;
	AHN *ahn = data->ahn;
	struct PN *pn = ahn->ahn_CurrentPN;

	if (!pn || !pn->pn_currentae) {
		return 0;
	}

	pn->pn_currentae->ae_ListerPos = DoMethod(obj, MUIM_ArcView_GetFirst);

	if (ahn->ahn_EmbeddedArc && !pn->pn_currentae->ae_ParentAE) {

		DoMethod(GETARCVIEW, MUIM_ArcView_GotoParentArc, ahn);

	} else if (pn->pn_currentae->ae_ParentAE) {

		pn->pn_currentae = pn->pn_currentae->ae_ParentAE;
		DoMethod(obj, MUIM_avmNListtree_ShowArcEntries, ahn);
	}

	return 0;
}

DECLARE(OpenAll)
{
	GETDATA;

	DoMethod(data->Listtree,
		MUIM_NListtree_Open,
		MUIV_NListtree_Open_ListNode_Root,
		MUIV_NListtree_Open_TreeNode_All, 0
	);

	return 0;
}

DECLARE(CloseAll)
{
	GETDATA;

	DoMethod(data->Listtree,
		MUIM_NListtree_Close,
	    MUIV_NListtree_Close_ListNode_Root,
		MUIV_NListtree_Close_TreeNode_All, 0
	);

	return 0;
}

DECLARE(DoubleClick) // LONG pos
{
	GETDATA;
	struct ArcEntry *ae;
	AHN *ahn = data->ahn;
	struct PN *pn;

	DoMethod(obj, MUIM_avmNListtree_GetActiveAE, &ae);

	if (!ae) {
		return 0;
	}

	pn = ahn->ahn_CurrentPN;

	if (ae->ae_IsDir) {
		if (data->ExplorerObj) {
			DoMethod(data->ExplorerObj, MUIM_avmExplorer_TreeActive, ae->ae_TreeNode);
		}

		pn->pn_currentae = ae;
		DoMethod(obj, MUIM_avmFilesAndDirs_ShowArcEntries, ahn);

		return 0;
	}

	if (data->DirsOnly) {
		return 0;
	}

	DoMethod(GETARCVIEW, MUIM_ArcView_DoDoubleClick, ahn, ae);
	return 0;
}

DECLARE(SingleClick) // LONG pos
{
	GETDATA;

	if (data->ExplorerObj) {
		//
		// If we're part of the Explorer view mode then a single click
		// shows the entries in the Files and Dirs part of the viewmode.
		//

		struct MUI_NListtree_TreeNode *tn = NULL;

		get(data->Listtree, MUIA_NListtree_Active, &tn);

		if (tn == NULL || tn == MUIV_NListtree_Active_Off || tn->tn_User == NULL) {
			return 0;
		}

		DoMethod(data->ExplorerObj, MUIM_avmExplorer_UpdateFilesAndDirsPart, data->ahn, tn->tn_User);

	} else {
		DoMethod(GETARCVIEW, MUIM_ArcView_DoSingleClick);
	}

	return 0;
}
