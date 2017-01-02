/*
 $Id: CLASS_CfgPageMain.c,v 1.3 2004/01/22 20:09:10 andrewbell Exp $
 Custom class for the main config page.

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
 *  Class Name: SettingsWinMainPage
 * Description: Main Settings Page
 *  Superclass: MUIC_Group
 */

#include "vx_include.h"
#include "CfgPageMain.h"
#include "vx_cfg.h"
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_misc.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *AutoSelectAll_Tick;
	Object *AutoVirusCheck_Tick;
	Object *KeepFullPath_Tick;
	Object *ShowExtractReq_Tick;
	Object *DefArcPath_PopASL;
	Object *DefArcPath_Str;
	Object *DefArcPath_PopASLButton;
	Object *DefDestPath_PopASLButton;
	Object *DefDestPath_Str;
	Object *TempPath_PopASL;
	Object *TempPath_Str;
	Object *TempPath_PopASLButton;
	Object *DefDestPath_PopASL;
	Object *DoExeCheck_Tick;
	Object *AutoEnterTars_Tick;
	Object *DelLingeringTemps_Tick;
	Object *CheckX_Str;
	Object *CheckX_PopASLButton;
	Object *CheckX_PopASL;
	Object *LeaveProgressOpen_Tick;
	STRPTR TempStr;
//------------------------------------------------------------------------------
*/

