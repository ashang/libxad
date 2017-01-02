/*
 $Id: CLASS_WriteDisk.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class for the "write disk" page.

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
 *  Class Name: WriteDisk
 * Description: UI for controlling disk writing.
 *  Superclass: MUIC_Group
 */

#include "vx_include.h"
#include "WriteDisk.h"
#include "vx_main.h"
#include "vx_arc.h"
#include "vx_misc.h"
#include "vx_debug.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *OverwriteTick;
	Object *VerifyTick;
	Object *FormatTick;
	Object *DestDevPop;
	Object *DestDevStr;
	Object *DestDevPopButton;
	Object *DestDevListview;
	Object *DestDevList;
	Object *IgnoreGeoTick;
	Object *UseSectorLabelsTick;
	Object *SetLowHighTick;
	Object *Low;
	Object *High;
	Object *Decompress;
	Object *AddArcText;
	Object *SectorSize;
	Object *TotalSector;
	Object *Cylinders;
	Object *LowCyl;
	Object *HighCyl;
	AHN *ahn;
//------------------------------------------------------------------------------
*/

SAVEDS ASM(void) Device_ObjStrFunc( REG_A2 (Object *PopupList), REG_A1 (Object *StrObj) ) {
	/* Close popup */

	UBYTE *devname;
	DoMethod(PopupList, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, &devname);

	if (!devname) {
		return;
	}

	set(StrObj, MUIA_String_Contents, devname);
}

//------------------------------------------------------------------------------

SAVEDS ASM(BOOL) Device_StrObjFunc( REG_A2 (Object *PopupList), REG_A1 (Object *StrObj) ) {
	/* Open popup */

	long i;
	UBYTE *nextname, *devname = (UBYTE *) xget(StrObj, MUIA_String_Contents);

	if (!devname) {
		return FALSE;
	}

	for (i = 0;; i++) {
		DoMethod(PopupList, MUIM_NList_GetEntry, i, &nextname);

		if (!nextname) {
			set(PopupList, MUIA_NList_Active, MUIV_NList_Active_Off);
			break;
		}

		if (!Stricmp(devname, nextname)) {
			set(PopupList, MUIA_NList_Active, i);
			break;
		}
	}

	return TRUE;
}


struct Hook Device_ObjStrHook = { {
		                                NULL, NULL
	                                }
	                                , (void *) Device_ObjStrFunc, NULL, NULL
                                };
struct Hook Device_StrObjHook = { {
		                                NULL, NULL
	                                }
	                                , (void *) Device_StrObjFunc, NULL, NULL
                                };

void GetDefaultDevices( struct Data *data ) {
	// TODO: This routine should scan the DOS list
	static STRPTR defdrives[] = { "DF0:", "DF1:", "DF2:", "DF3:", NULL };

	DoMethod(data->DestDevList, MUIM_NList_Insert, defdrives, -1, MUIV_NList_Insert_Top);
	set(data->DestDevStr, MUIA_String_Contents, defdrives[0]);
}


