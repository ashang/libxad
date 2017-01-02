/*
 $Id: vx_main.c,v 1.2 2004/01/25 04:32:52 andrewbell Exp $
 Startup code and location of main() function.

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

/*
 * VX 2.x source code - Archive Extraction Tool
 * Copyright © 2000-2004 Andrew Bell. All rights reserved.
 * Created: Thu/27/Jan/2000 
 *
 */

#include "vx_include.h"
#include "vx_main.h"
#include "vx_mem.h"
#include "vx_misc.h"
#include "vx_ipc.h"
#include "vx_cfg.h"
#include "vx_io.h"
#include "vx_arc.h"
#include "vx_global.h"
#include "vx_debug.h"
#include "Classes.h"

struct DiskObject *VXDiskObj = NULL;
BPTR VXDir                   = 0;
BPTR VXOldDir                = 0;
struct RDArgs *ArgInfo       = NULL;
struct ArgLayout AD;

#define MIN_OS_VER      39L
#define MIN_XAD_VER     9L
#define MIN_OPENURL_VER 3L

static struct GetLibs GetLibsData[] = {

{ "intuition.library", MIN_OS_VER, (struct Library **) &IntuitionBase, GL_NEEDED },
{ "utility.library",   MIN_OS_VER,      &UtilityBase,   GL_NEEDED },
{ "workbench.library", MIN_OS_VER,      &WorkbenchBase, GL_NEEDED },
{ "graphics.library",  MIN_OS_VER,      &GfxBase,       GL_NEEDED },
{ "icon.library",      MIN_OS_VER,      &IconBase,      GL_NEEDED },
{ "iffparse.library",  MIN_OS_VER,      &IFFParseBase,  GL_NEEDED },
{ "locale.library",    MIN_OS_VER,      (struct Library **) &LocaleBase,    GL_NEEDED },

/******** Non-OS libraries ********/

{ MUIMASTER_NAME,      MUIMASTER_VLATEST, &MUIMasterBase, GL_NEEDED },
{ "xadmaster.library", MIN_XAD_VER,     (struct Library **) &xadMasterBase, GL_NEEDED },
{ "xvs.library",       XVS_VERSION,     (struct Library **) &xvsBase,       GL_OPTIONAL },
{ "openurl.library",   MIN_OPENURL_VER,  &OpenURLBase,   GL_OPTIONAL },
{ NULL,                0,                 NULL,          0           },

};

#define LIBDBGSTR "OpenLib: %-25.25s Version: %-2ld Result: %s"
#define BUFSIZE 2000

GPROTO BOOL LIBS_Init( void ) {

	register struct GetLibs *GLD = GetLibsData;
	register BOOL OLFailure = FALSE;

	DB(dbg("Init Libs..."))

	while (GLD->gl_Name) {

		if (!(*GLD->gl_LibBasePtr = OpenLibrary(GLD->gl_Name, (LONG) GLD->gl_Version))) {

			DB(dbg(LIBDBGSTR, GLD->gl_Name, (LONG) GLD->gl_Version, "Fail"))

			if (GLD->gl_Mode == GL_NEEDED) {
				OLFailure = TRUE;
			}
		}

		DB(else dbg(LIBDBGSTR, GLD->gl_Name, (LONG) GLD->gl_Version, "OK"))

		GLD++;
	}

	if (OLFailure) {

		if (IntuitionBase) {

			UBYTE TmpStr[128];
			register UBYTE *ErrStr = MEM_AllocVec(BUFSIZE);

			if (ErrStr) {

				strncpy(ErrStr, STR(SID_ERR_LIBS, "Unable to open the following libraries:"), BUFSIZE-1);
				strncat(ErrStr, "\n\n", BUFSIZE-1);

				GLD = GetLibsData;

				while(GLD->gl_Name) {

					if ((*GLD->gl_LibBasePtr == NULL) &&
					        (GLD->gl_Mode == GL_NEEDED)) {

						sprintf(TmpStr, "%-25s %s %ld\n",
						        GLD->gl_Name, STR(SID_VERSION, "version"), GLD->gl_Version);
						strncat(ErrStr, TmpStr, BUFSIZE-1);
					}

					GLD++;
				}

				GUI_PrgError(ErrStr, NULL);
				MEM_FreeVec(ErrStr);
			}
		}

		return FALSE;
	}

	return TRUE;
}