// TODO: Make these obsolete
#define ENVID_CFGPAGEMAIN_START             1000
#define ENVID_CFGPAGEMAIN_AUTOSELECTALL     (ENVID_CFGPAGEMAIN_START +  1)
#define ENVID_CFGPAGEMAIN_AUTOVIRUSCHECK    (ENVID_CFGPAGEMAIN_START +  2)
#define ENVID_CFGPAGEMAIN_KEEPFULLPATH      (ENVID_CFGPAGEMAIN_START +  3)
#define ENVID_CFGPAGEMAIN_SHOWEXTRACTREQ    (ENVID_CFGPAGEMAIN_START +  4)
#define ENVID_CFGPAGEMAIN_DEFARCPATHSTR     (ENVID_CFGPAGEMAIN_START +  5)
#define ENVID_CFGPAGEMAIN_DEFDESTPATH       (ENVID_CFGPAGEMAIN_START +  6)
#define ENVID_CFGPAGEMAIN_TEMPPATHSTR       (ENVID_CFGPAGEMAIN_START +  7)
#define ENVID_CFGPAGEMAIN_DOEXECHECK        (ENVID_CFGPAGEMAIN_START +  8)
#define ENVID_CFGPAGEMAIN_AUTOENTERTARS     (ENVID_CFGPAGEMAIN_START +  9)
#define ENVID_CFGPAGEMAIN_DELLINGERINGTEMPS (ENVID_CFGPAGEMAIN_START + 10)
#define ENVID_CFGPAGEMAIN_CHECKX            (ENVID_CFGPAGEMAIN_START + 11)
#define ENVID_CFGPAGEMAIN_LEAVEPROGRESSOPEN (ENVID_CFGPAGEMAIN_START + 12)

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;

	obj = (Object *) DoSuperNew(cl, obj,
		Child, ScrollgroupObject,
			MUIA_Scrollgroup_UseWinBorder, FALSE,
			MUIA_Scrollgroup_Contents, HGroupV,
				VirtualFrame,
				Child, HSpace(0),
				Child, ColGroup(2),
					Child, Label1("Select all entries after loading a new archive?" ),
					Child, data.AutoSelectAll_Tick = MyCheckMarkID(
						CFG_Get(TAGID_MAIN_AUTOSELECTALL), NULL,
						HELP_ST_MAIN_AUTOSELECTALL,
						ENVID_CFGPAGEMAIN_AUTOSELECTALL
					),

					Child, Label1("Always check for viruses after extracting? (needs XVS)"),
					Child, data.AutoVirusCheck_Tick = MyCheckMarkID(
						CFG_Get(TAGID_MAIN_AUTOVIRUSCHECK), NULL,
						HELP_ST_MAIN_AUTOVIRUSCHECK, ENVID_CFGPAGEMAIN_AUTOVIRUSCHECK
					),

					Child, Label1("Keep full path intact when extracting?"),
					Child, data.KeepFullPath_Tick = MyCheckMarkID(
						CFG_Get(TAGID_MAIN_KEEPFULLPATH), NULL,
						HELP_ST_MAIN_KEEPFULLPATH,
						ENVID_CFGPAGEMAIN_KEEPFULLPATH
					),

					Child, Label1("Show requesters before extraction?"),
					Child, data.ShowExtractReq_Tick = MyCheckMarkID(
						CFG_Get(TAGID_MAIN_SHOWEXTRACTREQ), NULL,
						"Display a confirmation requester before extraction.",
						ENVID_CFGPAGEMAIN_SHOWEXTRACTREQ),

					Child, Label1("Check to see if files are executable on double click?"),
					Child, data.DoExeCheck_Tick = MyCheckMarkID(
						CFG_Get(TAGID_MAIN_DOEXECHECK), NULL,
						"Allows you to run executable files inside archives.",
						ENVID_CFGPAGEMAIN_DOEXECHECK
					),

					Child, Label1("Auto-enter compressed UNIX archives? (e.g. TAR.GZ/TGZ/BZ2/RPM/...)"),
					Child, data.AutoEnterTars_Tick = MyCheckMarkID(
						CFG_Get(TAGID_MAIN_AUTOENTERTARS), NULL,
						"Most archives that are created on UNIX are basically files strung\n"
						"together into one big file like a TAR or CPIO, which have then\n"
						"been compressed with a program like GZIP or BZIP2. This often\n"
						"produces better compression ratios since the compressor can locate\n"
						"redundant strings over several files at once. Unfortunately this\n"
						"means when extracting those archives you need to decompress the\n"
						"entire big file first, before you can reach the files inside the\n"
						"embedded archive.\n\n"
						"This option makes life a little quicker by automatically extracting\n"
						"such embedded archives automatically.",
						ENVID_CFGPAGEMAIN_AUTOENTERTARS
					),

					Child, Label1("Delete lingering temporary files on startup?"),
					Child, data.DelLingeringTemps_Tick = MyCheckMarkID(
						CFG_Get(TAGID_MAIN_DELLINGERINGTEMPS), NULL,
						"When you view a file, it's extracted and placed into your temporary\n"
						"dir. If you quit VX while your viewer program is still using\n"
						"the file, then VX can't delete the temp file. This option\n"
						"makes sure such files are removed (if possible) next time VX\n"
						"starts up a new session.\n",
						ENVID_CFGPAGEMAIN_DELLINGERINGTEMPS
					),

					Child, Label1("Leave progress window open after an operation?"),
					Child, data.LeaveProgressOpen_Tick = MyCheckMarkID(
						CFG_Get(TAGID_MAIN_LEAVEPROGRESSOPEN), NULL,
						"When you've finished extracting something, you may want to leave\n"
						"the progress window open. To view the file extraction report.",
						ENVID_CFGPAGEMAIN_LEAVEPROGRESSOPEN
					),

				End,
				Child, HSpace(0),
			End,	/* HGroupV */
		End,	/* ScrollgroupObject */
		Child, ColGroup(2),
			Child, KeyLabel2("Default archive path", 'a'),
			Child, data.DefArcPath_PopASL = PopaslObject,
				MUIA_ShortHelp,       	HELP_ST_MAIN_DEFARCPATH_POPASL,
				MUIA_Popstring_String,	data.DefArcPath_Str = StringObject,
					StringFrame,
					MUIA_ObjectID,        ENVID_CFGPAGEMAIN_DEFARCPATHSTR,
					MUIA_ShortHelp,       HELP_ST_MAIN_DEFARCPATH_STR,
					MUIA_CycleChain,      1,
					MUIA_ControlChar,     'd',
					MUIA_String_MaxLen,   512,
					MUIA_String_Contents, CFG_Get(TAGID_MAIN_DEFARCHIVEPATH),
				End,
				MUIA_Popstring_Button, data.DefArcPath_PopASLButton = PopButton(MUII_PopDrawer),
				ASLFR_TitleText,      "Please selected the default archive drawer...",
				ASLFR_DrawersOnly,     TRUE,
				ASLFR_DoSaveMode,      TRUE,
			End,
			Child, KeyLabel2("Default extract path", 'e'),
			Child, data.DefDestPath_PopASL = PopaslObject,
				MUIA_ShortHelp, HELP_ST_MAIN_DEFDESTPATH_POPASL,
				MUIA_Popstring_String,
				data.DefDestPath_Str = StringObject,
					StringFrame,
					MUIA_ObjectID,        ENVID_CFGPAGEMAIN_DEFDESTPATH,
					MUIA_ShortHelp,       HELP_ST_MAIN_DEFDESTPATH_STR,
					MUIA_CycleChain,      1,
					MUIA_ControlChar,     'd',
					MUIA_String_MaxLen,   512,
					MUIA_String_Contents, CFG_Get(TAGID_MAIN_DEFDESTPATH),
				End,
				MUIA_Popstring_Button, data.DefDestPath_PopASLButton = PopButton(MUII_PopDrawer),
				ASLFR_TitleText,      "Please selected the default destination drawer...",
				ASLFR_DrawersOnly,     TRUE,
				ASLFR_DoSaveMode,      TRUE,
			End,
			Child, KeyLabel2("Temporary files path", 't'),
			Child, data.TempPath_PopASL = PopaslObject,
				MUIA_ShortHelp, HELP_ST_MAIN_TEMPPATH_POPASL,
				MUIA_Popstring_String,
				data.TempPath_Str = StringObject,
					StringFrame,
					MUIA_ObjectID,        ENVID_CFGPAGEMAIN_TEMPPATHSTR,
					MUIA_ShortHelp,       HELP_ST_MAIN_TEMPPATH_STR,
					MUIA_CycleChain,      1,
					MUIA_ControlChar,     'd',
					MUIA_String_MaxLen,   512,
					MUIA_String_Contents, CFG_Get(TAGID_MAIN_TEMPPATH),
				End,
				MUIA_Popstring_Button, data.TempPath_PopASLButton = PopButton(MUII_PopDrawer),
				ASLFR_TitleText,      "Please select a location for temporary files...",
				ASLFR_DrawersOnly,     TRUE,
				ASLFR_DoSaveMode,      TRUE,
			End,

#define HELP_ST_MAIN_CHECKX_POPASL "Open a file requester."
#define HELP_ST_MAIN_CHECKX_STR    "Specify your CheckX command and parameters here."

			Child, KeyLabel2("CheckX command", 'c'),
			Child, data.CheckX_PopASL = PopaslObject,
				MUIA_ShortHelp,        HELP_ST_MAIN_CHECKX_POPASL,
				MUIA_Popstring_String, data.CheckX_Str = StringObject,
					StringFrame,
					MUIA_ObjectID,        ENVID_CFGPAGEMAIN_CHECKX,
					MUIA_ShortHelp,       HELP_ST_MAIN_CHECKX_STR,
					MUIA_CycleChain,      1,
					MUIA_String_MaxLen,   512,
					MUIA_String_Contents, CFG_Get(TAGID_MAIN_CHECKXCMD),
				End,
				MUIA_Popstring_Button, data.CheckX_PopASLButton = PopButton(MUII_PopDrawer),
				ASLFR_InitialDrawer,  "C:",
				ASLFR_TitleText,      "Please select the location of your CheckX command...",
			End,
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	set(data.DefArcPath_PopASLButton,  MUIA_CycleChain, 1);
	set(data.DefDestPath_PopASLButton, MUIA_CycleChain, 1);

	CHECKMARK_METHOD(data.AutoSelectAll_Tick,     MUIM_CfgPageMain_AutoSelectAll)
	CHECKMARK_METHOD(data.AutoVirusCheck_Tick,    MUIM_CfgPageMain_AutoVirusCheck)
	CHECKMARK_METHOD(data.KeepFullPath_Tick,      MUIM_CfgPageMain_KeepFullPath)
	CHECKMARK_METHOD(data.ShowExtractReq_Tick,    MUIM_CfgPageMain_ShowExtractReq)
	CHECKMARK_METHOD(data.DoExeCheck_Tick,        MUIM_CfgPageMain_DoExeCheck)
	CHECKMARK_METHOD(data.AutoEnterTars_Tick,     MUIM_CfgPageMain_AutoEnterTars)
	CHECKMARK_METHOD(data.DelLingeringTemps_Tick, MUIM_CfgPageMain_DelLingeringTemps)
	CHECKMARK_METHOD(data.LeaveProgressOpen_Tick, MUIM_CfgPageMain_LeaveProgressOpen)
	STRING_METHOD(data.DefArcPath_Str,            MUIM_CfgPageMain_DefArcPath)
	STRING_METHOD(data.DefDestPath_Str,           MUIM_CfgPageMain_DefDestPath)
	STRING_METHOD(data.TempPath_Str,              MUIM_CfgPageMain_TempPath)
	STRING_METHOD(data.CheckX_Str,                MUIM_CfgPageMain_CheckXCmd)

	PREPDATA;

	return (ULONG) obj;
}

