/*
 $Id: CLASS_XADInfoWin.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Custom class for the XAD information window.

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
 *  Class Name: XADInfoWin
 * Description: This class creates and handles the XAD client info window
 *  Superclass: MUIC_Window
 *
 */

#include "vx_include.h"
#include "XADInfoWin.h"
#include "CLASS_XADInfoWin_Defs.h"
#include "vx_mem.h"
#include "vx_global.h"
#include "vx_debug.h"
#include "vx_main.h"
#include "vx_misc.h"

typedef unsigned long uint32;

/* CLASSDATA
//------------------------------------------------------------------------------
	struct CI *ci_array;
	ULONG ci_array_cnt;
	struct AmigaGuideNodeDef *AGND;
	ULONG AGND_cnt;
	Object *Listview;
	Object *List;
	Object *ClientsAvail;
	Object *Ver;
	Object *Author;
	Object *MasterVer;
	Object *Flags;
	Object *Desc;
	Object *DescListview;
	Object *pc;
	Object *nc;
	Object *GuideVerObj;
	struct XADClientListEntry *xcle;
	UBYTE guide_ver[128];
//------------------------------------------------------------------------------
*/

/***************************************************************************/
/* XAD client info list hooks */
/***************************************************************************/

void GotoNode( UBYTE *node_name, struct Data *data ) {

	struct XADClientListEntry *xcle;
	LONG i;
	DB(dbg("Goto node: %s\n", node_name ? node_name : (UBYTE *) "<< NULL pointer! >>"))

	for (i = 0;; i++) {
		DoMethod(data->List, MUIM_NList_GetEntry, i, &xcle);

		if (!xcle) {
			break;
		}

		if (!xcle->xcle_CI) {
			continue;
		}

		if (Stricmp(node_name, xcle->xcle_CI->ci_node) == 0) {
			set
				(data->List, MUIA_NList_Active, i);
			return;
		}
	}

	GUI_PrgError("Couldn't find node '%s'!", &node_name);
}

UBYTE *FindEndOfText( UBYTE *in ) {

	while (*in) {
		switch(*in) {
		case ' ':
		case '\t':
		case '\n':
		case '}':
		case '"':
		case '|':
			return in;
			break;
		}
		in++;
	}

	return in;
}

UBYTE *ConvertHyperLink( UBYTE *str ) {

	/* Extracts the readable text out of an AmigaGuide hyperlink
	   definition. Also gets the node.

	   In other words, pass a string like
	   @{"readable text" link "node"} and get "readable text@node"
	   (without the quotes, of course). */

	UBYTE *start, ch;
	UBYTE buf[256];
	LONG len;

	while (*str && (*str == ' ' || *str == '\t')) {
		str++; /* Skip white spaces */
	}

	if (*str != '@') {
		return NULL;
	} else {
		str++;
	}

	while (*str && (*str == ' ' || *str == '\t')) {
		str++; /* Skip white spaces */
	}

	if (*str != '{') {
		return NULL;
	} else {
		str++;
	}

	while (*str && (*str == ' ' || *str == '\t')) {
		str++; /* Skip white spaces */
	}

	if (*str == '"') {
		start = ++str;

		while (*str && *str != '"') {
			str++; /* Get closing quote */
		}

		len = str - start;

		if (len > sizeof(buf)-1) {
			len = sizeof(buf)-1;
		}

		memcpy(buf, start, len);
		buf[len] = 0;
		str++;

	} else {
		start = str;
		str = FindEndOfText(str);
		len = str - start;

		if (len > sizeof(buf)-1) {
			len = sizeof(buf)-1;
		}

		memcpy(buf, start, len);
		buf[len] = 0;
	}

	strncat(buf, "@", sizeof(buf)-1);

	/* Get the node id... */

	while (*str && (*str == ' ' || *str == '\t')) {
		str++; /* Skip white spaces */
	}

	if (Strnicmp(str, "link", 4) != 0) {
		return NULL;
	}

	str += 4;

	while (*str && (*str == ' ' || *str == '\t')) {
		str++; /* Skip white spaces */
	}

	if (*str == '"') {
		start = ++str;

		while (*str && *str != '"') {
			str++;
		}

		ch = *str;
		*str = 0;
		strncat(buf, start, sizeof(buf)-1);
		*str = ch;

	} else {

		start = str;
		str = FindEndOfText(str);
		ch = *str;
		*str = 0;
		strncat(buf, start, sizeof(buf)-1);
		*str = ch;
	}

	return MEM_StrToVec(buf);
}

SAVEDS ASM(void) AuthorButtonTrigger(
    REG_A0 (struct Hook *hk),
    REG_A1 (LONG *param),
    REG_A2 (Object *obj) )
{
	UBYTE *emailURL = NULL;

	/*if (!data->ci_array || !obj) return; TODO: Fixthis!!! */

	if (!obj) {
		return;
	}

	get(obj, MUIA_UserData, &emailURL);

	if (!emailURL || strlen(emailURL) == 0) {
		return;
	}

	DispatchURL(emailURL);
}

