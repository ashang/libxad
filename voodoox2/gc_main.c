
/***************************************************************************

 GenClasses - MUI class dispatcher generator
 Copyright (C) 2001 and later by Andrew Bell <mechanismx@lineone.net>
 Created on 2/Aug/2001

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

 $Id: gc_main.c,v 1.4 2004/01/25 15:20:29 andrewbell Exp $

 TODO
    - Improve tag value generation.
    - Generation of desciptive class tree in header comment.
    - DISPATCHERPROTO macro should be included in generated source.
    - DECLARE param parser should allow normal C style comments to be used
      too.

***************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include "gc_main.h"
#include "gc_lists.h"

#ifdef ENABLE_UNIX
	#define CURRENT_DIR "."
#else
	#define CURRENT_DIR ""
#endif

char *verstr = "0.7";

/*
 * When compiling for YAM under VBCC use:
 *
 * GenClasses <classespath> -bYAM -gpl -i"/YAM.h" -mkfilevmakefile,vc,-o,-+,-c* 
 * Note: The DISPATCHERPROTO define is not included in the generated source.
 *
 *
 * History
 * -------
 *
 * 0.7 - Changed the scope of some makefile code.
 *     - Corrected UNIX/Linux current dir handling.
 *     - Cleaned up help text.
 *     - Style/whitespace changes. Some parts now look less
 *       odd when tab size is 8.
 *     - Adjusted fprintf() layouts so that code embedded in
 *       strings is easier to read.
 *
 * 0.6 - Fixed GCC 3.3.1 warnings.
 *
 * 0.5 - Many cleanups and fixes.
 *       Added support for -matchprefix and -destdir
 *
 * 0.4 - Class scan routine now ensures child classes are
 *       inserted before their parents in the list.
 *     - Added header dependancy to makefile generation.
 *
 * 0.3 - List counter wasn't being set to 0 on init
 *       resulting in some large trashy values. :-)
 *     - Some prototypes with MUIP_xxxx weren't saved
 *       correctly.
 *     - makefile is marked as do not edit.
 *     - Doesn't use join #?.o in makefile anymore.
 *     - New keyword called CLASSDATA.
 *     - xxxxGetSize() function is now saved in header.
 *     - Uses strstr() to extract SuperClass and Description.
 *       This allows for the keywords to be embedded in comments.
 *     - Misc code changes.
 *
 * 0.2 - Added error reports to all fopen() calls.
 *     - Added myaddpart(), mypathpart() and
 *       myfilepart(). All path related problems should
 *       now be gone.
 *     - Now checks for .c extensions at the end of the
 *       filename instead of using strstr().
 *     - No longer casts the msg argument.
 *     - Source files without a Superclass: keyword are
 *       skipped.
 *     - Unterminated EXPORT block warning now gives
 *       correct line number in warning.
 *     - Classes.h now generates the prototypes for the
 *       OVERLOADS and DECLARES.
 *     - makefile now produces all.o target.
 *     - Implemented free_classdef().
 *     - Implemented linked-list routines.
 *     - Removed some globals.
 *     - Added generation of class headers.
 *     - New tag value generation.
 *     - Using long instead of int now.
 *     - exlineno was defined as char, oops.
 *
 * 0.1 - Initial version, given to Allan and Jens.
 *
 *
 */

char  arg_basename         [256];
char  arg_includes         [256];
char  arg_matchprefix      [256];
char  arg_classdir         [256];
char  arg_destdir          [256];
char  arg_mkfile           [256];
long  arg_gpl              = 0;     /* booleans */
long  arg_storm            = 0;
long  arg_v                = 0;
long  arg_external         = 0;
char *arg_me;

/*******************************************************************************
 *
 * Emulation of opendir()/closedir()/readdir() on AmigaOS
 *
 * Only include this if your compiler doesn't support the <dirent.h> routines.
 *
 *******************************************************************************
 *
 *
 */ 

#ifdef EMULATE_DIRENT

#include <proto/dos.h>
#include <exec/types.h>
#include <dos/dosextens.h>

typedef struct dirent
{
	char *d_name;
	BPTR lock;
	struct FileInfoBlock *fib;
} DIR;

void closedir( DIR *de )
{
	if (!de) return;
	if (de->lock) UnLock(de->lock);
	if (de->fib) FreeDosObject(DOS_FIB, de->fib);
	free(de);
}

DIR *opendir( char *name )
{
	DIR *de;
	if (!(de = calloc(1, sizeof(struct dirent)))) return NULL;
	if ((de->fib = AllocDosObject(DOS_FIB, NULL)) &&
		(de->lock = Lock(name, SHARED_LOCK)) &&
		(Examine(de->lock, de->fib)) && (de->fib->fib_DirEntryType > 0))
	{
		de->d_name = de->fib->fib_FileName;
	}
	else
	{
		closedir(de); de = NULL;
	}
	return de;
}

DIR *readdir( DIR *de )
{
	if (!de) return NULL;
	return ExNext(de->lock, de->fib) ? de : NULL;
}

#else
#include <dirent.h>
#ifndef TAG_USER
#define TAG_USER ((unsigned long)(1UL<<31))
#endif
#endif /* EMULATE_DIRENT */


/*******************************************************************************
 *
 * Assorted routines
 *
 *******************************************************************************
 *
 *
 */ 

char *SkipWhiteSpaces( unsigned char *p ) /* Note: isspace() sucks... */
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

