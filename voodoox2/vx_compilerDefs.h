/*
 $Id: vx_compilerDefs.h,v 1.1 2004/01/20 06:24:59 stoecker Exp $
 Compiler abstraction defines.

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

#ifndef VX_COMPILERDEFS_H
#define VX_COMPILERDEFS_H

/*********************************************************************
 *
 * Defines for making cross compiling that little bit easier.
 *
 *********************************************************************
 *
 */

#undef NULL
#define NULL ((void *)0)

/*********************************************************************
 *
 * GCC
 *
 *********************************************************************
 *
 */

#ifdef __GNUC__

	#undef SAVEDS
	#undef ASM

	#define SAVEDS
	#define ASM(x) x

	#define REG_D0(x) x __asm("d0")
	#define REG_D1(x) x __asm("d1")
	#define REG_D2(x) x __asm("d2")
	#define REG_D3(x) x __asm("d3")
	#define REG_D4(x) x __asm("d4")
	#define REG_D5(x) x __asm("d5")
	#define REG_D6(x) x __asm("d6")
	#define REG_D7(x) x __asm("d7")
	#define REG_A0(x) x __asm("a0")
	#define REG_A1(x) x __asm("a1")
	#define REG_A2(x) x __asm("a2")
	#define REG_A3(x) x __asm("a3")
	#define REG_A4(x) x __asm("a4")
	#define REG_A5(x) x __asm("a5")
	#define REG_A6(x) x __asm("a6")
	#define REG_A7(x) x __asm("a7")

#endif

/*********************************************************************
 *
 * VBCC
 *
 *********************************************************************
 *
 */

#ifdef __VBCC__
	#undef SAVEDS
	#define SAVEDS
	#undef ASM
	#define ASM(x) x

	#define REG_D0(x) __reg("d0") x
	#define REG_D1(x) __reg("d1") x
	#define REG_D2(x) __reg("d2") x
	#define REG_D3(x) __reg("d3") x
	#define REG_D4(x) __reg("d4") x
	#define REG_D5(x) __reg("d5") x
	#define REG_D6(x) __reg("d6") x
	#define REG_D7(x) __reg("d7") x
	#define REG_A0(x) __reg("a0") x
	#define REG_A1(x) __reg("a1") x
	#define REG_A2(x) __reg("a2") x
	#define REG_A3(x) __reg("a3") x
	#define REG_A4(x) __reg("a4") x
	#define REG_A5(x) __reg("a5") x
	#define REG_A6(x) __reg("a6") x
	#define REG_A7(x) __reg("a7") x
#endif

/*********************************************************************
 *
 * Maxon C++ and Hisoft C++
 *
 *********************************************************************
 *
 */

#ifdef __MAXON__
	#define REG_D0(x) register __d0 x
	#define REG_D1(x) register __d1 x
	#define REG_D2(x) register __d2 x
	#define REG_D3(x) register __d3 x
	#define REG_D4(x) register __d4 x
	#define REG_D5(x) register __d5 x
	#define REG_D6(x) register __d6 x
	#define REG_D7(x) register __d7 x
	#define REG_A0(x) register __a0 x
	#define REG_A1(x) register __a1 x
	#define REG_A2(x) register __a2 x
	#define REG_A3(x) register __a3 x
	#define REG_A4(x) register __a4 x
	#define REG_A5(x) register __a5 x
	#define REG_A6(x) register __a6 x
	#define REG_A7(x) register __a7 x
#endif

/*********************************************************************
 *
 * DICE
 *
 *********************************************************************
 *
 */

#ifdef _DCC
	#define REG_D0(x) __d0 x
	#define REG_D1(x) __d1 x
	#define REG_D2(x) __d2 x
	#define REG_D3(x) __d3 x
	#define REG_D4(x) __d4 x
	#define REG_D5(x) __d5 x
	#define REG_D6(x) __d6 x
	#define REG_D7(x) __d7 x
	#define REG_A0(x) __a0 x
	#define REG_A1(x) __a1 x
	#define REG_A2(x) __a2 x
	#define REG_A3(x) __a3 x
	#define REG_A4(x) __a4 x
	#define REG_A5(x) __a5 x
	#define REG_A6(x) __a6 x
	#define REG_A7(x) __a7 x
#endif

/*********************************************************************
 *
 * SAS/C
 *
 *********************************************************************
 *
 */


#ifdef __SASC
	#undef SAVEDS
	#undef ASM

	#define SAVEDS __saveds
	#define ASM(x) __asm x

	#define REG_D0(x) register __d0 x
	#define REG_D1(x) register __d1 x
	#define REG_D2(x) register __d2 x
	#define REG_D3(x) register __d3 x
	#define REG_D4(x) register __d4 x
	#define REG_D5(x) register __d5 x
	#define REG_D6(x) register __d6 x
	#define REG_D7(x) register __d7 x
	#define REG_A0(x) register __a0 x
	#define REG_A1(x) register __a1 x
	#define REG_A2(x) register __a2 x
	#define REG_A3(x) register __a3 x
	#define REG_A4(x) register __a4 x
	#define REG_A5(x) register __a5 x
	#define REG_A6(x) register __a6 x
	#define REG_A7(x) register __a7 x
#endif

#endif /* VX_COMPILERDEFS_H */
