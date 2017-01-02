/*
 $Id: CLASS_GetPatternWin.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class which implements a "get pattern" window.

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
 *  Class Name: GetPattern
 * Description: Handles the pattern window
 *  Superclass: MUIC_Window
 */

#include "vx_include.h"
#include "GetPatternWin.h"
#include "vx_arc.h"
#include "vx_mem.h"
#include "vx_main.h"
#include "vx_misc.h"
#include "vx_debug.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *String;
	Object *SelPat;
	Object *ClearAll;
//------------------------------------------------------------------------------
*/

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_Title,     "Pattern window...",
		MUIA_Window_ID,        MAKE_ID('P','A','T','T'),
		WindowContents, VGroup,
			Child, HGroup,
				Child, KeyLabel("Pattern:", 'p'),
				Child, data.String = StringObject,
					StringFrame,
					MUIA_CycleChain,      1,
					MUIA_ShortHelp,       HELP_PAT_STRING,
					MUIA_ControlChar,     'p',
					MUIA_String_MaxLen,   256,
					MUIA_String_Contents, "#?",
				End,
			End,
			Child, HGroup,
				Child, data.SelPat   = MyKeyButton("Select pattern", 's', HELP_PAT_SELECTPAT),
				Child, data.ClearAll = MyKeyButton("Clear all",      'c', HELP_PAT_CLEARALL),
			End,
		End,
		TAG_MORE, (ULONG) inittags(msg)
	);

	if (!obj) {
		return 0;
	}

	CLOSEWIN_METHOD(obj,           MUIM_GetPatternWin_Close);
	BUTTON_METHOD  (data.SelPat,   MUIM_GetPatternWin_SelectPat);
	BUTTON_METHOD  (data.ClearAll, MUIM_GetPatternWin_ClearAll);
	STRING_METHOD  (data.String,   MUIM_GetPatternWin_String);

	PREPDATA;

	return (ULONG) obj;
}

DECLARE(Open)
{
	GETDATA;

	set(obj, MUIA_Window_Open, TRUE);
	set(obj, MUIA_Window_ActiveObject, data->String);

	return 0;
}

DECLARE(Close)
{
	set(obj, MUIA_Window_Open, FALSE);

	return 0;
}

DECLARE(SelectPat)
{
	GETDATA;
	struct ArcEntry *ae;
	UBYTE ppbuf[256], *str;
	LONG i = 0;
	INITAV;

	if (!(str = (UBYTE *) xget(data->String, MUIA_String_Contents))) {
		return 0;
	}

	if (ParsePatternNoCase(str, ppbuf, sizeof(ppbuf)-1) != 1) {
		GUI_PrintStatus("Bad pattern string!", 0);
		DisplayBeep(NULL);

		return 0;
	}

	for (;;) {

		UBYTE *name;
		APTR tn;

		if (!(ae = (struct ArcEntry *) DoMethod(av, MUIM_ArcView_GetEntry, i, &tn))) {
			break;
		}

		if (ae->ae_Name) {
			name = ae->ae_Name;
		} else {
			name = "";
		}

		if (MatchPatternNoCase(ppbuf, name)) {
			DB(dbg("Select index %lu, treenode %lx", i, tn))
			DoMethod(av, MUIM_ArcView_Select, i, tn);
		}

		i++;
	}

	DoMethod(GETARCVIEW, MUIM_ArcView_ShowArcInfo);

	return 0;
}

DECLARE(ClearAll)
{
	return DoMethod(GETMAINWIN, MUIM_MainWin_SelNone);
}

DECLARE(String) // STRPTR Str
{
	UBYTE ppbuf[256];

	if (ParsePatternNoCase(msg->Str, ppbuf, sizeof(ppbuf)-1) == 1) {
		return 0;
	}

	GUI_PrintStatus("Bad pattern string!", 0);
	DisplayBeep(NULL);

	return 0;
}