char *StrAlloc( unsigned char *str )
{
	/* Copy str to buffer and strip leading and trailing white spaces, LFs, CRs, etc. */
	unsigned char *strvec, *estr, c;			

	if (!str)
		str = "";

	if (!(strvec = calloc(1, strlen(str) + 1)))
		return NULL;


	if (*str == 0)
		return strvec; /* Empty string? */

	while ((c = *str))
	{
		if (c == '\t' || c == '\r' || c == '\n' || c == ' ' || c == 0xa0)
			++str;
		else
			break;
	}

	if (*str == 0)
		return strvec; /* Empty string still? */

	estr = str + (strlen(str) - 1);

	while (estr > str)
	{
		c = *estr;				
		if (c == '\t' || c == '\r' || c == '\n' || c == ' ' || c == 0xa0)
			--estr;
		else
			break;
	}

	memcpy(strvec, str, (size_t) (estr - str) + 1);
	return strvec;	
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

	//printf("DEBUG: MyAddPart returned %s\n", path);

	return 1;
}

char *MyPathPart( char *path )
{
	char *p;
	if (!path) return NULL;
	if ((p = strrchr(path, '/'))) return p;
	if ((p = strrchr(path, ':'))) return ++p;
	return path;
}

char *MyFilePart( char *path )
{
	char *p;
	if (!path) return NULL;
	if ((p = strrchr(path, '/'))) return ++p;
	if ((p = strrchr(path, ':'))) return ++p;
	return path;
}

/*******************************************************************************
 *
 * Tag value generation
 *
 *******************************************************************************
 *
 *
 */ 

long *collision_cnts;

unsigned long GetHash( char *str )
{
    static unsigned long primes[10] = { 3, 5, 7, 11, 13, 17, 19, 23, 27, 31 };
    unsigned long i = 0;
    unsigned long hash = strlen(str);

    while (*str)
    {
        hash *= 13;
        hash += (*str++) * primes[(++i) % 10];
    }

    return hash % BUCKET_CNT;
}

unsigned long GetTagValue( char *tag )
{
	unsigned long hash = GetHash(tag);
	unsigned long val = TAG_BASE | ((hash << 16) | (hash << 8) | (++collision_cnts[hash]));
	if (arg_v) printf("Assigning tag %-35s with value %lx\n", tag, val);
	return val;
}

/*******************************************************************************
 *
 * Parsing routines
 *
 *******************************************************************************
 *
 *
 */ 

void FreeOverload( struct overloaddef *od )
{
	if (!od) return;
	if (od->name) free(od->name);
}

void FreeDeclare( struct declaredef *dd )
{
	if (!dd) return;
	if (dd->name) free(dd->name);
	if (dd->params) free(dd->params);
}

void FreeExportBlk( struct exportdef *ed )
{
	if (!ed) return;
	if (ed->exporttext) free(ed->exporttext);
}

void FreeAttr( struct attrdef *ad )
{
	if (!ad) return;
	if (ad->name) free(ad->name);
}

void FreeClassDef( struct classdef *cd )
{
	struct node *n;
	if (!cd) return;
	if (cd->classdata) free(cd->classdata);
	while ((n = list_remhead(&cd->overloadlist)))
	{
		FreeOverload(n->data);
		free(n);
	}	
	while ((n = list_remhead(&cd->declarelist)))
	{
		FreeDeclare(n->data);
		free(n);
	}	
	while ((n = list_remhead(&cd->attrlist)))
	{
		FreeAttr(n->data);
		free(n);
	}	
	while ((n = list_remhead(&cd->exportlist)))
	{
		FreeExportBlk(n->data);
		free(n);
	}	
	free(cd);
}


void AddOverload( struct classdef *cd, char *name )
{
	struct overloaddef *od;
	if (!(od = (struct overloaddef *) calloc(1, sizeof(struct overloaddef)))) return;
	if (!(od->name = StrAlloc(name))) return;
	list_saveitem(&cd->overloadlist, od->name, od);
}

void AddDeclare( struct classdef *cd, char *name, char *params )
{
	struct declaredef *dd;
	if (!(dd = (struct declaredef *) calloc(1, sizeof(struct declaredef)))) return;
	dd->name = StrAlloc(name);
	dd->params = StrAlloc(params);
	if (!dd->name || !dd->params)
	{
		FreeDeclare(dd); return;
	}
	list_saveitem(&cd->declarelist, dd->name, dd);
}

void AddExportBlk( struct classdef *cd, char *textblk )
{
	struct exportdef *ed;
	if (!(ed = (struct exportdef *) calloc(1, sizeof(struct exportdef)))) return;
	if (!(ed->exporttext = StrAlloc(textblk))) return;
	list_saveitem(&cd->exportlist, ed->exporttext, ed);
}

void AddAttr( struct classdef *cd, char *name )
{
	struct attrdef *ad;
	if (!(ad = (struct attrdef *) calloc(1, sizeof(struct attrdef)))) return;
	if (!(ad->name = StrAlloc(name))) return;
	if (list_findname(&cd->attrlist, name))
	{
		if (arg_v) printf("ATTR %.100s already collected, skipped.\n", name);
		FreeAttr(ad);
		return;
	}
	list_saveitem(&cd->attrlist, ad->name, ad);
}

struct classdef *ProcessClassSrc( char *path )
{
	FILE *fp;
	long lineno = 0;
	char line[256], *p, *ob, *cb, *sub, *className;
	struct classdef *cd;
	long spos, epos = 0, exlineno = lineno; char *blk = NULL;

	if (!(cd = calloc(1, sizeof(struct classdef))))
	{
		return NULL;
	}

	if (!(fp = fopen(path, "r")))
	{
		printf("ERROR: Unable to open '%.100s' for reading\n", path);
		free(cd); return NULL;
	}

	list_init(&cd->overloadlist);
	list_init(&cd->declarelist);
	list_init(&cd->attrlist);
	list_init(&cd->exportlist);

