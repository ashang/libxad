/*
 $Id: vx_allHeaders.h,v 1.2 2004/01/25 04:34:25 andrewbell Exp $
 Includes all system headers and 3rd party library/class headers.

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

/* Don't place application specific stuff in here. */

#ifndef VX_ALLHEADERS_H
#define VX_ALLHEADERS_H

/***************************************************************************/
/* ANSI headers                                                            */
/***************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/***************************************************************************/
/* Amiga OS headers                                                        */
/***************************************************************************/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_EXECBASE_H
#include <exec/execbase.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef EXEC_PORTS_H
#include <exec/ports.h>
#endif

#ifndef EXEC_MEMORY_H
#include <exec/memory.h>
#endif

#ifndef DOS_STDIO_H
#include <dos/stdio.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef DOS_DOSTAGS_H
#include <dos/dostags.h>
#endif

#ifndef DOS_DOSEXTENS_H
#include <dos/dosextens.h>
#endif

#ifndef DOS_DATETIME_H
#include <dos/datetime.h>
#endif

#ifndef LIBRARIES_ASL_H
#include <libraries/asl.h>
#endif

#ifndef UTILITY_DATE_H
#include <utility/date.h>
#endif

#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef UTILITY_HOOKS_H
#include <utility/hooks.h>
#endif

#ifndef WORKBENCH_STARTUP_H
#include <workbench/startup.h>
#endif

#ifndef WORKBENCH_ICON_H
#include <workbench/icon.h>
#endif

#ifndef WORKBENCH_WORKBENCH_H
#include <workbench/workbench.h>
#endif

#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif

#ifndef LIBRARIES_MUI_H
#include <libraries/mui.h>
#endif

#ifndef LIBRARIES_GADTOOLS_H
#include <libraries/gadtools.h>
#endif


#ifndef LIBRARIES_LOCALE_H
#include <libraries/locale.h>
#endif

/***************************************************************************/
/* Amiga OS Protos                                                         */
/***************************************************************************/

#define __NOLIBBASE__

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/asl.h>
#include <proto/wb.h>
#include <proto/icon.h>
#include <proto/locale.h>
#include <clib/alib_protos.h>

/***************************************************************************/
/* Third Party                                                             */
/***************************************************************************/

#ifndef LIBRARIES_XADMASTER_H
#include <libraries/xadmaster.h>
#endif

#ifndef LIBRARIES_XVS_H
#include <libraries/xvs.h>
#endif

#ifndef LIBRARIES_WBSTART_H
#include <libraries/wbstart.h>
#endif

#ifndef MUI_NList_MCC_H
#include <mui/NList_mcc.h>
#endif

#ifndef MUI_NListview_MCC_H
#include <mui/NListview_mcc.h>
#endif

#ifndef SPEEDBAR_MCC_H
#include <mui/SpeedBar_mcc.h>
#endif

#ifndef BUSY_MCC_H
#include <mui/Busy_mcc.h>
#endif

/***************************************************************************/
/* Third Party Protos                                                      */
/***************************************************************************/

#include <proto/openurl.h>
#include <proto/muimaster.h>
#include <proto/xadmaster.h>
#include <proto/wbstart.h>
#include <proto/xvs.h>

#endif /* VX_ALLHEADERS_H */





