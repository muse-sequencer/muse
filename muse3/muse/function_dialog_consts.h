//=========================================================
//  MusE
//  Linux Music Editor
//
//  function_dialog_base.h
//  (C) Copyright 2018 Tim E. Real (terminator356 on users dot sourceforge dot net)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __FUNCTION_DIALOG_CONSTS_H__
#define __FUNCTION_DIALOG_CONSTS_H__

namespace MusEGui {

//--------------------------------------------------------
// FunctionDialogElements: Types of common elements which
//  can be displayed in all function dialogs:
//--------------------------------------------------------
enum FunctionDialogElements
{
  FunctionDialogNoElements = 0x0000,

  //------------------------------
  // Buttons:
  //------------------------------
  FunctionAllEventsButton = 0x100,
  FunctionSelectedEventsButton = 0x200,
  
  FunctionLoopedButton = 0x400,
  FunctionSelectedLoopedButton = 0x800,
  
  FunctionAllPartsButton = 0x1000,
  FunctionSelectedPartsButton = 0x2000,
  
  FunctionDialogAllElements =
    FunctionAllEventsButton | FunctionSelectedEventsButton |
    FunctionLoopedButton | FunctionSelectedLoopedButton |
    FunctionAllPartsButton | FunctionSelectedPartsButton
};

// Combination of FunctionDialogElements flags.
typedef int FunctionDialogElements_t;


//------------------------------------------------
// FunctionDialogReturnFlags: Types of convenience
//  return flags common to all function dialogs
//  (individual dialog members can also be accessed):
//------------------------------------------------
enum FunctionDialogReturnFlags
{
  FunctionReturnNoFlags = 0x0,
  
  FunctionReturnAllEvents = 0x01,
  
  FunctionReturnLooped = 0x02,
  
  FunctionReturnAllParts = 0x04,
  
  FunctionReturnAllFlags = FunctionReturnAllEvents | FunctionReturnLooped |
    FunctionReturnAllParts
};

// Combination of FunctionDialogReturnFlags flags.
typedef int FunctionReturnDialogFlags_t;

} // namespace MusEGui

#endif