	className = MyFilePart(path);

	if (arg_matchprefix[0])
	{
		// Skip the prefix so that it doesn't get
		// included in the C define.
		className += strlen(arg_matchprefix);
	}	

	if ((cd->name = StrAlloc(className)))
	{
		if ((p = strrchr(cd->name, '.')))	// Chop off suffic
			*p = 0;
	}

	while (fgets(line, 255, fp))
	{
		lineno++;
		p = SkipWhiteSpaces(line);

		if (!*p)
			continue;

		/*printf("line number = %ld [%.100s]\n", lineno, p);*/
		ob = cb = NULL;

		/******** Scan line for keywords and extract the associated information... ********/

		if (!cd->superclass && (sub = strstr(p, KEYWD_SUPERCLASS)))
		{
			if (arg_v) printf(KEYWD_SUPERCLASS " keyword found at line %ld in file %s\n", lineno, path);
			sub += sizeof(KEYWD_SUPERCLASS) - 1;
			cd->superclass = StrAlloc(SkipWhiteSpaces(sub));
		}
		else if (!cd->desc && (sub = strstr(p, KEYWD_DESC)))
		{
			if (arg_v) printf(KEYWD_DESC " keyword found at line %ld\n", lineno);
			sub += sizeof(KEYWD_DESC) - 1;
			cd->desc = StrAlloc(SkipWhiteSpaces(sub));
		}
		else if (!cd->classdata && strstr(line, KEYWD_CLASSDATA))
		{
			if (arg_v) printf(KEYWD_CLASSDATA " keyword found at line %ld\n", lineno);
			spos = ftell(fp);

			while(fgets(p = line, 255, fp))
			{
				lineno++;
				epos = ftell(fp);

				if (!(p = strstr(line, "*/")))
					continue;

				epos += (p - line) - 3; /* + offset into line... */

				fseek(fp, spos, SEEK_SET);

				if (!(blk = calloc(1, (size_t)(epos - spos + 1))))
				{
					printf("WARNING: Cannot read " KEYWD_CLASSDATA " block at line %ld, out of memory!\n", exlineno); break;
				}

				fread(blk, (size_t)(epos - spos), 1, fp);

				if ((ob = strchr(blk, '{')))
				{
					if (!(cb = strchr(ob + 1, '}')))
						cb = blk + strlen(blk);

					*cb = 0;

					if ((cd->classdata = calloc(1, strlen(++ob) + 1)))
					{
						strcpy(cd->classdata, ob);
					}
				}
				else if ((cd->classdata = calloc(1, strlen(blk) + 1)))
				{
					strcpy(cd->classdata, blk);
				}

				free(blk);
				break;
			}
		}
		else if (strncmp(KEYWD_OVERLOAD, p, sizeof(KEYWD_OVERLOAD) - 1) == 0)
		{
			if (arg_v)
				printf(KEYWD_OVERLOAD " keyword found at line %ld in file %s\n", lineno, path);

			p += sizeof(KEYWD_OVERLOAD) - 1;

			if (!(ob = strchr(p, '(')))
				continue; /* There's no open bracket, ignore it... */
			
			if (!(cb = strchr(ob, ')')))
				cb = p + strlen(p);

			*cb = 0;
			AddOverload(cd, ++ob);
		}
		else if (strncmp(KEYWD_DECLARE, p, sizeof(KEYWD_DECLARE) - 1) == 0)
		{
			if (arg_v)
				printf(KEYWD_DECLARE " keyword found at line %ld in file %s\n", lineno, path);

			p += sizeof(KEYWD_DECLARE) - 1;

			if (!(ob = strchr(p, '(')))
				continue; /* There's no open bracket, ignore it... */

			if (!(cb = strchr(ob, ')')))
				cb = p + strlen(p);

			if ((p = strstr(cb + 1, "//")))
				p += 2;

			*cb = 0;
			AddDeclare(cd, ++ob, p);
		}
		else if (strncmp(KEYWD_ATTR, p, sizeof(KEYWD_ATTR) - 1) == 0)
		{
			if (arg_v)
				printf(KEYWD_ATTR " keyword found at line %ld in file %.100s\n", lineno, path);
			
			p += sizeof(KEYWD_ATTR) - 1;
			
			if (!(ob = strchr(p, '(')))
				continue; /* There's no open bracket, ignore it... */
			
			if (!(cb = strchr(ob, ')')))
				cb = p + strlen(p);
			
			*cb = 0;
			AddAttr(cd, ++ob);
		}
		else if (strncmp("/*", p, 2) == 0) /* TODO: Use strstr() here, like CLASSDATA */
		{
			p = SkipWhiteSpaces(p + 2);

			if (strncmp(KEYWD_EXPORT, p, sizeof(KEYWD_EXPORT) - 1) == 0)
			{
				if (arg_v)
					printf(KEYWD_EXPORT " keyword found at line %ld in file %.100s\n", lineno, path);

				p += sizeof(KEYWD_EXPORT) - 1;
				spos = ftell(fp);

				while(fgets(p = line, 255, fp))
				{
					lineno++;
					epos = ftell(fp);

					if (!(p = strstr(line, "*/")))
						continue;

					epos += (p - line) - 3; /* + offset into line... */

					fseek(fp, spos, SEEK_SET);

					if (!(blk = calloc(1, (size_t)(epos - spos + 1))))
					{
						printf("WARNING: Cannot read " KEYWD_EXPORT " block at line %ld, out of memory!\n", exlineno); break;
					}

					fread(blk, (size_t)(epos - spos), 1, fp);
					AddExportBlk(cd, blk);
					free(blk);
					break;
				}
				if (epos == 0)
					printf("WARNING: Unterminated EXPORT block at line %ld\n", lineno);
			}
		}
	}	/* while() */

