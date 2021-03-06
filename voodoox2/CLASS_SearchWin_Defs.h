/*
 $Id: CLASS_SearchWin_Defs.h,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Holds defines for the search window.

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

#ifndef SEARCHWIN_DEF_H
#define SEARCHWIN_DEF_H

enum
{
    CYCLE_SEARCHMODE_CURRENT = 0,
    CYCLE_SEARCHMODE_ALL
};

struct SearchResultNode {
	UBYTE *srn_ArcPath;
	UBYTE *srn_EntryName;
	ULONG  srn_MatchCnt;
	UBYTE  srn_MatchCntStr[16];
	BOOL   srn_EmbeddedArc;
};

#endif /* SEARCHWIN_DEF_H */

