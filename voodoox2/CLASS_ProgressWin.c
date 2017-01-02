/*
 $Id: CLASS_ProgressWin.c,v 1.3 2004/01/21 17:47:50 andrewbell Exp $
 Custom class for the progress window.

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
 *  Class Name: ProgressWin
 * Description: Controls the progress window
 *  Superclass: MUIC_Window
 */

#include "vx_include.h"
#include "ProgressWin.h"
#include "vx_main.h"
#include "vx_misc.h"
#include "vx_mem.h"
#include "vx_cfg.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object	*status;
	Object	*gauge;
	Object	*abort;
	Object  *pLamp[6];
	Object  *Listview;
	Object  *List;
	BOOL	_32bitMode;
	BOOL	sizeUnknown;
	ULONG	_32bitTotal;
	ULONG	lastUnit;
	UBYTE	statusTextBuf[256];
	BOOL	abortStatus;
	BOOL    usingLamp;
//------------------------------------------------------------------------------
*/

/* EXPORT
//------------------------------------------------------------------------------
#ifndef VX_INCLUDE_H
#include "vx_include.h"
#endif
#ifndef VX_GLOBAL_H
#include "vx_global.h"
#endif
 
#define OpenProgress( TotalUnits, TitleText )        DoMethod(GETPROGRESSWIN, MUIM_ProgressWin_Open, TotalUnits, TitleText)
#define CloseProgress                                DoMethod(GETPROGRESSWIN, MUIM_ProgressWin_Close)
#define CloseProgressOverride			     DoMethod(GETPROGRESSWIN, MUIM_ProgressWin_CloseAndOverride)
#define ChangeProgressStatus( StatusText, TextFmt )  DoMethod(GETPROGRESSWIN, MUIM_ProgressWin_ChangeStatus, StatusText, TextFmt)
#define ChangeProgressTotal( NewTotal )              DoMethod(GETPROGRESSWIN, MUIM_ProgressWin_ChangeTotal, NewTotal)
#define CheckProgressAbort                           ((BOOL) xget(GETPROGRESSWIN, MUIA_ProgressWin_AbortStatus))
#define UpdateProgress( StatusText, TextFmt, SoFar ) ((BOOL) DoMethod(GETPROGRESSWIN, MUIM_ProgressWin_Update, StatusText, TextFmt, SoFar))
//------------------------------------------------------------------------------
*/

struct ProgressEntry {
	char *pe_source;
	char *pe_dest;
	char *pe_sizeStr;
	long  pe_size;
};

SAVEDS ASM(LONG) ProgressList_ShowFunc(
    REG_A2 (UBYTE **ColumnArray),
    REG_A1 (struct ProgressEntry *pe) )
{
	if (pe) {
		ColumnArray[0] = pe->pe_source;
		ColumnArray[1] = pe->pe_dest;
		ColumnArray[2] = pe->pe_sizeStr;
	} else {
		ColumnArray[0] = "\33bSource";
		ColumnArray[1] = "\33bDest";
		ColumnArray[2] = "\33bSize";
	}
	return 0;
}

SAVEDS ASM(void) ProgressList_KillFunc(
    REG_A2 (APTR Pool),
    REG_A1 (struct ProgressEntry *pe) )
{
	if (!pe) {
		return;
	}

	MEM_FreeVec(pe->pe_source);
	MEM_FreeVec(pe->pe_dest);
	MEM_FreeVec(pe->pe_sizeStr);
	MEM_FreeVec(pe);
}

