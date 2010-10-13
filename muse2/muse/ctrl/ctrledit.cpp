//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrledit.cpp,v 1.4.2.2 2009/02/02 21:38:00 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#include <qlayout.h>
#include <qpainter.h>
#include <qtoolbutton.h>
#include <q3popupmenu.h>
#include <qlabel.h>
//Added by qt3to4:
#include <Q3HBoxLayout>

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
   bool expand, const char* name) : QWidget(parent, name)
      {
      Q3HBoxLayout* hbox = new Q3HBoxLayout(this);
      panel             = new CtrlPanel(this, e, "panel");
      canvas            = new CtrlCanvas(e, this, xmag, "ctrlcanvas", panel);
      QWidget* vscale   = new VScale(this);

      canvas->setOrigin(-(config.division/4), 0);

      canvas->setMinimumHeight(50);
      
      panel->setFixedWidth(CTRL_PANEL_FIXED_WIDTH);
      hbox->addWidget(panel,  expand ? 100 : 0, Qt::AlignRight);
      hbox->addWidget(canvas, 100);
      hbox->addWidget(vscale, 0);

      connect(panel, SIGNAL(destroyPanel()), SLOT(destroy()));
      connect(panel, SIGNAL(controllerChanged(int)), canvas, SLOT(setController(int)));
      connect(canvas, SIGNAL(xposChanged(unsigned)), SIGNAL(timeChanged(unsigned)));
      connect(canvas, SIGNAL(yposChanged(int)), SIGNAL(yposChanged(int)));
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void CtrlEdit::writeStatus(int level, Xml& xml)
      {
      if (canvas->controller()) {
            xml.tag(level++, "ctrledit");
            xml.strTag(level, "ctrl", canvas->controller()->name());
            xml.tag(level, "/ctrledit");
            }
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void CtrlEdit::readStatus(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "ctrl") {
                              QString name = xml.parse1();
                              int portno = canvas->track()->outPort();
                              MidiPort* port = &midiPorts[portno];
                              MidiInstrument* instr = port->instrument();
                              MidiControllerList* mcl = instr->controller();

                              for (iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci) {
                                    if (ci->second->name() == name) {
                                          canvas->setController(ci->second->num());
                                          break;
                                          }
                                    }
                              }
                        else
                              xml.unknown("CtrlEdit");
                        break;
                  case Xml::TagEnd:
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
      close(true);      // close and destroy widget
      }

//---------------------------------------------------------
//   setCanvasWidth
//---------------------------------------------------------

void CtrlEdit::setCanvasWidth(int w)
{ 
  canvas->setFixedWidth(w); 
}
