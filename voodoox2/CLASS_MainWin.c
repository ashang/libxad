/*
 $Id: CLASS_MainWin.c,v 1.4 2004/01/25 15:22:10 andrewbell Exp $
 Custom class for the main window.

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
 *  Class Name: MainWin
 * Description: Handles the main window
 *  Superclass: MUIC_Window
 */

#include "vx_include.h"
#include "MainWin.h"
#include "vx_mem.h"
#include "vx_arc.h"
#include "vx_virus.h"
#include "vx_cfg.h"
#include "vx_io.h"
#include "vx_main.h"
#include "vx_misc.h"
#include "vx_debug.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *ArcHistory;
	Object *ArcView;
	Object *DestPop;
	Object *DestPopStr;
	Object *DestPopButton;
	Object *DestHistoryListView;
	Object *DestHistoryList;
	Object *DestPopASL;
	Object *DestPopASLButton;
	Object *Status;
	Object *AboutMUIWin;
	Object *VXToolBar;
	Object *RecentMenu;
	Object *MenuStrip;
	struct FileRequester *SaveReq;
	struct FileRequester *OpenReq;
	struct FileRequester *ExtractReq;
	UWORD DestAsl_LastTopEdge;
	UWORD DestAsl_LastLeftEdge;
	UWORD DestAsl_LastWidth;
	UWORD DestAsl_LastHeight;
//------------------------------------------------------------------------------
*/

enum {
    MENU_PROJECT_ABOUT = 1, MENU_PROJECT_ABOUT_MUI, MENU_PROJECT_ABOUT_VERSIONS,
    MENU_PROJECT_SHOW_ERRORS, MENU_PROJECT_XADCLIENTINFO, MENU_PROJECT_HIDE,
    MENU_PROJECT_QUIT, MENU_ARCHIVE, MENU_ARCHIVE_OPEN, MENU_ARCHIVE_OPENRECENT,
    MENU_ARCHIVE_OPENMULTIPARTARC, MENU_ARCHIVE_CLOSE, MENU_ARCHIVE_INSTALL,
    MENU_ARCHIVE_CLOSEALL, MENU_ARCHIVE_INFO, MENU_ARCHIVE_SEARCH,
    MENU_ARCHIVE_COPYTO, MENU_ARCHIVE_MOVETO, MENU_ARCHIVE_DELETEARCFROMDISK,
    MENU_ARCHIVE_SELECTALL, MENU_ARCHIVE_SELECTNONE, MENU_ARCHIVE_SELECTPATTERN,
    MENU_ARCHIVE_EXTRACTSELECTED, MENU_ARCHIVE_EXTRACTALL,
    MENU_ARCHIVE_VIRUSSCAN_THISARCHIVE, MENU_ARCHIVE_VIRUSSCAN_ALLARCHIVES,
    MENU_ARCHIVE_EXTFILEINFO,
    MENU_SETTINGS, MENU_SETTINGS_SETTINGS, MENU_SETTINGS_SETTINGS_MUI,
    MENU_SETTINGS_VIEWMODE_LINEAR, MENU_SETTINGS_VIEWMODE_FILESANDDIRS,
    MENU_SETTINGS_VIEWMODE_TREE, MENU_SETTINGS_VIEWMODE_NLISTTREE,
    MENU_SETTINGS_VIEWMODE_EXPLORER, MENU_SETTINGS_LOAD, MENU_SETTINGS_SAVE,
    MENU_SETTINGS_SAVEAS, MENU_ENABLEDISABLECONSOLE,
    MENU_SETTINGS_VIEWCOLUMN_NAME, MENU_SETTINGS_VIEWCOLUMN_SIZE,
    MENU_SETTINGS_VIEWCOLUMN_DAY, MENU_SETTINGS_VIEWCOLUMN_DATE,
    MENU_SETTINGS_VIEWCOLUMN_TIME, MENU_SETTINGS_VIEWCOLUMN_PROTECTION,
    MENU_SETTINGS_VIEWCOLUMN_COMMENT, MENU_SETTINGS_VIEWCOLUMN_SHOWBARS,
    MENU_SETTINGS_VIEWCOLUMN_ENTRYINFO,
    MENU_SETTINGS_VIEWCOLUMN_SUFFIX,
    MENU_SETTINGS_VIEWCOLUMN_AMIGAOSBITS,
    MENU_SETTINGS_VIEWCOLUMN_ALLBITS,
    MENU_ARCHIVE_VIRUSSCAN_THISARCHIVECX,
    MENU_ARCHIVE_VIRUSSCAN_ALLARCHIVESCX,
    MENU_MAX
};

#define nmSubMut(txt, commkey, flags, mut, mid)  { NM_SUB, txt, commkey, flags, mut, (APTR) mid },

const struct NewMenu MainMenuData[] = {
	nmTitle( "Project",                         0                                     )
	nmItem ( "About...",                   "a", 0, MENU_PROJECT_ABOUT                 )
	nmItem ( "About MUI...",              NULL, 0, MENU_PROJECT_ABOUT_MUI             )
	nmItem ( "About versions...",         NULL, 0, MENU_PROJECT_ABOUT_VERSIONS        )
	nmItem ( "Show errors...",             "e", 0, MENU_PROJECT_SHOW_ERRORS           )
	nmBar
	nmItem ( "XAD client information...",  "i", 0, MENU_PROJECT_XADCLIENTINFO         )
	nmBar
	nmItem ( "Hide",                       "h", 0, MENU_PROJECT_HIDE                  )
	nmItem ( "Quit",                       "q", 0, MENU_PROJECT_QUIT                  )
	nmTitle( "Archive",                            MENU_ARCHIVE                       )
	nmItem ( "Open...",                    "o", 0, MENU_ARCHIVE_OPEN                  )
	nmItem ( "Open multipart...",          "m", 0, MENU_ARCHIVE_OPENMULTIPARTARC      )
	nmItem ( "Close",                      "c", 0, MENU_ARCHIVE_CLOSE                 )
	nmItem ( "Close all...",               "k", 0, MENU_ARCHIVE_CLOSEALL              )
	nmBar
	nmItem ( "Information...",            NULL, 0, MENU_ARCHIVE_INFO                  )
	nmItem ( "Extended File Info...",     NULL, 0, MENU_ARCHIVE_EXTFILEINFO           )
	nmItem ( "Search...",                 NULL, 0, MENU_ARCHIVE_SEARCH                )
	nmBar
	nmItem ( "Install...",                NULL, 0, MENU_ARCHIVE_INSTALL               )
	nmItem ( "Copy to...",                 "[", 0, MENU_ARCHIVE_COPYTO                )
	nmItem ( "Move to...",                 "]", 0, MENU_ARCHIVE_MOVETO                )
	nmItem ( "Delete from disk...",        "d", 0, MENU_ARCHIVE_DELETEARCFROMDISK     )
	nmBar
	nmItem ( "Select",                    NULL, 0, 0                                  )
	nmSub  ( "All",                       "1",  0, MENU_ARCHIVE_SELECTALL             )
	nmSub  ( "None",                      "2",  0, MENU_ARCHIVE_SELECTNONE            )
	nmSub  ( "Pattern...",                "3",  0, MENU_ARCHIVE_SELECTPATTERN         )
	nmItem ( "Extract",                  NULL,  0, 0                                  )
	nmSub  ( "Selected",                 NULL,  0, MENU_ARCHIVE_EXTRACTSELECTED       )
	nmSub  ( "All...",                    "x",  0, MENU_ARCHIVE_EXTRACTALL            )
	nmBar
	nmItem ( "Virus Scan",               NULL,  0, 0                                    )
	nmSub  ( "XVS (This archive)",       "v",   0, MENU_ARCHIVE_VIRUSSCAN_THISARCHIVE   )
	nmSub  ( "XVS (All archives)",       NULL,  0, MENU_ARCHIVE_VIRUSSCAN_ALLARCHIVES   )
	nmSub  ( "CheckX (This archive)",    NULL,  0, MENU_ARCHIVE_VIRUSSCAN_THISARCHIVECX )
	nmSub  ( "CheckX (All archives)",    NULL,  0, MENU_ARCHIVE_VIRUSSCAN_ALLARCHIVESCX )
	nmTitle( "Settings",                           MENU_SETTINGS                      )
	nmItem ( "Settings...",               "s",  0, MENU_SETTINGS_SETTINGS             )
	nmItem ( "Settings MUI...",          NULL,  0, MENU_SETTINGS_SETTINGS_MUI         )
	nmBar
	
#define MUTEX_LINEAR       (1<<0)
#define MUTEX_FILESANDDIRS (1<<1)
#define MUTEX_TREE         (1<<2)
#define MUTEX_NLISTTREE    (1<<3)
#define MUTEX_EXPLORER     (1<<4)

	nmItem  ( "View mode",        NULL, 0, 0                                                                                                                               )
	nmSubMut( "Flat",             "5",  CHECKIT, (MUTEX_FILESANDDIRS | MUTEX_TREE         | MUTEX_NLISTTREE | MUTEX_EXPLORER), MENU_SETTINGS_VIEWMODE_LINEAR       )
	nmSubMut( "Files & Dirs",     "6",  CHECKIT, (MUTEX_LINEAR       | MUTEX_TREE         | MUTEX_NLISTTREE | MUTEX_EXPLORER), MENU_SETTINGS_VIEWMODE_FILESANDDIRS )
	nmSubMut( "Tree (ListTree)",  "7",  CHECKIT, (MUTEX_LINEAR       | MUTEX_FILESANDDIRS | MUTEX_NLISTTREE | MUTEX_EXPLORER), MENU_SETTINGS_VIEWMODE_TREE         )
	nmSubMut( "Tree (NListtree)", "8",  CHECKIT, (MUTEX_LINEAR       | MUTEX_FILESANDDIRS | MUTEX_TREE      | MUTEX_EXPLORER), MENU_SETTINGS_VIEWMODE_NLISTTREE    )
	nmSubMut( "Explorer",         "9",  CHECKIT, (MUTEX_LINEAR       | MUTEX_FILESANDDIRS | MUTEX_NLISTTREE | MUTEX_TREE    ), MENU_SETTINGS_VIEWMODE_EXPLORER     )
	nmItem( "View column",        NULL, 0, 0                                                                                                                               )
	nmSub( "Name",                NULL,  CHECKIT|MENUTOGGLE,         MENU_SETTINGS_VIEWCOLUMN_NAME       )
	nmSub( "Size",                NULL,  CHECKIT|MENUTOGGLE,         MENU_SETTINGS_VIEWCOLUMN_SIZE       )
	nmSub( "Day",                 NULL,  CHECKIT|MENUTOGGLE,         MENU_SETTINGS_VIEWCOLUMN_DAY        )
	nmSub( "Date",                NULL,  CHECKIT|MENUTOGGLE,         MENU_SETTINGS_VIEWCOLUMN_DATE       )
	nmSub( "Time",                NULL,  CHECKIT|MENUTOGGLE,         MENU_SETTINGS_VIEWCOLUMN_TIME       )
	nmSub( "Protection",          NULL,  CHECKIT|MENUTOGGLE,         MENU_SETTINGS_VIEWCOLUMN_PROTECTION )
	nmSub( "Comment",             NULL,  CHECKIT|MENUTOGGLE,         MENU_SETTINGS_VIEWCOLUMN_COMMENT    )
	nmSub( "Entry Info",          NULL,  CHECKIT|MENUTOGGLE,         MENU_SETTINGS_VIEWCOLUMN_ENTRYINFO  )
	nmSub( "Suffix",              NULL,  CHECKIT|MENUTOGGLE,         MENU_SETTINGS_VIEWCOLUMN_SUFFIX     )

#define MUTEX_AMIGAOSBITS  (1<<9)
#define MUTEX_ALLBITS      (1<<10)

	nmSubBar
	nmSubMut( "AmigaOS Bits",            NULL,  CHECKIT,             MUTEX_ALLBITS, MENU_SETTINGS_VIEWCOLUMN_AMIGAOSBITS )
	nmSubMut( "All Bits",                NULL,  CHECKIT,             MUTEX_AMIGAOSBITS, MENU_SETTINGS_VIEWCOLUMN_ALLBITS     )
	nmSubBar
	nmSub( "Show Bars",                  NULL,  CHECKIT|MENUTOGGLE,  MENU_SETTINGS_VIEWCOLUMN_SHOWBARS   )
	nmBar
	nmItem ( "Load settings...",          NULL, 0, MENU_SETTINGS_LOAD            )
	nmItem ( "Save settings",             NULL, 0, MENU_SETTINGS_SAVE            )
	nmItem ( "Save settings as...",       NULL, 0, MENU_SETTINGS_SAVEAS          )
	
#ifdef DEBUG
	nmTitle( "Debug", 0)
	nmItem ( "Enable/Disable debug console", NULL, 0, MENU_ENABLEDISABLECONSOLE)
#endif
	nmEnd
};