SAVEDS ASM(struct ProgressEntry *) ProgressList_MakeFunc(
    REG_A2 (APTR Pool),
    REG_A1 (struct ProgressEntry *tmpPe) )
{
	struct ProgressEntry *newPe;

	if ((newPe = MEM_AllocVec(sizeof(struct ProgressEntry)))) {

		newPe->pe_source  = MEM_StrToVec(tmpPe->pe_source);
		newPe->pe_dest    = MEM_StrToVec(tmpPe->pe_dest);
		newPe->pe_size    = tmpPe->pe_size;

		if (tmpPe->pe_size == 0) {
			newPe->pe_sizeStr = MEM_StrToVec("Empty");
		} else if (tmpPe->pe_size == -1) {
			newPe->pe_sizeStr = MEM_StrToVec("Unknown");
		} else {
			newPe->pe_sizeStr = FormatULONGToVec(newPe->pe_size);
		}

		if (!newPe->pe_source || !newPe->pe_dest || !newPe->pe_sizeStr) {
			ProgressList_KillFunc(Pool, newPe);
			newPe = NULL;
		}
	}

	return newPe;
}

struct Hook ProgressList_MakeHook = { {NULL, NULL}, (void *) ProgressList_MakeFunc, NULL, NULL };
struct Hook ProgressList_KillHook = { {NULL, NULL}, (void *) ProgressList_KillFunc, NULL, NULL };
struct Hook ProgressList_ShowHook = { {NULL, NULL}, (void *) ProgressList_ShowFunc, NULL, NULL };

OVERLOAD(OM_NEW)
{
	LONG i;
	Object *grp;
	DEFTMPDATA;
	CLRTMPDATA;

	if ((data.usingLamp = CheckForMCC("Busy.mcc", 0, 0, NULL, NULL))) {
		grp = HGroup,
			//
			// Note: The lamp eats up cycles
			//
			Child, data.pLamp[0] = PsychedelicLampObject, MUIA_Lamp_Type, MUIV_Lamp_Type_Huge, End,
			Child, data.pLamp[1] = PsychedelicLampObject, MUIA_Lamp_Type, MUIV_Lamp_Type_Huge, End,
			Child, data.pLamp[2] = PsychedelicLampObject, MUIA_Lamp_Type, MUIV_Lamp_Type_Huge, End,
			Child, data.status = TextObject,
				TextFrame,
				MUIA_Background,    MUII_TextBack,
				MUIA_Text_PreParse, "\33c",
			End,
   			Child, data.pLamp[3] = PsychedelicLampObject, MUIA_Lamp_Type, MUIV_Lamp_Type_Huge, End,
       		Child, data.pLamp[4] = PsychedelicLampObject, MUIA_Lamp_Type, MUIV_Lamp_Type_Huge, End,
       		Child, data.pLamp[5] = PsychedelicLampObject, MUIA_Lamp_Type, MUIV_Lamp_Type_Huge, End,
		End;
	} else {
		grp = HGroup,
			Child, data.status = TextObject,
				TextFrame,
				MUIA_Background,    MUII_TextBack,
				MUIA_Text_PreParse, "\33c",
			End,
		End;
	}

	if (!grp) {
		return 0;
	}

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_Title,	  "Progress...",
		MUIA_Window_ID,		  MAKE_ID('P','R','O','G'),
		MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Centered,
		MUIA_Window_TopEdge,  MUIV_Window_TopEdge_Centered,
		MUIA_Window_Width,    MUIV_Window_Width_Screen(80),
		MUIA_Window_Height,   MUIV_Window_Height_Screen(40),
		WindowContents, VGroup,
			Child, grp,
			Child, data.gauge = GaugeObject,
				GaugeFrame,
				MUIA_Gauge_Horiz,    TRUE,
				MUIA_Gauge_InfoText, "",
			End,
			Child, ScaleObject,
				MUIA_Scale_Horiz, TRUE,
			End,
			Child, data.Listview = NListviewObject,
				//MUIA_VertWeight,      200,
				MUIA_ShortHelp,       "Files that are being extracted",
				MUIA_CycleChain,      1,
				MUIA_NListview_NList, data.List = NListObject,
					ReadListFrame,
					MUIA_ObjectID,			  MAKE_ID('P','R','G','L'),
					MUIA_NList_Input,         FALSE,
					MUIA_NList_Active,        MUIV_List_Active_Top,
					MUIA_NList_Format,        "BAR,BAR,",
					MUIA_NList_Title,         TRUE,
					MUIA_NList_ConstructHook, &ProgressList_MakeHook,
					MUIA_NList_DestructHook,  &ProgressList_KillHook,
					MUIA_NList_DisplayHook,   &ProgressList_ShowHook,
				End,
			End,

			Child, data.abort = SimpleButton("Abort"),
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	if (data.usingLamp) {
		for (i = 0; i < 6; i++) {
			set(data.pLamp[i], MUIA_PsychedelicLamp_ColourMode, MUIV_PsychedelicLamp_ColourMode_Green);
		}
	}

	data.lastUnit = ~0;

	/* No point setting these to 0 when CLRTMPDATA does that for us.
	data._32bitMode  = FALSE;
	data.sizeUnknown = FALSE;
	data._32bitTotal = 0;
	data.abortStatus = FALSE;
	*/

	DoMethod(obj,
		MUIM_Notify, MUIA_Window_CloseRequest,
		TRUE, obj,
		1, MUIM_ProgressWin_CloseRequest
	);

	DoMethod(data.abort,
		MUIM_Notify, MUIA_Pressed,
		FALSE, obj,
		3, MUIM_Set, MUIA_ProgressWin_AbortStatus, TRUE
	);

	PREPDATA;
	return (ULONG) obj;
}

