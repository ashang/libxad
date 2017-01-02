/*
 $Id: CLASS_BusyWin.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class which implements a simple busy window.

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
 *  Class Name: BusyWin
 * Description: A small window with a busy bar
 *  Superclass: MUIC_Window
 */

#include "vx_include.h"
#include "BusyWin.h"
#include "vx_arc.h"
#include "vx_misc.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *BusyObj;
//------------------------------------------------------------------------------
*/

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_Title, "Busy - Put the kettle on... :-)",
		MUIA_Window_ID,     MAKE_ID('B','U','S','Y'),
		WindowContents, VGroup,
			Child, data.BusyObj = BusyObject, MUIA_HorizWeight, 30, /*MUIA_FixHeight, 16,*/ End,
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	PREPDATA;

	return (ULONG) obj;
}