	if (!cd->superclass)
	{
		printf("WARNING: Source file '%.100s' doesn't contain a " KEYWD_SUPERCLASS " keyword. Skipping.\n", path);
		FreeClassDef(cd); cd = NULL;
	}
	else if (!cd->classdata)
	{
		printf("WARNING: Source file '%.100s' doesn't contain a " KEYWD_CLASSDATA " keyword. Skipping.\n", path);
		FreeClassDef(cd); cd = NULL;
	}

	fclose(fp);
	return cd;
}

void PrintClassList( struct list *classlist )
{
	struct classdef *cd;
	struct overloaddef *od;
	struct declaredef *dd;
	struct attrdef *ad;
	struct node *n, *nn;
	printf("The following keywords were extracted:\n");

	for (nn = NULL; (nn = list_getnext(classlist, nn, (void **) &cd));)
	{
		printf("CLASS: %.100s\n", cd->name);

		for (n = NULL; (n = list_getnext(&cd->overloadlist, n, (void **) &od));)
			printf("  OVERLOAD: %.100s\n", od->name);
		for (n = NULL; (n = list_getnext(&cd->declarelist, n, (void **) &dd));)
			printf("   DECLARE: %.100s\n", dd->name);
		for (n = NULL; (n = list_getnext(&cd->attrlist, n, (void **) &ad));)
			printf("      ATTR: %.100s\n", ad->name);
	}
}

long ScanClasses( char *dirname, struct list *classlist )
{
	DIR *dir;
	struct dirent *de;
	char *name, dirbuf[256];
	long len, srccnt = 0;
	struct classdef *cd;

	strncpy(dirbuf, dirname, 255);
	if (arg_v) printf("scanning classes dir %.100s\n", dirbuf);

	if (!(dir = opendir(dirname)))
	{
		printf("Unable to open directory '%.100s'\n", dirname);
		return 0;
	}

	while ((de = readdir(dir)))
	{
		name = de->d_name; len = strlen(name);
		if (len < 2) continue;

		if ((name[len - 2] != '.') || (tolower(name[len - 1]) != 'c'))
		{
			if (arg_v) printf("Skipping: %.100s\n", name);
			continue;
		}	

		if (!strcmp(SOURCE_NAME, name) || !strcmp(HEADER_NAME, name))
			continue;

		if (strncmp(name, arg_matchprefix, strlen(arg_matchprefix)) != 0)
		{
			printf("%s is avoiding  : %-25.100s [Build invoker wants me to skip this]\n", arg_me, name);
			continue;
		}

		++srccnt;
		MyAddPart(dirbuf, de->d_name, 255);

		printf("%s is processing: %-25.100s ", arg_me, dirbuf);
		fflush(stdout);

		if ((cd = ProcessClassSrc(dirbuf)))
		{
			struct node *n;

			printf("[SUCCESS: Ovrlds=%ld Decls=%ld Attrs=%ld Exps=%ld]\n",
				cd->overloadlist.cnt,
				cd->declarelist.cnt,
				cd->attrlist.cnt,
				cd->exportlist.cnt
			);

			//printf("Adding [%.100s]\n", cd->name);

			for (n = classlist->head; n->succ; n = n->succ)
			{
				struct classdef *n_cd = (struct classdef *) n->data;

				if (strcmp(n_cd->name, &cd->superclass[5] /*skip MUIC_ prefix*/ ) == 0)
				{
					/* OK, the class which we are adding to the list has a superclass
					   which is already in the list. We need to make sure this child
					   comes before its parent in the list, so class init won't fail. */

					list_insertitem(classlist, cd->name, cd, n->pred);
					cd = NULL;
					break;
				}
			}

			if (cd)
			{
				list_saveitem(classlist, cd->name, cd);
			}
		}
		else
		{
			printf("[FAILED]\n");
		}

		*MyPathPart(dirbuf) = 0;
	}

	closedir(dir);

	if (srccnt == 0)
	{
		printf("ERROR: Was unable to find any sources in '%.100s'\n", dirname);
		return 0;
	}

	if (arg_v) PrintClassList(classlist);

	return 1;
}

/*******************************************************************************
 *
 * Source code generation
 *
 *******************************************************************************
 *
 *
 */ 

void GenGPL( FILE *fp )
{
	if (!fp || !arg_gpl) return;
	fprintf(fp,
	"\n"
	"/***************************************************************************\n"
	"\n"
	" This program is free software; you can redistribute it and/or modify\n"
	" it under the terms of the GNU General Public License as published by\n"
	" the Free Software Foundation; either version 2 of the License, or\n"
	" (at your option) any later version.\n"
	"\n"
	" This program is distributed in the hope that it will be useful,\n"
	" but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	" GNU General Public License for more details.\n"
	"\n"
	" You should have received a copy of the GNU General Public License\n"
	" along with this program; if not, write to the Free Software\n"
	" Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n"
	"\n"
	" $Id: gc_main.c,v 1.4 2004/01/25 15:20:29 andrewbell Exp $\n"
	"\n"
	"***************************************************************************/\n");
}

