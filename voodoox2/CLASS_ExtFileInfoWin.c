/*
 $Id: CLASS_ExtFileInfoWin.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class which show extended information about a file.

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
 *  Class Name: ExtFileInfo 
 * Description: Show extended information about a file.
 *  Superclass: MUIC_Window
 */

#include <stdarg.h>
#include "vx_include.h"
#include "vx_misc.h"
#include "vx_debug.h"
#include "vx_mem.h"
#include "ExtFileInfoWin.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *List;
	Object *Listview;
//------------------------------------------------------------------------------
*/

//typedef unsigned long uint32;

struct EFIEntry {
	UBYTE field[128], info[1024];
};

SAVEDS ASM(void) EFIListKillFunc(
    REG_A1 (struct NList_DestructMessage *NL_DMsg),
    REG_A2 (Object *Obj) )
{
	if (NL_DMsg->entry) {
		MEM_FreeVec(NL_DMsg->entry);
	}
}

SAVEDS ASM(struct EFIEntry *) EFIListMakeFunc(
    REG_A1 (struct NList_ConstructMessage *NL_CMsg),
    REG_A2 (Object *Obj) )
{
	struct EFIEntry *newentry;

	if (!(newentry = MEM_AllocVec(sizeof(struct EFIEntry)))) {
		return NULL;
	}

	CopyMem(NL_CMsg->entry, newentry, sizeof(struct EFIEntry));

	return newentry;
}

SAVEDS ASM(LONG) EFIListShowFunc(
    REG_A1 (struct NList_DisplayMessage *NL_DMsg),
    REG_A2 (Object *Obj) )
{
	register struct EFIEntry *entry = NL_DMsg->entry;
	register UBYTE **col            = (UBYTE **) NL_DMsg->strings;
	register UBYTE **pp             = (UBYTE **) NL_DMsg->preparses;

	if (entry) {

		*col++ = entry->field;
		*col   = entry->info;
		*pp++  = "\33b";
		*pp    = "";

	} else {

		*col++ = "Field";
		*col   = "Information";
		*pp++  = "\33b";
		*pp    = "";
	}

	return 0;
}

struct Hook EFIListMakeHook = {
	{ NULL, NULL}, (void *) EFIListMakeFunc, NULL, NULL };

struct Hook EFIListKillHook = {
	{ NULL, NULL}, (void *) EFIListKillFunc, NULL, NULL };

struct Hook EFIListShowHook = {
	{ NULL, NULL }, (void *) EFIListShowFunc, NULL, NULL };

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;
	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_Title,     "Extended file information",
		MUIA_Window_ID,        MAKE_ID('E','F','I','W'),
		WindowContents, VGroup,
			Child, data.Listview = NListviewObject,
				MUIA_CycleChain,			1,
				MUIA_NListview_NList,		data.List = NListObject,
					MUIA_ObjectID,				MAKE_ID('E','F','I','L'),
					MUIA_NList_Input,			FALSE,
					MUIA_NList_MultiSelect,		MUIV_NList_MultiSelect_None,
					MUIA_NList_Format,			"BAR,",
					MUIA_NList_Title,			TRUE,
					MUIA_NList_ConstructHook2,	&EFIListMakeHook,
					MUIA_NList_DestructHook2,	&EFIListKillHook,
					MUIA_NList_DisplayHook2,	&EFIListShowHook,
					MUIA_NList_MinColSortable,	0,
				End,
			End,
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	CLOSEWIN_METHOD(obj, MUIM_ExtFileInfoWin_CloseWindow);
	PREPDATA;
	return (ULONG) obj;
}

DECLARE(CloseWindow)
{
	set (obj, MUIA_Window_Open, FALSE);
	return 0;
}

