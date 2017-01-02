/*
 $Id: CLASS_ArcInfoWin.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class for archive information window.

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
 *  Class Name: ArcInfoWin
 * Description: Handles the archive information window
 *  Superclass: MUIC_Window
 *
 */

#include "vx_include.h"
#include "ArcInfoWin.h"
#include "vx_arc.h"
#include "vx_mem.h"
#include "vx_misc.h"
#include "vx_debug.h"

/* CLASSDATA
//------------------------------------------------------------------------------
	Object *Listview;
	Object *List;
	Object *TextListview;
	Object *TextList;
	Object *Comment;
	struct ArcHistoryNode *last_ahn;
//------------------------------------------------------------------------------
*/

struct ArcInfoEntry {
	UBYTE *name, *info;
};

SAVEDS ASM(void) WinArcInfo_ListKillFunc(
    REG_A1 (struct NList_DestructMessage *NL_DMsg),
    REG_A2 (Object *Obj) ) {
	struct ArcInfoEntry *aie = (struct ArcInfoEntry *) NL_DMsg->entry;

	if (aie->name) {
		MEM_FreeVec(aie->name);
	}

	if (aie->info) {
		MEM_FreeVec(aie->info);
	}

	MEM_FreeVec(aie);
}

SAVEDS ASM(struct ArcInfoEntry *) WinArcInfo_ListMakeFunc(
    REG_A1 (struct NList_ConstructMessage *NL_CMsg),
    REG_A2 (Object *Obj) ) {
	struct ArcInfoEntry *aie;
	UBYTE **param = (UBYTE **) NL_CMsg->entry;

	if ((aie = MEM_AllocVec(sizeof(struct ArcInfoEntry)))) {
		aie->name = MEM_StrToVec(param[0]);
		aie->info = MEM_StrToVec(param[1]);

		/* Note: "name" and "info" are allowed to be NULL */
	}

	return aie;
}

SAVEDS ASM(LONG) WinArcInfo_ListShowFunc(
    REG_A1 (struct NList_DisplayMessage *NL_DMsg),
    REG_A2 (Object *Obj) ) {
	register struct ArcInfoEntry *aie = NL_DMsg->entry;
	register UBYTE **col = (UBYTE **) NL_DMsg->strings;
	register UBYTE **pp  = (UBYTE **) NL_DMsg->preparses;

	if (aie) {
		/* [0] */ *col++ = aie->name ? aie->name : (UBYTE *) "????";
		/* [1] */
		*col   = aie->info ? aie->info : (UBYTE *) "????";
		/* [0] */
		*pp    = "\33b";
	} else /* Render titles */
	{
		/* [0] */ *col++  = "Name";
		/* [1] */
		*col    = "Info";
		/* [0] */
		*pp++   = "\33b";
		/* [1] */
		*pp     = "\33b";
	}
	return 0;
}

struct Hook WinArcInfo_ListMakeHook = { { NULL, NULL }, (void *) WinArcInfo_ListMakeFunc, NULL, NULL };
struct Hook WinArcInfo_ListKillHook = { { NULL, NULL }, (void *) WinArcInfo_ListKillFunc, NULL, NULL };
struct Hook WinArcInfo_ListShowHook = { { NULL, NULL }, (void *) WinArcInfo_ListShowFunc, NULL, NULL };