SAVEDS ASM(void) ClickTrigger(
    REG_A0 (struct Hook *hk),
    REG_A1 (LONG *param),
    REG_A2 (Object *obj) )
{
	LONG clickid;
	struct XADClientListEntry *xcle;
	struct Node *n;
	UBYTE url[256], *p;
	struct Data *data = NULL;
	get(obj, MUIA_UserData, &data);

	if (!data) {
		return;
	}

	get(data->Desc, MUIA_NList_ButtonClick, &clickid);
	xcle = data->xcle;

	if (!xcle) {
		return;
	}

	for (n = xcle->xcle_ClickList.lh_Head; n->ln_Succ; n = n->ln_Succ) {
		if (!--clickid) {
			break;
		}
	}

	if (!n->ln_Succ) {
		return;
	}

	switch(n->ln_Type) {
		case CLICKTYPE_NODE:
			if ((p = strchr(n->ln_Name, '@'))) {
				GotoNode(++p, data);
			}
			return;

		case CLICKTYPE_HTTP:
		case CLICKTYPE_FTP:
		case CLICKTYPE_MAILTO:
			strncpy(url, n->ln_Name, sizeof(url)-1);
			break;

		case CLICKTYPE_AMINET:
			strcpy(url, AMINET_LOCATION);
			p = n->ln_Name + sizeof("Aminet:");

			while (*p == ' ' || *p == '\t') {
				p++;
			}

			strncat(url, p, sizeof(url)-1);
			break;

		case CLICKTYPE_AMINETRM:
			strcpy(url, AMINET_LOCATION);
			p = n->ln_Pred->ln_Name + sizeof("Aminet:");

			while (*p == ' ' || *p == '\t') {
				p++;
			}

			strncat(url, p, sizeof(url)-1);

			if ((p = strrchr(url, '.'))) {
				*p = 0;
			}

			strncat(url, ".readme", sizeof(url)-1);
			break;

		default:
			return;
	}

	DispatchURL(url);
}

void BuildClickObjects( struct List *click_list, struct Data *data ) {

	struct Node *n;
	LONG buttonindex = 0;
	Object *bt, *desobj = data->Desc;
	UBYTE *p;
	DoMethod(desobj, MUIM_NList_UseImage, NULL, -1, 0);

	if (!click_list) {
		return;
	}

	for (n = click_list->lh_Head; n->ln_Succ; n = n->ln_Succ) {
		p = NULL;

		if (n->ln_Type == CLICKTYPE_NODE) {
			if ((p = strchr(n->ln_Name, '@'))) {
				*p = 0;
			} else {
				continue;
			}
		}

		DoMethod(desobj, MUIM_NList_UseImage, bt = SimpleButtonTiny(n->ln_Name), buttonindex++, ~0L);
		set(bt, MUIA_UserData, data);

		if (p) {
			*p = '@';
		}
	}
}

void KillClickList( struct List *click_list ) {
	struct Node *n;

	if (!click_list) {
		return;
	}

	while ((n = RemHead(click_list))) {

		if (n->ln_Name) {
			MEM_FreeVec(n->ln_Name);
		}

		MEM_FreeVec(n);
	}
}

void AddClicker( struct List *click_list, UBYTE *str, LONG type ) {

	struct Node *n;

	if (!click_list) {
		return;
	}

	if ((n = MEM_AllocVec(sizeof(struct Node)))) {
		n->ln_Type = type;
		n->ln_Name = MEM_StrToVec(str);

		if (!n->ln_Name) {
			MEM_FreeVec(n);
			return;
		}

		AddTail(click_list, n);
	}
}

UBYTE *ProcessNodeText( UBYTE *nodetext, struct List *click_list ) {
	UBYTE *outvec, *tmpvec, *in, *out, *start, *end, ch;
	LONG buttonindex = 0;
	LONG buttonid    = 1;
	in = nodetext;
	out = outvec = MEM_AllocVec((strlen(nodetext) * 2) + 4);

	if (!outvec) {
		return NULL;
	}

	while (*in) {
		if (*in == '@') {
			if ((tmpvec = ConvertHyperLink(in))) {

				AddClicker(click_list, tmpvec, CLICKTYPE_NODE);
				sprintf(out, "\33o[%ld@%ld]", buttonindex++, buttonid++);
				out += strlen(out);
				MEM_FreeVec(tmpvec);

				if ((in = strchr(in, '}'))) {
					in++;
				} else {
					break;
				}

			} else {

				*out++ = *in++;
				continue;
			}
		} else if (Strnicmp("http://", in, 7) == 0) {

			start = in;
			end = FindEndOfText(start);
			ch = *end;
			*end = 0;
			AddClicker(click_list, start, CLICKTYPE_HTTP);
			sprintf(out, "\33o[%ld@%ld]", buttonindex++, buttonid++);
			out += strlen(out);
			*end = ch;
			in = end;

		} else if (Strnicmp("ftp://", in, 6) == 0) {

			start = in;
			end = FindEndOfText(start);
			ch = *end;
			*end = 0;
			AddClicker(click_list, start, CLICKTYPE_FTP);
			sprintf(out, "\33o[%ld@%ld]", buttonindex++, buttonid++);
			out += strlen(out);
			*end = ch;
			in = end;

		} else if (Strnicmp("mailto:", in, 7) == 0) {

			start = in;
			end = FindEndOfText(start);
			ch = *end;
			*end = 0;
			AddClicker(click_list, start, CLICKTYPE_MAILTO);
			sprintf(out, "\33o[%ld@%ld]", buttonindex++, buttonid++);
			out += strlen(out);
			*end = ch;
			in = end;

		} else if (Strnicmp("Aminet:", in, 7) == 0) {

			start = in;
			in += 7;

			if (*in == ' ') {
				in++;
			}

			end = FindEndOfText(in);
			ch = *end;
			*end = 0;
			AddClicker(click_list, start, CLICKTYPE_AMINET);
			sprintf(out, "\33o[%ld@%ld]", buttonindex++, buttonid++);
			out += strlen(out);
			sprintf(out, " \33o[%ld@%ld]", buttonindex++, buttonid++);
			AddClicker(click_list, "Readme", CLICKTYPE_AMINETRM);
			out += strlen(out);
			*end = ch;
			in = end;

		} else {

			*out++ = *in++;
		}
	}

	return outvec;
}