OVERLOAD(OM_NEW)
{
	struct TagItem *tags = inittags(msg), *tag;
	DEFTMPDATA;
	CLRTMPDATA;

	while ((tag = NextTagItem(&tags))) {
		switch(tag->ti_Tag) {
			ATTR(AHN): data.ahn = (AHN *) tag->ti_Data;
			break;
		}
	}

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Group_Horiz, FALSE,
		Child, HGroup,
			Child, RectangleObject, End,
			Child, VGroup,
				GroupFrameT("Disk Information"),
				Child, ColGroup(2),
					Child, Label("Additional text"),
					Child, data.AddArcText = TextObject,
						TextFrame,
						MUIA_ShortHelp,     "",
						MUIA_Text_PreParse, "\33c",
						MUIA_Background,    MUII_TextBack,
						MUIA_FixWidthTxt,		"********************",
					End,

					Child, Label("Sector Size"),
					Child, data.SectorSize = TextObject,
						TextFrame,
						MUIA_ShortHelp,     "",
						MUIA_Text_PreParse, "\33c",
						MUIA_Background,    MUII_TextBack,
					End,

					Child, Label("Total Sectors"),
					Child, data.TotalSector = TextObject,
						TextFrame,
						MUIA_ShortHelp,     "",
						MUIA_Text_PreParse, "\33c",
						MUIA_Background,    MUII_TextBack,
					End,

					Child, Label("Cylinders"),
					Child, data.Cylinders = TextObject,
						TextFrame,
						MUIA_ShortHelp,     "",
						MUIA_Text_PreParse, "\33c",
						MUIA_Background,    MUII_TextBack,
					End,

					Child, Label("LowCyl"),
					Child, data.LowCyl = TextObject,
						TextFrame,
						MUIA_ShortHelp,     "",
						MUIA_Text_PreParse, "\33c",
						MUIA_Background,    MUII_TextBack,
					End,

					Child, Label("HighCyl"),
					Child, data.HighCyl = TextObject,
						TextFrame,
						MUIA_ShortHelp,     "",
						MUIA_Text_PreParse, "\33c",
						MUIA_Background,    MUII_TextBack,
					End,
				End,
			End,
			Child, VGroup,
				GroupFrameT("Write disk"),
				Child, HGroup,
					Child, Label1("Destination device"),
					Child, data.DestDevPop = PopobjectObject,
						MUIA_ShortHelp,        HELP_MAIN_ARCPOP,
						MUIA_Popstring_String,
						data.DestDevStr = StringObject,
							StringFrame,
							MUIA_FixWidthTxt,     "DFx:------",
							MUIA_HorizWeight,     70,
							MUIA_ShortHelp,       HELP_MAIN_ARCPOPSTR,
							MUIA_ControlChar,     'a',
							MUIA_String_MaxLen,   512,
							MUIA_String_Contents, "",
							MUIA_CycleChain,      1,
						End,
						MUIA_Popstring_Button,    	data.DestDevPopButton = PopButton(MUII_PopUp),
						MUIA_Popobject_StrObjHook,	&Device_StrObjHook,
						MUIA_Popobject_ObjStrHook,	&Device_ObjStrHook,
						MUIA_Popobject_Object,		data.DestDevListview = NListviewObject,
							MUIA_NListview_NList,		data.DestDevList = NListObject,
								InputListFrame,
								MUIA_CycleChain,			1,
								MUIA_MaxHeight,     	    100,
								MUIA_NList_Title,           FALSE,
								MUIA_NList_ConstructHook,	MUIV_NList_ConstructHook_String,
								MUIA_NList_DestructHook,	MUIV_NList_DestructHook_String,
								//MUIA_NList_DisplayHook,		MUIV_NList_DisplayHook_String,
							End,
						End,
					End,
				End,
				//GroupFrameT("Advanced"),
				Child, ColGroup(2),
					Child, Label1("Verify" ),
					Child, data.VerifyTick    = MyCheckMarkID(FALSE, NULL, "", MAKE_ID('w','d','\x00','\x02')),

					Child, Label1("Format" ),
					Child, data.FormatTick    = MyCheckMarkID(FALSE, NULL, "", MAKE_ID('w','d','\x00','\x03')),

					Child, RectangleObject, End,
					Child, RectangleObject, End,

					Child, Label1("Set Low Cylinder / High Cylinder"),
					Child, data.SetLowHighTick = MyCheckMarkID(FALSE, NULL, "", MAKE_ID('w','d','\x00','\x04')),

					Child, Label1("Low" ),
					Child, data.Low = StringObject,
						StringFrame,
						MUIA_Disabled,			TRUE,
						MUIA_FixWidthTxt,		"000 ",
						MUIA_String_Accept ,	"0123456879",
						MUIA_String_Integer,	0,
						MUIA_CycleChain,		1,
					End,

					Child, Label1("High" ),
					Child, data.High = StringObject,
						StringFrame,
						MUIA_Disabled,			TRUE,
						MUIA_FixWidthTxt,		"000 ",
						MUIA_String_Accept,		"0123456879",
						MUIA_String_Integer,	80,
						MUIA_CycleChain,		1,
					End,

					Child, Label1("Ignore geometry" ),
					Child, data.IgnoreGeoTick       = MyCheckMarkID(FALSE, NULL, "", MAKE_ID('w','d','\x00','\x05')),

					Child, Label1("Use sector labels" ),
					Child, data.UseSectorLabelsTick = MyCheckMarkID(FALSE, NULL, "", MAKE_ID('w','d','\x00','\x06')),
				End,
			End,
			Child, RectangleObject, End,
		End,
		Child, RectangleObject, End,
		Child, MUI_MakeObject(MUIO_HBar, 4),
		Child, RectangleObject, End,
		Child, HGroup,
			Child, RectangleObject, End,
			Child, data.Decompress = MyKeyButton("Start decompressing disk archive", 'w', ""),
			Child, RectangleObject, End,
		End,
		Child, RectangleObject, End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	GetDefaultDevices(&data);

	BUTTON_METHOD(data.Decompress, MUIM_WriteDisk_Decompress);

	DoMethod(data.DestDevListview, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
	         data.DestDevPop, 2, MUIM_Popstring_Close, TRUE);

	DoMethod(data.SetLowHighTick, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
	         data.Low, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

	DoMethod(data.SetLowHighTick, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
	         data.High, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

	set(data.DestDevListview, MUIA_UserData, INST_DATA(cl, obj));

	//	set(obj, MUIA_Disabled, TRUE);

	PREPDATA;

	return (ULONG) obj;
}

OVERLOAD(MUIM_Setup)
{
	AHN *ahn;
	GETDATA;

	if (!DoSuperMethodA(cl, obj, msg)) {
		return FALSE;
	}

	ahn = (AHN *) data->ahn;

	if (ahn && ahn->ahn_DiskAI) {
		DoMethod(obj, MUIM_WriteDisk_UpdateDiskInfo, ahn->ahn_DiskAI);
	}

	return TRUE;
}


SAVEDS ASM(ULONG) DiskProgressFunc(
    REG_A0 (struct Hook *hook),
    REG_A1 (struct xadProgressInfo *xpi) )
{
	ULONG status = xpi->xpi_Status;

	if (CheckSignal(SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
		Printf("DEBUG: Hook was aborted!\n");
		return 0;
	}

	switch (xpi->xpi_Mode) {
	case XADPMODE_ASK: {
			Printf("DEBUG: Hook got XADPMODE_ASK\n");

			if (status & XADPIF_OVERWRITE) {

retry:
				switch (GUI_Popup("Overwrite request",  "File '%s' already exists\nWhat should I do?",
				                  &xpi->xpi_FileName, "Overwrite|Skip|Rename|Abort")) {
					UBYTE *new_name;
					case 0: /* Abort     */
						return 0;
					case 1: /* Overwrite */
						return XADPIF_OK|XADPIF_OVERWRITE;
					case 2: /* Skip      */
						return XADPIF_OK|XADPIF_SKIP;
					case 3: /* Rename    */
						if ((new_name = GUI_GetString(
						                    "Rename file...", "Enter new name for file", NULL, xpi->xpi_FileName))) {
							if ((xpi->xpi_NewName = xadAllocVec(strlen(new_name) + 1, MEMF_ANY))) {
								strcpy(xpi->xpi_NewName, new_name);
							}
	
							return XADPIF_OK | XADPIF_RENAME;
						} else {
							goto retry;
						}
						break;
				}
			}

			if (xpi->xpi_Status & XADPIF_IGNOREGEOMETRY) {
				switch (GUI_Popup("Request", "Drive geometry isn't correct, ignore?", NULL, "Yes|No|Abort")) {
				case 0: /* No    */
					return XADPIF_OK;
				case 1: /* Yes   */
					return XADPIF_IGNOREGEOMETRY | XADPIF_OK;
				case 2: /* Abort */
					return 0;
				}
			}
			break;
		}

	case XADPMODE_PROGRESS: {
			ULONG fmt[4];

			Printf("DEBUG: Hook got XADPMODE_PROGRESS\n");

			if (!xpi->xpi_DiskInfo) {

				OpenProgress(0, "Writing disk...");
				UpdateProgress("Wrote %lu bytes...", &xpi->xpi_CurrentSize, 0);

			} else if (xpi->xpi_DiskInfo->xdi_Flags & (XADDIF_NOCYLINDERS | XADDIF_NOCYLSECTORS)) {

				fmt[0] = xpi->xpi_CurrentSize;
				fmt[1] = xpi->xpi_DiskInfo->xdi_TotalSectors * xpi->xpi_DiskInfo->xdi_SectorSize;
				fmt[2] = xpi->xpi_CurrentSize / xpi->xpi_DiskInfo->xdi_SectorSize;
				fmt[3] = xpi->xpi_DiskInfo->xdi_TotalSectors;
				OpenProgress(fmt[1], "Writing disk...");
				UpdateProgress("Wrote %lu of %lu bytes (%lu/%lu sectors)", &fmt, xpi->xpi_CurrentSize);
			} else {

				ULONG numCyl, fullSize, curCyl, i;
				i = xpi->xpi_DiskInfo->xdi_CylSectors * xpi->xpi_DiskInfo->xdi_SectorSize;
				numCyl   = (xpi->xpi_HighCyl + 1) - (xpi->xpi_LowCyl);
				fullSize = numCyl * i;
				curCyl   = xpi->xpi_CurrentSize / i;
				fmt[0]   = xpi->xpi_CurrentSize;
				fmt[1]   = fullSize;
				fmt[2]   = curCyl;
				fmt[3]   = numCyl;
				UpdateProgress("Wrote %lu of %lu bytes (%lu/%lu cylinders)", &fmt, xpi->xpi_CurrentSize);

			}

			return XADPIF_OK;
		}

	case XADPMODE_END:
		Printf("DEBUG: Hook got XADPMODE_END\n");
		UpdateProgress("Finished.", NULL, 0);
		CloseProgress;

		return XADPIF_OK;

	case XADPMODE_ERROR:
		Printf("DEBUG: Hook got XADPMODE_ERROR\n");
		DoMethod(GETERRWIN, MUIM_ErrorWin_ReportXADError,
		         STR(SID_ERRXAD_DFU_WITH, "Write disk hook got XADPMODE_ERROR"), NULL, xpi->xpi_Error);

		return 0;
	}

	Printf("DEBUG: Aborting with error.\n");

	return 0;	// unknown XADPMODE_#?
}

struct Hook DiskProgressHook = {
	{ NULL, NULL }, (void *) DiskProgressFunc, NULL, NULL
};

typedef unsigned long uint32;

DECLARE(Decompress)
{
	GETDATA;
	LONG err = XADERR_OK;
	STRPTR devName = (STRPTR) xget(data->DestDevStr, MUIA_String_Contents);
	struct xadDeviceInfo *dvi = NULL;
	AHN *ahn = (AHN *) data->ahn;
	UBYTE devNameBuf[64], *p;

	if (ahn == NULL || devName == NULL) {
		return 0;
	}

	strncpy(devNameBuf, devName, sizeof(devNameBuf)-1);

	if ((p = strrchr(devNameBuf, ':'))) {
		*p = 0;
	}

	Printf("devNameBuf=%.20s\n", devNameBuf);

	if ((dvi = (struct xadDeviceInfo *) xadAllocObjectA(XADOBJ_DEVICEINFO, 0))) {
		dvi->xdi_DOSName = devNameBuf;
	} else {
		err = XADERR_NOMEMORY;
	}

	if (err == XADERR_OK) {
		//BOOL highLowSet = (BOOL) xget(data->SetLowHighTick, MUIA_Selected);

		Printf("DEBUG: ahn = %lx, ai = %lx\n", ahn, ahn->ahn_CurrentPN->pn_ai);

		err = xadDiskUnArc(
		          ahn->ahn_CurrentPN->pn_ai,
		          XAD_ENTRYNUMBER,        1,
		          dvi         ? XAD_OUTDEVICE   : TAG_IGNORE, (ULONG) dvi,
		          /*dvi == NULL ? XAD_OUTFILENAME : TAG_IGNORE, (ULONG) devNameBuf,*/
		          //XAD_OVERWRITE,        xget(data->OverwriteTick,       MUIA_Selected),
		          //XAD_VERIFY,           xget(data->VerifyTick,          MUIA_Selected),
		          //XAD_FORMAT,           xget(data->FormatTick,          MUIA_Selected),
		          //XAD_IGNOREGEOMETRY,   xget(data->IgnoreGeoTick,       MUIA_Selected),
		          //XAD_USESECTORLABELS,  xget(data->UseSectorLabelsTick, MUIA_Selected),
		          //highLowSet ? XAD_LOWCYLINDER   : TAG_IGNORE, xget(data->Low,  MUIA_String_Integer),
		          //highLowSet ? XAD_HIGHCYLINDER  : TAG_IGNORE, xget(data->High, MUIA_String_Integer),
		          //XAD_PROGRESSHOOK,     (ULONG) &DiskProgressHook,
		          TAG_DONE);

		//Printf("WriteDisk err = %ld\n", err);

		if (err == XADERR_OK) {
			Printf("DEBUG: XAD said disk write operation was a success!\n");
		} else {
			DoMethod(GETERRWIN, MUIM_ErrorWin_ReportXADError,
			         STR(SID_ERRXAD_DFU_WITH, "Disk decompression failed"), NULL, err);
		}
	}

	xadFreeObjectA(dvi, 0);

	return 0;
}

DECLARE(UpdateDiskInfo) // struct xadDiskInfo *xdi
{
	GETDATA;
	UBYTE buf[128];

	if (!msg->xdi) {
		DB(dbg("Warning got NULL pointer in WriteDisk/UpdateDiskInfo!");)
		return 0;
	}

	set(data->AddArcText, MUIA_Text_Contents,
		        msg->xdi->xdi_EntryInfo ? msg->xdi->xdi_EntryInfo : (UBYTE *) "");

	sprintf(buf, "%lu", msg->xdi->xdi_SectorSize);
	set(data->SectorSize, MUIA_Text_Contents, buf);

	sprintf(buf, "%lu", msg->xdi->xdi_TotalSectors);
	set(data->TotalSector, MUIA_Text_Contents, buf);

	sprintf(buf, "%lu", msg->xdi->xdi_Cylinders);
	set(data->Cylinders, MUIA_Text_Contents, buf);

	sprintf(buf, "%lu", msg->xdi->xdi_LowCyl);
	set(data->LowCyl, MUIA_Text_Contents, buf);

	sprintf(buf, "%lu", msg->xdi->xdi_HighCyl);
	set(data->HighCyl, MUIA_Text_Contents, buf);

	return 0;
}
