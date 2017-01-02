/*
 $Id: vx_cfg.c,v 1.2 2004/01/21 17:49:55 andrewbell Exp $
 Config file management.

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

#include "vx_include.h"
#include "vx_mem.h"
#include "vx_cfg.h"
#include "vx_misc.h"
#include "vx_debug.h"
#include "vx_main.h"

/* Config data */

struct SymMap SymMap_ARCVIEWMODE[] = {
	/* These symbols map into a cycle gadget. */
	{ "LINEAR"        , MUIV_ArcView_SwapViewMode_Linear       },
	{ "FILES_AND_DIRS", MUIV_ArcView_SwapViewMode_FilesAndDirs },
	{ "TREE"          , MUIV_ArcView_SwapViewMode_Listtree     },
	{ "NLISTTREE"     , MUIV_ArcView_SwapViewMode_NListtree    },
	{ "EXPLORER"      , MUIV_ArcView_SwapViewMode_Explorer     },
	{ NULL            , 0                                      }
};

struct SymMap SymMap_TB_APPEARANCE[] = {
	/* These symbols map into a cycle gadget. */
	{ "ICONS",      MUIV_VXToolBar_Appearance_Icons        },
	{ "TEXT",       MUIV_VXToolBar_Appearance_Text         },
	{ "ICONS&TEXT", MUIV_VXToolBar_Appearance_IconsAndText },
	{ NULL        , 0                                      }
};

/* TODO: Implement default information */

struct ConfigTag CTArray[] = {
	{ "AUTOSELECTALL",     0,           TAGTYPE_BOOL,   NULL },
	{ "AUTOVIRUSCHECK",    0,           TAGTYPE_BOOL,   NULL                 },
	{ "DEFARCHIVEPATH",    (ULONG)NULL, TAGTYPE_STRING, NULL                 },
	{ "DEFDESTPATH",       (ULONG)NULL, TAGTYPE_STRING, NULL                 },
	{ "ARCVIEWMODE",       0,           TAGTYPE_SYMBOL, SymMap_ARCVIEWMODE   },
	{ "KEEPFULLPATH",      0,           TAGTYPE_BOOL,   NULL                 },
	{ "TEMPPATH",          (ULONG)NULL, TAGTYPE_STRING, NULL                 },
	{ "SHOWEXTRACTREQ",    0,           TAGTYPE_BOOL,   NULL                 },
	{ "CHECKXCMD",         (ULONG)NULL, TAGTYPE_STRING, NULL                 },
	{ "NOEXTXADCLIENTS",   0,           TAGTYPE_BOOL,   NULL                 },
	{ "TB_APPEARANCE",     0,           TAGTYPE_SYMBOL, SymMap_TB_APPEARANCE },
	{ "TB_SUNNY",          0,           TAGTYPE_BOOL,   NULL                 },
	{ "TB_RAISED",         0,           TAGTYPE_BOOL,   NULL                 },
	{ "TB_BORDERLESS",     0,           TAGTYPE_BOOL,   NULL                 },
	{ "TB_SMALL",          0,           TAGTYPE_BOOL,   NULL                 },
	{ "DOEXECHECK",        0,           TAGTYPE_BOOL,   NULL                 },
	{ "AUTOENTERTARS",     0,           TAGTYPE_BOOL,   NULL                 },
	{ "DELLINGERINGTEMPS", 0,           TAGTYPE_BOOL,   NULL                 },
	{ "VIEWCOLUMNS",       0,           TAGTYPE_VAL,    NULL                 },
	{ "LEAVEPROGRESSOPEN", 0,           TAGTYPE_BOOL,   NULL                 },
	{ NULL,                0,           0,              NULL                 },
};

UBYTE *GetConfigTagStr( LONG TagID ) {

	if (TagID >= TAGID_MAX) {

		return "";

	} else {

		return CTArray[TagID].ct_TagStr;
	}

}