const struct {
	ULONG menu_id;
	ULONG method_id;
}

MainMenuMethodMappings[] = {
	{ MENU_PROJECT_ABOUT,                  MUIM_MainWin_ShowAbout                },
	{ MENU_PROJECT_ABOUT_MUI,              MUIM_MainWin_ShowAboutMUI             },
	{ MENU_PROJECT_ABOUT_VERSIONS,         MUIM_MainWin_ShowVerInfo              },
	{ MENU_PROJECT_SHOW_ERRORS,            MUIM_MainWin_ShowErrors               },
	{ MENU_PROJECT_XADCLIENTINFO,          MUIM_MainWin_ShowXADInfo              },
	{ MENU_PROJECT_HIDE,                   MUIM_MainWin_Hide                     },
	{ MENU_PROJECT_QUIT,                   MUIM_MainWin_Quit                     },
	{ MENU_ARCHIVE_OPEN,                   MUIM_MainWin_OpenArc                  },
	{ MENU_ARCHIVE_OPENRECENT,             0,                                    },
	{ MENU_ARCHIVE_OPENMULTIPARTARC,       MUIM_MainWin_OpenMultiPartArc         },
	{ MENU_ARCHIVE_CLOSE,                  MUIM_MainWin_CloseArc                 },
	{ MENU_ARCHIVE_CLOSEALL,               MUIM_MainWin_CloseAllArcs             },
	{ MENU_ARCHIVE_INFO,                   MUIM_MainWin_ShowArcInfoWin           },
	{ MENU_ARCHIVE_EXTFILEINFO,            MUIM_MainWin_ExtendedFileInfo         },
	{ MENU_ARCHIVE_SEARCH,                 MUIM_MainWin_Search                   },
	{ MENU_ARCHIVE_INSTALL,                MUIM_MainWin_Install                  },
	{ MENU_ARCHIVE_COPYTO,                 MUIM_MainWin_CopyArcTo                },
	{ MENU_ARCHIVE_MOVETO,                 MUIM_MainWin_MoveArcTo                },
	{ MENU_ARCHIVE_DELETEARCFROMDISK,      MUIM_MainWin_DeleteArcFromDisk        },
	{ MENU_ARCHIVE_SELECTALL,              MUIM_MainWin_SelAll                   },
	{ MENU_ARCHIVE_SELECTNONE,             MUIM_MainWin_SelNone                  },
	{ MENU_ARCHIVE_SELECTPATTERN,          MUIM_MainWin_SelPat                   },
	{ MENU_ARCHIVE_EXTRACTSELECTED,        MUIM_MainWin_ExtractSel               },
	{ MENU_ARCHIVE_EXTRACTALL,             MUIM_MainWin_ExtractAll               },
	{ MENU_ARCHIVE_VIRUSSCAN_THISARCHIVE,  MUIM_MainWin_VirusScanThis            },
	{ MENU_ARCHIVE_VIRUSSCAN_ALLARCHIVES,  MUIM_MainWin_VirusScanAll             },
	{ MENU_ARCHIVE_VIRUSSCAN_THISARCHIVECX, MUIM_MainWin_VirusScanThisCheckX     },
	{ MENU_ARCHIVE_VIRUSSCAN_ALLARCHIVESCX, MUIM_MainWin_VirusScanAllCheckX      },
	{ MENU_SETTINGS_SETTINGS,              MUIM_MainWin_EditSettings             },
	{ MENU_SETTINGS_SETTINGS_MUI,          MUIM_MainWin_EditMUISettings          },
	{ MENU_SETTINGS_VIEWMODE_LINEAR,       MUIM_MainWin_GotoLinearViewMode       },
	{ MENU_SETTINGS_VIEWMODE_FILESANDDIRS, MUIM_MainWin_GotoFilesAndDirsViewMode },
	{ MENU_SETTINGS_VIEWMODE_TREE,         MUIM_MainWin_GotoListtreeViewMode     },
	{ MENU_SETTINGS_VIEWMODE_NLISTTREE,    MUIM_MainWin_GotoNListtreeViewMode    },
	{ MENU_SETTINGS_VIEWMODE_EXPLORER,     MUIM_MainWin_GotoExplorerViewMode     },
	{ MENU_SETTINGS_VIEWCOLUMN_NAME,       MUIM_MainWin_ViewColumn_Name          },
	{ MENU_SETTINGS_VIEWCOLUMN_SIZE,       MUIM_MainWin_ViewColumn_Size          },
	{ MENU_SETTINGS_VIEWCOLUMN_DAY,        MUIM_MainWin_ViewColumn_Day           },
	{ MENU_SETTINGS_VIEWCOLUMN_DATE,       MUIM_MainWin_ViewColumn_Date          },
	{ MENU_SETTINGS_VIEWCOLUMN_TIME,       MUIM_MainWin_ViewColumn_Time          },
	{ MENU_SETTINGS_VIEWCOLUMN_PROTECTION, MUIM_MainWin_ViewColumn_Protection    },
	{ MENU_SETTINGS_VIEWCOLUMN_COMMENT,    MUIM_MainWin_ViewColumn_Comment       },
	{ MENU_SETTINGS_VIEWCOLUMN_ENTRYINFO,  MUIM_MainWin_ViewColumn_EntryInfo     },
	{ MENU_SETTINGS_VIEWCOLUMN_SHOWBARS,   MUIM_MainWin_ViewColumn_ShowBars      },
	{ MENU_SETTINGS_VIEWCOLUMN_SUFFIX,     MUIM_MainWin_ViewColumn_Suffix        },
	{ MENU_SETTINGS_VIEWCOLUMN_AMIGAOSBITS, MUIM_MainWin_ViewColumn_AmigaOSBits  },
	{ MENU_SETTINGS_VIEWCOLUMN_ALLBITS,     MUIM_MainWin_ViewColumn_AllBits      },
	{ MENU_SETTINGS_LOAD,                  MUIM_MainWin_LoadSettings             },
	{ MENU_SETTINGS_SAVE,                  MUIM_MainWin_SaveSettings             },
	{ MENU_SETTINGS_SAVEAS,                MUIM_MainWin_SaveSettingsAs           },
	{ MENU_ENABLEDISABLECONSOLE,           MUIM_MainWin_FlickDebugConsole        },
	{ 0,                                   0                                     }
};

