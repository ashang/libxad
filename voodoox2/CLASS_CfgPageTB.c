/*
 $Id: CLASS_CfgPageTB.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class for the toolbar config page.

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
 *  Class Name: CfgPageTB
 * Description: XAD Settings Page
 *  Superclass: MUIC_Group
 */

#include "vx_include.h"
#include "CfgPageTB.h"
#include "vx_mem.h"
#include "vx_cfg.h"
#include "CLASS_VXToolBar_Defs.h" /* UGLY! UGLY! UGLY! TODO: Remove this include! */
#include "vx_debug.h"
#include "vx_misc.h"
#include "vx_main.h"

#ifndef SPEEDBAR_MCC_H
#include <mui/SpeedBar_mcc.h>
#endif /* SPEEDBAR_MCC_H */

#ifndef SPEEDBARCFG_MCC_H
#include <mui/SpeedBarCfg_mcc.h>
#endif /* SPEEDBARCFG_MCC_H */

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *Config;
	Object *Listview;
	Object *List;
	Object *Add;
	Object *Rem;
	Object *LabelStr;
	Object *ImagePopASL;
	Object *ImageStr;
	Object *ImagePopButton;
	Object *Action;
	Object *Update;
	Object *VXToolBar;
 
	LONG MaxKeywords;
	BOOL SetupDone;
//------------------------------------------------------------------------------
*/

#define LIST_FORMAT_TOOLBAR ""

struct ToolBarEntry {
	UBYTE *tbe_label, *tbe_image_path;
	LONG tbe_action;
};

/* TODO: VXToolbar classes should have methods for generating these arrays... */

static UBYTE *ToolBarActionEntries[] = /* Keep the following two arrays in sync */
{
	"(Insert spacer)", "Select All", "Select None", "Select Pattern",
	"Extract All", "Extract Selected", "Hide", "Quit", "Open archive",
	"Open multipart archive", "Close archive", "Close all archives", "Search",
	"Check for viruses", "Edit settings", "Edit MUI settings",
	"Show XAD information", "Show version information", "Show errors",
	"Show About window", "Show About MUI window",
	NULL
    };

static UBYTE *ToolBarActionKeywords[] =
{
	"INSERT_SPACE", "SELECT_ALL", "SELECT_NONE", "SELECT_PAT", "EXTRACT_ALL",
	"EXTRACT_SEL", "HIDE", "QUIT", "OPEN_ARC", "OPEN_MP_ARC", "CLOSE_ARC",
	"CLOSE_ALL_ARCS", "SEARCH", "CHECK_FOR_VIRUSES", "EDIT_SETT",
	"EDIT_MUI_SETT", "SHOW_XAD_INFO", "SHOW_VER_INFO", "SHOW_ERRORS",
	"SHOW_ABOUT", "SHOW_ABOUT_MUI", NULL
};

SAVEDS ASM(void) ListKill( REG_A1 (struct NList_DestructMessage *nl_dmsg), REG_A2 (Object *list_obj) ) {

	struct ToolBarEntry *tbe = nl_dmsg->entry;

	if (!tbe) return;
	if (tbe->tbe_label) MEM_FreeVec(tbe->tbe_label);
	if (tbe->tbe_image_path) MEM_FreeVec(tbe->tbe_image_path);

	MEM_FreeVec(tbe);
}

SAVEDS ASM(struct ToolBarEntry *) ListMake( REG_A1 (struct NList_ConstructMessage *nl_cmsg), REG_A2 (Object *list_obj) ) {

	struct ToolBarEntry *tbe = nl_cmsg->entry, *new_tbe;

	if (!(new_tbe = MEM_AllocVec(sizeof(struct ToolBarEntry)))) {
		return NULL;
	}

	new_tbe->tbe_action     = tbe->tbe_action;
	new_tbe->tbe_label      = MEM_StrToVec(tbe->tbe_label ? tbe->tbe_label : (UBYTE *) "");
	new_tbe->tbe_image_path = MEM_StrToVec(tbe->tbe_image_path ? tbe->tbe_image_path : (UBYTE *) "");

	if (!new_tbe->tbe_label || !new_tbe->tbe_image_path) {

		struct NList_DestructMessage tmp_dmsg;
		tmp_dmsg.entry = new_tbe;
		ListKill(&tmp_dmsg, list_obj);
		new_tbe = NULL;
	}

	return new_tbe;
}