void SaveConfigTag( BPTR OutFH, LONG TagID ) {

	UBYTE *TagDataStr = NULL, buf[16];
	ULONG data = CTArray[TagID].ct_TagData;

	if (TagID == -1) {
		FPrintf(OutFH, "\n");
		return;
	}

	switch (CTArray[TagID].ct_TagType) {
	case TAGTYPE_VAL:
		sprintf(TagDataStr = buf, "0x%08lx", data);
		break;

	case TAGTYPE_BOOL:
		TagDataStr = (data ? YES : NO);
		break;

	case TAGTYPE_STRING:
		TagDataStr = (UBYTE *) data;
		break;

	case TAGTYPE_SYMBOL: {
			struct SymMap *SM = CTArray[TagID].ct_SymMap;
			TagDataStr = (UBYTE *) "Unknown";

			while(SM->sm_Symbol) {
				if (data == SM->sm_Value) {
					TagDataStr = (UBYTE *) SM->sm_Symbol;
					break;
				}
				SM++;
			}
			break;
		}
	}

	if (TagDataStr) {
		FPrintf(OutFH, "%s=%s\n", GetConfigTagStr(TagID), TagDataStr);
	}
}

LONG MatchConfigTagStr( UBYTE *MatchStr ) {
	/* Note: MatchStr doesn't have to be NULL-terminated. */

	LONG Index = 0;

	while (CTArray[Index].ct_TagStr) {
		if (Strnicmp(CTArray[Index].ct_TagStr,
		             MatchStr, strlen(CTArray[Index].ct_TagStr)) == 0) {

			return Index;
		}
		++Index;
	}

	return TAGID_UNKNOWN;
}

UBYTE *ReadConfigTag( BPTR InFH, LONG *TagIDPtr ) {
	/* This routine will return a string which must be freed with MEM_FreeVec(). */

	UBYTE LineBuf[256], *StrVec;
	UBYTE *TagDataStr = NULL;

	*TagIDPtr = TAGID_UNKNOWN;

	if (FGets(InFH, LineBuf, sizeof(LineBuf)-1) == NULL) {
		return (UBYTE *) -1;
	}

	if ((StrVec = TrimStringToVec(LineBuf))) {
		UBYTE *LP = StrVec;
		UBYTE *StrUpper = StrVec + strlen(StrVec);

		while (*LP == ' ') {
			++LP;
		}

		if (*LP && *LP != '#') {
			if ((*TagIDPtr = MatchConfigTagStr(LP)) != TAGID_UNKNOWN) {

				LP += strlen(CTArray[*TagIDPtr].ct_TagStr);

				if (LP < StrUpper) {

					while (*LP == ' ') {
						++LP;
					}

					if (*LP && *LP == '=') {
						++LP;
						TagDataStr = MEM_StrToVec(LP);
					}
				}
			}
		}

		MEM_FreeVec(StrVec);

	} else {

		return FALSE;
	}

	return TagDataStr;
}

void SetDefaults( void ) {

	CFG_Set(TAGID_MAIN_AUTOSELECTALL,     FALSE);
	CFG_Set(TAGID_MAIN_AUTOVIRUSCHECK,    FALSE);
	CFG_Set(TAGID_MAIN_KEEPFULLPATH,      TRUE);
	CFG_Set(TAGID_MAIN_DEFARCHIVEPATH,    (ULONG) "Ram:");
	CFG_Set(TAGID_MAIN_DEFDESTPATH,       (ULONG) "Ram:");
	CFG_Set(TAGID_MAIN_ARCVIEWMODE,       MUIV_ArcView_SwapViewMode_Explorer);
	CFG_Set(TAGID_MAIN_TEMPPATH,          (ULONG) "T:");
	CFG_Set(TAGID_MAIN_SHOWEXTRACTREQ,    FALSE);
	CFG_Set(TAGID_MAIN_DOEXECHECK,        FALSE);
	CFG_Set(TAGID_MAIN_AUTOENTERTARS,     TRUE);
	CFG_Set(TAGID_MAIN_DELLINGERINGTEMPS, FALSE);
	CFG_Set(TAGID_MAIN_VIEWCOLUMNS,       VIEWCOLUMN_ALL | VIEWCOLUMN_SHOWBARS);
	CFG_Set(TAGID_MAIN_CHECKXCMD,         (ULONG) "C:CheckX %fp");
	CFG_Set(TAGID_MAIN_LEAVEPROGRESSOPEN, FALSE);
	CFG_Set(TAGID_XAD_NOEXTXADCLIENTS,    FALSE);
	CFG_Set(TAGID_TB_APPEARANCE,          MUIV_VXToolBar_Appearance_IconsAndText);
	CFG_Set(TAGID_TB_SUNNY,               FALSE);
	CFG_Set(TAGID_TB_RAISED,              TRUE);
	CFG_Set(TAGID_TB_BORDERLESS,          FALSE);
	CFG_Set(TAGID_TB_SMALL,               FALSE);
}

