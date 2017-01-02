/*
 $Id: CLASS_avmExplorer.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
  Custom class for Explorer archive view mode (AVM).

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
 *  Class Name: avmExplorer
 * Description: Handles the Windows Explorer style viewmode
 *  Superclass: MUIC_Group
 */

#include "vx_include.h"
#include "avmExplorer.h"
#include "vx_arc.h"
#include "vx_debug.h"
#include "vx_misc.h"
#include "vx_main.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *TreePart;
	Object *FilesAndDirsPart;
//------------------------------------------------------------------------------
*/

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;

	obj = (Object *) DoSuperNew(cl, obj,
		Child, HGroup,
		Child, data.TreePart         = avmNListtreeObject,
			MUIA_avmNListtree_DirsOnly,	TRUE,
			MUIA_HorizWeight,			50,
		End,
		Child, BalanceObject, End,
			Child, data.FilesAndDirsPart = avmFilesAndDirsObject,
			MUIA_HorizWeight,			150,
		End,
	End,
	TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	set(data.TreePart,         MUIA_avmNListtree_ExplorerObj,    obj);
	set(data.FilesAndDirsPart, MUIA_avmFilesAndDirs_ExplorerObj, obj);

	PREPDATA;

	return (ULONG) obj;
}

OVERLOAD(OM_DISPOSE)
{
	return DoSuperMethodA(cl, obj, msg);
}

DECLARE(GetActiveAE) // struct ArcEntry **ae_ptr
{
	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_GetActiveAE, msg->ae_ptr);
}

DECLARE(GetEntry) // LONG pos, APTR *tn_ptr
{
	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_GetEntry, msg->pos, msg->tn_ptr);
}

DECLARE(GetSelCnt)
{
	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_GetSelCnt);
}

DECLARE(Clear)
{
	DoMethod(DATAREF->TreePart,         MUIM_avmNListtree_Clear);
	DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_Clear);

	return 0;
}

DECLARE(Quiet) // LONG state
{
	DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_Quiet, msg->state);
	DoMethod(DATAREF->TreePart,         MUIM_avmNListtree_Quiet,    msg->state);

	return 0;
}

DECLARE(Select) // LONG pos, APTR tn
{
	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_Select, msg->pos, msg->tn);
}

DECLARE(TreeActive) // APTR tn
{
	return DoMethod(DATAREF->TreePart, MUIM_avmNListtree_SetActive, msg->tn);
}

DECLARE(SelectAll)
{
	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_SelectAll);
}

DECLARE(SelectNone)
{
	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_SelectNone);
}

DECLARE(NextSelected) // LONG lastpos
{
	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_NextSelected, msg->lastpos);
}

DECLARE(GotoFirst) // ULONG listerpos
{
	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_GotoFirst, msg->listerpos);
}

DECLARE(GetFirst)
{
	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_GetFirst);
}

DECLARE(Jump) // ULONG pos
{
	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_Jump, msg->pos);
}

DECLARE(CountEntries)
{
	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_CountEntries);
}

DECLARE(ShowArcEntries) // AHN *ahn
{
	DoMethod(DATAREF->TreePart,         MUIM_avmNListtree_ShowArcEntries,    msg->ahn);
	DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_ShowArcEntries, msg->ahn);

	return 0;
}

DECLARE(SetColumns) // ULONG viewmask
{
	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_SetColumns, msg->viewmask);
}

DECLARE(ShowColumns) // ULONG viewmask
{
	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_ShowColumns, msg->viewmask);
}

DECLARE(HideColumns) // ULONG viewmask
{
	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_HideColumns, msg->viewmask);
}

DECLARE(Disabled) // ULONG yes
{
	DoMethod(DATAREF->TreePart, MUIM_avmNListtree_Disabled, msg->yes);

	return DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_Disabled, msg->yes);
}

DECLARE(UpdateFilesAndDirsPart) // AHN *ahn, struct ArcEntry *ae
{
	msg->ahn->ahn_CurrentPN->pn_currentae = msg->ae;
	DoMethod(DATAREF->FilesAndDirsPart, MUIM_avmFilesAndDirs_ShowArcEntries, msg->ahn);

	return 0;
}

DECLARE(UpdateTreePart) // AHN *ahn, struct ArcEntry *ae
{
	msg->ahn->ahn_CurrentPN->pn_currentae = msg->ae;
	DoMethod(DATAREF->TreePart, MUIM_avmNListtree_ShowArcEntries, msg->ahn);

	return 0;
}

