//=========================================================
//  MusE
//  Linux Music Editor
//
//  script_delivery.h
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

#ifndef __SCRIPT_DELIVERY_H__
#define __SCRIPT_DELIVERY_H__

#include <QObject>

namespace MusECore {

class ScriptReceiver : public QObject
{
  Q_OBJECT
  public:
  ScriptReceiver() : QObject() { }
  void receiveExecDeliveredScript(int id);
  void receiveExecUserScript(int id);

  signals:
  // Dummy signals for lambda connection.
  void execDeliveredScriptReceived(int);
  void execUserScriptReceived(int);
};
  
} // namespace MusECore

#endif // __SCRIPT_DELIVERY_H__