//----------------------------------------------------------------------------//
// Dest popup stuff
//----------------------------------------------------------------------------//

SAVEDS ASM(void) DestHist_ObjStrFunc( REG_A2 (Object *ListObj), REG_A1 (Object *StrObj) ) {
	UBYTE *str = NULL;
	DoMethod(ListObj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &str);
	set(StrObj, MUIA_String_Contents, str);
}

SAVEDS ASM(LONG) DestHist_StrObjFunc( REG_A2 (Object *ListObj), REG_A1 (Object *StrObj) ) {
	UBYTE *eStr, *Str = (UBYTE *) xget(StrObj, MUIA_String_Contents);
	LONG i;

	for (i = 0;; i++) {
		DoMethod(ListObj, MUIM_List_GetEntry, i, &eStr);

		if (!eStr) {

			set(ListObj, MUIA_List_Active, MUIV_List_Active_Off);
			break;

		} else if (!Stricmp(eStr, Str)) {

			set(ListObj, MUIA_List_Active, i);
			break;
		}
	}
	return TRUE;
}

struct Hook DestHist_ObjStrHook = {
	{ NULL, NULL }, (void *) DestHist_ObjStrFunc, NULL, NULL
};

struct Hook DestHist_StrObjHook = {
	{ NULL, NULL }, (void *) DestHist_StrObjFunc, NULL, NULL
};

SAVEDS ASM(BOOL) DestAslPopup_StartFunc( REG_A2 (Object *PopAsl), REG_A1 (struct TagItem *TL) ) {
	UBYTE *DestPath = NULL;
	struct Data *data = (struct Data *) xget(PopAsl, MUIA_UserData);

	if (!TL || !data) {
		return FALSE;
	}

	#ifdef ENABLE_MUIASLHACK
	/* This implementation is a bit hacky because we're hacking into MUI's taglist. */
	{
		LONG Cnt;
		for (Cnt = -1; Cnt > -5; Cnt--) {
			if (TL[Cnt].ti_Tag == ASLFR_InitialTopEdge)
				TL[Cnt].ti_Data = (ULONG) data->DestAsl_LastTopEdge;
			if (TL[Cnt].ti_Tag == ASLFR_InitialLeftEdge)
				TL[Cnt].ti_Data = (ULONG) data->DestAsl_LastLeftEdge;
			if (TL[Cnt].ti_Tag == ASLFR_InitialWidth)
				TL[Cnt].ti_Data = data->DestAsl_LastWidth;
			if (TL[Cnt].ti_Tag == ASLFR_InitialHeight)
				TL[Cnt].ti_Data = data->DestAsl_LastHeight;
		}
	}
	#endif /* ENABLE_MUIASLHACK */
	
	get(data->DestPopStr, MUIA_String_Contents, &DestPath);

	TL[0].ti_Tag      = ASLFR_InitialHeight;
	TL[0].ti_Data     = (ULONG) data->DestAsl_LastHeight;

	if ((TL[1].ti_Data = (ULONG) DestPath)) {

		TL[1].ti_Tag = ASLFR_InitialDrawer;

	} else {

		TL[1].ti_Tag = TAG_IGNORE;
	}

	TL[2].ti_Tag      = TAG_DONE;
	TL[2].ti_Data     = 0;

	DB(dbg("2 end"));

	return TRUE;
}

SAVEDS ASM(void) DestAslPopup_StopFunc( REG_A2 (Object *PopAsl), REG_A1 (struct FileRequester *FR) ) {

	struct Data *data = (struct Data *) xget(PopAsl, MUIA_UserData);

	if (!data || !FR) {
		return;
	}

	data->DestAsl_LastLeftEdge = FR->fr_LeftEdge;
	data->DestAsl_LastTopEdge  = FR->fr_TopEdge;
	data->DestAsl_LastWidth    = FR->fr_Width;
	data->DestAsl_LastHeight   = FR->fr_Height;

	set(data->DestPopStr, MUIA_String_Contents, FR->fr_Drawer);

	DoMethod(data->DestHistoryList, MUIM_NList_InsertSingle, FR->fr_Drawer, MUIV_NList_Insert_Top);
}

SAVEDS ASM(void) DestAslPopup_AppendEntryFunc( REG_A2 (Object *TrigObj), REG_A1 (ULONG *Param) ) {
	/* This routine is called when somthing set()s the archive string gadget. */

	UBYTE *pathstr;

	struct Data *data = (struct Data *) xget(TrigObj, MUIA_UserData);

	if (!data) {
		return;
	}

	if (!(pathstr = (UBYTE *) xget(data->DestPopStr, MUIA_String_Contents))) {
		return;
	}

	DoMethod(data->DestHistoryList, MUIM_NList_InsertSingle, pathstr, MUIV_NList_Insert_Top);
}

struct Hook DestAslPopup_StartHook = {
	{ NULL, NULL }, (void *) DestAslPopup_StartFunc, NULL, NULL
};

struct Hook DestAslPopup_StopHook = {
	{ NULL, NULL }, (void *) DestAslPopup_StopFunc, NULL, NULL
};

struct Hook DestAslPopup_AppendEntryHook = {
	{ NULL, NULL }, (void *) DestAslPopup_AppendEntryFunc, NULL, NULL
};

