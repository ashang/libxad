/*
 $Id: vx_misc.c,v 1.3 2004/01/25 15:12:58 andrewbell Exp $
 Assorted functions that fit nowhere else.

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
#include "vx_misc.h"
#include "vx_main.h"
#include "vx_mem.h"
#include "vx_global.h"
#include "vx_debug.h"

GPROTO ULONG GetTasksStackSize( void ) {

	register struct Task *task = FindTask(NULL);

	return (ULONG) task->tc_SPUpper - (ULONG) task->tc_SPLower;
}

GPROTO BOOL DateStampToStr( struct DateStamp *DS, UBYTE *Buf, BOOL CompactDate ) {

	/* Construct a date string using a standard dos.library DateStamp
	 * structure. Example output: Friday 15-Oct-99 18:00:00
	 * Notes: Buf should point to a buffer at least 128 bytes long!
	 */

	UBYTE DayPart[LEN_DATSTRING * 2];
	UBYTE DatePart[LEN_DATSTRING * 2];
	UBYTE TimePart[LEN_DATSTRING * 2];

	struct DateTime dt = { { 0, 0, 0 }, FORMAT_DOS, 0, NULL, NULL, NULL };

	dt.dat_StrDay = DayPart;
	dt.dat_StrDate = DatePart;
	dt.dat_StrTime = TimePart;
	dt.dat_Stamp.ds_Days   = DS->ds_Days;
	dt.dat_Stamp.ds_Minute = DS->ds_Minute;
	dt.dat_Stamp.ds_Tick   = DS->ds_Tick;

	DayPart[0] = 0;
	DatePart[0] = 0;
	TimePart[0] = 0;

	if (DateToStr(&dt)) {

		if (CompactDate) {

			sprintf(Buf, "%s %s", (UBYTE *) &DatePart, (UBYTE *) &TimePart);

		} else {

			sprintf(Buf, "%s %s %s", (UBYTE *) &DayPart, (UBYTE *) &DatePart,
			        (UBYTE *) &TimePart);
		}

		return TRUE;
	}

	return FALSE;
}

#define _100BYTES 100
#define _1KB      1000
#define _10KB     (_1KB * 10)
#define _100KB    (_1KB * 100)
#define _1MB      (_1KB * 1000)
#define _1GB      (_1MB * 1000)

GPROTO UBYTE *FormatULONGToVec( ULONG bytes ) {

	/* Convert an integer that represents a quantity of bytes into a
	 * human readable string, that utilizes units such as K, M and G.
	 * The resulting string must be freed with MEM_FreeVec().
	 *
	 * Notes: We treat 1K as 1000 bytes, not 1024 bytes. 1KB is 1024 bytes.
	 */

	UBYTE bytesBuf[64];

	register UBYTE *suffix;
	register LONG fNum, sNum;

	if (bytes < _1KB) {

		fNum = bytes;
		suffix = " Bytes";
		sNum = -1;

	} else if (bytes < _1MB) {

		fNum = (bytes / _1KB);
		suffix = "K";                      /* Kilobytes: "K" suffix */
		sNum = (bytes % _1KB) / _100BYTES;

	} else if (bytes < _1GB) {

		fNum = (bytes / _1MB);
		suffix = "M";                     /* Megabytes: "M" suffix */
		sNum = (bytes % _1MB) / _100KB;

	} else {

		fNum = (bytes / _1GB);
		suffix = "G";                     /* Gigabytes: "G" suffix */
		sNum = (bytes % _1GB) / _1MB;

	}

	// FIXME: Should really be using snprintf!!!

	if (sNum == -1) {

		sprintf(bytesBuf, "%ld%s", fNum, suffix);

	} else {

		sprintf(bytesBuf, "%ld.%ld%s", fNum, sNum, suffix);

	}

	return MEM_StrToVec(bytesBuf);
}

GPROTO BPTR CloneWBPL( struct WBStartup *WBS ) {

	/* Clones the Workbench process's path list so it can be
	   passed to NP_Path for SystemTags() while under WB context. */

	BPTR FirstPL = 0;
	BOOL CloningFailed = FALSE;
	struct CommandLineInterface *WBCLI;
	struct Process *WBProc;
	register struct PathList *RealPL, *PrevPL = NULL, *ClonePL;

	/* Get the Workbench process from the startup message's
		 reply port SigTask field. */

	if (!WBS || !WBS->sm_Message.mn_ReplyPort ||
	        !WBS->sm_Message.mn_ReplyPort->mp_SigTask) {

		Forbid();
		WBProc = (struct Process *) FindTask("Workbench");
		Permit();

		if (!WBProc) {
			return 0;
		}

	} else {

		WBProc = (struct Process *) WBS->sm_Message.mn_ReplyPort->mp_SigTask;

	}

	if (WBProc->pr_Task.tc_Node.ln_Type != NT_PROCESS || !WBProc->pr_CLI) {

		return 0;
	}

	/* Iterate through the Pathlist, cloning each entry. */

	Forbid();

	WBCLI = (struct CommandLineInterface *) BADDR(WBProc->pr_CLI);
	RealPL = (struct PathList *) BADDR(WBCLI->cli_CommandDir);

	while (RealPL && !CloningFailed) {

		if (RealPL->pl_PathLock &&
		        (ClonePL = AllocVec(sizeof(struct PathList), MEMF_CLEAR|MEMF_PUBLIC))) {

			if (!FirstPL) {
				FirstPL = (BPTR) MKBADDR(ClonePL);
			}

			if ((ClonePL->pl_PathLock = DupLock(RealPL->pl_PathLock))) {

				if (PrevPL) {
					PrevPL->pl_NextPath = MKBADDR(ClonePL);
				}

				PrevPL = ClonePL;
			} else {

				CloningFailed = TRUE;
			}
		} else {

			CloningFailed = TRUE;
		}

		RealPL = (struct PathList *) BADDR(RealPL->pl_NextPath);
	}

	Permit();

	if (CloningFailed) {
		DeleteWBPL(FirstPL);
		FirstPL = 0;
	}

	return FirstPL;
}

