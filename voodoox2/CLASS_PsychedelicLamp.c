/*
 $Id: CLASS_PsychedelicLamp.c,v 1.2 2004/01/21 17:55:26 andrewbell Exp $
 Custom class for a funky lamp with cool fading.

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
 *  Class Name: PsychedelicLamp
 * Description: Funky lamp with cool fading
 *  Superclass: MUIC_Lamp
 */

#include "vx_include.h"
#include "PsychedelicLamp.h"
#include <stdio.h>
#include <mui/Lamp_mcc.h>
#include "vx_misc.h"


/* CLASSDATA
//------------------------------------------------------------------------------
	struct MUI_InputHandlerNode ihn;
	LONG state;
	LONG rolling;
	LONG colourmode;
	ULONG *spec_ptr;
//------------------------------------------------------------------------------
*/

enum {
    STATE_ON,			// Light is on
    STATE_FADINGON,		// Light is fading on
    STATE_OFF,			// Light is off
    STATE_FADINGOFF		// Light is fading off
};

OVERLOAD(OM_NEW)
{

	DEFTMPDATA;
	CLRTMPDATA;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Lamp_Green, 0,
		TAG_MORE, (ULONG)inittags(msg)
	);

	if (!obj) {
		return 0;
	}

	data.state      = STATE_OFF;
	data.colourmode = MUIV_PsychedelicLamp_ColourMode_Green;

	PREPDATA;

	return (ULONG) obj;
}

OVERLOAD(MUIM_Setup)
{
	struct Data *data;

	if (!DoSuperMethodA(cl, obj, msg)) {
		return FALSE;
	}

	data = INST_DATA(cl, obj);
	data->ihn.ihn_Object = obj;
	data->ihn.ihn_Method = MUIM_PsychedelicLamp_VroomVroom;
	data->ihn.ihn_Flags  = MUIIHNF_TIMER;
	data->ihn.ihn_Millis = 25;

	DoMethod(_app(obj), MUIM_Application_AddInputHandler, &data->ihn);

	return TRUE;
}

OVERLOAD(MUIM_Cleanup)
{
	GETDATA;

	DoMethod(_app(obj), MUIM_Application_RemInputHandler, &data->ihn);

	return DoSuperMethodA(cl, obj, msg);
}

#define LampOn(lo)                         \
	SetAttrs(lo,                           \
			MUIA_Lamp_Red,   0,            \
			MUIA_Lamp_Green, 0xffffffff,   \
			MUIA_Lamp_Blue,  0,            \
			TAG_DONE);

#define LampOff(lo) \
	set(lo, MUIA_Lamp_Color, MUIV_Lamp_Color_Off);


#define SPEC_END 0x00000001

static ULONG fadeon_spec[] =
{
	0x00000000,
	0x10101010,
	0x20202020,
	0x40404040,
	0x60606060,
	0x80808080,
	0xA0A0A0A0,
	0xC0C0C0C0,
	0xE0E0E0E0,
	0xF0F0F0F0,
	0xffffffff,
	SPEC_END
};

static ULONG fadeoff_spec[] =
{
	0xffffffff,
	0xF0F0F0F0,
	0xE0E0E0E0,
	0xC0C0C0C0,
	0xA0A0A0A0,
	0x80808080,
	0x60606060,
	0x40404040,
	0x20202020,
	0x10101010,
	0x00000000,
	SPEC_END
};

DECLARE(VroomVroom)
{
	GETDATA;
	ULONG *spec_ptr = data->spec_ptr;

	if (data->state == STATE_ON || data->state == STATE_OFF) {

		return 0;

	} else if (data->state == STATE_FADINGON) {

		if (*spec_ptr == SPEC_END) {

			if (data->rolling) {
				spec_ptr    = fadeoff_spec;
				data->state = STATE_FADINGOFF;
			} else {
				spec_ptr    = fadeon_spec;
				data->state = STATE_ON;
				return 0;
			}
		}
	} else if (data->state == STATE_FADINGOFF) {

		if (*spec_ptr == SPEC_END) {

			if (data->rolling) {
				spec_ptr    = fadeon_spec;
				data->state = STATE_FADINGON;
			} else {
				spec_ptr    = fadeoff_spec;
				data->state = STATE_OFF;
				return 0;
			}
		}
	}

	SetAttrs(obj,
	         MUIA_Lamp_Red,   (data->colourmode == MUIV_PsychedelicLamp_ColourMode_Red   ? *spec_ptr++ : 0),
	         MUIA_Lamp_Green, (data->colourmode == MUIV_PsychedelicLamp_ColourMode_Green ? *spec_ptr++ : 0),
	         MUIA_Lamp_Blue,  (data->colourmode == MUIV_PsychedelicLamp_ColourMode_Blue  ? *spec_ptr++ : 0),
	         TAG_DONE);

	data->spec_ptr = spec_ptr;

	return 0;
}

DECLARE(FadeOn)
{
	GETDATA;

	if (data->state != STATE_FADINGON) {
		data->spec_ptr = fadeon_spec;
		data->state    = STATE_FADINGON;
	}

	data->rolling = FALSE;

	return 0;
}

DECLARE(FadeOff)
{
	GETDATA;

	if (data->state != STATE_FADINGOFF) {
		data->spec_ptr = fadeoff_spec;
		data->state    = STATE_FADINGOFF;
	}

	data->rolling = FALSE;

	return 0;
}

/* EXPORT
//------------------------------------------------------------------------------
#define MUIV_PsychedelicLamp_ColourMode_Red   0
#define MUIV_PsychedelicLamp_ColourMode_Green 1
#define MUIV_PsychedelicLamp_ColourMode_Blue  2
//------------------------------------------------------------------------------
*/

OVERLOAD(OM_SET)
{
	GETDATA;
	struct TagItem *tags = inittags(msg), *tag;

	while ((tag = NextTagItem(&tags))) {
		switch(tag->ti_Tag) {
			ATTR(Rolling):    data->rolling    = tag->ti_Data; break;
			ATTR(ColourMode): data->colourmode = tag->ti_Data; break;
		}
	}

	return DoSuperMethodA(cl, obj, msg);
}

