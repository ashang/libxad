/*
 $Id: CLASS_VXToolBar.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class for the main toolbar.

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
 *  Class Name: VXToolBar
 * Description: Button Panel for main window (i.e. SpeedBar)
 *  Superclass: MUIC_Group
 */

/* CLASSDATA
//------------------------------------------------------------------------------
	struct VXToolBar *vxtb;
	Object *SpeedBar;
	Object *LinkGroup;
*/

/* EXPORT
//------------------------------------------------------------------------------
enum // Keep in sync with the cycle gadget
{	
	MUIV_VXToolBar_Appearance_IconsAndText = 0,
	MUIV_VXToolBar_Appearance_Icons,
	MUIV_VXToolBar_Appearance_Text
};
 
#define MAX_SPEEDBAR_BUTTONS 30
 
struct VXToolBar
{
	ULONG ID, Appear;
	ULONG Cnt, Max;
	LONG                         ActionArray    [MAX_SPEEDBAR_BUTTONS];
	UBYTE                       *ImagePathArray [MAX_SPEEDBAR_BUTTONS + 1];
	struct MyBrush              *ImageArray     [MAX_SPEEDBAR_BUTTONS + 1];
	struct MUIS_SpeedBar_Button  BtnArray       [MAX_SPEEDBAR_BUTTONS + 1];
};
*/

#ifndef SPEEDBARCFG_MCC_H
#include <mui/SpeedBarCfg_mcc.h>
#endif /* SPEEDBARCFG_MCC_H */

#include "vx_include.h"
#include "VXToolBar.h"
#include "CLASS_VXToolBar_Defs.h"
#include "vx_mem.h"
#include "vx_cfg.h"
#include "mybrush.h"
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_misc.h"

//------------------------------------------------------------------------------

struct TBTemplate {
	UBYTE *label, *imagepath, *action;
};
struct TBInfo     {
	ULONG methodid;
	STRPTR keyword;
	STRPTR desc;
};

static struct TBInfo TBMethodMap[] = {
	/* TBACTION_SPACER            */ { MUIM_MainWin_ShowAbout,         "INSERT_SPACE",      "(Insert spacer)"          },
	/* TBACTION_SELECT_ALL        */ { MUIM_MainWin_SelAll,            "SELECT_ALL",        "Select All"               },
	/* TBACTION_SELECT_NONE       */ { MUIM_MainWin_SelNone,           "SELECT_NONE",       "Select None"              },
	/* TBACTION_SELECT_PAT        */ { MUIM_MainWin_SelPat,            "SELECT_PAT",        "Select Pattern"           },
	/* TBACTION_EXTRACT_ALL       */ { MUIM_MainWin_ExtractAll,        "EXTRACT_ALL",       "Extract All"              },
	/* TBACTION_EXTRACT_SEL       */ { MUIM_MainWin_ExtractSel,        "EXTRACT_SEL",       "Extract Selected"         },
	/* TBACTION_HIDE              */ { MUIM_MainWin_Hide,              "HIDE",              "Hide"                     },
	/* TBACTION_QUIT              */ { MUIM_MainWin_Quit,              "QUIT",              "Quit"                     },
	/* TBACTION_OPEN_ARC          */ { MUIM_MainWin_OpenArc,           "OPEN_ARC",          "Open archive"             },
	/* TBACTION_OPEN_MP_ARC       */ { MUIM_MainWin_OpenMultiPartArc,  "OPEN_MP_ARC",       "Open multipart archive"   },
	/* TBACTION_CLOSE_ARC         */ { MUIM_MainWin_CloseArc,          "CLOSE_ARC",         "Close archive"            },
	/* TBACTION_CLOSE_ALL_ARCS    */ { MUIM_MainWin_CloseAllArcs,      "CLOSE_ALL_ARCS",    "Close all archives"       },
	/* TBACTION_SEARCH            */ { MUIM_MainWin_Search,            "SEARCH",            "Search"                   },
	/* TBACTION_CHECK_FOR_VIRUSES */ { MUIM_MainWin_VirusScanThis,     "CHECK_FOR_VIRUSES", "Check for viruses"        },
	/* TBACTION_EDIT_SETT         */ { MUIM_MainWin_EditSettings,      "EDIT_SETT",         "Edit settings"            },
	/* TBACTION_EDIT_MUI_SETT     */ { MUIM_MainWin_EditMUISettings,   "EDIT_MUI_SETT",     "Edit MUI settings"        },
	/* TBACTION_SHOW_XAD_INFO     */ { MUIM_MainWin_ShowXADInfo,       "SHOW_XAD_INFO",     "Show XAD information"     },
	/* TBACTION_SHOW_VER_INFO     */ { MUIM_MainWin_ShowVerInfo,       "SHOW_VER_INFO",     "Show version information" },
	/* TBACTION_SHOW_ERRORS       */ { MUIM_MainWin_ShowErrors,        "SHOW_ERRORS",       "Show errors"              },
	/* TBACTION_SHOW_ABOUT        */ { MUIM_MainWin_ShowAbout,         "SHOW_ABOUT",        "Show About window"        },
	/* TBACTION_SHOW_ABOUT_MUI    */ { MUIM_MainWin_ShowAboutMUI,      "SHOW_ABOUT_MUI",    "Show About MUI window"    },
	{ 0,                               NULL,                NULL                      }
};

