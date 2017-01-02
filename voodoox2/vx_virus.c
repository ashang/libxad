/*
 $Id: vx_virus.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Contains XVS virus identification code.

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
#include "vx_virus.h"
#include "vx_arc.h"
#include "vx_io.h"
#include "vx_mem.h"
#include "vx_global.h"
#include "vx_debug.h"
#include "vx_main.h"

GPROTO UBYTE *VIRUS_CheckFile( UBYTE *fileName ) {

	APTR fileVec;
	ULONG fileLen = 0;
	UBYTE *virusName = NULL;

	struct xvsFileInfo *xvsFI;

	DB(dbg("VIRUS_CheckFile(filename=\"%s\")", fileName))

	if (!xvsBase) {
		return NULL;
	}

	if ((fileVec = IO_LoadFileToVec(fileName, &fileLen))) {

		DB(dbg("VIRUS_CheckFileForVirus: FileVec = %lx, FileLen = %lu", fileVec, fileLen));

		if ((xvsFI = xvsAllocObject(XVSOBJ_FILEINFO))) {

			xvsFI->xvsfi_File    = fileVec;
			xvsFI->xvsfi_FileLen = fileLen;

			switch (xvsCheckFile(xvsFI)) {

			case XVSFT_DATAVIRUS:
			case XVSFT_FILEVIRUS:
			case XVSFT_LINKVIRUS:
				virusName = xvsFI->xvsfi_Name;
				break;

			default:
				virusName = NULL;
				break;
			}

			xvsFreeObject(xvsFI);
		}

		MEM_FreeVec(fileVec);
	} else {

		GUI_PrgDOSError("Couldn't load %s to memory for virus checking!", &fileName);

	}

	return virusName;
}

#ifdef DEBUG
UBYTE *GetXVSType( ULONG type ) {

	switch (type) {
	case XVSFT_EMPTYFILE:
		return "XVSFT_EMPTYFILE";
	case XVSFT_DATAFILE:
		return "XVSFT_DATAFILE";
	case XVSFT_EXEFILE:
		return "XVSFT_EXEFILE";
	case XVSFT_DATAVIRUS:
		return "XVSFT_DATAVIRUS";
	case XVSFT_FILEVIRUS:
		return "XVSFT_FILEVIRUS";
	case XVSFT_LINKVIRUS:
		return "XVSFT_LINKVIRUS";
	}

	return "Unknown";
}
#endif

GPROTO UBYTE *VIRUS_CheckMem( APTR fileBuf, ULONG fileLen ) {

	// TODO: Add a BOOL option here called Destroy that'll allow XVS
	//       to modify the buffer. Removing the need for the slow
	//       copy...

	UBYTE *virusName = NULL, *tempBuf;
	struct xvsFileInfo *xvsFI;
	ULONG t;

	DB(dbg("VIRUS_CheckMem(filebuf=0x%08lx, filelen=%lu)", fileBuf, fileLen))

	if (!xvsBase) {
		return NULL;
	}

	// XVS might modify the buffer, so take a copy of it first...

	if (!(tempBuf = MEM_AllocVec(fileLen))) {
		return NULL;
	}

	CopyMem(fileBuf, tempBuf, fileLen);

	if ((xvsFI = xvsAllocObject(XVSOBJ_FILEINFO))) {

		xvsFI->xvsfi_File = tempBuf;
		xvsFI->xvsfi_FileLen = fileLen;

		switch ((t = xvsCheckFile(xvsFI))) {
			/* Harmless */
		default:
		case XVSFT_EMPTYFILE:
		case XVSFT_DATAFILE:
		case XVSFT_EXEFILE:
			virusName = NULL;
			break;

			/* Virus was found */
		case XVSFT_DATAVIRUS:
		case XVSFT_FILEVIRUS:
		case XVSFT_LINKVIRUS:
			virusName = xvsFI->xvsfi_Name;
			break;
		}

		xvsFreeObject(xvsFI);
	}

	if (tempBuf) {
		MEM_FreeVec(tempBuf);
	}

	DB(dbg("xvsCheckFile() returned %s (name=%s)", GetXVSType(t),
	       xvsfi->xvsfi_Name ? xvsfi->xvsfi_Name : "?????"))

	return virusName;
}

GPROTO void VIRUS_ShowWarning( AHN *ahn, AE *ae, UBYTE *virusName ) {

	UBYTE *bodyFmt[3];

	if (!ahn || !ae || !virusName) {
		return;
	}

	bodyFmt[0] = ae->ae_xfi->xfi_FileName;
	bodyFmt[2] = virusName;

	if (ahn->ahn_EmbeddedArc) {
		bodyFmt[1] = ahn->ahn_EmbeddedListStr;
	} else {
		bodyFmt[1] = ahn->ahn_Path;
	}

	GUI_Popup("Virus warning...",
	          "xvs.library has detected a virus/trojan!\n" "\n"
	          "The file: %s\n"
	          "In archive: %s\n"
	          "\n" "Contains the '%s' virus/trojan!!!\n" "\n"
	          "Please take the necessary precautions with this archive.", &bodyFmt,
	          "Understood");
}

