/*
 $Id: vx_ipc.h,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 IPC definitions.

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

#ifndef VX_IPC_H
#define VX_IPC_H 1

#define VX_PORTNAME "VX-API-Port"
#define VX_PORTPRI  0
#define VX_API_VER  (VERSION << 16 | REVISION)

/* This structure should stay the same with each version of VX,
   in case we encounter a message from an older version of VX
   (unlikely, but possible). */

struct VXMsg {
	struct Message vxp_Msg;        /* API 0x00010001 */
	ULONG          vxp_APIVer;     /* API 0x00010001 */
	struct  Task  *vxp_Task;       /* API 0x00010001 */
	ULONG          vxp_CmdID;      /* API 0x00010001 */
	ULONG          vxp_ArgCnt;     /* API 0x00010001 */
	struct WBArg  *vxp_Args;       /* API 0x00010001 */
	UBYTE         *vxp_DestPath;   /* API 0x00010005 */
	LONG           vxp_Pri;        /* API 0x00010005 */
	ULONG          vxp_ViewMode;   /* API 0x00010005 */
};

/* vxp_APIVer holds a 32 bit value which embeds the version number of
   the VX instance that is dispatching the message to us.
 
   0x00010005
        |   |
        |   +---- VX revision 5
        |
        +--- VX version 1
 
 
   If VX version 1.1 sends a message to VX 1.5, that message will not have any
   fields that are marked "API 0x00010005" or higher. The receiving VX will
   notice that and not attempt to access those fields. This is important to
   prevent it from accessing "random" memory. 
 
   You might think the chance of this happing is pretty slim. It's not! Some
   people like to keep multiple versions of VX around for various reasons.
   Myself included, because I need access to older versions to verify bug
   reports. If I happen to have version 2.0 running, then I start up 1.5.
   1.5 will detect 2.0, send it some commands. If 2.0 didn't verify the
   message, then it would probably lock up or crash. So 2.0 needs a mechanism
   to ensure it doesn't access non-existing structure entries. This is it.
 
   Currently VX will only access such fields when it gets the associated
   command id (listed below). So we don't actually compare the version number
   in this case, because older versions will never send us commands it doesn't
   know of. But the mechanism still needs to be used for those fields that
   aren't releated to a command id.
 
   Example:
 
   if (VXMsg->vxp_APIVer >= VX_API_VER)
   {
		// VX sender has same version as us (or is newer)
		// which means it has "vxp_Foo". Good news! We can now
		// access it!
 
		long xyz = VXMsg->vxp_Foo;
 
		// Add your magic here...
   }
   else
   {
		// VX sender is too old and doesn't have "vxp_Foo"!!!!
		// Avoid it or we'll end up accessing a non-existant field
		// which will contain uninitialised trash.
   }
 
*/

#define VXP_CMDID_NOP        0   /* API 0x00010001 */
#define VXP_CMDID_LOADARCS   2   /* API 0x00010001 */
#define VXP_CMDID_QUIT       3   /* API 0x00010001 */
#define VXP_CMDID_ICONIFY    4   /* API 0x00010001 */
#define VXP_CMDID_UNICONIFY  5   /* API 0x00010001 */
#define VXP_CMDID_CHANGEDEST 6   /* API 0x00010005 */
#define VXP_CMDID_CHANGEPRI  7   /* API 0x00010005 */
#define VXP_CMDID_CHANGEVM   8   /* API 0x00010005 */

#ifndef VX_IPC_PROTOS_H
#include "vx_ipc_protos.h"
#endif

#endif /* VX_IPC_H */