SAVEDS ASM(void) ClientList_KillFunc(
    REG_A2 (APTR Pool),
    REG_A1 (struct XADClientListEntry *XCLE) )
{
	if (!XCLE) {
		return;
	}

	if (XCLE->xcle_ArchiverName) {
		MEM_FreeVec(XCLE->xcle_ArchiverName);
	}

	if (XCLE->xcle_Description) {
		MEM_FreeVec(XCLE->xcle_Description);
	}

	if (XCLE->xcle_Description_ready) {
		KillClickList(&XCLE->xcle_ClickList);
	}

	MEM_FreeVec(XCLE);
}

SAVEDS ASM(struct XADClientListEntry *) ClientList_MakeFunc(
    REG_A2 (APTR Pool),
    REG_A1 (struct ClientParam *cp) )
{
	struct XADClientListEntry *XCLE;
	struct xadClient *xadC = cp->cp_xadc;
	struct CI *ci = cp->cp_ci;
	UBYTE *p;
	ULONG f;

	if (!xadC) {
		return NULL;
	}

	if ((XCLE = MEM_AllocVec(sizeof(struct XADClientListEntry)))) {
		XCLE->xcle_Version           = xadC->xc_Version;
		XCLE->xcle_MasterVersion     = xadC->xc_MasterVersion;
		XCLE->xcle_ClientVersion     = xadC->xc_ClientVersion;
		XCLE->xcle_ClientRevision    = xadC->xc_ClientRevision;
		XCLE->xcle_RecogSize         = xadC->xc_RecogSize;
		XCLE->xcle_Flags             = xadC->xc_Flags;
		XCLE->xcle_Identifier        = xadC->xc_Identifier;
		XCLE->xcle_ArchiverName      = MEM_StrToVec(xadC->xc_ArchiverName);
		XCLE->xcle_Description_ready = FALSE;
		XCLE->xcle_CI                = ci;

		if (ci) {

			strncpy(XCLE->xcle_AuthorName,  ci->ci_author, sizeof(XCLE->xcle_AuthorName)-1);
			strncpy(XCLE->xcle_AuthorEMail, ci->ci_email,  sizeof(XCLE->xcle_AuthorEMail)-1);
			XCLE->xcle_Description = MEM_StrToVec(ci->ci_description);

		} else {

			strcpy(XCLE->xcle_AuthorName,  "Unknown");
			strcpy(XCLE->xcle_AuthorEMail, "Unknown");
			XCLE->xcle_Description = MEM_StrToVec(NOCLIENTINFOAVAIL);
		}

		if (!XCLE->xcle_ArchiverName) {

			MEM_FreeVec(XCLE);
			XCLE = NULL;

		} else {

			sprintf(XCLE->xcle_ClientVerRevStrBuf,  "%lu.%lu",
			        (ULONG) xadC->xc_ClientVersion, (ULONG) xadC->xc_ClientRevision);
			sprintf(XCLE->xcle_MasterVersionStrBuf,  "%lu",
			        (ULONG) xadC->xc_MasterVersion);

			p = XCLE->xcle_FlagsStrBuf;
			f = XCLE->xcle_Flags;

			strcpy(p, "");

			if (XADCF_FILEARCHIVER & f)
				strncat(p, "File Archiver, ", sizeof(XCLE->xcle_FlagsStrBuf)-1);

			if (XADCF_DISKARCHIVER & f)
				strncat(p, "Disk Archiver, ", sizeof(XCLE->xcle_FlagsStrBuf)-1);

			if (XADCF_EXTERN       & f)
				strncat(p, "External, ",      sizeof(XCLE->xcle_FlagsStrBuf)-1);

			if (XADCF_FILESYSTEM   & f)
				strncat(p, "Filesystem, ",    sizeof(XCLE->xcle_FlagsStrBuf)-1);

			if (XADCF_NOCHECKSIZE  & f)
				strncat(p, "No size check, ", sizeof(XCLE->xcle_FlagsStrBuf)-1);

			p = &XCLE->xcle_FlagsStrBuf[strlen(XCLE->xcle_FlagsStrBuf) - 2];

			if (*p == ',') {
				*p = ' ';
			}
		}
	}

	return XCLE;
}

