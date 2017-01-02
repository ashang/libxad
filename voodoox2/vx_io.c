/*
 $Id: vx_io.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 I/O related functions.

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
#include "vx_io.h"
#include "vx_mem.h"
#include "vx_global.h"
#include "vx_cfg.h"
#include "vx_main.h"
#include "vx_debug.h"
#include "vx_misc.h"

GPROTO BOOL IO_Init( void ) {

	/* Allocate and setup all resources required by the IO module. */

	DB(dbg("Init IO..."))

	return TRUE;
}

GPROTO void IO_Free( void ) {
	/* Clean up all the resources allocated by the IO module. */

	DB(dbg("Free IO..."))

	IO_FreeTmpFileList(TRUE);
}

#define MAX_TMP_FILES		255
#define TMPFILE_TEMPLATE	"$%s%08lx%02lx.%s"
#define TMPDIR_TEMPLATE		"VX%08lx%02lx"
#define PURGECNT			10
#define isDir(fib)			((fib)->fib_DirEntryType >= 0)
#define PATH_LEN			256

ULONG RollCounter;
UBYTE *TmpSlots[MAX_TMP_FILES];

LONG GetFreeTmpSlot( void ) {

	LONG i;

	for (i = 0; i < MAX_TMP_FILES; i++) {

		if (TmpSlots[i] == NULL) {
			return i;
		}
	}

	GUI_Popup("Warning", "Too many temporary files opened!", NULL, "Opps!");

	IO_FreeTmpFileList(FALSE);

	return -1;
}

GPROTO ULONG IO_KillLingeringTemps( void ) {

	/* Delete all temp files left over from the last VX session.
	      Note: We only scan one level deep here. */

	UBYTE path[256];
	BPTR lock, subLock;
	ULONG bytesDeleted = 0;
	struct FileInfoBlock *FIB = NULL, *subFIB = NULL;
	
	DB(UBYTE faultBuf[82])
	
	strncpy(path, (UBYTE *) CFG_Get(TAGID_MAIN_TEMPPATH), sizeof(path)-1);

	if (strlen(path) == 0) {
		strcpy(path, "T:");
	}

	if ((lock = Lock(path, SHARED_LOCK)) && (FIB = AllocDosObject(DOS_FIB, 0)) &&
	        (subFIB = AllocDosObject(DOS_FIB, 0)) && Examine(lock, FIB)) {

		while (ExNext(lock, FIB)) {

			if ((strncmp("VX", FIB->fib_FileName, 2) != 0) &&
			        (strncmp("$VX", FIB->fib_FileName, 3) != 0)) {
				continue;
			}

			if (!AddPart(path, FIB->fib_FileName, sizeof(path)-1)) {
				continue;
			}

			if ((subLock = Lock(path, SHARED_LOCK)) &&
			        Examine(subLock, subFIB) && subFIB->fib_DirEntryType > 0) {

				DB(dbg("Found a lingering temp dir : %s", path))

				while (ExNext(subLock, subFIB)) {

					if (subFIB->fib_DirEntryType > 0) {
						continue; /* We're only looking for files... */
					}

					if (!AddPart(path, subFIB->fib_FileName, sizeof(path)-1)) {
						continue;
					}

					bytesDeleted += subFIB->fib_Size;

					DB(dbg("Found a lingering temp file: %s", path))

					/*SetProtection(path, 0);*/

					if (DeleteFile(path)) {
						DB(dbg("Deleted lingering temp file: %s", path))
					} else {
						DB(if (Fault(IoErr(), "DOS error ", faultbuf, sizeof(faultbuf)-1)))
						DB(dbg("WARNING: Unable to delete lingering temp file: %s (%s)", path, faultbuf))
					}
					PathPart(path)[0] = 0;
				}

				/*SetProtection(path, 0);*/

				/* Now delete unlock and del the dir. BUG FIXED: Tue/2/Apr/2002 */

				UnLock(subLock);
				subLock = 0;

				if (DeleteFile(path)) {

					DB(dbg("Deleted lingering temp dir : %s", path))
				} else {

					DB(if (Fault(IoErr(), "DOS error ", faultbuf, sizeof(faultBuf)-1)))
					DB(dbg("WARNING: Unable to delete lingering temp dir : %s (%s)", path, faultBuf))
				}

			} else {
				DB(dbg("Found a lingering temp file : %s", path))

				SetProtection(path, 0);

				if (DeleteFile(path)) {

					DB(dbg("Deleted lingering temp file: %s", path))
				} else {

					DB(if (Fault(IoErr(), "DOS error ", faultbuf, sizeof(faultBuf)-1)))
					DB(dbg("WARNING: Unable to delete lingering temp file: %s (%s)", path, faultBuf))
				}
			}

			if (subLock) {
				UnLock(subLock);
			}

			PathPart(path)[0] = 0;
		}
	}

	if (lock) {
		UnLock(lock);
	}

	if (subFIB) {
		FreeDosObject(DOS_FIB, subFIB);
	}

	if (FIB) {
		FreeDosObject(DOS_FIB, FIB);
	}

	DB(dbg("Delete %lu bytes worth of lingering temps...", bytesDeleted))

	return bytesDeleted;
}