OVERLOAD(OM_GET)
{
	GETDATA;
	ULONG *store = ((struct opGet *)msg)->opg_Storage;

	switch (((struct opGet *)msg)->opg_AttrID) {
		ATTR(AbortStatus): *store = data->abortStatus;
		return TRUE;
	}

	return DoSuperMethodA(cl, obj, msg);
}

OVERLOAD(OM_SET)
{
	GETDATA;
	struct TagItem *tags = inittags(msg), *tag;

	while ((tag = NextTagItem(&tags))) {
		switch(tag->ti_Tag) {
			ATTR(AbortStatus): data->abortStatus = tag->ti_Data; break;
		}
	}

	return DoSuperMethodA(cl, obj, msg);
}

static void FadeLampOff( struct Data *data )
{
	if (data && data->usingLamp) {
		LONG i;
		for (i = 0; i < 6; i++) {
			DoMethod(data->pLamp[i], MUIM_PsychedelicLamp_FadeOn);
			set(data->pLamp[i], MUIA_PsychedelicLamp_Rolling, TRUE);
		}
	}
}

DECLARE(Open) // ULONG TotalUnits, UBYTE *TitleText
{
	GETDATA;
	ULONG TotalUnits = msg->TotalUnits;
	UBYTE *TitleText = msg->TitleText;

	DoMethod(data->List, MUIM_NList_Clear);

	if (xget(obj, MUIA_Window_Open)) {
		return 0;
	}

	data->abortStatus = FALSE;

	if (!TitleText) {
		TitleText = "Progress of current operation";
	}

	if (TotalUnits == 0 || TotalUnits == ~0) {

		data->sizeUnknown = TRUE;

	} else {
		if (TotalUnits > 0x0000ffff) {
			data->_32bitMode  = TRUE;
			data->_32bitTotal = TotalUnits;
			TotalUnits        = 100;
		} else {
			data->_32bitMode = FALSE;
		}
	}

	if (data->sizeUnknown) {
		set(data->gauge, MUIA_Gauge_Max,     100);
		set(data->gauge, MUIA_Gauge_Current, 100);
	} else {
		set(data->gauge, MUIA_Gauge_Max,     TotalUnits);
		set(data->gauge, MUIA_Gauge_Current, 0);
	}

	set(data->status, MUIA_Text_Contents,  TitleText);
	set(data->gauge,  MUIA_Gauge_InfoText, "");
	set(obj,          MUIA_Window_Open,    TRUE);

	FadeLampOff(data);

	return 0;
}