GPROTO void DeleteWBPL( BPTR WBPL ) {

	/* Only call this if SystemTags() fails. */

	register struct PathList *NextPL = BADDR(WBPL), *TmpPL;

	while (NextPL) {
		if (NextPL->pl_PathLock) {
			UnLock(NextPL->pl_PathLock);
		}

		TmpPL = (struct PathList *) BADDR(NextPL->pl_NextPath);
		FreeVec(NextPL);
		NextPL = TmpPL;
	}
}

GPROTO BOOL MySystem( UBYTE *CommandLine, BPTR UserOutFH ) {
	BPTR InFH, OutFH = 0, PathList = 0;
	BOOL Success = FALSE;

	/* TODO: Adapt this to use OS 3.5's launching support. */

	DB(dbg("MySystem(commandline=\"%s\")", CommandLine))

	if (WBMsg) {
		if (!(PathList = CloneWBPL(WBMsg))) {
			GUI_PrgError("Failed to clone WB path list!", NULL);
			return FALSE;
		}
	}

	InFH = Open("NIL:", MODE_OLDFILE);

	if (UserOutFH == 0) {
		OutFH = Open("NIL:", MODE_NEWFILE);
	}

	if (InFH && (OutFH || UserOutFH)) {

		if (SystemTags(CommandLine,
		               SYS_Input,    InFH,
		               SYS_Output,   UserOutFH ? UserOutFH : OutFH,
		               /* Note: SYS_Asynch basically tells it to close our FHs */
		               SYS_Asynch,   UserOutFH ? FALSE : TRUE,
		               NP_StackSize, LAUNCH_STACKSIZE,
		               (WBMsg ? NP_Path : TAG_IGNORE), PathList,
		               TAG_DONE) == -1) {

			GUI_PrgDOSError("Failed to run: %s - SystemTags() failed.",
			                &CommandLine);
		} else {

			PathList = 0;
			InFH = 0;
			OutFH = 0;
			Success = TRUE;
		}
	} else {

		GUI_PrgDOSError("Failed to create input and output handles!", NULL);
	}

	if (PathList) {
		DeleteWBPL(PathList);
	}

	if (InFH) {
		Close(InFH);
	}

	if (OutFH) {
		Close(OutFH);
	}

	return Success;
}

GPROTO UBYTE *TrimStringToVec( UBYTE *Str ) {

	/* This routine will take a string and strip all TABS, CRs, LFs and
	 * spaces before and after it. The resulting string is placed in a
	 * memory vector which must eventually be freed with MEM_FreeVec().
	 */

	register UBYTE *StrVec;

	if (!Str) {
		return NULL;
	}

	if ((StrVec = MEM_AllocVec(strlen(Str) + 1))) {

		register UBYTE *eStr, Ch;

		if (*Str) {

			while ((Ch = *Str)) {

				if (Ch == '\t' || Ch == '\r' || Ch == '\n' || Ch == ' ') {
					++Str;
				} else {
					break;
				}
			}

			if (*Str) {
				eStr = Str + (strlen(Str) - 1);

				while (eStr > Str) {
					Ch = *eStr;

					if (Ch == '\t' || Ch == '\r' || Ch == '\n' || Ch == ' ') {
						--eStr;
					} else {
						break;
					}
				}

				memcpy(StrVec, Str, (eStr - Str) + 1);
			}
		}
	}
	return StrVec;
}

GPROTO UBYTE *GetToolType( ULONG tooltypeid ) {

	/* Keep this in sync with the TTID_* enumeration! */

	static UBYTE *VXToolTypes[] = {"VIEWER", "TMPDIR", "ARCSEARCHPAT", "IMAGEPATH"};

	if (!VXDiskObj || tooltypeid >= TTID_AMOUNT) {
		return NULL;
	}

	return FindToolType((STRPTR *)VXDiskObj->do_ToolTypes, VXToolTypes[tooltypeid]);
}

