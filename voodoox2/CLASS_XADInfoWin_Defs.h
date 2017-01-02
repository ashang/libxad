/*
 $Id: CLASS_XADInfoWin_Defs.h,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Holds defines for XAD information window.

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

#ifndef XADINFOWIN_DEFS_H
#define XADINFOWIN_DEFS_H

/* Lets assume there'll never be more than 1000 XAD clients, yeah
   I know this is crappy hardcoded programming, but I'm being a
   bit lazy here. :-) */

#define MAX_XAD_CLIENT_COUNT      1000
#define AMINET_LOCATION           "http://www.aminet.net/pub/aminet/"
#define LIST_FORMAT_XADCLIENTINFO "BAR,BAR,BAR"

#define GetLine \
{ \
	UBYTE *p = bp = FGets(fh, (UBYTE *) &buf, sizeof(buf)-1); buf[sizeof(buf)-1] = 0; \
	if (!bp) break; \
	while (*bp == ' ' || *bp == '\t') bp++; \
	while (*p) if (*p == 0x1B) *p = ' '; else p++; /* Prevent MUI exploit, in case we get a tojan guide. */ \
}

#define NOXADGUIDEFILE \
	"xadclients.guide is not installed!\n" \
	"Please download this from Aminet at /docs/hyper/xadclients.lha and\n" \
	"copy xadclients.guide into VX's home directory.\n"

#define NOCLIENTINFOAVAIL \
	"Sorry, xadclients.guide doesn't hold any information on this client."

enum
{
    CLICKTYPE_NODE = 0,
    CLICKTYPE_HTTP,
    CLICKTYPE_FTP,
    CLICKTYPE_MAILTO,
    CLICKTYPE_AMINET,
    CLICKTYPE_AMINETRM
};

struct CI {
	UBYTE  *ci_name;
	UBYTE  *ci_node;
	UBYTE  *ci_author;
	UBYTE  *ci_email;
	UBYTE  *ci_description;
};

struct ClientParam {
	struct        CI *cp_ci;
	struct xadClient *cp_xadc;
};

struct XADClientListEntry {
	UWORD       xcle_Version;
	UWORD       xcle_MasterVersion;
	UWORD       xcle_ClientVersion;
	UWORD       xcle_ClientRevision;
	ULONG       xcle_RecogSize;
	ULONG       xcle_Flags;
	ULONG       xcle_Identifier;
	UBYTE      *xcle_ArchiverName;
	UBYTE       xcle_MasterVersionStrBuf[8];
	UBYTE       xcle_ClientVerRevStrBuf[32];
	UBYTE       xcle_FlagsStrBuf[128];
	UBYTE       xcle_AuthorName[80];
	UBYTE       xcle_AuthorEMail[80];
	UBYTE      *xcle_Description;
	BOOL        xcle_Description_ready;
	struct CI  *xcle_CI;
	struct List xcle_ClickList;
};

struct AmigaGuideNodeDef {
	UBYTE      *agnd_nodeid;
	UBYTE      *agnd_nodetitle; /* Never used... */
	LONG        agnd_pof_start;
	LONG        agnd_pof_end;
	UBYTE      *agnd_nodetext;
};

#endif /* XADINFOWIN_DEFS_H */