static long VXTB_GetNumFromKeyWord( STRPTR keyword ) {

	LONG i = 0;
	struct TBInfo *tbi = TBMethodMap;

	while (tbi->methodid) {

		if (Stricmp(tbi->keyword, keyword) == 0) {
			return i;
		}

		tbi++;
		i++;
	}

	DB(dbg("VXTB_GetNumFromKeyWord() returned 0 [KEYWORD NOT FOUND!]"))

	return 0;
}

void ShowMissingImages( UBYTE **failed_list ) {
#define BUFSIZE 2000

	UBYTE **fl = failed_list, *errvec;

	if (!(errvec = MEM_AllocVec(BUFSIZE))) {
		return;
	}

	strncpy(errvec, "Couldn't load the following toolbar images:\n\n", BUFSIZE-1);

	while (*fl++) {
		strncat(errvec, "< %s >\n", BUFSIZE-1);
	}

	GUI_PrgError(errvec, failed_list);

	MEM_FreeVec(errvec);
}

void VXTB_Free( struct VXToolBar *vxtb ) {

	LONG i;

	if (!vxtb) {
		return;
	}

	for (i = 0; i < MAX_SPEEDBAR_BUTTONS; i++) {

		if (vxtb->ImageArray[i]) {
			FreeBrush(vxtb->ImageArray[i]);
		}

		if (vxtb->ImagePathArray[i]) {
			MEM_FreeVec(vxtb->ImagePathArray[i]);
		}

		if (vxtb->BtnArray[i].Text) {
			MEM_FreeVec(vxtb->BtnArray[i].Text);
		}
	}

	MEM_FreeVec(vxtb);
}

struct VXToolBar *VXTB_Clone( struct VXToolBar *vxtb ) {

	LONG i;
	struct VXToolBar *new_vxtb;

	if (!(new_vxtb = MEM_AllocVec(sizeof(struct VXToolBar)))) {
		return NULL;
	}

	new_vxtb->ID     = vxtb->ID;
	new_vxtb->Appear = vxtb->Appear;
	new_vxtb->Max    = vxtb->Max;
	new_vxtb->Cnt    = vxtb->Cnt;

	for (i = 0; i < vxtb->Cnt; i++) {

		DB(dbg("Clone button %lu [%s]", i, vxtb->ImagePathArray[i]);)

		new_vxtb->ActionArray   [i]      = vxtb->ActionArray[i];
		new_vxtb->ImagePathArray[i]      = MEM_StrToVec(vxtb->ImagePathArray[i]);
		new_vxtb->BtnArray      [i].Img  = (vxtb->ActionArray[i] == TBACTION_SPACER) ? MUIV_SpeedBar_Spacer : i;
		new_vxtb->BtnArray      [i].Text = MEM_StrToVec(vxtb->BtnArray[i].Text);

		if (vxtb->ActionArray[i] != TBACTION_SPACER) {

			if (!(new_vxtb->ImageArray[i] = LoadBrush(vxtb->ImagePathArray[i]))) {

				VXTB_Free(new_vxtb);

				return NULL;
			}
		}

		if (!new_vxtb->ImagePathArray[i] || !new_vxtb->BtnArray[i].Text) {
			VXTB_Free(new_vxtb);
			return NULL;
		}
	}

	new_vxtb->BtnArray[i].Img = MUIV_SpeedBar_End;

	return new_vxtb;
}

struct VXToolBar *VXTB_CreateDefault( void ) {
	/* TODO: rewrite or simplify this code */

	struct VXToolBar *vxtb;
	LONG  *actionPtr, i = 0, iFail = 0, tFail = 0;
	struct MyBrush **imagePtr;
	struct MUIS_SpeedBar_Button *btnPtr;
	UBYTE *failedList[MAX_SPEEDBAR_BUTTONS + 1];
	UBYTE **fl = failedList, **imagePathPtr;

