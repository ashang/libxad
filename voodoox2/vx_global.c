/*
 $Id: vx_global.c,v 1.3 2004/01/25 15:12:58 andrewbell Exp $
 Global variables

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

/* Put all global variables and data into here. No C code please.
   Use vx_misc.c instead for global C code. */

#ifndef VX_INCLUDE_H
#include "vx_include.h"
#endif /* VX_INCLUDE_H */

Object *App;

/* Never access these object pointers directly!!!
   Always use the GETxxxxWIN macros instead! */

Object *MainWin;
Object *SettingsWin;
Object *ErrorWin;
Object *ProgressWin;
Object *GetStringWin;
Object *GetPatternWin;
Object *SearchWin;
Object *ArcInfoWin;
Object *MultiPartWin;
Object *AboutWin;
Object *XADInfoWin;
Object *NewFileTypeWin;
Object *VersionWin;
Object *ExtFileInfoWin;
Object *ArcHistory;
Object *ArcView;
Object *BusyWin;

struct Process *VXProcess           = NULL;
struct WBStartup *WBMsg             = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct Library *UtilityBase         = NULL;
struct Library *WorkbenchBase       = NULL;
struct Library *GfxBase             = NULL;
struct xadMasterBase *xadMasterBase = NULL;
struct Library *WBStartBase         = NULL;
struct Library *IconBase            = NULL;
struct Library *xvsBase             = NULL;
struct Library *OpenURLBase         = NULL;
struct Library *IFFParseBase        = NULL;
struct Library *MUIMasterBase       = NULL;
struct Library *LocaleBase          = NULL;

#ifdef __NOSTDLIBS__
struct ExecBase *SysBase            = NULL;
struct DosLibrary *DOSBase          = NULL;
#endif /* __NOSTDLIBS__ */

BOOL FromWB                         = FALSE;
UWORD PutChProc[]                   = { 0x16c0, 0x4e75 };
struct Locale *Locale               = NULL;

UBYTE *VTag = VERSTAG " Copyright © " YEAR " Andrew Bell. All rights reserved.";