GPROTO void LIBS_Free( void ) {

	register struct GetLibs *GLD = GetLibsData;

	DB(dbg("Free Libs..."))

	while (GLD->gl_Name) {

		if (*GLD->gl_LibBasePtr) {
			CloseLibrary(*GLD->gl_LibBasePtr);
			*GLD->gl_LibBasePtr = NULL;
		}

		GLD++;
	}
}

/***************************************************************************/
/* Base routines */
/***************************************************************************/

#define MIN_NLIST_VER     19
#define MIN_NLIST_REV     97
#define MIN_NLISTTREE_VER 18
#define MIN_NLISTTREE_REV 7
#define MIN_LISTTREE_VER  17
#define MIN_LISTTREE_REV  36
#define MIN_SPEEDBAR_VER  11
#define MIN_SPEEDBAR_REV  7

LPROTO BOOL InitPrg( void ) {
	BOOL dirok = FALSE, oldprefs = FALSE;
	UBYTE *tmpdir, *errvec, *oldname;
	BPTR fh;

	if (!(SysBase->AttnFlags & AFF_68020)) {
		GUI_PrgError("Sorry, VX needs a 68020 or better!", NULL);
		return FALSE;
	}

	if (SysBase->LibNode.lib_Version < 39) {
		GUI_PrgError(STR(SID_ERR_OS, "Sorry, VX requires OS 3.0 or better"), NULL);
		return FALSE;
	}

	DB(dbg_Init())
	DB(dbg("InitPrg(): Begins here..."));

	if (!LIBS_Init()) {
		return FALSE;
	}

	DB(dbg("InitPrg(): Init Locale..."))

	if (!(Locale = OpenLocale(NULL))) {
		return FALSE;
	}

	/* Now that MUI is resident, Lets make sure all the required MCCs are present...*/

	DB(dbg("InitPrg(): Make sure required MCCs are present..."));

	if ((errvec = MEM_AllocVec(1000))) {

		LONG vg, rg, i = 0, mcnt = 0;
		UBYTE buf[64], vgbuf[32];

		struct needed_mcc {
			UBYTE *name; LONG ver, rev;
		};

		static struct needed_mcc mccs[] = {
			{ "NList.mcc",     MIN_NLIST_VER,     MIN_NLIST_REV     },
			{ "SpeedBar.mcc",  MIN_SPEEDBAR_VER,  MIN_SPEEDBAR_REV  },
			{ NULL,            0,                 0                 },
		};

		strcpy(errvec, "VX requires the following MCCs in order to work:\n\n");

		while (mccs[i].name) {

			if (!CheckForMCC(mccs[i].name, mccs[i].ver, mccs[i].rev, &vg, &rg)) {
				mcnt++;

				if (vg == -1) {
					strcpy(vgbuf, "not installed");
				} else {
					sprintf(vgbuf, "you've got %ld.%ld", vg, rg);
				}

				sprintf(buf, "%s %ld.%ld (%s)\n",
				        mccs[i].name, mccs[i].ver, mccs[i].rev, vgbuf);
				strncat(errvec, buf, 999);
			}

			i++;
		}

		if (mcnt != 0) {

			sprintf(buf, "\nPlease install %s, then try running VX again.",
			        mcnt == 1 ? "it" : "them"); /* Get the plural right :-) */
			strncat(errvec, buf, 999);
			GUI_Popup("Missing MCCs!", errvec, NULL, "OK - I'll install them");
		}

		MEM_FreeVec(errvec);

		if (mcnt != 0) {
			return FALSE;
		}

	}

	if (!IO_Init()) {
		return FALSE;
	}

	DB(dbg("InitPrg(): Make sure ENVARC: is valid..."));

	if ((fh = Lock("ENVARC:", SHARED_LOCK))) {
		UnLock(fh);
	} else {
		GUI_Popup("Startup error",
		          "This system doesn't have ENVARC: setup correctly!!!\n"
		          "You need this in order for VX to work.", NULL, "OK");

		return FALSE;
	}

	DB(dbg("InitPrg(): Create ENV:/ENVARC: dirs (if they don't already exist)..."));

	if ((fh = Lock("ENV:VX", SHARED_LOCK))) {
		UnLock(fh);
	} else {
		UnLock(CreateDir("ENV:VX"));
	}

	if ((fh = Lock("ENVARC:VX", SHARED_LOCK))) {
		UnLock(fh);
	} else {
		UnLock(CreateDir("ENVARC:VX"));
	}

	DB(dbg("InitPrg(): Check for obsolete config files..."));

	if ((fh = Lock(oldname = "PROGDIR:VX.settings", SHARED_LOCK))) {
		UnLock(fh);
		if (IO_CopyFile(oldname, "ENVARC:VX/VX.settings", FALSE)) {
			DeleteFile(oldname);
			oldprefs = TRUE;
		}
	}

	if ((fh = Lock(oldname = "PROGDIR:VX.filetypes", SHARED_LOCK))) {
		UnLock(fh);
		if (IO_CopyFile(oldname, "ENVARC:VX/VX.filetypes...", FALSE)) {
			DeleteFile(oldname);
			oldprefs = TRUE;
		}
	}

	if ((fh = Lock(oldname = "PROGDIR:VX.recent...", SHARED_LOCK))) {
		UnLock(fh);
		if (IO_CopyFile(oldname, "ENVARC:VX/VX.recent", FALSE)) {
			DeleteFile(oldname);
			oldprefs = TRUE;
		}
	}

	if ((fh = Lock(oldname = "PROGDIR:VX.toolbar...", SHARED_LOCK))) {
		UnLock(fh);
		if (IO_CopyFile(oldname, "ENVARC:VX/VX.toolbar", FALSE)) {
			DeleteFile(oldname);
			oldprefs = TRUE;
		}
	}

	if (oldprefs) {
		GUI_Popup("Old prefs",
		          "VX found some old settings files in its home directory.\n"
		          "Settings are now stored in ENVARC:VX/. The existing settings\n"
		          "files have been moved there to avoid confusion.", NULL, "OK");
	}

	if (!CFG_Init()) {
		return FALSE;
	}

	if (!WB_and_CLI_Init()) {
		return FALSE;
	}

	if (!IPC_Init((struct ArgLayout *)&AD)) {
		return FALSE;
	}

	if (!ARC_Init()) {
		return FALSE;
	}

	DB(dbg("InitPrg(): Call VX_SetupClasses()..."));

	if (!VX_SetupClasses()) {
		GUI_Popup("Error", "Unable to create internal MUI classes!\n", NULL, "OK");
		return FALSE;
	}

	DB(dbg("InitPrg(): Check/create temporary directory..."))

	/* Check/create temporary directory... */

	if ((tmpdir = (UBYTE *) CFG_Get(TAGID_MAIN_TEMPPATH))) {

		struct FileInfoBlock *fib;
		BPTR dlock;

		if ((fib = IO_GetFIBVec(tmpdir))) {
			if (fib->fib_DirEntryType > 0) {
				dirok = TRUE;
			}

			MEM_FreeVec(fib);

		} else if (IoErr() == ERROR_OBJECT_NOT_FOUND) {

			if ((dlock = CreateDir(tmpdir))) {
				UnLock(dlock);
				dirok = TRUE;
			}
		}
	}

	if (!dirok) {
		CFG_Set(TAGID_MAIN_TEMPPATH, (ULONG) "T:");
	}

	DB(dbg("InitPrg(): Check for lingering temps..."))

	if (CFG_Get(TAGID_MAIN_DELLINGERINGTEMPS)) {
		IO_KillLingeringTemps(); /* Clean up any temp files */
	}

	DB(dbg("InitPrg() was successful!"))

	return TRUE;
}

