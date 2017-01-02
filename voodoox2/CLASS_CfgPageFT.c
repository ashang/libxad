/*
 $Id: CLASS_CfgPageFT.c,v 1.4 2004/01/21 19:23:40 andrewbell Exp $
 Custom class for the filetype config page.

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
 *  Class Name: CfgPageFT
 * Description: FileTypes Settings Page
 *  Superclass: MUIC_Group
 */

#include "vx_include.h"
#include "CfgPageFT.h"
#include "CLASS_CfgPageFT_Defs.h"
#include "vx_mem.h"
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_misc.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *Listview;
	Object *List;
	Object *StateCycle;
	Object *PatternStr;
	Object *ViewerStr;
	Object *ViewerPopASL;
	Object *ViewerPopASLButton;
	Object *CommentStr;
	Object *Add;
	Object *Rem;
	Object *SettingsMenuStrip;
 
	UWORD ASL_LastTopEdge;
	UWORD ASL_LastLeftEdge;
	UWORD ASL_LastWidth;
	UWORD ASL_LastHeight;
	UBYTE ASL_Path[256];
 
 //------------------------------------------------------------------------------
*/

#define LIST_FORMAT_FTYPES  "BAR,BAR,BAR,"

/* EXPORT
//------------------------------------------------------------------------------
enum // Keep in sync with filetypes state cycle gadget
{
	FTYPESSTATE_ON = 0, FTYPESSTATE_OFF
};
//------------------------------------------------------------------------------
*/

static UBYTE *EncodeQuotes( UBYTE *str ) {
	long len;
	UBYTE *vec, *in = str, *out;

	if (!str) {
		return MEM_StrToVec("");
	}

	len = strlen(str) * 2;

	if ((out = vec = MEM_AllocVec(len))) {
		while (*in) {
			if (*in == '"') {
				*out++ = '%';
				*out++ = '2';
				*out++ = '2';
				in++;
				continue;
			}

			if (*in == '%') {
				*out++ = '%';
				*out++ = '%';
				in++;
				continue;
			}

			*out++ = *in++;
		}
	}
	return vec;
}

static UBYTE *DecodeQuotes( UBYTE *str ) {
	long len;
	UBYTE *vec, *in = str, *out;
	LONG ch;

	if (!str) {
		return MEM_StrToVec("");
	}

	len = strlen(str) * 2;

	if (!(out = vec = MEM_AllocVec(len))) {
		return NULL;
	}

	while (*in) {
		UBYTE buf[3];

		if (*in != '%') {
			*out++ = *in++;
			continue;
		} else {
			in++;
		}

		if (*in == '%') {
			in++;
			*out++ = '%';
			continue;
		}

		buf[0] = *in++;
		buf[1] = *in++;
		buf[2] = 0;
		ch = strtol(buf, NULL, 16);
		*out++ = (UBYTE) ch;
	}

	return vec;
}

static void FreeFTE( struct FileTypeEntry *FTE ) {
	if (!FTE) {
		return;
	}

	if (FTE->fte_Pattern) {
		MEM_FreeVec(FTE->fte_Pattern);
	}

	if (FTE->fte_Viewer) {
		MEM_FreeVec(FTE->fte_Viewer);
	}

	if (FTE->fte_Comment) {
		MEM_FreeVec(FTE->fte_Comment);
	}

	MEM_FreeVec(FTE);
}

static struct FileTypeEntry *CreateFTE( UBYTE *Pattern, UBYTE *Viewer, UBYTE *Comment, ULONG State ) {
	struct FileTypeEntry *FTE;

	if (!(FTE = MEM_AllocVec(sizeof(struct FileTypeEntry)))) {
		return NULL;
	}

	FTE->fte_State   = State;
	FTE->fte_Pattern = MEM_StrToVec(Pattern);
	FTE->fte_Viewer  = MEM_StrToVec(Viewer);
	FTE->fte_Comment = MEM_StrToVec(Comment);

