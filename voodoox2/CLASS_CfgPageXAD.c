/*
 $Id: CLASS_CfgPageXAD.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class for the XAD config page.

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
 *  Class Name: CfgPageXAD
 * Description: XAD Settings Page
 *  Superclass: MUIC_Group
 */

#include "vx_include.h"
#include "CfgPageXAD.h"
#include "vx_cfg.h"
#include "vx_debug.h"
#include "vx_arc.h"
#include "vx_main.h"
#include "vx_misc.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *NoExtXADClientsTick;
//------------------------------------------------------------------------------
*/

#define ENVID_CFGPAGEXAD_START           2000
#define ENVID_CFGPAGEXAD_NOEXTXADCLIENTS (ENVID_CFGPAGEXAD_START + 1)

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;

	obj = (Object *) DoSuperNew(cl, obj,
		Child, HGroup,
			Child, HSpace(0),
			Child, ColGroup(2),
				Child, Label1("Disable external XAD clients?"),
				Child, data.NoExtXADClientsTick = MyCheckMarkID(CFG_Get(TAGID_XAD_NOEXTXADCLIENTS), NULL, HELP_ST_XAD_NOEXTXADCLIENTS, ENVID_CFGPAGEXAD_NOEXTXADCLIENTS),
			End,
			Child, HSpace(0),
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	CHECKMARK_METHOD(data.NoExtXADClientsTick, MUIM_CfgPageXAD_NoExtXADClients);

	PREPDATA;

	return (ULONG) obj;
}

DECLARE(NoExtXADClients)
{
	GETDATA;
	ULONG state;

	get(data->NoExtXADClientsTick, MUIA_Selected, &state);
	CFG_Set(TAGID_XAD_NOEXTXADCLIENTS, state);
	GUI_PrintStatus("No external XAD clients : %s", (ULONG) (state ? "On" : "Off"));

	return 0;
}
