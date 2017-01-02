/*
 $Id: xp_main.c,v 1.3 2004/01/25 04:21:37 andrewbell Exp $
 XProto - Tool for extracting prototypes from C sources.

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

/* Created: Sun/9/Jan/2000 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "xp_rev.h"
#include "xp_main.h"
#include "xp_protos.h"
#include "gc_lists.h"		// We borrow list functions from GenClasses

// Useful for compilers that can't handle comments inside strings.
#define OP_COM "/" "*"
#define CL_COM "*" "/"

const unsigned char *VTag = VERSTAG " Copyright © " YEAR " Andrew Bell. All rights reserved.";
FILE *LocalFH, *GlobalFH, *BothFH;
unsigned char LocalFH_Name[256], GlobalFH_Name[256], BothFH_Name[256];
struct ArgLayout AD;
struct list GlobalList, LocalList;
unsigned long LocalCnt, GlobalCnt;

int myToLower( int ch )
{
	if ((unsigned int) (ch - 'A') < 26u)
		return ch + 'a' - 'A';

	return ch;
}

long myStrnicmp( unsigned char *s1, unsigned char *s2, long len )
{
	int x, y;

	if (len == 0) return 0;

	while ((x = myToLower(*s1++)) == (y = myToLower(*s2++)) && x && --len);
		;

	return x - y;
}

char *SkipWhiteSpaces( unsigned char *p )
{
	unsigned char c;

	while ((c = *p))
	{
		if (c == '\t' || c == '\r' || c == '\n' || c == ' ' || c == 0xa0)
			++p;
		else
			break;
	}

	return p;
}

long GetStrArg(
	unsigned char *argKeyword, int argc, char *argv[], long *indexPtr,
	unsigned char *dest, long destLen )
{
	unsigned char *p;
	unsigned char *argLine = argv[*indexPtr];
	size_t argKeywordLen = strlen(argKeyword);

	if (strncmp(argKeyword, argLine, argKeywordLen) != 0)
		return 0;

	// Jump past argument keyword (i.e. --foo) and any
	// white spaces that may follow.

	p = SkipWhiteSpaces(&argLine[argKeywordLen]);

	if (*p == 0)
	{
		// First byte is 0 which means empty string, thus no data!
		// Looks like there's a space between arg and its data:
		// i.e. "--foo data" This means the data is situated in
		// the next slot on the array.

		if (*indexPtr == argc)
		{
			p = "";		// We're at the end of the array!
		}
		else
		{
			// Take the data in next slot then make our caller skip
			// the next slot by increasing the index (pointed to
			// by indexPtr). Also skip any white spaces that might
			// sit before the argument data.

			p = SkipWhiteSpaces(argv[++(*indexPtr)]);
		}
	}

	strncpy(dest, p, destLen);

	//printf("DEBUG: arg keyword='%s' arg data='%s'\n", argKeyword, dest);

	return 1;
}

long GetBlnArg(
	unsigned char *argKeyword, int argc, char *argv[], long *indexPtr,
	long *blnLongPtr )
{
	unsigned char *argLine = argv[*indexPtr];

	if (strncmp(argKeyword, argLine, strlen(argKeyword)) != 0)
		return 0;

	return *blnLongPtr = 1;
}

BOOL ReadArgs( int argc, char *argv[] )
{
	long i, j;

	if (argc <= 1)
	{
		printf(
		"Usage: %s <options> sources...\n"
		"\n"
		"-l  --localfile\n"
		"\n"
		"Use this parameter to specify the destination header file that will store local prototypes.\n"
		"\n"
		"\n"
		"-l  --globalfile\n"
		"\n"
		"Use this to specify the destination header file for global prototypes.\n"
		"\n"	
		"\n"
		"-b  --bothfile\n"
		"\n"
		"If you want local and global prototypes in the same header use this.\n"
		"\n"
		"-pd --protodir\n"
		"\n"
		"Write all protos to this dir."
		"\n"
		"-nps --noprotosuffix\n"
		"\n"
		"Don't append \"*_protos.h\" to the end of generated filenames."
		,
		argv[0]
		);
		return FALSE;
	}

	//  "SRCFILES/M,LOCAL=LOCALFILE/K,GLOBAL=GLOBALFILE/K,BOTH=BOTHFILES/K,PROTODIR/K,NOPROTOSUFFIX/S"

	memset(&AD, 0, sizeof(struct ArgLayout));

	for (i = 1, j = 0; i < argc; i++)
	{
		if (GetStrArg("-l",              argc, argv, &i, AD.al_LOCAL,     sizeof(AD.al_LOCAL   )-1)) continue;
		if (GetStrArg("--localfile",     argc, argv, &i, AD.al_LOCAL,     sizeof(AD.al_LOCAL   )-1)) continue;
		if (GetStrArg("-g",              argc, argv, &i, AD.al_GLOBAL,    sizeof(AD.al_GLOBAL  )-1)) continue;
		if (GetStrArg("--globalfile",    argc, argv, &i, AD.al_GLOBAL,    sizeof(AD.al_GLOBAL  )-1)) continue;
		if (GetStrArg("-b",              argc, argv, &i, AD.al_BOTH,      sizeof(AD.al_BOTH    )-1)) continue;
		if (GetStrArg("--bothfile",      argc, argv, &i, AD.al_BOTH,      sizeof(AD.al_BOTH    )-1)) continue;
		if (GetStrArg("-pd",             argc, argv, &i, AD.al_PROTODIR,  sizeof(AD.al_PROTODIR)-1)) continue;
		if (GetStrArg("--protodir",      argc, argv, &i, AD.al_PROTODIR,  sizeof(AD.al_PROTODIR)-1)) continue;
		if (GetBlnArg("-nps",            argc, argv, &i, &AD.al_NOPROTOSUFFIX                     )) continue;
		if (GetBlnArg("--noprotosuffix", argc, argv, &i, &AD.al_NOPROTOSUFFIX                     )) continue;

		// Anything that doesn't look like an argument we'll treat as a source file.
		// To make this really bulletproof we could add suffix checking here.

		if (j < MAX_SRC_FILES - 1)
			AD.al_SRCFILES[j++] = argv[i];
		else
			printf("WARNING: Too many arguments. Source file %s will be ignored.\n", argv[i]);
	}

	// Terminate the array with a NULL...

	AD.al_SRCFILES[j++] = NULL;

	if (!AD.al_LOCAL[0] && !AD.al_GLOBAL[0] && !AD.al_BOTH[0] && !AD.al_PROTODIR[0])
	{
		printf(
			"Please provide one or more of the following args:\n"
			"\t--localfile, --globalfile, --bothfile OR --protodir.\n"
		);

		return FALSE;
	}

	return TRUE;
}

char *MyFilePart( char *path )
{
	char *p;
	if (!path) return NULL;
	if ((p = strrchr(path, '/'))) return ++p;
	if ((p = strrchr(path, ':'))) return ++p;
	return path;
}

long MyAddPart( char *path, char *name, long len )
{
	/* A version of dos.library/AddPart() that should
	   work on both UNIX and Amiga systems. */

	char c;
	c = path[strlen(path) - 1];

	if (c != ':' && c != '/' && path[0] != 0)
		strncat(path, "/", len);

	strncat(path, name, len);

	return 1;
}

