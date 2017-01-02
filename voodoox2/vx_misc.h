/*
 $Id: vx_misc.h,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Misc module definitions.

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

#ifndef VX_MISC_H
#define VX_MISC_H 1

/* Programs launched by VX get this amount of stack space... */
#define LAUNCH_STACKSIZE (1024 * 16)

struct PathList {
	BPTR pl_NextPath;
	BPTR pl_PathLock;
};

#ifndef VX_MISC_PROTOS_H
#include "vx_misc_protos.h"
#endif

#endif /* VX_MISC_H */
