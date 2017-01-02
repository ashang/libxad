/*
 $Id: vx_include.h,v 1.2 2004/01/25 15:12:58 andrewbell Exp $
 Place stuff that is common to all modules and classes in here.

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

#ifndef VX_INCLUDE_H
#define VX_INCLUDE_H

/***************************************************************************/

#define YEAR             "2000-2004"
#define EMAILADDY        "andyb@starforge.co.uk"
#define WEBADDY          "http://andyb.starforge.co.uk"

////#define DEBUG /* Compile with all the debug code active? TODO: Offload this to the makefile */

//#define __NOLIBBASE__

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef VX_ALLHEADERS_H
#include "vx_allHeaders.h"
#endif

#ifndef VX_COMPILERDEFS_H
#include "vx_compilerDefs.h"
#endif

#include "vx_rev.h"

#ifndef CLASSES_CLASSES_H
#include "Classes.h"
#endif

#ifndef VX_GLOBAL_H
#include "vx_global.h"
#endif

/***************************************************************************/
/* Assorted */
/***************************************************************************/


/* These macros will be adapted in the future so dynamic "on the fly" window object
   creation can be introduced. Try to cache the results returned by these macros,
   instead of using them many times inside the same routine. Doing so will reduce
   bloat, also these macros might get larger in the future. */

#define GETMAINWIN        (MainWin        ? MainWin        : (MainWin        = (Object *) xget(App, MUIA_VX_MainWin)))
#define GETSETTINGSWIN    (SettingsWin    ? SettingsWin    : (SettingsWin    = (Object *) xget(App, MUIA_VX_SettingsWin)))
#define GETSEARCHWIN      (SearchWin      ? SearchWin      : (SearchWin      = (Object *) xget(App, MUIA_VX_SearchWin)))
#define GETERRWIN         (ErrorWin       ? ErrorWin       : (ErrorWin       = (Object *) xget(App, MUIA_VX_ErrorWin)))
#define GETPROGRESSWIN    (ProgressWin    ? ProgressWin    : (ProgressWin    = (Object *) xget(App, MUIA_VX_ProgressWin)))
#define GETGETSTRWIN      (GetStringWin   ? GetStringWin   : (GetStringWin   = (Object *) xget(App, MUIA_VX_GetStringWin)))
#define GETPATWIN         (GetPatternWin  ? GetPatternWin  : (GetPatternWin  = (Object *) xget(App, MUIA_VX_GetPatternWin)))
#define GETARCINFOWIN     (ArcInfoWin     ? ArcInfoWin     : (ArcInfoWin     = (Object *) xget(App, MUIA_VX_ArcInfoWin)))
#define GETMULTIPARTWIN   (MultiPartWin   ? MultiPartWin   : (MultiPartWin   = (Object *) xget(App, MUIA_VX_MultiPartWin)))
#define GETABOUTWIN       (AboutWin       ? AboutWin       : (AboutWin       = (Object *) xget(App, MUIA_VX_AboutWin)))
#define GETXADINFOWIN     (XADInfoWin     ? XADInfoWin     : (XADInfoWin     = (Object *) xget(App, MUIA_VX_XADInfoWin)))
#define GETNEWFILETYPEWIN (NewFileTypeWin ? NewFileTypeWin : (NewFileTypeWin = (Object *) xget(App, MUIA_VX_NewFileTypeWin)))
#define GETVERSIONWIN     (VersionWin     ? VersionWin     : (VersionWin     = (Object *) xget(App, MUIA_VX_VersionWin)))
#define GETEXTFILEINFOWIN (ExtFileInfoWin ? ExtFileInfoWin : (ExtFileInfoWin = (Object *) xget(App, MUIA_VX_ExtFileInfoWin)))
#define GETBUSYWIN        (BusyWin        ? BusyWin        : (BusyWin        = (Object *) xget(App, MUIA_VX_BusyWin)))

#define GETARCHISTORY     (ArcHistory     ? ArcHistory     : (ArcHistory     = (Object *) xget(GETMAINWIN, MUIA_MainWin_ArcHistory)))
#define GETARCVIEW        (ArcView        ? ArcView        : (ArcView        = (Object *) xget(GETMAINWIN, MUIA_MainWin_ArcView)))

/* These will probably change, hence the reason they're not cached in a global variable yet. */
#define GETFILETYPESOBJ ((Object *) xget(GETSETTINGSWIN, MUIA_SettingsWin_FTObject))
#define GETVXTOOLBAROBJ ((Object *) xget(GETMAINWIN,     MUIA_MainWin_VXToolBar))