GPROTO BOOL IO_RecursiveDelete( struct AnchorPath *ap, UBYTE *dirName ) {
	LONG mr;
	BOOL success = TRUE;

	if (!(ap = MEM_AllocVec(sizeof(struct AnchorPath) + PATH_LEN))) {
		return FALSE;
	}

	ap->ap_Strlen = PATH_LEN;

	for (mr = MatchFirst(dirName, ap); mr == 0; mr = MatchNext(ap)) {

		if (isDir(&ap->ap_Info)) {
			/**** Directory ****/

			if (ap->ap_Flags & APF_DIDDIR) {
				ap->ap_Flags &= ~APF_DIDDIR;
			}

			ap->ap_Flags |= APF_DODIR;

		} else {
			/**** File ****/

			SetProtection(ap->ap_Buf, 0);

			if (!DeleteFile(ap->ap_Buf)) {

				UBYTE *fmt = ap->ap_Buf;

				GUI_PrgDOSError(
				    "Failed to delete file \"%s\", recursive delete not fully successful!", &fmt);

				success = FALSE;
			}
		}
	}

	MEM_FreeVec(ap);

	return success;
}

GPROTO UBYTE *IO_GetTmpFileName( UBYTE *BaseName, UBYTE *Suffix ) {
	/*
	 * TODO: Make this obsolete, use IO_GetTmp instead.
	 *
	 * Get a temporary filename. BaseName is used to construct part of the
	 * filename, don't use characters that would be considered illegal in
	 * a filename.
	 *
	 * Example: T:$<basename>1234567812.lzx
	 *
	 */

	struct DateStamp ds;
	LONG i;
	UBYTE filename[128], path[256], *tp = (UBYTE *) CFG_Get(TAGID_MAIN_TEMPPATH);

	if (RollCounter % PURGECNT == 0) {
		IO_FreeTmpFileList(FALSE);
	}

	if ((i = GetFreeTmpSlot()) == -1) {
		return NULL;
	}

	if (!BaseName) {
		BaseName = "$VX";
	}

	if (!Suffix) {
		Suffix = "tmp";
	}

	if (tp) {
		strncpy(path, tp, sizeof(path)-1);
	} else {
		strcpy(path, "T:");
	}

	DateStamp(&ds);

	sprintf(filename, TMPFILE_TEMPLATE, BaseName,
	        (ULONG) VXProcess + ds.ds_Days + ds.ds_Minute + ds.ds_Tick,
	        (++RollCounter), Suffix);

	if (!AddPart(path, filename, sizeof(path)-1)) {
		return NULL;
	}

	DB(dbg("IO_GetTmpFileName: returned: %s\n", path))

	return TmpSlots[i] = MEM_StrToVec(path);
}

