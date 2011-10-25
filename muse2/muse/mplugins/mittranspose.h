//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mittranspose.h,v 1.1.1.1 2003/10/27 18:52:40 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#ifndef __MITTRANSPOSE_H__
#define __MITTRANSPOSE_H__

#include "mitplugin.h"
#include "ui_mittransposebase.h"
#include <list>

class QCloseEvent;

namespace MusECore {

class Xml;

struct KeyOn {
      unsigned char pitch;
      unsigned char channel;
      unsigned char port;
      char transpose;
      KeyOn(int a, int b, int c, int d) {
            pitch = a;
            transpose = d;
            channel = b;
            port = c;
            }
      };

typedef std::list<KeyOn > KeyOnList;
typedef KeyOnList::iterator iKeyOn;

} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   MITPluginTranspose
//---------------------------------------------------------

class MITPluginTranspose : public QWidget, public Ui::MITTransposeBase, public MusECore::MITPlugin {
      Q_OBJECT

      MusECore::KeyOnList keyOnList;
      int transpose;    // current pitch offset
      int trigger;
      bool on;
      bool transposeChangedFlag;

      void transposeChanged();
      virtual void closeEvent(QCloseEvent*);

   signals:
      void hideWindow();

   private slots:
      void onToggled(bool);
      void triggerKeyChanged(int);
      void noteReceived();

   public:
      MITPluginTranspose(QWidget* parent = 0, Qt::WFlags fl = 0);
      virtual void process(MusECore::MEvent&);
      virtual void readStatus(MusECore::Xml&);
      virtual void writeStatus(int, MusECore::Xml&) const;
      };

} // namespace MusEGui

namespace MusEGlobal {
extern MusEGui::MITPluginTranspose* mitPluginTranspose;
}

#endif

