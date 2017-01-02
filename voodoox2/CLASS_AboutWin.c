/*
 $Id: CLASS_AboutWin.c,v 1.3 2004/01/21 08:08:34 andrewbell Exp $
 Custom class for the "about progam" window.

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
 *  Class Name: AboutWin
 * Description: Handles the About window
 *  Superclass: MUIC_Window
 */

#include "vx_include.h"
#include "AboutWin.h"
#include "vx_mem.h"
#include "vx_misc.h"
#include "vx_logo.h"
#include "vx_debug.h"
#include "vx_main.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	UBYTE *HistoryText;
	Object *Logo;
	Object *Ver;
	Object *Listview;
	Object *List;
	Object *HistoryListview;
	Object *HistoryList;
	Object *Exit;
//------------------------------------------------------------------------------
*/

#define HISTORYFILE "PROGDIR:VX.history"
#define DIRKSADDY   "http://www.dstoecker.de"

static UBYTE *AboutTextArray[] = {
 "",
 TXT_MAINTITLE "XAD"                                                            TXT_OFF,
 "",
 TXT_CENTRE "This program wouldn't be possible without XAD. XAD is an archive"  TXT_OFF,
 TXT_CENTRE "decompression system developed by Dirk Stöcker and is Shareware."  TXT_OFF,
 TXT_CENTRE "Please support Amiga developers and register this fine piece of"   TXT_OFF,
 TXT_CENTRE "software. For more info on XAD and registration details, please"   TXT_OFF,
 TXT_CENTRE "visit the author's website at \33o[0@1]."  TXT_OFF,
 "",
 TXT_MAINTITLE "Credits"                                 TXT_OFF,
 "",
 TXT_SUBTITLE  "VX design and programming:"              TXT_OFF,
 TXT_CENTRE    "Andrew Bell"                             TXT_OFF,
 "",
 TXT_SUBTITLE  "XAD design and programming:"             TXT_OFF,
 TXT_CENTRE    "Dirk Stöcker"                            TXT_OFF,
 "",
 TXT_SUBTITLE  "MUI design and programming:"             TXT_OFF,
 TXT_CENTRE    "Stefan Stuntz"                           TXT_OFF,
 "",
 TXT_SUBTITLE  "SpeedBar design and programming:"        TXT_OFF,
 TXT_CENTRE    "Simone Tellini"                          TXT_OFF,
 "",
 TXT_SUBTITLE  "NList design and programming:"           TXT_OFF,
 TXT_CENTRE    "Gilles Masson"                           TXT_OFF,
 "",
 TXT_SUBTITLE  "Listtree design and programming:"        TXT_OFF,
 TXT_CENTRE    "Klaus Melchior"                          TXT_OFF,
 "",
 TXT_SUBTITLE  "XVS design and programming:"             TXT_OFF,
 TXT_CENTRE    "Georg Hörmann"                           TXT_OFF,
 "",
 TXT_SUBTITLE  "NListtree design and programming:"       TXT_OFF,
 TXT_CENTRE    "Carsten Scholling"                       TXT_OFF,
 "",
 TXT_SUBTITLE  "OpenURL design and programming:"         TXT_OFF,
 TXT_CENTRE    "Troels Walsted Hansen"                   TXT_OFF,
 "",
 TXT_SUBTITLE  "Toolbar images & WB icons"               TXT_OFF,
 TXT_CENTRE    "Martin Merz"                             TXT_OFF,
 "",
 TXT_MAINTITLE "Beta testers (in no apparent order)"     TXT_OFF,
 "",
 TXT_CENTRE    "Dirk Stöcker"                            TXT_OFF,
 TXT_CENTRE    "Charlene McNulty"                        TXT_OFF,
 TXT_CENTRE    "JanRoger Haugan"                         TXT_OFF,
 TXT_CENTRE    "Hervé Dupont"                            TXT_OFF,
 TXT_CENTRE    "Bill Duxbury"                            TXT_OFF,
 TXT_CENTRE    "Halil Ibrahim Tasova"                    TXT_OFF,
 TXT_CENTRE    "Stuart Caie"                             TXT_OFF,
 TXT_CENTRE    "(Plus those on the XAD mailing list)"    TXT_OFF,
 "",
 TXT_MAINTITLE "Copyright information"                   TXT_OFF,
 "",
 TXT_CENTRE "VX is Copyright © 1999 (and later) Andrew Bell."                      TXT_OFF,
 TXT_CENTRE "xadmaster.library is Copyright © 1998 (and later) Dirk Stöcker."            TXT_OFF,
 TXT_CENTRE "muimaster.library is Copyright © 1992 (and later) Stefan Stuntz."           TXT_OFF,
 TXT_CENTRE "SpeedBar custom class is Copyright © 1999 (and later) Simone Tellini."      TXT_OFF,
 TXT_CENTRE "NList MUI custom class is Copyright © 1996 (and later) Gilles Masson."      TXT_OFF,
 TXT_CENTRE "Listtree MUI custom class is Copyright © 1995 (and later) Klaus Melchior."  TXT_OFF,
 TXT_CENTRE "xvs.library is Copyright © 1997 (and later) Georg Hörmann."                 TXT_OFF,
 TXT_CENTRE "NListtree MUI custom class is © 2002 (and later) Carsten Scholling."             TXT_OFF,
 TXT_CENTRE "openurl.library is Copyright © 1998 (and later) Troels Walsted Hansen."     TXT_OFF,
 "",
 TXT_MAINTITLE "Contact"                                                          TXT_OFF,
 "",
 TXT_CENTRE "webpage: \33o[1@2]"  /*EMAILADDY*/                                   TXT_OFF,
 TXT_CENTRE "e-mail: \33o[2@3]" /*WEBADDY*/                                       TXT_OFF,
 "",
 TXT_CENTRE "Please refer to the program's document for more"                     TXT_OFF,
 TXT_CENTRE "information on this software."                                       TXT_OFF,
 NULL
};

