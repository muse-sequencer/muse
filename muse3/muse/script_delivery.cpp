//=========================================================
//  MusE
//  Linux Music Editor
//
//  script_delivery.cpp
//  (C) Copyright 2019 Tim E. Real (terminator356 on sourceforge)
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

#include "script_delivery.h"

namespace MusECore {
  
void ScriptReceiver::receiveExecDeliveredScript(int id)
{
  execDeliveredScriptReceived(id);
}
  
void ScriptReceiver::receiveExecUserScript(int id)
{
  execUserScriptReceived(id);
}
  
} // namespace MusECore
