/*
 $Id: CLASS_SettingsWin.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class for the settings window.

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
 *  Class Name: SettingsWin
 * Description: Handles the settings window
 *  Superclass: MUIC_Window
 */

#include "vx_include.h"
#include "SettingsWin.h"
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_misc.h"
#include "vx_cfg.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *Save;
	Object *Cancel;
	Object *SettingsMenuStrip;
	Object *FTObject;
	Object *TBObject;
	struct FileRequester *SettingsReq;
*/

enum {
    MENU_FILETYPES_LOAD = 1,
    MENU_FILETYPES_APPEND,
    MENU_FILETYPES_SAVE,
    MENU_FILETYPES_SAVEAS,
    MENU_SETTINGS_LOAD,
    MENU_SETTINGS_SAVE,
    MENU_SETTINGS_SAVEAS,
    MENU_MAX
};

static UBYTE *SettingsPages[]    = { "Main", "XAD", "Filetypes", "Toolbar", NULL };

static struct NewMenu SettingsMenuData[] = /* Filetypes menu (Settings window) */ {
	nmTitle( "Filetypes", 0)
	nmItem ( "Load...",       "1", 0, MENU_FILETYPES_LOAD           )
	nmItem ( "Append...",     "2", 0, MENU_FILETYPES_APPEND         )
	nmItem ( "Save",          "3", 0, MENU_FILETYPES_SAVE           )
	nmItem ( "Save as...",    "4", 0, MENU_FILETYPES_SAVEAS         )
	nmTitle( "Settings", 0)
	nmItem ( "Load settings...",       "l", 0, MENU_SETTINGS_LOAD   )
	nmItem ( "Save settings",          "s", 0, MENU_SETTINGS_SAVE   )
	nmItem ( "Save settings as...",    "a", 0, MENU_SETTINGS_SAVEAS )
	nmEnd
};

const struct {
	ULONG menu_id;
	ULONG method_id;
}

SettingsMenuMethodMappings[] = {
	{ MENU_SETTINGS_LOAD,    MUIM_SettingsWin_Load },
	{ MENU_SETTINGS_SAVE,    MUIM_SettingsWin_Save },
	{ MENU_SETTINGS_SAVEAS,  MUIM_SettingsWin_SaveAs },
	{ MENU_FILETYPES_LOAD,   MUIM_SettingsWin_LoadFileTypes },
	{ MENU_FILETYPES_APPEND, MUIM_SettingsWin_AppendFileTypes },
	{ MENU_FILETYPES_SAVE,   MUIM_SettingsWin_SaveFileTypes },
	{ MENU_FILETYPES_SAVEAS, MUIM_SettingsWin_SaveFileTypesAs },
	{ 0, 0 }
};

OVERLOAD(OM_NEW)
{
	long i;

	DEFTMPDATA;
	CLRTMPDATA;

	data.SettingsReq = MUI_AllocAslRequestTags(ASL_FileRequest,
	                   ASLFR_NegativeText,    "Cancel",
	                   //ASLFR_InitialLeftEdge, ILeftEdge,
	                   //ASLFR_InitialTopEdge,  ITopEdge,
	                   //ASLFR_InitialWidth,    IWidth,
	                   //ASLFR_InitialHeight,   IHeight,
	                   ASLFR_InitialDrawer,   "PROGDIR:",
	                   ASLFR_Flags2,          (FRF_REJECTICONS),
	                   ASLFR_SleepWindow,     TRUE,
	                   TAG_DONE);

	if (!data.SettingsReq) {
		return FALSE;
	}

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_Title,     "Settings window...",
		MUIA_Window_ID,        MAKE_ID('S','E','T','T'),
		MUIA_Window_Menustrip, data.SettingsMenuStrip = MUI_MakeObject(MUIO_MenustripNM, SettingsMenuData, 0),
		WindowContents, VGroup,
			Child, RegisterGroup(SettingsPages),
				MUIA_Register_Frame, TRUE,
				Child, CfgPageMainObject,                End, /* Page 1 - Main      */
				Child, CfgPageXADObject,                 End, /* Page 2 - XAD       */
				Child, data.FTObject = CfgPageFTObject,  End, /* Page 3 - Filetypes */
				Child, data.TBObject = CfgPageTBObject,  End, /* Page 4 - ToolBar   */
			End,
			Child, HGroup,
				Child, data.Save   = MyKeyButton("Save settings",'s', HELP_ST_SAVE  ),
				Child, data.Cancel = MyKeyButton("Done",         'd', HELP_ST_CANCEL),
			End,
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		MUI_FreeAslRequest(data.SettingsReq);
		return 0;
	}

	/* Link the menu items to their methods... */

	for (i = 0; i < MENU_MAX; i++) {
		ULONG methodID = SettingsMenuMethodMappings[i].method_id;

		if (!methodID) {
			continue;
		}

		DoMethod((Object *)DoMethod(data.SettingsMenuStrip, MUIM_FindUData,
		                            SettingsMenuMethodMappings[i].menu_id),
		         MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 1, methodID);
	}

	// TODO: load settings file here

	CLOSEWIN_METHOD(obj,    MUIM_SettingsWin_CloseWindow)
	BUTTON_METHOD  (data.Save,   MUIM_SettingsWin_Save)
	BUTTON_METHOD  (data.Cancel, MUIM_SettingsWin_Cancel)
	PREPDATA;

	return (ULONG) obj;
}