	if (!(vxtb = MEM_AllocVec(sizeof(struct VXToolBar)))) {
		return NULL;
	}

	vxtb->ID      = MAKE_ID('V','X','T','B');
	vxtb->Appear  = MUIV_VXToolBar_Appearance_IconsAndText;
	vxtb->Cnt     = 0;
	vxtb->Max     = MAX_SPEEDBAR_BUTTONS;
	actionPtr     = &vxtb->ActionArray[0];
	imagePtr      = &vxtb->ImageArray[0];
	imagePathPtr  = &vxtb->ImagePathArray[0];
	btnPtr        = &vxtb->BtnArray[0];

#define DEFBTN(label, path, action) *actionPtr++ = action;                          \
		if (TBACTION_SPACER == action) {                                            \
			btnPtr->Img = MUIV_SpeedBar_Spacer;                                     \
			vxtb->Cnt += 1; imagePtr++; btnPtr++; i++; imagePathPtr++;              \
		} else {                                                                    \
			if (!(*imagePathPtr++ = MEM_StrToVec(path))) { ++iFail; *fl++ = path; } \
			if (!(*imagePtr++ = LoadBrush(path))) { ++iFail; *fl++ = path; }        \
			btnPtr->Img = i++;                                                      \
			if (!(btnPtr->Text = MEM_StrToVec(label))) ++tFail;                     \
			vxtb->Cnt += 1; btnPtr++;                                               \
		}


	#define DEFEND                         \
		*imagePtr     = NULL;              \
		*imagePathPtr = NULL;              \
		btnPtr->Img   = MUIV_SpeedBar_End; \
		*fl = NULL;

	DEFBTN("Open...",           "PROGDIR:Images/Open",            TBACTION_OPEN_ARC    )
	DEFBTN("Close",             "PROGDIR:Images/Close",           TBACTION_CLOSE_ARC   )
	DEFBTN("",                  "PROGDIR:Images/Spacer",          TBACTION_SPACER      )
	DEFBTN("Select all",        "PROGDIR:Images/SelectAll",       TBACTION_SELECT_ALL  )
	DEFBTN("Select none",       "PROGDIR:Images/SelectNone",      TBACTION_SELECT_NONE )
	DEFBTN("Select pattern...", "PROGDIR:Images/SelectPattern",   TBACTION_SELECT_PAT  )
	DEFBTN("",                  "PROGDIR:Images/Spacer",          TBACTION_SPACER      )
	DEFBTN("Extract All",       "PROGDIR:Images/ExtractAll",      TBACTION_EXTRACT_ALL )
	DEFBTN("Extract selected",  "PROGDIR:Images/ExtractSelected", TBACTION_EXTRACT_SEL )
	DEFBTN("",                  "PROGDIR:Images/Spacer",          TBACTION_SPACER      )
	DEFBTN("Hide",              "PROGDIR:Images/Hide",            TBACTION_HIDE        )
	DEFBTN("Quit",              "PROGDIR:Images/Quit",            TBACTION_QUIT        ) // TODO: Fix 2 DA warnings
	DEFEND

	if (tFail > 0) {
		/* Some text failed to allocate - low mem/total failure... */
		VXTB_Free(vxtb);
		return NULL;
	}

	if (iFail > 0) {
		/* Some images failed to load... */
		vxtb->Appear = MUIV_VXToolBar_Appearance_Text;
		ShowMissingImages(failedList);
	}

	return vxtb;
}

struct VXToolBar *VXTB_Load( UBYTE *filename ) {

	struct VXToolBar *vxtb;
	LONG i = 0, failcnt = 0, actnum;
	struct TBTemplate template;
	UBYTE *failed_list[MAX_SPEEDBAR_BUTTONS + 1], **fl = failed_list;
	UBYTE *path, linebuf[256];
	BPTR infh;
	struct RDArgs *rda;

	if (!(infh = Open(filename, MODE_OLDFILE))) {
		return NULL;
	}

	if (DOSBase->dl_lib.lib_Version >= 40) {
		if (SetVBuf(infh, NULL, BUF_FULL, TOOLBAR_IOBUFFER_SIZE)) {
			return NULL;
		}
	}

	if (!FGets(infh, linebuf, sizeof(linebuf)-1) ||
	        (Strnicmp(linebuf, TOOLBAR_ID, sizeof(TOOLBAR_ID)-1) != 0) ||
	        !(vxtb = MEM_AllocVec(sizeof(struct VXToolBar)))) {

		Close(infh);
		return NULL;
	}

	vxtb->ID     = MAKE_ID('V','X','T','B');
	vxtb->Appear = CFG_Get(TAGID_TB_APPEARANCE);
	vxtb->Max    = MAX_SPEEDBAR_BUTTONS;

