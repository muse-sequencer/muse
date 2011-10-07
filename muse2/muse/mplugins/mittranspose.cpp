//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mittranspose.cpp,v 1.2.2.1 2009/05/03 04:14:00 terminator356 Exp $
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

#include <QCloseEvent>
#include <QTimer>

#include "mittranspose.h"
#include "song.h"
#include "mpevent.h"
#include "pitchedit.h"
#include "xml.h"
#include "globals.h"

namespace MusEGlobal {
MusEGui::MITPluginTranspose* mitPluginTranspose;
}

namespace MusEGui {

//---------------------------------------------------------
//   MITPluginTranspose
//---------------------------------------------------------

MITPluginTranspose::MITPluginTranspose(QWidget* parent, Qt::WFlags fl)
    : QWidget(parent, fl)
      {
      setupUi(this);
      on        = false;
      transpose = 0;
      trigger   = 24;
      transposeChangedFlag = false;
      triggerKeySpinBox->setValue(trigger);

      onToggled(false);
      connect(onCheckBox, SIGNAL(toggled(bool)), SLOT(onToggled(bool)));
      connect(triggerKeySpinBox, SIGNAL(valueChanged(int)),
         SLOT(triggerKeyChanged(int)));
      connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(noteReceived()));
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MITPluginTranspose::closeEvent(QCloseEvent* ev)
      {
      emit hideWindow();
      QWidget::closeEvent(ev);
      }

//---------------------------------------------------------
//   noteReceived
//---------------------------------------------------------

void MITPluginTranspose::noteReceived()
    {
      if (transposeChangedFlag)
      {
            // Added by Tim. p3.3.6
            //printf("MITPluginTranspose::noteReceived\n");
            
            transposeChanged();
      }      
    }

//---------------------------------------------------------
//   triggerKeyChanged
//---------------------------------------------------------

void MITPluginTranspose::triggerKeyChanged(int val)
      {
      trigger = val;
      }

//---------------------------------------------------------
//   transposeChanged
//---------------------------------------------------------

void MITPluginTranspose::transposeChanged()
      {
      QString s;
      s.sprintf("%c%d", transpose >= 0 ? '-' : ' ', transpose);
      transposeLabel->setText(s);
      transposeChangedFlag = false;
      }

//---------------------------------------------------------
//   onToggled
//---------------------------------------------------------

void MITPluginTranspose::onToggled(bool f)
      {
      on = f;
      if (!on) {
            transpose = 0;
            transposeChanged();
            keyOnList.clear();
            }
      transposeLabel->setEnabled(on);
      triggerKeySpinBox->setEnabled(on);
      }

//---------------------------------------------------------
//   process
//---------------------------------------------------------

void MITPluginTranspose::process(MusECore::MEvent& ev)
      {
      if (!on || (ev.type() != 0x90))
            return;
      int pitch = ev.dataA();
      if (pitch >= trigger && pitch < (trigger+12)) {
            // process control keys
            int diff = transpose - (pitch-trigger);
            transpose -= diff;
            transposeChangedFlag = true;
            return;
            }
      if (ev.dataB() == 0) {
            // Note Off
            for (MusECore::iKeyOn i = keyOnList.begin(); i != keyOnList.end(); ++i) {
                  if (i->pitch == pitch && i->channel == ev.channel()
                     && i->port == ev.port()) {
                        pitch += i->transpose;
                        keyOnList.erase(i);
                        break;
                        }
                  }
            }
      else {
            // Note On
            keyOnList.push_back(MusECore::KeyOn(pitch, ev.channel(), ev.port(), transpose));
            pitch += transpose;
            }
      ev.setA(pitch);
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void MITPluginTranspose::readStatus(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::Text:
                        if (tag == "on")
                              on = xml.parseInt();
                        else if (tag == "trigger")
                              trigger = xml.parseInt();
                        else
                              xml.unknown("TransposePlugin");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (xml.s1() == "mplugin")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void MITPluginTranspose::writeStatus(int level, MusECore::Xml& xml) const
      {
      xml.intTag(level, "on", on);
      xml.intTag(level, "trigger", trigger);
      }

} // namespace MusEGui