SAVEDS ASM(void) ListShow( REG_A1 (struct NList_DisplayMessage *nl_dmsg), REG_A2 (Object *list_obj) )
{
	register struct ToolBarEntry *tbe = nl_dmsg->entry;
	register UBYTE **col = (UBYTE **) nl_dmsg->strings;
	register UBYTE **pp  = (UBYTE **) nl_dmsg->preparses;

	if (!tbe) {
		*pp  = "\33b";
		*col = "Toolbar entries";

		return;
	}

	*col = tbe->tbe_label;
}

SAVEDS ASM(LONG) ListSort( REG_A1 (struct NList_CompareMessage *nl_cmsg), REG_A2 (Object *list_obj) )
{
	return 0;
}

OVERLOAD(OM_NEW)
{
	struct MUIS_SpeedBarCfg_Config cfg;
	HOOK(ListMake_Hk, ListMake)
	HOOK(ListKill_Hk, ListKill)
	HOOK(ListShow_Hk, ListShow)
	HOOK(ListSort_Hk, ListSort)
	DEFTMPDATA;
	CLRTMPDATA;

	data.MaxKeywords = -1;

	memset(&cfg, 0, sizeof(struct MUIS_SpeedBarCfg_Config));

	obj = (Object *) DoSuperNew(cl, obj,
		Child, VGroup,
			MUIA_Frame, MUIV_Frame_Group,
			Child, data.Config = SpeedBarCfgObject, End,
		End,
		Child, HGroup,
			Child, VGroup,
				Child, data.Listview = NListviewObject,
					MUIA_VertWeight,			200,
					MUIA_NListview_NList,		data.List = NListObject,
						MUIA_ObjectID,				MAKE_ID('T','B','L','!'),
						MUIA_CycleChain,         	1,
						MUIA_NList_Input,			TRUE,
						MUIA_NList_MultiSelect,		MUIV_NList_MultiSelect_None,
						MUIA_NList_Format,			LIST_FORMAT_TOOLBAR,
						MUIA_NList_Active,			MUIV_NList_Active_Top,
						MUIA_NList_Title,			TRUE,
						MUIA_NList_AutoVisible,  	TRUE,
						MUIA_NList_ConstructHook2,	&ListMake_Hk,
						MUIA_NList_DestructHook2,	&ListKill_Hk,
						MUIA_NList_DisplayHook2,	&ListShow_Hk,
						MUIA_NList_CompareHook2,   	&ListSort_Hk,
						MUIA_NList_MinColSortable, 	0,
						MUIA_NList_DragSortable,   	TRUE,
					End,
				End,
				Child, ColGroup(3),
					Child, data.Add = MyKeyButton("Add", "a", ""),
					Child, RectangleObject, End,
					Child, data.Rem = MyKeyButton("Remove", "r", ""),
				End,
			End,
			Child, ColGroup(2),
				Child, KeyLabel2("Label", 'l'),
				Child, data.LabelStr = StringObject,
					StringFrame,
					MUIA_Disabled,        TRUE,
					MUIA_CycleChain,      1,
					MUIA_ControlChar,     'l',
					MUIA_String_MaxLen,   512,
					MUIA_String_Contents, "",
				End,

				Child, KeyLabel2("Image", 'i'),
				Child, data.ImagePopASL = PopaslObject,
					MUIA_Popstring_String, data.ImageStr = StringObject,
					StringFrame,
						MUIA_Disabled,        TRUE,
						MUIA_CycleChain,      1,
						MUIA_ControlChar,     'i',
						MUIA_String_MaxLen,   512,
						MUIA_String_Contents, "",
					End,
					MUIA_Popstring_Button, data.ImagePopButton = PopButton(MUII_PopDrawer),
					ASLFR_TitleText,      "Please selected an image...",
					ASLFR_DrawersOnly,     FALSE,
					ASLFR_DoSaveMode,      FALSE,
				End,

				Child, KeyLabel2("Action", 'a'),
				Child, data.Action = CycleObject,
					MUIA_CycleChain,    1,
					MUIA_Font,          MUIV_Font_Button,
					MUIA_Cycle_Entries, ToolBarActionEntries,
				End,
			
				Child, VSpace(0),
				Child, data.Update = MyKeyButton("Update Toolbar", "u", ""),
			
				Child, VSpace(0),
				Child, VSpace(0),
			End,
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj)
		return 0;

	cfg.ViewMode = CFG_Get(TAGID_TB_APPEARANCE);
	cfg.Flags    = 0;

	if (CFG_Get(TAGID_TB_SUNNY))
		cfg.Flags |= MUIV_SpeedBarCfg_Sunny;
	if (CFG_Get(TAGID_TB_RAISED))
		cfg.Flags |= MUIV_SpeedBarCfg_Raising;
	if (CFG_Get(TAGID_TB_BORDERLESS))
		cfg.Flags |= MUIV_SpeedBarCfg_Borderless;
	if (CFG_Get(TAGID_TB_SMALL))
		cfg.Flags |= MUIV_SpeedBarCfg_SmallButtons;

	set(data.Config, MUIA_SpeedBarCfg_Config, &cfg);

	// Call method "MUIM_CfgPageTB_TBConfigChanged" in object "obj" when the
	// "MUIA_SpeedBarCfg_Config" attribute changes in object "data.Config"
	//

	DoMethod(data.Config, MUIM_Notify, MUIA_SpeedBarCfg_Config, MUIV_EveryTime,
	         obj, 2, MUIM_CfgPageTB_TBConfigChanged, MUIV_TriggerValue);

	NLIST_SCLICK_METHOD(data.List, MUIM_CfgPageTB_SingleClick)
	BUTTON_METHOD(data.Add,        MUIM_CfgPageTB_Add)
	BUTTON_METHOD(data.Rem,        MUIM_CfgPageTB_Remove)
	BUTTON_METHOD(data.Update,     MUIM_CfgPageTB_Update)
	STRINGCN_METHOD(data.LabelStr, MUIM_CfgPageTB_LabelStrChangeContents)
	STRINGCN_METHOD(data.ImageStr, MUIM_CfgPageTB_ImageStrChangeContents)
	CYCLE_METHOD(data.Action,      MUIM_CfgPageTB_ActionCycle)

	PREPDATA;
	return (ULONG) obj;
}

