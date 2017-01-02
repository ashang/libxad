/*
 $Id: vx_ipc.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Interprocess communication functions.

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

#include "vx_include.h"
#include "vx_main.h"
#include "vx_ipc.h"
#include "vx_global.h"
#include "vx_misc.h"
#include "vx_arc.h"
#include "vx_debug.h"

struct MsgPort *VXP;

/* TODO: Send an entire tag list to the other VX process instead of using
   a message struct. That way we don't have to worry about message fields
   disappearing between VX versions. Or new ones being added. */

GPROTO BOOL IPC_Init( struct ArgLayout *AD ) {
	DB(dbg("Init IPC..."))

	if (IPC_FindPort()) {

		/* At this point, we've found another VX process in the system.
		   Now we must dispatch our incomming archive list  to it, as a WBArg
		   array. */

		struct WBArg *wba;
		ULONG argcnt = 0;
		struct VXMsg vmsg;

		/* Tell the other VX instance to bring its interface to front */

		IPC_DispatchSimpleCmd(NULL, VXP_CMDID_UNICONIFY);

		/* Collect our arguments, so we can prepare to dispatch them. */

		if ((wba = GetIncommingArcs(&argcnt))) {
			IPC_DispatchWBArgs(NULL, argcnt, wba);
			FreeIncommingArcs(wba, argcnt);
		}

		/* Wipe the message */

		memset(&vmsg, 0, sizeof(struct VXMsg));

		/* Did caller provide a new desination path? If so, dispatch it to the other instance. */

		if (AD->al_DEST) {
			vmsg.vxp_DestPath = AD->al_DEST;
			IPC_DispatchComplexCmd(NULL, VXP_CMDID_CHANGEDEST, &vmsg);
		}

		/* Did caller provide a new priority? If so, dispatch it to the other instance. */

		if (AD->al_PRI) {
			vmsg.vxp_Pri = *(AD->al_PRI);
			IPC_DispatchComplexCmd(NULL, VXP_CMDID_CHANGEPRI, &vmsg);
		}

		/* Does the caller want a new view mode? If so, tell the other instance to change its view mode to suit. */

		if (AD->al_VIEWMODE) {
			vmsg.vxp_ViewMode = ParseViewModeStr(AD->al_VIEWMODE);
			IPC_DispatchComplexCmd(NULL, VXP_CMDID_CHANGEVM, &vmsg);
		}

		/* Quit this process */

		return FALSE;

	} else {

		/* We are the only VX process in the system. Continue with setup. */

		if (!IPC_CreatePort()) {
			return FALSE;
		}

		/* Continue with this process */

		return TRUE;
	}
}

GPROTO void IPC_Free( void ) {
	DB(dbg("Free IPC..."))

	Forbid();
	IPC_FlushPort();
	IPC_DeletePort();
	Permit();
}

LPROTO BOOL IPC_CreatePort( void ) {

	if (!(VXP = CreateMsgPort())) {
		return FALSE;
	}

	VXP->mp_Node.ln_Name = VX_PORTNAME;
	VXP->mp_Node.ln_Pri  = VX_PORTPRI;
	AddPort(VXP);

	return TRUE;
}

LPROTO void IPC_DeletePort( void ) {

	if (!VXP) {
		return;
	}

	Forbid();
	IPC_FlushPort();
	RemPort(VXP);
	DeleteMsgPort(VXP);
	VXP = NULL;

	Permit();
}

LPROTO void IPC_FlushPort( void ) {
	struct Message *m;

	if (!VXP) {
		return;
	}

	while ((m = GetMsg(VXP))) {
		ReplyMsg(m);
	}
}

LPROTO struct MsgPort *IPC_FindPort( void ) {
	struct MsgPort *alienVXP;

	Forbid();
	alienVXP = FindPort(VX_PORTNAME);
	Permit();

	return alienVXP;
}


GPROTO void IPC_DispatchWBArgs( struct MsgPort *destvxp, ULONG argcnt, struct WBArg *wba ) {

	struct MsgPort *temp_rp;

	Forbid();

	if (!destvxp) {
		destvxp = IPC_FindPort();
	}

	if (destvxp && (temp_rp = CreateMsgPort())) {
		struct VXMsg vmsg;

		memset(&vmsg, 0, sizeof(struct VXMsg));

		vmsg.vxp_Msg.mn_Node.ln_Type = NT_MESSAGE;
		vmsg.vxp_Msg.mn_Length       = sizeof(struct VXMsg);
		vmsg.vxp_Msg.mn_ReplyPort    = temp_rp;
		vmsg.vxp_Task                = (struct Task *) VXProcess;
		vmsg.vxp_APIVer              = VX_API_VER;
		vmsg.vxp_CmdID               = VXP_CMDID_LOADARCS;
		vmsg.vxp_ArgCnt              = argcnt;
		vmsg.vxp_Args                = wba;

		PutMsg(destvxp, (struct Message *)&vmsg);
		Wait(1UL << temp_rp->mp_SigBit | SIGBREAKF_CTRL_C);
		DeleteMsgPort(temp_rp);
	}

	Permit();
}

