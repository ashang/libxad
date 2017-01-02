/*
 $Id: xp_protos.h,v 1.2 2004/01/25 04:21:37 andrewbell Exp $
 XProto's own proto file.

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

/* This proto file was generated on Sunday 28-Dec-03 14:17:19 */

#ifndef XP_PROTOS_H
#define XP_PROTOS_H 1

#ifndef GPROTO
#define GPROTO
#endif /* GPROTO */

#ifndef LPROTO
#define LPROTO
#endif /* LPROTO */


/*
 * Global prototypes for module xp_main.c
 *
 * Auto-generated with XProto 1.3 by Andrew Bell
 *
 */

GPROTO BOOL XPROTO_BuildLists( FILE *InFH );
GPROTO unsigned char *XPROTO_ExtractProtoLine( unsigned char *FirstLine, FILE *SrcFH );
GPROTO void XPROTO_FreeLists( void );
GPROTO BOOL XPROTO_SaveList( FILE *DestFH, long xpID, unsigned char *ModuleName );
GPROTO BOOL OpenProtoFiles( void );
GPROTO void CloseProtoFiles( void );
GPROTO void SaveHeader( FILE *OutFH, unsigned char * filename );
GPROTO void SaveFooter( FILE *OutFH, unsigned char * filename );
GPROTO unsigned char *TrimString( unsigned char *Str );
GPROTO unsigned char *callocstr( unsigned char *str );

/*
 * Local prototypes for module xp_main.c
 *
 * Auto-generated with XProto 1.3 by Andrew Bell
 *
 */

LPROTO BOOL InitPrg( int argc, char *argv[] );
LPROTO void EndPrg( void );
LPROTO void DoPrg( void );
LPROTO void DoErr( void );


#endif /* XP_PROTOS_H */