GPROTO UBYTE *IO_GetTmpDir( UBYTE *BaseName ) {

	struct DateStamp ds;
	LONG i;
	UBYTE dirname[128], path[256], *tp = (UBYTE *) CFG_Get(TAGID_MAIN_TEMPPATH);

	if (RollCounter % PURGECNT == 0) {
		IO_FreeTmpFileList(FALSE);
	}

	if ((i = GetFreeTmpSlot()) == -1) {
		return NULL;
	}

	if (tp) {
		strcpy(path, tp);
	} else {
		strcpy(path, "T:");
	}

	DateStamp(&ds);

	sprintf(dirname, TMPDIR_TEMPLATE,
	        ds.ds_Days + ds.ds_Minute + ds.ds_Tick + (ULONG) VXProcess,
	        (++RollCounter));

	if (!AddPart(path, dirname, sizeof(path)-1)) {
		return NULL;
	}

	UnLock(CreateDir(path));

	DB(dbg("IO_GetTmpDir returned: %s\n", path))

	return TmpSlots[i] = MEM_StrToVec(path);
}

GPROTO UBYTE *IO_GetTmpLocation( UBYTE *filename ) {

	/*
	 * TODO: Rename this to IO_GetTmp()
	 *
	 * Example: T:VX1234567812/<filename>
	 *
	 */

	struct DateStamp ds;
	LONG i;
	UBYTE dirname[128], path[256];
	UBYTE *tp = (UBYTE *) CFG_Get(TAGID_MAIN_TEMPPATH);

	if (RollCounter % PURGECNT == 0) {
		IO_FreeTmpFileList(FALSE);
	}

	if ((i = GetFreeTmpSlot()) == -1) {
		return NULL;
	}

	if (!filename) {
		filename = "temp.dat";
	}

	if (tp) {
		strcpy(path, tp);
	} else {
		strcpy(path, "T:");
	}

	DateStamp(&ds);

	sprintf(dirname, TMPDIR_TEMPLATE,
	        ds.ds_Days + ds.ds_Minute + ds.ds_Tick + (ULONG) VXProcess, (++RollCounter));

	if (!AddPart(path, dirname, sizeof(path)-1)) {
		return NULL;
	}

	UnLock(CreateDir(path));

	if (!AddPart(path, filename, sizeof(path)-1)) {
		SetProtection(path, 0);
		DeleteFile(path);
		return NULL;
	}

	DB(dbg("IO_GetTmpLocation returned: %s\n", path))

	return TmpSlots[i] = MEM_StrToVec(path);
}

GPROTO void IO_DeleteTmpFile( UBYTE *TmpFileName ) {

	LONG i, ok;

	if (!TmpFileName) {
		return;
	}

	for (i = 0; i < MAX_TMP_FILES; i++) {

		if (Stricmp(TmpFileName, TmpSlots[i]) != 0) {
			continue;
		}

		SetProtection(TmpFileName, 0);

		ok = DeleteFile(TmpFileName);

		if (ok || (!ok && IoErr() == ERROR_OBJECT_NOT_FOUND)) {

			// If file/dirname starts with a dollar symbol then it was
			// created with IO_GetTmpFileName() and doesn't reside inside
			// a wrapper dir. Which means we don't need to delete it.

			if (*FilePart(TmpSlots[i]) != '$') {
				*PathPart(TmpSlots[i]) = 0;
				SetProtection(TmpSlots[i], 0);
				if (!DeleteFile(TmpSlots[i])) {
					// If this fails then a dir will be lingering...
				}
			}
			MEM_FreeVec(TmpSlots[i]);
			TmpSlots[i] = NULL;
			return;
		}
		DB(dbg("DEBUG: Cannot free tmp %s : it's in use by something!!!!\n", TmpFileName))
	}
}

#define BUFSIZE 2000

