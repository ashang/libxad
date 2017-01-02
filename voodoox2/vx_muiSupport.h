/*
 $Id: vx_muiSupport.h,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Assorted MUI support defines and macros.

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

/* MUI triggers/hook stuff. Created: Sun/5/Nov/2000 */

/* Install Button hook */
#define SET_TRIGGER_BT(butobj, hs, hf) { HOOK(hs, hf) \
		DoMethod(butobj, MUIM_Notify, MUIA_Pressed, FALSE, butobj, \
			3, MUIM_CallHook, &hs, MUIV_TriggerValue); }

/* NList Single Click hook */
#define SET_TRIGGER_NLSC(listobj, hs, hf) { HOOK(hs, hf) \
	DoMethod(listobj, MUIM_Notify, MUIA_NList_Active, MUIV_EveryTime, listobj, \
		3, MUIM_CallHook, &hs, MUIV_TriggerValue); }

/* NList Double Click hook */
#define SET_TRIGGER_NLDC(listobj, hs, hf) { HOOK(hs, hf) \
	DoMethod(listobj, MUIM_Notify, \
		MUIA_NList_DoubleClick, MUIV_EveryTime, listobj, \
		3, MUIM_CallHook, &hs, MUIV_TriggerValue); }

/* Listtree Single Click hook */
#define SET_TRIGGER_LTSC(listobj, hs, hf) { HOOK(hs, hf) \
	DoMethod(listobj, MUIM_Notify, \
		MUIA_Listtree_Active, MUIV_EveryTime, listobj, \
		3, MUIM_CallHook, &hs, MUIV_TriggerValue); }

/* Listtree Double Click hook */
#define SET_TRIGGER_LTDC(listobj, hs, hf) { HOOK(hs, hf) \
	DoMethod(listobj, MUIM_Notify, \
		MUIA_Listtree_DoubleClick, MUIV_EveryTime, listobj, \
		3, MUIM_CallHook, &hs, MUIV_TriggerValue); }

/* NListtree Single Click hook */
#define SET_TRIGGER_NLTSC(listobj, hs, hf) { HOOK(hs, hf) \
	DoMethod(listobj, MUIM_Notify, \
		MUIA_NListtree_Active, MUIV_EveryTime, listobj, \
		3, MUIM_CallHook, &hs, MUIV_TriggerValue); }

/* NListtree Double Click hook */
#define SET_TRIGGER_NLTDC(listobj, hs, hf) { HOOK(hs, hf) \
	DoMethod(listobj, MUIM_Notify, \
		MUIA_NListtree_DoubleClick, MUIV_EveryTime, listobj, \
		3, MUIM_CallHook, &hs, MUIV_TriggerValue); }


/* String Change Contents hook */
#define SET_TRIGGER_STCC(stobj, hs, hf) { HOOK(hs, hf) \
	DoMethod(stobj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, stobj, \
		3, MUIM_CallHook, &hs, MUIV_TriggerValue); }

/* String Acknowledge Contents hook */
#define SET_TRIGGER_STA(stobj, hs, hf) { HOOK(hs, hf) \
	DoMethod(stobj, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, stobj, \
		3, MUIM_CallHook, &hs, MUIV_TriggerValue); }

/* Check/Tickbox hook */
#define SET_TRIGGER_CB(cbobj, hs, hf) { HOOK(hs, hf) \
	DoMethod(cbobj, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, cbobj, \
		3, MUIM_CallHook, &hs, MUIV_TriggerValue); }

/* Close Window hook */
#define TRIGGER_WCL(winobj, hook) \
	DoMethod(winobj, MUIM_Notify, \
		MUIA_Window_CloseRequest, TRUE, winobj, \
		3, MUIM_CallHook, &hook, MUIV_TriggerValue);

/* Close Window (tell window to close itself) */
#define TRIGGER_WCLSELF( winobj ) \
	DoMethod(winobj, MUIM_Notify, \
		MUIA_Window_CloseRequest, TRUE, winobj, \
		3, MUIM_Set, MUIA_Window_Open, FALSE);

#define SET_TRIGGER_CYACT(cyobj, hs, hf) { HOOK(hs, hf) \
	DoMethod(cyobj, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, cyobj, \
		3, MUIM_CallHook, &hs, MUIV_TriggerValue); }