	while (FGets(infh, linebuf, sizeof(linebuf)-1)) {

		if (i > MAX_SPEEDBAR_BUTTONS) {
			break;
		}

		if (linebuf[0] == ';') {
			continue; /* Ignore commented lines */
		}

		if (!(rda = AllocDosObject(DOS_RDARGS, NULL))) {
			continue;
		}

		memset(&template, 0, sizeof(struct TBTemplate));

		rda->RDA_Source.CS_Buffer = linebuf;
		rda->RDA_Source.CS_Length = strlen(linebuf);
		rda->RDA_Source.CS_CurChr = 0;

		if (ReadArgs(TOOLBAR_TEMPLATE, (LONG *) &template, rda)) {
			vxtb->ImagePathArray[i] = path = MEM_StrToVec(template.imagepath);

			// TODO: Error checks
			actnum = VXTB_GetNumFromKeyWord(template.action);

			if (actnum == TBACTION_SPACER) {

				vxtb->ImageArray[i] = NULL;

			} else if (vxtb->Appear == MUIV_VXToolBar_Appearance_Text || !path || !path[0]) {

				vxtb->Appear = MUIV_VXToolBar_Appearance_Text; /* Force it to text */
				vxtb->ImageArray[i] = NULL;

			} else if (!(vxtb->ImageArray[i] = LoadBrush(path))) {

				*fl++ = path ? path : (UBYTE *) "No IFF image path specified";
				failcnt++;
			}

			vxtb->BtnArray   [i].Img    = (actnum == TBACTION_SPACER) ? MUIV_SpeedBar_Spacer : i;
			vxtb->BtnArray   [i].Text   = MEM_StrToVec(template.label);
			vxtb->BtnArray   [i].Help   = NULL; // TODO: Set this
			vxtb->BtnArray   [i].Flags  = 0;
			vxtb->BtnArray   [i].Class  = NULL;
			vxtb->BtnArray   [i].Object = NULL;
			vxtb->ActionArray[i]        = actnum;

			FreeArgs(rda);
		}

		FreeDosObject(DOS_RDARGS, rda);
		i++;

	} /* While () */

	Close(infh);
	*fl = NULL;

	vxtb->BtnArray  [i].Img = MUIV_SpeedBar_End;
	vxtb->ImagePathArray[i] = NULL;
	vxtb->ImageArray[i]     = NULL;
	vxtb->Cnt = i;

	if (failcnt > 0) {
		ShowMissingImages(failed_list);
		vxtb->Appear = MUIV_VXToolBar_Appearance_Text;
	}

	if (vxtb->Cnt == 0) {
		VXTB_Free(vxtb);
		vxtb = VXTB_CreateDefault();
	}

	return vxtb;
}

OVERLOAD(OM_NEW)
{
	STRPTR path = NULL;
	ULONG appear;
	struct TagItem *tags = inittags(msg), *tag;
	DEFTMPDATA;
	CLRTMPDATA;

	while ((tag = NextTagItem(&tags))) {
		switch(tag->ti_Tag) {
			ATTR(Load): path = (STRPTR) tag->ti_Data;
			break;
		}
	}

	if (!path) {
		path = TOOLBAR_FILENAME;
	}

	if (!(data.vxtb = VXTB_Load(path))) {
		if (!(data.vxtb = VXTB_CreateDefault())) {
			GUI_Popup("Error", "Couldn't create default toolbar!!!", NULL, "Abort");
			return 0;
		}
	}

	switch (data.vxtb->Appear) {

		case MUIV_VXToolBar_Appearance_Icons:
			appear = MUIV_SpeedBar_ViewMode_Gfx;
			break;

		case MUIV_VXToolBar_Appearance_IconsAndText:
			appear = MUIV_SpeedBar_ViewMode_TextGfx;
			break;

		default:
			appear = MUIV_SpeedBar_ViewMode_Text;
			break;
	}

	obj = (Object *) DoSuperNew(cl, obj,
		Child, data.LinkGroup = HGroup,
			Child, HSpace(0),
			Child, data.SpeedBar = SpeedBarObject,
				MUIA_Group_Horiz,           TRUE,
				MUIA_HorizWeight,           200,
				MUIA_SpeedBar_Borderless,   CFG_Get(TAGID_TB_BORDERLESS),
				MUIA_SpeedBar_Sunny,        CFG_Get(TAGID_TB_SUNNY),
				MUIA_SpeedBar_SmallImages,  CFG_Get(TAGID_TB_SMALL),
				MUIA_SpeedBar_RaisingFrame, CFG_Get(TAGID_TB_RAISED),
				MUIA_SpeedBar_ViewMode,     appear,
				MUIA_SpeedBar_Buttons,      data.vxtb->BtnArray,
				MUIA_SpeedBar_Images,       data.vxtb->ImageArray,
				MUIA_SpeedBar_SpacerIndex,  -1,
				MUIA_SpeedBar_SameWidth,    TRUE,
				MUIA_SpeedBar_Spread,       TRUE,
			End,
			Child, HSpace(0),
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		GUI_Popup("Error", "Problem while creating VXToolbar object instance!!!", NULL, "Abort");
		return 0;
	}

	PREPDATA;

	return (ULONG) obj;
}

