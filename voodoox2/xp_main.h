/*
 $Id: xp_main.h,v 1.2 2004/01/25 04:21:37 andrewbell Exp $
 Main definitions for XProto.

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

#ifndef XP_MAIN_H
#define XP_MAIN_H

typedef long BOOL;
#define TRUE 1
#define FALSE 0

#define GPROTO
#define LPROTO
//#define ARGPLATE      "SRCFILES/M,LOCAL=LOCALFILE/K,GLOBAL=GLOBALFILE/K,BOTH=BOTHFILES/K,PROTODIR/K,NOPROTOSUFFIX/S"
#define TMPSTRVEC_LEN 5000
#define INBUFLEN      (1024*4)
#define IOBUFSIZE     (32*1024)

#define MAX_SRC_FILES 1000

struct ArgLayout
{
	unsigned char  *al_SRCFILES[MAX_SRC_FILES];
	unsigned char   al_LOCAL[256];
	unsigned char   al_GLOBAL[256];
	unsigned char   al_BOTH[256];
	unsigned char   al_PROTODIR[256];
	long            al_NOPROTOSUFFIX;
};

enum 
{
	XPROTO_BOTH = 0,
	XPROTO_LOCAL,
	XPROTO_GLOBAL,
	XPROTO_PROTODIR
};

#define YEAR       "2000-2004"
#define STR_GPROTO "GPROTO"
#define STR_LPROTO "LPROTO"

#endif /* XP_MAIN_H */
