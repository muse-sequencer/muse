//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrledit.cpp,v 1.4.2.2 2009/02/02 21:38:00 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#include <stdio.h>
#include "ctrledit.h"
#include "ctrlcanvas.h"
#include "midieditor.h"
#include "xml.h"
#include "vscale.h"
#include "ctrlpanel.h"
#include "globals.h"
#include "midiport.h"
#include "instruments/minstrument.h"
#include "gconfig.h"

#include <QHBoxLayout>

namespace MusEGui {

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void CtrlEdit::setTool(int t)
      {
      canvas->setTool(t);
      }

//---------------------------------------------------------
//   CtrlEdit
//---------------------------------------------------------

CtrlEdit::CtrlEdit(QWidget* parent, MidiEditor* e, int xmag,
   bool expand, const char* name) : QWidget(parent)
      {
      setObjectName(name);
      setAttribute(Qt::WA_DeleteOnClose);
      QHBoxLayout* hbox = new QHBoxLayout;
      panel             = new CtrlPanel(0, e, "panel");
      canvas            = new CtrlCanvas(e, 0, xmag, "ctrlcanvas", panel);
      QWidget* vscale   = new MusEGui::VScale;

      hbox->setContentsMargins(0, 0, 0, 0);
      hbox->setSpacing (0);

      canvas->setOrigin(-(MusEGlobal::config.division/4), 0);

      canvas->setMinimumHeight(50);
      
      panel->setFixedWidth(CTRL_PANEL_FIXED_WIDTH);
      hbox->addWidget(panel,  expand ? 100 : 0, Qt::AlignRight);
      hbox->addWidget(canvas, 100);
      hbox->addWidget(vscale, 0);
      setLayout(hbox);

      connect(panel, SIGNAL(destroyPanel()), SLOT(destroy()));
      connect(panel, SIGNAL(controllerChanged(int)), canvas, SLOT(setController(int)));
      connect(canvas, SIGNAL(xposChanged(unsigned)), SIGNAL(timeChanged(unsigned)));
      connect(canvas, SIGNAL(yposChanged(int)), SIGNAL(yposChanged(int)));
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void CtrlEdit::writeStatus(int level, MusECore::Xml& xml)
      {
      if (canvas->controller()) {
            xml.tag(level++, "ctrledit");
            xml.intTag(level, "ctrlnum", canvas->controller()->num());
            xml.tag(level, "/ctrledit");
            }
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void CtrlEdit::readStatus(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "ctrl")
                              xml.parse1();  // Obsolete.
                        else if (tag == "ctrlnum") {
                              int num = xml.parseInt();
                              canvas->setController(num);
                              }
                        else
                              xml.unknown("CtrlEdit");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "ctrledit")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   destroy
//---------------------------------------------------------

void CtrlEdit::destroy()
      {
      emit destroyedCtrl(this);
      close();      // close and destroy widget
      }

//---------------------------------------------------------
//   setCanvasWidth
//---------------------------------------------------------

void CtrlEdit::setCanvasWidth(int w)
{ 
  canvas->setFixedWidth(w); 
}

void CtrlEdit::setController(int n)
{
  canvas->setController(n);
}

void CtrlEdit::setController(const QString& name)
{
  int portno = canvas->track()->outPort();
  MusECore::MidiPort* port = &MusEGlobal::midiPorts[portno];
  MusECore::MidiInstrument* instr = port->instrument();
  MusECore::MidiControllerList* mcl = instr->controller();

  for (MusECore::iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci) 
  {
    if (ci->second->name() == name) 
    {
      canvas->setController(ci->second->num());
      break;
    }
  }
}

} // namespace MusEGui