OVERLOAD(OM_GET)
{
	GETDATA;
	ULONG *store = ((struct opGet *)msg)->opg_Storage;

	switch (((struct opGet *)msg)->opg_AttrID) {
		ATTR(FTObject): *store = (ULONG) data->FTObject;
		break;
	}

	return DoSuperMethodA(cl, obj, msg);
}

OVERLOAD(OM_DISPOSE)
{
	GETDATA;

	if (data->SettingsReq) {
		MUI_FreeAslRequest(data->SettingsReq);
	}

	return DoSuperMethodA(cl, obj, msg);
}

DECLARE(CloseWindow)
{
	set(obj, MUIA_Window_Open, FALSE);

	return 0;
}

DECLARE(Load)
{
	GETDATA;
	UBYTE pathbuf[256];
	struct Window *wndptr = (struct Window *) xget(obj, MUIA_Window_Window);

	if (GUI_Popup("Request...",
	              "Loading new settings will clear all current settings!\n"
	              "Are you sure?", NULL, "Yes|No") != 1) {
		return 0;
	}

	if (!MUI_AslRequestTags(data->SettingsReq,
	                        ASLFR_Window,        wndptr,
	                        ASLFR_RejectIcons,   TRUE,
	                        //ASLFR_InitialDrawer, "PROGDIR:",
	                        ASLFR_InitialFile,   "VX.settings",
	                        ASLFR_DoPatterns,    TRUE,
	                        ASLFR_TitleText,     "Please select a VX settings file to load...",
	                        ASLFR_DoSaveMode,    FALSE,
	                        TAG_DONE)) {
		return 0;
	}

	strncpy(pathbuf, data->SettingsReq->fr_Drawer, sizeof(pathbuf)-1);

	if (AddPart(pathbuf, data->SettingsReq->fr_File, sizeof(pathbuf)-1)) {
		CFG_LoadConfig(pathbuf);
	}

	// TODO: Update GUI here

	return 0;
}

DECLARE(SaveAllSettingsFiles)
{
	GETDATA;
	BOOL err = FALSE;

	/* Note: We must call this before we call CFG_SaveConfig().
	         Since it sets some config tags for us. */

	if (!DoMethod(data->TBObject, MUIM_CfgPageTB_SaveToolBar, NULL)) {
		GUI_PrgError("Error while saving toolbar information!", NULL);
		err = TRUE;
	}

	if (!CFG_SaveConfig(NULL)) {
		GUI_PrgError("Error while saving settings!", NULL);
		err = TRUE;
	}

	if (!DoMethod(data->FTObject, MUIM_CfgPageFT_Save, NULL)) {
		GUI_PrgError("Error while saving filetypes!", NULL);
		err = TRUE;
	}

	if (err) {
		GUI_PrintStatus("Settings not saved correctly.", 0);
	} else {
		GUI_PrintStatus("Settings saved.", 0);
	}

	return 0;
}

DECLARE(Save)
{
	DoMethod(obj, MUIM_SettingsWin_SaveAllSettingsFiles);
	set(obj, MUIA_Window_Open, FALSE);

	return 0;
}