OVERLOAD(OM_DISPOSE)
{
	if (DATAREF->vxtb) {
		VXTB_Free(DATAREF->vxtb);
	}

	return DoSuperMethodA(cl, obj, msg);
}

OVERLOAD(OM_GET)
{
	GETDATA;

	ULONG *store = ((struct opGet *)msg)->opg_Storage;

	switch (((struct opGet *)msg)->opg_AttrID) {
		ATTR(VXTB): *store = (ULONG) data->vxtb; break;
	}

	return DoSuperMethodA(cl, obj, msg);
}

DECLARE(LinkTriggers) // Object *winobj
{
	GETDATA;
	LONG i, actnum;
	struct VXToolBar *vxtb = data->vxtb;

	DB(
		if (!vxtb) {
			dbg("VXToolBar::LinkTriggers got NULL vxtb!!!!");
			return 0;
		}
	)

	for (i = 0; i < vxtb->Cnt; i++) {
		actnum = vxtb->ActionArray[i];

		if (actnum >= TBACTION_MAX) {
			continue;
		}

		DoMethod(vxtb->BtnArray[i].Object, MUIM_Notify, MUIA_Pressed, FALSE,
		         msg->winobj, 1, TBMethodMap[actnum].methodid);
	}

	return 0;
}

DECLARE(Update) // struct VXToolBar *vxtb, struct MUIS_SpeedBarCfg_Config *sbcfg
{
	GETDATA;
	struct VXToolBar *vxtb;
	Object *NewLinkGroup, *NewSpeedBar;

	if (!msg->vxtb || !msg->sbcfg) {
		return 0;
	}

	/* I can't be arsed using MUIM_SpeedBar_AddButton here, so I'm just
	      gonna kill the lot and re-create from scratch. ;-) */
	
	if (!(vxtb = VXTB_Clone(msg->vxtb))) {
		return 0;
	}

	NewLinkGroup = HGroup,
		Child, HSpace(0),
		Child, NewSpeedBar = SpeedBarObject,
			MUIA_Group_Horiz,           TRUE,
			MUIA_HorizWeight,           200,
			MUIA_SpeedBar_Borderless,   msg->sbcfg->Flags & MUIV_SpeedBarCfg_Borderless,
			MUIA_SpeedBar_Sunny,        msg->sbcfg->Flags & MUIV_SpeedBarCfg_Sunny,
			MUIA_SpeedBar_SmallImages,  msg->sbcfg->Flags & MUIV_SpeedBarCfg_SmallButtons,
			MUIA_SpeedBar_RaisingFrame, msg->sbcfg->Flags & MUIV_SpeedBarCfg_Raising,
			MUIA_SpeedBar_ViewMode,     msg->sbcfg->ViewMode,
			MUIA_SpeedBar_Buttons,      vxtb->BtnArray,
			MUIA_SpeedBar_Images,       vxtb->ImageArray,
			MUIA_SpeedBar_SpacerIndex,  -1,
			MUIA_SpeedBar_SameWidth,    TRUE,
			MUIA_SpeedBar_Spread,       TRUE,
		End,
		Child, HSpace(0),
	End;

	if (!NewLinkGroup) {
		VXTB_Free(vxtb);
		return 0;
	}

	DoMethod(obj, MUIM_Group_InitChange);
	DoMethod(obj, OM_REMMEMBER, data->LinkGroup);
	MUI_DisposeObject(data->LinkGroup);
	DoMethod(obj, OM_ADDMEMBER, data->LinkGroup = NewLinkGroup);
	DoMethod(obj, MUIM_Group_ExitChange);
	data->SpeedBar = NewSpeedBar;
	VXTB_Free(data->vxtb);
	data->vxtb = vxtb;

	//
	// Note: LinkTriggers requires data->vxtb to be valid...
	//

	DoMethod(obj, MUIM_VXToolBar_LinkTriggers, GETMAINWIN);
	return 0;
}

