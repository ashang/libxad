/*
 $Id: CLASS_NewFileTypeWin.c,v 1.2 2004/01/21 19:23:40 andrewbell Exp $
 Custom class for adding new unknown file types.

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
 *  Class Name: NewFileTypeWin
 * Description: When an unknown filetype is encountered this window will appear.
 *  Superclass: MUIC_Window
 */

#include "vx_include.h"
#include "NewFileTypeWin.h"
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_misc.h"

/* TODO: intergrate this class somehow
         into the main filetype config page */

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *PatternStr;
	Object *ViewerStr;
	Object *ViewerPopASL;
	Object *ViewerPopASLButton;
	Object *CommentStr;
	Object *Save;
	Object *Cancel;
	Object *FileNameText;
 
	UWORD ASL_LastTopEdge;
	UWORD ASL_LastLeftEdge;
	UWORD ASL_LastWidth;
	UWORD ASL_LastHeight;
 
	UBYTE ASL_Path[256];
	UBYTE FileName[256];
//------------------------------------------------------------------------------
*/

static SAVEDS ASM(BOOL) Popup_StartFunc( REG_A2 (Object *PopAsl), REG_A1 (struct TagItem *TL) ) {
	UBYTE *Path = NULL;
	struct Data *data = (struct Data *) xget(PopAsl, MUIA_UserData);

	if (!data || !TL) {
		return FALSE;
	}

	#ifdef ENABLE_MUIASLHACK
	{
		LONG Cnt;
		/* This implementation is a bit hacky because we're hacking into MUI's taglist. */
		for (Cnt = -1; Cnt > -5; Cnt--)
		{
			if (TL[Cnt].ti_Tag == ASLFR_InitialTopEdge)  TL[Cnt].ti_Data = (ULONG) data->ASL_LastTopEdge;
			if (TL[Cnt].ti_Tag == ASLFR_InitialLeftEdge) TL[Cnt].ti_Data = (ULONG) data->ASL_LastLeftEdge;
			if (TL[Cnt].ti_Tag == ASLFR_InitialWidth)    TL[Cnt].ti_Data = data->ASL_LastWidth;
			if (TL[Cnt].ti_Tag == ASLFR_InitialHeight)   TL[Cnt].ti_Data = data->ASL_LastHeight;
		}
	}
	#endif /* ENABLE_MUIASLHACK */

	get(data->ViewerStr, MUIA_String_Contents, &Path);
	TL[0].ti_Tag  = ASLFR_InitialHeight;
	TL[0].ti_Data = (ULONG) data->ASL_LastHeight;

	if (Path) {
		strncpy(data->ASL_Path, Path, sizeof(data->ASL_Path)-1);
		*((UBYTE *)PathPart(data->ASL_Path)) = 0;

		TL[1].ti_Tag  = ASLFR_InitialDrawer;
		TL[1].ti_Data = (ULONG) data->ASL_Path;
		TL[2].ti_Tag  = ASLFR_InitialFile;
		TL[2].ti_Data = (ULONG) FilePart(Path);
	} else {
		TL[1].ti_Tag = TAG_IGNORE;
		TL[2].ti_Tag = TAG_IGNORE;
	}

	TL[3].ti_Tag = TAG_DONE;
	TL[3].ti_Data = 0;

	return TRUE;
}

static SAVEDS ASM(void) Popup_StopFunc( REG_A2 (Object *PopAsl), REG_A1 (struct FileRequester *FR) ) {
	UBYTE TmpPath[256];
	struct Data *data = (struct Data *) xget(PopAsl, MUIA_UserData);

	if (!data || !FR) {
		return;
	}

	data->ASL_LastLeftEdge = FR->fr_LeftEdge;
	data->ASL_LastTopEdge  = FR->fr_TopEdge;
	data->ASL_LastWidth    = FR->fr_Width;
	data->ASL_LastHeight   = FR->fr_Height;
	strncpy(TmpPath, FR->fr_Drawer, sizeof(TmpPath)-1);

	if (!AddPart(TmpPath, FR->fr_File, sizeof(TmpPath)-1)) {
		return;
	}

	set(data->ViewerStr, MUIA_String_Contents, TmpPath);
	////UpdateFTE(data, NULL); // TODO: Fixme
}

static struct Hook Popup_StartHook = {
	{ NULL, NULL }, (void *) Popup_StartFunc, NULL, NULL
};

