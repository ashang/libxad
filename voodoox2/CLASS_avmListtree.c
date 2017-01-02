/*
 $Id: CLASS_avmListtree.c,v 1.2 2004/01/25 04:39:16 andrewbell Exp $
  Custom class for Listtree archive view mode (AVM).

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
 *  Class Name: avmListtree
 * Description: Handles the Tree view mode (using Listtree)
 *  Superclass: MUIC_Group
 */

#include "vx_include.h"
#include "avmListtree.h"
#include "vx_arc.h"
#include <mui/Listtree_mcc.h>
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_misc.h"
#include "vx_cfg.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *Listview;
	Object *Listtree;
	Object *Parent;
	Object *OpenAll;
	Object *CloseAll;
	Object *ExplorerObj;
	Object *LinkGroup;
 
	AHN *ahn;
	struct	MUIS_Listtree_TreeNode	*last_tn;
 
	ULONG viewmask;
	UBYTE buf[256];
 
	// Keep 16bit BOOLs at end to avoid alignment issues...
 
	BOOL DirsOnly;
//------------------------------------------------------------------------------
*/

/* EXPORT
void MCC_GetListtreeVerRev( ULONG *verptr, ULONG *revptr );
*/

void MCC_GetListtreeVerRev( ULONG *verptr, ULONG *revptr ) {

	Object *objTree;

	if ((objTree = ListtreeObject, InputListFrame, End)) {

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

#define MULTISEL_NOT_SUPPORTED GUI_Popup("Notice", \
	"Sorry, MUI class Listtree.mcc doesn't support multiple selection.", NULL, "Understood");

SAVEDS ASM(struct ArcEntry *) TreeMakeFunc(
    REG_A0 (struct Hook *hook),
    REG_A2 (APTR mem_pool),
    REG_A1 (struct ArcEntry *ae) )
{
	return ae;
}

SAVEDS ASM(LONG) TreeKillFunc(
    REG_A0 (struct Hook *hook),
    REG_A2 (APTR mem_pool),
    REG_A1 (struct ArcEntry *ae) )
{
	return 0;
}

SAVEDS ASM(LONG) TreeShowFunc(
    REG_A0 (struct Hook *hook),
    REG_A2 (UBYTE **col),
    REG_A1 (struct MUIS_Listtree_TreeNode *treenode) )
{
	register struct ArcEntry *ae;

	if (treenode) {
		ae = treenode->tn_User;
	} else {
		ae = NULL;
	}

	ArcView_FillHookArrays(col, NULL, ae, FALSE);

	if (col && ae && ae->ae_IsDir && ae->ae_Name) {

		strcpy(ae->ae_DisplayBuf, "\338");
		strncat(ae->ae_DisplayBuf, ae->ae_Name, sizeof(ae->ae_DisplayBuf)-1);
		col[0] = ae->ae_DisplayBuf;
	}

	return 0;
}


SAVEDS ASM(LONG) TreeDirsOnlyShowFunc(
    REG_A0 (struct Hook *hook),
    REG_A2 (UBYTE **col),
    REG_A1 (struct MUIS_Listtree_TreeNode *treenode) )
{
	register struct ArcEntry *ae;

	if (!treenode) {
		*col = "\33bName";
		return 1;
	}

	ae = treenode->tn_User;

	if (!ae->ae_IsDir || !ae->ae_Name) {
		return 0;
	}

	/* TODO: Use a DisplayBuf_Valid flag to gain some speed... */

	strcpy(ae->ae_DisplayBuf, "\338");
	strncat(ae->ae_DisplayBuf, ae->ae_Name, sizeof(ae->ae_DisplayBuf)-1);

	*col = ae->ae_DisplayBuf;

	return 1;
}

SAVEDS ASM(LONG) TreeOpenFunc(
    REG_A0 (struct Hook *hook),
    REG_A1 (struct MUIS_Listtree_TreeNode *treenode) )
{
	((struct ArcEntry *)treenode->tn_User)->ae_NodeOpened = TRUE;
	return 0;
}

SAVEDS ASM(LONG) TreeCloseFunc(
    REG_A0 (struct Hook *hook),
    REG_A1 (struct MUIS_Listtree_TreeNode *treenode) )
{
	((struct ArcEntry *)treenode->tn_User)->ae_NodeOpened = FALSE;
	return 0;
}

SAVEDS ASM(LONG) TreeSortFunc(
    REG_A1 (struct ArcEntry *AE1),
    REG_A2 (struct ArcEntry *AE2) )
{
	if (!AE1->ae_Name) {
		return -1;
	}

	if (!AE2->ae_Name) {
		return 1;
	}

	return Stricmp(AE1->ae_Name, AE2->ae_Name);
}

struct Hook TreeMakeHook = {
	{ NULL, NULL }, (void *) TreeMakeFunc, NULL, NULL };

struct Hook TreeKillHook = {
	{ NULL, NULL }, (void *) TreeKillFunc, NULL, NULL };


struct Hook TreeShowHook = {
	{ NULL, NULL }, (void *) TreeShowFunc, NULL, NULL
};

struct Hook TreeDirsOnlyShowHook  = {
	{ NULL, NULL }, (void *) TreeDirsOnlyShowFunc, NULL, NULL
};

struct Hook TreeOpenHook = {
	{ NULL, NULL }, (void *) TreeOpenFunc, NULL, NULL
};

struct Hook TreeCloseHook = {
	{ NULL, NULL }, (void *) TreeCloseFunc, NULL, NULL
};

struct Hook TreeSortHook = {
	{ NULL, NULL }, (void *) TreeSortFunc, NULL, NULL
};

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;

	struct TagItem *tags = inittags(msg), *tag;
	CLRTMPDATA;

	while ((tag = NextTagItem(&tags))) {

		switch(tag->ti_Tag) {
			ATTR(DirsOnly):    data.DirsOnly    = (BOOL)     tag->ti_Data; break;
			ATTR(ExplorerObj): data.ExplorerObj = (Object *) tag->ti_Data; break;
		}
	}

	obj = (Object *) DoSuperNew(cl, obj,
		Child, data.LinkGroup = VGroup,
			Child, data.Listview = ListviewObject,
				MUIA_CycleChain,        1,
				MUIA_Listview_Input,    TRUE,
				MUIA_Listview_DragType, MUIV_Listview_DragType_None,
				MUIA_Listview_List,     data.Listtree = ListtreeObject,
					InputListFrame,
					MUIA_Listtree_MultiSelect,   (data.DirsOnly ? MUIV_Listview_MultiSelect_None : MUIV_Listview_MultiSelect_Always),
					MUIA_Listtree_DisplayHook,   (data.DirsOnly ? &TreeDirsOnlyShowHook          : &TreeShowHook),
					MUIA_Listtree_SortHook,      &TreeSortHook,
					MUIA_Listtree_ConstructHook, &TreeMakeHook,
					MUIA_Listtree_DestructHook,  &TreeKillHook,
					MUIA_Listtree_OpenHook,      &TreeOpenHook,
					MUIA_Listtree_CloseHook,     &TreeCloseHook,
					MUIA_Listtree_Title,         TRUE,
					MUIA_Listtree_EmptyNodes,    TRUE,
				End,
			End,
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		GUI_Popup("Error",
		          "Couldn't create Listtree.mcc group object!\n"
		          "Possible cause: Listtree.mcc 17.53+ is not installed.", NULL, "OK");
		return 0;
	}

	if (data.DirsOnly) {

		set(data.Listtree, MUIA_Listtree_Format, "");

	} else {

		ArcView_BuildFormatString(
		    data.viewmask = CFG_Get(TAGID_MAIN_VIEWCOLUMNS), data.buf, sizeof(data.buf)-1);

		set(data.Listtree, MUIA_Listtree_Format, data.buf);
	}

	DoMethod(data.LinkGroup, MUIM_Group_InitChange);

	if (data.DirsOnly) {

		DoMethod(data.LinkGroup, OM_ADDMEMBER,
		         HGroup,
		         Child, data.OpenAll     = MyKeyButton("Expand",'o',  HELP_MAIN_AETREE_OPENALL),
		         Child, data.CloseAll    = MyKeyButton("Collapse",'c', HELP_MAIN_AETREE_CLOSEALL),
		         End);

	} else {

		DoMethod(data.LinkGroup, OM_ADDMEMBER,
		         HGroup,
		         Child, data.Parent      = MyKeyButton("Parent archive",'p', HELP_MAIN_PARENT),
		         Child, RectangleObject, End,
		         Child, data.OpenAll     = MyKeyButton("Expand",'o',  HELP_MAIN_AETREE_OPENALL),
		         Child, data.CloseAll    = MyKeyButton("Collapse",'c', HELP_MAIN_AETREE_CLOSEALL),
		         End);
	}

	DoMethod(data.LinkGroup, MUIM_Group_ExitChange);

	if (data.Parent) {
		BUTTON_METHOD(data.Parent, MUIM_avmListtree_Parent)
	}

	BUTTON_METHOD(data.OpenAll,  MUIM_avmListtree_OpenAll)
	BUTTON_METHOD(data.CloseAll, MUIM_avmListtree_CloseAll)

	LISTTREE_DCLICK_METHOD(data.Listtree, MUIM_avmListtree_DoubleClick)
	LISTTREE_SCLICK_METHOD(data.Listtree, MUIM_avmListtree_SingleClick)

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

	set(_win(obj), MUIA_Window_DefaultObject, DATAREF->Listview);

	return TRUE;
}

