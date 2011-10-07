//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midiitransform.cpp,v 1.2.2.2 2009/05/24 21:43:44 terminator356 Exp $
//
//  (C) Copyright 2001-2003 Werner Schweer (ws@seh.de)
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
#include <QCloseEvent>

#include <QButtonGroup>
#include <QDialog>
#include <QListWidgetItem>

#include "spinboxFP.h"
#include "midi.h"
#include "midictrl.h"
#include "event.h"
#include "mpevent.h"
#include "midiitransform.h"
#include "track.h"
#include "song.h"
#include "xml.h"
#include "globals.h"
#include "gconfig.h"
//#include "comboQuant.h"
//#include "pitchedit.h"
#include "helper.h"

#define MIDITRANSFORM_NOTE        0
#define MIDITRANSFORM_POLY        1
#define MIDITRANSFORM_CTRL        2
#define MIDITRANSFORM_ATOUCH      3
#define MIDITRANSFORM_PITCHBEND   4
#define MIDITRANSFORM_NRPN        5
#define MIDITRANSFORM_RPN         6

namespace MusECore {

static int selTypeTable[] = {
      MIDITRANSFORM_NOTE, MIDITRANSFORM_POLY, MIDITRANSFORM_CTRL, MIDITRANSFORM_ATOUCH,
         MIDITRANSFORM_PITCHBEND, MIDITRANSFORM_NRPN, MIDITRANSFORM_RPN
      };
      
static int procTypeTable[] = {
      MIDITRANSFORM_POLY, MIDITRANSFORM_CTRL, MIDITRANSFORM_ATOUCH,
         MIDITRANSFORM_PITCHBEND, MIDITRANSFORM_NRPN, MIDITRANSFORM_RPN
      };

static int procVal2Map[] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11 };

struct TDict {
      TransformFunction id;
      const QString text;
      TDict(TransformFunction f, const QString& s) : id(f), text(s) {}
      };

static const TDict oplist[] = {
      TDict(Transform, QString("Transform")),
      TDict(Delete, QString("Filter"))
      };

//---------------------------------------------------------
//   MidiInputTransform
//---------------------------------------------------------

class MidiInputTransformation {
   public:
      QString name;
      QString comment;

      ValOp selEventOp;
      int selType;

      ValOp selVal1;
      int selVal1a, selVal1b;
      ValOp selVal2;
      int selVal2a, selVal2b;
      ValOp selPort;
      int selPorta, selPortb;
      ValOp selChannel;
      int selChannela, selChannelb;

      InputTransformProcEventOp procEvent;
      int eventType;
      
      TransformOperator procVal1;
      int procVal1a, procVal1b;
      TransformOperator procVal2;
      int procVal2a, procVal2b;
      TransformOperator procPort;
      int procPorta, procPortb;
      TransformOperator procChannel;
      int procChannela, procChannelb;

      TransformFunction funcOp;
      int quantVal;

      MidiInputTransformation(const QString& s) {
            name         = s;
            selEventOp   = All;
            selType      = MIDITRANSFORM_NOTE;
            selVal1      = Ignore;
            selVal1a     = 0;
            selVal1b     = 0;
            selVal2      = Ignore;
            selVal2a     = 0;
            selVal2b     = 0;
            procEvent    = KeepType;
            eventType    = MIDITRANSFORM_POLY;
            procVal1     = Keep;
            procVal1a    = 0;
            procVal1b    = 0;
            procVal2     = Keep;
            procVal2a    = 0;
            procVal2b    = 0;
            funcOp       = Transform;
            quantVal     = MusEGlobal::config.division;
            selPort      = Ignore;
            selChannel   = Ignore;
            selChannela  = 0;
            selChannelb  = 0;
            procPort     = Keep;
            procChannel  = Keep;
            procPorta    = 0;
            procPortb    = 0;
            procChannela = 0;
            procChannelb = 0;
            }
      void write(int level, Xml& xml) const;
      int apply(MidiRecordEvent& ev) const;
      bool typesMatch(MidiRecordEvent& e, int selType) const;
      };

typedef std::list<MidiInputTransformation*> MidiInputTransformationList;
typedef std::list<MidiInputTransformation*>::iterator iMidiInputTransformation;
typedef std::list<MidiInputTransformation*>::const_iterator ciMidiInputTransformation;

// this is the list of defined transformations:
static MidiInputTransformationList mtlist;

// list of modules to apply:

struct ITransModul {
      bool valid;
      MidiInputTransformation* transform;
      };

const int MIDI_INPUT_TRANSFORMATIONS = 4;
static ITransModul modules[MIDI_INPUT_TRANSFORMATIONS];

//---------------------------------------------------------
//   applyMidiInputTransformation
//    return false if event should be dropped
//    (filter)
//---------------------------------------------------------

bool applyMidiInputTransformation(MidiRecordEvent& event)
      {
      for (int i = 0; i < 4; ++i) {
            if (modules[i].valid && modules[i].transform) {
                  int rv = modules[i].transform->apply(event);
                  if (rv == 1)
                  {
                      if(MusEGlobal::debugMsg)
                          printf("drop input event\n");
                  }      
                  if (rv)
                        return rv != 1;
                  }
            }
      return true;
      }

//---------------------------------------------------------
//   filterValOp
//---------------------------------------------------------

static bool filterValOp(ValOp op, int val, int val1, int val2)
      {
      switch (op) {
            case Ignore:
                  break;
            case Equal:
                  if (val != val1)
                        return true;
                  break;
            case Unequal:
                  if (val == val1)
                        return true;
                  break;
            case Higher:
                  if (val <= val1)
                        return true;
                  break;
            case Lower:
                  if (val >= val1)
                        return true;
                  break;
            case Inside:
                  if ((val < val1) || (val >= val2))
                        return true;
                  break;
            case Outside:
                  if ((val >= val1) && (val < val2))
                        return true;
                  break;
            }
      return false;
      }

//---------------------------------------------------------
//   apply
//    apply Select filter
//    return  0 - not applied
//            1 - drop event
//            2 - event changed
//---------------------------------------------------------