void GenSupportRoutines( FILE *fp )
{
	char *bn = arg_basename;
	fprintf(fp,
"%s%s%s"
"Object *%s_NewObject( STRPTR class, ULONG tag, ... )\n"
"{\n"
"  long i;\n"
"  for (i = 0; i < NUMBEROFCLASSES; i++)\n"
"  {\n"
"    if (!strcmp(MCCInfo[i].Name, class))\n"
"      return NewObjectA(%sClasses[i]->mcc_Class, NULL, (struct TagItem *)&tag);\n"
"  }\n"
"  return NULL;\n"
"}\n"
"%s"
"\n"
"%s%s%s"
"long %s_SetupClasses( void )\n"
"{\n"
"  long i;\n"
"  memset(%sClasses, 0, sizeof(%sClasses));\n"
"  for (i = 0; i < NUMBEROFCLASSES; i++)\n"
"  {\n"
"    struct MUI_CustomClass *superMCC = MCCInfo[i].SuperMCC == -1 ? NULL : %sClasses[MCCInfo[i].SuperMCC];\n"
"    %sClasses[i] = MUI_CreateCustomClass(NULL, MCCInfo[i].SuperClass, superMCC, (int) MCCInfo[i].GetSize(), MCCInfo[i].Dispatcher);\n"
"    if (!%sClasses[i])\n"
"    {\n"
"      %s_CleanupClasses();\n"
"      return 0;\n"
"    }\n"
"  }\n"
"  return 1;\n"
"}\n"
"%s"
"\n"
"%s%s%s"
"void %s_CleanupClasses( void )\n"
"{\n"
"  long i;\n"
"  for (i = NUMBEROFCLASSES-1; i >= 0; i--)\n"
"  {\n"
"    if (%sClasses[i])\n"
"    {\n"
"      MUI_DeleteCustomClass(%sClasses[i]);\n"
"      %sClasses[i] = NULL;\n"
"    }\n"
"  }\n"
"}\n"
"%s"
"\n",

	arg_storm ? "/// "                : "",	
	arg_storm ? bn                    : "",	
	arg_storm ? "_NewObject()\n"      : "",
	bn, bn,
	arg_storm ? "///\n"               : "",	

	arg_storm ? "/// "                : "",	
	arg_storm ? bn                    : "",	
	arg_storm ? "_SetupClasses()\n"   : "",
	bn, bn, bn, bn, bn, bn, bn,
	arg_storm ? "///\n"               : "",	

	arg_storm ? "/// "                : "",	
	arg_storm ? bn                    : "",	
	arg_storm ? "_CleanupClasses()\n" : "",
	bn, bn, bn, bn,
	arg_storm ? "///\n"               : "");
}

long GenMainSource( char *destfile, struct list *classlist )
{
	FILE *fp;
	struct classdef *nextcd;
	struct overloaddef *nextod;
	struct declaredef *nextdd;
	struct node *n, *nn;

	printf("%s is writing   : %.100s\n", arg_me, destfile);

	if (!(fp = fopen(destfile, "w")))
	{
		printf("ERROR: Unable to open '%.100s' for writing\n", destfile);
		return 0;
	}

	/***************************************/
	/*           Write header...           */
	/***************************************/

	GenGPL(fp);

	fprintf(fp,
		"\n"
		" /* " EDIT_WARNING " */\n"
		"\n"
		"#include <string.h>\n"
		"#include \"Classes.h\"\n"
		"\n"
		"struct MUI_CustomClass *%sClasses[NUMBEROFCLASSES];\n\n",
		arg_basename
	);

	/***************************************/
	/*        Write dispatchers...         */
	/***************************************/
	
	for (nn = NULL; (nn = list_getnext(classlist, nn, (void **) &nextcd));)
	{
		if (arg_storm) fprintf(fp, "/// %sDispatcher()\n", nextcd->name);

		fprintf(fp,
			"DISPATCHERPROTO(%sDispatcher)\n"
			"{\n  switch(msg->MethodID)\n  {\n",
			nextcd->name
		);

		/* Write OVERLOADs */

		for (n = NULL; (n = list_getnext(&nextcd->overloadlist, n, (void **) &nextod));)
		{
			fprintf(fp,
				"    case %-40s: return m_%s_%-20s(cl, obj, msg);\n",
				nextod->name, nextcd->name, nextod->name
			);
		}

		/* Write DECLAREs */

		for (n = NULL; (n = list_getnext(&nextcd->declarelist, n, (void **) &nextdd));)
		{
			char name[128];
			sprintf(name, "MUIM_%s_%s", nextcd->name, nextdd->name);

			fprintf(fp,
				"    case %-40s: return m_%s_%-20s(cl, obj, (APTR)msg);\n",
				name, nextcd->name, nextdd->name
			);
		}

		fprintf(fp,
			"  }\n  return DoSuperMethodA(cl, obj, msg);\n}\n"
		);

		if (arg_storm) fprintf(fp, "///\n");
		fprintf(fp, "\n");
	}

	/*****************************************/
	/*        Write MCCInfo struct           */
	/*****************************************/

	fprintf(fp,
		"const struct {\n"
		"  STRPTR Name; STRPTR SuperClass; LONG SuperMCC; ULONG (*GetSize) (void); APTR Dispatcher;\n"
		"} MCCInfo[NUMBEROFCLASSES] = {\n"
	);

	for (n = NULL; (n = list_getnext(classlist, n, (void **) &nextcd));)
	{
		fprintf(fp,
			"  { MUIC_%-20s, %-20s, -1, %sGetSize, ENTRY(%sDispatcher) }",
			nextcd->name, nextcd->superclass, nextcd->name, nextcd->name
		);
		if (nextcd) fprintf(fp, ",\n"); else fprintf(fp, "\n");
	}

	fprintf(fp,
		"};\n\n"
	);
	
	/*****************************************/
	/*        Append support routines        */
	/*****************************************/

	GenSupportRoutines(fp);
	fclose(fp);
	return 1;
}