SAVEDS ASM(LONG) ClientList_ShowFunc(
    REG_A2 (UBYTE **ColumnArray),
    REG_A1 (struct XADClientListEntry *XCLE) )
{
	if (XCLE) {
		ColumnArray[0] = XCLE->xcle_ArchiverName;
		ColumnArray[1] = (UBYTE *)&XCLE->xcle_ClientVerRevStrBuf;
		ColumnArray[2] = "Unknown";

		if (XADCF_FILEARCHIVER & XCLE->xcle_Flags)
			ColumnArray[2] = "File";

		if (XADCF_DISKARCHIVER & XCLE->xcle_Flags)
			ColumnArray[2] = "Disk";

		if (XADCF_FILESYSTEM   & XCLE->xcle_Flags)
			ColumnArray[2] = "FS";
	} else {

		ColumnArray[0] = "\33bName";
		ColumnArray[1] = "\33bVersion";
		ColumnArray[2] = "\33bType";
	}

	return 0;
}

SAVEDS ASM(LONG) ClientList_SortFunc(
    REG_A1 (struct XADClientListEntry *XCLE1),
    REG_A2 (struct XADClientListEntry *XCLE2) )
{
	return Stricmp(XCLE1->xcle_ArchiverName, XCLE2->xcle_ArchiverName);
}

struct Hook ClientList_MakeHook = { {NULL, NULL}, (void *) ClientList_MakeFunc, NULL, NULL };
struct Hook ClientList_KillHook = { {NULL, NULL}, (void *) ClientList_KillFunc, NULL, NULL };
struct Hook ClientList_ShowHook = { {NULL, NULL}, (void *) ClientList_ShowFunc, NULL, NULL };
struct Hook ClientList_SortHook = { {NULL, NULL}, (void *) ClientList_SortFunc, NULL, NULL };

SAVEDS ASM(void) ClientList_SingleClick(
    REG_A0 (struct Hook *hk),
    REG_A1 (LONG *selected),
    REG_A2 (Object *obj) )
{
	struct XADClientListEntry *xcle;
	struct Data *data = NULL;
	UBYTE *text_vec;

	get(obj, MUIA_UserData, &data);

	if (!data) {
		return;
	}

	if (*selected == -1) {
		return;
	}

	DoMethod(data->Listview, MUIM_NList_GetEntry, *selected, &xcle);

	if (!xcle) {
		return;
	}

	set(data->Ver,       MUIA_Text_Contents, &xcle->xcle_ClientVerRevStrBuf);
	set(data->MasterVer, MUIA_Text_Contents, &xcle->xcle_MasterVersionStrBuf);
	set(data->Flags,     MUIA_Text_Contents, &xcle->xcle_FlagsStrBuf);

	if (data->ci_array) {

		UBYTE author[256];
		static UBYTE mailto_url[256];

		sprintf(mailto_url,
		        "mailto:%s?Subject=%s XAD client",
		        (UBYTE *) &xcle->xcle_AuthorEMail, (UBYTE *) xcle->xcle_ArchiverName);
		set(data->Author, MUIA_UserData, &mailto_url);
		sprintf(author, "%s <%s>",
		        (UBYTE *) &xcle->xcle_AuthorName, (UBYTE *) &xcle->xcle_AuthorEMail);
		set(data->Author, MUIA_Text_Contents, author);

		DoMethod(data->Desc, MUIM_NList_Clear);

		if (xcle->xcle_Description) {
			if (!xcle->xcle_Description_ready) {

				NewList(&xcle->xcle_ClickList);
				if ((text_vec = ProcessNodeText(xcle->xcle_Description, &xcle->xcle_ClickList))) {

					MEM_FreeVec(xcle->xcle_Description);
					xcle->xcle_Description       = text_vec;
					xcle->xcle_Description_ready = TRUE;
				}
			}

			data->xcle = xcle;
			BuildClickObjects(&xcle->xcle_ClickList, data);
			DoMethod(data->Desc, MUIM_List_Insert, xcle->xcle_Description, -2,
			         MUIV_NList_Insert_Bottom);
		}
	} else {

		DoMethod(data->Desc, MUIM_NList_Clear);
		DoMethod(data->Desc, MUIM_List_Insert, NOXADGUIDEFILE, -2, MUIV_NList_Insert_Bottom);
		set(data->Author, MUIA_Text_Contents, "Unavailable");
	}
}