int MidiInputTransformation::apply(MidiRecordEvent& event) const
      {
      int t = event.type();
      
      switch (selEventOp) {
            case Equal:
                  switch(t) {
                        case ME_NOTEON:
                        case ME_NOTEOFF:
                              if (selType != MIDITRANSFORM_NOTE)
                                    return 0;
                              break;
                        default:
                              if(!typesMatch(event, selType))
                                    return 0;
                              break;
                        }
                  break;
            case Unequal:
                  switch(event.type()) {
                        case ME_NOTEON:
                        case ME_NOTEOFF:
                              if (selType == MIDITRANSFORM_NOTE)
                                    return 0;
                              break;
                        default:
                              if(typesMatch(event, selType))
                                    return 0;
                              break;
                        }
                  break;
            default:
                  break;
            }
      if (filterValOp(selVal1, event.dataA(), selVal1a, selVal1b))
            return 0;
      if (filterValOp(selVal2, event.dataB(), selVal2a, selVal2b))
            return 0;
      if (filterValOp(selPort, event.port(), selPorta, selPortb))
            return 0;
      if (filterValOp(selChannel, event.channel(), selChannela, selChannelb))
            return 0;

      if (funcOp == Delete)
            return 1;     // discard event

      // transform event
//printf("transform\n");
      if (procEvent != KeepType)
      {
        switch(eventType)
        {
          case MIDITRANSFORM_POLY:
            event.setType(ME_POLYAFTER);
          break;
          case MIDITRANSFORM_CTRL:
            event.setType(ME_CONTROLLER);
          break;
          case MIDITRANSFORM_ATOUCH:
            event.setType(ME_AFTERTOUCH);
          break;
          case MIDITRANSFORM_PITCHBEND:
          {
            event.setType(ME_PITCHBEND);
          }  
          break;
          case MIDITRANSFORM_NRPN: 
          {
            event.setA(MidiController::NRPN);
            event.setType(ME_CONTROLLER);
          }  
          break;
          case MIDITRANSFORM_RPN:
          {
            event.setA(MidiController::RPN);
            event.setType(ME_CONTROLLER);
          }  
          break;
          default:
          break;
        }
      }

      //---------------------------------------------------
      //    transform value A
      //---------------------------------------------------

      int val = event.dataA();
      switch (procVal1) {
            case Keep:
                  break;
            case Plus:
                  val += procVal1a;
                  break;
            case Minus:
                  val -= procVal1a;
                  break;
            case Multiply:
                  val = int(val * (procVal1a/100.0) + .5);
                  break;
            case Divide:
                  val = int(val / (procVal1a/100.0) + .5);
                  break;
            case Fix:
                  val = procVal1a;
                  break;
            case Value:
                  val = procVal2a;
                  break;
            case Invert:
                  val = 127 - val;
                  break;
            case ScaleMap:
                  printf("scale map not implemented\n");
                  break;
            case Flip:
                  val = procVal1a - val;
                  break;
            case Dynamic:           // "crescendo"
                  printf("transform not implemented\n");
                  break;
            case Random:
                  {
                  int range = procVal1b - procVal1a;
                  if (range > 0)
                        val = (rand() % range) + procVal1a;
                  else if (range < 0)
                        val = (rand() % -range) + procVal1b;
                  else
                        val = procVal1a;
                  }
                  break;
            }
      if (val < 0)
            val = 0;
      if (val > 127)
            val = 127;
      event.setA(val);

      //---------------------------------------------------
      //    transform value B
      //---------------------------------------------------

      val = event.dataB();
      switch (procVal2) {
            case Plus:
                  val += procVal2a;
                  break;
            case Minus:
                  val -= procVal2a;
                  break;
            case Multiply:
                  val = int(val * (procVal2a/100.0) + .5);
                  break;
            case Divide:
                  val = int(val / (procVal2a/100.0) + .5);
                  break;
            case Fix:
                  val = procVal2a;
                  break;
            case Value:
                  val = procVal1a;
                  break;
            case Invert:
                  val = 127 - val;
                  break;
            case Dynamic:
                  printf("transform not implemented\n");
                  break;
            case Random:
                  {
                  int range = procVal2b - procVal2a;
                  if (range > 0)
                        val = (rand() % range) + procVal2a;
                  else if (range < 0)
                        val = (rand() % -range) + procVal2b;
                  else
                        val = procVal2a;
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
      event.setB(val);

      //---------------------------------------------------
      //    transform port
      //---------------------------------------------------

      val = event.port();
      switch (procPort) {
            case Plus:
                  val += procPorta;
                  break;
            case Minus:
                  val -= procPorta;
                  break;
            case Multiply:
                  val = int(val * (procPorta/100.0) + .5);
                  break;
            case Divide:
                  val = int(val / (procPorta/100.0) + .5);
                  break;
            case Fix:
                  val = procPorta;
                  break;
            case Value:
                  val = procPorta;
                  break;
            case Invert:
                  val = 15 - val;
                  break;
            case Dynamic:
                  printf("transform not implemented\n");
                  break;
            case Random:
                  {
                  int range = procPortb - procPorta;
                  if (range > 0)
                        val = (rand() % range) + procPorta;
                  else if (range < 0)
                        val = (rand() % -range) + procPortb;
                  else
                        val = procPorta;
                  }
                  break;
            case ScaleMap:
            case Keep:
            case Flip:
                  break;
            }
      if (val < 0)
            val = 0;
      if (val > 15)
            val = 15;
      event.setPort(val);

      //---------------------------------------------------
      //    transform channel
      //---------------------------------------------------

      val = event.channel();
      switch (procChannel) {
            case Plus:
                  val += procChannela;
                  break;
            case Minus:
                  val -= procChannela;
                  break;
            case Multiply:
                  val = int(val * (procChannela/100.0) + .5);
                  break;
            case Divide:
                  val = int(val / (procChannela/100.0) + .5);
                  break;
            case Fix:
                  val = procChannela;
                  break;
            case Value:
                  val = procChannela;
                  break;
            case Invert:
                  val = 16 - val;
                  break;
            case Dynamic:
                  printf("transform not implemented\n");
                  break;
            case Random:
                  {
                  int range = procChannelb - procChannela;
                  if (range > 0)
                        val = (rand() % range) + procChannela;
                  else if (range < 0)
                        val = (rand() % -range) + procChannelb;
                  else
                        val = procChannela;
                  }
                  break;
            case ScaleMap:
            case Keep:
            case Flip:
                  break;
            }
      if (val < 0)
            val = 0;
      if (val > 15)
            val = 15;
      event.setChannel(val);

      return 2;
      }

//---------------------------------------------------------
//   typesMatch
//---------------------------------------------------------

bool MidiInputTransformation::typesMatch(MidiRecordEvent& e, int selType) const
      {
      bool matched = false;
      int t = e.type();
      switch (selType)
         {
         case MIDITRANSFORM_NOTE:
            matched = ((t == ME_NOTEON) || (t == ME_NOTEOFF));
            break;
         case MIDITRANSFORM_POLY:
            matched = (t == ME_POLYAFTER);
            break;
         case MIDITRANSFORM_CTRL:
            matched = (t == ME_CONTROLLER);
            break;
         case MIDITRANSFORM_ATOUCH:
            matched = (t == ME_AFTERTOUCH);
            break;
         case MIDITRANSFORM_PITCHBEND:
            {
            //if (t == ME_CONTROLLER) {
            //      MidiController::ControllerType c = midiControllerType(e.dataA());
            //      matched = (c == MidiController::Pitch);
            matched = (t = ME_PITCHBEND);
            }
            break;
         case MIDITRANSFORM_NRPN:
            {
            if (t == ME_CONTROLLER) {
                  MidiController::ControllerType c = midiControllerType(e.dataA());
                  matched = (c == MidiController::NRPN);
                  }
            }
            break;
         case MIDITRANSFORM_RPN:
            {
            if (t == ME_CONTROLLER) {
                  MidiController::ControllerType c = midiControllerType(e.dataA());
                  matched = (c == MidiController::RPN);
                  }
            }
            break;
         default:
            fprintf(stderr, "Error matching type in MidiTransformerDialog: unknown eventtype!\n");
            break;
         }
      //printf("Event type=%d, selType =%d matched=%d\n", e.type(), selType, matched);
      return matched;
      }

//---------------------------------------------------------
//   writeMidiTransforms
//---------------------------------------------------------

void writeMidiInputTransforms(int level, Xml& xml)
      {
      for (iMidiInputTransformation i = mtlist.begin(); i != mtlist.end(); ++i) {
            (*i)->write(level, xml);
            }
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MidiInputTransformation::write(int level, Xml& xml) const
      {
      xml.tag(level++, "midiInputTransform");
      xml.strTag(level, "name", name);
      xml.strTag(level, "comment", comment);
      xml.intTag(level, "function", int(funcOp));

      // apply this transformation?
      for (int i = 0; i < MIDI_INPUT_TRANSFORMATIONS; ++i) {
            if (modules[i].transform == this) {
                  xml.intTag(level, "apply", int(modules[i].valid));
                  break;
                  }
            }
      if (funcOp == Quantize) {
            xml.intTag(level, "quantVal", quantVal);
            }
      if (funcOp == Transform || funcOp == Insert) {
            if (procEvent != KeepType) {
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
            if (procPort != Keep) {
                  xml.intTag(level, "procPortOp", int(procPort));
                  xml.intTag(level, "procPorta", procPorta);
                  xml.intTag(level, "procPortb", procPortb);
                  }
            if (procChannel != Keep) {
                  xml.intTag(level, "procChannelOp", int(procChannel));
                  xml.intTag(level, "procChannela", procChannela);
                  xml.intTag(level, "procChannelb", procChannelb);
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
      if (selPort != Ignore) {
            xml.intTag(level, "selPortOp", int(selPort));
            xml.intTag(level, "selPorta", selPorta);
            xml.intTag(level, "selPortb", selPortb);
            }
      if (selChannel != Ignore) {
            xml.intTag(level, "selChannelOp", int(selChannel));
            xml.intTag(level, "selChannela", selChannela);
            xml.intTag(level, "selChannelb", selChannelb);
            }
      xml.etag(level, "midiInputTransform");
      }

//---------------------------------------------------------
//   readMidiTransform
//---------------------------------------------------------

void readMidiInputTransform(Xml& xml)
      {
      MidiInputTransformation trans(QString("new"));
      int apply = -1;

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
                        else if (tag == "quantVal")
                              trans.quantVal = xml.parseInt();
                        else if (tag == "procEventOp")
                              trans.procEvent = InputTransformProcEventOp(xml.parseInt());
                        else if (tag == "eventType")
                              trans.eventType = xml.parseInt();
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
                        else if (tag == "selEventOp")
                              trans.selEventOp = ValOp(xml.parseInt());
                        else if (tag == "selEventType")
                              trans.selType = xml.parseInt();
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

                        else if (tag == "procPortOp")
                              trans.procPort = TransformOperator(xml.parseInt());
                        else if (tag == "procPorta")
                              trans.procPorta = xml.parseInt();
                        else if (tag == "procPortb")
                              trans.procPortb = xml.parseInt();
                        else if (tag == "procChannelOp")
                              trans.procChannel = TransformOperator(xml.parseInt());
                        else if (tag == "procChannela")
                              trans.procChannela = xml.parseInt();
                        else if (tag == "procChannelb")
                              trans.procChannelb = xml.parseInt();

                        else if (tag == "selPortOp")
                              trans.selPort = ValOp(xml.parseInt());
                        else if (tag == "selPorta")
                              trans.selPorta = xml.parseInt();
                        else if (tag == "selPortb")
                              trans.selPortb = xml.parseInt();
                        else if (tag == "selChannelOp")
                              trans.selChannel = ValOp(xml.parseInt());
                        else if (tag == "selChannela")
                              trans.selChannela = xml.parseInt();
                        else if (tag == "selChannelb")
                              trans.selChannelb = xml.parseInt();

                        else if (tag == "apply")
                              apply = xml.parseInt();
                        else
                              xml.unknown("midiInputTransform");
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "midiInputTransform") {
// printf("midi Input transform <%s> apply %d\n",
//   trans.name.toLatin1().constData(), apply);
                              
                              // By T356. A previous bug caused some .med files to grow very large
                              //  with duplicate transforms. Here we can eliminate those duplicates.
                              for(iMidiInputTransformation i = mtlist.begin(); i != mtlist.end(); ++i) 
                              {
                                if((*i)->name == trans.name)
                                {
                                  return;
                                }  
                              }
                              
                              MidiInputTransformation* t = new MidiInputTransformation(trans);
                              // search free slot in modules
                              if (apply != -1) {
                                    for (int i = 0; i < MIDI_INPUT_TRANSFORMATIONS; ++i) {
                                          if (modules[i].transform == 0) {
                                                modules[i].transform = t;
                                                modules[i].valid = apply;
                                                break;
                                                }
                                          }
                                    }
                              mtlist.push_back(t);
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   clearMidiInputTransforms
//---------------------------------------------------------

void clearMidiInputTransforms()
{
  for (int i = 0; i < MIDI_INPUT_TRANSFORMATIONS; ++i) 
  {
    modules[i].transform = 0;
    modules[i].valid = false;
  }
  for (iMidiInputTransformation i = mtlist.begin(); i != mtlist.end(); ++i) 
  {
    MidiInputTransformation* t = *i;
    if(t)
      delete t;
  }
  mtlist.clear();
}

} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   MidiInputTransformDialog
//    Widgets:
//    presetList nameEntry commentEntry
//    selEventOp   selType
//    selVal1Op    selVal1a selVal1b
//    selVal2Op    selVal2a selVal2b
//
//    procEventOp  procType
//    procVal1Op   procVal1a procVal1b
//    procVal2Op   procVal2a procVal2b
//    funcOp       funcQuantVal
//    buttonNew    buttonDelete
//
//    modulGroup
//    modul1select  modul1enable
//    modul2select  modul2enable
//    modul3select  modul3enable
//    modul4select  modul4enable
//
//    selPortOp     selPortVala    selPortValb
//    selChannelOp  selChannelVala selChannelValb
//
//    procPortOp    procPortVala    procPortValb
//    procChannelOp procChannelVala procChannelValb
//---------------------------------------------------------

MidiInputTransformDialog::MidiInputTransformDialog(QDialog* parent, Qt::WFlags fl)
   : QDialog(parent, fl)
      {
      setupUi(this);
      cindex = 0;
      cmodul = 0;
      cmt    = 0;

      modulGroup = new QButtonGroup;
      modulGroup->addButton(modul1select,0);
      modulGroup->addButton(modul2select,1);
      modulGroup->addButton(modul3select,2);
      modulGroup->addButton(modul4select,3);

      for (unsigned i = 0; i < sizeof(MusECore::oplist)/sizeof(*MusECore::oplist); ++i)
            funcOp->insertItem(i, MusECore::oplist[i].text);

      connect(buttonNew,    SIGNAL(clicked()),      SLOT(presetNew()));
      connect(buttonDelete, SIGNAL(clicked()),      SLOT(presetDelete()));
      connect(selEventOp,   SIGNAL(activated(int)), SLOT(selEventOpSel(int)));
      connect(selType,      SIGNAL(activated(int)), SLOT(selTypeSel(int)));
      connect(selVal1Op,    SIGNAL(activated(int)), SLOT(selVal1OpSel(int)));
      connect(selVal2Op,    SIGNAL(activated(int)), SLOT(selVal2OpSel(int)));
      connect(procEventOp,  SIGNAL(activated(int)), SLOT(procEventOpSel(int)));
      connect(procType,     SIGNAL(activated(int)), SLOT(procEventTypeSel(int)));
      connect(procVal1Op,   SIGNAL(activated(int)), SLOT(procVal1OpSel(int)));
      connect(procVal2Op,   SIGNAL(activated(int)), SLOT(procVal2OpSel(int)));
      connect(funcOp,       SIGNAL(activated(int)), SLOT(funcOpSel(int)));
      connect(presetList,   SIGNAL(itemActivated(QListWidgetItem*)),
         SLOT(presetChanged(QListWidgetItem*)));
      connect(nameEntry,    SIGNAL(textChanged(const QString&)),
         SLOT(nameChanged(const QString&)));
      connect(commentEntry,    SIGNAL(textChanged()), SLOT(commentChanged()));

      connect(selVal1a,  SIGNAL(valueChanged(int)), SLOT(selVal1aChanged(int)));
      connect(selVal1b,  SIGNAL(valueChanged(int)), SLOT(selVal1bChanged(int)));
      connect(selVal2a,  SIGNAL(valueChanged(int)), SLOT(selVal2aChanged(int)));
      connect(selVal2b,  SIGNAL(valueChanged(int)), SLOT(selVal2bChanged(int)));
      connect(procVal1a, SIGNAL(valueChanged(int)), SLOT(procVal1aChanged(int)));
      connect(procVal1b, SIGNAL(valueChanged(int)), SLOT(procVal1bChanged(int)));
      connect(procVal2a, SIGNAL(valueChanged(int)), SLOT(procVal2aChanged(int)));
      connect(procVal2b, SIGNAL(valueChanged(int)), SLOT(procVal2bChanged(int)));

      connect(modul1enable, SIGNAL(toggled(bool)), SLOT(modul1enableChanged(bool)));
      connect(modul2enable, SIGNAL(toggled(bool)), SLOT(modul2enableChanged(bool)));
      connect(modul3enable, SIGNAL(toggled(bool)), SLOT(modul3enableChanged(bool)));
      connect(modul4enable, SIGNAL(toggled(bool)), SLOT(modul4enableChanged(bool)));
      connect(modulGroup,   SIGNAL(buttonClicked(int)),  SLOT(changeModul(int)));

      connect(selPortOp,   SIGNAL(activated(int)), SLOT(selPortOpSel(int)));
      connect(selPortVala, SIGNAL(valueChanged(int)), SLOT(selPortValaChanged(int)));
      connect(selPortValb, SIGNAL(valueChanged(int)), SLOT(selPortValbChanged(int)));

      connect(selChannelOp,   SIGNAL(activated(int)), SLOT(selChannelOpSel(int)));
      connect(selChannelVala, SIGNAL(valueChanged(int)), SLOT(selChannelValaChanged(int)));
      connect(selChannelValb, SIGNAL(valueChanged(int)), SLOT(selChannelValbChanged(int)));

      connect(procPortOp,   SIGNAL(activated(int)), SLOT(procPortOpSel(int)));
      connect(procPortVala, SIGNAL(valueChanged(int)), SLOT(procPortValaChanged(int)));
      connect(procPortValb, SIGNAL(valueChanged(int)), SLOT(procPortValbChanged(int)));

      connect(procChannelOp,   SIGNAL(activated(int)), SLOT(procChannelOpSel(int)));
      connect(procChannelVala, SIGNAL(valueChanged(int)), SLOT(procChannelValaChanged(int)));
      connect(procChannelValb, SIGNAL(valueChanged(int)), SLOT(procChannelValbChanged(int)));

      //---------------------------------------------------
      //  populate preset list
      //---------------------------------------------------

      updatePresetList();
      presetList->setCurrentItem(presetList->item(0));
      presetChanged(presetList->item(0));
      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiInputTransformDialog::songChanged(int flags)
{
  // Whenever a song is loaded, flags is -1. Since transforms are part of configuration, 
  //  use SC_CONFIG here, to filter unwanted song change events.
  if(flags & SC_CONFIG)
    updatePresetList();
}

//---------------------------------------------------------
//   updatePresetList
//---------------------------------------------------------

void MidiInputTransformDialog::updatePresetList()
{
      cmt = 0;
      presetList->clear();
      
      modul1select->setChecked(true);
      for (MusECore::iMidiInputTransformation i = MusECore::mtlist.begin(); i != MusECore::mtlist.end(); ++i) {
            presetList->addItem((*i)->name);
            if (cmt == 0)
                  cmt = *i;
            }
      if (cmt == 0) {
            // create default "New" preset
            cmt = new MusECore::MidiInputTransformation(tr("New"));
            MusECore::mtlist.push_back(cmt);
            presetList->addItem(tr("New"));
            presetList->setCurrentItem(0);
            }
      changeModul(0);

      modul1enable->setChecked(MusECore::modules[0].valid);
      modul2enable->setChecked(MusECore::modules[1].valid);
      modul3enable->setChecked(MusECore::modules[2].valid);
      modul4enable->setChecked(MusECore::modules[3].valid);
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MidiInputTransformDialog::closeEvent(QCloseEvent* ev)
      {
      emit hideWindow();
      QWidget::closeEvent(ev);
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void MidiInputTransformDialog::accept()
      {
      reject();
      }

//---------------------------------------------------------
//   reject
//---------------------------------------------------------

void MidiInputTransformDialog::reject()
      {
      close();
      }

//---------------------------------------------------------
//   setValOp
//---------------------------------------------------------

void MidiInputTransformDialog::setValOp(QWidget* a, QWidget* b, MusECore::ValOp op)
      {
      switch (op) {
            case MusECore::Ignore:
                  a->setEnabled(false);
                  b->setEnabled(false);
                  break;
            case MusECore::Equal:
            case MusECore::Unequal:
            case MusECore::Higher:
            case MusECore::Lower:
                  a->setEnabled(true);
                  b->setEnabled(false);
                  break;
            case MusECore::Inside:
            case MusECore::Outside:
                  a->setEnabled(true);
                  b->setEnabled(true);
                  break;
            }
      }

//---------------------------------------------------------
//   selEventOpSel
//---------------------------------------------------------

void MidiInputTransformDialog::selEventOpSel(int val)
      {
      selType->setEnabled(val != MusECore::All);
      cmt->selEventOp = MusECore::ValOp(val);
      selVal1aChanged(cmt->selVal1a);
      selVal1bChanged(cmt->selVal1b);
      }

//---------------------------------------------------------
//   selTypeSel
//---------------------------------------------------------

void MidiInputTransformDialog::selTypeSel(int val)
      {
      cmt->selType = MusECore::selTypeTable[val];
      selVal1aChanged(cmt->selVal1a);
      selVal1bChanged(cmt->selVal1b);
      }

//---------------------------------------------------------
//   selVal1OpSel
//---------------------------------------------------------

void MidiInputTransformDialog::selVal1OpSel(int val)
      {
      setValOp(selVal1a, selVal1b, MusECore::ValOp(val));
      cmt->selVal1 = MusECore::ValOp(val);
      }

//---------------------------------------------------------
//   selVal2OpSel
//---------------------------------------------------------

void MidiInputTransformDialog::selVal2OpSel(int val)
      {
      setValOp(selVal2a, selVal2b, MusECore::ValOp(val));
      cmt->selVal2 = MusECore::ValOp(val);
      }

//---------------------------------------------------------
//   procEventOpSel
//---------------------------------------------------------

void MidiInputTransformDialog::procEventOpSel(int val)
      {
      MusECore::InputTransformProcEventOp op = val == 0 ? MusECore::KeepType : MusECore::FixType;
      procType->setEnabled(op == MusECore::FixType);
      cmt->procEvent = op;
      
      procVal1aChanged(cmt->procVal1a);
      procVal1bChanged(cmt->procVal1b);
      }

//---------------------------------------------------------
//   procEventTypeSel
//---------------------------------------------------------

void MidiInputTransformDialog::procEventTypeSel(int val)
      {
      cmt->eventType = MusECore::procTypeTable[val];
      procVal1aChanged(cmt->procVal1a);
      procVal1bChanged(cmt->procVal1b);
      }

//---------------------------------------------------------
//   procVal1OpSel
//---------------------------------------------------------

void MidiInputTransformDialog::procVal1OpSel(int val)
      {
      cmt->procVal1 = MusECore::TransformOperator(val);
      switch(MusECore::TransformOperator(val)) {
            case MusECore::Keep:
            case MusECore::Invert:
                  procVal1a->setEnabled(false);
                  procVal1b->setEnabled(false);
                  break;
            case MusECore::Multiply:
            case MusECore::Divide:
                  procVal1a->setEnabled(true);
                  procVal1a->setDecimals(2);
                  procVal1b->setEnabled(false);
                  break;
            case MusECore::Plus:
            case MusECore::Minus:
            case MusECore::Fix:
            case MusECore::Value:
            case MusECore::Flip:
                  procVal1a->setDecimals(0);
                  procVal1a->setEnabled(true);
                  procVal1b->setEnabled(false);
                  break;
            case MusECore::Random:
            case MusECore::ScaleMap:
            case MusECore::Dynamic:
                  procVal1a->setDecimals(0);
                  procVal1a->setEnabled(true);
                  procVal1b->setEnabled(true);
                  break;
            }
      procVal1aChanged(cmt->procVal1a);
      procVal1bChanged(cmt->procVal1b);
      }

//---------------------------------------------------------
//   procVal2OpSel
//---------------------------------------------------------

void MidiInputTransformDialog::procVal2OpSel(int val)
      {
      MusECore::TransformOperator op = MusECore::TransformOperator(MusECore::procVal2Map[val]);
      cmt->procVal2 = op;

      switch (op) {
            case MusECore::Keep:
            case MusECore::Invert:
                  procVal2a->setEnabled(false);
                  procVal2b->setEnabled(false);
                  break;
            case MusECore::Multiply:
            case MusECore::Divide:
                  procVal2a->setEnabled(true);
                  procVal2a->setDecimals(2);
                  procVal2b->setEnabled(false);
                  break;
            case MusECore::Plus:
            case MusECore::Minus:
            case MusECore::Fix:
            case MusECore::Value:
                  procVal2a->setDecimals(0);
                  procVal2a->setEnabled(true);
                  procVal2b->setEnabled(false);
                  break;
            case MusECore::Random:
            case MusECore::Dynamic:
                  procVal2a->setDecimals(0);
                  procVal2a->setEnabled(true);
                  procVal2b->setEnabled(true);
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   funcOpSel
//---------------------------------------------------------

void MidiInputTransformDialog::funcOpSel(int val)
      {
      MusECore::TransformFunction op = MusECore::oplist[val].id;

      bool isFuncOp(op == MusECore::Transform);

      procEventOp->setEnabled(isFuncOp);
      procType->setEnabled(isFuncOp);
      procVal1Op->setEnabled(isFuncOp);
      procVal1a->setEnabled(isFuncOp);
      procVal1b->setEnabled(isFuncOp);
      procVal2Op->setEnabled(isFuncOp);
      procVal2a->setEnabled(isFuncOp);
      procVal2b->setEnabled(isFuncOp);
      procPortOp->setEnabled(isFuncOp);
      procPortVala->setEnabled(isFuncOp);
      procPortValb->setEnabled(isFuncOp);
      procChannelOp->setEnabled(isFuncOp);
      procChannelVala->setEnabled(isFuncOp);
      procChannelValb->setEnabled(isFuncOp);
      if (isFuncOp) {
            procEventOpSel(cmt->procEvent);
            procVal1OpSel(cmt->procVal1);
            procVal2OpSel(cmt->procVal2);
            procPortOpSel(cmt->procPort);
            procChannelOpSel(cmt->procChannel);
            }
      cmt->funcOp = op;
      }

//---------------------------------------------------------
//   presetNew
//---------------------------------------------------------

void MidiInputTransformDialog::presetNew()
      {
      QString name;
      for (int i = 0;; ++i) {
            name.sprintf("New-%d", i);
            MusECore::iMidiInputTransformation imt;
            for (imt = MusECore::mtlist.begin(); imt != MusECore::mtlist.end(); ++imt) {
                  if (name == (*imt)->name)
                        break;
                  }
            if (imt == MusECore::mtlist.end())
                  break;
            }
      MusECore::MidiInputTransformation* mt = new MusECore::MidiInputTransformation(name);
      QListWidgetItem* lbi      = new QListWidgetItem(name);
      presetList->addItem(lbi);
      MusECore::mtlist.push_back(mt);
      presetList->setCurrentItem(lbi);
      presetChanged(lbi);
      }

//---------------------------------------------------------
//   presetDelete
//---------------------------------------------------------

void MidiInputTransformDialog::presetDelete()
      {
      if (cindex != -1) {
            MusECore::iMidiInputTransformation mt = MusECore::mtlist.begin();
            for (int i = 0; i < cindex; ++i, ++mt) {
                  MusECore::mtlist.erase(mt);
                  presetList->setCurrentItem(presetList->item(cindex - 1));
                  presetList->takeItem(cindex);
                  presetChanged(presetList->item(cindex - 1));                  
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   nameChanged
//---------------------------------------------------------

void MidiInputTransformDialog::nameChanged(const QString& s)
      {
      cmt->name = s;
      QListWidgetItem* item = presetList->item(cindex);
      if (s != item->text()) {
            disconnect(presetList,   SIGNAL(itemActivated(QListWidgetItem*)),
               this, SLOT(presetChanged(QListWidgetItem*)));
            presetList->insertItem(cindex, s);
            presetList->takeItem(cindex+1);
            presetList->setCurrentItem(presetList->item(cindex));
            connect(presetList,   SIGNAL(itemActivated(QListWidgetItem*)),
               SLOT(presetChanged(QListWidgetItem*)));
            }
      }

//---------------------------------------------------------
//   commentChanged
//---------------------------------------------------------

void MidiInputTransformDialog::commentChanged()
      {
      cmt->comment = commentEntry->toPlainText();
      }

//---------------------------------------------------------
//   selVal1aChanged
//---------------------------------------------------------

void MidiInputTransformDialog::selVal1aChanged(int val)
      {
      cmt->selVal1a = val;
      if ((cmt->selEventOp != MusECore::All)
         && (cmt->selType == MIDITRANSFORM_NOTE)) {
            selVal1a->setSuffix(" - " + MusECore::pitch2string(val));
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

void MidiInputTransformDialog::selVal1bChanged(int val)
      {
      cmt->selVal1b = val;
      if ((cmt->selEventOp != MusECore::All)
         && (cmt->selType == MIDITRANSFORM_NOTE)) {
            selVal1b->setSuffix(" - " + MusECore::pitch2string(val));
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

void MidiInputTransformDialog::selVal2aChanged(int val)
      {
      cmt->selVal2a = val;
      }

//---------------------------------------------------------
//   selVal2bChanged
//---------------------------------------------------------

void MidiInputTransformDialog::selVal2bChanged(int val)
      {
      cmt->selVal2b = val;
      }

//---------------------------------------------------------
//   procVal1aChanged
//---------------------------------------------------------

void MidiInputTransformDialog::procVal1aChanged(int val)
      {
      cmt->procVal1a = val;
      
      if((cmt->procEvent == MusECore::KeepType && cmt->selType == MIDITRANSFORM_NOTE) && 
           (cmt->procVal1 == MusECore::Fix || cmt->procVal1 == MusECore::ScaleMap || cmt->procVal1 == MusECore::Dynamic || 
            cmt->procVal1 == MusECore::Random || cmt->procVal1 == MusECore::Flip)) 
        {
            procVal1a->setSuffix(" - " + MusECore::pitch2string(val));
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

void MidiInputTransformDialog::procVal1bChanged(int val)
      {
      cmt->procVal1b = val;
      
      if((cmt->procEvent == MusECore::KeepType && cmt->selType == MIDITRANSFORM_NOTE) && 
           (cmt->procVal1 == MusECore::Fix || cmt->procVal1 == MusECore::ScaleMap || cmt->procVal1 == MusECore::Dynamic || 
            cmt->procVal1 == MusECore::Random || cmt->procVal1 == MusECore::Flip)) 
        {
            procVal1b->setSuffix(" - " + MusECore::pitch2string(val));
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

void MidiInputTransformDialog::procVal2aChanged(int val)
      {
      cmt->procVal2a = val;
      }

//---------------------------------------------------------
//   procVal2bChanged
//---------------------------------------------------------

void MidiInputTransformDialog::procVal2bChanged(int val)
      {
      cmt->procVal2b = val;
      }

//---------------------------------------------------------
//   modul1enableChanged
//---------------------------------------------------------

void MidiInputTransformDialog::modul1enableChanged(bool val)
      {
      MusECore::modules[0].valid = val;
      }

//---------------------------------------------------------
//   modul2enableChanged
//---------------------------------------------------------

void MidiInputTransformDialog::modul2enableChanged(bool val)
      {
      MusECore::modules[1].valid = val;
      }

//---------------------------------------------------------
//   modul3enableChanged
//---------------------------------------------------------

void MidiInputTransformDialog::modul3enableChanged(bool val)
      {
      MusECore::modules[2].valid = val;
      }

//---------------------------------------------------------
//   modul4enableChanged
//---------------------------------------------------------

void MidiInputTransformDialog::modul4enableChanged(bool val)
      {
      MusECore::modules[3].valid = val;
      }

//---------------------------------------------------------
//   selPortOpSel
//---------------------------------------------------------

void MidiInputTransformDialog::selPortOpSel(int val)
      {
      setValOp(selPortVala, selPortValb, MusECore::ValOp(val));
      cmt->selPort = MusECore::ValOp(val);
      }

//---------------------------------------------------------
//   selPortValaChanged
//---------------------------------------------------------

void MidiInputTransformDialog::selPortValaChanged(int val)
      {
      cmt->selPorta = val;
      }

//---------------------------------------------------------
//   selPortValbChanged
//---------------------------------------------------------

void MidiInputTransformDialog::selPortValbChanged(int val)
      {
      cmt->selPortb = val;
      }

//---------------------------------------------------------
//   selChannelOpSel
//---------------------------------------------------------

void MidiInputTransformDialog::selChannelOpSel(int val)
      {
      setValOp(selChannelVala, selChannelValb, MusECore::ValOp(val));
      cmt->selChannel = MusECore::ValOp(val);
      }

//---------------------------------------------------------
//   selChannelValaChanged
//---------------------------------------------------------

void MidiInputTransformDialog::selChannelValaChanged(int val)
      {
      cmt->selChannela = val;
      }

//---------------------------------------------------------
//   selChannelValbChanged
//---------------------------------------------------------

void MidiInputTransformDialog::selChannelValbChanged(int val)
      {
      cmt->selChannelb = val;
      }

//---------------------------------------------------------
//   procPortOpSel
//---------------------------------------------------------

void MidiInputTransformDialog::procPortOpSel(int val)
      {
      cmt->procPort = MusECore::TransformOperator(val);
      switch(MusECore::TransformOperator(val)) {
            case MusECore::Keep:
            case MusECore::Invert:
                  procPortVala->setEnabled(false);
                  procPortValb->setEnabled(false);
                  break;
            case MusECore::Multiply:
            case MusECore::Divide:
                  procPortVala->setEnabled(true);
                  procPortVala->setDecimals(2);
                  procPortValb->setEnabled(false);
                  break;
            case MusECore::Plus:
            case MusECore::Minus:
            case MusECore::Fix:
            case MusECore::Value:
            case MusECore::Flip:
                  procPortVala->setDecimals(0);
                  procPortVala->setEnabled(true);
                  procPortValb->setEnabled(false);
                  break;
            case MusECore::Random:
            case MusECore::ScaleMap:
            case MusECore::Dynamic:
                  procPortVala->setDecimals(0);
                  procPortVala->setEnabled(true);
                  procPortValb->setEnabled(true);
                  break;
            }
      }

//---------------------------------------------------------
//   procPortValaChanged
//---------------------------------------------------------

void MidiInputTransformDialog::procPortValaChanged(int val)
      {
      cmt->procPorta = val;
      }

//---------------------------------------------------------
//   procPortValbChanged
//---------------------------------------------------------

void MidiInputTransformDialog::procPortValbChanged(int val)
      {
      cmt->procPortb = val;
      }

//---------------------------------------------------------
//   procChannelOpSel
//---------------------------------------------------------

void MidiInputTransformDialog::procChannelOpSel(int val)
      {
      cmt->procChannel = MusECore::TransformOperator(val);
      switch(MusECore::TransformOperator(val)) {
            case MusECore::Keep:
            case MusECore::Invert:
                  procChannelVala->setEnabled(false);
                  procChannelValb->setEnabled(false);
                  break;
            case MusECore::Multiply:
            case MusECore::Divide:
                  procChannelVala->setEnabled(true);
                  procChannelVala->setDecimals(2);
                  procChannelValb->setEnabled(false);
                  break;
            case MusECore::Plus:
            case MusECore::Minus:
            case MusECore::Fix:
            case MusECore::Value:
            case MusECore::Flip:
                  procChannelVala->setDecimals(0);
                  procChannelVala->setEnabled(true);
                  procChannelValb->setEnabled(false);
                  break;
            case MusECore::Random:
            case MusECore::ScaleMap:
            case MusECore::Dynamic:
                  procChannelVala->setDecimals(0);
                  procChannelVala->setEnabled(true);
                  procChannelValb->setEnabled(true);
                  break;
            }
      }

//---------------------------------------------------------
//   procChannelValaChanged
//---------------------------------------------------------

void MidiInputTransformDialog::procChannelValaChanged(int val)
      {
      cmt->procChannela = val;
      }

//---------------------------------------------------------
//   procChannelValbChanged
//---------------------------------------------------------

void MidiInputTransformDialog::procChannelValbChanged(int val)
      {
      cmt->procChannelb = val;
      }

//---------------------------------------------------------
//   changeModul
//---------------------------------------------------------

void MidiInputTransformDialog::changeModul(int k)
      {
//printf("change modul %d\n", k);

      cmodul = k;       // current modul

      if (MusECore::modules[k].transform == 0) {
            //printf("transform %d ist null\n", k);
            MusECore::modules[k].transform = cmt;
            }
      else {
            //---------------------------------------------
            //  search transformation in list
            //---------------------------------------------

            int idx = 0;
            MusECore::iMidiInputTransformation i;
            for (i = MusECore::mtlist.begin(); i != MusECore::mtlist.end(); ++i, ++idx) {
                  if (*i == MusECore::modules[k].transform) {
                        presetList->setCurrentItem(presetList->item(idx));
                        break;
                        }
                  }
            if (i == MusECore::mtlist.end())
                  printf("change to unknown transformation!\n");
            }
      }

//---------------------------------------------------------
//   presetChanged
//---------------------------------------------------------

void MidiInputTransformDialog::presetChanged(QListWidgetItem* item)
      {
      cindex = presetList->row(item);

      //---------------------------------------------------
      //   search transformation in list and set
      //   cmt
      //---------------------------------------------------

      MusECore::iMidiInputTransformation i;
      for (i = MusECore::mtlist.begin(); i != MusECore::mtlist.end(); ++i) {
            if (item->text() == (*i)->name) {
                  if(MusEGlobal::debugMsg)
                    printf("found %s\n", (*i)->name.toLatin1().constData());
                  cmt = *i;
                  if (cmodul != -1) {
                        MusECore::modules[cmodul].transform = *i;
                        }
                  break;
                  }
            }
      if (i == MusECore::mtlist.end()) {
            printf("MidiInputTransformDialog::presetChanged: not found\n");
            return;
            }
      nameEntry->setText(cmt->name);
      commentEntry->setText(cmt->comment);

      selEventOp->setCurrentIndex(cmt->selEventOp);
      selEventOpSel(cmt->selEventOp);

      for (unsigned i = 0; i < sizeof(MusECore::selTypeTable)/sizeof(*MusECore::selTypeTable); ++i) {
            if (MusECore::selTypeTable[i] == cmt->selType) {
                  selType->setCurrentIndex(i);
                  break;
                  }
            }

      selVal1Op->setCurrentIndex(cmt->selVal1);
      selVal1OpSel(cmt->selVal1);

      selVal2Op->setCurrentIndex(cmt->selVal2);
      selVal2OpSel(cmt->selVal2);

      selPortOp->setCurrentIndex(cmt->selPort);
      selPortOpSel(cmt->selPort);

      selChannelOp->setCurrentIndex(cmt->selChannel);
      selChannelOpSel(cmt->selChannel);

      {
      unsigned i;
      for (i = 0; i < sizeof(MusECore::oplist)/sizeof(*MusECore::oplist); ++i) {
            if (MusECore::oplist[i].id == cmt->funcOp) {
                  funcOp->setCurrentIndex(i);
                  break;
                  }
            }
      if (i == sizeof(MusECore::oplist)/sizeof(*MusECore::oplist))
            printf("internal error: bad OpCode\n");
      funcOpSel(i);
      }

      for (unsigned i = 0; i < sizeof(MusECore::procTypeTable)/sizeof(*MusECore::procTypeTable); ++i) {
            if (MusECore::procTypeTable[i] == cmt->eventType) {
                  procType->setCurrentIndex(i);
                  break;
                  }
            }

      procEventOp->setCurrentIndex(cmt->procEvent);
      procEventOpSel(cmt->procEvent);

      procVal1Op->setCurrentIndex(cmt->procVal1);
      procVal1OpSel(cmt->procVal1);

      for (unsigned i = 0; i < sizeof(MusECore::procVal2Map)/sizeof(*MusECore::procVal2Map); ++i) {
            if (MusECore::procVal2Map[i] == cmt->procVal2) {
                  procVal2Op->setCurrentIndex(i);
                  break;
                  }
            }

      selVal1a->setValue(cmt->selVal1a);
      selVal1b->setValue(cmt->selVal1b);
      selVal1aChanged(cmt->selVal1a);
      selVal1bChanged(cmt->selVal1b);

      selVal2a->setValue(cmt->selVal2a);
      selVal2b->setValue(cmt->selVal2b);

      selPortVala->setValue(cmt->selPorta);
      selPortValb->setValue(cmt->selPortb);

      selChannelVala->setValue(cmt->selChannela);
      selChannelValb->setValue(cmt->selChannelb);

      procVal1a->setValue(cmt->procVal1a);
      procVal1b->setValue(cmt->procVal1b);

      procVal2a->setValue(cmt->procVal2a);
      procVal2b->setValue(cmt->procVal2b);

      procPortVala->setValue(cmt->procPorta);
      procPortValb->setValue(cmt->procPortb);

      procChannelVala->setValue(cmt->procChannela);
      procChannelValb->setValue(cmt->procChannelb);
      
      procPortOp->setCurrentIndex(cmt->procPort);
      procPortOpSel(cmt->procPort);
      
      procChannelOp->setCurrentIndex(cmt->procChannel);
      procChannelOpSel(cmt->procChannel);
      
      }

} // namespace MusEGui