GPROTO void DispatchURL( UBYTE *url ) {

	DB(dbg("DispatchURL(url=%s)\n", url ? url : (UBYTE *)"<< NULL pointer! >>"))

	if (!url) {
		return;
	}

	if (OpenURLBase) {
		URL_Open(url, TAG_DONE);
	} else {
		GUI_PrgError("openurl.library version 3 required!", NULL);
	}
}

GPROTO BOOL CheckForMCC( UBYTE *mccname, LONG minver, LONG minrev,
                         LONG *vergot_ptr, LONG *revgot_ptr ) {

	BOOL available = FALSE;
	LONG ver, rev;
	Object *obj;

	if (vergot_ptr) {
		*vergot_ptr = -1;
	}

	if (revgot_ptr) {
		*revgot_ptr = -1;
	}

	if (!MUIMasterBase) {
		return FALSE;
	}

	if ((obj = MUI_NewObject(mccname, TAG_DONE))) {
		get(obj, MUIA_Version,  &ver);
		get(obj, MUIA_Revision, &rev);

		if (vergot_ptr) {
			*vergot_ptr = ver;
		}

		if (revgot_ptr) {
			*revgot_ptr = rev;
		}

		if (ver > minver || (ver == minver && rev >= minrev)) {
			available = TRUE;
		}

		MUI_DisposeObject(obj);
	}

	return available;
}

GPROTO Object *DoSuperNew( struct IClass *cl, Object *obj, ULONG tag1, ... ) {

	return (Object *) DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL);

}

GPROTO ULONG xget( Object *obj, ULONG attr ) {

	/* IDEA: Maybe we could make this inline assembly under VBCC... */

	ULONG store = 0;
	get(obj, attr, &store);

	return store;
}

GPROTO void FilterString( UBYTE *str ) {

	/* Remove garbage from a string. This doesn't do
	   much at the moment. :-) */

}

GPROTO ULONG ParseViewModeStr( UBYTE *vm_str ) {

	UBYTE *strVec;
	ULONG vm = MUIV_ArcView_SwapViewMode_FilesAndDirs;

	if (!(strVec = TrimStringToVec(vm_str))) {
		return vm;
	}

	if (!Stricmp(strVec, "LIN") || !Stricmp(strVec, "LINEAR")) {

		vm = MUIV_ArcView_SwapViewMode_Linear;

	} else if (!Stricmp(strVec, "FILESANDDIRS") ||
	           !Stricmp(strVec, "FAD")          ||
	           !Stricmp(strVec, "FD")           ||
	           !Stricmp(strVec, "F&D")          ||
	           !Stricmp(strVec, "F+D")) {

		vm = MUIV_ArcView_SwapViewMode_FilesAndDirs;

	} else if (!Stricmp(strVec, "TREE")) {

		vm = MUIV_ArcView_SwapViewMode_Listtree;

	} else if (!Stricmp(strVec, "NLISTTREE")) {

		vm = MUIV_ArcView_SwapViewMode_NListtree;

	} else if (!Stricmp(strVec, "EXPLORER")) {

		vm = MUIV_ArcView_SwapViewMode_Explorer;
	}

	MEM_FreeVec(strVec);

	return vm;
}

GPROTO UBYTE *FindFile( UBYTE **PathList, UBYTE *FileName ) {

	// TODO: Remove req pointer
	// Note: Remember to free the result with MEM_FreeVec().

	long i;
	BPTR lock;
	UBYTE pathbuf[256];

	for (i = 0; PathList[i]; i++) {
		strncpy(pathbuf, PathList[i], sizeof(pathbuf)-1);

		if (!AddPart(pathbuf, FileName, sizeof(pathbuf)-1)) {
			continue;
		}

		if ((lock = Lock(pathbuf, SHARED_LOCK))) {
			UnLock(lock);

			return MEM_StrToVec(pathbuf);
		}
	}

	return NULL;
}

GPROTO BOOL ConstructCommandLine( UBYTE *viewerStr, UBYTE *outBuf, ULONG outLen, UBYTE *filePath ) {

	/* Format string components
	 *
	 * %%   = Literal '%'
	 * %fp  = Full path of extracted subject file.
	 * %psn = Public screen name.
	 */

	register UBYTE *in  = viewerStr, *out = outBuf;

	UBYTE *upper = outBuf + outLen;

	if (!strchr(in, '%')) {

		/* The user hasn't provided any format components, so we'll
		   just append the file arg onto the end of the string. */

		sprintf(out, "%s %s", viewerStr, filePath);

	} else {

		while (*in && out < upper) {

			if (*in == '%') {

				if (!strncmp(in, "%%", 2)) {

					in += 2;
					*out++ = '%';

				} else if (!strncmp(in, "%fp", 3)) {

					in += 3;
					strncpy(out, filePath, upper - out);
					out += strlen(out);

				} else if (!strncmp(in, "%psn", 4)) {

					in += 4;
					strncpy(out, "Workbench", upper - out);
					out += strlen(out);

				} else {
					++in;
				}

			} else {
				*out++ = *in++;
			}
		}
	}
	return TRUE;
}