/* Reduces typing ;-) */
#define INITAH Object *ah = GETARCHISTORY
#define INITAV Object *av = GETARCVIEW
#define INITFT Object *ft = GETFILETYPESOBJ

/* Move this to vx_compilerDefs.h */
#ifndef __FUNC__
#define __FUNC__ "<FunctionNameUnknown>"
#endif

#define STR(id, str)     str

#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d)	\
	((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

#define HOOK(hs, hf) static struct Hook hs = { { NULL, NULL }, (void *) &hf, NULL, NULL };
#define HK(hn) static struct Hook hn ## _Hook = { { NULL, NULL }, (void *) &hn ## _Func, NULL, NULL };


/***************************************************************************/
/* vx_gui.c */
/***************************************************************************/

#define INITIAL_ASLREQ_WIDTH     440
#define INITIAL_ASLREQ_HEIGHT    512
#define INITIAL_ASLREQ_TOPEDGE   50
#define INITIAL_ASLREQ_LEFTEDGE  50

#define TXT_CENTRE     "\33c"
#define TXT_BOLD       "\33b"
#define TXT_CENTER     "\33c"
#define TXT_UNDERLINED "\33u"
#define TXT_OFF        "\33n"
#define TXT_ITALICS    "\33i"
#define TXT_MAINTITLE  TXT_CENTRE TXT_BOLD TXT_UNDERLINED
#define TXT_SUBTITLE   TXT_CENTRE TXT_BOLD TXT_ITALICS


#define MID_PORTION_START 50000
#define MID_RECENT_START  55000
#define MID_RECENT_NULL   (MID_RECENT_START-1)

/******** Main window ********/

#define HELP_MAIN_DESTPOP         "Press this to access the destination history list."
#define HELP_MAIN_ARCPOP          "Press this to access the archive history list."
#define HELP_MAIN_ARCPOPSTR       "This is the path of the currently loaded archive."
#define HELP_MAIN_PREVARC         "Go to previous archive."
#define HELP_MAIN_NEXTARC         "Go to next archive."
#define HELP_MAIN_POPASL          "Press this to load one or more archives."
#define HELP_MAIN_AELISTVIEW      "The contents of the currently loaded archive."
#define HELP_MAIN_AETREE_OPENALL  "Pressing this will open up all the tree's nodes."
#define HELP_MAIN_AETREE_CLOSEALL "Pressing this will close all the tree's nodes."
#define HELP_MAIN_DESTPOPSTR      "Files will be extracted to this location.\n" \
                                  "Sub-directories will be automatically created if they don't exist."
#define HELP_MAIN_DESTPOPASL      "Open up a drawer requester to select destination path."	
#define HELP_MAIN_STATUS          "Status and information."
#define HELP_MAIN_SELECTALL       "Select all entries."
#define HELP_MAIN_SELECTNONE      "Clear all selected entries."
#define HELP_MAIN_SELECTPAT       "Open the pattern selection window."
#define HELP_MAIN_EXTRACTALL      "Extract all files in the archive."
#define HELP_MAIN_EXTRACTSELECTED "Extract only the selected entries."
#define HELP_MAIN_QUIT            "Quit the program."
#define HELP_MAIN_ROOT            "Go to the root of the archive."
#define HELP_MAIN_PARENT          "Go to the parent directory."
#define HELP_MAIN_NARROWPARENT    HELP_MAIN_PARENT
#define HELP_MAIN_ARCLOCATION     "Your location within the archive."

/******** Settings window ********/

#define HELP_ST_SAVE                 "Save these settings."
#define HELP_ST_CANCEL               "Close the settings window."
#define HELP_ST_FTYPES_ADD           "Add a new filetype."
#define HELP_ST_FTYPES_REM           "Remove the currently selected filetype."
#define HELP_ST_FTYPES_LISTVIEW      "Filetypes/viewer configuration."
#define HELP_ST_FTYPES_COMMENTSTR    "Enter comment string here."
#define HELP_ST_FTYPES_VIEWERPOPASL  "Select a viewer via a file requester."
#define HELP_ST_FTYPES_VIEWERSTR     "Enter path of viewer here and its arguments. Use %fp to tell\nVX where to insert the file path."
#define HELP_ST_FTYPES_PATTERN       "Enter match pattern that will invoke viewer."
#define HELP_ST_FTYPES_STATECYCLE    "Use this to disable and enable filetypes at will."

#define HELP_ST_XAD_NOEXTXADCLIENTS  "Disable all external XAD clients.\n" \
									 "\n" \
									 "Warning: Only tick this if you know what\n" \
									 "you're doing! Some archive formats (like LZX)\n" \
									 "will not be recognized if this is ticked!\n" \
									 "\n" \
									 "Disabling external clients can be handy if you\n" \
									 "encounter a misbehaving 3rd party client that\n" \
									 "crashes your machine."

#define HELP_ST_MAIN_AUTOSELECTALL   "When you load a new archive, all of its entries\n" \
                                     "will be automatically selected, if this is ticked."

#define HELP_ST_MAIN_AUTOVIRUSCHECK  "When ticked, every single file extracted by\n" \
                                     "VX will be scanned for viruses.\n" "\n" \
                                     "Note: This feature might slow down VX,\n" \
                                     "it also requires xvs.library to be installed."

#define HELP_ST_MAIN_KEEPFULLPATH    "Keep the full path intact when extracting.\n" \
									 "\n" \
									 "If you extract some selected files in a sub directory\n" \
									 "you might want to extract the parent directory names\n" \
									 "too, if so, tick this option.\n" \
									 "\n" \
                                     "Only applies to \"Extract selected\" and will\n" \
                                     "be ignored if the view mode is set to \"Flat\"."

#define HELP_ST_MAIN_DEFARCPATH_STR  "Default archive directory, used in the file\n" \
									 "requesters when VX starts up for the\n" \
									 "first time (without any archives loaded)."


#define HELP_ST_MAIN_DEFDESTPATH_STR "Default destination directory, used when\n" \
									 "VX starts up for the first time."

#define HELP_ST_MAIN_DEFDESTPATH_POPASL  "Open up an ASL drawer requester."
#define HELP_ST_MAIN_DEFARCPATH_POPASL   "Open up an ASL drawer requester."
#define HELP_ST_MAIN_TEMPPATH_POPASL     "Open up an ASL drawer requester."
#define HELP_ST_MAIN_TEMPPATH_STR        "Select a location where temporary files\n" \
                                         "should be saved to (Note: T: should suffice\n" \
                                         "in most cases)."
#define HELP_ST_MAIN_ARCVIEWMODE         "View mode for archive entry list."

/******** Pattern window ********/

#define HELP_PAT_STRING                  "Type in an AmigaDOS wildcard pattern here."
#define HELP_PAT_SELECTPAT               "Select entries, according to the pattern gadget."
#define HELP_PAT_CLEARALL                "Quickly clear all selected entries."

/******** XAD client info window ********/

#define HELP_CI_LISTVIEW                 "This is a list of all XAD's clients."

/******** Multipart window ********/

#define HELP_MP_LISTVIEW                 "Add the different parts of the archive to this list."
#define HELP_MP_CANCEL                   "Cancel and don't view the archive."
#define HELP_MP_INFO                     "Information."
#define HELP_MP_VIEW                     "After you've added all of the archive segments,\n" \
                                         "use this to view the archive."
#define HELP_MP_ADD                      "Add another archive segment to the list."
#define HELP_MP_REMOVE                   "Remove the currently selected archive segment from the list."

/******** Password window ********/

#define HELP_PW_INFO                     "Information."
#define HELP_PW_STRING                   "Enter a password here."
#define HELP_PW_ACCEPT                   "Accept the above password."
#define HELP_PW_CANCEL                   "Cancel this window."
#define HELP_ABOUT_EXIT                  "Close the About Window."
#define HELP_ABOUT_VERSION               "Current date and version of VX."
#define HELP_ABOUT_LISTVIEW              "Information on the program."

/******** Error window ********/

#define HELP_ER_LISTVIEW                 "List of recent errors."
#define HELP_ER_CLEARERRORS              "Clear error lister."
#define HELP_ER_SAVEERRORS               "Save errors as ASCII text."
#define HELP_ER_EXIT                     "Close error window."

/******** Search window ********/

#define HELP_SEARCH_LISTVIEW             "Search results. Double click on an entry to launch viewer."
#define HELP_SEARCH_GAUGE                "Progress of search."
#define HELP_SEARCH_STOP                 "Press this to stop the search."
#define HELP_SEARCH_EXIT                 "Close the search window."
#define HELP_SEARCH_STARTSTRINGSEARCH    "Begin a string search."
#define HELP_SEARCH_SEARCHEMBEDDED_TICK  "Search archives inside archives."
#define HELP_SEARCH_ARCTOSEARCH          "What archive do you want to search?"
#define HELP_SEARCH_STRING_STRING        "The string to search for."
#define HELP_SEARCH_STRING_UCEQLC        "Treat uppercase letters and lowercase letters as\n" "the same when searching."
#define HELP_SEARCH_FILENAME_PATTERN     "Enter an AmigaDOS wildcard search pattern here."
#define HELP_SEARCH_FILENAME_STARTSEARCH "Start a filename search."
#define HELP_SEARCH_OPENARCHIVE          "Open the archive for browsing, in the main window."
#define HELP_SEARCH_EXTRACTFILE          "Quickly extract the selected file to disk."

/***************************************************************************/
/* vx_str.c */
/***************************************************************************/

enum
{
	SID_END   = -1,
	SID_START = 0,

	SID_ERR_NOMEM,               /* Out of memory! */
	SID_ERR_NOSTACKMEM,          /* Failed to allocate stack memory! */
	SID_ERR_CPU,                 /* I need at least a MC68020 processor! */
	SID_ERR_OS,                  /* VX requires OS 3.0 or better */
	SID_ERR_ARGS,                /* Failed to read Shell/CLI arguments */
	SID_ERR_VX,                  /* VX Error */
	SID_ERR_DOS,                 /* VX DOS error */
	SID_ERR_DOSFAILED,           /* Encountered a problem with dos.library! */
	SID_ERR_DOSCODE,             /* DOS error code */
	SID_ERR_LIBS,                /* Unable to open the following libraries: */
	SID_ERR_DELTMPFILES,         /* Failed to delete %lu temporary file(s) in your temporary dir!
		                              I suggest you delete these later, after you quit the programs that are using them. */
	SID_ERR_CREATEDESTDIR,       /* Couldn't create destination directory! */
	SID_ERR_EXTRACT,             /* Extract error... */
	SID_ERR_LINKSNOTSUPP,        /* Sorry, this version of VX does not extract links. */
	SID_ERR_EXTFILES,            /* Extracted %ld files. */
	SID_ERR_ISAFILE_NEEDADIR,    /* '%s' is a file, I need a directory! */
	SID_ERR_LOADINGARC,          /* Error while loading archive '%s'! */
	SID_ERR_XADMASTER,           /* %s\nxadmaster.library error : %s */
	SID_ERR_NOPARENTARC,         /* Couldn't load parent archive! */
	SID_ERR_NDOSDISK,            /* Non-DOS disk encountered! */
	/*SID_ERRXAD_GDI,*/              /* xadGetDiskInfo() failed */
	SID_ERRXAD_GDI_WITH,         /* xadGetDiskInfo() failed on %s */
	SID_ERRXAD_CD_WITH,          /* xadConvertDates() failed on %s */
	SID_ERRXAD_DFU_WITH,         /* xadDiskFileUnArc() failed on %s */
	SID_ERRXAD_FU_WITH,          /* xadFileUnArc() failed on %s */

	SID_COPYFILE,                /* Please wait, copying file data... */
	SID_COPYFILE_FAILED,         /* Error while copying file data! */
	SID_COPYFILE_OK,             /* Finished copying file data.  */
	SID_OVERWRITE_REQUEST,       /* Overwrite request... */
	SID_OVERWRITE_BODY,          /* File %s already exists, overwrite it? */
	SID_OVERWRITE_GADGETS,       /* Yes, overwrite it|No, don't overwrite it */

	SID_SETTINGS_INVALID,        /* Invalid settings file! Using defaults. */
	SID_SETTINGS_SAVING,         /* Saving settings... */
	SID_SETTINGS_SAVED,          /* Settings saved. */

	SID_EXTANDVIEW,              /* Please wait, extracting and viewing... */
	SID_EXTFILESDIRS_FMT,        /* Please wait, extracting %lu files and dirs... */
	SID_EXTFILESDIRS,            /* Please wait, extracting files and dirs... */

	SID_OK,                      /* OK */ 
	SID_CANCEL,                  /* Cancel */ 
	SID_RETRY,                   /* Retry */ 
	SID_SKIPIT,                  /* Skip it */
	SID_ABORT,                   /* Abort */
	SID_ADDARCSTOLIST,           /* Adding archives to history list... */
	SID_VERSION,                 /* version */
	SID_LINKPOINTSTO,            /* Link points to: %s */
	SID_PASSWORDREQ,             /* %s requires a password */
	SID_FEATURE_REQS,            /* Sorry, this feature requires %s version %lu. */

	SID_MAX
};

#endif /* VX_INCLUDE_H */