DECLARE(TBConfigChanged)
{
	// TODO: Remove me
	return 0;
}

OVERLOAD(MUIM_Setup)
{
	//------------------------------------------------------------------------//
	//
	// Do the setup for this class. The VXToolBar class actually handles the
	// loading and parsing of the toolbar config file. Here we're taking the
	// the VXTB struct produced by that class and updating the lister to
	// reflect it. The reason this code is in MUIM_Setup and not OM_NEW is
	// because the GETVXTOOLBAROBJ macro requires the global App pointer to be
	// valid, the App pointer is never valid in any OM_NEW call.
	//
	//------------------------------------------------------------------------//
	//

	GETDATA;
	struct VXToolBar *vxtb;
	struct ToolBarEntry tbe;
	LONG i;

	if (!DoSuperMethodA(cl, obj, msg)) {
		return FALSE;
	}

	if (!data->SetupDone) {
		data->SetupDone = TRUE;
		data->VXToolBar = GETVXTOOLBAROBJ;

		vxtb = (struct VXToolBar *) xget(data->VXToolBar, MUIA_VXToolBar_VXTB);

		if (!vxtb) {
			return FALSE;
		}

		set(data->List, MUIA_NList_Quiet, TRUE);
		DoMethod(data->List, MUIM_NList_Clear);

		for (i = 0; i < vxtb->Cnt; i++) {
			tbe.tbe_label      = vxtb->BtnArray[i].Text;
			tbe.tbe_image_path = vxtb->ImagePathArray[i];
			tbe.tbe_action     = vxtb->ActionArray[i];
			DoMethod(data->List, MUIM_NList_InsertSingle, &tbe, MUIV_NList_Insert_Bottom);
		}

		set(data->List, MUIA_NList_Quiet, FALSE);

		// When Enter is pressed in the label string, then jump to the image path string
		DoMethod(data->LabelStr, MUIM_Notify,
		         MUIA_String_Acknowledge, MUIV_EveryTime, _win(obj),
		         3, MUIM_Set, MUIA_Window_ActiveObject, data->ImageStr);

		// When Enter is pressed in the image path string, then jump to the label string
		DoMethod(data->ImageStr, MUIM_Notify,
		         MUIA_String_Acknowledge, MUIV_EveryTime, _win(obj),
		         3, MUIM_Set, MUIA_Window_ActiveObject, data->LabelStr);
	}

	return TRUE;
}

