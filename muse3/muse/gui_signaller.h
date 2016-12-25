//=========================================================
//  MusE
//  Linux Music Editor
//
//  gui_signaller.h
//  (C) Copyright 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __GUI_SIGNALLER_H__
#define __GUI_SIGNALLER_H__

#include <QObject>

namespace MusECore {

class GuiSignaller : public QObject
{
  Q_OBJECT

  public:
    enum GuiSignalType { Command, AudioMessage };
    void sendSignal(int type, int signal) { emit wakeup(type, signal); }

  // The magic of automatic connection types:
  // ----------------------------------------
  // Sometimes this will be a direct connection (when sending initialization
  //  data from GUI thread for example), and sometimes this will be a queued
  //  connection (when sending user-adjusted controller values from the realtime
  //  audio thread for example).
  // Tested OK so far in debugger. It decouples just fine by itself when needed !
  //
  // Quote from http://wiki.qt.io/Threads_Events_QObjects:
  // "...the current Qt documentation is simply wrong when it states:
  //  ' Auto Connection (default) The behavior is the same as the Direct Connection,
  //     if the emitter and receiver are in the same thread. The behavior is the same
  //     as the Queued Connection, if the emitter and receiver are in different threads.'
  //  because the emitter object's thread affinity does not matter."
  signals:
    void wakeup(int type, int signal);
};


} // namespace MusECore

#endif