UBYTE *GetNodeID( UBYTE *str ) {
	/* This modifies the buffer which is passed... */

	UBYTE *node_id;

	if (!str) {
		return NULL;
	}

	if (Strnicmp(str, "@node", 5) == 0) {
		str += 5;
	}

	while (*str && ((*str == ' ') || (*str == '\t'))) {
		str++;
	}

	if (!*str) {
		return NULL;
	}

	if (*str == '"') {
		/* Node ID is quoted... */

		node_id = ++str;

		while (*str && (*str != '"') && (*str != '\n') && (*str != '\t')) {
			str++;
		}

		*str = 0;
	} else {

		/* Node ID is not quoted... */

		node_id = str;

		while (*str && (*str != ' ') && (*str != '\n') && (*str != '\t')) {
			str++;
		}

		*str = 0;
	}

	return node_id;
}

void FreeAmigaGuideNodes( struct AmigaGuideNodeDef *agnd, ULONG cnt ) {

	ULONG i;

	if (!agnd) {
		return;
	}

	for (i = 0; i < cnt; i++) {

		if (agnd[i].agnd_nodeid) {
			MEM_FreeVec(agnd[i].agnd_nodeid);
		}

		if (agnd[i].agnd_nodetitle) {
			MEM_FreeVec(agnd[i].agnd_nodetitle);
		}

		if (agnd[i].agnd_nodetext) {
			MEM_FreeVec(agnd[i].agnd_nodetext);
		}
	}

	MEM_FreeVec(agnd);
}

struct AmigaGuideNodeDef *LoadAmigaGuideNodes( BPTR fh, ULONG *node_cnt, ULONG max_nodes ) {

	struct AmigaGuideNodeDef *agnd, *next_agnd;
	UBYTE buf[256], *bp;
	BOOL abort = FALSE;
	UBYTE *tmpvec, *p;
	LONG cnt, len, got;

	next_agnd = agnd = MEM_AllocVec((1 + max_nodes) * sizeof(struct AmigaGuideNodeDef));

	if (!agnd) {
		return NULL;
	}

	Seek(fh, 0, OFFSET_BEGINNING);

	for (cnt = 0; !abort;) {
		GetLine

		if (Strnicmp(bp, "@node", 5) != 0) {
			continue;
		}

		cnt++;

		if (cnt >= max_nodes) {
			break; /* The limit */
		}

		next_agnd->agnd_pof_end   = -1;
		next_agnd->agnd_nodeid    = MEM_StrToVec(GetNodeID(bp));
		next_agnd->agnd_nodetitle = NULL;
		next_agnd->agnd_pof_start = Seek(fh, 0, OFFSET_CURRENT);

		for (;;) { /* Find the end of the node text */

			GetLine
			if ((Strnicmp(bp, "@endnode", 8) == 0) ||
			        (Strnicmp(bp, "@node", 5) == 0)) {

				next_agnd->agnd_pof_end = Seek(fh, 0, OFFSET_CURRENT);

				if (bp[1] == 'e') {
					next_agnd->agnd_pof_end -= 9;
				} else {
					next_agnd->agnd_pof_end -= 6;
				}

				break;
			}
		}

		if (next_agnd->agnd_pof_start > 0 && next_agnd->agnd_pof_end > 0) {

			len = next_agnd->agnd_pof_end - next_agnd->agnd_pof_start;

			if (len && len > 0) {

				if ((tmpvec = MEM_AllocVec(len + 4))) {
					abort = UpdateProgress("Loading node: %s", &next_agnd->agnd_nodeid,
					                       next_agnd->agnd_pof_start);
					if (abort) {
						/* do something here */

					}
					Seek(fh, next_agnd->agnd_pof_start, OFFSET_BEGINNING);
					got = Read(fh, tmpvec, len);

					if (got != -1) {

						/* Remove MUI exploit from incomming text. */

						for (p = tmpvec; p < tmpvec + got; p++) {
							if (*p == 0x1B) {
								*p = ' ';
							}
						}
					}

					Seek(fh, next_agnd->agnd_pof_end, OFFSET_BEGINNING);
					next_agnd->agnd_nodetext = tmpvec;
				}
			}
		}

		next_agnd++;
	}

	if (node_cnt) {
		*node_cnt = cnt;
	}

	if (abort) {
		FreeAmigaGuideNodes(agnd, cnt);
		*node_cnt = 0;
		agnd = NULL;
	}

	return agnd;
}

void UnloadClientsGuide( struct CI *ci, ULONG cnt ) {
	ULONG i;

	if (!ci) {
		return;
	}

	for (i = 0; i < MAX_XAD_CLIENT_COUNT; i++) {

		if (ci[i].ci_name)
			MEM_FreeVec(ci[i].ci_name);

		if (ci[i].ci_node)
			MEM_FreeVec(ci[i].ci_node);

		if (ci[i].ci_author)
			MEM_FreeVec(ci[i].ci_author);

		if (ci[i].ci_email)
			MEM_FreeVec(ci[i].ci_email);

		if (ci[i].ci_description)
			MEM_FreeVec(ci[i].ci_description);
	}

	MEM_FreeVec(ci);
}