DECLARE(Update)
{
	//------------------------------------------------------------------------//
	//
	// Update the tool bar on the main window to reflect the new changes
	// made by the user.
	//
	//------------------------------------------------------------------------//
	//

	GETDATA;
	LONG i;

	struct VXToolBar *vxtb;  // Temporary toolbar
	struct ToolBarEntry *tbe = NULL;

	DB(dbg("ToolBarEntry size is %lu\n", sizeof(struct VXToolBar)))

	if (!(vxtb = MEM_AllocVec(sizeof(struct VXToolBar)))) {
		return 0;
	}

	vxtb->ID     = MAKE_ID('V','X','T','B');
	vxtb->Appear = CFG_Get(TAGID_TB_APPEARANCE); // TODO: Fix this
	vxtb->Max    = MAX_SPEEDBAR_BUTTONS;

	for (i = 0; i < MAX_SPEEDBAR_BUTTONS; i++) {
		DoMethod(data->List, MUIM_NList_GetEntry, i, &tbe);

		if (!tbe) {
			break;
		}

		vxtb->ActionArray[i]     = tbe->tbe_action;
		vxtb->ImagePathArray[i]  = tbe->tbe_image_path;
		vxtb->ImageArray[i]      = NULL;
		vxtb->BtnArray[i].Img    = (tbe->tbe_action == TBACTION_SPACER) ? MUIV_SpeedBar_Spacer : i;
		vxtb->BtnArray[i].Text   = tbe->tbe_label;
		vxtb->BtnArray[i].Help   = NULL;
		vxtb->BtnArray[i].Flags  = 0;
		vxtb->BtnArray[i].Class  = NULL;
		vxtb->BtnArray[i].Object = NULL;
	}

	vxtb->Cnt                   = i;
	vxtb->BtnArray      [i].Img = MUIV_SpeedBar_End;
	vxtb->ImagePathArray[i]     = NULL;
	vxtb->ImageArray    [i]     = NULL;

	DoMethod(data->VXToolBar, MUIM_VXToolBar_Update, vxtb, xget(data->Config, MUIA_SpeedBarCfg_Config));

	MEM_FreeVec(vxtb);

	return 0;
}


/*
static LONG GetActionNumFromStr( UBYTE *action_str )
{
	UBYTE **ap = ToolBarActionEntries;
	LONG i = 0;
 
	while (*ap)
	{
		if (Stricmp(*ap, action_str) == 0)
		{
			return i;
		}
 
		++ap;
		++i;
	}	
 
	return 0;
}
*/

