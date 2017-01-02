/*
 $Id: CLASS_RecentMenu.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class for creating the "recent" menu.

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
 *  Class Name: RecentMenu
 * Description: Handles the recently loaded archives menu.
 *  Superclass: MUIC_Menuitem
 */

#include "vx_include.h"
#include "RecentMenu.h"
#include "vx_mem.h"
#include "vx_misc.h"
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_arc.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	UBYTE	*RecentArcList[MAX_RECENT_COUNTS + 1];
	Object	*Recent_Menu_Objs[MAX_RECENT_COUNTS + 1];
*/

OVERLOAD(OM_NEW)
{
	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Menuitem_Title,		"Open recent",
	    MUIA_Menuitem_CommandString, FALSE,
	    TAG_MORE, (ULONG)inittags(msg)
	);

	if (!obj) {
		return 0;
	}

	return (ULONG) obj;
}

OVERLOAD(OM_DISPOSE)
{
	GETDATA;
	LONG i;

	for (i = 0; i < MAX_RECENT_COUNTS; i++) {

		if (!data->RecentArcList[i]) {
			continue;
		}

		MEM_FreeVec(data->RecentArcList[i]);

		data->RecentArcList[i] = NULL;
	}

	return DoSuperMethodA(cl, obj, msg);
}

DECLARE(Load) // STRPTR recentfile
{
	GETDATA;
	UBYTE line[256], *p, *recentfile = msg->recentfile;
	APTR oldwndptr;
	BPTR fh, lock;
	LONG i;

	if (!recentfile) {
		recentfile = RECENT_LOCATION;
	}

	DB(dbg("RecentMenu/Load(recentfile=%s)", recentfile))

	if (!(fh = Open(recentfile, MODE_OLDFILE))) {
		return 0;
	}

	oldwndptr = VXProcess->pr_WindowPtr;
	DB(dbg("RecentMenu/Load(): Removing non-existant entries..."));
	VXProcess->pr_WindowPtr = (APTR) -1;

	for (i = 0; i < MAX_RECENT_COUNTS; i++) {
		if (!FGets(fh, line, sizeof(line)-1)) {
			break;
		}

		if ((p = strrchr(line, '\n'))) {
			*p = 0;
		}

		/* Don't add archives that don't exist... */

		if ((lock = Lock(line, SHARED_LOCK))) {

			/* TODO: Make this optional, to speed up boot time. */
			UnLock(lock);
			data->RecentArcList[i] = MEM_StrToVec(line);

		} else {

			DB(dbg("RecentMenu: Archive %s doesn't exist anymore, not added to recent menu.", line);)
			--i;
		}
	}

	VXProcess->pr_WindowPtr = oldwndptr;

	DB(dbg("RecentMenu/Load(): Removing non-existant entries done."));

	Close(fh);
	DoMethod(obj, MUIM_RecentMenu_Rebuild);

	return 0;
}

DECLARE(Save) // STRPTR recentfile
{
	GETDATA;
	STRPTR recentfile = msg->recentfile;
	long i, cnt;
	BPTR fh;

	if (!recentfile) {
		recentfile = RECENT_LOCATION;
	}

	for (i = 0, cnt = 0; i < MAX_RECENT_COUNTS; i++) {
		if (data->RecentArcList[i]) {
			cnt++;
		}
	}

	if (cnt == 0) {
		return 0;
	}

	if (!(fh = Open(recentfile, MODE_NEWFILE))) {
		return 0;
	}

	for (i = 0; i < MAX_RECENT_COUNTS; i++) {

		if (data->RecentArcList[i]) {
			FPrintf(fh, "%s\n", data->RecentArcList[i]);
		}
	}

	Close(fh);

	return 0;
}

DECLARE(Add) // STRPTR arcpath
{
	GETDATA;
	UBYTE *tmp_recentarclist[MAX_RECENT_COUNTS + 1], *arcpath = msg->arcpath;

	long i;

	/* Don't insert if already present */

	for (i = 0; i < MAX_RECENT_COUNTS; i++) {

		if (!data->RecentArcList[i]) {
			continue;
		}

		if (Stricmp(arcpath, data->RecentArcList[i]) == 0) {
			return 0;
		}
	}

	/* Free last slot */

	if (data->RecentArcList[MAX_RECENT_COUNTS]) {
		DB(dbg("AddToRecent(): Remove %s", data->RecentArcList[MAX_RECENT_COUNTS]));

		MEM_FreeVec(data->RecentArcList[MAX_RECENT_COUNTS]);
		data->RecentArcList[MAX_RECENT_COUNTS] = NULL;
	}

	/* Shift array up one */

	memcpy(&tmp_recentarclist, &data->RecentArcList, sizeof(UBYTE *) * (MAX_RECENT_COUNTS - 1));
	memcpy(&data->RecentArcList[1], &tmp_recentarclist, sizeof(UBYTE *) * (MAX_RECENT_COUNTS - 1));

	/* Insert new slot */

	data->RecentArcList[0] = MEM_StrToVec(arcpath);
	DoMethod(obj, MUIM_RecentMenu_Rebuild);

	return 0;
}

DECLARE(Rebuild)
{
	GETDATA;
	ULONG i, cnt = 0, mid = MID_RECENT_START;

	for (i = 0; i < MAX_RECENT_COUNTS; i++) {

		if (!data->RecentArcList[i]) {
			continue;
		} else {
			cnt++;
		}

		DoMethod(obj, MUIM_Family_Remove, data->Recent_Menu_Objs[i]);
		DoMethod(obj, MUIM_Family_AddTail,
		         data->Recent_Menu_Objs[i] = MUI_MakeObject(MUIO_Menuitem, data->RecentArcList[i],
		                                     0, MUIO_Menuitem_CopyStrings, mid));
		DoMethod(data->Recent_Menu_Objs[i], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
		         obj, 2, MUIM_RecentMenu_GotoArc, mid++);
	}

	if (cnt == 0) {
		DoMethod(obj, MUIM_Family_AddTail,
		         MUI_MakeObject(MUIO_Menuitem, "(No archives available)", 0, MUIO_Menuitem_CopyStrings, MID_RECENT_NULL));
	}

	return 0;
}

DECLARE(GotoArc) // ULONG id
{
	GETDATA;
	UBYTE *path;
	ULONG id = msg->id;

	if (id < MID_RECENT_START || id == MID_RECENT_NULL) {
		return 0;
	}

	id -= MID_RECENT_START;
	get(data->Recent_Menu_Objs[id], MUIA_Menuitem_Title, &path);

	if (!path) {
		return 0;
	}

	ARC_LoadNewArchive(path, NULL, NULL, FALSE);
	DoMethod(GETARCVIEW, MUIM_ArcView_ShowArcEntries, NULL);

	return 0;
}