static struct Hook Popup_StopHook  = {
	{ NULL, NULL }, (void *) Popup_StopFunc,  NULL, NULL
};

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;
	WHERE;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_Title,     "Edit new filetype",
		MUIA_Window_ID,        MAKE_ID('N','F','T','W'),
		WindowContents, VGroup,
			Child, RectangleObject, End,
			Child, TextObject,
				TextFrame,
				MUIA_Text_PreParse, "\33c",
				MUIA_Text_Contents, "\n"
					"Sorry, there is no viewer configured for this type of file.\n"
					"\n"
					"Please use this window to associated a viewer program with\n"
					"this file and the next time you click on a file of this type,\n"
					"your viewer of choice will be launched immediately.\n"
					"\n"
					"If you wish to change this viewer in the future, you can do\n"
					"so via the FileTypes page in the Settings Window."
					"\n",
				MUIA_Background,    MUII_TextBack,
			End,

			Child, RectangleObject, End,
			Child, ColGroup(2),
				Child, Label("Filename"),
				Child, data.FileNameText = TextObject,
					TextFrame,
					MUIA_Text_PreParse, "\33c",
					MUIA_Background,    MUII_TextBack,
				End,

				Child, KeyLabel2("Comment", 'c'),
				Child, data.CommentStr = StringObject,
					StringFrame,
					MUIA_CycleChain,      1,
					MUIA_ShortHelp,       HELP_ST_FTYPES_COMMENTSTR,
					MUIA_ControlChar,     'v',
					MUIA_String_MaxLen,   512,
					MUIA_String_Contents, "",
				End,

				Child, KeyLabel2("Pattern", 'p'),
				Child, data.PatternStr = StringObject,
					StringFrame,
					MUIA_CycleChain,      1,
					MUIA_ShortHelp,       HELP_ST_FTYPES_PATTERN,
					MUIA_ControlChar,     'p',
					MUIA_String_MaxLen,   512,
					MUIA_String_Contents, "",
					MUIA_HorizWeight,     80,
				End,

				Child, KeyLabel2("Viewer", 'v'),
				Child, HGroup,
					Child, data.ViewerStr = StringObject,
						StringFrame,
						MUIA_CycleChain,      1,
						MUIA_ShortHelp,       HELP_ST_FTYPES_VIEWERSTR,
						MUIA_ControlChar,     'v',
						MUIA_String_MaxLen,   512,
						MUIA_String_Contents, "",
					End,
					Child, data.ViewerPopASL = PopaslObject,
						MUIA_ShortHelp,        HELP_ST_FTYPES_VIEWERPOPASL,
						MUIA_Popstring_Button, data.ViewerPopASLButton = PopButton(MUII_PopFile),
						MUIA_Popasl_StartHook, &Popup_StartHook,
						MUIA_Popasl_StopHook,  &Popup_StopHook,
						MUIA_Popasl_Type,      ASL_FileRequest,
						ASLFR_RejectIcons,     TRUE,
						ASLFR_DoPatterns,      TRUE,
						ASLFR_InitialDrawer,   "",
						ASLFR_TitleText,       "Please select a viewer...",
					End,
				End,

			End,
			Child, HGroup,
				Child, data.Save   = MyKeyButton("Save and view", 's', ""),
				Child, data.Cancel = MyKeyButton("Cancel", 'c', ""),
			End,
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	GUI_GetInitialAslDims(
	    &data.ASL_LastLeftEdge,
	    &data.ASL_LastTopEdge,
	    &data.ASL_LastWidth,
	    &data.ASL_LastHeight);

	CLOSEWIN_METHOD(obj,       MUIM_NewFileTypeWin_CloseWindow)
	BUTTON_METHOD(data.Save,   MUIM_NewFileTypeWin_Save)
	BUTTON_METHOD(data.Cancel, MUIM_NewFileTypeWin_Cancel)

	set(data.ViewerPopASL, MUIA_UserData, INST_DATA(cl, obj));

	PREPDATA;

	return (ULONG) obj;
}

/******************************************************************************/

DECLARE(Open) // STRPTR filename
{
	GETDATA;
	UBYTE suffix[32], buf[128], *p;
	strncpy(data->FileName, msg->filename, sizeof(data->FileName)-1);

	if ((p = strrchr(msg->filename, '.'))) {
		p++;

	} else {

		p = FilePart(msg->filename);
	}

	set(data->FileNameText, MUIA_Text_Contents, FilePart(msg->filename));

	strncpy(suffix, p, sizeof(suffix)-1);

	for (p = suffix; *p; p++) {
		*p = toupper(*p);
	}

	sprintf(buf, "%s file", suffix); /* Create comment... */
	set(data->CommentStr, MUIA_String_Contents, buf);
	sprintf(buf, "#?.%s", suffix); /* Create pattern... */
	set(data->PatternStr, MUIA_String_Contents, buf);
	set(data->ViewerStr, MUIA_String_Contents, "multiview");
	set(obj, MUIA_Window_Open, TRUE);

	return 0;
}

DECLARE(CloseWindow)
{
	set(obj, MUIA_Window_Open, FALSE);
	return 0;
}

DECLARE(Save)
{
	GETDATA;

	DoMethod(GETFILETYPESOBJ, MUIM_CfgPageFT_AddFileType,
	         xget(data->PatternStr, MUIA_String_Contents),
	         xget(data->ViewerStr,  MUIA_String_Contents),
	         xget(data->CommentStr, MUIA_String_Contents),
	         FTYPESSTATE_ON);

	set(obj, MUIA_Window_Open, FALSE);
	DoMethod(GETFILETYPESOBJ, MUIM_CfgPageFT_Launch, FilePart(data->FileName), data->FileName);

	return 0;
}

DECLARE(Cancel)
{
	set(obj, MUIA_Window_Open, FALSE);

	return 0;
}
