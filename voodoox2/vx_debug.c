/*
 $Id: vx_debug.c,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Debug support functions.

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

#define VX_DEBUG_C

#include "vx_include.h"

#define DBCONWINDOW "CON:0/11/900/200/VX_debug_window/CLOSE"

BPTR ConFH;
BOOL ActivateDebug = TRUE;

void dbg_ShowHide( void );
void dbg_Init( void );
void dbg_Free( void );
void dbg( UBYTE *FmtStr, APTR FmtStream, ... );

void dbg_ShowHide( void ) {
	/* Open or close the main debug console window. */

#ifdef DEBUG

	if (ConFH) {
		dbg_Free();
	} else {
		dbg_Init();
	}

#endif /* DEBUG */

}

void dbg_Init( void ) {
	/* This routine is used to open the debug console window. */
#ifdef DEBUG

	if (!DOSBase) {
		return;
	}

	if (!ActivateDebug) {
		return;
	}

	dbg_Free(); /* For a restart */
	ConFH = Open(DBCONWINDOW, MODE_NEWFILE);

	if (ConFH) {
		dbg("%s debug console\n\n", VERS);
	}

#endif /* DEBUG */
}

void dbg_Free( void ) {
	/* This routine will close the debug console window. */

#ifdef DEBUG

	if (!DOSBase || !ActivateDebug) {
		return;
	}

	if (ConFH) {
		Close(ConFH);
		ConFH = 0;
	}

#endif
}

void dbg( UBYTE *FmtStr, APTR FmtStream, ... ) {

	/*
	 * Print some text to the console window. If the string has no new-
	 * line at the end of it, a newline char '\n' will be printed after
	 * the string, to the console window.
	 *
	 * If the console window is currently closed, then the print operation
	 * is ignored.
	 */

#ifdef DEBUG

	if (!DOSBase || !ActivateDebug) {
		return;
	}

	if (ConFH) {
		VFPrintf(ConFH, FmtStr, &FmtStream);

		/* This saves us from having to add a newline :) */

		if (FmtStr[strlen(FmtStr) - 1] != '\n') {
			FPrintf(ConFH, "\n");
		}
	}

#endif /* DEBUG */
}

void X( void ) {

	/* Used for tracing / debugging. */

}