LPROTO void EndPrg( void ) {

	VX_CleanupClasses();
	ARC_Free();
	IPC_Free();
	WB_and_CLI_Free();
	CFG_Free();
	IO_Free();

	DB(dbg_Free())

	if (Locale) {
		CloseLocale(Locale);
	}

	LIBS_Free();

	DB(dbg("Bye bye!"))
}

LPROTO BOOL WB_and_CLI_Init( void ) {

	UBYTE tmpbuf[256];

	if (WBMsg) { /* We're from WB */
		BPTR oldcd;

		FromWB = TRUE;
		oldcd = CurrentDir(WBMsg->sm_ArgList[0].wa_Lock);
		strncpy(tmpbuf, WBMsg->sm_ArgList[0].wa_Name, sizeof(tmpbuf)-1);
		VXDiskObj = GetDiskObject(tmpbuf);
		CurrentDir(oldcd);

		if ((VXDir = DupLock(GetProgramDir()))) {
			VXOldDir = CurrentDir(VXDir);
		}

		return TRUE;

	} else { /* We're from Shell/CLI */

		FromWB = FALSE;

		if (!(ArgInfo = ReadArgs(ARGPLATE, (LONG *) &AD, NULL))) {
			GUI_PrgDOSError(STR(SID_ERR_ARGS, "Failed to read Shell/CLI arguments"), NULL);
			return FALSE;
		}

		if (GetProgramName(tmpbuf, sizeof(tmpbuf)-1)) {
			VXDiskObj = GetDiskObject(tmpbuf);
		}

		return TRUE;
	}
}