LPROTO BOOL InitPrg( int argc, char *argv[] )
{
	if (!ReadArgs(argc, argv))
		return FALSE;

	return TRUE;
}

LPROTO void EndPrg( void )
{
	CloseProtoFiles();
}

LPROTO int main( int argc, char *argv[] )
{
	if (InitPrg(argc, argv))
	{
		DoPrg();
	}

	EndPrg();
	return EXIT_SUCCESS;
}

LPROTO void DoPrg( void )
{
	if (OpenProtoFiles())
	{
		int i;

		if (GlobalFH) SaveHeader(GlobalFH, GlobalFH_Name);
		if (LocalFH)  SaveHeader(LocalFH,  LocalFH_Name);
		if (BothFH)   SaveHeader(BothFH,   BothFH_Name);

		for (i = 0; AD.al_SRCFILES[i]; i++) 
		{
			FILE *InFH = 0;
			unsigned char *NextFile = AD.al_SRCFILES[i];

			printf("Opening input file '%s'...\n", NextFile);

			if (!(InFH = fopen(NextFile, "r")))
			{
				printf("Unable to open '%s'!\n", NextFile);
				DoErr();
				continue;
			}

			if (setvbuf(InFH, NULL, _IOFBF, IOBUFSIZE) != 0)
			{
				printf("Could not setvbuf() on '%s'!\n", NextFile);
				DoErr();
				continue;
			}
				
			if (XPROTO_BuildLists(InFH))
			{
				if (GlobalFH) XPROTO_SaveList(GlobalFH, XPROTO_GLOBAL, MyFilePart(NextFile));
				if (LocalFH)  XPROTO_SaveList(LocalFH,  XPROTO_LOCAL,  MyFilePart(NextFile));
				if (BothFH)   XPROTO_SaveList(BothFH,   XPROTO_BOTH,   MyFilePart(NextFile));

				if (AD.al_PROTODIR[0])
				{
					unsigned char path[256], *p;
					FILE *fh;
					strncpy(path, AD.al_PROTODIR, sizeof(path)-1);

					if (MyAddPart(path, NextFile, sizeof(path)-1))
					{
						if ((p = strrchr(path, '.'))) *p = 0;

						if (AD.al_NOPROTOSUFFIX)
							strncat(path, ".h", sizeof(path)-1);
						else
							strncat(path, "_protos.h", sizeof(path)-1);

						if ((fh = fopen(path, "w")))
						{
							SaveHeader(fh, MyFilePart(path));
							XPROTO_SaveList(fh, XPROTO_PROTODIR, MyFilePart(NextFile));
							SaveFooter(fh, MyFilePart(path));
							fclose(fh);
						}
						else
						{
							printf("Can't open %s\n", path);
							DoErr();
						}
					}
				}
			}
			XPROTO_FreeLists();
	
			fclose(InFH);
		}

		if (GlobalFH) SaveFooter(GlobalFH, GlobalFH_Name);
		if (LocalFH)  SaveFooter(LocalFH,  LocalFH_Name);
		if (BothFH)   SaveFooter(BothFH,   BothFH_Name);

		printf("Finished. Extracted %lu global and %lu local prototypes.\n", GlobalCnt, LocalCnt);
	}
	CloseProtoFiles();
}