static UBYTE *GetActionStrFromNum( struct Data *data, LONG action )
{
	if (data->MaxKeywords == -1) {
		UBYTE **ap = ToolBarActionEntries;

		while (*ap++) {
			data->MaxKeywords += 1;
		}
	}

	if (action > data->MaxKeywords) {
		return "";
	} else {
		return ToolBarActionKeywords[action];
	}
}

DECLARE(SaveToolBar) // STRPTR filename
{
	//------------------------------------------------------------------------//
	//
	// Save the current ToolBar layout to disk.
	//
	//------------------------------------------------------------------------//
	//

	GETDATA;
	struct ToolBarEntry *tbe;
	BOOL success = FALSE;
	BPTR outfh;
	LONG r = 0, i;
	STRPTR filename = msg->filename;
	struct MUIS_SpeedBarCfg_Config *sbcfg;

	get(data->Config, MUIA_SpeedBarCfg_Config, &sbcfg);

	if (sbcfg->Flags & MUIV_SpeedBarCfg_Sunny)        CFG_Set(TAGID_TB_SUNNY,      TRUE);
	if (sbcfg->Flags & MUIV_SpeedBarCfg_Raising)      CFG_Set(TAGID_TB_RAISED,     TRUE);
	if (sbcfg->Flags & MUIV_SpeedBarCfg_Borderless)   CFG_Set(TAGID_TB_BORDERLESS, TRUE);
	if (sbcfg->Flags & MUIV_SpeedBarCfg_SmallButtons) CFG_Set(TAGID_TB_SMALL,      TRUE);

	if (filename == NULL) {
		filename = TOOLBAR_FILENAME;
	}

	DB(dbg("Saving toolbar file as %s", filename));

	if (!(outfh = Open(filename, MODE_NEWFILE))) {
		return FALSE;
	}

	if (DOSBase->dl_lib.lib_Version >= 40) {
		r = SetVBuf(outfh, NULL, BUF_FULL, TOOLBAR_IOBUFFER_SIZE);
	}

	if (r == 0) {
		FPrintf(outfh, TOOLBAR_HEADER);

		for (i = 0;;) {
			DoMethod(DATAREF->List, MUIM_NList_GetEntry, i++, &tbe);

			if (!tbe) {
				break;
			}

			FPrintf(outfh, TOOLBAR_SAVETEMPLATE,
			        tbe->tbe_label, tbe->tbe_image_path, GetActionStrFromNum(data, tbe->tbe_action));
		}

		success = TRUE;
	}

	Close(outfh);

	return success;
}

DECLARE(SingleClick)
{
	//------------------------------------------------------------------------//
	//
	// Called when the user single clicks on a lister item.
	//
	//------------------------------------------------------------------------//
	//

	GETDATA;

	struct ToolBarEntry *tbe = NULL;
	DoMethod(data->List, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, &tbe);

	if (!tbe) {
		return 0;
	}

	set(data->LabelStr, MUIA_String_Contents, tbe->tbe_label);
	set(data->ImageStr, MUIA_String_Contents, tbe->tbe_image_path);
	set(data->Action,   MUIA_Cycle_Active,    tbe->tbe_action);

	if (tbe->tbe_action == TBACTION_SPACER) {

		set(data->LabelStr, MUIA_Disabled, TRUE);
		set(data->ImageStr, MUIA_Disabled, TRUE);

	} else {

		set(data->LabelStr, MUIA_Disabled, FALSE);
		set(data->ImageStr, MUIA_Disabled, FALSE);
	}

	return 0;
}

DECLARE(Add)
{
	//------------------------------------------------------------------------//
	//
	// Called when the Add button is clicked.
	//
	//------------------------------------------------------------------------//
	//

	GETDATA;
	struct ToolBarEntry tbe;
	LONG cnt;

	get(data->List, MUIA_NList_Entries, &cnt);

	if (cnt >= MAX_SPEEDBAR_BUTTONS) {
		LONG bc = MAX_SPEEDBAR_BUTTONS;
		GUI_Popup("Error", "Sorry, you can only have up to %lu buttons!", &bc, "OK");
		return 0;
	}

	tbe.tbe_label      = "<< New Button >>";
	tbe.tbe_image_path = "";
	tbe.tbe_action     = TBACTION_SPACER;

	DoMethod(data->List, MUIM_NList_InsertSingle, &tbe, MUIV_NList_Insert_Bottom);
	set(data->List, MUIA_NList_Active, MUIV_NList_Active_Bottom);

	return 0;
}