LPROTO void WB_and_CLI_Free( void ) {

	if (VXDiskObj) {
		FreeDiskObject(VXDiskObj);
		VXDiskObj = NULL;
	}

	if (ArgInfo) {
		FreeArgs(ArgInfo);
		ArgInfo = NULL;
	}

	if (VXDir) {
		UnLock(CurrentDir(VXOldDir));
		VXDir = 0;
	}
}

LPROTO void DoPrg( void ) {

	ULONG argcnt = 0, IPC_SigFlag = IPC_GetSigMask();
	struct WBArg *wba;
	extern ULONG MEM_Flush_SigFlag;

	DB(dbg("DoPrg(): Create application object"))

	if (!(App = VXObject, MUIA_VX_ArgLayout, &AD, End)) {
		return;
	}

	DB(dbg("DoPrg(): Check incomming args..."))

	if ((wba = GetIncommingArcs(&argcnt))) {
		ARC_LoadArcsFromWBArgs(wba, argcnt, TRUE);
		FreeIncommingArcs(wba, argcnt);
	}

	DB(dbg("DoPrg(): Entering event loop..."))

	for (;;) {

		ULONG Sigs = 0, SigEvent;

		if (DoMethod(App, MUIM_Application_NewInput, &Sigs) == MUIV_Application_ReturnID_Quit) {
			break;
		}

		if (Sigs == 0) {
			continue;
		}

		SigEvent = Wait(Sigs | SIGBREAKF_CTRL_C | IPC_SigFlag | MEM_Flush_SigFlag);

		if (SigEvent & SIGBREAKF_CTRL_C) {
			break;
		}

		if (SigEvent & IPC_SigFlag) {
			if (IPC_ProcessCmds(SigEvent)) {
				break;
			}
		}

		if (SigEvent & MEM_Flush_SigFlag) {
			GUI_PrintStatus("Memory is low! Flushing non-essential resources...", 0);
			ARC_Flush();
		}
	}

	MUI_DisposeObject(App);
	App = NULL;
}