DECLARE(Close)
{
	GETDATA;

	// Standard way to close a progress window. This version
	// of the function will check the user's preferences to
	// see of they actually want the window left open. So it
	// might not really close it at all. If you really want
	// the window closed, use the CloseAndOverride method below.

	data->abortStatus = TRUE;
	FadeLampOff(data);

	if (!CFG_Get(TAGID_MAIN_LEAVEPROGRESSOPEN)) {
		set(obj, MUIA_Window_Open, FALSE);
	} else {
		// OK, window is being left open. Blank the gauge.
		set(data->status, MUIA_Text_Contents,  "Press the window's close gadget to continue.");
		set(data->gauge, MUIA_Gauge_InfoText, "Finished.");
	}

	return 0;
}

DECLARE(CloseAndOverride)
{
	GETDATA;

	// Same as Close, but TAGID_MAIN_LEAVEPROGRESSOPEN
	// is disregarded and the window is shut right away.

	set(obj, MUIA_Window_Open, FALSE);
	data->abortStatus = TRUE;
	FadeLampOff(data);

	return 0;
}


DECLARE(CloseRequest)
{
	GETDATA;
	// Don't get this method confused with the Close and
	// CloseAndOverride methods. This function is an event
	// handler that is invoked by MUI when the user presses
	// the close gadget at the top left of the window. It
	// will always ignore TAGID_MAIN_LEAVEPROGRESSOPEN!!!
	// Technically in OO it should really be a private method,
	// but this is BOOPSI.

	set(obj, MUIA_Window_Open, FALSE);
	data->abortStatus = TRUE;
	FadeLampOff(data);
	return 0;
}

DECLARE(Update) // UBYTE *StatusText, APTR TextFmt, ULONG UnitsDoneSoFar
{
	GETDATA;
	UBYTE *StatusText    = msg->StatusText;
	APTR TextFmt         = msg->TextFmt;
	ULONG UnitsDoneSoFar = msg->UnitsDoneSoFar;

	//
	// NOTE 22/12/2001:
	//
	// There's a harmless bug here that only affects very large archives.
	// Multiplying the UnitsDoneSoFar by 100 can cause the ULONG to
	// overflow, effectively resetting the progress bar back to 0. So
	// UnitsDoneSoFar would be better off as a 64 bit long long, though
	// my compiler does actually support this. :-/
	//
	// Maybe the dividend and divisor could be scaled down.
	//

	if (!data->sizeUnknown && data->_32bitMode && data->_32bitTotal) {
		UnitsDoneSoFar = (UnitsDoneSoFar * 100) / data->_32bitTotal;
	}

	if (!data->sizeUnknown && (data->lastUnit != UnitsDoneSoFar)) {
		data->lastUnit = UnitsDoneSoFar;
		set(data->gauge, MUIA_Gauge_Current, UnitsDoneSoFar);
	}

	if (StatusText) {
		DoMethod(obj, MUIM_ProgressWin_ChangeStatus, StatusText, TextFmt);
	}

	DoMethod(_app(obj), MUIM_Application_InputBuffered);

	return (ULONG) data->abortStatus;
}

DECLARE(ChangeStatus) // UBYTE *StatusText, APTR TextFmt
{
	GETDATA;

	RawDoFmt(msg->StatusText, msg->TextFmt, (void *) &PutChProc, data->statusTextBuf);
	set(data->gauge, MUIA_Gauge_InfoText, (UBYTE *) &data->statusTextBuf);
	DoMethod(_app(obj), MUIM_Application_InputBuffered);

	return 0;
}

DECLARE(ChangeTotal) // ULONG NewTotal
{
	GETDATA;

	set(data->gauge, MUIA_Gauge_Max, msg->NewTotal);
	data->sizeUnknown = FALSE;
	data->lastUnit    = ~0;
	DoMethod(_app(obj), MUIM_Application_InputBuffered);

	return 0;
}

DECLARE(PrintReport) // UBYTE *sourceName, UBYTE *destName, ULONG size
{
	GETDATA;
	struct ProgressEntry tmpPe;

	tmpPe.pe_source = msg->sourceName;
	tmpPe.pe_dest   = msg->destName;
	tmpPe.pe_size   = msg->size;
	DoMethod(data->List, MUIM_NList_InsertSingle, &tmpPe, MUIV_NList_Insert_Top);

	return 0;
}