OVERLOAD(OM_NEW)
{
	UWORD ILeftEdge, ITopEdge, IWidth, IHeight;
	LONG i;
	ULONG viewMask;
	Object *menuItem;
	DEFTMPDATA;
	CLRTMPDATA;

	GUI_GetInitialAslDims(&ILeftEdge, &ITopEdge, &IWidth, &IHeight);

	data.SaveReq = MUI_AllocAslRequestTags(ASL_FileRequest,
		ASLFR_NegativeText,    "Cancel",
		ASLFR_InitialLeftEdge, ILeftEdge,
		ASLFR_InitialTopEdge,  ITopEdge,
		ASLFR_InitialWidth,    IWidth,
		ASLFR_InitialHeight,   IHeight,
		ASLFR_InitialDrawer,   "Ram:",
		ASLFR_Flags1,          (FRF_DOSAVEMODE),
		ASLFR_Flags2,          (FRF_REJECTICONS),
		ASLFR_SleepWindow,     TRUE,
		TAG_DONE);

	/* This requester is used for the "Open..." menu item. */

	data.OpenReq = MUI_AllocAslRequestTags(ASL_FileRequest,
		ASLFR_NegativeText,    "Cancel",
		ASLFR_InitialLeftEdge, ILeftEdge,
		ASLFR_InitialTopEdge,  ITopEdge,
		ASLFR_InitialWidth,    IWidth,
		ASLFR_InitialHeight,   IHeight,
		ASLFR_InitialDrawer,   CFG_Get(TAGID_MAIN_DEFARCHIVEPATH),
		ASLFR_Flags2,          (FRF_REJECTICONS),
		ASLFR_SleepWindow,     TRUE,
		TAG_DONE);

	/* General purpose extract requester... */

	data.ExtractReq = MUI_AllocAslRequestTags(ASL_FileRequest,
		ASLFR_NegativeText,    "Cancel",
		ASLFR_InitialLeftEdge, ILeftEdge,
		ASLFR_InitialTopEdge,  ITopEdge,
		ASLFR_InitialWidth,    IWidth,
		ASLFR_InitialHeight,   IHeight,
		ASLFR_InitialDrawer,   "Ram:",
		ASLFR_Flags1,          (FRF_DOSAVEMODE),
		ASLFR_Flags2,          (FRF_REJECTICONS),
		ASLFR_SleepWindow,     TRUE,
		TAG_DONE);

	if (!data.SaveReq || !data.OpenReq || !data.ExtractReq) {

		if (data.SaveReq) {
			MUI_FreeAslRequest(data.SaveReq);
		}

		if (data.OpenReq) {
			MUI_FreeAslRequest(data.OpenReq);
		}

		if (data.ExtractReq) {
			MUI_FreeAslRequest(data.ExtractReq);
		}

		return 0;
	}

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_ScreenTitle, VERS " by Andrew Bell. " WEBADDY,
		MUIA_Window_Title,       VERS " Copyright © " YEAR " Andrew Bell. All rights reserved.",
		MUIA_Window_Menustrip,   data.MenuStrip = MUI_MakeObject(MUIO_MenustripNM, MainMenuData, 0),
		MUIA_Window_ID,          MAKE_ID('M','A','I','N'),
		MUIA_Window_AppWindow,   TRUE,
		WindowContents, VGroup,
			Child, data.ArcHistory = ArcHistoryObject,
			End,
			Child, data.ArcView = ArcViewObject,
				MUIA_ArcView_InitialVM, CFG_Get(TAGID_MAIN_ARCVIEWMODE),
			End,
			Child, HGroup,
				Child, KeyLabel2("Dest:",'d'),
				Child, data.DestPop = PopobjectObject,
					MUIA_Popstring_String, data.DestPopStr = StringObject,
						StringFrame,
						MUIA_CycleChain,      1,
						MUIA_ShortHelp,       HELP_MAIN_DESTPOPSTR,
						MUIA_ControlChar,     'd',
						MUIA_String_MaxLen,   512,
					End,
					MUIA_ShortHelp,             HELP_MAIN_DESTPOP,
					MUIA_Popstring_Button,    	data.DestPopButton = PopButton(MUII_PopUp),
					MUIA_Popobject_StrObjHook,	&DestHist_StrObjHook,
					MUIA_Popobject_ObjStrHook,	&DestHist_ObjStrHook,
					MUIA_Popobject_Object,		data.DestHistoryListView = NListviewObject,
						MUIA_NListview_NList,	data.DestHistoryList = NListObject,
							InputListFrame,
							MUIA_MaxHeight,     	     	50,
							MUIA_NList_AutoVisible, 		TRUE,
							MUIA_NList_ConstructHook,		MUIV_NList_ConstructHook_String,
							MUIA_NList_DestructHook,		MUIV_NList_DestructHook_String,
						End,
					End,
				End,
				Child, data.DestPopASL = PopaslObject,
					MUIA_ShortHelp,			HELP_MAIN_DESTPOPASL,
					MUIA_Popstring_Button,	data.DestPopASLButton = PopButton(MUII_PopDrawer),
					MUIA_Popasl_StartHook,	&DestAslPopup_StartHook,
					MUIA_Popasl_StopHook,	&DestAslPopup_StopHook,
					MUIA_Popasl_Type,		ASL_FileRequest,
					ASLFR_TitleText,		"Please select a destination drawer...",
					ASLFR_DrawersOnly,		TRUE,
					ASLFR_DoSaveMode,		TRUE,
				End,
			End,
			Child, data.Status = TextObject,
				TextFrame,
				MUIA_ShortHelp,     HELP_MAIN_STATUS,
				MUIA_Text_PreParse, "\33c",
				MUIA_Background,    MUII_TextBack,
			End,
			Child, data.VXToolBar = VXToolBarObject,
				MUIA_VXToolBar_Load, NULL,
			End,
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	DoMethod(data.VXToolBar, MUIM_VXToolBar_LinkTriggers, obj);
	CLOSEWIN_METHOD(obj, MUIM_MainWin_CloseWindow);

	DoMethod(data.DestHistoryList, MUIM_Notify,
	         MUIA_NList_DoubleClick, MUIV_EveryTime, data.DestPop,
	         2, MUIM_Popstring_Close, TRUE);

	DoMethod(data.DestPopStr, MUIM_Notify,
	         MUIA_String_Acknowledge, MUIV_EveryTime, data.DestPopStr,
	         3, MUIM_CallHook, &DestAslPopup_AppendEntryHook, MUIV_TriggerValue);

	set(data.DestPopStr, MUIA_UserData, INST_DATA(cl, obj)); /* AppendEntryHook needs this */
	set(data.DestPopASL, MUIA_UserData, INST_DATA(cl, obj)); /* StartHook needs this */

	DoMethod(obj, MUIM_Notify, MUIA_AppMessage, MUIV_EveryTime, obj,
	         2, MUIM_MainWin_AppMsg, MUIV_TriggerValue);
	//
	// Create the recent menu object and add it to main menus...
	//

	if ((data.RecentMenu = RecentMenuObject, End)) {

		Object *arcmenu = (Object *) DoMethod(data.MenuStrip, MUIM_FindUData, MENU_ARCHIVE_OPEN);
		DoMethod(data.MenuStrip, MUIM_Family_Insert, data.RecentMenu, arcmenu);
		DoMethod(data.RecentMenu, MUIM_RecentMenu_Load, NULL);
		set(data.ArcHistory, MUIA_ArcHistory_RecentMenu, data.RecentMenu);
	}

	set(data.DestPopButton,    MUIA_CycleChain, 1);
	set(data.DestPopASLButton, MUIA_CycleChain, 1);
	set(data.DestPopStr,       MUIA_String_Contents, CFG_Get(TAGID_MAIN_DEFDESTPATH));

	DoMethod(data.DestHistoryList, MUIM_NList_InsertSingle,
	         CFG_Get(TAGID_MAIN_DEFDESTPATH), MUIV_NList_Insert_Top);

	GUI_GetInitialAslDims(&data.DestAsl_LastLeftEdge, &data.DestAsl_LastTopEdge,
	                      &data.DestAsl_LastWidth, &data.DestAsl_LastHeight);

	/* Link the menu items to their methods... */

	for (i = 0; i < MENU_MAX; i++) {
		ULONG methodID = MainMenuMethodMappings[i].method_id;

		if (!methodID) {
			continue;
		}

		DoMethod((Object *)DoMethod(data.MenuStrip, MUIM_FindUData, MainMenuMethodMappings[i].menu_id),
		         MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 1, methodID);
	}

	/* Figure out what view mode menu item should be ticked... */

	switch (CFG_Get(TAGID_MAIN_ARCVIEWMODE)) {

	case MUIV_ArcView_SwapViewMode_Linear:
		menuItem = (Object *) DoMethod(data.MenuStrip, MUIM_FindUData,
		                               MENU_SETTINGS_VIEWMODE_LINEAR);
		break;

	case MUIV_ArcView_SwapViewMode_FilesAndDirs:
		menuItem = (Object *) DoMethod(data.MenuStrip, MUIM_FindUData,
		                               MENU_SETTINGS_VIEWMODE_FILESANDDIRS);
		break;

	case MUIV_ArcView_SwapViewMode_Listtree:
		menuItem = (Object *) DoMethod(data.MenuStrip, MUIM_FindUData,
		                               MENU_SETTINGS_VIEWMODE_TREE);
		break;

	case MUIV_ArcView_SwapViewMode_NListtree:
		menuItem = (Object *) DoMethod(data.MenuStrip, MUIM_FindUData,
		                               MENU_SETTINGS_VIEWMODE_NLISTTREE);
		break;

	case MUIV_ArcView_SwapViewMode_Explorer:
	default:
		menuItem = (Object *) DoMethod(data.MenuStrip, MUIM_FindUData,
		                               MENU_SETTINGS_VIEWMODE_EXPLORER);
		break;
	}

	/* Tick it... */

	if (menuItem) {
		set(menuItem, MUIA_Menuitem_Checked, TRUE);
	}

	/* Figure out what view columns should be ticked... */

	viewMask = CFG_Get(TAGID_MAIN_VIEWCOLUMNS);

	if (viewMask & VIEWCOLUMN_NAME)
		set((Object *)DoMethod(data.MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_NAME), MUIA_Menuitem_Checked, TRUE);

	if (viewMask & VIEWCOLUMN_SIZE)
		set((Object *)DoMethod(data.MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_SIZE), MUIA_Menuitem_Checked, TRUE);

	if (viewMask & VIEWCOLUMN_DAY)
		set((Object *)DoMethod(data.MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_DAY), MUIA_Menuitem_Checked, TRUE);

	if (viewMask & VIEWCOLUMN_DATE)
		set((Object *)DoMethod(data.MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_DATE), MUIA_Menuitem_Checked, TRUE);

	if (viewMask & VIEWCOLUMN_TIME)
		set((Object *)DoMethod(data.MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_TIME), MUIA_Menuitem_Checked, TRUE);

	if (viewMask & VIEWCOLUMN_PROT)
		set((Object *)DoMethod(data.MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_PROTECTION), MUIA_Menuitem_Checked, TRUE);

	if (viewMask & VIEWCOLUMN_COMMENT)
		set((Object *)DoMethod(data.MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_COMMENT), MUIA_Menuitem_Checked, TRUE);

	if (viewMask & VIEWCOLUMN_ENTRYINFO)
		set((Object *)DoMethod(data.MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_ENTRYINFO), MUIA_Menuitem_Checked, TRUE);

	if (viewMask & VIEWCOLUMN_SHOWBARS)
		set((Object *)DoMethod(data.MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_SHOWBARS), MUIA_Menuitem_Checked, TRUE);

	if (viewMask & VIEWCOLUMN_SUFFIX)
		set((Object *)DoMethod(data.MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_SUFFIX), MUIA_Menuitem_Checked, TRUE);

	if (viewMask & VIEWCOLUMN_ALLBITS) {

		GLOBALHACK_AmigaOSBits = FALSE;
		set((Object *)DoMethod(data.MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_ALLBITS), MUIA_Menuitem_Checked, TRUE);

	} else { // Default to Amiga OS bits

		GLOBALHACK_AmigaOSBits = TRUE;
		set((Object *)DoMethod(data.MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_AMIGAOSBITS), MUIA_Menuitem_Checked, TRUE);
	}

	PREPDATA;

	return (ULONG) obj;
}

OVERLOAD(OM_DISPOSE)
{
	GETDATA;

	if (data->RecentMenu) {
		DoMethod(data->RecentMenu, MUIM_RecentMenu_Save, NULL);
	}

	if (data->OpenReq) {
		MUI_FreeAslRequest(data->OpenReq);
	}

	if (data->SaveReq) {
		MUI_FreeAslRequest(data->SaveReq);
	}

	if (data->ExtractReq) {
		MUI_FreeAslRequest(data->ExtractReq);
	}

	return DoSuperMethodA(cl, obj, msg);
}

OVERLOAD(OM_GET)
{
	GETDATA;
	ULONG *store = ((struct opGet *)msg)->opg_Storage;

	switch (((struct opGet *)msg)->opg_AttrID) {
		ATTR(ArcHistory): *store = (ULONG) data->ArcHistory; break;
		ATTR(ArcView):    *store = (ULONG) data->ArcView;    break;
		ATTR(VXToolBar):  *store = (ULONG) data->VXToolBar;  break;
	}

	return DoSuperMethodA(cl, obj, msg);
}