LPROTO void DoErr( void )
{
	// Does StdC have an equivalent of PrintFault()?
	printf("XProto io error: %d ", errno);
}

GPROTO BOOL XPROTO_BuildLists( FILE *InFH )
{
	static unsigned char inbuf[INBUFLEN+2];
	BOOL success = FALSE;
	
	if (!InFH) return FALSE;

	list_init(&GlobalList);
	list_init(&LocalList);
	
	/* Build two linked lists, one for global protos and the
	   other one for local protos. */

	while (fgets(inbuf, INBUFLEN, InFH) != 0)
	{
		register unsigned char *strptr = inbuf;

		while (*strptr == ' ' || *strptr == '\t') strptr++; /* Skip spaces/tabs */
				
		if (myStrnicmp(strptr, STR_GPROTO, sizeof(STR_GPROTO) - 1) == 0)
		{		
			strptr += sizeof(STR_GPROTO);
			list_saveitem(&GlobalList, XPROTO_ExtractProtoLine(strptr, InFH), "");
			success = TRUE;
			GlobalCnt++;
		}
		else if (myStrnicmp(strptr, STR_LPROTO, sizeof(STR_LPROTO) - 1) == 0)
		{
			strptr += sizeof(STR_LPROTO);
			list_saveitem(&LocalList, XPROTO_ExtractProtoLine(strptr, InFH), "");
			success = TRUE;
			LocalCnt++;
		}	
	}
	return success;
}