GPROTO BOOL CFG_Init( void ) {

	DB(dbg("Init Cfg..."))
	CFG_LoadConfig(NULL);

	return TRUE;
}

GPROTO void CFG_Free( void ) {
	/* Free all string ConfigTags */

	struct ConfigTag *CTA = CTArray;

	DB(dbg("Free Cfg..."))

	while (CTA->ct_TagStr) {

		if (CTA->ct_TagType == TAGTYPE_STRING) {

			MEM_FreeVec((APTR)CTA->ct_TagData);
			CTA->ct_TagData = (ULONG) NULL;
		}

		CTA++;
	}
}

GPROTO void CFG_Set( ULONG TagID, ULONG TagData ) {

	switch (CTArray[TagID].ct_TagType) {

	case TAGTYPE_VAL:
		DB(dbg("CFG_Set(tagid=%-20.20s, tagdata VAL    = 0x%08lx)",
		       GetConfigTagStr(TagID), TagData))

		CTArray[TagID].ct_TagData = TagData;
		break;

	case TAGTYPE_BOOL:
		DB(dbg("CFG_Set(tagid=%-20.20s, tagdata BOOL   = %-15.15s)",
		       GetConfigTagStr(TagID), TagData ? "True" : "False"))

		CTArray[TagID].ct_TagData = TagData;
		break;

	case TAGTYPE_STRING:
		DB(dbg("CFG_Set(tagid=%-20.20s, tagdata STRING = %-15.15s)",
		       GetConfigTagStr(TagID), TagData))

		if (CTArray[TagID].ct_TagData) {
			MEM_FreeVec((APTR)CTArray[TagID].ct_TagData);
			CTArray[TagID].ct_TagData = (ULONG) NULL;
		}

		if (TagData) {
			CTArray[TagID].ct_TagData = (ULONG) MEM_StrToVec((UBYTE *)TagData);
		}
		break;

	case TAGTYPE_SYMBOL:
		DB(dbg("CFG_Set(tagid=%-20.20s, tagdata SYMBOL = %-15.15s)",
		       GetConfigTagStr(TagID), CTArray[TagID].ct_SymMap[TagData]))

		CTArray[TagID].ct_TagData = TagData;
		break;
	}
}

