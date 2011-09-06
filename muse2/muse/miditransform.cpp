//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: miditransform.cpp,v 1.8.2.3 2009/05/24 21:43:44 terminator356 Exp $
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

#include <stdio.h>
#include <list>


#include <QDialog>
#include <QListWidgetItem>

#include "helper.h"
#include "spinboxFP.h"
#include "event.h"
#include "miditransform.h"
#include "track.h"
#include "song.h"
#include "xml.h"
#include "globals.h"
#include "comboQuant.h"
//#include "pitchedit.h"
#include "audio.h"
#include "gconfig.h"
#include "midictrl.h"

//
// Order of events:
// Note, Poly Pressure, Control, AfterTouch, Pitch Bend, NRPN, RPN
//
#define MIDITRANSFORM_NOTE        0
#define MIDITRANSFORM_POLY        1
#define MIDITRANSFORM_CTRL        2
#define MIDITRANSFORM_ATOUCH      3
#define MIDITRANSFORM_PITCHBEND   4
#define MIDITRANSFORM_NRPN        5
#define MIDITRANSFORM_RPN         6


static int eventTypeTable[] = {
      MIDITRANSFORM_NOTE, MIDITRANSFORM_POLY, MIDITRANSFORM_CTRL, MIDITRANSFORM_ATOUCH,
         MIDITRANSFORM_PITCHBEND, MIDITRANSFORM_NRPN, MIDITRANSFORM_RPN
      };

static int procVal2Map[] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11 };

//---------------------------------------------------------
//   MidiTransformation
//---------------------------------------------------------

class MidiTransformation {
   public:
      QString name;
      QString comment;

      ValOp selEventOp;
      EventType selType;

      ValOp selVal1;
      int selVal1a, selVal1b;
      ValOp selVal2;
      int selVal2a, selVal2b;
      ValOp selLen;
      int selLenA, selLenB;
      ValOp selRange;
      int selRangeA, selRangeB;

      TransformOperator procEvent;
      EventType eventType;
      TransformOperator procVal1;
      int procVal1a, procVal1b;
      TransformOperator procVal2;
      int procVal2a, procVal2b;
      TransformOperator procLen;
      int procLenA;
      TransformOperator procPos;
      int procPosA;

      TransformFunction funcOp;
      int quantVal;
      bool selectedTracks;
      bool insideLoop;

      MidiTransformation(const QString& s) {
            name       = s;
            selEventOp = All;
            selType    = Note;
            selVal1    = Ignore;
            selVal1a   = 0;
            selVal1b   = 0;
            selVal2    = Ignore;
            selVal2a   = 0;
            selVal2b   = 0;
            selLen     = Ignore;
            selLenA    = 0;
            selLenB    = 0;
            selRange   = Ignore;
            selRangeA  = 0;
            selRangeB  = 0;
            procEvent  = Keep;
            eventType  = Note;
            procVal1   = Keep;
            procVal1a  = 0;
            procVal1b  = 0;
            procVal2   = Keep;
            procVal2a  = 0;
            procVal2b  = 0;
            procLen    = Keep;
            procLenA   = 0;
            procPos    = Keep;
            procPosA   = 0;
            funcOp     = Select;
            quantVal   = config.division;
            selectedTracks = false;
            insideLoop = false;
            }
      void write(int level, Xml& xml);
      };

class MidiTransformPrivate {
   public:
      MidiTransformation* cmt;
      int cindex;                   // current index in preset list
      };

typedef std::list<MidiTransformation* > MidiTransformationList;
typedef std::list<MidiTransformation* >::iterator iMidiTransformation;
typedef std::list<MidiTransformation* >::const_iterator ciMidiTransformation;

static MidiTransformationList mtlist;

//---------------------------------------------------------
//   MidiTransformDialog
//    Widgets:
//    presetList nameEntry commentEntry
//    selEventOp   selType
//    selVal1Op    selVal1a selVal1b
//    selVal2Op    selVal2a selVal2b
//    selLenOp     selLenA  selLenB
//    selRangeOp   selBarA  selBarB
//
//    procEventOp  procType
//    procVal1Op   procVal1a procVal1b
//    procVal2Op   procVal2a procVal2b
//    procLenOp    procLenA
//    procPosOp    procPosA
//    funcOp       funcQuantVal
//    processAll   selectedTracks   insideLoop
//    buttonNew buttonDelete buttonApply buttonOk
//---------------------------------------------------------

MidiTransformerDialog::MidiTransformerDialog(QDialog* parent, Qt::WFlags fl)
   : QDialog(parent, fl)
      {
      setupUi(this);
      data         = new MidiTransformPrivate;
      data->cmt    = 0;
      data->cindex = -1;
      connect(buttonApply,  SIGNAL(clicked()),      SLOT(apply()));
      connect(buttonNew,    SIGNAL(clicked()),      SLOT(presetNew()));
      connect(buttonDelete, SIGNAL(clicked()),      SLOT(presetDelete()));
      connect(selEventOp,   SIGNAL(activated(int)), SLOT(selEventOpSel(int)));
      connect(selType,      SIGNAL(activated(int)), SLOT(selTypeSel(int)));
      connect(selVal1Op,    SIGNAL(activated(int)), SLOT(selVal1OpSel(int)));
      connect(selVal2Op,    SIGNAL(activated(int)), SLOT(selVal2OpSel(int)));
      connect(selLenOp,     SIGNAL(activated(int)), SLOT(selLenOpSel(int)));
      connect(selRangeOp,   SIGNAL(activated(int)), SLOT(selRangeOpSel(int)));
      connect(procEventOp,  SIGNAL(activated(int)), SLOT(procEventOpSel(int)));
      connect(procType,     SIGNAL(activated(int)), SLOT(procEventTypeSel(int)));
      connect(procVal1Op,   SIGNAL(activated(int)), SLOT(procVal1OpSel(int)));
      connect(procVal2Op,   SIGNAL(activated(int)), SLOT(procVal2OpSel(int)));
      connect(procLenOp,    SIGNAL(activated(int)), SLOT(procLenOpSel(int)));
      connect(procPosOp,    SIGNAL(activated(int)), SLOT(procPosOpSel(int)));
      connect(funcOp,       SIGNAL(activated(int)), SLOT(funcOpSel(int)));
      connect(funcQuantVal, SIGNAL(valueChanged(int)), SLOT(funcQuantValSel(int)));
      connect(presetList,   SIGNAL(itemClicked(QListWidgetItem*)),
         SLOT(presetChanged(QListWidgetItem*)));
      connect(nameEntry,    SIGNAL(textChanged(const QString&)),
         SLOT(nameChanged(const QString&)));
      connect(commentEntry,    SIGNAL(textChanged()), SLOT(commentChanged()));

      connect(selVal1a,  SIGNAL(valueChanged(int)), SLOT(selVal1aChanged(int)));
      connect(selVal1b,  SIGNAL(valueChanged(int)), SLOT(selVal1bChanged(int)));
      connect(selVal2a,  SIGNAL(valueChanged(int)), SLOT(selVal2aChanged(int)));
      connect(selVal2b,  SIGNAL(valueChanged(int)), SLOT(selVal2bChanged(int)));
      connect(selLenA,   SIGNAL(valueChanged(int)), SLOT(selLenAChanged(int)));
      connect(selLenB,   SIGNAL(valueChanged(int)), SLOT(selLenBChanged(int)));
      connect(selBarA,   SIGNAL(valueChanged(int)), SLOT(selBarAChanged(int)));
      connect(selBarB,   SIGNAL(valueChanged(int)), SLOT(selBarBChanged(int)));
      connect(procVal1a, SIGNAL(valueChanged(int)), SLOT(procVal1aChanged(int)));
      connect(procVal1b, SIGNAL(valueChanged(int)), SLOT(procVal1bChanged(int)));
      connect(procVal2a, SIGNAL(valueChanged(int)), SLOT(procVal2aChanged(int)));
      connect(procVal2b, SIGNAL(valueChanged(int)), SLOT(procVal2bChanged(int)));
      connect(procLenA,  SIGNAL(valueChanged(int)), SLOT(procLenAChanged(int)));
      connect(procPosA,  SIGNAL(valueChanged(int)), SLOT(procPosAChanged(int)));

      connect(processAll, SIGNAL(toggled(bool)), SLOT(processAllChanged(bool)));
      connect(selectedTracks, SIGNAL(toggled(bool)), SLOT(selectedTracksChanged(bool)));
      connect(insideLoop, SIGNAL(toggled(bool)), SLOT(insideLoopChanged(bool)));

      //---------------------------------------------------
      //  populate preset list
      //---------------------------------------------------

      updatePresetList();
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      }

