/*
 $Id: vx_io.h,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 I/O definitions.

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

#ifndef VX_IO_H
#define VX_IO_H 1

#define COPYFILE_CHUNKSIZE (1024 * 64)

enum
{
    TMPTYPE_NORMAL = 0, TMPTYPE_DIR
};

#ifndef VX_IO_PROTOS_H
#include "vx_io_protos.h"
#endif

#endif /* VX_IO_H */
