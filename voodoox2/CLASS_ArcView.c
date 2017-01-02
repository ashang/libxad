/*
 $Id: CLASS_ArcView.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class for archive view mode handling.

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
#include "ArcView.h"
#include "vx_arc.h"
#include "vx_virus.h"
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_misc.h"
#include "vx_mem.h"
#include "vx_cfg.h"
#include "vx_io.h"

// TODO: Write disk class: show disk details in the window
// TODO: Close all results in ghosting


/* CLASSDATA
//------------------------------------------------------------------------------
	Object *ArcEntriesGrp; // Shows list of files
	Object *WriteDiskGrp;  // Shows disk controls
	Object *RegGroup;      // Register group for dual viewmode. Dynamically created,
	                       //   depending on current ahn.
	Object *PLObj;         // PortionList object
	Object *BalObj;        // Balance object (only valid when PLObj is valid)
	Object *PortionLinkGrp;
	LONG	ViewMode;      // The ViewMode that we're currently in (one of MUIV_ArcView_SwapViewMode_#?).
	ULONG   viewmask;      // What columns are currently being shown.
	AHN *ahn;              // The ArcHistoryNode that we're currently displaying.
//------------------------------------------------------------------------------
*/

// !!!HACK ALERT!!!
//------------------------------------------------------------------------------
// Because it's very difficult to pass this BOOL right down to all the display hooks,
// I've hacked up these globals.

BOOL GLOBALHACK_AmigaOSBits = FALSE;
BOOL GLOBALHACK_NewAmigaOSBits = FALSE;

/* EXPORT
//------------------------------------------------------------------------------
enum
{
	MUIV_ArcView_SwapViewMode_None = -1,
	MUIV_ArcView_SwapViewMode_Linear,
	MUIV_ArcView_SwapViewMode_FilesAndDirs,
	MUIV_ArcView_SwapViewMode_Listtree,
	MUIV_ArcView_SwapViewMode_NListtree,
	MUIV_ArcView_SwapViewMode_Explorer
};
 
// Warning: These bitdefs must stay the same with each release, because VX
// stores the mask in the external config file. We don't want the user's column
// settings to mess up when they upgrade to a newer version.
 
// TODO: Rename these to VIEWCOLUMNF_#?
 
#define VIEWCOLUMN_NAME        (1 << 0)
#define VIEWCOLUMN_SIZE        (1 << 1)
#define VIEWCOLUMN_DAY         (1 << 2)
#define VIEWCOLUMN_DATE        (1 << 3)
#define VIEWCOLUMN_TIME        (1 << 4)
#define VIEWCOLUMN_PROT        (1 << 5)
#define VIEWCOLUMN_COMMENT     (1 << 6)
#define VIEWCOLUMN_ENTRYINFO   (1 << 7)
#define VIEWCOLUMN_SUFFIX      (1 << 8)
#define VIEWCOLUMN_AMIGAOSBITS (1 << 29)
#define VIEWCOLUMN_ALLBITS     (1 << 30)
#define VIEWCOLUMN_SHOWBARS    (1 << 31)
 
#define VIEWCOLUMN_ALL (VIEWCOLUMN_NAME | VIEWCOLUMN_SIZE | VIEWCOLUMN_DAY | VIEWCOLUMN_DATE | \
        VIEWCOLUMN_TIME | VIEWCOLUMN_PROT | VIEWCOLUMN_COMMENT | \
        VIEWCOLUMN_ENTRYINFO | VIEWCOLUMN_SUFFIX | VIEWCOLUMN_ALLBITS | VIEWCOLUMN_SHOWBARS)
 
// Special prototypes we need to export. Should be methods but...
 
void ArcView_BuildFormatString( ULONG view, UBYTE *buf, ULONG len );
void ArcView_FillHookArrays( UBYTE **col_array, UBYTE **pp_array, AE *ae, BOOL ShowFullPath );
extern BOOL GLOBALHACK_AmigaOSBits;
extern BOOL GLOBALHACK_NewAmigaOSBits;
 
//------------------------------------------------------------------------------
*/

struct VMD {
	STRPTR clname;
	LONG   vmid;
	ULONG GetActiveAE;
	ULONG GetEntry;
	ULONG GetSelCnt;
	ULONG Clear;
	ULONG Quiet;
	ULONG Select;
	ULONG SelectAll;
	ULONG SelectNone;
	ULONG NextSelected;
	ULONG GotoFirst;
	ULONG GetFirst;
	ULONG Jump;
	ULONG CountEntries;
	ULONG ShowArcEntries;
	ULONG SetColumns;
	ULONG ShowColumns;
	ULONG HideColumns;
	ULONG Disabled;
};