	if (!FTE->fte_Pattern || !FTE->fte_Viewer || !FTE->fte_Comment) {
		FreeFTE(FTE);
		FTE = NULL;
	}

	return FTE;
}

static struct FileTypeEntry *GetActiveFTE( struct Data *data ) {
	struct FileTypeEntry *FTE = NULL;

	DoMethod(data->Listview, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, &FTE);
	return FTE;
}

DECLARE(AddFileType) // UBYTE *Pattern, UBYTE *Viewer, UBYTE *Comment, ULONG State
{
	GETDATA;
	struct FileTypeEntry *TmpFTE;

	if (!(TmpFTE = CreateFTE(msg->Pattern, msg->Viewer, msg->Comment, msg->State))) {
		return 0;
	}

	DoMethod(data->Listview, MUIM_NList_InsertSingle, TmpFTE, MUIV_NList_Insert_Bottom);
	set(data->Listview, MUIA_NList_Active, MUIV_NList_Active_Bottom);
	FreeFTE(TmpFTE);
	return 0;
}

static void SetFTE_State( struct FileTypeEntry *FTE, ULONG State ) {
	if (!FTE) {
		return;
	}

	FTE->fte_State = State;
}

static void SetFTE_Pattern( struct FileTypeEntry *FTE, UBYTE *PatternStr ) {
	if (!FTE || !PatternStr || !(PatternStr = MEM_StrToVec(PatternStr))) {
		return;
	}

	if (FTE->fte_Pattern) {
		MEM_FreeVec(FTE->fte_Pattern);
	}

	FTE->fte_Pattern = PatternStr;
}

static void SetFTE_Viewer( struct FileTypeEntry *FTE, UBYTE *ViewerStr ) {
	if (!FTE || !ViewerStr || !(ViewerStr = MEM_StrToVec(ViewerStr))) {
		return;
	}

	if (FTE->fte_Viewer) {
		MEM_FreeVec(FTE->fte_Viewer);
	}

	FTE->fte_Viewer = ViewerStr;
}

static void SetFTE_Comment( struct FileTypeEntry *FTE, UBYTE *CommentStr ) {
	if (!FTE || !CommentStr || !(CommentStr = MEM_StrToVec(CommentStr))) {
		return;
	}

	if (FTE->fte_Comment) {
		MEM_FreeVec(FTE->fte_Comment);
	}

	FTE->fte_Comment = CommentStr;
}

DECLARE(Launch) // STRPTR MatchName, STRPTR FileName
{
	GETDATA;
	STRPTR MatchName = msg->MatchName;
	STRPTR FileName = msg->FileName;
	struct FileTypeEntry *NextFTE = NULL;
	LONG i;

	for (i = 0;;) {
		UBYTE PatBuf[256], CmdLineBuf[128];
		DoMethod(data->Listview, MUIM_NList_GetEntry, i++, &NextFTE);

		if (!NextFTE) {
			break;
		}

		if (NextFTE->fte_State == FTYPESSTATE_OFF) {
			continue;
		}

		if (ParsePatternNoCase(NextFTE->fte_Pattern, PatBuf, sizeof(PatBuf)-1) &&
		        MatchPatternNoCase(PatBuf, MatchName) &&
		        ConstructCommandLine(NextFTE->fte_Viewer, CmdLineBuf, sizeof(CmdLineBuf)-1, FileName)) {
			GUI_PrintStatus("Running \"%s\" (%s)", (ULONG) &CmdLineBuf, NextFTE->fte_Comment);

			if (!MySystem(CmdLineBuf, 0)) {
				/* Fail, TODO: Something exciting... */
			}

			return TRUE;
		}
	}

	// No viewer configured? Then open the New FileType window...
	DoMethod(GETNEWFILETYPEWIN, MUIM_NewFileTypeWin_Open, FileName);

	return FALSE;
}