DECLARE(CloseWindow)
{
	set(obj, MUIA_Window_Open, FALSE);
	DoMethod(_app(obj), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	return 0;
}

DECLARE(AppMsg) // struct AppMessage *AppMsgPtr
{
	struct AppMessage *CurAppMsg = msg->AppMsgPtr;
	switch (CurAppMsg->am_Type) {
		default:
			break;
	
		case AMTYPE_APPICON:
			GUI_Iconify(FALSE);
			if (CurAppMsg->am_ArgList) {
				ARC_LoadArcsFromWBArgs(CurAppMsg->am_ArgList, CurAppMsg->am_NumArgs, TRUE);
			}
			GUI_Iconify(TRUE);
			break;
	
		case AMTYPE_APPMENUITEM:
			break;
	
		case AMTYPE_APPWINDOW:
			if (CurAppMsg->am_ArgList) {
				ARC_LoadArcsFromWBArgs(CurAppMsg->am_ArgList, CurAppMsg->am_NumArgs, TRUE);
			}
			break;
	}

	return 0;
}

DECLARE(Install)
{
	AHN *ahn;
	UBYTE *fmtData[2], *tmpDir, *tmpLog = NULL;
	AE **aeArray, *ae = NULL;
	BPTR lock, oldCD;

	if (!(ahn = (AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN))) {
		return 0;
	}

	if ((aeArray = ARC_GetSelected(ahn, NULL, NULL, NULL, FALSE))) {
		ae = aeArray[0];
		ARC_FreeSelected(aeArray);
	}

	if (!ae) {
		GUI_Popup("Install", "Please select this archive's installer script.", NULL, "OK");
		return 0;
	}

	fmtData[0] = (UBYTE *) ae->ae_FullPath;

	if (!(fmtData[1] = FormatULONGToVec(ahn->ahn_CurrentPN->pn_expsize))) {
		return 0;
	}

	if (GUI_Popup("Request...",
	              "Are you sure you want run the selected installer script '%s'?\n\n"
	              "The entire archive must be extracted to your temporary directory first.\n"
	              "Which will require %s of disk space.",
	              &fmtData, "Yes, install it|No, don't install it") == 0) {

		/* Reponse was NO */

		GUI_PrintStatus("Archive not extracted", 0);

	} else {

		/* Reponse was YES */

		if ((tmpDir = IO_GetTmpDir(NULL)) && (tmpLog = IO_GetTmpLocation( "installation-log.txt" ))) {

			UBYTE scriptCmd[256];

			strncpy(scriptCmd, "installer ", sizeof(scriptCmd)-1);
			strncat(scriptCmd, tmpDir, sizeof(scriptCmd)-1);

			if (AddPart(scriptCmd, ae->ae_FullPath, sizeof(scriptCmd)-1)) {

				strncat(scriptCmd, " LOGFILE ", sizeof(scriptCmd)-1);
				strncat(scriptCmd, tmpLog, sizeof(scriptCmd)-1);
				GUI_PrintStatus("Please wait, extracting installation files...", 0);
				ARC_DoExtract(tmpDir, TRUE, TRUE);

				// TODO: Add virus scanning support here

				if ((lock = Lock(tmpDir, SHARED_LOCK))) {

					oldCD = CurrentDir(lock);
					GUI_PrintStatus("Installing...", 0);

					DB(dbg("Running installer command line: %s", scriptCmd);)

					MySystem(scriptCmd, 0); // LOGFILE
					CurrentDir(oldCD);
					UnLock(lock);

					// TODO: Recursive delete here
					/* Let's see if the installer created a log file... */

					if ((lock = Lock(tmpLog, SHARED_LOCK))) {
						UnLock(lock);

						if (GUI_Popup("Request...",
						              "The installer produced a log file, would you like to read it?",
						              &fmtData, "Yes|No") == 0) {

							/* Reponse was NO */

						} else {

							/* Reponse was YES */

							DoMethod(GETFILETYPESOBJ, MUIM_CfgPageFT_Launch, FilePart(tmpLog), tmpLog);
						}
					}
				}
			}
		}

		if (tmpDir) {
			IO_DeleteTmpFile(tmpDir);
		}

		if (tmpLog) {
			IO_DeleteTmpFile(tmpLog);
		}
	}

	MEM_FreeVec(fmtData[1]);

	return 0;
}

DECLARE(CopyArcTo)
{
	GETDATA;
	AHN *ahn = (AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN);
	UBYTE pathBuf[256], *destName;

	if (!ahn) {
		DisplayBeep(NULL);
		return 0;
	}

	if (ahn->ahn_EmbeddedArc) {
		destName = ahn->ahn_EmbeddedPath;
	} else {
		destName = (FilePart(ahn->ahn_Path)[0] == 0 ? data->SaveReq->fr_File : ahn->ahn_Path);
	}

	destName = FilePart(destName);

	if (MUI_AslRequestTags(data->SaveReq,
	                       ASLFR_Window,       xget(obj, MUIA_Window_Window),
	                       ASLFR_TitleText,    "Copy archive...",
	                       ASLFR_PositiveText, "Copy",
	                       ASLFR_InitialFile,  destName,
	                       TAG_DONE)) {

		strncpy(pathBuf, data->SaveReq->fr_Drawer, sizeof(pathBuf)-1);

		if (AddPart(pathBuf, data->SaveReq->fr_File, sizeof(pathBuf)-1)) {

			if (!IO_SameFile(ahn->ahn_Path, pathBuf)) {

				if (IO_ConfirmOverwrite(pathBuf)) {

					if (!IO_CopyFile(ahn->ahn_Path, pathBuf, TRUE)) {
						GUI_PrintStatus("Failed to copy file!", 0);
						GUI_PrgDOSError("Failed to copy file!", NULL);
					} else {
						IO_CopyIcon(ahn->ahn_Path, pathBuf, FALSE);
						GUI_PrintStatus("File copied OK.", 0);
					}

				} else {
					GUI_PrintStatus("Aborted - file already exists.", 0);
				}

			} else {
				GUI_PrgError("Source and destination files are the same!", NULL);
				GUI_PrintStatus("Copy aborted, source and destination files were the same.", 0);
			}

		} else {
			GUI_PrintStatus("Failed to construct path!", 0);
		}

	} else {
		GUI_PrintStatus("File requester cancelled.", 0);
	}

	return 0;
}

DECLARE(MoveArcTo)
{
	GETDATA;
	UBYTE pathBuf[256], *killPathVec;
	AHN *ahn = (AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN);

	if (!ahn) {
		DisplayBeep(NULL);
		return 0;
	}

	if (ahn->ahn_EmbeddedArc) {
		GUI_PrgError("You cannot move embedded archives!", NULL);
		return 0;
	}

	if (!MUI_AslRequestTags(data->SaveReq,
	                        ASLFR_Window,         xget(obj, MUIA_Window_Window),
	                        ASLFR_TitleText,      "Move archive...",
	                        ASLFR_PositiveText,   "Move",
	                        ASLFR_InitialFile,
	                        (FilePart(ahn->ahn_Path)[0] == 0 ? data->SaveReq->fr_File : FilePart(ahn->ahn_Path)),
	                        TAG_DONE)) {

		GUI_PrintStatus("File requester cancelled.", 0);
		return 0;
	}

	strncpy(pathBuf, data->SaveReq->fr_Drawer, sizeof(pathBuf)-1);

	if (!AddPart(pathBuf, data->SaveReq->fr_File, sizeof(pathBuf)-1)) {
		GUI_PrintStatus("Failed to construct path!", 0);
		return 0;
	}

	if (!IO_ConfirmOverwrite(pathBuf)) {
		GUI_PrintStatus("Aborted - file already exists.", 0);
		return 0;
	}

	if (IO_SameFile(ahn->ahn_Path, pathBuf)) {
		GUI_PrgError("Source and destination files are the same!", NULL);
		GUI_PrintStatus("Move aborted, source and destination files were the same.", 0);
		return 0;
	}

	if (Rename(ahn->ahn_Path, pathBuf)) {

		IO_CopyIcon(ahn->ahn_Path, pathBuf, TRUE);
		DoMethod(data->ArcHistory, MUIM_ArcHistory_RemoveAHN, ahn);
		DoMethod(data->ArcHistory, MUIM_ArcHistory_RedrawAll);
		ARC_LoadNewArchive(pathBuf, NULL, NULL, FALSE);
		DoMethod(data->ArcView, MUIM_ArcView_ShowArcEntries, NULL);
		GUI_PrintStatus("File moved OK (via rename).", 0);

	} else if ((IoErr() == ERROR_RENAME_ACROSS_DEVICES) &&
	           IO_CopyFile(ahn->ahn_Path, pathBuf, TRUE)) {

		if ((killPathVec = MEM_StrToVec(ahn->ahn_Path))) {
			IO_CopyIcon(ahn->ahn_Path, pathBuf, TRUE);
			DoMethod(data->ArcHistory, MUIM_ArcHistory_RemoveAHN, ahn);
			DoMethod(data->ArcHistory, MUIM_ArcHistory_RedrawAll); // todo: remove this method
			DeleteFile(killPathVec);
			ARC_LoadNewArchive(pathBuf, NULL, NULL, FALSE);
			DoMethod(data->ArcView, MUIM_ArcView_ShowArcEntries, NULL);
			MEM_FreeVec(killPathVec);
			GUI_PrintStatus("File moved OK.", 0);

		} else {
			GUI_PrintStatus("No memory!", 0);
		}

	} else {
		UBYTE *errStr = "Failed to move file!";

		GUI_PrintStatus(errStr, 0);
		GUI_PrgDOSError(errStr, NULL);
	}

	return 0;
}

DECLARE(DeleteArcFromDisk)
{
	GETDATA;
	AHN *ahn = (AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN);
	UBYTE *arcPathVec = NULL;
	LONG r;

	if (ahn == NULL) {
		DisplayBeep(NULL);
		return 0;
	}

	if (ahn->ahn_EmbeddedArc) {
		GUI_PrgError("You cannot delete embedded archives!", NULL);
		return 0;
	}

	if ((arcPathVec = MEM_StrToVec(ahn->ahn_Path))) {

		r = GUI_Popup("Warning...",
		              "Are you sure you want to delete the archive:\n"
		              "\n" "%s\n" "\n"
		              "from disk? You will not be able to get it back!",
		              &arcPathVec, "Yes, delete it|No, don't delete it");

		if (r == 1) {

			DoMethod(data->ArcHistory, MUIM_ArcHistory_RemoveAHN, NULL);
			DoMethod(data->ArcHistory, MUIM_NList_Redraw, MUIV_NList_Redraw_All);
			DeleteFile(arcPathVec);
			DeleteDiskObject(arcPathVec);
			DoMethod(data->ArcView, MUIM_ArcView_ShowArcEntries, NULL);

		} else {

			GUI_PrintStatus("Archive not deleted.", 0);
		}

		MEM_FreeVec(arcPathVec);
	}

	return 0;
}

DECLARE(SelAll)
{
	GETDATA;

	DoMethod(data->ArcView, MUIM_ArcView_SelectAll);
	DoMethod(GETARCVIEW, MUIM_ArcView_ShowArcInfo);

	return 0;
}

DECLARE(SelNone)
{
	GETDATA;

	DoMethod(data->ArcView, MUIM_ArcView_SelectNone);
	DoMethod(GETARCVIEW, MUIM_ArcView_ShowArcInfo);

	return 0;
}

DECLARE(SelPat)
{
	DoMethod(GETPATWIN, MUIM_GetPatternWin_Open);

	return 0;
}

DECLARE(ExtractAll)
{

	GETDATA;
	AHN *ahn;
	UBYTE *fmtData[3];

	if (!(ahn = (AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN))) {
		return 0;
	}

	get(data->DestPopStr, MUIA_String_Contents, &fmtData[1]);

	if (!fmtData[1]) {
		return 0;
	}

	if (!(fmtData[2] = FormatULONGToVec(ahn->ahn_CurrentPN->pn_expsize))) {
		return 0;
	}

	fmtData[0] = (ahn->ahn_EmbeddedArc ? ahn->ahn_EmbeddedListStr : ahn->ahn_Path);

	if (!CFG_Get(TAGID_MAIN_SHOWEXTRACTREQ)) {
		ARC_DoExtract(fmtData[1], TRUE, TRUE);
		return 0;
	}

	if (GUI_Popup("Request...",
	              "Are you sure you want to extract the entire contents of\n"
	              "\n%s\n\nto %s? Space needed: %s\n",
	              &fmtData, "Yes, extract all|No, don't extract anything") == 0) {
		GUI_PrintStatus("Archive not extracted", 0);

	} else {

		ARC_DoExtract(fmtData[1], TRUE, TRUE);
	}

	MEM_FreeVec(fmtData[2]);
	return 0;
}

DECLARE(ExtractSel)
{
	GETDATA;

	UBYTE *savePath = (UBYTE *) xget(data->DestPopStr, MUIA_String_Contents);

	if (!savePath) {
		return 0;
	}

	ARC_DoExtract(savePath, FALSE, TRUE);

	return 0;
}

DECLARE(Hide)
{
	GUI_Iconify(TRUE);

	return 0;
}

DECLARE(Quit)
{
	DoMethod(_app(obj), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

	return 0;
}

DECLARE(OpenArc)
{
	UBYTE pathBuf[256], fileBuf[128];
	GETDATA;
	AHN *ahn;

	if ((ahn = (AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN))) {

		strncpy(pathBuf, ahn->ahn_Path, sizeof(pathBuf)-1);
		*((UBYTE *)PathPart(pathBuf)) = 0;
		strncpy(fileBuf, FilePart(ahn->ahn_Path), sizeof(fileBuf)-1);

	} else {

		UBYTE *defPath = (UBYTE *) CFG_Get(TAGID_MAIN_DEFARCHIVEPATH);
		strncpy(pathBuf, defPath ? (char *) defPath : "", sizeof(pathBuf)-1);
		strncpy(fileBuf, "", sizeof(fileBuf)-1);
	}

	if (MUI_AslRequestTags(data->OpenReq,
	                       ASLFR_Window,         xget(obj, MUIA_Window_Window),
	                       ASLFR_DoMultiSelect,  TRUE,
	                       ASLFR_RejectIcons,    TRUE,
	                       ASLFR_DoPatterns,     TRUE,
	                       ASLFR_TitleText,      "Please select one or more archives...",
	                       (fileBuf[0] == 0 ? TAG_IGNORE : ASLFR_InitialFile),    &fileBuf,
	                       (pathBuf[0] == 0 ? TAG_IGNORE : ASLFR_InitialDrawer),  &pathBuf,
	                       TAG_DONE)) {

		ARC_LoadArcsFromWBArgs(data->OpenReq->fr_ArgList, data->OpenReq->fr_NumArgs, TRUE);
	}
	return 0;
}

DECLARE(OpenMultiPartArc)
{
	set(GETMULTIPARTWIN, MUIA_Window_Open, TRUE);

	return 0;
}

DECLARE(CloseArc)
{
	GETDATA;

	//set(GETBUSYWIN, MUIA_Window_Open, TRUE);
	DoMethod(data->ArcHistory, MUIM_ArcHistory_RemoveAHN,
	         (AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN));
	//DoMethod(data->ArcView, MUIM_ArcView_ShowArcEntries, NULL);
	IO_FreeTmpFileList(FALSE);

	return 0;
}

DECLARE(CloseAllArcs)
{
	GETDATA;
	AHN *skipAHN;
	ULONG closeCnt, cnt = xget(data->ArcHistory, MUIA_ArcHistory_Count);

	if (cnt < 1) {
		GUI_Popup("Notice", "There are no archives to close", NULL, "OK");
		return 0;
	}

	switch (GUI_Popup("Warning...", "Are you sure you want to close all archives?",
	                  NULL, "Yes, close all|Yes, but keep current|No, don't close anything")) {
		default:
		case 0: /* No                    */
			GUI_PrintStatus("Archive history not cleared.", 0);
			return 0;
		case 1: /* Yes, clear all        */
			skipAHN = NULL;
			break;
		case 2: /* Yes, but keep current */
			skipAHN = (AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN);
			break;
	}

	GUI_PrintStatus("Please wait, closing archives...", 0);
	DoMethod(data->ArcView, MUIM_ArcView_ShowArcEntries, NULL);
	DoMethod(data->ArcHistory, MUIM_ArcHistory_SetQuiet, TRUE);
	closeCnt = DoMethod(data->ArcHistory, MUIM_ArcHistory_CloseAll, skipAHN);
	IO_FreeTmpFileList(FALSE);
	DoMethod(data->ArcHistory, MUIM_ArcHistory_SetQuiet, FALSE);
	IO_FreeTmpFileList(FALSE);
	GUI_PrintStatus("Closed %lu archives.", closeCnt);

	return 0;
}

DECLARE(Search)
{
	set(GETSEARCHWIN, MUIA_Window_Open, TRUE);
	return 0;
}


// TODO: Maybe move this scanner to vx_virus?
BOOL VirusScan( AHN *ahn, ULONG *extractCntPtr, ULONG *virusCntPtr, BOOL progressWin, BOOL *abortPtr ) {

	// TODO: update this to scan embedded archives
	// ArcView_GetSelected_Start
	// ArcView_GetSelected_Next
	// ArcView_GetSelected_End

	struct ArcEntry **aeArray, **aea, *ae;
	ULONG exCnt = 0, virusCnt = 0;
	ULONG exTotal = 0, exFileCnt = 0, exDirCnt = 0, pCnt = 0;
	UBYTE *destFileName, *virusName;
	BOOL abortLoop = FALSE;
	LONG xadErr;

	if (!ahn) {
		return FALSE;
	}

	if (!xvsBase) {
		static ULONG fmt[2] = {(ULONG) XVS_NAME, XVS_VERSION };
		GUI_PrgError(STR(SID_FEATURE_REQS, "Sorry, this feature requires %s version %lu."), &fmt);
		return FALSE;
	}

	if (!(destFileName = IO_GetTmpFileName("VX", NULL))) {
		return FALSE;
	}

	if ((aea = aeArray = ARC_GetSelected(ahn, &exTotal, &exFileCnt, &exDirCnt, TRUE))) {

		if (progressWin) {
			OpenProgress(exFileCnt, "Checking archive for viruses...");
		}

		GUI_PrintStatus("Please wait, checking for viruses...", 0);

		while ((ae = *aea) && !abortLoop) {

			struct xadFileInfo *xfi = ae->ae_xfi;

			/* We don't need to check dir entries for viruses. :) */

			if (!xfi || ae->ae_IsDir) {
				++aea;
				continue;
			}

			if (progressWin) {
				ChangeProgressStatus(xfi->xfi_FileName, NULL);
			}

			if ((xfi->xfi_Flags & XADFIF_DIRECTORY) || (xfi->xfi_Flags & XADFIF_LINK)) {

				/* Skip this entry... */

			} else {

				/*APTR fileVec = NULL; ULONG fileLen = 0;

				if (ARC_ExtractFileToBuf(ahn, ae, &fileVec, &fileLen, FALSE, &xadErr)) {

					exCnt++;

					if (virusName = VIRUS_CheckFile(fileVec, fileLen)) {
						VIRUS_ShowWarning(ahn, ae, virusName);
						virusCnt++;
					}

					MEM_FreeVec(fileVec);

				} else*/ if (ARC_ExtractFile(ahn, ae, destFileName, FALSE, &xadErr, "")) {

					/* If we can't extract the file to memory then extract it to
					   a temporary file. */

					SetProtection(destFileName, 0);
					++exCnt;

					if ((virusName = VIRUS_CheckFile(destFileName))) {
						VIRUS_ShowWarning(ahn, ae, virusName);
						virusCnt++;
					}

				} else if (xadErr == XADERR_BREAK) {
					abortLoop = TRUE;
				}

				if (!abortLoop && progressWin) {
					abortLoop = UpdateProgress(xfi->xfi_FileName, NULL, ++pCnt);
				}
			}

			DoMethod(App, MUIM_Application_InputBuffered);
			SetProtection(destFileName, 0);
			DeleteFile(destFileName);

			if (abortLoop) {
				break;
			}

			aea++;

		} /* while () */

		if (progressWin) {
			CloseProgress;
		}

		ARC_FreeSelected(aeArray);
	}

	IO_DeleteTmpFile(destFileName);

	if (extractCntPtr) {
		*extractCntPtr = exCnt;
	}

	if (virusCntPtr) {
		*virusCntPtr = virusCnt;
	}

	if (abortPtr) {
		*abortPtr = abortLoop;
	}

	return TRUE;
}

DECLARE(VirusScanThis)
{
	ULONG exCnt, virusCnt;

	if (!VirusScan((AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN), &exCnt, &virusCnt, TRUE, NULL)) {
		return 0;
	}

	GUI_PrintStatus("%lu files checked, %lu virus%s found.",
	                exCnt, virusCnt, (virusCnt == 1 ? "" : "es"));
	return 0;
}

DECLARE(VirusScanAll)
{
	// TODO: Fix this entire thing

	GETDATA;
	Object *ah = GETARCHISTORY;
	ULONG exCnt = 0, virusCnt = 0, total = xget(data->ArcHistory, MUIA_ArcHistory_Count);
	BOOL abort = FALSE;
	LONG id, i;
	AHN *ahn;

	OpenProgress(total, "Checking all archives for viruses...");

	for (id = -1, i = 0; !abort; i++) {

		ULONG ec, vc;

		id = DoMethod(ah, MUIM_ArcHistory_GetNext, id, &ahn);

		if (id == -1) {
			break;
		}

		if (!VirusScan(ahn, &ec, &vc, FALSE, &abort)) {
			continue;
		}

		exCnt += ec;
		virusCnt += vc;

		if (!abort) {
			abort = UpdateProgress(ahn->ahn_Path, NULL, i);
		}
	}

	CloseProgress;

	GUI_PrintStatus("%ld archives checked, %lu files checked, %lu virus%s found.",
	                i + 1, exCnt, virusCnt, (virusCnt == 1 ? "" : "es"));

	return 0;
}

#define CHECKXCON "CON:0/11/900/200/CheckX/CLOSE/WAIT"

static UBYTE *MakeCheckXCmdLine( UBYTE *arcPath ) {

	/* Warning, not thread safe! */

	static UBYTE cmdBuf[256];

	UBYTE *cxCmd;
	cxCmd = (UBYTE *) CFG_Get(TAGID_MAIN_CHECKXCMD);

	if (!cxCmd || strlen(cxCmd) == 0) {
		cxCmd = "CheckX %fp";
	}

	if (ConstructCommandLine(cxCmd, cmdBuf, sizeof(cmdBuf)-1, arcPath)) {
		return cmdBuf;
	}

	return "";
}

DECLARE(VirusScanThisCheckX)
{
	UBYTE *cxCmd;
	AHN *ahn;
	BPTR conFH;

	if ((ahn = (AHN *) xget(GETARCHISTORY, MUIA_ArcHistory_ActiveAHN)) == NULL) {
		return 0;
	}

	if ((conFH = Open(CHECKXCON, MODE_NEWFILE)) == 0) {
		return 0;
	}

	cxCmd = MakeCheckXCmdLine(ahn->ahn_Path);

	if (cxCmd[0]) {

		if (!MySystem(cxCmd, conFH)) {
			FPrintf(conFH, "ERROR: Couldn't run CheckX!\n");
		}

	} else {

		FPrintf(conFH, "ERROR: Unable to construct CheckX command line for execution!\n");

	}
	Close(conFH);

	return 0;
}


// Scan entire archive history list for viruses. Using CheckX.
DECLARE(VirusScanAllCheckX)
{
	Object *ah = GETARCHISTORY;
	UBYTE buf[256];
	UBYTE *cxCmd;
	LONG i, id, len;
	AHN *ahn;
	BPTR conFH;

	if ((conFH = Open(CHECKXCON, MODE_NEWFILE)) == 0) {
		return 0;
	}

	for (id = -1;;) {
		if ((id = DoMethod(ah, MUIM_ArcHistory_GetNext, id, &ahn)) == -1) {
			break;
		}
		strcpy(buf, "Checking: ");
		strncat(buf, ahn->ahn_Path, sizeof(buf)-1);
		len = strlen(buf);
		FPrintf(conFH, "\n\n");

		for (i = 0; i < len; i++) {
			FPrintf(conFH, "-");
		}

		FPrintf(conFH, "\n");
		FPrintf(conFH, "\x1b[32m%s\x1b[0m\n", buf);

		for (i = 0; i < len; i++) {
			FPrintf(conFH, "-");
		}

		FPrintf(conFH, "\n");
		cxCmd = MakeCheckXCmdLine(ahn->ahn_Path);

		if (cxCmd[0]) {

			if (!MySystem(cxCmd, conFH)) {
				FPrintf(conFH, "ERROR: Couldn't run CheckX!\n");
				break;
			}

		} else {

			FPrintf(conFH, "ERROR: Unable to construct CheckX command line for execution!\n");
		}
	}

	Close(conFH);

	return 0;
}


// Show the VX "edit settings" window.
DECLARE(EditSettings)
{
	set(GETSETTINGSWIN, MUIA_Window_Open, TRUE);

	return 0;
}


// Show the MUI "edit settings" window.
DECLARE(EditMUISettings)
{
	DoMethod(_app(obj), MUIM_Application_OpenConfigWindow, 0L);

	return 0;
}


// Show the archive information window.
DECLARE(ShowArcInfoWin)
{
	Object *aiWin = GETARCINFOWIN;
	set(aiWin, MUIA_Window_Open, TRUE);

	return DoMethod(aiWin, MUIM_ArcInfoWin_Update, xget(DATAREF->ArcHistory, MUIA_ArcHistory_ActiveAHN));
}


// Show the extended file information window.
DECLARE(ExtendedFileInfo)
{
	AE *ae;
	Object *efiWin = GETEXTFILEINFOWIN;
	DoMethod(GETARCVIEW, MUIM_ArcView_GetActiveAE, &ae);
	DoMethod(efiWin, MUIM_ExtFileInfoWin_Update, ae ? ae->ae_xfi : NULL);
	set(efiWin, MUIA_Window_Open, TRUE);

	return 0;
}


// Show the XAD information window.
DECLARE(ShowXADInfo)
{
	DoMethod(GETXADINFOWIN, MUIM_XADInfoWin_Open);

	return 0;
}


// Show the version information window.
DECLARE(ShowVerInfo)
{
	//set(GETVERSIONWIN, MUIA_Window_Open, TRUE);
	ULONG fmtStream[20];
	UBYTE xvsTmpBuf[32], ourlTmpBuf[32];

#define VERINFO_TXT "" \
		"\33b\33uVersions of currently loaded external modules:\33n\n" \
		"\n"                          \
		"xadmaster.library %lu.%lu\n" \
		"muimaster.library %lu.%lu\n" \
		"xvs.library %s\n"            \
		"Listtree.mcc %lu.%lu\n"      \
		"NListtree.mcc %lu.%lu\n"     \
		"openurl.library %s"

	fmtStream[0]  = xadMasterBase->xmb_LibNode.lib_Version;
	fmtStream[1]  = xadMasterBase->xmb_LibNode.lib_Revision;
	fmtStream[2]  = MUIMasterBase->lib_Version;
	fmtStream[3]  = MUIMasterBase->lib_Revision;
	fmtStream[4]  = (ULONG) &xvsTmpBuf;
	// I've moved these functions over to CLASS_avm#? sources.
	// To help hide NListtree and Listtree includes from
	// each other. This means VX can be compiled now without
	// modifying your includes. Wouldn't it be nice if C
	// had namespaces. :-)
	MCC_GetListtreeVerRev ( &fmtStream[5],  &fmtStream[6] );
	MCC_GetNListtreeVerRev( &fmtStream[7], &fmtStream[8] );
	fmtStream[9] = (ULONG) &ourlTmpBuf;

	if (xvsBase) {
		// XVS 33.37 and later has a private library base...
		sprintf(xvsTmpBuf, "%lu.%lu",
		        (ULONG) xvsBase->lib_Version, (ULONG) xvsBase->lib_Revision);
	} else {
		strncpy(xvsTmpBuf, "(not available)", sizeof(xvsTmpBuf)-1);
	}

	if (OpenURLBase) {
		sprintf(ourlTmpBuf, "%lu.%lu",
		        (ULONG) OpenURLBase->lib_Version, (ULONG) OpenURLBase->lib_Revision);
	} else {
		strncpy(ourlTmpBuf, "(not available)", sizeof(ourlTmpBuf)-1);
	}

	GUI_Popup("Version information...", VERINFO_TXT, &fmtStream, "Continue");

	return 0;
}


// Show the VX error window.
DECLARE(ShowErrors)
{
	set(GETERRWIN, MUIA_Window_Open, TRUE);
	return 0;
}


// Show the VX "About" window.
DECLARE(ShowAbout)
{
	set(GETABOUTWIN, MUIA_Window_Open, TRUE);

	return 0;
}


// Show the standard "About MUI" window.
DECLARE(ShowAboutMUI)
{
	GETDATA;

	if (!data->AboutMUIWin) {
		data->AboutMUIWin = AboutmuiObject,
		                    MUIA_Window_RefWindow,     obj,
		                    MUIA_Aboutmui_Application, _app(obj), End;
	}

	if (data->AboutMUIWin) {
		set
			(data->AboutMUIWin, MUIA_Window_Open, TRUE);
	}

	return 0;
}


// Load the program settings from their default locations.
DECLARE(LoadSettings)
{
	DoMethod(GETSETTINGSWIN, MUIM_SettingsWin_Load);

	return 0;
}


// Save the program settings to their default locations.
DECLARE(SaveSettings)
{
	DoMethod(GETSETTINGSWIN, MUIM_SettingsWin_SaveAllSettingsFiles);

	return 0;
}


// Save the program settings with using a file-requester.
DECLARE(SaveSettingsAs)
{
	DoMethod(GETSETTINGSWIN, MUIM_SettingsWin_SaveAs);

	return 0;
}


// Toggle the debug console window.
DECLARE(FlickDebugConsole)
{
	DB(dbg_ShowHide())

	return 0;
}


// Change the destination path on the GUI.
//
// @param destpath  The new string path you want to place in the destination path string gadget on the GUI.
//
DECLARE(SetDestPath) // STRPTR destpath
{
	set(DATAREF->DestPopStr, MUIA_String_Contents, msg->destpath);
	DoMethod(DATAREF->DestHistoryList, MUIM_NList_InsertSingle, msg->destpath, MUIV_NList_Insert_Top);

	return 0;
}


// Bring the main window to the front of all other windows.
DECLARE(ToFront)
{
	DoMethod(obj, MUIM_Window_ToFront);

	return 0;
}


// Change the archive path on the GUI.
//
// @param path	The new string path you want to place in the archive path string gadget on the GUI.
//
DECLARE(SetArcPath) // STRPTR path
{
	DoMethod(DATAREF->ArcHistory, MUIM_ArcHistory_SetArcPath, msg->path);

	return 0;
}


// Change the text string in the status bar on the GUI.
//
// @param str	The new string you want to place in the status bar.

DECLARE(SetStatus) // STRPTR str
{
	set(DATAREF->Status, MUIA_Text_Contents, msg->str);

	return 0;
}


// Swap the view mode to "Linear" a.k.a. "Flat" mode.
DECLARE(GotoLinearViewMode)
{
	DoMethod(DATAREF->ArcView, MUIM_ArcView_SwapViewMode, MUIV_ArcView_SwapViewMode_Linear);

	return 0;
}


// Swap the view mode to "Files And Dirs" mode.
DECLARE(GotoFilesAndDirsViewMode)
{
	DoMethod(DATAREF->ArcView, MUIM_ArcView_SwapViewMode, MUIV_ArcView_SwapViewMode_FilesAndDirs);

	return 0;
}


// Swap the view mode to "Listtree" mode.
DECLARE(GotoListtreeViewMode)
{
	DoMethod(DATAREF->ArcView, MUIM_ArcView_SwapViewMode, MUIV_ArcView_SwapViewMode_Listtree);

	return 0;
}


// Swap the view mode to "NListtree" mode.
DECLARE(GotoNListtreeViewMode)
{
	DoMethod(DATAREF->ArcView, MUIM_ArcView_SwapViewMode, MUIV_ArcView_SwapViewMode_NListtree);

	return 0;
}


// Swap the view mode to "Explorer" mode.
DECLARE(GotoExplorerViewMode)
{
	DoMethod(DATAREF->ArcView, MUIM_ArcView_SwapViewMode, MUIV_ArcView_SwapViewMode_Explorer);

	return 0;
}


// Toggle the name column in the archive listview.
DECLARE(ViewColumn_Name)
{
	if (xget((Object *) DoMethod(DATAREF->MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_NAME), MUIA_Menuitem_Checked)) {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_ShowColumns, VIEWCOLUMN_NAME);
	} else {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_HideColumns, VIEWCOLUMN_NAME);
	}

	return 0;
}