GPROTO ULONG IO_FreeTmpFileList( BOOL Quitting ) {

	ULONG i, r, failcnt;
	UBYTE *vec, faultbuf[82];

RETRY:

	if ((vec = MEM_AllocVec(BUFSIZE))) {
		strncat(vec, "Unable to delete the following %lu temporary file(s)...\n\n", BUFSIZE-1);
	}

	for (failcnt = 0, i = 0; i < MAX_TMP_FILES; i++) {

		if (TmpSlots[i] == NULL) {
			continue;
		}

		DB(dbg("Deleting temp file: %s ", TmpSlots[i]))

		SetProtection(TmpSlots[i], 0);

		if ((r = DeleteFile(TmpSlots[i]))) {

			/* The temp file was deleted OK, now delete the dir it resides in. */

			DB(dbg("(success)\n"))

			PathPart(TmpSlots[i])[0] = 0; // Now delete the directory...
			SetProtection(TmpSlots[i], 0);
			DeleteFile(TmpSlots[i]);

			// Free the slot
			MEM_FreeVec(TmpSlots[i]);
			TmpSlots[i] = NULL;

		} else {

			DB(dbg("(failed)\n"))

			++failcnt;

			if (vec && IoErr() != ERROR_OBJECT_NOT_FOUND) {

				strncat(vec, TmpSlots[i], BUFSIZE-1);

				if (Fault(IoErr(), "DOS error ", faultbuf, sizeof(faultbuf)-1)) {
					strncat(vec, " (", BUFSIZE-1);
					strncat(vec, faultbuf, BUFSIZE-1);
					strncat(vec, ")", BUFSIZE-1);
				}

				strncat(vec, "\n", BUFSIZE-1);
			}

			if (IoErr() == ERROR_OBJECT_NOT_FOUND) {
				--failcnt;
			}
		}
	}

	if (Quitting) {
		if (failcnt > 0) {

			ULONG r = GUI_Popup("Notice",
			                    (vec ? vec : (UBYTE *) "Some temporary files were not deleted!"),
			                    &failcnt, "Retry|Quit");

			if (r == 1) {

				/* Retry */
				MEM_FreeVec(vec);
				goto RETRY;
			}
		}

		/* Since we quitting, we must free all the record slots... */

		for (i = 0; i < MAX_TMP_FILES; i++) {

			if (TmpSlots[i]) {
				MEM_FreeVec(TmpSlots[i]);
			}

			TmpSlots[i] = NULL;

		}
	}

	/* If we're not quitting out of the program, then don't show the requester. */

	MEM_FreeVec(vec);

	return failcnt;
}

GPROTO BOOL IO_CopyFile( UBYTE *SrcName, UBYTE *DestName, BOOL progress ) {
	/*
	 * Simple copyfile routine that copies in chunks of 64KB. Will return
	 * TRUE on success.
	 */

	BOOL Result = FALSE, UserBreak = FALSE;
	APTR CpyBuf;

	if (progress) {
		GUI_PrintStatus(STR(SID_COPYFILE, "Please wait, copying file data..."), 0);
	}

	if ((CpyBuf = MEM_AllocVec(COPYFILE_CHUNKSIZE))) {

		BPTR InFile, OutFile = 0;
		LONG RL = -1, Len, BytesLeft, SoFar;

		if ((InFile = Open(SrcName, MODE_OLDFILE)) &&
		        (OutFile = Open(DestName, MODE_NEWFILE)) &&
		        (Seek(InFile, 0, OFFSET_END) != -1) &&
		        ((Len = Seek(InFile, 0, OFFSET_BEGINNING)) != -1)) {

			if (progress) {
				OpenProgress((ULONG) Len, "Copying file data...");
			}

			for(;;) {
				if ((SoFar = Seek(InFile, 0, OFFSET_CURRENT)) != -1) {

					BytesLeft = Len - SoFar;

					if (progress && UpdateProgress("Copying file data, %lu bytes left...", &BytesLeft, SoFar)) {

						/* Aborted */
						UserBreak = TRUE;
						break;
					}
				} else if (progress && App) {

					DoMethod(App, MUIM_Application_InputBuffered);

				}

				RL = Read(InFile, CpyBuf, COPYFILE_CHUNKSIZE);

				if (RL == 0 || RL == -1) {
					break;
				}

				if (Write(OutFile, CpyBuf, RL) == -1) {
					break;
				}

				if (UserBreak) {
					break;
				}
			}

			if (UserBreak) {
				Result = FALSE;
			} else if (RL == 0) {
				Result = TRUE;
			}

			if (progress) {
				CloseProgress;
			}
		}

		if (InFile) {
			Close(InFile);
		}

		if (OutFile) {
			Close(OutFile);
		}

		MEM_FreeVec(CpyBuf);
	} else {

		SetIoErr(ERROR_NO_FREE_STORE);
	}
	if (UserBreak) {

		DeleteFile(DestName);

		if (progress) {
			GUI_PrintStatus("Aborted.", 0);
		}

		SetIoErr(ERROR_BREAK);
	} else if (!Result) {

		DeleteFile(DestName);

		if (progress) {
			GUI_PrintStatus(STR(SID_COPYFILE_FAILED, "Error while copying file data!"), 0);
		}
	} else {
		struct FileInfoBlock *FIBVec;

		if ((FIBVec = IO_GetFIBVec(SrcName))) {
			SetComment(DestName, (UBYTE *) &FIBVec->fib_Comment);
			MEM_FreeVec(FIBVec);
		}

		if (progress) {
			GUI_PrintStatus(STR(SID_COPYFILE_OK, "Finished copying file data."), 0);
		}
	}

	return Result;
}