#define SimpleButtonTiny(name) \
  TextObject, \
    ButtonFrame, \
    MUIA_Font, MUIV_Font_Tiny, \
    MUIA_Text_Contents, name, \
    MUIA_Text_PreParse, "\33c", \
    MUIA_InputMode    , MUIV_InputMode_RelVerify, \
    MUIA_Background   , MUII_ButtonBack, \
  End


#define MyButton(Name, ShortHelp) \
	TextObject, \
		ButtonFrame, \
		MUIA_CycleChain,    1, \
		MUIA_ShortHelp,			ShortHelp, \
		MUIA_Font,					MUIV_Font_Button, \
		MUIA_Text_Contents,	Name, \
		MUIA_Text_PreParse, "\33c", \
		MUIA_InputMode,			MUIV_InputMode_RelVerify, \
		MUIA_Background,		MUII_ButtonBack, \
	End


#define MyButtonUD(Name, ShortHelp, ud) \
	TextObject, \
		ButtonFrame, \
		MUIA_CycleChain,    1, \
		MUIA_ShortHelp,			ShortHelp, \
		MUIA_Font,					MUIV_Font_Button, \
		MUIA_Text_Contents,	Name, \
		MUIA_Text_PreParse, "\33c", \
		MUIA_InputMode,			MUIV_InputMode_RelVerify, \
		MUIA_Background,		MUII_ButtonBack, \
		MUIA_UserData,      ud, \
	End



#define MyKeyButton(Name, CtrlKey, ShortHelp) \
	TextObject, \
		ButtonFrame, \
		MUIA_CycleChain,    1, \
		MUIA_ShortHelp,			ShortHelp, \
		MUIA_Font,					MUIV_Font_Button, \
		MUIA_Text_Contents,	Name, \
		MUIA_Text_PreParse, "\33c", \
		MUIA_Text_HiChar,		CtrlKey, \
		MUIA_ControlChar,		CtrlKey, \
		MUIA_InputMode,			MUIV_InputMode_RelVerify, \
		MUIA_Background,		MUII_ButtonBack, \
	End

#define MyMiniKeyButton(Name, CtrlKey, ShortHelp) \
	TextObject, \
		ButtonFrame, \
		MUIA_CycleChain,    1, \
		MUIA_ShortHelp,			ShortHelp, \
		MUIA_Font,					MUIV_Font_Button, \
		MUIA_Text_Contents,	Name, \
		MUIA_Text_PreParse, "\33c", \
		MUIA_Text_HiChar,		CtrlKey, \
		MUIA_ControlChar,		CtrlKey, \
		MUIA_InputMode,			MUIV_InputMode_RelVerify, \
		MUIA_Background,		MUII_ButtonBack, \
		MUIA_HorizWeight,   25, \
	End

#define MyCheckMark(State, CtrlKey, ShortHelp) \
	ImageObject, \
		ImageButtonFrame, \
		MUIA_CycleChain,      1, \
		MUIA_ShortHelp,			  ShortHelp, \
		MUIA_InputMode,				MUIV_InputMode_Toggle, \
		MUIA_Image_Spec,			MUII_CheckMark, \
		MUIA_Image_FreeVert,	TRUE, \
		MUIA_Selected,				State, \
		MUIA_Background,			MUII_ButtonBack, \
		MUIA_ShowSelState,		FALSE, \
		MUIA_ControlChar,			CtrlKey, \
	End


#define MyCheckMarkID(State, CtrlKey, ShortHelp, ID) \
	ImageObject, \
		ImageButtonFrame, \
		MUIA_ObjectID,			ID, \
		MUIA_CycleChain,		1, \
		MUIA_ShortHelp,			ShortHelp, \
		MUIA_InputMode,			MUIV_InputMode_Toggle, \
		MUIA_Image_Spec,		MUII_CheckMark, \
		MUIA_Image_FreeVert,	TRUE, \
		MUIA_Selected,			State, \
		MUIA_Background,		MUII_ButtonBack, \
		MUIA_ShowSelState,		FALSE, \
		MUIA_ControlChar,		CtrlKey, \
	End

#define ImageButton(ImgSpec, CtrlKey, ShortHelp) \
	ImageObject, \
		ImageButtonFrame, \
		MUIA_CycleChain,      1, \
		MUIA_Background,      MUII_ButtonBack, \
		MUIA_InputMode ,      MUIV_InputMode_RelVerify, \
		MUIA_Image_Spec,      ImgSpec, \
		MUIA_ControlChar,     CtrlKey, \
		MUIA_ShortHelp,       ShortHelp, \
		MUIA_Image_FontMatch, TRUE, \
	End
	
