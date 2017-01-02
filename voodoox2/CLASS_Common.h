/*
 $Id: CLASS_Common.h,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Place anything that is common to all classes in this file.

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

#ifndef CLASS_COMMON_H
#define CLASS_COMMON_H

#ifndef VX_INCLUDE_H
#include "vx_include.h"
#endif

#ifndef VX_COMPILERDEFS_H
#include "vx_compilerDefs.h"
#endif

#ifndef V_ARC_H
#include "vx_arc.h"
#endif

/* This included is needed for the VXToolBar struct exported by VXToolBar */
#ifndef SPEEDBAR_MCC_H
#include <mui/SpeedBar_mcc.h>
#endif

#ifndef LAMP_MCC_H
#include <mui/Lamp_mcc.h>
#endif


#define DATAREF ((struct Data *)INST_DATA(cl,obj))

#define nmTitle(txt, mid)                { NM_TITLE, txt, NULL, 0, 0, (APTR) mid },
#define nmItem(txt, commkey, flags, mid) { NM_ITEM, txt, commkey, flags, 0, (APTR) mid },
#define nmSub(txt, commkey, flags, mid)  { NM_SUB, txt, commkey, flags, 0, (APTR) mid },
#define nmSubBar                         { NM_SUB | NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
#define nmBar                            { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
#define nmEnd                            { NM_END, NULL, NULL, 0, 0, NULL }

#define MAX_RECENT_COUNTS 20
#define RECENT_LOCATION   "ENVARC:Voodoo-X/Voodoo-X.recent"

#define HOOKPROTO(name, ret, obj, param)   static SAVEDS ASM(ret) name( REG_A0 (struct Hook *hook), REG_A2 (obj),  REG_A1 (param) )
#define HOOKPROTONO(name, ret, param)      static SAVEDS ASM(ret) name( REG_A0 (struct Hook *hook), REG_A1 (param) )
#define HOOKPROTONH(name, ret, obj, param) static SAVEDS ASM(ret) name( REG_A2 (obj),               REG_A1 (param) )
#define HOOKPROTONHNO(name, ret, param)    static SAVEDS ASM(ret) name( REG_A1 (param) )
#define HOOKPROTONHNP(name, ret, obj)      static SAVEDS ASM(ret) name( REG_A2 (obj) )
#define HOOKPROTONHNONP(name, ret)         static SAVEDS ret name(void)
#define DISPATCHERPROTO(name)              static SAVEDS ASM(ULONG) name( REG_A0 (struct IClass *cl),  REG_A2 (Object *obj),  REG_A1 (Msg msg) )
#define MakeHook(hookname, funcname)       struct Hook hookname = { {NULL, NULL }, (HOOKFUNC) funcname, NULL, NULL }
#define MakeStaticHook(hookname, funcname) static struct Hook hookname = { {NULL, NULL}, (HOOKFUNC) funcname, NULL, NULL }
#define ENTRY(func)                        (APTR)func
#define InitHook(hook, orighook, data)     ((hook)->h_Entry = (orighook).h_Entry, (hook)->h_Data = (APTR)(data))

/* TODO: Move these to MUISupport.h */

#define BUTTON_METHOD(button, method) /* Call "method" when object "button" is clicked. */ \
	DoMethod(button, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, method);

#define CLOSEWIN_METHOD(winobj, method) /* Call "method" when object "winobj" is closed.*/ \
	DoMethod(winobj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 1, method);


// TODO: Rename this macro to STRINGACK_METHOD
#define STRING_METHOD(strobj, method)  /* Call "method" everytime object "strobj" is edited. */ \
	DoMethod(strobj, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 2, method, MUIV_TriggerValue);

#define STRINGCN_METHOD(strobj, method)  /* Call "method" everytime object "strobj" is edited. */ \
	DoMethod(strobj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 2, method, MUIV_TriggerValue);

#define NLIST_DCLICK_METHOD(listobj, method) /* Call "method" everytime object "listobj" is dbl clicked. */ \
	DoMethod(listobj, MUIM_Notify, MUIA_NList_DoubleClick, MUIV_EveryTime, obj, 2, method, MUIV_TriggerValue);

#define NLIST_SCLICK_METHOD(listobj, method) /* Call "method" everytime object "listobj" is sngl clicked. */ \
	DoMethod(listobj, MUIM_Notify, MUIA_NList_Active, MUIV_EveryTime, obj, 2, method, MUIV_TriggerValue);

#define NLISTTREE_DCLICK_METHOD(listobj, method) /* Call "method" everytime object "listobj" is dbl clicked. */ \
	DoMethod(listobj, MUIM_Notify, MUIA_NListtree_DoubleClick, MUIV_EveryTime, obj, 2, method, MUIV_TriggerValue);

#define NLISTTREE_SCLICK_METHOD(listobj, method) /* Call "method" everytime object "listobj" is sngl clicked. */ \
	DoMethod(listobj, MUIM_Notify, MUIA_NListtree_Active, MUIV_EveryTime, obj, 2, method, MUIV_TriggerValue);

#define LISTTREE_DCLICK_METHOD(listobj, method) /* Call "method" everytime object "listobj" is dbl clicked. */ \
	DoMethod(listobj, MUIM_Notify, MUIA_Listtree_DoubleClick, MUIV_EveryTime, obj, 2, method, MUIV_TriggerValue);

#define LISTTREE_SCLICK_METHOD(listobj, method) /* Call "method" everytime object "listobj" is sngl clicked. */ \
	DoMethod(listobj, MUIM_Notify, MUIA_Listtree_Active, MUIV_EveryTime, obj, 2, method, MUIV_TriggerValue);

#define CHECKMARK_METHOD(checkobj, method)  /* Call "method" everytime object "checkobj" is selected. */ \
	DoMethod(checkobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 2, method, MUIV_TriggerValue);

#define CYCLE_METHOD(cycobj, method) \
	DoMethod(cycobj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, obj, 2, method, MUIV_TriggerValue);


/* Only use these macros inside OM_NEW overloads.  */

#define CLRTMPDATA   memset(&data, 0, sizeof(struct Data))
#define DEFTMPDATA   struct Data data
#define PREPDATA     memcpy(INST_DATA(cl, obj), &data, sizeof(struct Data))

/* DEFTMPDATA allocates a temporary data structure on the stack. Since instance
   data cannot be wrote to until DoSuperNew() completes successfully, we
   write to this temporary data space. All data in this temporary spaces gets
   copied to the actual instance data of the object instance before the
   OM_NEW returns using the PREPDATA macro. Don't forget to clear the temporary
   data structure with CLRTMPDATA.
 
   Note: Only a few classes currently use this scheme. TODO: Adapt all
   classes to use it. */

#endif /* CLASS_COMMON_H */