struct CI *LoadClientsGuide( UBYTE *xadguide_filename, ULONG *cnt_ptr, struct Data *data ) {
	struct CI *ci, *nextci;
	BOOL success = FALSE, gotver = FALSE;
	ULONG ci_cnt = 0, i, x;
	BPTR fh;
	UBYTE *p, path[256], *pathvec;

	/* VX will search all these dirs for xadclients.guide. First
	   one found will be the one that's used. */

	UBYTE *paths[] =  { "PROGDIR:", "PROGDIR:Docs", "PROGDIR:Guides", "PROGDIR:Help",
	                    "HELP:VX", "HELP:XAD", "HELP:English", "HELP:", NULL };

	if (!(pathvec = FindFile(paths, "xadclients.guide"))) {
		pathvec = MEM_StrToVec("PROGDIR:xadclients.guide");
	}

	if (pathvec) {
		strncpy(path, pathvec, sizeof(path)-1);
		MEM_FreeVec(pathvec);

	} else {
		return NULL;
	}

	nextci = ci = MEM_AllocVec(sizeof(struct CI) * (MAX_XAD_CLIENT_COUNT + 1));

	if (!ci) {
		return NULL;
	}

	if ((fh = Open(path, MODE_OLDFILE))) {
		UBYTE buf[256], *bp;
		UBYTE *rmk = "@remark";
		LONG rmk_len = strlen(rmk), len;

		if (DOSBase->dl_lib.lib_Version >= 40) {
			SetVBuf(fh, NULL, BUF_FULL, 1024 * 64);
		}

		Seek(fh, 0, OFFSET_END);
		len = Seek(fh, 0, OFFSET_BEGINNING);
		OpenProgress(len, "Please wait, processing xadclients.guide...");
		ChangeProgressStatus("Processing header...", NULL);

		for (;;) {
			GetLine
			if (!gotver) {
				if (!(p = strstr(bp, "$VER:"))) {
					if (!(p = strstr(bp, "$ver:"))) {
						p = strstr(bp, "$Ver:");
					}
				}

				if (p) {
					p += 5;
					while (*p == ' ' || *p == '\t') {
						p++;
					}

					strncpy(data->guide_ver, p, sizeof(data->guide_ver)-1);

					if ((p = strchr(data->guide_ver, '\n'))) {
						*p = 0;
					}

					gotver = TRUE;
				}
			}

			if (ci_cnt >= MAX_XAD_CLIENT_COUNT) {
				//success = TRUE; /* All slots filled... */
				break;
			}

			if (Strnicmp(bp, rmk, rmk_len) == 0) {
				bp += rmk_len;

				while (*bp == ' ' || *bp == '\t') {
					bp++;
				}

				if (Strnicmp(bp, "CISTART", 7) == 0) {

					/* Found the start */

				} else if (Strnicmp(bp, "CIEND", 5) == 0) {

					break;

				} else if (strchr(bp, '|')) {

					UBYTE *ci, *cname = NULL,
						*cnode = NULL, *aname = NULL, *email = NULL;

					// TODO: Use macros here
					ci = bp;

					while (*bp && *bp != '|' && *bp != '\n') {
						++bp;
					}

					if (*bp) {
						*bp++ = 0;
					} else {
						goto quick_and_dirty_exit;
					}

					cname = bp;

					while (*bp && *bp != '|' && *bp != '\n') {
						++bp;
					}

					if (*bp) {
						*bp++ = 0;
					} else {
						goto quick_and_dirty_exit;
					}

					cnode = bp;

					while (*bp && *bp != '|' && *bp != '\n') {
						++bp;
					}

					if (*bp) {
						*bp++ = 0;
					} else {
						goto quick_and_dirty_exit;
					}

					aname = bp;

					while (*bp && *bp != '|' && *bp != '\n') {
						++bp;
					}

					if (*bp) {
						*bp++ = 0;
					} else {
						goto quick_and_dirty_exit;
					}

					email = bp;

					while (*bp && *bp != '|' && *bp != '\n') {
						++bp;
					}

					if (*bp) {
						*bp = 0;
					}

quick_and_dirty_exit:

					if (ci && (Stricmp(ci, "CI") == 0)) {
						if (!cname) cname = "";
						if (!cnode) cnode = "";
						if (!aname) aname = "";
						if (!email) email = "";

						nextci->ci_name   = MEM_StrToVec(cname);
						nextci->ci_node   = MEM_StrToVec(cnode);
						nextci->ci_author = MEM_StrToVec(aname);
						nextci->ci_email  = MEM_StrToVec(email);

						if (nextci->ci_name && nextci->ci_node && nextci->ci_author && nextci->ci_email) {
							ci_cnt++;
							nextci++;
						}
					}
				}
			}
		}	/* for (;;) */

		/* Now lets rip the description text from the guide file... */

		if ((data->AGND = LoadAmigaGuideNodes(fh, &data->AGND_cnt, MAX_XAD_CLIENT_COUNT))) {

			UpdateProgress("Processing nodes...", NULL, len);

			/* Iterate through the client info array we got from the
			   start of xadclients.guide. Try to match up the node IDs.*/

			for (i = 0; i <= ci_cnt; i++) {

				BOOL node_found = FALSE;

				if (!ci[i].ci_node) {
					continue;
				}

				/* Do we have an AmigaGuide node for this client node? */

				for (x = 0; x < data->AGND_cnt; x++) {

					if (!data->AGND[x].agnd_nodeid) {
						continue;
					}

					if (Stricmp(data->AGND[x].agnd_nodeid, ci[i].ci_node) == 0) {
						ci[i].ci_description = MEM_StrToVec(data->AGND[x].agnd_nodetext);
						node_found = TRUE;
						break;
					}
				}

				if (!node_found) {
					sprintf(buf, "Error: Node '%s' doesn't exist in xadclients.guide!", ci[i].ci_node);
					ci[i].ci_description = MEM_StrToVec(buf);
				}

			}	/* for (;;) */
		}

		success = TRUE;
		Close(fh);
		CloseProgress;
	}