static SAVEDS ASM(void) FTListKillFunc( REG_A1 (struct NList_DestructMessage *NL_DMsg), REG_A2 (Object *Obj) ) {
	FreeFTE(NL_DMsg->entry);
}

static SAVEDS ASM(struct FileTypeEntry *) FTListMakeFunc( REG_A1 (struct NList_ConstructMessage *NL_CMsg), REG_A2 (Object *Obj) ) {
	struct FileTypeEntry *FTE = NL_CMsg->entry;
	return CreateFTE(FTE->fte_Pattern, FTE->fte_Viewer, FTE->fte_Comment, FTE->fte_State);
}

static SAVEDS ASM(LONG) FTListShowFunc( REG_A1 (struct NList_DisplayMessage *NL_DMsg), REG_A2 (Object *Obj) ) {
	register struct FileTypeEntry *FTE = NL_DMsg->entry;
	register UBYTE **col = (UBYTE **) NL_DMsg->strings;
	register UBYTE **pp  = (UBYTE **) NL_DMsg->preparses;

	if (FTE) {
		/*[0]*/ *col++ = (FTE->fte_State == FTYPESSTATE_ON) ? "On" : "Off";
		/*[1]*/ *col++ = FTE->fte_Pattern;
		/*[2]*/ *col++ = FTE->fte_Viewer;
		/*[3]*/ *col   = FTE->fte_Comment;

	} else {
		/* Render titles */
	
		/*[0]*/ *pp++  = "\33b";
		/*[1]*/ *pp++  = "\33b";
		/*[2]*/ *pp++  = "\33b";
		/*[3]*/ *pp    = "\33b";
		/*[0]*/ *col++ = "State";
		/*[1]*/ *col++ = "Pattern";
		/*[2]*/ *col++ = "Viewer";
		/*[3]*/ *col   = "Comment";
	}
	return 0;
}

static SAVEDS ASM(LONG) FTListSortFunc( REG_A1 (struct NList_CompareMessage *NL_CMsg), REG_A2 (Object *Obj) ) {
	return 0;
}

struct Hook FTListMakeHook = {
	{ NULL, NULL }, (void *) FTListMakeFunc, NULL, NULL
};

struct Hook FTListKillHook = {
	{ NULL, NULL }, (void *) FTListKillFunc, NULL, NULL
};

struct Hook FTListShowHook = {
	{ NULL, NULL }, (void *) FTListShowFunc, NULL, NULL
};

struct Hook FTListSortHook = {
	{ NULL, NULL }, (void *) FTListSortFunc, NULL, NULL
};

UBYTE *StateEntries[] = { "On", "Off", NULL };

static void UpdateFTE( struct Data *data, struct FileTypeEntry *FTE ) {
	if (!FTE) {
		if (!(FTE = GetActiveFTE(data))) {
			return;
		}
	}

	SetFTE_Viewer(FTE,  (UBYTE *) xget(data->ViewerStr,  MUIA_String_Contents));
	SetFTE_Pattern(FTE, (UBYTE *) xget(data->PatternStr, MUIA_String_Contents));
	SetFTE_Comment(FTE, (UBYTE *) xget(data->CommentStr, MUIA_String_Contents));
	SetFTE_State(FTE,   xget(data->StateCycle, MUIA_Cycle_Active));
	DoMethod(data->Listview, MUIM_NList_Redraw, MUIV_NList_Redraw_Active);
}