GPROTO BOOL CFG_LoadConfig( UBYTE *FileName ) {

	BPTR InFH;
	UBYTE LineBuf[256];
	LONG NewTagID;

	if (!FileName) {
		FileName = CFG_NAME;
	}

	DB(dbg("CFG_LoadConfig(filename=\"%s\")", FileName))

	SetDefaults();

	if ((InFH = Open(FileName, MODE_OLDFILE))) {

		if (FGets(InFH, LineBuf, sizeof(LineBuf)-1) &&
		        Strnicmp(LineBuf, CFG_ID, sizeof(CFG_ID)-1) == 0) {

			for (;;) {

				UBYTE *TagDataStr = ReadConfigTag(InFH, &NewTagID);

				if (((LONG)TagDataStr) == -1) {
					break; /* EOF */
				}

				if (!TagDataStr) {
					continue; /* Unknown tag ID, comment, etc. */
				}

				switch (CTArray[NewTagID].ct_TagType) {

				case TAGTYPE_VAL:
					if (TagDataStr[0] == '0' && TagDataStr[1] == 'x') {
						CFG_Set(NewTagID, strtoul(TagDataStr + 2, NULL, 16));
					} else if (TagDataStr[0] == '$') {
						CFG_Set(NewTagID, strtoul(TagDataStr + 1, NULL, 16));
					}
					break;

				case TAGTYPE_BOOL:
					if (Stricmp(TagDataStr, YES) == 0) {
						CFG_Set(NewTagID, TRUE);
					} else {
						CFG_Set(NewTagID, FALSE);
					}
					break;

				case TAGTYPE_STRING:
					CFG_Set(NewTagID, (ULONG) TagDataStr);
					break;

				case TAGTYPE_SYMBOL: {
						struct SymMap *SM = CTArray[NewTagID].ct_SymMap;
						CFG_Set(NewTagID, 0);

						while(SM->sm_Symbol) {
							if (Stricmp(SM->sm_Symbol, TagDataStr) == 0) {
								CFG_Set(NewTagID, SM->sm_Value);
								break;
							}
							SM++;
						}
					}
					break;
				}

				MEM_FreeVec(TagDataStr);
			}
		} else {

			GUI_PrgError(STR(SID_SETTINGS_INVALID, "Invalid settings file! Using defaults."), NULL);
			SetDefaults();

		}

		Close(InFH);
	}

	return TRUE;
}

#define CFG_IOBUFFER_SIZE (1024*8)

GPROTO BOOL CFG_SaveConfig( UBYTE *FileName ) {

	BPTR outfh;

	if (!FileName) {
		FileName = CFG_NAME;
	}

	DB(dbg("CFG_SaveConfig(filename=\"%s\")", FileName))

	GUI_PrintStatus(STR(SID_SETTINGS_SAVING, "Saving settings..."), 0);

	if ((outfh = Open(FileName, MODE_NEWFILE))) {

		LONG r = 0;

		if (DOSBase->dl_lib.lib_Version >= 40) {
			r = SetVBuf(outfh, NULL, BUF_FULL, CFG_IOBUFFER_SIZE);
		}

		if (r == 0) {
			FPuts(outfh, CFG_HEADER);

			// FIXME: This is the ugly way!
			SaveConfigTag(outfh, TAGID_MAIN_AUTOSELECTALL);
			SaveConfigTag(outfh, TAGID_MAIN_AUTOVIRUSCHECK);
			SaveConfigTag(outfh, TAGID_MAIN_DEFARCHIVEPATH);
			SaveConfigTag(outfh, TAGID_MAIN_DEFDESTPATH);
			SaveConfigTag(outfh, TAGID_MAIN_ARCVIEWMODE);
			SaveConfigTag(outfh, TAGID_MAIN_KEEPFULLPATH);
			SaveConfigTag(outfh, TAGID_MAIN_TEMPPATH);
			SaveConfigTag(outfh, TAGID_MAIN_SHOWEXTRACTREQ);
			SaveConfigTag(outfh, TAGID_MAIN_CHECKXCMD);
			SaveConfigTag(outfh, TAGID_MAIN_DELLINGERINGTEMPS);
			SaveConfigTag(outfh, TAGID_MAIN_AUTOENTERTARS);
			SaveConfigTag(outfh, TAGID_MAIN_DOEXECHECK);
			SaveConfigTag(outfh, TAGID_MAIN_VIEWCOLUMNS);
			SaveConfigTag(outfh, TAGID_MAIN_LEAVEPROGRESSOPEN);
			SaveConfigTag(outfh, TAGID_XAD_NOEXTXADCLIENTS);
			SaveConfigTag(outfh, TAGID_TB_APPEARANCE);
			SaveConfigTag(outfh, TAGID_TB_SUNNY);
			SaveConfigTag(outfh, TAGID_TB_RAISED);
			SaveConfigTag(outfh, TAGID_TB_BORDERLESS);
			SaveConfigTag(outfh, TAGID_TB_SMALL);
		}
		Close(outfh);
	}

	GUI_PrintStatus(STR(SID_SETTINGS_SAVED, "Settings saved."), 0);

	return TRUE;
}