GPROTO struct WBArg *GetIncommingArcs( ULONG *argcnt ) {

	struct WBArg *wba = NULL, *waptr;
	register ULONG cnt;

	/* This routine construct a WBArg array of all in archives
	   the user pass at the command line prompt or via Workbench. */

	if (FromWB && WBMsg && WBMsg->sm_NumArgs > 1) {

		if ((wba = MEM_AllocVec(sizeof(struct WBArg) * WBMsg->sm_NumArgs))) {

			*argcnt = WBMsg->sm_NumArgs - 1;
			cnt = 0;

			while (cnt < *argcnt) {

				wba[cnt].wa_Name = MEM_StrToVec(WBMsg->sm_ArgList[1 + cnt].wa_Name);
				wba[cnt].wa_Lock = DupLock(WBMsg->sm_ArgList[1 + cnt].wa_Lock);

				if (!wba[cnt].wa_Name || !wba[cnt].wa_Lock) {

					if (wba[cnt].wa_Name) {
						MEM_FreeVec(wba[cnt].wa_Name);
						wba[cnt].wa_Name = NULL;

					}
					if (wba[cnt].wa_Lock) {
						UnLock(wba[cnt].wa_Lock);
						wba[cnt].wa_Lock = 0;

					}
					/* Note: If this fails, we don't increment Cnt, we also subtract
					   from the arg cnt. */

					(*argcnt)--;

				} else {

					++cnt;
				}
			}
		} else {

			GUI_PrgError(STR(SID_ERR_NOMEM, "Out of memory!"), NULL);

		}
	} else if (!FromWB && AD.al_ARCHIVES) {

		register UBYTE **nptr = (UBYTE **) AD.al_ARCHIVES;

		*argcnt = 0;

		while (*nptr++) {
			(*argcnt)++;
		}

		if ((wba = MEM_AllocVec(sizeof(struct WBArg) * (*argcnt)))) {
			waptr = wba;
			nptr = (UBYTE **) AD.al_ARCHIVES;

			while (*nptr) {
				UBYTE tmppath[256];

				strncpy(tmppath, *nptr++, sizeof(tmppath)-1);
				waptr->wa_Name = MEM_StrToVec(FilePart(tmppath));
				*((UBYTE *)PathPart(tmppath)) = 0;
				waptr->wa_Lock = Lock(tmppath, SHARED_LOCK);

				if (!waptr->wa_Lock || !waptr->wa_Name) {

					if (waptr->wa_Lock) {
						UnLock(waptr->wa_Lock);
						waptr->wa_Lock = 0;
					}

					if (waptr->wa_Name) {
						MEM_FreeVec(waptr->wa_Name);
						waptr->wa_Name = NULL;
					}

					/* Note: If this fails, we don't increment "waptr", we also
					         subtract from the arg cnt. */

					(*argcnt)--;

				} else {
					++waptr;
				}
			}
		}
	}

	return wba;
}

GPROTO void FreeIncommingArcs( struct WBArg *wba, ULONG argcnt ) {

	register ULONG cnt = 0;

	if (!wba || !argcnt) {
		return;
	}

	while (cnt < argcnt) {

		if (wba[cnt].wa_Lock) {
			UnLock(wba[cnt].wa_Lock);
		}

		if (wba[cnt].wa_Name) {
			MEM_FreeVec(wba[cnt].wa_Name);
			++cnt;
		}
	}

	MEM_FreeVec(wba);
}

LPROTO int main( void ) {

	static int retcode = RETURN_FAIL; /* Keep static! */

	/* DOSBase = OpenLibrary("dos.library", MIN_OS_VER); */

#ifdef __NOSTDLIBS__

	SysBase = *((struct ExecBase **)4);
	VXProcess = (struct Process *) FindTask(NULL);
	if (!VXProcess->pr_CLI) {
		for (;;) {
			WaitPort(&VXProcess->pr_MsgPort);
			if (WBMsg = (struct WBStartup *) GetMsg(&VXProcess->pr_MsgPort)) {
				break;
			}
		}
	} else {
		WBMsg = NULL;
	}
#else

	VXProcess = (struct Process *) FindTask(NULL);

#endif /* __NOSTDLIBS__ */

	SetTaskPri((struct Task *) VXProcess, VX_PRIORITY);

	if (MEM_Init()) {

		if (GetTasksStackSize() >= VX_STACKSIZE) {

			if (InitPrg()) {
				DoPrg();
				retcode = RETURN_OK;
			}

			EndPrg();

		} else {

			static UBYTE *StackVec = NULL;
			static struct StackSwapStruct SSS;

			if ((StackVec = AllocVec(VX_STACKSIZE, MEMF_PUBLIC | MEMF_CLEAR))) {

				SSS.stk_Lower   = StackVec;
				SSS.stk_Upper   = (ULONG) (StackVec + VX_STACKSIZE);
				SSS.stk_Pointer = (APTR) SSS.stk_Upper;

				StackSwap(&SSS);

				if (InitPrg()) {
					DoPrg();
					retcode = RETURN_OK;
				}

				EndPrg();

				StackSwap(&SSS);
				FreeVec(StackVec);

			} else {

				GUI_PrgError(STR(SID_ERR_NOSTACKMEM, "Failed to allocate stack memory!"), NULL);
			}
		}

		MEM_Free();
	} else {

		GUI_PrgError(STR(SID_ERR_NOMEM, "Out of memory!"), NULL);
	}

#ifdef __NOSTDLIBS__
	if (WBMsg) {

		Forbid();
		ReplyMsg((struct Message *)WBMsg);
		WBMsg = NULL;

	}
#endif

	return retcode;
}