static UBYTE *GetXFIFlagsStr( ULONG flags ) {
	static UBYTE buf[512];
	buf[0] = 0;

	if (flags & XADFIF_CRYPTED)
		strncat(buf, "Crypted ",        sizeof(buf)-1);

	if (flags & XADFIF_DIRECTORY)
		strncat(buf, "Directory ",      sizeof(buf)-1);

	if (flags & XADFIF_LINK)
		strncat(buf, "Link ",           sizeof(buf)-1);

	if (flags & XADFIF_INFOTEXT)
		strncat(buf, "InfoText ",       sizeof(buf)-1);

	if (flags & XADFIF_GROUPED)
		strncat(buf, "Grouped ",        sizeof(buf)-1);

	if (flags & XADFIF_ENDOFGROUP)
		strncat(buf, "EndOfGroup ",     sizeof(buf)-1);

	if (flags & XADFIF_NODATE)
		strncat(buf, "NoDate ",         sizeof(buf)-1);

	if (flags & XADFIF_DELETED)
		strncat(buf, "Deleted ",        sizeof(buf)-1);

	if (flags & XADFIF_SEEKDATAPOS)
		strncat(buf, "SeekDataPos ",    sizeof(buf)-1);

	if (flags & XADFIF_NOFILENAME)
		strncat(buf, "NoFileName ",     sizeof(buf)-1);

	if (flags & XADFIF_NOUNCRUNCHSIZE)
		strncat(buf, "NoUncrunchSize ", sizeof(buf)-1);

	if (flags & XADFIF_PARTIALFILE)
		strncat(buf, "PartialFile ",    sizeof(buf)-1);

	if (flags & XADFIF_MACDATA)
		strncat(buf, "MacData ",        sizeof(buf)-1);

	if (flags & XADFIF_MACRESOURCE)
		strncat(buf, "MacResource ",    sizeof(buf)-1);

	if (flags & XADFIF_EXTRACTONBUILD)
		strncat(buf, "ExtractOnBuild ", sizeof(buf)-1);

	if (flags & XADFIF_UNIXPROTECTION)
		strncat(buf, "UnixProtection ", sizeof(buf)-1);

	if (flags & XADFIF_DOSPROTECTION)
		strncat(buf, "DosProtection ",  sizeof(buf)-1);

	if (flags & XADFIF_ENTRYMAYCHANGE)
		strncat(buf, "EntryMayChange ", sizeof(buf)-1);

	return buf;
}

static UBYTE *GetFileTypeStr( UBYTE flags ) {

	switch (flags) {
	case XADFILETYPE_DATACRUNCHER:
		return "Data Cruncher";

	case XADFILETYPE_TEXTLINKER:
		return "Text-Linked";

	case XADFILETYPE_AMIGAEXECRUNCHER:
		return "Amiga exe cruncher";

	case XADFILETYPE_AMIGAEXELINKER:
		return "Amiga exe linker";

	case XADFILETYPE_AMIGATEXTLINKER:
		return "Amiga text-exe linker";

	case XADFILETYPE_AMIGAADDRESS:
		return "Amiga address cruncher";

	case XADFILETYPE_UNIXBLOCKDEVICE:
		return "Unix block device";

	case XADFILETYPE_UNIXCHARDEVICE:
		return "Unix character device";

	case XADFILETYPE_UNIXFIFO:
		return "Unix pipe";

	case XADFILETYPE_UNIXSOCKET:
		return "Unix socket";

	case XADFILETYPE_MSDOSEXECRUNCHER:
		return "MSDOS exe cruncher";
	}

	return "Unknown";
}

static UBYTE *GetStrFromXADDate( struct xadDate *xdate ) {

	struct DateStamp DS;

	struct DateTime my_dt = {
		{0, 0, 0}, FORMAT_DOS, 0, NULL, NULL, NULL
	};
	
	UBYTE datepart[LEN_DATSTRING * 2], timepart[LEN_DATSTRING * 2], daypart[LEN_DATSTRING * 2];
	static UBYTE buf[((LEN_DATSTRING * 2) * 3) + 4];

	if (!xadConvertDates(
			XAD_DATEXADDATE, (ULONG) xdate,
			XAD_GETDATEDATESTAMP, (ULONG) &DS,
			TAG_DONE)) {

		my_dt.dat_StrDate         = datepart;
		my_dt.dat_StrTime         = timepart;
		my_dt.dat_StrDay          = daypart;
		my_dt.dat_Stamp.ds_Days   = DS.ds_Days;
		my_dt.dat_Stamp.ds_Minute = DS.ds_Minute;
		my_dt.dat_Stamp.ds_Tick   = DS.ds_Tick;

		if (DateToStr((struct DateTime *) &my_dt)) {
			sprintf(buf, "%s %s %s", daypart, datepart, timepart);
			return buf;
		}
	}

	return "Unknown";
}