	if (success) {

		if (cnt_ptr) {
			*cnt_ptr = ci_cnt;
		}

	} else {

		UnloadClientsGuide(ci, ci_cnt);

		if (cnt_ptr) {
			*cnt_ptr = 0;
		}
		ci = NULL;
	}

	return ci;
}

struct CI *FindCI( struct CI *ci, ULONG cnt, UBYTE *name ) {
	ULONG i;

	if (!ci || cnt == 0 || !name) {
		return NULL;
	}

	for (i = 0; i < MAX_XAD_CLIENT_COUNT; i++) {
		if (ci[i].ci_name && Stricmp(ci[i].ci_name, name) == 0) {
			return &ci[i];
		}
	}

	return NULL;
}

OVERLOAD(OM_NEW)
{
	HOOK(ClickTrigger_Hk, ClickTrigger)
	DEFTMPDATA;
	UBYTE verbuf[256], *p = verbuf;
	LONG id;
	CLRTMPDATA;

	strncpy(verbuf, xadMasterBase->xmb_LibNode.lib_IdString, sizeof(verbuf)-1);

	while (*p) {
		if (*p == '\n' || *p == '\r') {
			*p = ' ';
		}
		++p;
	}

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_Title,     "XAD client info window...",
		MUIA_Window_ID,        MAKE_ID('X','A','D','C'),
		WindowContents, VGroup,
			Child, TextObject,
				TextFrame,
				MUIA_Text_PreParse, "\33c",
				MUIA_Text_Contents, verbuf,
				MUIA_Background,    MUII_TextBack,
			End,
			Child, data.GuideVerObj = TextObject,
				TextFrame,
				MUIA_Text_PreParse, "\33c",
				MUIA_Background,    MUII_TextBack,
			End,
			Child, HGroup,
				Child, VGroup,
					MUIA_HorizWeight,   25,
					Child, data.Listview = NListviewObject,
						MUIA_CycleChain,		1,
						MUIA_ShortHelp,			HELP_CI_LISTVIEW,
						MUIA_HorizWeight,		25,
						MUIA_NListview_NList,	data.List = NListObject,
							MUIA_ObjectID,			MAKE_ID('X','C','I','L'),
							MUIA_NList_Input,		TRUE,
							MUIA_NList_MultiSelect,	MUIV_NList_MultiSelect_None,
							MUIA_NList_Format,		LIST_FORMAT_XADCLIENTINFO,
							MUIA_NList_Title,		TRUE,
							MUIA_NList_AutoVisible,		TRUE,
							MUIA_NList_ConstructHook,	&ClientList_MakeHook,
							MUIA_NList_DestructHook,	&ClientList_KillHook,
							MUIA_NList_DisplayHook,		&ClientList_ShowHook,
							MUIA_NList_CompareHook,		&ClientList_SortHook,
							/* TODO: Add click sortable columns here!
							MUIA_NList_ConstructHook2,	&ARC_XADClientInfo_ListMakeHook,
							MUIA_NList_DestructHook2,	&ARC_XADClientInfo_ListKillHook,
							MUIA_NList_DisplayHook2,	&ARC_XADClientInfo_ListShowHook,
							MUIA_NList_CompareHook2,	&ARC_XADClientInfo_ListSortHook,*/
						End,
					End,
					Child, HGroup,
						Child, Label("Clients available:"),
						Child, data.ClientsAvail = TextObject,
							MUIA_ShortHelp,     "",
							MUIA_Text_PreParse, "\33c",
							TextFrame,
							MUIA_Background,    MUII_TextBack,
						End,
					End,
				End,
				Child, BalanceObject, End,
				Child, VGroup,
					MUIA_HorizWeight, 75,
					Child, ColGroup(2),
						Child, Label("Client version"),
						Child, data.Ver = TextObject,
							MUIA_ShortHelp,     "",
							MUIA_Text_PreParse, "\33c",
							TextFrame,
							MUIA_Background,    MUII_TextBack,
						End,
						Child, Label("Client author"),
						Child, data.Author = MyButton("", "Author's e-mail address"),
						Child, Label("Min XAD ver needed"),
						Child, data.MasterVer = TextObject,
							MUIA_ShortHelp,     "",
							MUIA_Text_PreParse, "\33c",
							TextFrame,
							MUIA_Background,    MUII_TextBack,
						End,
						Child, Label("Client properties"),
						Child, data.Flags = TextObject,
							MUIA_ShortHelp,     "",
							MUIA_Text_PreParse, "\33c",
							TextFrame,
							MUIA_Background,    MUII_TextBack,
						End,
					End,
					Child, data.DescListview = NListviewObject,
						MUIA_CycleChain, 1,
						MUIA_NListview_NList, data.Desc = NListObject,
							MUIA_NList_Input,                FALSE,
							MUIA_NList_DefaultObjectOnClick, TRUE,
							MUIA_NList_ConstructHook,        MUIV_NList_ConstructHook_String,
							MUIA_NList_DestructHook,         MUIV_NList_DestructHook_String,
						End,
						MUIA_ShortHelp, "Description of selected XAD client",
					End,
					Child, HGroup,
						// TODO: add these to history...
						Child, data.pc = MyButton("Prev client", ""),
						Child, data.nc = MyButton("Next client", ""),
						Child, RectangleObject, End,
						Child, RectangleObject, End,
						Child, RectangleObject, End,
						Child, RectangleObject, End,
					End,
				End,
			End,
		End,
		TAG_MORE, (ULONG)inittags(msg));

	if (!obj) {
		return 0;

	}

	DoMethod(data.pc, MUIM_Notify, MUIA_Pressed, FALSE, data.List,
	         3, MUIM_Set, MUIA_NList_Active, MUIV_NList_Active_Up);
	DoMethod(data.nc, MUIM_Notify, MUIA_Pressed, FALSE, data.List,
	         3, MUIM_Set, MUIA_NList_Active, MUIV_NList_Active_Down);

	set(data.List,         MUIA_UserData, INST_DATA(cl, obj));
	set(data.Listview,     MUIA_UserData, INST_DATA(cl, obj));
	set(data.Desc,         MUIA_UserData, INST_DATA(cl, obj));
	set(data.DescListview, MUIA_UserData, INST_DATA(cl, obj));

	DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIV_Notify_Self,
	         3, MUIM_Set, MUIA_Window_Open, FALSE);

	SET_TRIGGER_NLSC(data.Listview, ClientList_SingleClick_Hk, ClientList_SingleClick);
	SET_TRIGGER_BT  (data.Author,   AuthorButtonTrigger_Hk,    AuthorButtonTrigger);

	for (id = 1; id < 100; id++) {
		DoMethod(data.Desc, MUIM_Notify, MUIA_NList_ButtonClick,
		         id, data.Desc, 2, MUIM_CallHook, &ClickTrigger_Hk);
	}

	PREPDATA;

	return (ULONG) obj;
}

