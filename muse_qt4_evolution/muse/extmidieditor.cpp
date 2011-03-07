//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2004 Mathias Lundgren <lunar_shuttle@users.sourceforge.net>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __EXTMIDIEDITOR_CPP__
#define __EXTMIDIEDITOR_CPP__

#include "extmidieditor.h"
#include "part.h"
#include "al/xml.h"
#include "al/pos.h"
#include "song.h"
#include "midiedit/ecanvas.h"
#include "waveedit/waveview.h"
#include "esettings.h"
// #include "../ctrl/ctrledit.h"

//---------------------------------------------------------
//   GraphMidiEditor
//---------------------------------------------------------

GraphMidiEditor::GraphMidiEditor(PartList* pl)
   : MidiEditor(pl)
      {
      setIconSize(QSize(ICON_SIZE));
      _followSong = true;
      }

//---------------------------------------------------------
//   ~GraphMidiEditor
//---------------------------------------------------------

GraphMidiEditor::~GraphMidiEditor()
      {
      refreshSettings();
      }

//---------------------------------------------------------
//   newDefaultSettings
//---------------------------------------------------------

EditorSettings* GraphMidiEditor::newDefaultSettings()
      {
      return new GraphEditorSettings();
      }

//---------------------------------------------------------
//   initSettings
//---------------------------------------------------------

void GraphMidiEditor::initSettings()
      {
      MidiEditor::initSettings();
#if 0 //TD
      GraphEditorSettings* s = ((GraphEditorSettings*)(settings));
      hscroll->setXmag(s->xmag()); //horizontal zoom
      hscroll->setPos(s->pos());

      if (editorType != ET_WAVEEDIT)
            vscroll->setPos(((GraphEditorSettings*)settings)->ypos());
#endif
      }


//---------------------------------------------------------
//   refreshSettings
//---------------------------------------------------------
void GraphMidiEditor::refreshSettings() const
      {
      MidiEditor::refreshSettings();
//      if (editorType != ET_WAVEEDIT) //hACK! TODO: Separate waveedit class
//            ((GraphEditorSettings*)settings)->setYpos(vscroll->pos());
      }

//---------------------------------------------------------
//   raster
//---------------------------------------------------------
int GraphMidiEditor::raster() const
      {
      return settings->raster();
      }

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------
void GraphMidiEditor::setRaster(int val)
      {
      settings->setRaster(val);
//      canvas->setFocus();     // give back focus after kb input
      //_raster = val;
      }

//---------------------------------------------------------
//   rasterStep
//---------------------------------------------------------
int GraphMidiEditor::rasterStep(unsigned tick) const
      {
      return AL::sigmap.rasterStep(tick, settings->raster());
      }

//---------------------------------------------------------
//   rasterVal
//---------------------------------------------------------
unsigned GraphMidiEditor::rasterVal(unsigned v) const
      {
      return AL::sigmap.raster(v, settings->raster());
      }

//---------------------------------------------------------
//   rasterVal1
//---------------------------------------------------------
unsigned GraphMidiEditor::rasterVal1(unsigned v) const
      {
      return AL::sigmap.raster1(v, settings->raster());
      }

//---------------------------------------------------------
//   rasterVal2
//---------------------------------------------------------
unsigned GraphMidiEditor::rasterVal2(unsigned v) const
      {
      return AL::sigmap.raster2(v, settings->raster());
      }


//---------------------------------------------------------
//   ExtMidiEditor
//---------------------------------------------------------

ExtMidiEditor::ExtMidiEditor(PartList* pl)
   : GraphMidiEditor(pl)
      {
      _curDrumInstrument = -1;
      //printf("ExtMidiEditor, defaultSettings: %x\n",&defaultSettings);
      }

//---------------------------------------------------------
//   ~ExtMidiEditor
//---------------------------------------------------------

ExtMidiEditor::~ExtMidiEditor()
      {
      refreshSettings();
      }

//---------------------------------------------------------
//   updateCtrlEdits
//---------------------------------------------------------