// Toggle the size column in the archive listview.
DECLARE(ViewColumn_Size)
{
	if (xget((Object *) DoMethod(DATAREF->MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_SIZE), MUIA_Menuitem_Checked)) {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_ShowColumns, VIEWCOLUMN_SIZE);
	} else {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_HideColumns, VIEWCOLUMN_SIZE);
	}

	return 0;
}


// Toggle the day column in the archive listview.
DECLARE(ViewColumn_Day)
{
	if (xget((Object *) DoMethod(DATAREF->MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_DAY), MUIA_Menuitem_Checked)) {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_ShowColumns, VIEWCOLUMN_DAY);
	} else {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_HideColumns, VIEWCOLUMN_DAY);
	}

	return 0;
}


// Toggle the date column in the archive listview.
DECLARE(ViewColumn_Date)
{
	if (xget((Object *) DoMethod(DATAREF->MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_DATE), MUIA_Menuitem_Checked)) {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_ShowColumns, VIEWCOLUMN_DATE);
	} else {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_HideColumns, VIEWCOLUMN_DATE);
	}

	return 0;
}

// Toggle the time column in the archive listview.
DECLARE(ViewColumn_Time)
{
	if (xget((Object *) DoMethod(DATAREF->MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_TIME), MUIA_Menuitem_Checked)) {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_ShowColumns, VIEWCOLUMN_TIME);
	} else {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_HideColumns, VIEWCOLUMN_TIME);
	}

	return 0;
}