OVERLOAD(OM_DISPOSE)
{
	GETDATA;

	set(obj, MUIA_Window_Open, FALSE);

	if (data->Desc) {
		DoMethod(data->Desc, MUIM_NList_UseImage, NULL, -1, 0);
	}

	if (data->ci_array) {
		UnloadClientsGuide(data->ci_array, data->ci_array_cnt);
	}

	if (data->AGND) {
		FreeAmigaGuideNodes(data->AGND, data->AGND_cnt);
	}

	return DoSuperMethodA(cl, obj, msg);
}

DECLARE(Open)
{
	struct xadClient *xadc;
	ULONG cnt = 0;
	UBYTE buf[32], *gv;
	struct ClientParam cp;
	GETDATA;

	xadc = xadGetClientInfo();

	if (!xadc) {
		return 0;
	}

	if (!data->ci_array) {
		data->ci_array = LoadClientsGuide(NULL, &data->ci_array_cnt, data);
	}

	DoMethod(data->List, MUIM_NList_Clear);

	if (data->ci_array) {
		gv = strlen(data->guide_ver) == 0 ? (UBYTE *) "Unknown" : data->guide_ver;
	} else {
		gv = "Not available";
	}

	set(data->List, MUIA_NList_Quiet, TRUE);

	do {
		cp.cp_ci   = FindCI(data->ci_array, data->ci_array_cnt, xadc->xc_ArchiverName);
		cp.cp_xadc = xadc;
		DoMethod(data->List, MUIM_NList_InsertSingle, &cp, MUIV_NList_Insert_Sorted);
		cnt++;

	} while ((xadc = xadc->xc_Next));

	sprintf(buf, "%lu", cnt);

	set(data->ClientsAvail, MUIA_Text_Contents, &buf);
	set(data->List,         MUIA_NList_Quiet,   FALSE);
	set(data->List,         MUIA_NList_Active,  0);
	set(data->GuideVerObj,  MUIA_Text_Contents, gv);
	set(obj,                MUIA_Window_Open,   TRUE);

	return 0;
}

DECLARE(LoadXADClientsGuide) // UBYTE *filename
{
	/* Unimplemented */

	return 0;
}