OVERLOAD(OM_SET)
{
	GETDATA;

	struct TagItem *tags = inittags(msg), *tag;

	while ((tag = NextTagItem(&tags))) {

		switch (tag->ti_Tag) {
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
	struct MUIS_Listtree_TreeNode *tn = NULL;

	get(data->Listtree, MUIA_Listtree_Active, &tn);

	if (tn == NULL || tn == MUIV_Listtree_Active_Off) {

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
	struct MUIS_Listtree_TreeNode *tn;

	tn = (struct MUIS_Listtree_TreeNode *)
	     DoMethod(data->Listtree, MUIM_Listtree_GetEntry,
	              MUIV_Listtree_GetEntry_ListNode_Root, msg->pos, 0);

	ae = (tn ? tn->tn_User : NULL);

	if (msg->tn_ptr) {
		*(msg->tn_ptr) = tn;
	}

	return (ULONG) ae;
}

DECLARE(GetSelCnt)
{
	struct ArcEntry *ae = NULL;

	DoMethod(obj, MUIM_avmListtree_GetActiveAE, &ae);

	return ae ? 1UL : 0UL;
}

DECLARE(Clear)
{
	GETDATA;

	DoMethod(data->Listtree, MUIM_Listtree_Remove,
	         MUIV_Listtree_Remove_ListNode_Root, MUIV_Listtree_Remove_TreeNode_All, 0);

	return 0;
}

DECLARE(Quiet) // LONG state
{
	GETDATA;
	set(data->Listtree, MUIA_Listtree_Quiet, msg->state);

	return 0;
}

DECLARE(Select) // LONG pos, APTR tn
{
	return 0;
}

DECLARE(SetActive) // APTR tn
{
	GETDATA;
	set(data->Listtree, MUIA_Listtree_Active, msg->tn);

	return 0;
}

DECLARE(SelectAll)
{
	MULTISEL_NOT_SUPPORTED

	return 0;
}

DECLARE(SelectNone)
{
	MULTISEL_NOT_SUPPORTED

	return 0;
}

DECLARE(NextSelected) // LONG lastpos
{
	return (ULONG) -1;
}

DECLARE(GotoFirst) // ULONG listerpos
{
	/* Not supported by Listtree */

	return 0;
}

DECLARE(GetFirst)
{
	/* Not supported by Listtree */

	return (ULONG) -1;
}

DECLARE(Jump) // ULONG pos
{
	/* Not supported by Listtree */

	return 0;
}

DECLARE(CountEntries)
{
	GETDATA;

	return DoMethod(data->Listtree, MUIM_Listtree_GetNr,
	                NULL, MUIV_Listtree_GetNr_Flags_CountAll);
}

static struct MUIS_Listtree_TreeNode *ShowListTree(
	Object *lt,
	AHN *ahn,
	struct ArcEntry *aetree,
	struct MUIS_Listtree_TreeNode *lasttreenode )
{
	/* Recursive! */

	struct ArcEntry *nextae;
	register struct MUIS_Listtree_TreeNode *treenode = NULL;

	if (!aetree || !aetree->ae_IsDir) {
		return NULL;
	}

	for (nextae = (struct ArcEntry *) aetree->ae_ChildAEL.mlh_Head;
		 nextae->ae_Node.mln_Succ;
		 nextae = (struct ArcEntry *) nextae->ae_Node.mln_Succ) {

		if (nextae->ae_IsDir) {

			if (!lasttreenode) {

				treenode = (struct MUIS_Listtree_TreeNode *)
				           DoMethod(lt, MUIM_Listtree_Insert,
				                    nextae->ae_Name, nextae,
				                    MUIV_Listtree_Insert_ListNode_Root,
				                    MUIV_Listtree_Insert_PrevNode_Tail,
				                    TNF_LIST | (nextae->ae_NodeOpened ? TNF_OPEN : 0));

			} else {

				treenode = (struct MUIS_Listtree_TreeNode *)
				           DoMethod(lt, MUIM_Listtree_Insert,
				                    nextae->ae_Name, nextae, lasttreenode,
				                    MUIV_Listtree_Insert_PrevNode_Tail,
				                    TNF_LIST | (nextae->ae_NodeOpened ? TNF_OPEN : 0));
			}

			treenode = ShowListTree(lt, ahn, nextae, treenode);

		} else {

			treenode = (struct MUIS_Listtree_TreeNode *)
			           DoMethod(lt, MUIM_Listtree_Insert,
			                    nextae->ae_Name, nextae, lasttreenode,
			                    MUIV_Listtree_Insert_PrevNode_Tail, 0);

		}
	}

	return treenode;
}

DECLARE(ShowArcEntries) // AHN *ahn
{
	GETDATA;
	AHN *ahn = msg->ahn;
	struct PN *pn;

#ifdef DEBUG

	if (ahn == NULL) {
		dbg("WARNING: ShowArcEntries gave avmListtree a NULL pointer, that shouldn't happen!\n");
	}

	if (ahn->ahn_DebugID != DEBUGID_AHN) {
		dbg("WARNING: ShowArcEntries gave avmListtree an invalid AHN pointer, that shouldn't happen!\n");
	}
#endif

	pn = ahn->ahn_CurrentPN;

	if (!pn->pn_rootae->ae_ChildAETreeSorted) {
		GUI_PrintStatus("Sorting entries...", 0);
		AETree_Sort(ahn, pn->pn_rootae, SORTMODE_DIRSABOVE);
		pn->pn_rootae->ae_ChildAETreeSorted = TRUE;
	}

	GUI_PrintStatus("Displaying tree...", 0);

	set(data->Listtree, MUIA_Listtree_Quiet, TRUE);

	DoMethod(data->Listtree, MUIM_Listtree_Remove,
	         MUIV_Listtree_Remove_ListNode_Root,
	         MUIV_Listtree_Remove_TreeNode_All, 0);

	ShowListTree(data->Listtree, ahn, pn->pn_rootae, NULL);

	set(data->Listtree, MUIA_Listtree_Quiet, FALSE);

	data->ahn = ahn;

	return 0;
}


DECLARE(SetColumns) // ULONG viewmask
{
	GETDATA;

	ArcView_BuildFormatString(
	    data->viewmask = msg->viewmask, data->buf, sizeof(data->buf)-1);

	set(data->Listtree, MUIA_Listtree_Format, data->buf);

	return 0;
}

DECLARE(ShowColumns) // ULONG viewmask
{
	GETDATA;

	ArcView_BuildFormatString(
	    data->viewmask |= msg->viewmask, data->buf, sizeof(data->buf)-1);

	set(data->Listtree, MUIA_Listtree_Format, data->buf);

	return 0;
}

DECLARE(HideColumns) // ULONG viewmask
{
	GETDATA;

	ArcView_BuildFormatString(
	    data->viewmask &= ~msg->viewmask, data->buf, sizeof(data->buf)-1);

	set(data->Listtree, MUIA_Listtree_Format, data->buf);

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
		DoMethod(obj, MUIM_avmListtree_ShowArcEntries, ahn);
	}

	return 0;
}

DECLARE(OpenAll)
{
	GETDATA;

	DoMethod(data->Listtree, MUIM_Listtree_Open, MUIV_Listtree_Open_ListNode_Root,
	         MUIV_Listtree_Open_TreeNode_All, 0);

	return 0;
}

DECLARE(CloseAll)
{
	GETDATA;

	DoMethod(data->Listtree,
	         MUIM_Listtree_Close,
	         MUIV_Listtree_Close_ListNode_Root,
	         MUIV_Listtree_Close_TreeNode_All, 0);

	return 0;
}

DECLARE(DoubleClick) // LONG pos
{
	GETDATA;
	struct ArcEntry *ae;
	AHN *ahn = data->ahn;
	struct PN *pn;

	DoMethod(obj, MUIM_avmListtree_GetActiveAE, &ae);

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
		struct ArcEntry *ae = NULL;

		DoMethod(obj, MUIM_avmListtree_GetActiveAE, &ae);

		if (!ae) {
			return 0;
		}

		DoMethod(data->ExplorerObj, MUIM_avmExplorer_UpdateFilesAndDirsPart, data->ahn, ae);

	} else {

		DoMethod(GETARCVIEW, MUIM_ArcView_DoSingleClick);
	}

	return 0;
}