GPROTO unsigned char *XPROTO_ExtractProtoLine( unsigned char *FirstLine, FILE *SrcFH )
{
	unsigned char *ProtoLineVec = NULL;
	register unsigned char *TmpStrVec = NULL;
	register unsigned char *VecCh;

	// Skip tabs and spaces
	while ((*FirstLine == ' ') || (*FirstLine == '\t')) FirstLine++;

	// Empty line? If so return NULL.
	if (FirstLine[0] == 0) return NULL;

	// Allocate a temporary string buffer
	if (!(TmpStrVec = calloc(1, TMPSTRVEC_LEN+8))) return NULL;

	// Set pointer
	VecCh = TmpStrVec;
	
	// Copy line to it
	strcpy(TmpStrVec, FirstLine);

	for(;;)
	{
		// Loop until we find a "{", ";" or zero-termintation.
		while (  *VecCh!='{'  &&  *VecCh!=';'  &&  *VecCh!=0  )
			VecCh++;

		// Did we line termination?
		if (*VecCh == 0)
		{
			// Bytes left is: total - distance.
			long BytesLeft = TMPSTRVEC_LEN - (VecCh - TmpStrVec);
			
			// Readin some more		
			if ((BytesLeft <= 1) || (fgets(VecCh, BytesLeft, SrcFH) == 0))
			{
				// No more, termineate for (;;) loop
				free(TmpStrVec);
				TmpStrVec = NULL;						
				break;
			}
		}
		else if (*VecCh == '{')
		{
			//Printf("FOUND CURLY TEST: %.300s", FirstLine);
			// Back one, pass to the curly "{"
			--VecCh;
			// Loop back till we find a non white space.	
			while ((*VecCh == ' ') || (*VecCh == '\t')) --VecCh;
			VecCh++;			
			*VecCh++ = ';';
			*VecCh = 0;
			break;
		}
		else if (*VecCh == ';')
		{
			*(++VecCh) = '\0';
			break;
		}
	}

	if (TmpStrVec)
	{
		ProtoLineVec = TrimString(TmpStrVec);
		free(TmpStrVec);
	}
	
	
	return ProtoLineVec;
}

GPROTO void XPROTO_FreeLists( void )
{
	register struct node *n, *tmpn;

	for (n = LocalList.head; n->succ;)
	{
		tmpn = n->succ;
		if (n->name) free(n->name);
		free(n); n = tmpn;
	}

	for (n = GlobalList.head; n->succ;)
	{
		tmpn = n->succ;
		if (n->name) free(n->name);
		free(n); n = tmpn;
	}

	list_init(&GlobalList);
	list_init(&LocalList);
}

GPROTO BOOL XPROTO_SaveList( FILE *DestFH, long xpID, unsigned char *ModuleName )
{
	register struct node *N = NULL;

	if (xpID == XPROTO_GLOBAL || xpID == XPROTO_BOTH || xpID == XPROTO_PROTODIR)
	{
		if (GlobalList.cnt != 0)
		{
			fprintf(DestFH,
				OP_COM
				"\n"
				" * Global prototypes for module %s\n"
				" *\n"
				" * Auto-generated with " VERS " by Andrew Bell\n"
				" *\n"
				" "
				CL_COM "\n"
				"\n",
				ModuleName
			);

			while ((N = list_getnext(&GlobalList, N, NULL)))
			{
				fputs("GPROTO ", DestFH);
				fputs(N->name, DestFH);
				fputs("\n", DestFH);
			}

			fputs("\n", DestFH);
		}
	}

	if (xpID == XPROTO_LOCAL || xpID == XPROTO_BOTH || xpID == XPROTO_PROTODIR)
	{
		if (LocalList.cnt != 0)
		{
			fprintf(DestFH, OP_COM "\n"
				" * Local prototypes for module %s\n"
				" *\n"
				" * Auto-generated with " VERS " by Andrew Bell\n"
				" *\n"
				" " CL_COM "\n\n",
				ModuleName
			);

			while ((N = list_getnext(&LocalList, N, NULL)))
			{
				fputs("LPROTO ", DestFH);
				fputs(N->name, DestFH);
				fputs("\n", DestFH);
			}

			fputs("\n", DestFH);
		}
	}

	return TRUE;
}