long GenMainHeader( char *destfile, struct list *classlist )
{
	FILE *fp;
	char *bn = arg_basename, *className, *p;
	struct classdef *nextcd;
	struct declaredef *nextdd;
	struct exportdef *nexted;
	struct attrdef *nextad;
	struct overloaddef *nextod;
	struct node *n, *nn;

	printf("%s is writing   : %.100s\n", arg_me, destfile);

	if (!(fp = fopen(destfile, "w")))
	{
		printf("ERROR: Unable to open %.100s\n", destfile);
		return 0;
	}

	/***************************************/
	/*           Write header...           */
	/***************************************/

	GenGPL(fp);
	fprintf(fp, "\n#ifndef CLASSES_CLASSES_H\n"
		"#define CLASSES_CLASSES_H\n"
		"\n /* " EDIT_WARNING " */\n\n");

	/* TODO: write class tree in header here */

	/***************************************/
	/*          Write includes...          */
	/***************************************/
	
	fprintf(fp, 
		"#include <clib/alib_protos.h>\n"
		"#include <libraries/mui.h>\n"
		"#include <proto/intuition.h>\n"
		"#include <proto/muimaster.h>\n"
		"#include <proto/utility.h>\n");

	if (arg_includes[0])
	{
		char *nx, *p = arg_includes;
		fprintf(fp, "/* These are user defined includes */\n");
		do
		{
			if ((nx = strchr(p, ','))) *nx++ = 0;
			fprintf(fp, "#include %s\n", p);
		}
		while ((p = nx));
	}

	/***************************************/
	/*            Write misc...            */
	/***************************************/

	fprintf(fp, 
		"\n"
		"#define inittags(msg)   (((struct opSet *)msg)->ops_AttrList)\n"
		"#define GETDATA         struct Data *data = (struct Data *)INST_DATA(cl,obj)\n"
		"#define NUMBEROFCLASSES %ld\n"
		"\n"
		"extern struct MUI_CustomClass *%sClasses[NUMBEROFCLASSES];\n"
		"Object *%s_NewObject( STRPTR class, ULONG tag, ... );\n"
		"long %s_SetupClasses( void );\n"
		"void %s_CleanupClasses( void );\n"
		"\n",
		classlist->cnt, bn, bn, bn, bn
	);

	/***************************************/
	/*             Class loop              */
	/***************************************/

	for (nn = NULL; (nn = list_getnext(classlist, nn, (void **) &nextcd));)
	{
		className = nextcd->name;

		if (className == NULL)
		{
			printf("INTERNAL ERROR: className is NULL in GenMainHeader!");
			exit(EXIT_FAILURE);
		}	

		/***********************************************/
		/* Write MUIC_, xxxObject, etc. for this class */
		/***********************************************/

		fprintf(fp, 
			"/******** Class: %s ********/\n"
			"\n"
			"#define MUIC_%s \"%s_%s\"\n"
			"#define %sObject %s_NewObject(MUIC_%s\n",
			className, className, bn, className, className, bn, className
		);

		for (n = NULL; (n = list_getnext(&nextcd->declarelist, n, (void **) &nextdd));)
		{
			char name[128];
			sprintf(name, "MUIM_%s_%s", className, nextdd->name);
			fprintf(fp, "#define %-45s 0x%08lx\n", name, GetTagValue(name));
		}

		/***************************************/
		/* Write attributes for this class     */
		/***************************************/

		for (n = NULL; (n = list_getnext(&nextcd->attrlist, n, (void **) &nextad));)
		{
			char name[128];
			sprintf(name, "MUIA_%s_%s", className, nextad->name);
			fprintf(fp, "#define %-45s 0x%08lx\n", name, GetTagValue(name));
		}

		fprintf(fp, "\n");

		/*****************************************/
		/* Write MUIP_ structures for this class */
		/*****************************************/

		for (n = NULL; (n = list_getnext(&nextcd->declarelist, n, (void **) &nextdd));)
		{
			fprintf(fp,
				"struct MUIP_%s_%s\n"
				"{\n"
				"  ULONG methodID;\n", className, nextdd->name
			);

			if (strlen(nextdd->params) > 0)
			{
				for (p = nextdd->params;;) if ((p = strchr(p, ','))) *p++ = ';'; else break;
				fprintf(fp, "  %s;\n", nextdd->params);
			}

			fprintf(fp, "};\n\n");
		}
		fprintf(fp, "\n");

		/***************************************/
		/* Write protos for this class         */
		/***************************************/

		fprintf(fp, "ULONG %sGetSize( void );\n", className);

		/* Write OVERLOADs */

		for (n = NULL; (n = list_getnext(&nextcd->overloadlist, n, (void **) &nextod));)
		{
			fprintf(fp,
				"ULONG m_%s_%-20s(struct IClass *cl, Object *obj, Msg msg);\n",
				nextcd->name, nextod->name
			);
		}

		/* Write DECLAREs */

		for (n = NULL; (n = list_getnext(&nextcd->declarelist, n, (void **) &nextdd));)
		{
			fprintf(fp,
				"ULONG m_%s_%-20s(struct IClass *cl, Object *obj, struct MUIP_%s_%s *msg);\n",
				nextcd->name, nextdd->name, className, nextdd->name
			);
		}

		fprintf(fp, "\n");

		/***************************************/
		/* Write exported text for this class  */
		/***************************************/
			
		if (nextcd->exportlist.cnt)
		{
			fprintf(fp, "/* Exported text */\n\n");

			for (n = NULL; (n = list_getnext(&nextcd->exportlist, n, (void **) &nexted));)
			{
				fprintf(fp, "%s\n\n", nexted->exporttext);
			}

		}
	}
	
	fprintf(fp,
		"\n#endif /* CLASSES_CLASSES_H */\n\n"
	);
	
	fclose(fp);
	return 1;
}

