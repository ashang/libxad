/*
 $Id: vx_global.h,v 1.3 2004/01/25 15:12:58 andrewbell Exp $
 Header for vx_global.c

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

#ifndef VX_GLOBAL_H
#define VX_GLOBAL_H

#ifndef VX_INCLUDE_H
#include <vx_include.h>
#endif

extern Object *App;
extern Object *MainWin;
extern Object *SettingsWin;
extern Object *ErrorWin;
extern Object *ProgressWin;
extern Object *GetStringWin;
extern Object *GetPatternWin;
extern Object *SearchWin;
extern Object *ArcInfoWin;
extern Object *MultiPartWin;
extern Object *AboutWin;
extern Object *XADInfoWin;
extern Object *NewFileTypeWin;
extern Object *VersionWin;
extern Object *ExtFileInfoWin;
extern Object *BusyWin;

extern Object *ArcHistory;
extern Object *ArcView;
extern struct Process *VXProcess;
extern struct WBStartup *WBMsg;
extern struct Library *MUIMasterBase;
extern struct Library *IFFParseBase;
extern struct IntuitionBase *IntuitionBase;
extern struct Library *UtilityBase;
extern struct Library *WorkbenchBase;
extern struct Library *DiskFontBase;
extern struct Library *GfxBase;
extern struct xadMasterBase *xadMasterBase;
extern struct Library *OpenURLBase;
extern struct Library *WBStartBase;
extern struct Library *IconBase;
extern struct Library *xvsBase;
extern struct Library *LocaleBase;
extern UWORD PutChProc[];

extern BOOL FromWB;
extern struct Locale *Locale;

/* From the startup code */
extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;

#endif /* VX_GLOBAL_H */

