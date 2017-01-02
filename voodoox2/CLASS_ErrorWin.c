/*
 $Id: CLASS_ErrorWin.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class for the error window.

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
 *  Class Name: ErrorWin
 * Description: Error window handling
 *  Superclass: MUIC_Window
 *
 */

#include "vx_include.h"
#include "ErrorWin.h"
#include "vx_mem.h"
#include "vx_main.h"
#include "vx_misc.h"
#include "vx_debug.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	ULONG ErrorCnt;
	Object *ClearErrors;
	Object *Listview;
	Object *List;
	Object *Exit;
//------------------------------------------------------------------------------
*/

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_Title,     "Error window",
		MUIA_Window_ID,        MAKE_ID('E','R','R','W'),
		WindowContents, VGroup,
			MUIA_Group_SameHeight, FALSE,
			Child, data.Listview = NListviewObject,
				MUIA_VertWeight,      200,
				MUIA_ShortHelp,       HELP_ER_LISTVIEW,
				MUIA_CycleChain,      1,
				MUIA_NListview_NList, data.List = NListObject,
					ReadListFrame,
					MUIA_ObjectID,            MAKE_ID('E','R','R','L'),
					MUIA_NList_Input,         FALSE,
					MUIA_NList_Active,        MUIV_List_Active_Top,
					MUIA_NList_Format,        "",
					MUIA_NList_Title,         FALSE,
					MUIA_NList_ConstructHook, MUIV_NList_ConstructHook_String,
					MUIA_NList_DestructHook,  MUIV_NList_DestructHook_String,
				End,
			End,
			Child, HGroup,
				Child, data.ClearErrors = MyKeyButton("Clear errors", 'c', HELP_ER_CLEARERRORS),
				Child, data.Exit = MyKeyButton("Exit", 'e', HELP_ER_EXIT),
			End,
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	data.ErrorCnt = 0;

	CLOSEWIN_METHOD(obj, MUIM_ErrorWin_CloseWindow);
	BUTTON_METHOD(data.ClearErrors, MUIM_ErrorWin_Clear);
	BUTTON_METHOD(data.Exit,        MUIM_ErrorWin_CloseWindow);
	/*BUTTON_METHOD(data.SaveErrors,  MUIM_ErrorWin_SaveErrors);*/

	PREPDATA;

	return (ULONG) obj;
}

DECLARE(CloseWindow)
{
	set(obj, MUIA_Window_Open, FALSE);
	return 0;
}

DECLARE(Clear)
{
	DoMethod(DATAREF->List, MUIM_NList_Clear);

	return 0;
}

DECLARE(Save) // STRPTR filename
{
	/* Unimplemented */

	return 0;

	// TODO: Get each line, if it contains ESC then filter it and save....
}

DECLARE(ReportError) // UBYTE *body, APTR bodyfmt
{
	GETDATA;
	UBYTE *outbuf;

	data->ErrorCnt += 1;

#define BUFSIZE (4096L)

	if (!(outbuf = MEM_AllocVec(BUFSIZE))) {
		return 0;
	}

	/* TODO: Use sprintf() here instead */

	RawDoFmt("\033E\33b\33iError %004lu\n", &data->ErrorCnt, (void *) &PutChProc, outbuf);
	RawDoFmt(msg->body, msg->bodyfmt, (void *) &PutChProc, outbuf + strlen(outbuf));
	strncat(outbuf, "\n", BUFSIZE-1);

	if (!xget(obj, MUIA_Window_Open)) {
		// If the Error window is not open, then open it...

		set
			(obj, MUIA_Window_Open, TRUE);
		DisplayBeep(NULL);
	} else {
		// If it is already open, then bring it to front and activate it...

		DoMethod(obj, MUIM_Window_ToFront);
		set
			(obj, MUIA_Window_Activate, TRUE);
	}

	DoMethod(data->List, MUIM_NList_InsertWrap, outbuf, -2,
	         MUIV_NList_Insert_Bottom, WRAPCOL0, ALIGN_LEFT);
	DoMethod(data->List, MUIM_NList_Jump, MUIV_NList_Jump_Bottom);
	MEM_FreeVec(outbuf);

	return 0;
}

DECLARE(ReportDOSError) // UBYTE *body, APTR bodyfmt, LONG ioerr
{
	UBYTE bodyfmtbuf[256], doserrstr[80];
	LONG doserrcode;
	ULONG fmtdata[4];

	RawDoFmt(msg->body ? msg->body : (UBYTE *) STR(SID_ERR_DOSFAILED,
	         "Encountered a problem with dos.library!"),
	         msg->bodyfmt, (void *) &PutChProc, bodyfmtbuf);

	doserrcode = msg->ioerr == -1 ? IoErr() : msg->ioerr;

	if (!Fault(doserrcode, "", doserrstr, sizeof(doserrstr)-1)) {
		strcpy(doserrstr, ": unknown error");
	}

	fmtdata[0] = (ULONG) &bodyfmtbuf;
	fmtdata[1] = (ULONG) STR(SID_ERR_DOSCODE, "DOS error code");
	fmtdata[2] = doserrcode;
	fmtdata[3] = (ULONG) &doserrstr;

	DoMethod(obj, MUIM_ErrorWin_ReportError, "%s\n%s %lu %s", &fmtdata);

	return 0;
}

DECLARE(ReportXADError) // UBYTE *BodyTxt, APTR BodyFmt, LONG xaderr
{
	UBYTE TmpBuf[256];
	UBYTE *Fmt[2];

	/* We're not interested in reporting these types of errors... */

	if (msg->xaderr == XADERR_BREAK || msg->xaderr == XADERR_SKIP) {
		return 0;
	}

	if (!(Fmt[1] = xadGetErrorText(msg->xaderr))) {
		return 0;
	}

	if (msg->BodyTxt) {

		RawDoFmt(msg->BodyTxt, msg->BodyFmt, (void *) &PutChProc, TmpBuf);
		Fmt[0] = TmpBuf;

	} else {

		Fmt[0] = "";
	}

	DoMethod(obj, MUIM_ErrorWin_ReportError, "%s\nxadmaster.library error : %s", &Fmt);

	return 0;
}