// Toggle the protection column in the archive listview.
DECLARE(ViewColumn_Protection)
{
	if (xget((Object *) DoMethod(DATAREF->MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_PROTECTION), MUIA_Menuitem_Checked)) {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_ShowColumns, VIEWCOLUMN_PROT);
	} else {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_HideColumns, VIEWCOLUMN_PROT);
	}

	return 0;
}


// Toggle the comment column in the archive listview.
DECLARE(ViewColumn_Comment)
{
	if (xget((Object *) DoMethod(DATAREF->MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_COMMENT), MUIA_Menuitem_Checked)) {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_ShowColumns, VIEWCOLUMN_COMMENT);
	} else {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_HideColumns, VIEWCOLUMN_COMMENT);
	}

	return 0;
}


// Toggle the entry info. column in the archive listview.
DECLARE(ViewColumn_EntryInfo)
{
	if (xget((Object *) DoMethod(DATAREF->MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_ENTRYINFO), MUIA_Menuitem_Checked)) {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_ShowColumns, VIEWCOLUMN_ENTRYINFO);
	} else {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_HideColumns, VIEWCOLUMN_ENTRYINFO);
	}

	return 0;
}


// Toggle the suffix column in the archive listview.
DECLARE(ViewColumn_Suffix)
{
	if (xget((Object *) DoMethod(DATAREF->MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_SUFFIX), MUIA_Menuitem_Checked)) {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_ShowColumns, VIEWCOLUMN_SUFFIX);
	} else {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_HideColumns, VIEWCOLUMN_SUFFIX);
	}

	return 0;
}


