/*
 $Id: vx_main.h,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Main module definitions.

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

#ifndef VX_MAIN_H
#define VX_MAIN_H 1

#define VX_STACKSIZE   (1024 * 48) /* 48KB of stack space */
#define VX_PRIORITY    0
#define ARGPLATE    "ARCHIVES/M,DEST/K,PRI=PRIORITY/N/K,VM=VIEWMODE/K"
#define GL_OPTIONAL 1
#define GL_NEEDED   2
extern struct DiskObject *VXDiskObj;

struct GetLibs {
	UBYTE           *gl_Name;
	LONG             gl_Version;
	struct Library **gl_LibBasePtr;
	ULONG            gl_Mode;       /* GL_#? */
};

struct ArgLayout {
	UBYTE **al_ARCHIVES;
	UBYTE  *al_DEST;
	LONG   *al_PRI;
	UBYTE  *al_VIEWMODE;
};

enum
{
	TTID_VIEWER = 0,
	TTID_TMPDIR,
	TTID_ARCSEARCHPAT,
	TTID_IMAGEPATH,
	TTID_AMOUNT
};

#ifndef VX_MAIN_PROTOS_H
#include "vx_main_protos.h"
#endif

#endif /* VX_MAIN_H */
