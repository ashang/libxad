/*
 $Id: CLASS_VXToolBar_Defs.h,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Holds defines for toolbar custom class.

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

#ifndef VXTOOLBAR_DEFS_H
#define VXTOOLBAR_DEFS_H 1

enum /* Toolbar actions */
{
    /* Keep this enumeration in sync with the
       ToolbarActionKeywords[] and ToolbarActionEntries[]
       arrays in v_win_settings.c */

    TBACTION_SPACER = 0,
    TBACTION_SELECT_ALL,
    TBACTION_SELECT_NONE,
    TBACTION_SELECT_PAT,
    TBACTION_EXTRACT_ALL,
    TBACTION_EXTRACT_SEL,
    TBACTION_HIDE,
    TBACTION_QUIT,
    TBACTION_OPEN_ARC,
    TBACTION_OPEN_MP_ARC,
    TBACTION_CLOSE_ARC,
    TBACTION_CLOSE_ALL_ARCS,
    TBACTION_SEARCH,
    TBACTION_CHECK_FOR_VIRUSES,
    TBACTION_EDIT_SETT,
    TBACTION_EDIT_MUI_SETT,
    TBACTION_SHOW_XAD_INFO,
    TBACTION_SHOW_VER_INFO,
    TBACTION_SHOW_ERRORS,
    TBACTION_SHOW_ABOUT,
    TBACTION_SHOW_ABOUT_MUI,

    TBACTION_MAX
};

#define TOOLBAR_FILENAME      "ENVARC:VX/VX.toolbar"
#define TOOLBAR_IOBUFFER_SIZE (1024*8)
#define TOOLBAR_ID            "VXTOOLBAR"
#define TOOLBAR_HEADER        TOOLBAR_ID "\n;\n" "; Do not edit this file by hand!\n" ";\n"
#define TOOLBAR_SAVETEMPLATE  "LABEL=\"%s\" IMAGEPATH=\"%s\" ACTION=\"%s\"\n"
#define TOOLBAR_TEMPLATE      "LABEL/K/A,IMAGEPATH/K/A,ACTION/K/A"

#endif /* VXTOOLBAR_DEFS_H */