struct VMD VMDetails[] = {
{
	"VX_avmLinear",
	MUIV_ArcView_SwapViewMode_Linear,
    MUIM_avmLinear_GetActiveAE,
    MUIM_avmLinear_GetEntry,
    MUIM_avmLinear_GetSelCnt,
    MUIM_avmLinear_Clear,
    MUIM_avmLinear_Quiet,
    MUIM_avmLinear_Select,
    MUIM_avmLinear_SelectAll,
    MUIM_avmLinear_SelectNone,
    MUIM_avmLinear_NextSelected,
    MUIM_avmLinear_GotoFirst,
    MUIM_avmLinear_GetFirst,
    MUIM_avmLinear_Jump,
    MUIM_avmLinear_CountEntries,
    MUIM_avmLinear_ShowArcEntries,
    MUIM_avmLinear_SetColumns,
    MUIM_avmLinear_ShowColumns,
    MUIM_avmLinear_HideColumns,
    MUIM_avmLinear_Disabled
},
{
	"VX_avmFilesAndDirs",
	MUIV_ArcView_SwapViewMode_FilesAndDirs,
	MUIM_avmFilesAndDirs_GetActiveAE,
	MUIM_avmFilesAndDirs_GetEntry,
	MUIM_avmFilesAndDirs_GetSelCnt,
	MUIM_avmFilesAndDirs_Clear,
	MUIM_avmFilesAndDirs_Quiet,
	MUIM_avmFilesAndDirs_Select,
	MUIM_avmFilesAndDirs_SelectAll,
	MUIM_avmFilesAndDirs_SelectNone,
	MUIM_avmFilesAndDirs_NextSelected,
	MUIM_avmFilesAndDirs_GotoFirst,
	MUIM_avmFilesAndDirs_GetFirst,
	MUIM_avmFilesAndDirs_Jump,
	MUIM_avmFilesAndDirs_CountEntries,
	MUIM_avmFilesAndDirs_ShowArcEntries,
	MUIM_avmFilesAndDirs_SetColumns,
	MUIM_avmFilesAndDirs_ShowColumns,
	MUIM_avmFilesAndDirs_HideColumns,
	MUIM_avmFilesAndDirs_Disabled
},
{
	"VX_avmListtree",
	MUIV_ArcView_SwapViewMode_Listtree,
	MUIM_avmListtree_GetActiveAE,
	MUIM_avmListtree_GetEntry,
	MUIM_avmListtree_GetSelCnt,
	MUIM_avmListtree_Clear,
	MUIM_avmListtree_Quiet,
	MUIM_avmListtree_Select,
	MUIM_avmListtree_SelectAll,
	MUIM_avmListtree_SelectNone,
	MUIM_avmListtree_NextSelected,
	MUIM_avmListtree_GotoFirst,
	MUIM_avmListtree_GetFirst,
	MUIM_avmListtree_Jump,
	MUIM_avmListtree_CountEntries,
	MUIM_avmListtree_ShowArcEntries,
	MUIM_avmListtree_SetColumns,
	MUIM_avmListtree_ShowColumns,
	MUIM_avmListtree_HideColumns,
	MUIM_avmListtree_Disabled
},
{
	"VX_avmNListtree",
	MUIV_ArcView_SwapViewMode_NListtree,
	MUIM_avmNListtree_GetActiveAE,
	MUIM_avmNListtree_GetEntry,
	MUIM_avmNListtree_GetSelCnt,
	MUIM_avmNListtree_Clear,
	MUIM_avmNListtree_Quiet,
	MUIM_avmNListtree_Select,
	MUIM_avmNListtree_SelectAll,
	MUIM_avmNListtree_SelectNone,
	MUIM_avmNListtree_NextSelected,
	MUIM_avmNListtree_GotoFirst,
	MUIM_avmNListtree_GetFirst,
	MUIM_avmNListtree_Jump,
	MUIM_avmNListtree_CountEntries,
	MUIM_avmNListtree_ShowArcEntries,
	MUIM_avmNListtree_SetColumns,
	MUIM_avmNListtree_ShowColumns,
	MUIM_avmNListtree_HideColumns,
	MUIM_avmNListtree_Disabled
},
{
	"VX_avmExplorer",
	MUIV_ArcView_SwapViewMode_Explorer,
	MUIM_avmExplorer_GetActiveAE,
	MUIM_avmExplorer_GetEntry,
	MUIM_avmExplorer_GetSelCnt,
	MUIM_avmExplorer_Clear,
	MUIM_avmExplorer_Quiet,
	MUIM_avmExplorer_Select,
	MUIM_avmExplorer_SelectAll,
	MUIM_avmExplorer_SelectNone,
	MUIM_avmExplorer_NextSelected,
	MUIM_avmExplorer_GotoFirst,
	MUIM_avmExplorer_GetFirst,
	MUIM_avmExplorer_Jump,
	MUIM_avmExplorer_CountEntries,
	MUIM_avmExplorer_ShowArcEntries,
	MUIM_avmExplorer_SetColumns,
	MUIM_avmExplorer_ShowColumns,
	MUIM_avmExplorer_HideColumns,
	MUIM_avmExplorer_Disabled
},
{
	NULL,
	MUIV_ArcView_SwapViewMode_None,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
}
};

struct VMD *GetVMDetails( LONG vmid ) {
	struct VMD *vmd = &VMDetails[0];

	while (vmd->clname) {
		if (vmd->vmid == vmid) {
			return vmd;
		} else {
			vmd++;
		}
	}

	return &VMDetails[0];
}