/***************************************************************************/
/* GUI code from the old 1.x days (that survived the great rewrite) */
/***************************************************************************/

GPROTO void GUI_GetInitialAslDims(

    UWORD *LeftEdgePtr, UWORD *TopEdgePtr,
    UWORD *WidthPtr, UWORD *HeightPtr ) {
	struct Screen *Scr = NULL;

	DB(if (GETMAINWIN == NULL) dbg("WARNING: GETMAINWIN in GUI_GetInitialAslDims is NULL!!!\n");)

	get(GETMAINWIN, MUIA_Window_Screen, &Scr);

	if (Scr) {

		if (Scr->Height < INITIAL_ASLREQ_HEIGHT) {
			*TopEdgePtr = 0;
			*HeightPtr = Scr->Height;
		} else {
			*TopEdgePtr = (Scr->Height - INITIAL_ASLREQ_HEIGHT) / 2;
			*HeightPtr = INITIAL_ASLREQ_HEIGHT;
		}

		if (Scr->Width < INITIAL_ASLREQ_WIDTH) {
			*LeftEdgePtr = 0;
			*WidthPtr = Scr->Width;
		} else {
			*LeftEdgePtr = (Scr->Width - INITIAL_ASLREQ_WIDTH) / 2;
			*WidthPtr = INITIAL_ASLREQ_WIDTH;
		}

	} else {

		*LeftEdgePtr  = INITIAL_ASLREQ_LEFTEDGE;
		*TopEdgePtr   = INITIAL_ASLREQ_TOPEDGE;
		*WidthPtr     = INITIAL_ASLREQ_WIDTH;
		*HeightPtr    = INITIAL_ASLREQ_HEIGHT;
	}
}

GPROTO void GUI_PrintStatus( UBYTE *Str, ULONG Arg1, ... ) {

	UBYTE tmpbuf[256];

	if (!App) {
		return;
	}

	RawDoFmt(Str, &Arg1, (void *) &PutChProc, &tmpbuf);
	DoMethod(GETMAINWIN, MUIM_MainWin_SetStatus, &tmpbuf);

}

GPROTO void MCC_GetNListVerRev( ULONG *verptr, ULONG *revptr ) {

	Object *o;

	if ((o = NListObject, InputListFrame, End)) {

		if (verptr) {
			get(o, MUIA_Version,  verptr);
		}

		if (revptr) {
			get(o, MUIA_Revision, verptr);
		}

		MUI_DisposeObject(o);

	} else {

		if (verptr) {
			*verptr = 0;
		}

		if (revptr) {
			*revptr = 0;
		}
	}
}

GPROTO void MCC_GetNListviewVerRev( ULONG *verptr, ULONG *revptr ) {
	Object *o;

	if ((o = NListviewObject, InputListFrame, End)) {

		if (verptr) {
			get(o, MUIA_Version, verptr);
		}

		if (revptr) {
			get(o, MUIA_Revision, verptr);
		}

		MUI_DisposeObject(o);

	} else {

		if (verptr) {
			*verptr = 0;
		}

		if (revptr) {
			*revptr = 0;
		}
	}
}