static SAVEDS ASM(BOOL) Popup_StartFunc( REG_A2 (Object *PopAsl), REG_A1 (struct TagItem *TL) ) {
	UBYTE *Path;
	struct Data *data = (struct Data *) xget(PopAsl, MUIA_UserData);

	if (!data) {
		return FALSE;
	}

	if (!TL) {
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
	#endif
		
	TL[0].ti_Tag  = ASLFR_InitialHeight;
	TL[0].ti_Data = (ULONG) data->ASL_LastHeight;

	if ((Path = (UBYTE *) xget(data->ViewerStr, MUIA_String_Contents))) {
		strncpy(data->ASL_Path, Path, sizeof(data->ASL_Path)-1);
		*((UBYTE *)PathPart(data->ASL_Path)) = '\0';

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
	UpdateFTE(data, NULL);
}

struct Hook Popup_StartHook = {
	{ NULL, NULL }, (void *) Popup_StartFunc, NULL, NULL
};

struct Hook Popup_StopHook  = {
	{ NULL, NULL}, (void *) Popup_StopFunc,  NULL, NULL
};

OVERLOAD(OM_NEW)
{
	DEFTMPDATA;
	CLRTMPDATA;

	obj = (Object *) DoSuperNew(cl, obj,
		Child, data.Listview = NListviewObject,
			MUIA_ShortHelp,				HELP_ST_FTYPES_LISTVIEW,
			MUIA_HorizWeight,			200,
			MUIA_NListview_NList,		data.List = NListObject,
				MUIA_ObjectID,				MAKE_ID('F','L','T','Y'),
				MUIA_CycleChain,			1,
				MUIA_NList_Input,			TRUE,
				MUIA_NList_MultiSelect,		MUIV_NList_MultiSelect_None,
				MUIA_NList_Format,			LIST_FORMAT_FTYPES,
				MUIA_NList_Active,			MUIV_NList_Active_Top,
				MUIA_NList_Title,			TRUE,
				MUIA_NList_AutoVisible,		TRUE,
				MUIA_NList_ConstructHook2,	&FTListMakeHook,
				MUIA_NList_DestructHook2,	&FTListKillHook,
				MUIA_NList_DisplayHook2,	&FTListShowHook,
				MUIA_NList_CompareHook2,	&FTListSortHook,
				MUIA_NList_MinColSortable,	0,
				MUIA_NList_DragSortable,	TRUE,
			End,
		End,	/* Listview object */
		
		Child, VGroup,
			MUIA_Frame,           MUIV_Frame_Group,
			MUIA_Group_SameWidth, FALSE,
			Child, ColGroup(2),

				Child, KeyLabel2("State", 't'),
				Child, HGroup,
					Child, data.StateCycle = CycleObject,
						MUIA_ShortHelp,     HELP_ST_FTYPES_STATECYCLE,
						MUIA_CycleChain,    1,
						MUIA_ControlChar,   't',
						MUIA_Cycle_Active,  0,
						MUIA_Font,          MUIV_Font_Button,
						MUIA_Cycle_Entries, StateEntries,
						MUIA_HorizWeight,   20,
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
				
				Child, KeyLabel2("Comment", 'c'),
				Child, data.CommentStr = StringObject,
					StringFrame,
					MUIA_CycleChain,      1,
					MUIA_ShortHelp,       HELP_ST_FTYPES_COMMENTSTR,
					MUIA_ControlChar,     'v',
					MUIA_String_MaxLen,   512,
					MUIA_String_Contents, "",
				End,
			End,	// End ColGroup
		End,
		Child, ColGroup(3),
			Child, data.Add = MyKeyButton("Add filetype",'a', HELP_ST_FTYPES_ADD),
			Child, RectangleObject, End,
			Child, data.Rem = MyKeyButton("Remove filetype",'r', HELP_ST_FTYPES_REM),
		End,
		TAG_MORE, (ULONG)inittags(msg));


	if (!obj) {
		return 0;
	}

	GUI_GetInitialAslDims(&data.ASL_LastLeftEdge, &data.ASL_LastTopEdge,
	                      &data.ASL_LastWidth, &data.ASL_LastHeight);

	BUTTON_METHOD(data.Add, MUIM_CfgPageFT_Add);
	BUTTON_METHOD(data.Rem, MUIM_CfgPageFT_Remove);

	/* Setup notify on filetype string gadgets to activate the next
	   string gadget when Return is pressed. */

	NLIST_SCLICK_METHOD(data.List,   MUIM_CfgPageFT_ListSingleClick);
	NLIST_DCLICK_METHOD(data.List,   MUIM_CfgPageFT_ListDoubleClick);
	CYCLE_METHOD(data.StateCycle,    MUIM_CfgPageFT_StateCycle);
	STRINGCN_METHOD(data.ViewerStr,  MUIM_CfgPageFT_ViewerStr);
	STRINGCN_METHOD(data.CommentStr, MUIM_CfgPageFT_CommentStr);
	STRINGCN_METHOD(data.PatternStr, MUIM_CfgPageFT_PatternStr);
	STRINGCN_METHOD(data.PatternStr, MUIM_CfgPageFT_PatternStr);

	set(data.ViewerPopASLButton, MUIA_CycleChain, 1);
	set(data.ViewerPopASL,       MUIA_UserData, INST_DATA(cl, obj));

	PREPDATA;

	if (!DoMethod(obj, MUIM_CfgPageFT_Load, NULL)) {
		DoMethod(obj, MUIM_CfgPageFT_AddFileType, "#?", "multiview \"%fp\"", "Default viewer", FTYPESSTATE_ON);
	}

	return (ULONG) obj;
}

OVERLOAD(MUIM_Setup)
{
	GETDATA;

	if (!DoSuperMethodA(cl, obj, msg)) {
		return FALSE;
	}

	DoMethod(data->PatternStr, MUIM_Notify,
	         MUIA_String_Acknowledge, MUIV_EveryTime, _win(obj),
	         3, MUIM_Set, MUIA_Window_ActiveObject, data->ViewerStr);
	DoMethod(data->ViewerStr, MUIM_Notify,
	         MUIA_String_Acknowledge, MUIV_EveryTime, _win(obj),
	         3, MUIM_Set, MUIA_Window_ActiveObject, data->CommentStr);
	DoMethod(data->CommentStr, MUIM_Notify,
	         MUIA_String_Acknowledge, MUIV_EveryTime, _win(obj),
	         3, MUIM_Set, MUIA_Window_ActiveObject, data->PatternStr);

	return TRUE;
}

DECLARE(Load) // STRPTR FileName
{
	GETDATA;
	STRPTR FileName = msg->FileName;

	if (FileName == NULL) {
		FileName = FTYPES_FILENAME;
	}

	DoMethod(data->Listview, MUIM_NList_Clear);
	return DoMethod(obj, MUIM_CfgPageFT_Append, FileName);
}

DECLARE(Save) // STRPTR FileName
{
	GETDATA;

	STRPTR FileName = msg->FileName;
	struct FileTypeEntry *NextFTE = NULL;
	BOOL Success = FALSE;
	BPTR OutFH;
	LONG R = 0, i;
	UBYTE *pvec, *vvec, *cvec;

	GUI_PrintStatus("Saving filetypes...", 0);

	if (FileName == NULL) {
		FileName = FTYPES_FILENAME;
	}

	if (!(OutFH = Open(FileName, MODE_NEWFILE))) {
		return 0;
	}

	if (DOSBase->dl_lib.lib_Version >= 40) {
		R = SetVBuf(OutFH, NULL, BUF_FULL, FTYPES_IOBUFFER_SIZE);
	}

	if (R == 0) {
		FPrintf(OutFH, FTYPES_HEADER);

		for(i = 0;;) {
			DoMethod(data->Listview, MUIM_NList_GetEntry, i++, &NextFTE);

			if (!NextFTE) {
				break;
			}

			FPrintf(OutFH, FTYPES_SAVETEMPLATE,
			        (NextFTE->fte_State == FTYPESSTATE_ON ? "ON" : "OFF"),
			        pvec = EncodeQuotes(NextFTE->fte_Pattern),
			        vvec = EncodeQuotes(NextFTE->fte_Viewer),
			        cvec = EncodeQuotes(NextFTE->fte_Comment));

			if (pvec)
				MEM_FreeVec(pvec);
			if (vvec)
				MEM_FreeVec(vvec);
			if (cvec)
				MEM_FreeVec(cvec);
		}

		Success = TRUE;
	}

	Close(OutFH);

	return (ULONG) Success;
}

DECLARE(Append) // UBYTE *FileName
{
	GETDATA;

	UBYTE *FileName = msg->FileName;
	BOOL Success = FALSE, decode;
	UBYTE *pvec, *vvec, *cvec;
	struct RDArgs *RDA;
	struct FT_Template FTT;
	UBYTE LineBuf[256];
	BPTR InFH;
	LONG r = 0;

	if (!(InFH = Open(FileName, MODE_OLDFILE))) {
		return 0;
	}

	if (DOSBase->dl_lib.lib_Version >= 40) {
		r = SetVBuf(InFH, NULL, BUF_FULL, FTYPES_IOBUFFER_SIZE);
	}

	if (r == 0 && FGets(InFH, LineBuf, sizeof(LineBuf)-1)) {

		if (Strnicmp(LineBuf, OLDFTYPES_ID, sizeof(OLDFTYPES_ID)-1) == 0) {
			decode = FALSE;
			goto valid_id_found;
		} else if (Strnicmp(LineBuf, FTYPES_ID, sizeof(FTYPES_ID)-1) == 0) {
			decode = TRUE;
			goto valid_id_found;
		}

		GUI_PrgError("Invalid filetypes ID!", NULL);
		goto quick_and_dirty_exit;

valid_id_found:
		if ((RDA = AllocDosObject(DOS_RDARGS, NULL))) {

			memset(&FTT, 0, sizeof(struct FT_Template));
			set(data->Listview, MUIA_NList_Quiet, TRUE);

			for (;;) {
				if (!FGets(InFH, LineBuf, sizeof(LineBuf)-1)) {
					break;
				}

				if (LineBuf[0] == ';') /* Ignore commented lines */
				{
					continue;
				}

				RDA->RDA_Source.CS_Buffer = LineBuf;
				RDA->RDA_Source.CS_Length = strlen(LineBuf);
				RDA->RDA_Source.CS_CurChr = 0;

				if (!ReadArgs(FTYPES_TEMPLATE, (LONG *)&FTT, RDA)) {
					FreeArgs(RDA);
					continue;
				}

				DoMethod(obj, MUIM_CfgPageFT_AddFileType,
				         pvec = decode ? DecodeQuotes(FTT.ftt_Pattern) : MEM_StrToVec(FTT.ftt_Pattern),
				         vvec = decode ? DecodeQuotes(FTT.ftt_Viewer)  : MEM_StrToVec(FTT.ftt_Viewer),
				         cvec = decode ? DecodeQuotes(FTT.ftt_Comment) : MEM_StrToVec(FTT.ftt_Comment),
				         (Stricmp(FTT.ftt_State, "ON") == 0) ?
				         FTYPESSTATE_ON : FTYPESSTATE_OFF);

				if (pvec) MEM_FreeVec(pvec);
				if (vvec) MEM_FreeVec(vvec);
				if (cvec) MEM_FreeVec(cvec);
			}

			set(data->Listview, MUIA_NList_Quiet, FALSE);
			Success = TRUE;
			FreeDosObject(DOS_RDARGS, RDA);
		}
	}

quick_and_dirty_exit:

	Close(InFH);

	if (!Success) {
		GUI_PrgError("Problem while loading filetypes!", NULL);
	}

	return Success;
}

DECLARE(ListSingleClick)
{
	GETDATA;

	ULONG sel;
	struct FileTypeEntry *FTE;
	get(data->Listview, MUIA_NList_Active, &sel);

	if (sel == MUIV_NList_Active_Off) {
		return 0;
	}

	if (!(FTE = GetActiveFTE(data))) {
		return 0;
	}

	set(data->ViewerStr,  MUIA_String_Contents, FTE->fte_Viewer);
	set(data->PatternStr, MUIA_String_Contents, FTE->fte_Pattern);
	set(data->CommentStr, MUIA_String_Contents, FTE->fte_Comment);
	set(data->StateCycle, MUIA_Cycle_Active,    FTE->fte_State);
	DoMethod(obj, MUIM_CfgPageFT_GhostPanelControl);

	return 0;
}

DECLARE(ListDoubleClick)
{
	GETDATA;
	ULONG tmp = 0, newstate, sel;

	get(data->Listview, MUIA_NList_Active, &sel);

	if (sel == MUIV_NList_Active_Off) {
		return 0;
	}

	get(data->StateCycle, MUIA_Cycle_Active, &tmp);
	newstate = (tmp == FTYPESSTATE_ON) ? FTYPESSTATE_OFF : FTYPESSTATE_ON;
	set(data->StateCycle, MUIA_Cycle_Active, newstate);
	SetFTE_State(GetActiveFTE(data), newstate);
	DoMethod(data->Listview, MUIM_NList_Redraw, MUIV_NList_Redraw_Active);
	DoMethod(obj, MUIM_CfgPageFT_GhostPanelControl);

	return 0;
}

DECLARE(ViewerStr)
{
	GETDATA;
	UBYTE *tmpStr = NULL;

	get(data->ViewerStr, MUIA_String_Contents, &tmpStr);
	SetFTE_Viewer(GetActiveFTE(data), tmpStr);
	DoMethod(data->Listview, MUIM_NList_Redraw, MUIV_NList_Redraw_Active);

	return 0;
}

DECLARE(CommentStr)
{
	GETDATA;

	UBYTE *tmpStr = NULL;

	get(data->CommentStr, MUIA_String_Contents, &tmpStr);
	SetFTE_Comment(GetActiveFTE(data), tmpStr);
	DoMethod(data->Listview, MUIM_NList_Redraw, MUIV_NList_Redraw_Active);

	return 0;
}

DECLARE(PatternStr)
{
	GETDATA;

	UBYTE *tmpStr = NULL;

	get(data->PatternStr, MUIA_String_Contents, &tmpStr);
	SetFTE_Pattern(GetActiveFTE(data), tmpStr);
	DoMethod(data->Listview, MUIM_NList_Redraw, MUIV_NList_Redraw_Active);

	return 0;
}

DECLARE(StateCycle)
{
	GETDATA;

	ULONG tmp = 0;

	get(data->StateCycle, MUIA_Cycle_Active, &tmp);

	SetFTE_State(GetActiveFTE(data), tmp);
	DoMethod(data->Listview, MUIM_NList_Redraw, MUIV_NList_Redraw_Active);

	return 0;
}

DECLARE(Add)
{
	GETDATA;

	DoMethod(obj, MUIM_CfgPageFT_AddFileType, "#?", "", "<< new entry >>", FTYPESSTATE_ON);
	set(_win(obj), MUIA_Window_ActiveObject, data->PatternStr);

	return 0;
}

DECLARE(Remove)
{
	GETDATA;

	DoMethod(data->Listview, MUIM_NList_Remove, MUIV_NList_Remove_Active);

	return 0;
}

DECLARE(GhostPanelControl)
{
	GETDATA;

	if (GetActiveFTE(data)) {

		set(data->ViewerStr,  MUIA_Disabled, FALSE);
		set(data->PatternStr, MUIA_Disabled, FALSE);
		set(data->CommentStr, MUIA_Disabled, FALSE);
		set(data->StateCycle, MUIA_Disabled, FALSE);

	} else {

		set(data->ViewerStr,  MUIA_String_Contents, "");
		set(data->PatternStr, MUIA_String_Contents, "");
		set(data->CommentStr, MUIA_String_Contents, "");
		set(data->ViewerStr,  MUIA_Disabled, TRUE);
		set(data->PatternStr, MUIA_Disabled, TRUE);
		set(data->CommentStr, MUIA_Disabled, TRUE);
		set(data->StateCycle, MUIA_Disabled, TRUE);
	}

	return 0;
}