OVERLOAD(OM_NEW)
{
	struct TagItem *tags = inittags(msg), *tag;

	DEFTMPDATA;
	CLRTMPDATA;

	data.viewmask = CFG_Get(TAGID_MAIN_VIEWCOLUMNS);
	data.ViewMode = MUIV_ArcView_SwapViewMode_Explorer; /* Default */

	while ((tag = NextTagItem(&tags))) {
		switch (tag->ti_Tag) {
			ATTR(InitialVM): data.ViewMode = tag->ti_Data;
			break;
		}
	}

	obj = (Object *) DoSuperNew(cl, obj,
	                            MUIA_Group_Horiz, FALSE,
	                            Child, data.ArcEntriesGrp =
	                                VX_NewObject(GetVMDetails(data.ViewMode)->clname, TAG_DONE),
	                            TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	DoMethod(obj, VMDetails[data.ViewMode].Disabled, TRUE);

	PREPDATA;

	return (ULONG) obj;
}

OVERLOAD(OM_GET)
{
	GETDATA;
	ULONG *store = ((struct opGet *)msg)->opg_Storage;

	switch (((struct opGet *)msg)->opg_AttrID) {
		ATTR(AHN): *store = (ULONG) data->ahn;
		break;
	}

	return DoSuperMethodA(cl, obj, msg);
}

DECLARE(GetActiveAE) // struct ArcEntry **ae_ptr
{
	return DoMethod(DATAREF->ArcEntriesGrp, VMDetails[DATAREF->ViewMode].GetActiveAE, msg->ae_ptr);
}

DECLARE(GetEntry) // LONG pos, APTR *tn_ptr
{
	return DoMethod(DATAREF->ArcEntriesGrp, VMDetails[DATAREF->ViewMode].GetEntry, msg->pos, msg->tn_ptr);
}

DECLARE(GetSelCnt)
{
	return DoMethod(DATAREF->ArcEntriesGrp, VMDetails[DATAREF->ViewMode].GetSelCnt);
}

DECLARE(Clear)
{
	return DoMethod(DATAREF->ArcEntriesGrp, VMDetails[DATAREF->ViewMode].Clear);
}

DECLARE(Quiet) // LONG state
{
	return DoMethod(DATAREF->ArcEntriesGrp, VMDetails[DATAREF->ViewMode].Quiet, msg->state);
}

DECLARE(Select) // LONG pos, APTR tn
{
	return DoMethod(DATAREF->ArcEntriesGrp, VMDetails[DATAREF->ViewMode].Select, msg->pos, msg->tn);
}

DECLARE(SelectAll)
{
	return DoMethod(DATAREF->ArcEntriesGrp, VMDetails[DATAREF->ViewMode].SelectAll);
}

DECLARE(SelectNone)
{
	return DoMethod(DATAREF->ArcEntriesGrp, VMDetails[DATAREF->ViewMode].SelectNone);
}

DECLARE(NextSelected) // LONG lastpos
{
	/* Pass -1 to start off. Will return -1 when no more entries are selected. */
	return DoMethod(DATAREF->ArcEntriesGrp, VMDetails[DATAREF->ViewMode].NextSelected, msg->lastpos);
}

DECLARE(GotoFirst) // ULONG listerpos
{
	return DoMethod(DATAREF->ArcEntriesGrp, VMDetails[DATAREF->ViewMode].GotoFirst, msg->listerpos);
}

DECLARE(GetFirst)
{
	return DoMethod(DATAREF->ArcEntriesGrp, VMDetails[DATAREF->ViewMode].GetFirst);
}

DECLARE(Jump) // ULONG pos
{
	return DoMethod(DATAREF->ArcEntriesGrp, VMDetails[DATAREF->ViewMode].Jump, msg->pos);
}

DECLARE(CountEntries)
{
	return DoMethod(DATAREF->ArcEntriesGrp, VMDetails[DATAREF->ViewMode].CountEntries);
}

DECLARE(ShowArcEntries) // AHN *ahn
{
	Object *mainwin = GETMAINWIN;
	AHN *ahn;
	GETDATA;

	static STRPTR ArcViewPages[] = {
		"Embedded Filesystem", "Disk", NULL
	};

	DB(dbg("ArcView/ShowArcEntries(ahn=%lx)", msg->ahn));

	if ((ahn = msg->ahn) == NULL) {
		ahn = GetActiveAHN();
	}

	DoMethod(data->ArcEntriesGrp, VMDetails[data->ViewMode].Disabled, TRUE);

	if (ahn == NULL) {
		//
		// If there is no AHN to display, then disable the entire ArcView display,
		// clear the lister and wipe the status line. Else enable the ArcView display.
		//
		GUI_PrintStatus("", 0);
		DoMethod(data->ArcEntriesGrp, VMDetails[data->ViewMode].Clear);

		//
		// Clear/disable the portion list object too.
		//

		if (data->PLObj) {
			DoMethod(data->PLObj, MUIM_NList_Clear);
			set(data->PLObj, MUIA_Disabled, TRUE);
		}

		// TODO: Wipe the arc info window
		data->ahn = NULL;

		DoMethod(data->ArcEntriesGrp, VMDetails[data->ViewMode].Disabled, FALSE);
		return 0;
	}

	// Else enable the ArcView display

	DoMethod(data->ArcEntriesGrp, VMDetails[data->ViewMode].Disabled, FALSE);

	// And enable the PortionList object

	if (data->PLObj && xget(data->PLObj, MUIA_Disabled)) {
		set(data->PLObj, MUIA_Disabled, FALSE);
	}

#ifdef DEBUG

	if (ahn->ahn_DebugID == DEBUGID_AHNBAD) {
		dbg("ShowArcEntries WARNING: Given AHN pointer has been freed!\n");

		return 0;
	}

	if (data->ahn == ahn) {
		dbg("WARNING: Given AHN pointer is already being displayed!\n");
	}

	if (!ahn->ahn_CurrentPN) {
		GUI_Popup("Internal error",
		          "CLASS ArcView METHOD ShowArcEntries got NULL ahn pointer!", NULL, "OK");

		return 0;
	}

#endif /* DEBUG */

	if (!ahn->ahn_CurrentPN->pn_ready) {
		if (!ARC_PrepPN(ahn, ahn->ahn_CurrentPN)) {
			return 0;
		}
	}

	DoMethod(obj, MUIM_Group_InitChange);

	//
	// If the current AHN is a disk archive, then we go into dual view mode
	// Dual viewmode is where the filesystem portion and disk geometry
	// information is shown in a register group.
	//

	if (ahn->ahn_IsDiskArc) // Switch to paged viewmode
	{
		if (!data->RegGroup) {
			//
			// Unlink the the ArcEntries group
			//
			DoMethod(obj, OM_REMMEMBER, data->ArcEntriesGrp);
			data->RegGroup = RegisterGroup(ArcViewPages),
			                 MUIA_Register_Frame, TRUE,
			                 Child, data->ArcEntriesGrp,
			                 Child, data->WriteDiskGrp = WriteDiskObject, MUIA_WriteDisk_AHN, ahn, End,
			                                             End;
			DoMethod(obj, OM_ADDMEMBER, data->RegGroup);
		}
	}
	else // Switch to dual viewmode
	{
		if (data->RegGroup) {
			DoMethod(data->RegGroup, MUIM_Group_InitChange);
			DoMethod(data->RegGroup, OM_REMMEMBER, data->ArcEntriesGrp);
			DoMethod(data->RegGroup, MUIM_Group_ExitChange);
			DoMethod(obj, OM_REMMEMBER, data->RegGroup);
			MUI_DisposeObject(data->RegGroup);
			DoMethod(obj, OM_ADDMEMBER, data->ArcEntriesGrp);
			data->RegGroup = NULL;
		}
	}

	//
	// If this archive has multiple portions (> 1) then display them.
	//
	if (ahn->ahn_PNListCnt > 1) {

		if (data->PLObj == NULL) {
			//
			// If there's no PLObj attached, then attach on, with its
			// Balance object.
			//
			DoMethod(obj, OM_ADDMEMBER, (data->BalObj = BalanceObject, End));
			DoMethod(obj, OM_ADDMEMBER, (data->PLObj = PortionListObject, MUIA_PortionList_AHN, ahn, End));
		}

	} else {
		//
		// This archive only contains 1 portions. So remove the list and its
		// balance object.
		//

		if (data->PLObj) {
			DoMethod(obj, OM_REMMEMBER, data->BalObj);
			DoMethod(obj, OM_REMMEMBER, data->PLObj);
			data->BalObj = NULL;
			data->PLObj = NULL;
		}
	}

	DoMethod(obj, MUIM_Group_ExitChange);

	// Update the archive information window (if open)
	DoMethod(GETARCINFOWIN, MUIM_ArcInfoWin_Update, ahn);

	// Call the ShowArcEntries member function

	DoMethod(data->ArcEntriesGrp, VMDetails[data->ViewMode].ShowArcEntries, ahn);
	data->ahn = ahn;

	DoMethod(mainwin, MUIM_MainWin_SetArcPath,
	         (ahn->ahn_EmbeddedArc ? ahn->ahn_EmbeddedListStr : ahn->ahn_Path));

	DoMethod(obj, MUIM_ArcView_ShowArcInfo);

	return 0;
}

DECLARE(SetColumns) // ULONG viewmask
{
	GETDATA;

	data->viewmask = msg->viewmask;
	CFG_Set(TAGID_MAIN_VIEWCOLUMNS, data->viewmask);

	return DoMethod(data->ArcEntriesGrp, VMDetails[data->ViewMode].SetColumns, msg->viewmask);
}

DECLARE(ShowColumns) // ULONG viewmask
{
	GETDATA;

	DB(dbg("ArcView/ShowColumns(viewmask=0x%08lx)", msg->viewmask));
	data->viewmask |= msg->viewmask;
	CFG_Set(TAGID_MAIN_VIEWCOLUMNS, data->viewmask);

	return DoMethod(data->ArcEntriesGrp, VMDetails[data->ViewMode].ShowColumns, msg->viewmask);
}

DECLARE(HideColumns) // ULONG viewmask
{
	GETDATA;

	DB(dbg("ArcView/HideColumns(viewmask=0x%08lx)", msg->viewmask));
	data->viewmask &= ~msg->viewmask;
	CFG_Set(TAGID_MAIN_VIEWCOLUMNS, data->viewmask);

	return DoMethod(data->ArcEntriesGrp, VMDetails[data->ViewMode].HideColumns, msg->viewmask);
}

DECLARE(SwitchPortionNum) // ULONG num
{
	ULONG num = msg->num;
	AHN *ahn = DATAREF->ahn;
	PN *pn;

	for (pn = (struct PN *) ahn->ahn_PNList.mlh_Head;
	        pn->pn_node.mln_Succ;
	        pn = (struct PN *) pn->pn_node.mln_Succ) {

		if (num == 0) {
			DoMethod(obj, MUIM_ArcView_SwitchPortion, pn);
			break;
		}
		--num;
	}

	return 0;
}

DECLARE(SwitchPortion) // struct PN *pn
{
	GETDATA;
	AHN *ahn = data->ahn;

	if (!msg->pn) {
		return 0;
	}

	if (ahn->ahn_CurrentPN == msg->pn) {
		return 0;
	}

	data->ahn->ahn_CurrentPN = msg->pn;
	data->ahn = NULL;
	DoMethod(obj, MUIM_ArcView_ShowArcEntries, data->ahn);

	return 0;
}


// This should really be a method, but...

GPROTO void ArcView_BuildFormatString( ULONG view, UBYTE *buf, ULONG len ) {
	ULONG slen;

	if (!buf || !len) {
		return;
	}

	buf[0] = 0;

	if (view & VIEWCOLUMN_SHOWBARS) {

		if (view & VIEWCOLUMN_NAME)
			strncat(buf, "COL=0 BAR,", len);

		if (view & VIEWCOLUMN_SIZE)
			strncat(buf, "COL=1 BAR,", len);

		if (view & VIEWCOLUMN_DAY)
			strncat(buf, "COL=2 BAR,", len);

		if (view & VIEWCOLUMN_DATE)
			strncat(buf, "COL=3 BAR,", len);

		if (view & VIEWCOLUMN_TIME)
			strncat(buf, "COL=4 BAR,", len);

		if (view & VIEWCOLUMN_PROT)
			strncat(buf, "COL=5 BAR,", len);

		if (view & VIEWCOLUMN_COMMENT)
			strncat(buf, "COL=6 BAR,", len);

		if (view & VIEWCOLUMN_ENTRYINFO)
			strncat(buf, "COL=7 BAR,", len);

		if (view & VIEWCOLUMN_SUFFIX)
			strncat(buf, "COL=8 BAR,", len);

	} else {

		if (view & VIEWCOLUMN_NAME)
			strncat(buf, "COL=0,", len);

		if (view & VIEWCOLUMN_SIZE)
			strncat(buf, "COL=1,", len);

		if (view & VIEWCOLUMN_DAY)

			strncat(buf, "COL=2,", len);
		if (view & VIEWCOLUMN_DATE)
			strncat(buf, "COL=3,", len);

		if (view & VIEWCOLUMN_TIME)
			strncat(buf, "COL=4,", len);

		if (view & VIEWCOLUMN_PROT)
			strncat(buf, "COL=5,", len);

		if (view & VIEWCOLUMN_COMMENT)
			strncat(buf, "COL=6,", len);

		if (view & VIEWCOLUMN_ENTRYINFO)
			strncat(buf, "COL=7,", len);

		if (view & VIEWCOLUMN_SUFFIX)
			strncat(buf, "COL=8,", len);
	}

	/*if (!buf[0]) // Show name if nothing was given...
	{
		strncat(buf, "COL=0", len);
	}	
	else*/

	if (buf[slen = strlen(buf) - 1] == ',') {
		buf[slen] = 0; // Remove the tailing comma...
	}

	DB(dbg("ArcView/BuildFormatString() returned %s", buf))
}

// So should this...

void ArcView_FillHookArrays( UBYTE **col_array, UBYTE **pp_array, AE *ae, BOOL ShowFullPath ) {

	register UBYTE **col = col_array;
	register UBYTE **pp = pp_array;

	if (!ae)  /* Render titles */
	{
		/* [0] */ *col++ = "\33bName";
		/* [1] */ *col++ = "\33bSize";
		/* [2] */ *col++ = "\33bDay";
		/* [3] */ *col++ = "\33bDate";
		/* [4] */ *col++ = "\33bTime";
		/* [5] */ *col++ = "\33bBits";
		/* [6] */ *col++ = "\33bComment";
		/* [7] */ *col++ = "\33bEntryInfo";
		/* [8] */ *col   = "\33bSuffix";

		return;
	}

	if (pp && ae->ae_IsDir) {
		// Make dirs bold...

		/* [0] */ *pp++  = "\338";
		/* [1] */ *pp++  = "\338";
		/* [2] */ *pp++  = "\338\33r";
		/* [3] */ *pp++  = "\338";
		/* [4] */ *pp++  = "\338";
		/* [5] */ *pp++  = "\338";
		/* [6] */ *pp++  = "\338";
		/* [7] */ *pp++  = "\338";
		/* [8] */ *pp    = "\338";

	} else if (pp) {

		// Make files ordinary...

		/* [0] */ *pp++  = "";
		/* [1] */ *pp++  = "";
		/* [2] */ *pp++  = "\33r";	// Right aligned text for day
		/* [3] */ *pp++  = "";
		/* [4] */ *pp++  = "";
		/* [5] */ *pp++  = "";
		/* [6] */ *pp++  = "";
		/* [7] */ *pp++  = "";
		/* [8] */ *pp    = "";
	}

	ARC_ResolveAEDate(ae);
	ARC_ResolveAESize(ae);
	ARC_ResolveAEProt(ae);

	//
	// In Linear/Flat mode, we need to see the whole path.
	//

	if (ShowFullPath) {

		/* [0] */ *col++ = ae->ae_FullPath;

	} else {

		/* [0] */ *col++ = ae->ae_Name ? ae->ae_Name : (UBYTE *) "(no name)";
	}

	/* [1] */ *col++ = (UBYTE *) &ae->ae_SizeBuf;
	/* [2] */ *col++ = ae->ae_DayPart;
	/* [3] */ *col++ = ae->ae_DatePart;
	/* [4] */ *col++ = ae->ae_TimePart;
	/* [5] */ *col++ = ae->ae_ProtBuf;
	/* [6] */ *col++ = ae->ae_xfi ? (ae->ae_xfi->xfi_Comment   ? ae->ae_xfi->xfi_Comment   : (UBYTE *) "") : (UBYTE *) "";
	/* [7] */ *col++ = ae->ae_xfi ? (ae->ae_xfi->xfi_EntryInfo ? ae->ae_xfi->xfi_EntryInfo : (UBYTE *) "") : (UBYTE *) "";

	//
	// Get the file suffix
	//
	//

	if (ae->ae_SuffixPtr) {
		//
		// Suffix was previously found
		//

		/* [8] */ *col = ae->ae_SuffixPtr;

	} else {
		//
		// We need to find it
		//

		UBYTE *p;

		if (ae->ae_IsDir) {

			p = "";

		} else {

			p = strrchr(ae->ae_Name, '.');

			if (p) {
				p++;
			} else {
				p = "";
			}
		}

		/* [8] */ *col = ae->ae_SuffixPtr = p;
	}
}

DECLARE(SwapViewMode) // LONG vm
{
	struct VMD *vmd = GetVMDetails(msg->vm);
	Object *new_ArcEntriesGrp;
	AHN *tmpahn;
	GETDATA;

	DB(dbg("ArcView/SwapViewMode(vm=%ld)", msg->vm));

	if (msg->vm == MUIV_ArcView_SwapViewMode_None || msg->vm == data->ViewMode) {
		return 0;
	}

	//
	// Create the new ArcEntries (avmXXXXX) group object we want to switch to...
	//

	if (!(new_ArcEntriesGrp = VX_NewObject(vmd->clname, TAG_DONE))) {
		GUI_Popup("Internal error", "Unable to create %s ArcView class!!!", &vmd->clname, "Oh No!");
		return 0;
	}

	DoMethod(new_ArcEntriesGrp, VMDetails[data->ViewMode].Disabled, TRUE);

	if (data->RegGroup) // Are we in dual view mode?
	{
		//
		// If we're in dual view mode, then we need to remove the
		// ArcEntriesGrp from inside the register group. We also
		// need to temporarily remove and add the WriteDiskGrp
		// so that the register order remains intact.
		//
		DoMethod(data->RegGroup, MUIM_Group_InitChange);
		DoMethod(data->RegGroup, OM_REMMEMBER, data->ArcEntriesGrp);// Remove the old ArcEntriesGrp
		DoMethod(data->RegGroup, OM_REMMEMBER, data->WriteDiskGrp); // Remove the WriteDiskGrp

		MUI_DisposeObject(data->ArcEntriesGrp);						// Dispose the old ArcEntriesGrp

		DoMethod(data->RegGroup, OM_ADDMEMBER, new_ArcEntriesGrp);	// Add the new ArcEntriesGrp
		DoMethod(data->RegGroup, OM_ADDMEMBER, data->WriteDiskGrp); // Re-add the WriteDiskGrp
		DoMethod(data->RegGroup, MUIM_Group_ExitChange);

	} else {

		DoMethod(obj, MUIM_Group_InitChange);
		DoMethod(obj, OM_REMMEMBER, data->ArcEntriesGrp);	// Remove the old ArcEntriesGrp

		MUI_DisposeObject(data->ArcEntriesGrp);				// Dispose the old ArcEntriesGrp

		DoMethod(obj, OM_ADDMEMBER, new_ArcEntriesGrp);		// Add the new one
		DoMethod(obj, MUIM_Group_ExitChange);
	}

	data->ArcEntriesGrp = new_ArcEntriesGrp;			// Update variables to reflect new changes
	data->ViewMode      = msg->vm;

	tmpahn = data->ahn;
	data->ahn = NULL;				// Fool ShowArcEntries into thinking this a new AHN
	DoMethod(obj, MUIM_ArcView_ShowArcEntries, tmpahn);	// New viewmode is in place, redisplay entries
	CFG_Set(TAGID_MAIN_ARCVIEWMODE, msg->vm);

	return 0;
}

DECLARE(GotoParentArc) // AHN *ahn
{
	GETDATA;

	AHN *ahn = data->ahn;
	BPTR lock;
	ULONG r;
	PN *pn;
	INITAV;

	if (!ahn) {
		return 0;
	}

	pn = ahn->ahn_CurrentPN;

	if (!(ahn->ahn_EmbeddedArc && pn->pn_currentae->ae_ParentAE == NULL)) {
		return 0;
	}

	if (ahn->ahn_EmbeddedSCA) /* Parent archive is not loaded. */
	{
		if (!ARC_LoadNewArchive(ahn->ahn_ParentArcPath, NULL, NULL, FALSE)) {
			GUI_PrgError(STR(SID_ERR_NOPARENTARC, "Couldn't load parent archive: "), &ahn->ahn_ParentArcPath);
		}

		return 0;
	}

	if ((lock = Lock(ahn->ahn_ParentArcPath, SHARED_LOCK))) {

		AHN *parent_ahn;

		if ((parent_ahn = (AHN *) DoMethod(GETARCHISTORY, MUIM_ArcHistory_FindArc, lock, NULL))) {

				DoMethod(av, MUIM_ArcView_ShowArcEntries, parent_ahn);

		} else {

			r = GUI_Popup("Request...",
			              "The parent of this embedded archive...\n"
			              "\n%s\n\n...is not loaded. Would you like to load it again?",
			              &ahn->ahn_ParentArcPath, "Yes|No");

			if (r == 1) { /* Yes */

				if (!ARC_LoadNewArchive(ahn->ahn_ParentArcPath, NULL, NULL, FALSE)) {
					GUI_PrintStatus("Couldn't load parent archive!", 0);
				}

				DoMethod(av, MUIM_ArcView_ShowArcEntries, NULL);

			} else { /* No */

				GUI_PrintStatus("Parent archive was not loaded.", 0);
			}
		}

		UnLock(lock);
	}

	return 0;
}

DECLARE(ShowArcInfo)
{
	AHN *ahn;
	PN *pn;
	UBYTE *sizestr, *expstr, *client_name;

	if ((ahn = GetActiveAHN()) == NULL) {
		return 0;
	}

	pn = ahn->ahn_CurrentPN;

	if (!pn || !pn->pn_currentae) {
		return 0;
	}

	client_name = pn->pn_ai->xai_Client->xc_ArchiverName;

	if (!(sizestr = FormatULONGToVec(ahn->ahn_FIB->fib_Size))) {
		return 0;
	}

	if ((expstr = FormatULONGToVec(pn->pn_expsize))) {
		switch (CFG_Get(TAGID_MAIN_ARCVIEWMODE)) {

		default: /* TODO: revamp */
		case MUIV_ArcView_SwapViewMode_Linear:
		case MUIV_ArcView_SwapViewMode_FilesAndDirs:
		case MUIV_ArcView_SwapViewMode_NListtree:
		case MUIV_ArcView_SwapViewMode_Explorer: {

				ULONG selcnt = DoMethod(obj, MUIM_ArcView_GetSelCnt);

				GUI_PrintStatus(
				    "\33bSize:\33n %s (expanded %s)  "
				    "\33bDate:\33n %s  "
				    "\33bFiles:\33n %lu  "
				    "\33bSelected:\33n %lu  "
				    "\33bType:\33n %s",
				    (ULONG) sizestr, expstr, &ahn->ahn_DateBuf,
				    pn->pn_filecnt, selcnt, client_name);

				break;
			}

		case MUIV_ArcView_SwapViewMode_Listtree: {

				GUI_PrintStatus(
				    "\33bSize:\33n %s (expanded %s)  "
				    "\33bDate:\33n %s  "
				    "\33bFiles:\33n %lu  "
				    "\33bType:\33n %s",
				    (ULONG) sizestr, expstr, &ahn->ahn_DateBuf,
				    pn->pn_filecnt, client_name);

				break;
			}
		}

		MEM_FreeVec(expstr);
	}

	MEM_FreeVec(sizestr);

	return 0;
}

DECLARE(DoSingleClick)
{

	// This method is called whenever the user single clicks on a lister item. IT
	// is invoked by whatever avm#? class is currently in action.

	AE *ae;

	DoMethod(obj, MUIM_ArcView_GetActiveAE, &ae);

	if (!ae) {
		return 0;
	}

	DoMethod(GETEXTFILEINFOWIN, MUIM_ExtFileInfoWin_Update, ae->ae_xfi);

	if (ae->ae_xfi && (ae->ae_xfi->xfi_Flags & XADFIF_LINK)) {

		GUI_PrintStatus(STR(SID_LINKPOINTSTO, "Link points to: %s"),
		                (ULONG) ae->ae_xfi->xfi_LinkName);
	} else {

		DoMethod(obj, MUIM_ArcView_ShowArcInfo);
	}

	return 0;
}

DECLARE(DoDoubleClick) // AHN *ahn, struct ArcEntry *ae
{
	/* This is called when the user double clicks on an archive entry. */

	AHN *embedded_ahn, *ahn = msg->ahn;
	struct ArcEntry *ae = msg->ae;
	Object *ah = GETARCHISTORY;
	UBYTE *ename;

	if (!ae || !ae->ae_xfi) {
		return 0;
	}

	if (ae->ae_xfi->xfi_Flags & XADFIF_LINK) {
		GUI_PrgError("Sorry, you cannot view link entries!", NULL);
		return 0;
	}

	/* Check if the user just clicked on an embedded archive that is
	   already loaded, if so display that entry instead on reloading
	   the archive. */

	if ((embedded_ahn = (AHN *) DoMethod(ah, MUIM_ArcHistory_FindEmbeddedArc, ahn, ae, NULL))) {
		DoMethod(obj, MUIM_ArcView_ShowArcEntries, embedded_ahn);

		return 0;
	}

	GUI_PrintStatus("Please wait, extracting and examining entry...", 0);

	if (!(ename = IO_GetTmpLocation(FilePart(ae->ae_FullPath)))) {
		return 0;
	}

	if (ARC_ExtractFile(ahn, ae, ename, TRUE, NULL, "")) {
		UBYTE *vn;
		BPTR fh;
		ULONG tmp[2];

		DB(dbg("DoDoubleClick: ARC_ExtractFile() was a success. TempName=%s", ename))
		SetProtection(ename, 0);

		if (CFG_Get(TAGID_MAIN_AUTOVIRUSCHECK)) {

			if ((vn = VIRUS_CheckFile(ename))) {
				VIRUS_ShowWarning(ahn, ae, vn);
			}
		}

		if (CFG_Get(TAGID_MAIN_DOEXECHECK)) {

			if ((fh = Open(ename, MODE_OLDFILE))) /* Let's see if it's an executable... */
			{

				if ((Read(fh, &tmp[0], 8) == 8) && tmp[0] == 0x000003f3 && tmp[1] == 0x00000000) {
					/* TODO: Add name check to make sure we're not executing a library or something... */

					tmp[0] = GUI_Popup("Run exe...", "This file looks like an executable, do you want to run it?", NULL, "Yes|No");

					if (tmp[0] == 1) {
						MySystem(ename, 0); /* Yes */
					}
				}

				Close(fh);
			}
		}

		if (ARC_IsArc(ename)) {

			GUI_PrintStatus("Please wait, loading archive from within archive...", 0);
			ARC_LoadNewArchive(ename, ahn, ae, FALSE);
			DoMethod(obj, MUIM_ArcView_ShowArcEntries, NULL);
		} else {

			DoMethod(GETFILETYPESOBJ, MUIM_CfgPageFT_Launch, ae->ae_xfi->xfi_FileName, ename);
		}
	} else {

		DB(dbg("DoDoubleClick: ARC_ExtractFile() failed. Deleting %s", ename))
		IO_DeleteTmpFile(ename);
	}

	return 0;
}

DECLARE(ShowAmigaOSBits)
{
	AHN *tmpahn;
	GETDATA;

	/* GLOBALHACK_#? are special globals that vx_arc.c/ARC_ResolveAEProt()
	   checks. I couldn't find an easy way to pass this down to the display hooks, that's why
	   I've hacked it in as a global. TODO: Remove it and find a cleaner alternative. */

	GLOBALHACK_NewAmigaOSBits = TRUE;
	GLOBALHACK_AmigaOSBits = TRUE;
	tmpahn = data->ahn;
	DATAREF->ahn = NULL;				// Fool ShowArcEntries into thinking this a new AHN

	DoMethod(obj, MUIM_ArcView_ShowArcEntries, tmpahn);	// New viewmode is in place, redisplay entries
	GLOBALHACK_NewAmigaOSBits = FALSE;

	data->viewmask &= ~VIEWCOLUMN_ALLBITS;			// Clear ALLBITS bit
	data->viewmask |= VIEWCOLUMN_AMIGAOSBITS;		// Set AMIGAOSBITS bit
	CFG_Set(TAGID_MAIN_VIEWCOLUMNS, data->viewmask);

	return 0;
}

DECLARE(ShowAllBits)
{
	AHN *tmpahn;
	GETDATA;

	GLOBALHACK_NewAmigaOSBits = TRUE;
	GLOBALHACK_AmigaOSBits = FALSE;
	tmpahn = data->ahn;
	DATAREF->ahn = NULL;				// Fool ShowArcEntries into thinking this is a new AHN

	DoMethod(obj, MUIM_ArcView_ShowArcEntries, tmpahn);	// New viewmode is in place, redisplay entries
	GLOBALHACK_NewAmigaOSBits = FALSE;

	data->viewmask &= ~VIEWCOLUMN_AMIGAOSBITS; // Clear AMIGAOSBITS bit
	data->viewmask |= VIEWCOLUMN_ALLBITS;      // Set ALLBITS bit
	CFG_Set(TAGID_MAIN_VIEWCOLUMNS, data->viewmask);

	return 0;
}