void ExtMidiEditor::updateCtrlEdits() const
      {
#if 0 //TD
      ExtEditorSettings* s = (ExtEditorSettings*) settings;
      int j=0;
      for (std::list<CtrlEdit*>::const_iterator i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i, j++) {
            CtrlEdit* ctrlEdit = (*i);
            int ctrlid = ctrlEdit->controllerId();
            CtrlEditSettings* ctrl = new CtrlEditSettings(ctrlEdit->getPanelHeight(), ctrlid);
            s->setControlEditSettings(j, ctrl);
            }
#endif
      }

//---------------------------------------------------------
//   newDefaultSettings
//---------------------------------------------------------
EditorSettings* ExtMidiEditor::newDefaultSettings()
      {
      return (EditorSettings*) new ExtEditorSettings();
      }

//---------------------------------------------------------
//   initSettings
//---------------------------------------------------------

void ExtMidiEditor::initSettings()
      {
      GraphMidiEditor::initSettings();

      ExtEditorSettings* s = (ExtEditorSettings*) settings;
      srec->setChecked(s->steprec()); //set steprec
      midiin->setChecked(s->midiin()); //set midiin
      int ctrlsize = s->getControlEditSize();
      //Set to 0 again to not get the double amount of ctrledits:
      s->setControlEditSize(0);
      for (int i=0; i< ctrlsize; i++) {
            CtrlEditSettings* settings = s->getControlEditSettings(i);
            CtrlEdit* ctrlEdit = addCtrl();
//TD            ctrlEdit->setController(settings->getController());
            }
      QList<int> vl;
      vl.push_back(400);      // dummy: canvas height
      for (int i = 0; i < ctrlsize; i++) {
            CtrlEditSettings* settings = s->getControlEditSettings(i);
            vl.push_back(settings->getHeight());
            }
//TD      splitter->setSizes(vl);
      }

//---------------------------------------------------------
//   quantVal
//---------------------------------------------------------

