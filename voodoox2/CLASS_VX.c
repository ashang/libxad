/*
 $Id: CLASS_VX.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 The main VX application class.

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
 *  Class Name: VX
 * Description: Application object
 *  Superclass: MUIC_Application
 */

#include "vx_include.h"
#include "VX.h"
#include "vx_main.h"
#include "vx_misc.h"
#include "vx_debug.h"
#include "vx_arc.h"


/* CLASSDATA
//------------------------------------------------------------------------------
	Object *MainWin;
	Object *SettingsWin;
	Object *GetPatternWin;
	Object *ProgressWin;
	Object *GetStringWin;
	Object *ErrorWin;
	Object *SearchWin;
	Object *ArcInfoWin;
	Object *AboutWin;
	Object *MultiPartWin;
	Object *XADInfoWin;
	Object *NewFileTypeWin;
	Object *BusyWin;
	Object *VersionWin;
	Object *ExtFileInfoWin;
	struct DiskObject *DiskObj;
*/

OVERLOAD(OM_NEW)
{
	struct TagItem *tags = inittags(msg), *tag;
	struct ArgLayout *ad = NULL;
	DEFTMPDATA;
	CLRTMPDATA;

	while ((tag = NextTagItem(&tags))) {
		switch(tag->ti_Tag) {
			ATTR(ArgLayout): ad = (struct ArgLayout *) tag->ti_Data;
			break;
		}
	}

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Application_Title,       "VX",
		MUIA_Application_Version,     "$VER: " VERS " (" DATE ")",
		MUIA_Application_Copyright,   "Copyright © " YEAR " Andrew Bell. All rights reserved.",
		MUIA_Application_Author,      "Andrew Bell",
		MUIA_Application_Description, "XAD based unarchive system",
		MUIA_Application_Base,        "VX",
		MUIA_Application_SingleTask,  TRUE,
		MUIA_Application_DiskObject,  data.DiskObj = GetDiskObject("PROGDIR:VX"),
		SubWindow, data.MainWin = MainWinObject, End,
		TAG_MORE, (ULONG)inittags(msg)
	);

	if (!obj) {
		GUI_Popup("Error", "Failed to create MUI application object!", NULL, "Abort");
		return 0;
	}

	DoMethod(obj, MUIM_Notify, MUIA_Application_DoubleStart, MUIV_EveryTime, obj,
	         2, MUIM_VX_DoubleStart);

	set(obj, MUIA_Application_DropObject, data.MainWin);

	if (ad) {
		if (ad->al_DEST) {
			DoMethod(data.MainWin, MUIM_MainWin_SetDestPath, ad->al_DEST);
		}

		if (ad->al_PRI) {
			SetTaskPri((struct Task *)VXProcess, *(ad->al_PRI));
		}

		if (ad->al_VIEWMODE) {
			ARC_ChangeViewMode(ParseViewModeStr(ad->al_VIEWMODE));
		}
	}

	PREPDATA;
	set(data.MainWin, MUIA_Window_Open, TRUE);

	return (ULONG) obj;
}

OVERLOAD(OM_DISPOSE)
{
	GETDATA;

	if (data->DiskObj) {
		FreeDiskObject(data->DiskObj);
	}

	return DoSuperMethodA(cl, obj, msg);
}

OVERLOAD(OM_GET)
{
	GETDATA;
	ULONG *store = ((struct opGet *)msg)->opg_Storage;

	/* Windows are dynamically allocated when something wants a window object pointer.
	   This makes VX's startup quicker because we don't need to create all
	   windows at once. Also helps us cut down on the amount of memory used. */

#define DYNAMICWIN(winname)                  \
		if (data->winname) {                 \
			*store = (ULONG) data->winname;  \
		} else {                             \
			DoMethod(obj, OM_ADDMEMBER, (*store = (ULONG) (data -> winname = (winname ## Object, End)))); \
			if (data->winname && data->MainWin) set(data->winname, MUIA_Window_RefWindow, data->MainWin); \
		}

	switch (((struct opGet *)msg)->opg_AttrID) {
		ATTR(MainWin)        : *store = (ULONG) data->MainWin;
		break;
		ATTR(ProgressWin)    : DYNAMICWIN(ProgressWin)    break;
		ATTR(ErrorWin)       : DYNAMICWIN(ErrorWin)       break;
		ATTR(SettingsWin)    : DYNAMICWIN(SettingsWin)    break;
		ATTR(GetPatternWin)  : DYNAMICWIN(GetPatternWin)  break;
		ATTR(GetStringWin)   : DYNAMICWIN(GetStringWin)   break;
		ATTR(SearchWin)      : DYNAMICWIN(SearchWin)      break;
		ATTR(ArcInfoWin)     : DYNAMICWIN(ArcInfoWin)     break;
		ATTR(AboutWin)       : DYNAMICWIN(AboutWin)       break;
		ATTR(MultiPartWin)   : DYNAMICWIN(MultiPartWin)   break;
		ATTR(XADInfoWin)     : DYNAMICWIN(XADInfoWin)     break;
		ATTR(NewFileTypeWin) : DYNAMICWIN(NewFileTypeWin) break;
		//ATTR(VersionWin)   : DYNAMICWIN(VersionWin)     break;
		ATTR(ExtFileInfoWin) : DYNAMICWIN(ExtFileInfoWin) break;
		ATTR(BusyWin)        : DYNAMICWIN(BusyWin)        break;
	}

	return DoSuperMethodA(cl, obj, msg);
}

DECLARE(DoubleStart)
{
	return 0;
}

