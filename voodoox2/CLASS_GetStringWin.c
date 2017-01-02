/*
 $Id: CLASS_GetStringWin.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class which implements a "get string" window.

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
 *  Class Name: GetStringWin
 * Description: Prompts user for a string
 *  Superclass: MUIC_Window
 */

#include "vx_include.h"
#include "GetStringWin.h"
#include "vx_mem.h"
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_misc.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *Info;
	Object *String;
	Object *Accept;
	Object *Cancel;
 
	UBYTE *UserString;
	BOOL AbortLoop;
//------------------------------------------------------------------------------
*/

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_Title,     "Input...",
		MUIA_Window_ID,        MAKE_ID('G','E','T','S'),
		MUIA_Window_Height,    MUIV_Window_Height_Visible(30),
		MUIA_Window_Width,     MUIV_Window_Width_Visible(45),
		WindowContents, VGroup,
			Child, data.Info = TextObject,
				MUIA_Text_PreParse, "\33c\33b",
				TextFrame,
				MUIA_Background,    MUII_TextBack,
			End,
			Child, data.String = StringObject,
				StringFrame,
				MUIA_CycleChain,      1,
				MUIA_String_MaxLen,   256,
				MUIA_String_Contents, "",
			End,
			Child, HGroup,
				Child, data.Accept = MyKeyButton("Accept", 'a', ""),
				Child, data.Cancel = MyKeyButton("Cancel", 'c', ""),
			End,
		End, /* PASS WindowContents */
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj,
	         1, MUIM_GetStringWin_Cancel);

	DoMethod(data.Accept, MUIM_Notify, MUIA_Pressed, FALSE, obj,
	         1, MUIM_GetStringWin_Accept);

	DoMethod(data.Cancel, MUIM_Notify, MUIA_Pressed, FALSE, obj,
	         1, MUIM_GetStringWin_Cancel);

	DoMethod(data.String, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj,
	         1, MUIM_GetStringWin_String);

	return (ULONG) obj;
}

DECLARE(GetString) // UBYTE **str_ptr, UBYTE *title, UBYTE *infostr, APTR infofmt
{
	BOOL running = TRUE;
	UBYTE tmpbuf[256];
	GETDATA;

	data->AbortLoop = FALSE;

	if (data->UserString) {
		MEM_FreeVec(data->UserString);
	}

	data->UserString = NULL;

	if (!msg->infostr) {
		msg->infostr = "";
	}

	if (!msg->str_ptr) {
		return 0;
	}

	*(msg->str_ptr) = NULL;
	RawDoFmt(msg->infostr, msg->infofmt, (void *) &PutChProc, &tmpbuf);

	set(data->Info,   MUIA_Text_Contents,       &tmpbuf);
	set(data->String, MUIA_String_Contents,     msg->str_ptr ? *(msg->str_ptr) : (UBYTE *) "");
	set(obj,          MUIA_Window_ActiveObject, data->String);
	set(obj,          MUIA_Window_Open,         TRUE);

	while (running) {
		ULONG sigs = 0, sigevent;

		switch(DoMethod(_app(obj), MUIM_Application_NewInput, &sigs)) {
		case MUIV_Application_ReturnID_Quit:
			running = FALSE;
			break;
		}

		if (sigs && running) {
			sigevent = Wait(sigs | SIGBREAKF_CTRL_C);

			if (sigevent & SIGBREAKF_CTRL_C) {
				running = FALSE;
			}
		}

		if (data->AbortLoop) {
			break;
		}
	}

	set(obj, MUIA_Window_Open, FALSE);

	*(msg->str_ptr) = data->UserString;

	return 0;
}

DECLARE(GetPassword)
{
	/*
	MUIA_Window_Title,     "Password window...",
	MUIA_String_Secret,   TRUE,
	*/

	return 0;
}

DECLARE(Accept)
{
	UBYTE *tmpstr;
	GETDATA;

	if (data->UserString) {
		MEM_FreeVec(data->UserString);
	}

	data->UserString = NULL;
	get(data->String, MUIA_String_Contents, &tmpstr);

	if (tmpstr) {
		data->UserString = MEM_StrToVec(tmpstr);
	}

	set(obj, MUIA_Window_Open, FALSE);
	data->AbortLoop = TRUE;

	return 0;
}

DECLARE(Cancel)
{
	GETDATA;

	if (data->UserString) {
		MEM_FreeVec(data->UserString);
	}

	data->UserString = NULL;
	set(obj, MUIA_Window_Open, FALSE);
	data->AbortLoop = TRUE;

	return 0;
}

DECLARE(String)
{
	return 0;
}