DECLARE(SaveAs)
{
	GETDATA;
	UBYTE pathbuf[256];

	struct Window *wndptr = (struct Window *) xget(obj, MUIA_Window_Window);

	if (MUI_AslRequestTags(data->SettingsReq,
	                       ASLFR_Window,         wndptr,
	                       ASLFR_RejectIcons,    TRUE,
	                       //ASLFR_InitialDrawer,  "PROGDIR:",
	                       ASLFR_InitialFile,    "VX.settings",
	                       ASLFR_TitleText,      "Save VX settings as...",
	                       ASLFR_DoSaveMode,     TRUE,
	                       TAG_DONE)) {

		strncpy(pathbuf, data->SettingsReq->fr_Drawer, sizeof(pathbuf)-1);

		if (AddPart(pathbuf, data->SettingsReq->fr_File, sizeof(pathbuf)-1)) {
			CFG_SaveConfig(pathbuf);
		}
	}

	return 0;
}

DECLARE(LoadFileTypes)
{
	GETDATA;
	UBYTE pathbuf[256];
	struct Window *wndptr = (struct Window *) xget(obj, MUIA_Window_Window);

	if (MUI_AslRequestTags(data->SettingsReq,
	                       ASLFR_Window,         wndptr,
	                       ASLFR_RejectIcons,    TRUE,
	                       //ASLFR_InitialDrawer,  "PROGDIR:",
	                       ASLFR_InitialFile,    "VX.filetypes",
	                       ASLFR_TitleText,      "Load filetypes...",
	                       TAG_DONE)) {

		strncpy(pathbuf, data->SettingsReq->fr_Drawer, sizeof(pathbuf)-1);

		if (AddPart(pathbuf, data->SettingsReq->fr_File, sizeof(pathbuf)-1)) {
			DoMethod(data->FTObject, MUIM_CfgPageFT_Load, pathbuf);
		}
	}

	return 0;
}

DECLARE(AppendFileTypes)
{
	GETDATA;
	UBYTE pathbuf[256];

	struct Window *wndptr = (struct Window *) xget(obj, MUIA_Window_Window);

	if (MUI_AslRequestTags(data->SettingsReq,
	                       ASLFR_Window,         wndptr,
	                       ASLFR_RejectIcons,    TRUE,
	                       //ASLFR_InitialDrawer,  "PROGDIR:",
	                       ASLFR_InitialFile,    "VX.filetypes",
	                       ASLFR_TitleText,      "Load and Append filetypes...",
	                       TAG_DONE)) {
		strncpy(pathbuf, data->SettingsReq->fr_Drawer, sizeof(pathbuf)-1);

		if (AddPart(pathbuf, data->SettingsReq->fr_File, sizeof(pathbuf)-1)) {
			DoMethod(data->FTObject, MUIM_CfgPageFT_Append, pathbuf);
		}
	}

	return 0;
}

DECLARE(SaveFileTypes)
{
	GETDATA;

	if (!DoMethod(data->FTObject, MUIM_CfgPageFT_Save, NULL)) {
		GUI_PrgError("Error while saving filetypes!", NULL);
	}

	return 0;
}

DECLARE(SaveFileTypesAs)
{
	GETDATA;
	UBYTE pathbuf[256];
	struct Window *wndptr = (struct Window *) xget(obj, MUIA_Window_Window);

	if (MUI_AslRequestTags(data->SettingsReq,
	                       ASLFR_Window,         wndptr,
	                       ASLFR_RejectIcons,    TRUE,
	                       ASLFR_InitialFile,    "VX.filetypes",
	                       ASLFR_TitleText,      "Save filetypes as...",
	                       ASLFR_DoSaveMode,     TRUE,
	                       TAG_DONE)) {
		strncpy(pathbuf, data->SettingsReq->fr_Drawer, sizeof(pathbuf)-1);

		if (AddPart(pathbuf, data->SettingsReq->fr_File, sizeof(pathbuf)-1)) {
			DoMethod(data->FTObject, MUIM_CfgPageFT_Save, pathbuf);
		}

	}
	return 0;
}

DECLARE(Cancel)
{
	set(obj, MUIA_Window_Open, FALSE);
	return 0;
}