DECLARE(Remove)
{
	//------------------------------------------------------------------------//
	//
	// Called when the Remove button is clicked.
	//
	//------------------------------------------------------------------------//
	//

	DoMethod(DATAREF->Listview, MUIM_NList_Remove, MUIV_NList_Remove_Active);

	return 0;
}

DECLARE(LabelStrChangeContents)
{
	//------------------------------------------------------------------------//
	//
	// Called everytime the label string's contents change. Here we're
	// simply updating the ToolBarEntry in the lister to reflect the new
	// contents of the string.
	//
	//------------------------------------------------------------------------//
	//

	GETDATA;
	struct ToolBarEntry *tbe;
	UBYTE *str, *strvec;
	Object *listobj = data->List, *strobj = data->LabelStr;

	DoMethod(listobj, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, &tbe);

	if (tbe == NULL) {
		return 0;
	}

	if ((str = (UBYTE *) xget(strobj, MUIA_String_Contents)) == NULL) {
		return 0;
	}

	if ((strvec = MEM_StrToVec(str)) == NULL) {
		return 0;
	}

	set(listobj, MUIA_NList_Quiet, TRUE);

	if (tbe->tbe_label) {
		MEM_FreeVec(tbe->tbe_label);
	}

	tbe->tbe_label = strvec;
	set(listobj, MUIA_NList_Quiet, FALSE);
	DoMethod(listobj, MUIM_NList_RedrawEntry, tbe);

	return 0;
}

DECLARE(ImageStrChangeContents)
{
	//------------------------------------------------------------------------//
	//
	// Called everytime the image string's contents change. Here we're
	// simply updating the ToolBarEntry in the lister to reflect the new
	// contents of the string.
	//
	//------------------------------------------------------------------------//
	//

	GETDATA;
	struct ToolBarEntry *tbe;
	UBYTE *str, *strvec;

	Object *listobj = data->List, *strobj = data->ImageStr;
	DoMethod(listobj, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, &tbe);

	if (tbe == NULL) {
		return 0;
	}

	if ((str = (UBYTE *) xget(strobj, MUIA_String_Contents)) == NULL) {
		return 0;
	}

	if ((strvec = MEM_StrToVec(str)) == NULL) {
		return 0;
	}

	set(listobj, MUIA_NList_Quiet, TRUE);

	if (tbe->tbe_image_path) {
		MEM_FreeVec(tbe->tbe_image_path);
	}

	tbe->tbe_image_path = strvec;
	set(listobj, MUIA_NList_Quiet, FALSE);

	return 0;
}

DECLARE(ActionCycle)
{
	//------------------------------------------------------------------------//
	//
	// Called everytime the cycle gadget changes. Here we're simply updating
	// the ToolBarEntry in the lister to reflect the new contents of the string.
	//
	//------------------------------------------------------------------------//
	//

	GETDATA;
	struct ToolBarEntry *tbe;

	DoMethod(data->List, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, &tbe);

	if (tbe == NULL) {
		return 0;
	}

	get(data->Action, MUIA_Cycle_Active, &tbe->tbe_action);

	if (tbe->tbe_action == TBACTION_SPACER) {

		set(data->LabelStr, MUIA_Disabled, TRUE);
		set(data->ImageStr, MUIA_Disabled, TRUE);

	} else {

		set(data->LabelStr, MUIA_Disabled, FALSE);
		set(data->ImageStr, MUIA_Disabled, FALSE);
		//set(_win(obj), MUIA_Window_ActiveObject, data->LabelStr);
	}

	return 0;
}