DECLARE(DefArcPath)
{
	// TODO: Use method params here

	GETDATA;

	get(data->DefArcPath_Str, MUIA_String_Contents, &data->TempStr);

	if (!data->TempStr) {
		return 0;
	}

	CFG_Set(TAGID_MAIN_DEFARCHIVEPATH, (ULONG) data->TempStr);

	return 0;
}

DECLARE(DefDestPath)
{
	GETDATA;

	get(data->DefDestPath_Str, MUIA_String_Contents, &data->TempStr);

	if (!data->TempStr) {
		return 0;
	}

	CFG_Set(TAGID_MAIN_DEFDESTPATH, (ULONG) data->TempStr);

	return 0;
}

DECLARE(TempPath)
{
	GETDATA;

	get(data->TempPath_Str, MUIA_String_Contents, &data->TempStr);

	if (!data->TempStr) {
		return 0;
	}

	CFG_Set(TAGID_MAIN_TEMPPATH, (ULONG) data->TempStr);

	return 0;
}

DECLARE(CheckXCmd)
{
	GETDATA;

	get(data->CheckX_Str, MUIA_String_Contents, &data->TempStr);

	if (!data->TempStr) {
		return 0;
	}

	CFG_Set(TAGID_MAIN_CHECKXCMD, (ULONG) data->TempStr);

	return 0;
}