GPROTO void IPC_DispatchSimpleCmd(

    struct MsgPort *destvxp, ULONG cmdid ) {

	/* Dispatch a simple command that needs no arguments. */

	struct MsgPort *temp_rp;

	Forbid();

	if (!destvxp) {
		destvxp = IPC_FindPort();
	}

	if (destvxp && (temp_rp = CreateMsgPort())) {
		struct VXMsg vmsg;

		memset(&vmsg, 0, sizeof(struct VXMsg));

		vmsg.vxp_Msg.mn_Node.ln_Type = NT_MESSAGE;
		vmsg.vxp_Msg.mn_Length       = sizeof(struct VXMsg);
		vmsg.vxp_Msg.mn_ReplyPort    = temp_rp;
		vmsg.vxp_CmdID               = cmdid;
		vmsg.vxp_APIVer              = VX_API_VER;
		vmsg.vxp_Task                = (struct Task *) VXProcess;

		PutMsg(destvxp, (struct Message *)&vmsg);
		Wait(1UL << temp_rp->mp_SigBit | SIGBREAKF_CTRL_C);
		DeleteMsgPort(temp_rp);
	}

	Permit();
}

GPROTO void IPC_DispatchComplexCmd(

    struct MsgPort *destvxp, ULONG cmdid, struct VXMsg *vmsg ) {
	struct MsgPort *temp_rp;

	Forbid();

	if (!destvxp) {
		destvxp = IPC_FindPort();
	}

	if (destvxp && (temp_rp = CreateMsgPort())) {

		vmsg->vxp_Msg.mn_Node.ln_Type = NT_MESSAGE;
		vmsg->vxp_Msg.mn_Length       = sizeof(struct VXMsg);
		vmsg->vxp_Msg.mn_ReplyPort    = temp_rp;
		vmsg->vxp_CmdID               = cmdid;
		vmsg->vxp_APIVer              = VX_API_VER;
		vmsg->vxp_Task                = (struct Task *) VXProcess;

		PutMsg(destvxp, (struct Message *) vmsg);
		Wait(1UL << temp_rp->mp_SigBit | SIGBREAKF_CTRL_C);
		DeleteMsgPort(temp_rp);
	}

	Permit();
}

GPROTO ULONG IPC_GetSigMask( void ) {

	if (!VXP) {
		return 0UL;
	}

	return 1UL << VXP->mp_SigBit;
}

GPROTO BOOL IPC_ProcessCmds( ULONG sigEvent ) {
	/* Return TRUE from this function to quit VX. Generally this function is
	   called from a event loop which processes incoming signals for a process.
	   "sigEvent" provides the signals returned by exec.library/Wait().  */
	   struct VXMsg *vxm;

	BOOL quitflag = FALSE;
	
	if (!VXP) {
		/* If we haven't allocated a message port then there's no
		   point in continuing. */

		return FALSE;
	}

	if (!(sigEvent & (1UL << VXP->mp_SigBit))) {
		/* Our port didn't get a message, so don't do anything. */

		return FALSE;
	}

	/* At this point another VX process has been started, it has
	   discovered this VX process, and is sending us command(s). 
	   Note that in the future this code could be intergrated with an
	   AREXX port, so script writers can have more control over VX. */

	while ((vxm = (struct VXMsg *) GetMsg(VXP))) {

		DB(dbg("IPC: Received msg 0x%08lx with command 0x%08lx",
		       vxm, vxm->vxp_CmdID))

		switch (vxm->vxp_CmdID) {

		default:
			GUI_PrgError(
			    VX_PORTNAME " received an unknown command ID: 0x%08lx",
			    &vxm->vxp_CmdID);
			break;
			/* For debugging only */

		case VXP_CMDID_NOP:
			break;
			/* Other VX is giving us some archives to load. The list of
			   archives is inside the message itself. */

		case VXP_CMDID_LOADARCS:
			ARC_LoadArcsFromWBArgs(vxm->vxp_Args, vxm->vxp_ArgCnt, TRUE);
			break;
			/* Other VX is telling us to quit */

		case VXP_CMDID_QUIT:
			quitflag = TRUE;
			break;
			/* Other VX is telling us to hide our interface */

		case VXP_CMDID_ICONIFY:
			GUI_Iconify(TRUE);
			break;
			/* Other VX is telling us to show our interface */

		case VXP_CMDID_UNICONIFY:
			GUI_Iconify(FALSE);
			DoMethod(GETMAINWIN, MUIM_MainWin_ToFront);
			set(GETMAINWIN, MUIA_Window_Activate, TRUE);
			break;
			/* Other VX is telling us to change our destination path */

		case VXP_CMDID_CHANGEDEST:
			DoMethod(GETMAINWIN, MUIM_MainWin_SetDestPath, vxm->vxp_DestPath);
			break;
			/* Other VX is telling us to change our task priority */

		case VXP_CMDID_CHANGEPRI:
			SetTaskPri((struct Task *)VXProcess, vxm->vxp_Pri);
			break;
			/* Other VX is telling us to change our view mode */

		case VXP_CMDID_CHANGEVM:
			ARC_ChangeViewMode(vxm->vxp_ViewMode);
			break;
		}

		ReplyMsg((struct Message *)vxm);
	}

	return quitflag;
}