int ExtMidiEditor::quantVal(int v) const
      {
      ExtEditorSettings* s = (ExtEditorSettings*) settings;
      //int val = ((v+_quant/2)/_quant)*_quant;
      int q = s->quant();
      int val = ((v+q/2)/q)*q;
      if (val == 0)
            val = q;
      return val;
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

/*
void ExtMidiEditor::readStatus(Xml& xml)
      {
      if (_pl == 0)
            _pl = new PartList;

      for (;;) {
            Xml::Token token = xml.parse();
            QString tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "quant")
                              _quant = xml.parseInt();
                        else if (tag == "raster")
                              _raster = xml.parseInt();
                        else if (tag == "topwin")
                              TopWin::readStatus(xml);
                        else
                              xml.unknown("MidiEditor");
                        break;
                  case Xml::TagEnd:
                        if (tag == "midieditor")
                              return;
                  default:
                        break;
                  }
            }
      }
*/

//---------------------------------------------------------
//   setCurDrumInstrument
//---------------------------------------------------------

void ExtMidiEditor::setCurDrumInstrument(int instr)
      {
      _curDrumInstrument = instr;
      emit curDrumInstrumentChanged(_curDrumInstrument);
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------
/*
void ExtMidiEditor::writeStatus(int level, Xml& xml) const
      {
      xml.tag(level++, "midieditor");
      TopWin::writeStatus(level, xml);
      xml.intTag(level, "quant", _quant);
      xml.intTag(level, "raster", _raster);
      xml.tag(level, "/midieditor");
      }
*/

//---------------------------------------------------------
//   quant
//---------------------------------------------------------
int ExtMidiEditor::quant() const
      {
      ExtEditorSettings* s = (ExtEditorSettings*) settings;
      return s->quant();
      }

//---------------------------------------------------------
//   setQuant
//---------------------------------------------------------

void ExtMidiEditor::setQuant(int val)
      {
      ExtEditorSettings* s = (ExtEditorSettings*) settings;
      s->setQuant(val);
      canvas->setQuant(val);
      canvas->setFocus();
      }


//---------------------------------------------------------
//   follow
//---------------------------------------------------------

void ExtMidiEditor::follow(int pos)
      {
      int s, e;
      canvas->range(&s, &e);

//      if (pos < e && pos >= s)
//            hscroll->setOffset(pos);
//      if (pos < s)
//            hscroll->setOffset(s);
      }

//---------------------------------------------------------
//   removeCtrl
//---------------------------------------------------------

void ExtMidiEditor::removeCtrl(CtrlEdit* ctrl)
      {
      for (std::list<CtrlEdit*>::iterator i = ctrlEditList.begin();
         i != ctrlEditList.end(); ++i) {
            if (*i == ctrl) {
                  ctrlEditList.erase(i);
                  break;
                  }
            }
      ExtEditorSettings* s = (ExtEditorSettings*)settings;
      int n = s->getControlEditSize();
      n--;
      s->setControlEditSize(n);
      }

//---------------------------------------------------------
//   addCtrl
//---------------------------------------------------------

CtrlEdit* ExtMidiEditor::addCtrl()
      {
#if 0 //TD
      bool expanding = editorType == ET_DRUMEDIT ? true : false;

      CtrlEdit* ctrlEdit = new CtrlEdit(splitter, this, EXTMIDIEDITOR_XSCALE, expanding, "extMidiEditorCtrlEdit");
      splitter->setResizeMode(ctrlEdit, QSplitter::KeepSize);

      // Calculate sizes:
      QList<int> sizes = splitter->sizes();
      QList<int> newSizes;
      QListIterator<int> i = sizes.begin();

      int editorHeight = *i - EXTMIDIEDITOR_DEFAULT_CTRLHEIGHT;
      int ctrlHeight   = EXTMIDIEDITOR_DEFAULT_CTRLHEIGHT;
      if (editorHeight < 0) {
            ctrlHeight = EXTMIDIEDITOR_DEFAULT_CTRLHEIGHT + editorHeight;
            editorHeight = *i - ctrlHeight;
            }
      if (editorHeight < 0) {
            editorHeight = 5;
            }

      newSizes.append(editorHeight);
      for (i++; i != sizes.end(); i++) {
            newSizes.append(*i);
            }
      newSizes.pop_back();
      newSizes.append(ctrlHeight);
      splitter->setSizes(newSizes);

      ctrlEdit->blockSignals(true);
      connect(tools2,   SIGNAL(toolChanged(int)),   ctrlEdit, SLOT(setTool(int)));
//      connect(hscroll,  SIGNAL(scrollChanged(int)), ctrlEdit, SLOT(setXPos(int)));
//      connect(hscroll,  SIGNAL(scaleChanged(int)),  ctrlEdit, SLOT(setXMag(int)));
      connect(ctrlEdit, SIGNAL(timeChanged(unsigned)),   SLOT(setTime(unsigned)));
      connect(ctrlEdit, SIGNAL(destroyedCtrl(CtrlEdit*)), SLOT(removeCtrl(CtrlEdit*)));
//      connect(ctrlEdit, SIGNAL(yposChanged(int)), toolbar, SLOT(setInt(int)));

      ctrlEdit->setTool(tools2->curTool());
//      ctrlEdit->setXPos(hscroll->pos());
//      ctrlEdit->setXMag(hscroll->getScaleValue());
      ctrlEdit->setController(CTRL_VELOCITY);

      ctrlEdit->show();
      ctrlEditList.push_back(ctrlEdit);
      ctrlEdit->blockSignals(false);
      ExtEditorSettings* s = (ExtEditorSettings*)settings;
      int n = s->getControlEditSize();
      n++;
      s->setControlEditSize(n);
      return ctrlEdit;
#endif
      return 0;
      }

//---------------------------------------------------------
//   refreshSettings
//---------------------------------------------------------
void ExtMidiEditor::refreshSettings() const
      {
      GraphMidiEditor::refreshSettings();
      ExtEditorSettings* s = (ExtEditorSettings*) settings;
      s->setSteprec(canvas->steprec());
      s->setMidiin(canvas->midiin());
//      s->setXmag(hscroll->xmag());
//      s->setPos(hscroll->pos());
      updateCtrlEdits();
      }

#endif