DECLARE(AutoSelectAll)
{
	GETDATA;
	ULONG state;

	get(data->AutoSelectAll_Tick, MUIA_Selected, &state);
	CFG_Set(TAGID_MAIN_AUTOSELECTALL, state);
	GUI_PrintStatus("Auto select all : %s", (ULONG) (state ? "On" : "Off"));

	return 0;
}

DECLARE(AutoVirusCheck)
{
	GETDATA;
	ULONG state;

	get(data->AutoVirusCheck_Tick, MUIA_Selected, &state);
	CFG_Set(TAGID_MAIN_AUTOVIRUSCHECK, state);
	GUI_PrintStatus("Virus checking : %s", (ULONG) (state ? "On" : "Off"));

	return 0;
}

DECLARE(KeepFullPath)
{
	GETDATA;
	ULONG state;

	get(data->KeepFullPath_Tick, MUIA_Selected, &state);
	CFG_Set(TAGID_MAIN_KEEPFULLPATH, state);
	GUI_PrintStatus("Keep full path: %s", (ULONG) (state ? "On" : "Off"));

	return 0;
}

DECLARE(ShowExtractReq)
{
	GETDATA;
	ULONG state;

	get(data->ShowExtractReq_Tick, MUIA_Selected, &state);
	CFG_Set(TAGID_MAIN_SHOWEXTRACTREQ, state);
	GUI_PrintStatus("Show extract req: %s", (ULONG) (state ? "On" : "Off"));

	return 0;
}

DECLARE(DoExeCheck)
{
	GETDATA;
	ULONG state;

	get(data->DoExeCheck_Tick, MUIA_Selected, &state);
	CFG_Set(TAGID_MAIN_DOEXECHECK, state);
	GUI_PrintStatus("Do exe check: %s", (ULONG) (state ? "On" : "Off"));

	return 0;
}

DECLARE(AutoEnterTars)
{
	GETDATA;
	ULONG state;

	get(data->AutoEnterTars_Tick, MUIA_Selected, &state);
	CFG_Set(TAGID_MAIN_AUTOENTERTARS, state);
	GUI_PrintStatus("Auto-enter tars: %s", (ULONG) (state ? "On" : "Off"));

	return 0;
}

DECLARE(DelLingeringTemps)
{
	GETDATA;
	ULONG state;

	get(data->DelLingeringTemps_Tick, MUIA_Selected, &state);
	CFG_Set(TAGID_MAIN_DELLINGERINGTEMPS, state);
	GUI_PrintStatus("Delete lingering temps: %s", (ULONG) (state ? "On" : "Off"));

	return 0;
}

DECLARE(LeaveProgressOpen)
{
	GETDATA;
	ULONG state;

	get(data->LeaveProgressOpen_Tick, MUIA_Selected, &state);
	CFG_Set(TAGID_MAIN_LEAVEPROGRESSOPEN, state);
	GUI_PrintStatus("Leave progress open: %s", (ULONG) (state ? "On" : "Off"));

	return 0;
}