//---------------------------------------------------------
//   ~MidiTransformDialog
//---------------------------------------------------------

MidiTransformerDialog::~MidiTransformerDialog()
      {
      delete data;
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiTransformerDialog::songChanged(int flags)
{
  // Whenever a song is loaded, flags is -1. Since transforms are part of configuration, 
  //  use SC_CONFIG here, to filter unwanted song change events.
  if(flags & SC_CONFIG)
    updatePresetList();
}

//---------------------------------------------------------
//   updatePresetList
//---------------------------------------------------------

void MidiTransformerDialog::updatePresetList()
{
      data->cmt = 0;
      data->cindex = 0;
      presetList->clear();
      for (iMidiTransformation i = mtlist.begin(); i != mtlist.end(); ++i) {
            presetList->addItem((*i)->name);
            if (data->cmt == 0)
                  data->cmt = *i;
            }
      if (data->cmt == 0) {
            data->cmt = new MidiTransformation(tr("New"));
            mtlist.push_back(data->cmt);
            presetList->addItem(tr("New"));
            presetList->setCurrentItem(0);
            }
      
      //data->cindex = 0;
      //presetList->setCurrentItem(0);
      
}

//---------------------------------------------------------
//   writeMidiTransforms
//---------------------------------------------------------

void writeMidiTransforms(int level, Xml& xml)
      {
      for (iMidiTransformation i = mtlist.begin(); i != mtlist.end(); ++i) {
            (*i)->write(level, xml);
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MidiTransformation::write(int level, Xml& xml)
      {
      xml.tag(level++, "midiTransform");
      xml.strTag(level, "name", name);
      xml.strTag(level, "comment", comment);
      xml.intTag(level, "function", int(funcOp));
      xml.intTag(level, "selectedTracks", selectedTracks);
      xml.intTag(level, "insideLoop", insideLoop);
      if (funcOp == Quantize) {
            xml.intTag(level, "quantVal", quantVal);
            }
      if (funcOp == Transform || funcOp == Insert) {
            if (procEvent != Keep) {
                  xml.intTag(level, "procEventOp", int(procEvent));
                  xml.intTag(level, "eventType", int(eventType));
                  }
            if (procVal1 != Keep) {
                  xml.intTag(level, "procVal1Op", int(procVal1));
                  xml.intTag(level, "procVal1a", procVal1a);
                  xml.intTag(level, "procVal1b", procVal1b);
                  }
            if (procVal2 != Keep) {
                  xml.intTag(level, "procVal2Op", int(procVal2));
                  xml.intTag(level, "procVal2a", procVal2a);
                  xml.intTag(level, "procVal2b", procVal2b);
                  }
            if (procLen != Keep) {
                  xml.intTag(level, "procLenOp", int(procLen));
                  xml.intTag(level, "procLen",  procLenA);
                  }
            if (procPos != Keep) {
                  xml.intTag(level, "procPosOp", int(procPos));
                  xml.intTag(level, "procPos", procPosA);
                  }
            }
      if (selEventOp != Ignore) {
            xml.intTag(level, "selEventOp", int(selEventOp));
            xml.intTag(level, "selEventType", int(selType));
            }
      if (selVal1 != Ignore) {
            xml.intTag(level, "selVal1Op", int(selVal1));
            xml.intTag(level, "selVal1a", selVal1a);
            xml.intTag(level, "selVal1b", selVal1b);
            }
      if (selVal2 != Ignore) {
            xml.intTag(level, "selVal2Op", int(selVal2));
            xml.intTag(level, "selVal2a", selVal2a);
            xml.intTag(level, "selVal2b", selVal2b);
            }
      if (selLen != Ignore) {
            xml.intTag(level, "selLenOp", int(selLen));
            xml.intTag(level, "selLenA", selLenA);
            xml.intTag(level, "selLenB", selLenB);
            }
      if (selRange != Ignore) {
            xml.intTag(level, "selRangeOp", int(selRange));
            xml.intTag(level, "selRangeA", selRangeA);
            xml.intTag(level, "selRangeB", selRangeB);
            }
      xml.etag(level, "midiTransform");
      }

//---------------------------------------------------------
//   readMidiTransform
//---------------------------------------------------------

void readMidiTransform(Xml& xml)
      {
      MidiTransformation trans(QWidget::tr("new"));

      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "name")
                              trans.name = xml.parse1();
                        else if (tag == "comment")
                              trans.comment = xml.parse1();
                        else if (tag == "function")
                              trans.funcOp = TransformFunction(xml.parseInt());
                        else if (tag == "selectedTracks")
                              trans.selectedTracks = xml.parseInt();
                        else if (tag == "insideLoop")
                              trans.insideLoop = xml.parseInt();
                        else if (tag == "quantVal")
                              trans.quantVal = xml.parseInt();
                        else if (tag == "procEventOp")
                              trans.procEvent = TransformOperator(xml.parseInt());
                        else if (tag == "eventType")
                              trans.eventType = EventType(xml.parseInt());
                        else if (tag == "procVal1Op")
                              trans.procVal1 = TransformOperator(xml.parseInt());
                        else if (tag == "procVal1a")
                              trans.procVal1a = xml.parseInt();
                        else if (tag == "procVal1b")
                              trans.procVal1b = xml.parseInt();
                        else if (tag == "procVal2Op")
                              trans.procVal2 = TransformOperator(xml.parseInt());
                        else if (tag == "procVal2a")
                              trans.procVal2a = xml.parseInt();
                        else if (tag == "procVal2b")
                              trans.procVal2b = xml.parseInt();
                        else if (tag == "procLenOp")
                              trans.procLen = TransformOperator(xml.parseInt());
                        else if (tag == "procLen")
                              trans.procLenA = xml.parseInt();
                        else if (tag == "procPosOp")
                              trans.procPos = TransformOperator(xml.parseInt());
                        else if (tag == "procPos")
                              trans.procPosA = xml.parseInt();
                        else if (tag == "selEventOp")
                              trans.selEventOp = ValOp(xml.parseInt());
                        else if (tag == "selEventType")
                              trans.selType = EventType(xml.parseInt());
                        else if (tag == "selVal1Op")
                              trans.selVal1 = ValOp(xml.parseInt());
                        else if (tag == "selVal1a")
                              trans.selVal1a = xml.parseInt();
                        else if (tag == "selVal1b")
                              trans.selVal1b = xml.parseInt();
                        else if (tag == "selVal2Op")
                              trans.selVal2 = ValOp(xml.parseInt());
                        else if (tag == "selVal2a")
                              trans.selVal2a = xml.parseInt();
                        else if (tag == "selVal2b")
                              trans.selVal2b = xml.parseInt();
                        else if (tag == "selLenOp")
                              trans.selLen = ValOp(xml.parseInt());
                        else if (tag == "selLenA")
                              trans.selLenA = xml.parseInt();
                        else if (tag == "selLenB")
                              trans.selLenB = xml.parseInt();
                        else if (tag == "selRangeOp")
                              trans.selRange = ValOp(xml.parseInt());
                        else if (tag == "selRangeA")
                              trans.selRangeA = xml.parseInt();
                        else if (tag == "selRangeB")
                              trans.selRangeB = xml.parseInt();
                        else
                              xml.unknown("midiTransform");
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "midiTransform") {
                              // By T356. A previous bug caused some .med files to grow very large
                              //  with duplicate transforms. Here we can eliminate those duplicates.
                              for(iMidiTransformation i = mtlist.begin(); i != mtlist.end(); ++i) 
                              {
                                if((*i)->name == trans.name)
                                  return;
                              }
                              
                              MidiTransformation* t = new MidiTransformation(trans);
                              mtlist.push_back(t);
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   clearMidiTransforms
//---------------------------------------------------------

void clearMidiTransforms()
{
  for (iMidiTransformation i = mtlist.begin(); i != mtlist.end(); ++i) 
  {
    MidiTransformation* t = *i;
    if(t)
      delete t;
  }
  mtlist.clear();
}

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void MidiTransformerDialog::accept()
      {
      apply();
      reject();
      }

//---------------------------------------------------------
//   transformEvent
//    subfunction of processEvent()
//---------------------------------------------------------

void MidiTransformerDialog::transformEvent(Event& event, MidiPart* part,
   MidiPart* newPart)
      {
      MidiTransformation* cmt = data->cmt;
      Event newEvent = event.clone();

      if (cmt->procEvent != Keep)
            newEvent.setType(cmt->eventType);

      //---------------------------------------------------
      //    transform value A
      //---------------------------------------------------

      int val = newEvent.dataA();
      switch (cmt->procVal1) {
            case Keep:
                  break;
            case Plus:
                  val += cmt->procVal1a;
                  break;
            case Minus:
                  val -= cmt->procVal1a;
                  break;
            case Multiply:
                  val = int(val * (cmt->procVal1a/100.0) + .5);
                  break;
            case Divide:
                  val = int(val / (cmt->procVal1a/100.0) + .5);
                  break;
            case Fix:
                  val = cmt->procVal1a;
                  break;
            case Value:
                  val = cmt->procVal2a;
                  break;
            case Invert:
                  val = 128 - val;
                  break;
            case ScaleMap:
                  printf("scale map not implemented\n");
                  break;
            case Flip:
                  val = cmt->procVal1a - val;
                  break;
            case Dynamic:           // "crescendo"
                  val = (((cmt->procVal2b-cmt->procVal2a)
                        * (newEvent.tick() - song->lpos()))
                        / (song->rpos() - song->lpos())) + cmt->procVal2a;
                  break;
            case Random:
                  {
                  int range = cmt->procVal1b - cmt->procVal1a;
                  if (range > 0)
                        val = (rand() % range) + cmt->procVal1a;
                  else if (range < 0)
                        val = (rand() % -range) + cmt->procVal1b;
                  else
                        val = cmt->procVal1a;
                  }
                  break;
            }
      if (val < 0)
            val = 0;
      if (val > 127)
            val = 127;
      newEvent.setA(val);

      //---------------------------------------------------
      //    transform value B
      //---------------------------------------------------

      val = newEvent.dataB();
      switch (cmt->procVal2) {
            case Plus:
                  val += cmt->procVal2a;
                  break;
            case Minus:
                  val -= cmt->procVal2a;
                  break;
            case Multiply:
                  val = int(val * (cmt->procVal2a/100.0) + .5);
                  break;
            case Divide:
                  val = int(val / (cmt->procVal2a/100.0) + .5);
                  break;
            case Fix:
                  val = cmt->procVal2a;
                  break;
            case Value:
                  val = cmt->procVal1a;
                  break;
            case Invert:
                  val = 128 - val;
                  break;
            case Dynamic:
                  val = (((cmt->procVal2b-cmt->procVal2a)
                        * (newEvent.tick() - song->lpos()))
                        / (song->rpos() - song->lpos())) + cmt->procVal2a;
                  break;
            case Random:
                  {
                  int range = cmt->procVal2b - cmt->procVal2a;
                  if (range > 0)
                        val = (rand() % range) + cmt->procVal2a;
                  else if (range < 0)
                        val = (rand() % -range) + cmt->procVal2b;
                  else
                        val = cmt->procVal1a;
                  }
                  break;
            case ScaleMap:
            case Keep:
            case Flip:
                  break;
            }
      if (val < 0)
            val = 0;
      if (val > 127)
            val = 127;
      newEvent.setB(val);

      //---------------------------------------------------
      //    transform len
      //---------------------------------------------------

      int len = newEvent.lenTick();
      switch (cmt->procLen) {
            case Plus:
                  len += cmt->procLenA;
                  break;
            case Minus:
                  len -= cmt->procLenA;
                  break;
            case Multiply:
                  len = int(val * (cmt->procLenA/100.0) + .5);
                  break;
            case Divide:
                  len = int(val / (cmt->procLenA/100.0) + .5);
                  break;
            case Fix:
                  len = cmt->procLenA;
                  break;
            case Invert:
            case ScaleMap:
            case Dynamic:
            case Random:
            case Keep:
            case Flip:
            case Value:
                  break;
            }
      if (len < 0)
            len = 0;
      newEvent.setLenTick(len);

      //---------------------------------------------------
      //    transform pos
      //---------------------------------------------------

      int pos = newEvent.tick();
      switch (cmt->procPos) {
            case Plus:
                  pos += cmt->procPosA;
                  break;
            case Minus:
                  pos -= cmt->procPosA;
                  break;
            case Multiply:
                  pos = int(val * (cmt->procPosA/100.0) + .5);
                  break;
            case Divide:
                  pos = int(val / (cmt->procPosA/100.0) + .5);
                  break;
            case Fix:
            case Invert:
            case ScaleMap:
            case Dynamic:
            case Random:
            case Keep:
            case Flip:
            case Value:
                  break;
            }
      if (pos < 0)
            pos = 0;
      newEvent.setTick(pos);

      Event dummy;
      switch(data->cmt->funcOp) {
            case Transform:
                  // Indicate do clone parts. 
                  removePortCtrlEvents(event, part, true);
                  song->changeEvent(event, newEvent, part);
                  // Indicate do clone parts. 
                  addPortCtrlEvents(newEvent, part, true);
                  // Indicate do port controller values and clone parts. 
                  //song->addUndo(UndoOp(UndoOp::ModifyEvent, newEvent, event, part));
                  song->addUndo(UndoOp(UndoOp::ModifyEvent, newEvent, event, part, true, true));
                  song->addUpdateFlags(SC_EVENT_MODIFIED);
                  break;
            case Insert:
                  // Indicate do port controller values and clone parts. 
                  //song->addUndo(UndoOp(UndoOp::AddEvent, dummy, newEvent, part));
                  song->addUndo(UndoOp(UndoOp::AddEvent, dummy, newEvent, part, true, true));
                  song->addEvent(newEvent, part);
                  // Indicate do clone parts. 
                  addPortCtrlEvents(newEvent, part, true);
                  song->addUpdateFlags(SC_EVENT_INSERTED);
                  break;
            case Extract:
                  // Indicate do port controller values and clone parts. 
                  //song->addUndo(UndoOp(UndoOp::DeleteEvent, dummy, event, part));
                  song->addUndo(UndoOp(UndoOp::DeleteEvent, dummy, event, part, true, true));
                  // Indicate do clone parts. 
                  removePortCtrlEvents(event, part, true);
                  song->deleteEvent(event, part);
                  song->addUpdateFlags(SC_EVENT_REMOVED);
            case Copy:
                  newPart->addEvent(newEvent);
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   processEvent
//---------------------------------------------------------

void MidiTransformerDialog::processEvent(Event& event, MidiPart* part, MidiPart* newPart)
      {
      switch(data->cmt->funcOp) {
            case Select:
                  break;
            case Quantize:
                  {
                  int tick = event.tick();
                  int rt = AL::sigmap.raster(tick, data->cmt->quantVal) - tick;
                  if (tick != rt) {
                        // Indicate do clone parts. 
                        removePortCtrlEvents(event, part, true);
                        Event newEvent = event.clone();
                        newEvent.setTick(rt);
                        song->changeEvent(event, newEvent, part);
                        // Indicate do clone parts. 
                        addPortCtrlEvents(newEvent, part, true);
                        // Indicate do port controller values and clone parts. 
                        //song->addUndo(UndoOp(UndoOp::ModifyEvent, newEvent, event, part));
                        song->addUndo(UndoOp(UndoOp::ModifyEvent, newEvent, event, part, true, true));
                        song->addUpdateFlags(SC_EVENT_MODIFIED);
                        }
                  }
                  break;
            case Delete:
                  {
                  Event ev;
                  // Indicate do port controller values and clone parts. 
                  //song->addUndo(UndoOp(UndoOp::DeleteEvent, ev, event, part, true, true));
                  song->addUndo(UndoOp(UndoOp::DeleteEvent, ev, event, part, true, true));
                  // Indicate do clone parts. 
                  removePortCtrlEvents(event, part, true);
                  song->deleteEvent(event, part);
                  song->addUpdateFlags(SC_EVENT_REMOVED);
                  }
                  break;
            case Transform:
            case Insert:
            case Copy:
            case Extract:
                  transformEvent(event, part, newPart);
                  break;
            }
      }

//---------------------------------------------------------
//   isSelected
//    apply Select filter
//    return true if event is selected
//---------------------------------------------------------

bool MidiTransformerDialog::isSelected(Event& event, MidiPart*)
      {
      MidiTransformation* cmt = data->cmt;

      switch (cmt->selEventOp) {
            case Equal:
                  if (!typesMatch(event, cmt->selType)) {
                        return false;
                        }
                  break;
            case Unequal:
                  if (typesMatch(event, cmt->selType))
                        return false;
                  break;
            default:
                  break;
            }
      switch (cmt->selVal1) {
            case Ignore:
                  break;
            case Equal:
                  if (event.dataA() != cmt->selVal1a)
                        return false;
                  break;
            case Unequal:
                  if (event.dataA() == cmt->selVal1a)
                        return false;
                  break;
            case Higher:
                  if (event.dataA() <= cmt->selVal1a)
                        return false;
                  break;
            case Lower:
                  if (event.dataA() >= cmt->selVal1a)
                        return false;
                  break;
            case Inside:
                  if ((event.dataA() < cmt->selVal1a)
                     || (event.dataA() >= cmt->selVal1b))
                        return false;
                  break;
            case Outside:
                  if ((event.dataA() >= cmt->selVal1a)
                     && (event.dataA() < cmt->selVal1b))
                        return false;
                  break;
            }
      switch (cmt->selVal2) {
            case Ignore:
                  break;
            case Equal:
                  if (event.dataB() != cmt->selVal2a)
                        return false;
                  break;
            case Unequal:
                  if (event.dataB() == cmt->selVal2a)
                        return false;
                  break;
            case Higher:
                  if (event.dataB() <= cmt->selVal2a)
                        return false;
                  break;
            case Lower:
                  if (event.dataB() >= cmt->selVal2a)
                        return false;
                  break;
            case Inside:
                  if ((event.dataB() < cmt->selVal2a)
                     || (event.dataB() >= cmt->selVal2b))
                        return false;
                  break;
            case Outside:
                  if ((event.dataB() >= cmt->selVal2a)
                     && (event.dataB() < cmt->selVal2b))
                        return false;
                  break;
            }
      int len = event.lenTick();
      switch (cmt->selLen) {
            case Ignore:
                  break;
            case Equal:
                  if (len != cmt->selLenA)
                        return false;
                  break;
            case Unequal:
                  if (len == cmt->selLenA)
                        return false;
                  break;
            case Higher:
                  if (len <= cmt->selLenA)
                        return false;
                  break;
            case Lower:
                  if (len >= cmt->selLenA)
                        return false;
                  break;
            case Inside:
                  if ((len < cmt->selLenA) || (len >= cmt->selLenB))
                        return false;
                  break;
            case Outside:
                  if ((len >= cmt->selLenA) && (len < cmt->selLenB))
                        return false;
                  break;
            }
      int bar, beat;
      unsigned tick;
      AL::sigmap.tickValues(event.tick(), &bar, &beat, &tick);
      int beat1 = cmt->selRangeA / 1000;
      unsigned tick1 = cmt->selRangeA % 1000;
      int beat2 = cmt->selRangeB / 1000;
      unsigned tick2 = cmt->selRangeB % 1000;
      switch (cmt->selRange) {
            case Ignore:
                  break;
            case Equal:
                  if (beat != beat1 || tick != tick1)
                        return false;
                  break;
            case Unequal:
                  if (beat == beat1 && tick == tick1)
                        return false;
                  break;
            case Higher:
                  if (beat <= beat1)
                        return false;
                  if (beat == beat1 && tick <= tick1)
                        return false;
                  break;
            case Lower:
                  if (beat >= beat1)
                        return false;
                  if (beat == beat1 && tick >= tick1)
                        return false;
                  break;
            case Inside:
                  if ((beat < beat1) || (beat >= beat2))
                        return false;
                  if (beat == beat1 && tick < tick1)
                        return false;
                  if (beat == beat2 && tick >= tick2)
                        return false;
                  break;
            case Outside:
                  if ((beat >= beat1) || (beat < beat2))
                        return false;
                  if (beat == beat1 && tick >= tick1)
                        return false;
                  if (beat == beat2 && tick < tick2)
                        return false;
                  break;
            }
      return true;
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void MidiTransformerDialog::apply()
      {
      int flags = 0;
      song->startUndo();
      audio->msgIdle(true);
      bool copyExtract = (data->cmt->funcOp == Copy)
                         || (data->cmt->funcOp == Extract);

      std::vector< EventList* > doneList;
      typedef std::vector< EventList* >::iterator iDoneList;
      iDoneList idl;
      
      MidiTrackList* tracks = song->midis();
      MidiTrackList tl;
      for (iMidiTrack t = tracks->begin(); t != tracks->end(); ++t) {
            if (data->cmt->selectedTracks && !(*t)->selected())
                  continue;
            MidiTrack* newTrack = 0;
            PartList *pl = (*t)->parts();
            if (copyExtract) {
                  // check wether we must generate a new track
                  for (iPart p = pl->begin(); p != pl->end(); ++p) {
                        MidiPart* part = (MidiPart *) p->second;
                        EventList* el = part->events();
                        // Check if the event list has already been done. Skip repeated clones.
                        for(idl = doneList.begin(); idl != doneList.end(); ++idl)
                          if(*idl == el)
                            break;
                        if(idl != doneList.end())
                          break;
                        doneList.push_back(el);
                        
                        for (iEvent i = el->begin(); i != el->end(); ++i) {
                              Event event = i->second;
                              unsigned tick = event.tick();
                              if (data->cmt->insideLoop && (tick < song->lpos() || tick >= song->rpos()))
                                    continue;
                              if (isSelected(event, part)) {
                                    newTrack = new MidiTrack();
                                    tl.push_back(newTrack);
                                    break;
                                    }
                              }
                        if (newTrack)
                              break;
                        }
                  }

            for (iPart p = pl->begin(); p != pl->end(); ++p) {
                  MidiPart* part = (MidiPart *) p->second;
                  MidiPart* newPart = 0;
                  EventList* el = part->events();
                  // Check if the event list has already been done. Skip repeated clones.
                  for(idl = doneList.begin(); idl != doneList.end(); ++idl)
                    if(*idl == el)
                      break;
                  if(idl != doneList.end())
                    break;
                  doneList.push_back(el);
                  
                  if (copyExtract) {
                        // check wether we must generate a new part
                        for (iEvent i = el->begin(); i != el->end(); ++i) {
                              Event event = i->second;
                              unsigned tick = event.tick();
                              if (data->cmt->insideLoop && (tick < song->lpos() || tick >= song->rpos()))
                                    continue;
                              if (isSelected(event, part)) {
                                    newPart = new MidiPart(newTrack);
                                    newPart->setName(part->name());
                                    newPart->setColorIndex(part->colorIndex());
                                    newPart->setTick(part->tick());
                                    newPart->setLenTick(part->lenTick());
                                    song->addPart(newPart);
                                    flags |= SC_PART_INSERTED;
                                    break;
                                    }
                              }
                        }
                  EventList pel;
                  for (iEvent i = el->begin(); i != el->end(); ++i) {
                        Event event = i->second;
                        unsigned tick = event.tick();
                        if (data->cmt->insideLoop && (tick < song->lpos() || tick >= song->rpos()))
                              continue;
                        int flag = isSelected(event, part);
                        if (data->cmt->funcOp == Select)
                              event.setSelected(flag);
                        else if (flag)
                              pel.add(event);
                        }
                  for (iEvent i = pel.begin(); i != pel.end(); ++i) {
                        Event event = i->second;
                        processEvent(event, part, newPart);
                        }
                  }
            }
      if (!tl.empty()) {
            flags |= SC_TRACK_INSERTED;
            for (iTrack t = tl.begin(); t != tl.end(); ++t) {
                  song->insertTrack0(*t, -1);
                  }
            }

      switch(data->cmt->funcOp) {
            case Select:
                  flags |= SC_SELECTION;
                  break;
            case Quantize:
                  flags |= SC_EVENT_MODIFIED;
                  break;
            case Delete:
                  flags |= SC_EVENT_REMOVED;
                  break;
            case Transform:
                  flags |= SC_EVENT_MODIFIED;
                  break;
            case Insert:
                  flags |= SC_EVENT_INSERTED;
                  break;
            case Copy:
                  flags |= SC_EVENT_INSERTED;
            case Extract:
                  break;
            }
      audio->msgIdle(false);
      song->endUndo(flags);
      }

//---------------------------------------------------------
//   setValOp
//---------------------------------------------------------

void MidiTransformerDialog::setValOp(QWidget* a, QWidget* b, ValOp op)
      {
      switch (op) {
            case Ignore:
                  a->setEnabled(false);
                  b->setEnabled(false);
                  break;
            case Equal:
            case Unequal:
            case Higher:
            case Lower:
                  a->setEnabled(true);
                  b->setEnabled(false);
                  break;
            case Inside:
            case Outside:
                  a->setEnabled(true);
                  b->setEnabled(true);
                  break;
            }
      }

//---------------------------------------------------------
//   selEventOpSel
//---------------------------------------------------------

void MidiTransformerDialog::selEventOpSel(int val)
      {
      selType->setEnabled(val != All);
      data->cmt->selEventOp = ValOp(val);
      selVal1aChanged(data->cmt->selVal1a);
      selVal1bChanged(data->cmt->selVal1b);
      }

//---------------------------------------------------------
//   selTypeSel
//---------------------------------------------------------

void MidiTransformerDialog::selTypeSel(int val)
      {
      data->cmt->selType = EventType(eventTypeTable[val]);
      selVal1aChanged(data->cmt->selVal1a);
      selVal1bChanged(data->cmt->selVal1b);
      }

//---------------------------------------------------------
//   selVal1OpSel
//---------------------------------------------------------

void MidiTransformerDialog::selVal1OpSel(int val)
      {
      setValOp(selVal1a, selVal1b, ValOp(val));
      data->cmt->selVal1 = ValOp(val);
      }

//---------------------------------------------------------
//   selVal2OpSel
//---------------------------------------------------------

void MidiTransformerDialog::selVal2OpSel(int val)
      {
      setValOp(selVal2a, selVal2b, ValOp(val));
      data->cmt->selVal2 = ValOp(val);
      }

//---------------------------------------------------------
//   selLenOpSel
//---------------------------------------------------------

void MidiTransformerDialog::selLenOpSel(int val)
      {
      setValOp(selLenA, selLenB, ValOp(val));
      data->cmt->selLen = ValOp(val);
      }

//---------------------------------------------------------
//   selRangeOpSel
//---------------------------------------------------------

void MidiTransformerDialog::selRangeOpSel(int val)
      {
      setValOp(selBarA, selBarB, ValOp(val));
      data->cmt->selRange = ValOp(val);
      }

//---------------------------------------------------------
//   procEventOpSel
//---------------------------------------------------------

void MidiTransformerDialog::procEventOpSel(int val)
      {
      TransformOperator op = val == 0 ? Keep : Fix;
      procType->setEnabled(op == Fix);
      data->cmt->procEvent = op;
      
      procVal1aChanged(data->cmt->procVal1a);
      procVal1bChanged(data->cmt->procVal1b);
      }

//---------------------------------------------------------
//   procEventTypeSel
//---------------------------------------------------------

void MidiTransformerDialog::procEventTypeSel(int val)
      {
      data->cmt->eventType = EventType(eventTypeTable[val]);
      procVal1aChanged(data->cmt->procVal1a);
      procVal1bChanged(data->cmt->procVal1b);
      }

//---------------------------------------------------------
//   procVal1OpSel
//---------------------------------------------------------

void MidiTransformerDialog::procVal1OpSel(int val)
      {
      data->cmt->procVal1 = TransformOperator(val);
      switch(TransformOperator(val)) {
            case Keep:
            case Invert:
                  procVal1a->setEnabled(false);
                  procVal1b->setEnabled(false);
                  break;
            case Multiply:
            case Divide:
                  procVal1a->setEnabled(true);
                  procVal1a->setDecimals(2);
                  procVal1b->setEnabled(false);
                  break;
            case Plus:
            case Minus:
            case Fix:
            case Value:
            case Flip:
                  procVal1a->setDecimals(0);
                  procVal1a->setEnabled(true);
                  procVal1b->setEnabled(false);
                  break;
            case Random:
            case ScaleMap:
            case Dynamic:
                  procVal1a->setDecimals(0);
                  procVal1a->setEnabled(true);
                  procVal1b->setEnabled(true);
                  break;
            }
      procVal1aChanged(data->cmt->procVal1a);
      procVal1bChanged(data->cmt->procVal1b);
      }

//---------------------------------------------------------
//   procVal2OpSel
//---------------------------------------------------------

void MidiTransformerDialog::procVal2OpSel(int val)
      {
      TransformOperator op = TransformOperator(procVal2Map[val]);
      data->cmt->procVal2 = op;

      switch (op) {
            case Keep:
            case Invert:
                  procVal2a->setEnabled(false);
                  procVal2b->setEnabled(false);
                  break;
            case Multiply:
            case Divide:
                  procVal2a->setEnabled(true);
                  procVal2a->setDecimals(2);
                  procVal2b->setEnabled(false);
                  break;
            case Plus:
            case Minus:
            case Fix:
            case Value:
                  procVal2a->setDecimals(0);
                  procVal2a->setEnabled(true);
                  procVal2b->setEnabled(false);
                  break;
            case Random:
            case Dynamic:
                  procVal2a->setDecimals(0);
                  procVal2a->setEnabled(true);
                  procVal2b->setEnabled(true);
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   procLenOpSel
//---------------------------------------------------------

void MidiTransformerDialog::procLenOpSel(int val)
      {
      TransformOperator op = TransformOperator(val);
      data->cmt->procLen = op;

      switch (op) {
            case Keep:
            case Invert:
                  procLenA->setEnabled(false);
                  break;
            case Plus:
            case Minus:
            case Fix:
                  procLenA->setDecimals(0);
                  procLenA->setEnabled(true);
                  break;
            case Multiply:
            case Divide:
                  procLenA->setDecimals(2);
                  procLenA->setEnabled(true);
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   procPosOpSel
//---------------------------------------------------------

void MidiTransformerDialog::procPosOpSel(int val)
      {
      TransformOperator op = TransformOperator(val);
      data->cmt->procPos = op;

      switch (op) {
            case Keep:
            case Invert:
                  procPosA->setEnabled(false);
                  break;
            case Multiply:
            case Divide:
                  procPosA->setDecimals(2);
                  procPosA->setEnabled(true);
                  break;
            case Plus:
            case Minus:
                  procPosA->setDecimals(0);
                  procPosA->setEnabled(true);
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   funcOpSel
//---------------------------------------------------------

void MidiTransformerDialog::funcOpSel(int val)
      {
      funcQuantVal->setEnabled(val == Quantize);
      bool isFuncOp = val == Transform || val == Insert;

      procEventOp->setEnabled(isFuncOp);
      procType->setEnabled(isFuncOp);
      procVal1Op->setEnabled(isFuncOp);
      procVal1a->setEnabled(isFuncOp);
      procVal1b->setEnabled(isFuncOp);
      procVal2Op->setEnabled(isFuncOp);
      procVal2a->setEnabled(isFuncOp);
      procVal2b->setEnabled(isFuncOp);
      procLenOp->setEnabled(isFuncOp);
      procLenA->setEnabled(isFuncOp);
      procPosOp->setEnabled(isFuncOp);
      procPosA->setEnabled(isFuncOp);
      if (isFuncOp) {
            procEventOpSel(data->cmt->procEvent);
            procVal1OpSel(data->cmt->procVal1);
            procVal2OpSel(data->cmt->procVal2);
            procLenOpSel(data->cmt->procLen);
            procPosOpSel(data->cmt->procPos);
            }
      data->cmt->funcOp = TransformFunction(val);
      }

//---------------------------------------------------------
//   presetNew
//---------------------------------------------------------

void MidiTransformerDialog::presetNew()
      {
      QString name;
      for (int i = 0;; ++i) {
            name.sprintf("New-%d", i);
            iMidiTransformation imt;
            for (imt = mtlist.begin(); imt != mtlist.end(); ++imt) {
                  if (name == (*imt)->name)
                        break;
                  }
            if (imt == mtlist.end())
                  break;
            }
      MidiTransformation* mt = new MidiTransformation(name);
      QListWidgetItem* lbi      = new QListWidgetItem(name);
      presetList->addItem(lbi);
      mtlist.push_back(mt);
      presetList->setCurrentItem(lbi);
      presetChanged(lbi);
      }

//---------------------------------------------------------
//   presetDelete
//---------------------------------------------------------

void MidiTransformerDialog::presetDelete()
      {
      if (data->cindex != -1) {
            iMidiTransformation mt = mtlist.begin();
            for (int i = 0; i < data->cindex; ++i, ++mt) {
                  mtlist.erase(mt);
                  presetList->setCurrentItem(presetList->item(data->cindex - 1));
                  presetList->takeItem(data->cindex);
                  presetChanged(presetList->item(data->cindex - 1));
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   presetChanged
//---------------------------------------------------------

void MidiTransformerDialog::presetChanged(QListWidgetItem* item)
      {
      data->cindex = presetList->row(item);
      iMidiTransformation i;
      for (i = mtlist.begin(); i != mtlist.end(); ++i) {
            if (item->text() == (*i)->name) {
                  data->cmt = *i;
                  break;
                  }
            }
      if (i == mtlist.end()) {
            printf("MidiTransformerDialog::presetChanged: not found\n");
            return;
            }
      nameEntry->setText(data->cmt->name);
      commentEntry->setText(data->cmt->comment);

      selEventOp->setCurrentIndex(data->cmt->selEventOp);
      selEventOpSel(data->cmt->selEventOp);

      for (unsigned i = 0; i < sizeof(eventTypeTable)/sizeof(*eventTypeTable); ++i) {
            if (eventTypeTable[i] == data->cmt->selType) {
                  selType->setCurrentIndex(i);
                  break;
                  }
            }

      selVal1Op->setCurrentIndex(data->cmt->selVal1);
      selVal1OpSel(data->cmt->selVal1);

      selVal2Op->setCurrentIndex(data->cmt->selVal2);
      selVal2OpSel(data->cmt->selVal2);

      selLenOp->setCurrentIndex(data->cmt->selLen);
      selLenOpSel(data->cmt->selLen);

      selRangeOp->setCurrentIndex(data->cmt->selRange);
      selRangeOpSel(data->cmt->selRange);

      funcOp->setCurrentIndex(data->cmt->funcOp);
      funcOpSel(data->cmt->funcOp);

      // TransformOperator procEvent: Keep, Fix
      procEventOp->setCurrentIndex(data->cmt->procEvent == Fix);

      procEventOpSel(data->cmt->procEvent);

      procVal1Op->setCurrentIndex(data->cmt->procVal1);
      procVal1OpSel(data->cmt->procVal1);

      for (unsigned i = 0; i < sizeof(procVal2Map)/sizeof(*procVal2Map); ++i) {
            if (procVal2Map[i] == data->cmt->procVal2) {
                  procVal2Op->setCurrentIndex(i);
                  break;
                  }
            }
      procLenOp->setCurrentIndex(data->cmt->procLen);
      procLenOpSel(data->cmt->procLen);

      procPosOp->setCurrentIndex(data->cmt->procPos);
      procPosOpSel(data->cmt->procPos);

      selVal1aChanged(data->cmt->selVal1a);
      selVal1bChanged(data->cmt->selVal1b);
      selVal2a->setValue(data->cmt->selVal2a);
      selVal2b->setValue(data->cmt->selVal2b);
      selLenA->setValue(data->cmt->selLenA);
      selLenB->setValue(data->cmt->selLenB);
      selBarA->setValue(data->cmt->selRangeA);
      selBarB->setValue(data->cmt->selRangeB);
      procVal1a->setValue(data->cmt->procVal1a);
      procVal1b->setValue(data->cmt->procVal1b);
      procVal2a->setValue(data->cmt->procVal2a);
      procVal2b->setValue(data->cmt->procVal2b);
      procLenA->setValue(data->cmt->procLenA);
      procPosA->setValue(data->cmt->procPosA);
      funcQuantVal->setValue(data->cmt->quantVal);

      selectedTracks->setChecked(data->cmt->selectedTracks);
      selectedTracksChanged(data->cmt->selectedTracks);
      insideLoop->setChecked(data->cmt->insideLoop);
      insideLoopChanged(data->cmt->insideLoop);
      }

//---------------------------------------------------------
//   nameChanged
//---------------------------------------------------------

void MidiTransformerDialog::nameChanged(const QString& s)
      {
      data->cmt->name = s;
      QListWidgetItem* item = presetList->item(data->cindex);
      if (s != item->text()) {
            disconnect(presetList, SIGNAL(highlighted(QListWidgetItem*)),
               this, SLOT(presetChanged(QListWidgetItem*)));
	    presetList->insertItem(data->cindex, s);
	    presetList->takeItem(data->cindex);
            presetList->setCurrentItem(presetList->item(data->cindex));
            connect(presetList,   SIGNAL(highlighted(QListWidgetItem*)),
               SLOT(presetChanged(QListWidgetItem*)));
            }
      }

//---------------------------------------------------------
//   commentChanged
//---------------------------------------------------------

void MidiTransformerDialog::commentChanged()
      {
      data->cmt->comment = commentEntry->toPlainText();
      }

//-----------------------------op----------------------------
//   selVal1aChanged
//---------------------------------------------------------

void MidiTransformerDialog::selVal1aChanged(int val)
      {
      data->cmt->selVal1a = val;
      if ((data->cmt->selEventOp != All)
         && (data->cmt->selType == Note)) {
            selVal1a->setSuffix(" - " + pitch2string(val));
            }
      else
      {
            if(!selVal1a->suffix().isEmpty())
              selVal1a->setSuffix(QString(""));
      }      
      }

//---------------------------------------------------------
//   selVal1bChanged
//---------------------------------------------------------

void MidiTransformerDialog::selVal1bChanged(int val)
      {
      data->cmt->selVal1b = val;
      if ((data->cmt->selEventOp != All)
         && (data->cmt->selType == Note)) {
            selVal1b->setSuffix(" - " + pitch2string(val));
            }
      else
      {
            if(!selVal1b->suffix().isEmpty())
              selVal1b->setSuffix(QString(""));
      }      
      }

//---------------------------------------------------------
//   selVal2aChanged
//---------------------------------------------------------

void MidiTransformerDialog::selVal2aChanged(int val)
      {
      data->cmt->selVal2a = val;
      }

//---------------------------------------------------------
//   selVal2bChanged
//---------------------------------------------------------

void MidiTransformerDialog::selVal2bChanged(int val)
      {
      data->cmt->selVal2b = val;
      }

//---------------------------------------------------------
//   selLenAChanged
//---------------------------------------------------------

void MidiTransformerDialog::selLenAChanged(int val)
      {
      data->cmt->selLenA = val;
      }

//---------------------------------------------------------
//   selLenBChanged
//---------------------------------------------------------

void MidiTransformerDialog::selLenBChanged(int val)
      {
      data->cmt->selLenB = val;
      }

//---------------------------------------------------------
//   selBarAChanged
//---------------------------------------------------------

void MidiTransformerDialog::selBarAChanged(int val)
      {
      data->cmt->selRangeA = val;
      }

//---------------------------------------------------------
//   selBarBChanged
//---------------------------------------------------------

void MidiTransformerDialog::selBarBChanged(int val)
      {
      data->cmt->selRangeB = val;
      }

//---------------------------------------------------------
//   procVal1aChanged
//---------------------------------------------------------

void MidiTransformerDialog::procVal1aChanged(int val)
      {
      data->cmt->procVal1a = val;
      
      if((data->cmt->procEvent == Keep && data->cmt->selType == MIDITRANSFORM_NOTE) && 
           (data->cmt->procVal1 == Fix || data->cmt->procVal1 == ScaleMap || data->cmt->procVal1 == Dynamic || 
            data->cmt->procVal1 == Random || data->cmt->procVal1 == Flip)) 
        {
            procVal1a->setSuffix(" - " + pitch2string(val));
        }
      else
      {
            if(!procVal1a->suffix().isEmpty())
              procVal1a->setSuffix(QString(""));
      }      
      
      }

//---------------------------------------------------------
//   procVal1bChanged
//---------------------------------------------------------

void MidiTransformerDialog::procVal1bChanged(int val)
      {
      data->cmt->procVal1b = val;
      
      if((data->cmt->procEvent == Keep && data->cmt->selType == MIDITRANSFORM_NOTE) && 
           (data->cmt->procVal1 == Fix || data->cmt->procVal1 == ScaleMap || data->cmt->procVal1 == Dynamic || 
            data->cmt->procVal1 == Random || data->cmt->procVal1 == Flip)) 
        {
            procVal1b->setSuffix(" - " + pitch2string(val));
        }
      else
      {
            if(!procVal1b->suffix().isEmpty())
              procVal1b->setSuffix(QString(""));
      }      
      }

//---------------------------------------------------------
//   procVal2aChanged
//---------------------------------------------------------

void MidiTransformerDialog::procVal2aChanged(int val)
      {
      data->cmt->procVal2a = val;
      }

//---------------------------------------------------------
//   procVal2bChanged
//---------------------------------------------------------

void MidiTransformerDialog::procVal2bChanged(int val)
      {
      data->cmt->procVal2b = val;
      }

//---------------------------------------------------------
//   procLenAChanged
//---------------------------------------------------------

void MidiTransformerDialog::procLenAChanged(int val)
      {
      data->cmt->procLenA = val;
      }

//---------------------------------------------------------
//   procPosAChanged
//---------------------------------------------------------

void MidiTransformerDialog::procPosAChanged(int val)
      {
      data->cmt->procPosA = val;
      }

//---------------------------------------------------------
//   funcQuantValSel
//---------------------------------------------------------

void MidiTransformerDialog::funcQuantValSel(int val)
      {
      data->cmt->quantVal = val;
      }

//---------------------------------------------------------
//   processAllChanged
//---------------------------------------------------------

void MidiTransformerDialog::processAllChanged(bool val)
      {
      if (val == true) {
            selectedTracks->setChecked(false);
            insideLoop->setChecked(false);
            data->cmt->selectedTracks = false;
            data->cmt->insideLoop = false;
            }
      }

//---------------------------------------------------------
//   selectedTracksChanged
//---------------------------------------------------------

void MidiTransformerDialog::selectedTracksChanged(bool val)
      {
      data->cmt->selectedTracks = val;
      processAll->setChecked(!val && !data->cmt->insideLoop);
      }

//---------------------------------------------------------
//   insideLoopChanged
//---------------------------------------------------------

void MidiTransformerDialog::insideLoopChanged(bool val)
      {
      data->cmt->insideLoop = val;
      processAll->setChecked(!data->cmt->selectedTracks && !val);
      }



/*!
    \fn MidiTransformerDialog::typesMatch(MidiEvent e, unsigned t)
 */
bool MidiTransformerDialog::typesMatch(Event& e, unsigned selType)
      {
      bool matched = false;
      switch (selType)
         {
         case MIDITRANSFORM_NOTE:
            matched = (e.type() == Note);
            break;
         case MIDITRANSFORM_POLY:
            matched = (e.type() == PAfter);
            break;
         case MIDITRANSFORM_CTRL:
            matched = (e.type() == Controller);
            break;
         case MIDITRANSFORM_ATOUCH:
            matched = (e.type() == CAfter);
            break;
         case MIDITRANSFORM_PITCHBEND:
            {
            if (e.type() == Controller) {
                  MidiController::ControllerType c = midiControllerType(e.dataA());
                  matched = (c == MidiController::Pitch);
                  }
            break;
            }
         case MIDITRANSFORM_NRPN:
            {
            if (e.type() == Controller) {
                  MidiController::ControllerType c = midiControllerType(e.dataA());
                  matched = (c == MidiController::NRPN);
                  }
            }
         case MIDITRANSFORM_RPN:
            {
            if (e.type() == Controller) {
                  MidiController::ControllerType c = midiControllerType(e.dataA());
                  matched = (c == MidiController::RPN);
                  }
            }
         default:
            fprintf(stderr, "Error matching type in MidiTransformerDialog: unknown eventtype!\n");
            break;
         }
      //printf("Event type=%d, selType =%d matched=%d\n", e.type(), selType, matched);
      return matched;
      }