GPROTO struct FileInfoBlock *IO_GetFIBVec( UBYTE *FilePath ) {

	/*
	 * Load the FileInfoBlock of the specified file into a memory vector.
	 * This vector should eventually be freed with MEM_FreeVec() when not
	 * in use.
	 */

	struct FileInfoBlock *FIBVec;
	BOOL success = FALSE;

	if ((FIBVec = MEM_AllocVec(sizeof(struct FileInfoBlock)))) {
		register BPTR testLock;

		if ((testLock = Lock(FilePath, SHARED_LOCK))) {

			if (Examine(testLock, FIBVec)) {
				success = TRUE;
			}

			UnLock(testLock);
		}
	}

	if (!success) {
		MEM_FreeVec(FIBVec);
		FIBVec = NULL;
	}

	return FIBVec;
}

GPROTO BOOL IO_ConfirmOverwrite( UBYTE *OverwritePath ) {

	/*
	 * Ask the use if the specified file can be overwritten. TRUE will be
	 * returned it can. If the file dosn't exist then TRUE will be
	 * returned anyway and the user will not see a requester.
	 *
	 * FALSE will always be returned in the event of an error.
	 *
	 * TODO: Check if file is delete protected, if so, prompt user if he/she
	 * wants to overwrite it anyway.
	 */

	BPTR Lk;
	BOOL KillIt = FALSE;
	struct FileInfoBlock *FIB;

	if ((Lk = Lock(OverwritePath, SHARED_LOCK))) {
		UnLock(Lk);

		if (GUI_Popup(
		            STR(SID_OVERWRITE_REQUEST, "Overwrite request..."),
		            STR(SID_OVERWRITE_BODY,    "File %s already exists, overwrite it?"), &OverwritePath,
		            STR(SID_OVERWRITE_GADGETS, "Yes, overwrite it|No, don't overwrite it")) == 1) {

			/* User pressed Yes */

			if ((FIB = IO_GetFIBVec(OverwritePath))) {
				if (FIB->fib_Protection & FIBF_DELETE) {

					/* File is protected. */

					if (GUI_Popup("File is protected...",
					              "File %s is delete protected, remove protection?",
					              &OverwritePath,
					              "Yes, remove protection|No, don't remove protection") == 1) {

						/* Yes */
						SetProtection(OverwritePath, FIB->fib_Protection & ~FIBF_DELETE);
						KillIt = TRUE;
					} else {

						/* No */
						KillIt = FALSE;
					}
				} else {
					KillIt = TRUE; /* File is not protected. */
				}

				MEM_FreeVec(FIB);
			} else {

				KillIt = FALSE; /* Error */
			}
		} else {

			KillIt = FALSE;
		}
	} else if (IoErr() == ERROR_OBJECT_NOT_FOUND) {

		KillIt = TRUE;
	}

	return KillIt;
}