static UBYTE *AboutPages[] =  { "Credits", "History", NULL };

#define EMAILDISPATCHADDY "mailto:" EMAILADDY "?Subject=" VERS " (" DATE ")..."

SAVEDS ASM(void) AboutClickTrigger(
	REG_A0 (struct Hook *hk),
	REG_A1 (LONG *param),
	REG_A2 (Object *obj) )
{
	
	LONG clickID;

	get(obj, MUIA_NList_ButtonClick, &clickID);

	switch(clickID) {
	case 1:
		DispatchURL(DIRKSADDY);
		break;

	case 2:
		DispatchURL(WEBADDY);
		break;

	case 3:
		DispatchURL(EMAILDISPATCHADDY);
		break;

	default:
		break;
	}
}

OVERLOAD(OM_NEW)
{
	LONG id;
	HOOK(AboutClickTrigger_Hk, AboutClickTrigger)
	DEFTMPDATA;
	CLRTMPDATA;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_Title,     "About VX window",
		MUIA_Window_ID,        MAKE_ID('A','B','V','X'),
		WindowContents, VGroup,
			MUIA_Group_SameHeight, FALSE,
			Child, HGroup,
				GroupFrame,
				MUIA_Group_SameWidth, FALSE,
				Child, RectangleObject, MUIA_HorizWeight, 1, End,
				Child, data.Logo = BodychunkObject,
					MUIA_HorizWeight,           200,
					MUIA_FixWidth,              BODY_vx_logo_Width,
					MUIA_FixHeight,             BODY_vx_logo_Height,
					MUIA_Bitmap_SourceColors,   BODY_vx_logo_Colors,
					MUIA_Bitmap_Width,          BODY_vx_logo_Width,
					MUIA_Bitmap_Height,         BODY_vx_logo_Height,
					MUIA_Bitmap_Transparent,    BODY_vx_logo_Transparent,
					MUIA_Bodychunk_Body,        BODY_vx_logo_Data,
					MUIA_Bodychunk_Compression, BODY_vx_logo_Compression,
					MUIA_Bodychunk_Depth,       BODY_vx_logo_Depth,
					MUIA_Bodychunk_Masking,     BODY_vx_logo_Masking,
				End,
				Child, RectangleObject, MUIA_HorizWeight, 1, End,
			End,
			Child, data.Ver = TextObject,
				TextFrame,
				MUIA_ShortHelp,     HELP_ABOUT_VERSION,
				MUIA_Background,    MUII_TextBack,
				MUIA_Text_PreParse, "\33c",
				MUIA_Text_Contents, VERS " (" DATE ")",
			End,
			Child, RegisterGroup(AboutPages),
				MUIA_Register_Frame, TRUE,

				/************ Page 1 ************/

				Child, data.Listview = NListviewObject,
					MUIA_VertWeight,      200,
					MUIA_ShortHelp,       HELP_ABOUT_LISTVIEW,
					MUIA_CycleChain,      1,
					MUIA_NListview_NList, data.List = NListObject,
						ReadListFrame,
						MUIA_NList_Input,       FALSE,
						MUIA_NList_SourceArray, AboutTextArray,
						MUIA_NList_Format,      "",
						MUIA_NList_Title,       FALSE,
					End,
				End,

				/************ Page 2 ************/

				Child, data.HistoryListview = NListviewObject,
					MUIA_VertWeight,      200,
					MUIA_CycleChain,      1,
					MUIA_NListview_NList, data.HistoryList = NListObject,
						ReadListFrame,
						MUIA_Font,	MUIV_NList_Font_Fixed,
						MUIA_NList_Input,       FALSE,
						MUIA_NList_Active,      MUIV_List_Active_Top,
						MUIA_NList_Format,      "",
						MUIA_NList_Title,       FALSE,
					End,
				End,
			End,
			Child, data.Exit = MyKeyButton("Exit", 'e', HELP_ABOUT_EXIT),
		End,
		TAG_MORE, (ULONG)inittags(msg)
	);

	if (!obj) {
		return 0;
	}

	DoMethod(data.Listview, MUIM_NList_UseImage, NULL, -1, 0);
	CLOSEWIN_METHOD(obj,       MUIM_AboutWin_Close)
	BUTTON_METHOD  (data.Exit, MUIM_AboutWin_Exit)

	for (id = 1; id < 10; id++) {
		DoMethod(data.List, MUIM_Notify, MUIA_NList_ButtonClick, id,
		         data.List, 2, MUIM_CallHook, &AboutClickTrigger_Hk);
	}

	DoMethod(data.List, MUIM_NList_UseImage, SimpleButtonTiny(DIRKSADDY), 0, ~0L);
	DoMethod(data.List, MUIM_NList_UseImage, SimpleButtonTiny(WEBADDY),   1, ~0L);
	DoMethod(data.List, MUIM_NList_UseImage, SimpleButtonTiny(EMAILADDY), 2, ~0L);

	PREPDATA;
	DoMethod(obj, MUIM_AboutWin_LoadHistory, NULL);

	return (ULONG) obj;
}