GPROTO void MCC_GetSpeedbarVerRev( ULONG *verptr, ULONG *revptr ) {
	Object *o;

	if ((o = SpeedBarObject, InputListFrame, End)) {
		if (verptr) {
			get(o, MUIA_Version, verptr);
		}

		if (revptr) {
			get(o, MUIA_Revision, verptr);
		}

		MUI_DisposeObject(o);

	} else {

		if (verptr) {
			*verptr = 0;
		}

		if (revptr) {
			*revptr = 0;
		}
	}
}

GPROTO LONG GUI_Popup( UBYTE *Title, UBYTE *Body, void *BodyFmt, UBYTE *Gads ) {

	/* Display a requester, with custom text & title. Will default to
	 * intuition.library for its requester if MUIMasterBase has not yet
	 * been initialized.
	 */

	if (SysBase->LibNode.lib_Version < 36) {
		return 0;
	}

	if (MUIMasterBase && App) {

		return MUI_RequestA(App, GETMAINWIN, 0, Title, Gads, Body, BodyFmt);

	} else if (IntuitionBase) {

		/* This fallback code executed when MUIMasterBase is NULL */

		struct EasyStruct EZ = {
			sizeof(struct EasyStruct), 0, NULL, NULL, NULL
		};

		EZ.es_Title        = Title;
		EZ.es_TextFormat   = Body;
		EZ.es_GadgetFormat = Gads;

		/* Note: If window is NULL, then req will default to WB */

		return EasyRequestArgs(NULL, &EZ, NULL, BodyFmt);

	} else if ((IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 36L))) {

		/* This code executed when MUIMasterBase and IntuitionBase aren't valid */

		LONG r;

		struct EasyStruct EZ = {
			sizeof(struct EasyStruct), 0, NULL, NULL, NULL
		};

		EZ.es_Title        = Title;
		EZ.es_TextFormat   = Body;
		EZ.es_GadgetFormat = Gads;

		r = EasyRequestArgs(NULL, &EZ, NULL, BodyFmt);
		CloseLibrary((struct Library *) IntuitionBase);
		IntuitionBase = NULL;

		return r;
	}

	return 0; /* Pre OS 36 people don't see any requesters. :-( */
}

GPROTO void GUI_Iconify( BOOL state ) {
	struct Screen *scr = NULL;

	set(App, MUIA_Application_Iconified, state);

	if (state) {
		return;
	}

	get(GETMAINWIN, MUIA_Window_Screen, &scr);

	if (scr) {
		ScreenToFront(scr);
	}
}

GPROTO void GUI_PrgError( UBYTE *body, APTR bodyfmt ) {
	if (App) {

		DoMethod(GETERRWIN, MUIM_ErrorWin_ReportError, body, bodyfmt);

	} else {

		GUI_Popup("Error", body, bodyfmt, "OK");
	}
}

GPROTO void GUI_PrgDOSError( UBYTE *body, void *bodyfmt ) {
	if (App) {

		DoMethod(GETERRWIN, MUIM_ErrorWin_ReportDOSError, body, bodyfmt, -1);

	} else {

		GUI_Popup("DOS Error", body, bodyfmt, "OK");
	}
}

GPROTO UBYTE *GUI_GetString( UBYTE *title, UBYTE *infostr, APTR infofmt, UBYTE *str ) {
	/* Note: Method MUIM_GetString_GetString will block until Accept/Cancel gadget is pressed.
			 String will remain valid until next call to this method. */
	DoMethod(GETGETSTRWIN, MUIM_GetStringWin_GetString, &str, title, infostr, infofmt);

	return str;
}

GPROTO UBYTE *GUI_GetPassword( UBYTE *InfoStr, APTR InfoFmt ) {

	/* Note: Method MUIM_GetString_GetPassword will block until Accept/Cancel gadget is pressed.
			 String will remain valid until next call to this method. */

	UBYTE *passwd = NULL;
	DoMethod(GETGETSTRWIN, MUIM_GetStringWin_GetPassword, &passwd, "Enter password", InfoStr, InfoFmt);

	return passwd;
}