GPROTO APTR IO_LoadFileToVec( UBYTE *FileName, ULONG *LengthPtr ) {

	/*
	 * Load a file to a memory vector. The resulting file must be freed
	 * with MEM_FreeVec() when not needed anymore.
	 *
	 * Use the LengthPtr parameter to pass the pointer of a ULONG that
	 * will store the size of the loaded file. This value will only be
	 * valid if this routine returns a non-NULL result. If you don't
	 * need the size, then pass NULL to LengthPtr.
	 */

	register APTR FileVec = NULL;
	register struct FileInfoBlock *FIB;
	register BPTR InFile;

	if ((FIB = IO_GetFIBVec(FileName)) && FIB->fib_Size) {

		if (LengthPtr) {
			*LengthPtr = FIB->fib_Size;
		}

		if ((FileVec = MEM_AllocVec(FIB->fib_Size))) {

			if ((InFile = Open(FileName, MODE_OLDFILE))) {

				if (Read(InFile, FileVec, FIB->fib_Size) == -1) {
					MEM_FreeVec(FileVec);
					FileVec = NULL;
				}

				Close(InFile);
			}
		}

		MEM_FreeVec(FIB);
	}

	return FileVec;
}

GPROTO BOOL IO_CopyIcon( UBYTE *SrcPath, UBYTE *DestPath, BOOL DeleteSrc ) {

	/*
	 * Copy an icon to somewhere. Don't include the ".info" extension in
	 * the paths. If DeleteSrc is TRUE then the SrcPath icon will be
	 * deleted.
	 */

	BOOL Success = FALSE;
	struct DiskObject *DiskObj;

	if (!SrcPath || !DestPath) {
		return FALSE;
	}

	if ((DiskObj = GetDiskObject(SrcPath))) {

		if (PutDiskObject(DestPath, DiskObj)) {
			Success = TRUE;
		}

		FreeDiskObject(DiskObj);
	}

	if (DeleteSrc && Success) {
		DeleteDiskObject(SrcPath);
	}

	return Success;
}

GPROTO BOOL IO_SameFile( UBYTE *File1, UBYTE *File2 ) {

	BPTR Lock1, Lock2;
	BOOL Result = FALSE;

	if ((Lock1 = Lock(File1, SHARED_LOCK))) {
		if ((Lock2 = Lock(File2, SHARED_LOCK))) {
			if (SameLock(Lock1, Lock2) == LOCK_SAME) {
				Result = TRUE;
			}
			UnLock(Lock2);
		}
		UnLock(Lock1);
	}

	return Result;
}

GPROTO BOOL IO_MyCreateDir( UBYTE *DirPath ) {

	/*
	 * Create a directory. This routine will create all the sub-
	 * directories that are in the path, if they don't exist.
	 *
	 * NOTE: This routine modifies the DestPath string (but restores it).
	 */

	register BOOL success = TRUE;
	register BPTR lock;
	register UBYTE ch;
	register ULONG i = 0;

	while (DirPath[i] && success) {

		for (; DirPath[i] && DirPath[i] != '/'; ++i)
			;

		ch = DirPath[i];

		DirPath[i] = 0;

		if ((lock = Lock(DirPath, SHARED_LOCK))) {
			UnLock(lock);
		}
		else if ((lock = CreateDir(DirPath))) {
			UnLock(lock);
		}
		else {
			success = FALSE;
		}

		DirPath[i++] = ch;
	}

	return success;
}

GPROTO BOOL IO_SaveFile( UBYTE *destname, APTR filebuf, ULONG filelen ) {

	BOOL success = FALSE;
	BPTR outfh;

	if ((outfh = Open(destname, MODE_NEWFILE))) {

		if (Write(outfh, filebuf, filelen) == filelen) {
			success = TRUE;
		}

		Close(outfh);
	}

	if (!success) {
		DeleteFile(destname);
	}

	return success;
}

GPROTO UBYTE *IO_GetSuffix( UBYTE *Filename ) {

	UBYTE *suffix;

	if (!Filename) {
		return NULL;
	}

	suffix = strrchr(Filename, '.');

	if (suffix) {
		return ++suffix;
	} else {
		return NULL;
	}
}