void Add( Object *list, UBYTE *field, UBYTE *str, ... ) {

	struct EFIEntry newentry;
	va_list ap;

	memset(&newentry, 0, sizeof(struct EFIEntry));
	va_start(ap, str);

	if (!field || !str) {
		return;
	}

	if (strlen(str) > sizeof(newentry.info)-1) {
		str[sizeof(newentry.info)-1] = 0;		// I wish Amiga had vnsprintf()...
	}

	vsprintf(newentry.info, str, ap);
	va_end(ap);

	if (field) {
		strncpy(newentry.field, field, sizeof(newentry.field)-1);
	}

	DoMethod(list, MUIM_NList_InsertSingleWrap, &newentry,
	         MUIV_NList_Insert_Bottom, WRAPCOL1, ALIGN_LEFT);
}

DECLARE(Update) // struct xadFileInfo *xfi
{
	GETDATA;
	struct xadFileInfo *xfi;
	Object *list = data->List;

	set(list, MUIA_NList_Quiet, TRUE);
	DoMethod(list, MUIM_NList_Clear);

	if (!(xfi = msg->xfi)) {

		DB(dbg("ExtFileInfoWin::Update() NULL xfi given!\n"))

		Add(list, "Nothing selected.", NULL);
		set(list, MUIA_NList_Quiet, FALSE);

		return 0;
	}

	Add(list, "Entry Number",        "%lu",     xfi->xfi_EntryNumber);
	Add(list, "Entry Info",          "%s",      xfi->xfi_EntryInfo ? xfi->xfi_EntryInfo : (UBYTE *) "");
	Add(list, "Flags",               "%s",      GetXFIFlagsStr(xfi->xfi_Flags));
	Add(list, "File Name",           "%s",      xfi->xfi_FileName ? xfi->xfi_FileName : (UBYTE *) "");
	Add(list, "File Comment",        "%s",      xfi->xfi_Comment ? xfi->xfi_Comment : (UBYTE *) "");
	Add(list, "File Protection",     "0x%08lx", xfi->xfi_Protection);
	Add(list, "Owner User ID",       "%lu",     xfi->xfi_OwnerUID);
	Add(list, "Owner Group ID",      "%lu",     xfi->xfi_OwnerGID);
	Add(list, "Size",                "%lu",     xfi->xfi_Size);
	Add(list, "Group Crunched Size", "%lu",     xfi->xfi_GroupCrSize);
	Add(list, "Crunched Size",       "%lu",     xfi->xfi_CrunchSize);
	Add(list, "Link Name",           "%s",      xfi->xfi_LinkName ? xfi->xfi_LinkName : (UBYTE *) "");
	Add(list, "Date",                "%s",      GetStrFromXADDate(&xfi->xfi_Date));
	Add(list, "Generation",          "0x%04lx", xfi->xfi_Generation);
	//Add(list, "Data Position",       "0x%08lx", xfi->xfi_DataPos);

	// TODO: Add xfi_MacFork here

	if (xadMasterBase->xmb_LibNode.lib_Version >= 11) {
		Add(list, "Unix Protection",     "0x%04lx", xfi->xfi_UnixProtect);
		Add(list, "MS-DOS Protection",   "0x%02lx", xfi->xfi_DosProtect);
		Add(list, "FileType",            "%s",      GetFileTypeStr(xfi->xfi_FileType));
	}

	set(list, MUIA_NList_Quiet, FALSE);
	return 0;
}
