/*
 $Id: vx_debug.h,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Debug module definitions.

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

#ifndef VX_DEBUG_H
#define VX_DEBUG_H

#ifndef VX_INCLUDE_H
#include "vx_include.h"
#endif /* VX_INLCUDE_H */

/* We must check for VX_DEBUG_C to stop the protos from clashing. */

#ifndef VX_DEBUG_C
extern void dbg_ShowHide( void );
extern void dbg_Init( void );
extern void dbg_Free( void );
extern void dbg( UBYTE *FmtStr, ... );
extern void X( void );
#endif /* VX_DEBUG_C */

#ifdef DEBUG
#define DB(x) x;
#else
#define DB(x)
#endif

#define WHERE dbg("MARKER REACHED ---> " __FILE__ "/" __FUNC__ "() line %ld\n", __LINE__)

#endif /* VX_DEBUG_H */