long GenIndividualHeaders( struct list *classlist, char *destPath )
{
	struct node *n;
	struct classdef *nextcd;
	FILE *fp;
	char destBuf[256];

	strncpy(destBuf, destPath, sizeof(destBuf)-1);

	for (n = NULL; (n = list_getnext(classlist, n, (void **) &nextcd));)
	{
		char name[128], buf[128], *p;
		char *className = nextcd->name;

		if (className == NULL)
		{
			printf("INTERNAL ERROR: GOT NULL in nextcd->name in GenIndividualHeaders!\n");
			exit(EXIT_FAILURE);
		}	

		if (!className[0])
		{
			printf("WARNING: Empty name in nextcd->name in GenIndividualHeaders!\n");
		}

		if (arg_matchprefix[0])
		{
			if (strncmp(className, arg_matchprefix, strlen(arg_matchprefix)) == 0)
			{
				className += strlen(arg_matchprefix);
			}
		}

		sprintf(name, "%s.h", className);

		MyAddPart(destBuf, name, sizeof(destBuf)-1);

		printf("%s is writing   : \"%.100s\"\n", arg_me, destBuf);

		if (!(fp = fopen(destBuf, "w")))
		{
			printf("WARNING: Unable to write '%.100s'\n", destBuf);

			*MyPathPart(destBuf) = 0;
			continue;
		}

		strncpy(buf, className, 127);
		for (p = buf; *p; p++) *p = toupper(*p);

		fprintf(fp,
			"#ifndef %s_H\n"
			"#define %s_H\n"
			"\n"
			"#ifndef CLASSES_CLASSES_H\n"
			"#include \"Classes.h\"\n"
			"#endif /* CLASSES_CLASSES_H */\n"
			"\n"
			"#define DECLARE(method)  ULONG m_%s_## method( struct IClass *cl, Object *obj, struct MUIP_%s_## method *msg )\n"
			"#define OVERLOAD(method) ULONG m_%s_## method( struct IClass *cl, Object *obj, Msg msg )\n"
			"#define ATTR(attr)       case MUIA_%s_## attr\n"
			"\n"
			"/* Exported CLASSDATA */\n"
			"\n"
			"struct Data\n"
			"{\n"
			"%s\n"
			"};\n"
			"\n"
			"ULONG %sGetSize( void ) { return sizeof(struct Data); }\n"
			"\n"
			"#endif /* %s_H */\n",
				buf, buf, className, className, className, className, nextcd->classdata, className, buf
		);

		*MyPathPart(destBuf) = 0;
		fclose(fp);
	}	
	return 1;
}

/*******************************************************************************
 *
 * Makefile generation
 *
 *
 * .p.s. I'm not an expert on makefiles, so someone improve this. ;-)
 *
 *******************************************************************************
 *
 *
 */ 

long GenMakefile( struct list *classlist )
{
	struct classdef *nextcd;
	char *cn, *p;
	FILE *fp;
	struct node *n;
	//char  arg_mkfile_dest      [256];
	char *arg_mkfile_cc        = NULL;
	char *arg_mkfile_outarg    = NULL;
	char *arg_mkfile_ccopts    = NULL;

	if (!arg_mkfile[0]) return 1; /* Caller doesn't want a makefile */

	/* FORMAT: -mkfileVMakefile,vc,-o,-O3,-+ */

	if ((arg_mkfile_cc = strchr(arg_mkfile, ',')))
	{
		*arg_mkfile_cc++ = 0;

		if ((arg_mkfile_outarg = strchr(arg_mkfile_cc, ',')))
		{
			*arg_mkfile_outarg++ = 0;

			if ((arg_mkfile_ccopts = strchr(arg_mkfile_outarg, ',')))
			{
				*arg_mkfile_ccopts++ = 0;
			}
		}
	}
	
	/* Use defaults for things we didn't get... */

	if (!arg_mkfile_cc)     arg_mkfile_cc     = "cc";
	if (!arg_mkfile_outarg) arg_mkfile_outarg = "-o";
	if (!arg_mkfile_ccopts) arg_mkfile_ccopts = "";

	//strncpy(arg_mkfile_dest,   destdir,    255);
	//MyAddPart(arg_mkfile_dest, arg_mkfile, 255);

	for (p = arg_mkfile_ccopts;;)
	{
		if ((p = strchr(p, ',')))
			*p++ = ' ';
		else
			break;
	}

	printf("Creating makefile  : %s\n", arg_mkfile);

	if (!(fp = fopen(arg_mkfile, "w")))
	{
		printf("ERROR: Unable to open %s\n", arg_mkfile);
		return 0;
	}

	fprintf(fp,
		"#\n"
		"# " EDIT_WARNING "\n"
		"#\n"
	);

	fprintf(fp,
		"\nCC = %s\n"
		"CCOPTS = %s\n\n", arg_mkfile_cc, arg_mkfile_ccopts
	);

	fprintf(fp, "all.o : %s ", OBJECT_NAME);

	for (n = NULL; (n = list_getnext(classlist, n, (void **) &nextcd));)
	{
		fprintf(fp, "%s.o ", nextcd->name);
	}

	fprintf(fp, "\n\tjoin " OBJECT_NAME " ");

	for (n = NULL; (n = list_getnext(classlist, n, (void **) &nextcd));)
	{
		fprintf(fp, "%s.o ", nextcd->name);
	}

	fprintf(fp, "AS all.o\n\n");

	fprintf(fp,
		"%s : %s %s ",
		OBJECT_NAME,
		SOURCE_NAME,
		HEADER_NAME
	);
	
	fprintf(fp,
		"\n\t$(CC) %s %s %s $(CCOPTS)\n\n",
		SOURCE_NAME,
		arg_mkfile_outarg,
		OBJECT_NAME
	);

	for (n = NULL; (n = list_getnext(classlist, n, (void **) &nextcd));)
	{
		cn = nextcd->name;
		fprintf(fp,
			"%s.o : %s.c \n" /* %s.h */
			"\t$(CC) %s.c %s %s.o $(CCOPTS)\n\n",
			cn, cn, cn, arg_mkfile_outarg, cn
		);
	}

	fclose(fp);
	return 1;
}