OVERLOAD(OM_NEW)
{
	AHN *ahn = NULL;
	struct TagItem *tags = inittags(msg), *tag;
	DEFTMPDATA;
	CLRTMPDATA;
	WHERE;

	while ((tag = NextTagItem(&tags))) {
		switch(tag->ti_Tag) {
			ATTR(AHN): ahn = (struct ArcHistoryNode *) tag->ti_Data;
			break;
		}
	}

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_Title,     "Archive information",
		MUIA_Window_ID,        MAKE_ID('A','R','C','I'),
		WindowContents, VGroup,
			Child, data.Listview = NListviewObject,
				MUIA_VertWeight,      200,
				MUIA_ShortHelp,       "Archive information",
				MUIA_CycleChain,      1,
				MUIA_NListview_NList, data.List = NListObject,
					ReadListFrame,
					MUIA_NList_Input,         FALSE,
					MUIA_NList_Active,        MUIV_List_Active_Top,
					MUIA_NList_Format,        "BAR,",
					MUIA_NList_Title,         FALSE,
					MUIA_NList_ConstructHook2,	&WinArcInfo_ListMakeHook,
					MUIA_NList_DestructHook2,	&WinArcInfo_ListKillHook,
					MUIA_NList_DisplayHook2,	&WinArcInfo_ListShowHook,
				End,
			End,
			Child, data.TextListview = NListviewObject,
				MUIA_VertWeight,      200,
				MUIA_ShortHelp,       "Archive text",
				MUIA_CycleChain,      1,
				MUIA_NListview_NList, data.TextList = NListObject,
					ReadListFrame,
					MUIA_NList_Input,         FALSE,
					MUIA_NList_Active,        MUIV_List_Active_Top,
					MUIA_NList_Format,        "",
					MUIA_NList_Title,         FALSE,
					MUIA_NList_ConstructHook, MUIV_NList_ConstructHook_String,
					MUIA_NList_DestructHook,  MUIV_NList_DestructHook_String,
				End,
			End,
			Child, HGroup,
				Child, Label("Comment"),
				Child, data.Comment = StringObject,
					StringFrame,
					MUIA_CycleChain,      1,
					MUIA_ControlChar,     'c',
					MUIA_String_MaxLen,   82,
				End,
			End,
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;
	}

	CLOSEWIN_METHOD(obj, MUIM_ArcInfoWin_CloseWindow);

	if (ahn) {
		DoMethod(obj, MUIM_ArcInfoWin_Update, ahn);
	}

	STRING_METHOD(data.Comment, MUIM_ArcInfoWin_NewComment);
	PREPDATA;

	return (ULONG) obj;
}

DECLARE(CloseWindow)
{
	set(obj, MUIA_Window_Open, FALSE);

	return 0;
}