// Toggle the bars in the archive listview.
DECLARE(ViewColumn_ShowBars)
{
	if (xget((Object *) DoMethod(DATAREF->MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_SHOWBARS), MUIA_Menuitem_Checked)) {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_ShowColumns, VIEWCOLUMN_SHOWBARS);
	} else {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_HideColumns, VIEWCOLUMN_SHOWBARS);
	}

	return 0;
}


// Show only AmigaOS related bits in the archive listview.
//
// This function is called when the ViewColumn/AmigaOSBits menu item changes. It will query the
// item to see if it has been checked. If so, then the MUIM_ArcView_ShowAmigaOSBits method
// of the archive view class is called to tell it to show only Amiga bits in the listview gadget.
//
DECLARE(ViewColumn_AmigaOSBits)
{
	if (xget((Object *) DoMethod(DATAREF->MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_AMIGAOSBITS), MUIA_Menuitem_Checked)) {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_ShowAmigaOSBits);
	}

	return 0;
}


// Show all protection bits in the archive listview.
//
// This function is called when the ViewColumn/AllBits menu item changes. It will query the
// item to see if it has been checked. If so, then the MUIM_ArcView_ShowAllBits method
// of the archive view class is called to tell it to show every bit in the listview gadget.
//
DECLARE(ViewColumn_AllBits)
{
	if (xget((Object *) DoMethod(DATAREF->MenuStrip, MUIM_FindUData, MENU_SETTINGS_VIEWCOLUMN_ALLBITS), MUIA_Menuitem_Checked)) {
		DoMethod(DATAREF->ArcView, MUIM_ArcView_ShowAllBits);
	}

	return 0;
}