/*******************************************************************************
 *
 * Argument processing and entry point
 *
 *******************************************************************************
 *
 *
 */ 

long GetStrArg( char *argid, char *argline, char *dest, long destlen )
{
	char *p;
	size_t arglen = strlen(argid);

	if (strncmp(argid, argline, arglen) != 0)
		return 0;

	p = &argline[arglen];
	strncpy(dest, p, destlen);
	return 1;
}

long GetBlnArg( char *argid, char *argline, long *blnlong )
{
	if (strncmp(argid, argline, strlen(argid)) != 0)
		return 0;

	*blnlong = 1;
	return 1;
}

long DoArgs( int argc, char *argv[] )
{
	long i, success;

	if (argc < 2)
	{
		printf(
	"Usage: GenClasses <classdir> -b<basename> <options>\n"
   	"\n"
	"Options:\n"
	"\n"
	" -b<basename>                                 - basename (.i.e. YAM) used in sources (required)\n"
	" -gpl                                         - write GPL headers onto sources\n"
	" -storm                                       - include Storm/GoldED fold markers\n"
	" -i<includes>                                 - includes for Classes.h\n"
	"                                                (.i.e. -i\"YAM.h\",\"YAM_hook.h\",<stdlib.h>\n"
	" -mkfile<makefile>,<cc>,<outarg>,<ccopts,...> - Create a makefile\n"
	"                                                (.i.e. -mkfileVMakefile,vc,-o,-O3,-+\n"
	" -matchprefix<prefix>                         - Only process files with this prefix\n"
	" -destdir<dir>                                - Use an alternative dir for storing generated sources.\n"
	"                                                The default behaviour is to place them in <classdir>.\n"
	" -external                                    - External MCC mode (unimplemented ATM)\n"
		);

		return 0;
	}

	arg_me = argv[0];
	arg_gpl = 0;

	for (i = 1; i < argc; i++)
	{
		if (GetStrArg("-b",           argv[i], arg_basename,     255)) continue;
		if (GetStrArg("-i",           argv[i], arg_includes,     255)) continue;
		if (GetBlnArg("-gpl",         argv[i], &arg_gpl             )) continue;
		if (GetBlnArg("-storm",       argv[i], &arg_storm           )) continue;
		if (GetStrArg("-mkfile",      argv[i], arg_mkfile,       255)) continue;
		if (GetBlnArg("-v",           argv[i], &arg_v               )) continue;
		if (GetBlnArg("-verbose",     argv[i], &arg_v               )) continue;
		if (GetStrArg("-matchprefix", argv[i], arg_matchprefix,  255)) continue;
		if (GetStrArg("-destdir",     argv[i], arg_destdir,      255)) continue;
		if (GetBlnArg("-external",    argv[i], &arg_external        )) continue;
	}

	success = 1;
	strncpy(arg_classdir, argv[1], 255);

	if (arg_classdir[0] == 0 || arg_classdir[0] == '-')
	{
		printf("No/Empty class dir name specified, reading classes from current dir.\n");
		strcpy(arg_classdir, CURRENT_DIR);
	}

	if (!arg_basename[0])
	{
		success = 0;
		printf("ERROR: You MUST provide a basename using the -b argument\n");
	}

	return success;
}

int main( int argc, char *argv[] )
{
	struct node *n;
	struct list classlist;

	list_init(&classlist);
	printf("GenClasses %s by Andrew Bell <mechanismx@lineone.net>\n\n", verstr);

	if (!DoArgs(argc, argv))
	{
		return 0;
	}

	/* Get memory for the hash */
	if(!(collision_cnts = calloc(BUCKET_CNT, sizeof(long))))
	{
		return 0;
	}

	if (arg_external)
	{
		/* We're in external mode.  Every dir found in the classes
		   dir will represent an external MCC. The dir name represents
		   the actual MCC name.
		   Inside these dirs must be a Sourcefile of the same name.
		   So if the dir name is MyClass.mcc, there should be a source called
		   MyClass.c. A file called MyClass_auto.(c|h) will be created that
		   contains....
   
		   NOTE: This was never finished to do lack of time. Perhaps one day.
		*/
	}
	else
	{
		/* We're in normal mode */

		if (!arg_destdir[0])
		{
			// User hasn't given us a destination dir
			// So use the class dir.
			strncpy(arg_destdir, arg_classdir, sizeof(arg_destdir));
		}

		if (ScanClasses(arg_classdir, &classlist))
		{
			// Make Classes.c

			MyAddPart(arg_destdir, SOURCE_NAME, 255);
			GenMainSource(arg_destdir, &classlist);
			*MyPathPart(arg_destdir) = 0;

			// Make Classes.h

			MyAddPart(arg_destdir, HEADER_NAME, 255);
			GenMainHeader(arg_destdir, &classlist);
			*MyPathPart(arg_destdir) = 0;

			// Make each MCC header

			GenIndividualHeaders(&classlist, arg_destdir);

			// Create the makefile

			GenMakefile(&classlist);
		}
	}

	while ((n = list_remhead(&classlist)))
	{
		FreeClassDef(n->data);
		free(n);
	}

	free(collision_cnts);

	return EXIT_SUCCESS;
}