GPROTO BOOL OpenProtoFiles( void )
{
	if (AD.al_LOCAL[0])
	{
		printf("Opening output file for locals: '%s'...\n", AD.al_LOCAL);

		if ((LocalFH = fopen(AD.al_LOCAL, "w")))
		{
			if (setvbuf(LocalFH, NULL, _IOFBF, IOBUFSIZE) != 0)
				return FALSE;

			strncpy(LocalFH_Name, MyFilePart(AD.al_LOCAL), sizeof(LocalFH_Name)-1);
		}
		else
		{
			return FALSE;
		}
	}

	if (AD.al_GLOBAL[0])
	{
		printf("Opening output file for globals: '%s'...\n", AD.al_GLOBAL);

		if ((GlobalFH = fopen(AD.al_GLOBAL, "w")))
		{
			if (setvbuf(GlobalFH, NULL, _IOFBF, IOBUFSIZE) != 0)
				return FALSE;

			strncpy(GlobalFH_Name, MyFilePart(AD.al_GLOBAL), sizeof(GlobalFH_Name)-1);
		}
		else
		{
			return FALSE;
		}
	}

	if (AD.al_BOTH[0])
	{
		printf("Opening output file for both: '%s'...\n", AD.al_BOTH);

		if ((BothFH = fopen(AD.al_BOTH, "w")))
		{
			if (setvbuf(BothFH, NULL, _IOFBF, IOBUFSIZE) != 0)
				return FALSE;

			strncpy(BothFH_Name, MyFilePart(AD.al_BOTH), sizeof(BothFH_Name)-1);
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
}

GPROTO void CloseProtoFiles( void )
{
	if (LocalFH)  fclose(LocalFH);  LocalFH = 0;
	if (GlobalFH) fclose(GlobalFH); GlobalFH = 0;
	if (BothFH)   fclose(BothFH);   BothFH = 0;
}

GPROTO void SaveHeader( FILE *OutFH, unsigned char * filename )
{
	time_t t;
	unsigned char datebuf[128+4], namebuf[128], *p = namebuf;
	
	strncpy(namebuf, filename, sizeof(namebuf)-1);

	for (p = namebuf; *p; p++)
	{
		if (*p == '.')
			*p = '_';
		else
			*p = toupper(*p);
	}

	//ctime(t)
	//localtime(&tm);
	
	//if (!TimeToStr(&tm, datebuf))
	//	strcpy(datebuf, "<< Unknown >>");

	t = time(NULL);
	strncpy(datebuf, ctime(&t), sizeof(datebuf)-1);

	fprintf(OutFH,
		"\n"
		OP_COM " This proto file was generated on %s " CL_COM "\n"
		"\n"
		"#" "ifndef %s\n"
		"#" "define %s 1\n"
		"\n"
		"#" "ifndef GPROTO\n"
		"#" "define GPROTO\n"
		"#" "endif " OP_COM " GPROTO " CL_COM "\n"
		"\n"
		"#" "ifndef LPROTO\n"
		"#" "define LPROTO\n"
		"#" "endif " OP_COM " LPROTO " CL_COM "\n"
		"\n"
		"\n",
		datebuf, namebuf, namebuf
	);
}

GPROTO void SaveFooter( FILE *OutFH, unsigned char * filename )
{
	unsigned char namebuf[128], *p;
	strncpy(namebuf, filename, 127);

	for (p = namebuf; *p; p++)
	{
		if (*p == '.')
			*p = '_';
		else
			*p = toupper(*p);
	}

	fprintf(OutFH,
		"\n#" "endif "
		OP_COM " %s " CL_COM "\n",
		namebuf
	);
}

/*********************************************************************/
/* String routines */

GPROTO unsigned char *TrimString( unsigned char *Str )
{
	register unsigned char *StrVec;
	
	if (!Str) return NULL;

	if ((StrVec = calloc(1, strlen(Str) + 1)))
	{
		register unsigned char *eStr, Ch;
		
		if (*Str)
		{
			while ((Ch = *Str))
			{
				if (Ch == '\t' || Ch == '\r' || Ch == '\n' || Ch == ' ')
					++Str;
				else
					break;
			}

			if (*Str)
			{
				eStr = Str + (strlen(Str) - 1);

				while (eStr > Str)
				{
					Ch = *eStr;				
					if (Ch == '\t' || Ch == '\r' || Ch == '\n' || Ch == ' ')
						--eStr;
					else
						break;
				}
				memcpy(StrVec, Str, (eStr - Str) + 1);
			}
		}
	}
	return StrVec;	
}

GPROTO unsigned char *callocstr( unsigned char *str )
{
	register unsigned char *strmem;
	if (!str) return NULL;
	if ((strmem = calloc(1, strlen(str) + 1))) strcpy(strmem, str);
	return strmem;
}