DECLARE(Update) // struct ArcHistoryNode *ahn
{
	GETDATA;

	Object *l = data->List;
	Object *tl = data->TextList;
	struct ArcHistoryNode *ahn = msg->ahn;
	UBYTE buf[256], *vec, *p[2];
	BOOL hastext = FALSE;
	struct PN *pn;
	LONG i;

	DB(dbg("[CLASS ArcInfoWin] [METHOD Update(ahn=%lx)]", ahn))

	if (!ahn) {

		DoMethod(l, MUIM_NList_Clear);

		p[0] = "No archive present!";
		p[1] = "";
		DoMethod(l, MUIM_NList_InsertSingle, &p, MUIV_NList_Insert_Bottom);

		return 0;
	}

	/* Don't bother updating window if we're already showing this AHN. */

	if (data->last_ahn == ahn) {
		return 0;
	}

	data->last_ahn = ahn;

	DoMethod(l, MUIM_NList_Clear);

	pn = ahn->ahn_CurrentPN;

	if (!pn) {

		p[0] = "No portion(s) present!";
		p[1] = "";
		DoMethod(l, MUIM_NList_InsertSingle, &p, MUIV_NList_Insert_Bottom);

		return 0;
	}

	set(l, MUIA_NList_Quiet, TRUE);

	p[0] = "Path";
	p[1] = ahn->ahn_Path;
	DoMethod(l, MUIM_NList_InsertSingle, &p, MUIV_NList_Insert_Bottom);

	p[0] = "Type";
	p[1] = pn->pn_ai->xai_Client->xc_ArchiverName;
	DoMethod(l, MUIM_NList_InsertSingle, &p, MUIV_NList_Insert_Bottom);

	vec = FormatULONGToVec(ahn->ahn_FIB->fib_Size);
	sprintf(buf, "%s (%lu bytes)",
	        (char *)(vec ? vec : (UBYTE *)"????"), (ULONG) ahn->ahn_FIB->fib_Size);

	p[0] = "Size";
	p[1] = buf;
	DoMethod(l, MUIM_NList_InsertSingle, &p, MUIV_NList_Insert_Bottom);
	MEM_FreeVec(vec);

	vec = FormatULONGToVec(pn->pn_expsize);
	sprintf(buf, "%s (%lu bytes)", vec ? vec : (UBYTE *)"????", pn->pn_expsize);

	p[0] = "Expanded size";
	p[1] = buf;
	DoMethod(l, MUIM_NList_InsertSingle, &p, MUIV_NList_Insert_Bottom);
	MEM_FreeVec(vec);

	sprintf(buf, "%ld", ahn->ahn_PNListCnt);

	p[0] = "Number of portions";
	p[1] = buf;
	DoMethod(l, MUIM_NList_InsertSingle, &p, MUIV_NList_Insert_Bottom);

	sprintf(buf, "%lu", pn->pn_filecnt);

	p[0] = "File count";
	p[1] = buf;
	DoMethod(l, MUIM_NList_InsertSingle, &p, MUIV_NList_Insert_Bottom);

	p[0] = "Comment";
	p[1] = ahn->ahn_FIB->fib_Comment;
	DoMethod(l, MUIM_NList_InsertSingle, &p, MUIV_NList_Insert_Bottom);

	set(data->Comment, MUIA_String_Contents, ahn->ahn_FIB->fib_Comment);

	p[0] = "Date";
	p[1] = ahn->ahn_DateBuf;
	DoMethod(l, MUIM_NList_InsertSingle, &p, MUIV_NList_Insert_Bottom);


	vec = FormatULONGToVec(ahn->ahn_MemUsage);
	sprintf(buf, "%s (%lu bytes, %lu bytes per entry)",
	        (char *) (vec ? vec : (UBYTE *) "????"), (ULONG) ahn->ahn_MemUsage, AE_SIZE);

	p[0] = "Memory Usage";
	p[1] = buf;
	DoMethod(l, MUIM_NList_InsertSingle, &p, MUIV_NList_Insert_Bottom);

	MEM_FreeVec(vec);


	set(l, MUIA_NList_Quiet, FALSE);

	/**** Show the archive information texts ****/

	DoMethod(tl, MUIM_NList_Clear);
	set(tl, MUIA_NList_Quiet, TRUE);

	for (i = 0; i < MAX_INFOTEXTS; i++) {
		UBYTE *p, *line;

		if ((p = ahn->ahn_InfoTexts[i])) {
			hastext = TRUE;
		} else {
			continue;
		}

		sprintf(buf, "\33b»»»» Text %ld ««««", i + 1);
		DoMethod(tl, MUIM_NList_InsertSingle, buf, MUIV_NList_Insert_Bottom);

		for (;;) {

			if ((p = strchr(line = p, '\n'))) {
				*p = 0;

				DoMethod(tl, MUIM_NList_InsertSingle,
				         line, MUIV_NList_Insert_Bottom);

				*p++ = '\n';

			} else {

				DoMethod(tl, MUIM_NList_InsertSingle,
				         line, MUIV_NList_Insert_Bottom);

				break;
			}
		}
	}

	if (!hastext) {
		DoMethod(tl, MUIM_NList_InsertSingle,
		         "Archive contains no additional text", MUIV_NList_Insert_Bottom);
	}

	set(tl, MUIA_NList_Quiet, FALSE);

	return 0;
}

DECLARE(NewComment)
{
	GETDATA;
	AHN *tmpahn = data->last_ahn;
	UBYTE *comment = (UBYTE *) xget(data->Comment, MUIA_String_Contents);

	DB(dbg("ArcInfoWin: Change %s's file comment to %s", data->last_ahn->ahn_Path, comment))

	if (!ARC_ChangeComment(data->last_ahn, comment)) {
		return 0;
	}

	DB(dbg("ArcInfoWin: Change file comment was a success"))

	data->last_ahn = NULL;
	DoMethod(obj, MUIM_ArcInfoWin_Update, tmpahn);

	return 0;
}