OVERLOAD(OM_DISPOSE)
{
	GETDATA;

	if (data->HistoryText) {
		MEM_FreeVec(data->HistoryText);
	}

	return DoSuperMethodA(cl, obj, msg);
}

DECLARE(Open)
{
	DoMethod(obj, MUIM_AboutWin_LoadHistory, NULL);
	set(obj, MUIA_Window_Open, TRUE);

	return 0;
}

DECLARE(Close)
{
	set(obj, MUIA_Window_Open, FALSE);

	return 0;
}

DECLARE(Exit)
{
	set(obj, MUIA_Window_Open, FALSE);

	return 0;
}

#define LOADHISTORY_IOBUFFER_SIZE (1024*32)
#define HISTORYFILE_NOTFOUND "Please copy VX.history into VX's home directory."


DECLARE(LoadHistory) // STRPTR filename
{
	GETDATA;

	LONG len;
	BPTR fh;
	Object *histListObj = data->HistoryList;
	STRPTR fn = msg->filename ? msg->filename : (STRPTR) HISTORYFILE;

	DB(dbg("Loading history file: %s...", HISTORYFILE))

	if (data->HistoryText) {
		return 0;
	}

	DoMethod(histListObj, MUIM_List_Clear);

	if ((fh = Open(fn, MODE_OLDFILE))) {
		if (DOSBase->dl_lib.lib_Version >= 40) {
			SetVBuf(fh, NULL, BUF_FULL, LOADHISTORY_IOBUFFER_SIZE);
		}

		Seek(fh, 0, OFFSET_END);
		len = Seek(fh, 0, OFFSET_BEGINNING);

		if ((data->HistoryText = MEM_AllocVec(len + 4))) {
			Read(fh, data->HistoryText, len);
			DoMethod(histListObj, MUIM_List_Insert, data->HistoryText, -2, MUIV_NList_Insert_Bottom);
		}

		Close(fh);

	} else if (IoErr() == ERROR_OBJECT_NOT_FOUND) {

		DoMethod(histListObj, MUIM_List_Insert, HISTORYFILE_NOTFOUND,
		         -2, MUIV_NList_Insert_Bottom);

	} else {

		DoMethod(histListObj, MUIM_List_Insert, "Error while loading VX.history!",
		         -2, MUIV_NList_Insert_Bottom);

	}

	return 0;
}

