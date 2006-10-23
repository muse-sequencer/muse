//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: transform.cpp,v 1.8 2005/11/17 18:19:30 wschweer Exp $
//
//  (C) Copyright 2001-2005 Werner Schweer (ws@seh.de)
//=========================================================

#include "midievent.h"
#include "transform.h"

static int eventTypeTable[] = {
      1, 6, 4, 7, 8, 10, 11
      };
static int procVal2Map[] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11 };

struct TDict {
      TransformFunction id;
      const QString text;
      TDict(TransformFunction f, const QString& s) : id(f), text(s) {}
      };

static const TDict oplist[] = {
      TDict(Trans, QString("Transform")),
      TDict(Delete, QString("Filter"))
      };

static const char* vall[] = {
      "c","c#","d","d#","e","f","f#","g","g#","a","a#","h"
      };
static const char* valu[] = {
      "C","C#","D","D#","E","F","F#","G","G#","A","A#","H"
      };

//---------------------------------------------------------
//   pitch2string
//---------------------------------------------------------

static QString pitch2string(int v)
      {
      if (v < 0 || v > 127)
            return QString("----");
      int octave = (v / 12) - 2;
      QString o;
      o.sprintf("%d", octave);
      int i = v % 12;
      QString s(octave < 0 ? valu[i] : vall[i]);
      return s + o;
      }
//---------------------------------------------------------
//   Transform
//---------------------------------------------------------

Transform::Transform(const char* name, const MempiHost* h)
   : Mempi(name, h)
      {
      }

bool Transform::init()
      {
      gui = new TransformDialog(this, 0);
      gui->setWindowTitle(QString(name()));
      gui->show();

      data.selEventOp   = All;
      data.selType      = 0x90;
      data.selVal1      = Ignore;
      data.selVal1a     = 0;
      data.selVal1b     = 0;
      data.selVal2      = Ignore;
      data.selVal2a     = 0;
      data.selVal2b     = 0;
      data.procEvent    = Keep;
      data.eventType    = 0x90;
      data.procVal1     = Keep;
      data.procVal1a    = 0;
      data.procVal1b    = 0;
      data.procVal2     = Keep;
      data.procVal2a    = 0;
      data.procVal2b    = 0;
      data.funcOp       = Trans;
      data.quantVal     = host->division();
      data.selChannel   = Ignore;
      data.selChannela  = 0;
      data.selChannelb  = 0;
      data.procChannel  = Keep;
      data.procChannela = 0;
      data.procChannelb = 0;
      return false;
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

void Transform::process(unsigned, unsigned, MPEventList* il, MPEventList* ol)
      {
      for (iMPEvent i = il->begin(); i != il->end(); ++i) {
            MidiEvent event(*i);
            if (filterEvent(event) == 1)
                  continue;
            ol->insert(event);
            }
      }

//---------------------------------------------------------
//   filterEvent
//---------------------------------------------------------

int Transform::filterEvent(MidiEvent& event)
      {
      switch (data.selEventOp) {
            case Equal:
                  switch(event.type()) {
                        case 0x90:
                        case 0x80:
                              if (data.selType != 0x90)
                                    return 0;
                              break;
                        default:
                              if (event.type() != data.selType)
                                    return 0;
                              break;
                        }
                  break;
            case Unequal:
                  switch(event.type()) {
                        case 0x90:
                        case 0x80:
                              if (data.selType == 0x90)
                                    return 0;
                              break;
                        default:
                              if (event.type() == data.selType)
                                    return 0;
                              break;
                        }
                  break;
            default:
                  break;
            }
      if (filterValOp(data.selVal1, event.dataA(), data.selVal1a, data.selVal1b))
            return 0;
      if (filterValOp(data.selVal2, event.dataB(), data.selVal2a, data.selVal2b))
            return 0;
      if (filterValOp(data.selChannel, event.channel(), data.selChannela, data.selChannelb))
            return 0;

      if (data.funcOp == Delete)
            return 1;     // discard event

      // transform event
      if (data.procEvent != Keep)
            event.setType(data.eventType);

      //---------------------------------------------------
      //    transform value A
      //---------------------------------------------------

      int val = event.dataA();
      switch (data.procVal1) {
            case Keep:
                  break;
            case Plus:
                  val += data.procVal1a;
                  break;
            case Minus:
                  val -= data.procVal1a;
                  break;
            case Multiply:
                  val = int(val * (data.procVal1a/100.0) + .5);
                  break;
            case Divide:
                  val = int(val / (data.procVal1a/100.0) + .5);
                  break;
            case Fix:
                  val = data.procVal1a;
                  break;
            case Value:
                  val = data.procVal2a;
                  break;
            case Invert:
                  val = 127 - val;
                  break;
            case ScaleMap:
                  printf("scale map not implemented\n");
                  break;
            case Flip:
                  val = data.procVal1a - val;
                  break;
            case Dynamic:           // "crescendo"
                  printf("transform not implemented\n");
                  break;
            case Random:
                  {
                  int range = data.procVal1b - data.procVal1a;
                  if (range > 0)
                        val = (rand() % range) + data.procVal1a;
                  else if (range < 0)
                        val = (rand() % -range) + data.procVal1b;
                  else
                        val = data.procVal1a;
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
      switch (data.procVal2) {
            case Plus:
                  val += data.procVal2a;
                  break;
            case Minus:
                  val -= data.procVal2a;
                  break;
            case Multiply:
                  val = int(val * (data.procVal2a/100.0) + .5);
                  break;
            case Divide:
                  val = int(val / (data.procVal2a/100.0) + .5);
                  break;
            case Fix:
                  val = data.procVal2a;
                  break;
            case Value:
                  val = data.procVal1a;
                  break;
            case Invert:
                  val = 127 - val;
                  break;
            case Dynamic:
                  printf("transform not implemented\n");
                  break;
            case Random:
                  {
                  int range = data.procVal2b - data.procVal2a;
                  if (range > 0)
                        val = (rand() % range) + data.procVal2a;
                  else if (range < 0)
                        val = (rand() % -range) + data.procVal2b;
                  else
                        val = data.procVal2a;
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
      //    transform channel
      //---------------------------------------------------

      val = event.channel();
      switch (data.procChannel) {
            case Plus:
                  val += data.procChannela;
                  break;
            case Minus:
                  val -= data.procChannela;
                  break;
            case Multiply:
                  val = int(val * (data.procChannela/100.0) + .5);
                  break;
            case Divide:
                  val = int(val / (data.procChannela/100.0) + .5);
                  break;
            case Fix:
                  val = data.procChannela;
                  break;
            case Value:
                  val = data.procChannela;
                  break;
            case Invert:
                  val = 16 - val;
                  break;
            case Dynamic:
                  printf("transform not implemented\n");
                  break;
            case Random:
                  {
                  int range = data.procChannelb - data.procChannela;
                  if (range > 0)
                        val = (rand() % range) + data.procChannela;
                  else if (range < 0)
                        val = (rand() % -range) + data.procChannelb;
                  else
                        val = data.procChannela;
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
//   getGeometry
//---------------------------------------------------------

void Transform::getGeometry(int* x, int* y, int* w, int* h) const
      {
      QPoint pos(gui->pos());
      QSize size(gui->size());
      *x = pos.x();
      *y = pos.y();
      *w = size.width();
      *h = size.height();
      }

//---------------------------------------------------------
//   setGeometry
//---------------------------------------------------------

void Transform::setGeometry(int x, int y, int w, int h)
      {
      gui->resize(QSize(w, h));
      gui->move(QPoint(x, y));
      }

//---------------------------------------------------------
//   TransformDialog
//    Widgets:
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
//    selChannelOp  selChannelVala selChannelValb
//    procChannelOp procChannelVala procChannelValb
//---------------------------------------------------------

TransformDialog::TransformDialog(Transform* tf, QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      cmt = tf;

      for (unsigned i = 0; i < sizeof(oplist)/sizeof(*oplist); ++i)
            funcOp->addItem(oplist[i].text, i);

      connect(selEventOp,      SIGNAL(activated(int)),    SLOT(selEventOpSel(int)));
      connect(selType,         SIGNAL(activated(int)),    SLOT(selTypeSel(int)));
      connect(selVal1Op,       SIGNAL(activated(int)),    SLOT(selVal1OpSel(int)));
      connect(selVal2Op,       SIGNAL(activated(int)),    SLOT(selVal2OpSel(int)));
      connect(procEventOp,     SIGNAL(activated(int)),    SLOT(procEventOpSel(int)));
      connect(procType,        SIGNAL(activated(int)),    SLOT(procEventTypeSel(int)));
      connect(procVal1Op,      SIGNAL(activated(int)),    SLOT(procVal1OpSel(int)));
      connect(procVal2Op,      SIGNAL(activated(int)),    SLOT(procVal2OpSel(int)));
      connect(funcOp,          SIGNAL(activated(int)),    SLOT(funcOpSel(int)));
      connect(selVal1a,        SIGNAL(valueChanged(int)), SLOT(selVal1aChanged(int)));
      connect(selVal1b,        SIGNAL(valueChanged(int)), SLOT(selVal1bChanged(int)));
      connect(selVal2a,        SIGNAL(valueChanged(int)), SLOT(selVal2aChanged(int)));
      connect(selVal2b,        SIGNAL(valueChanged(int)), SLOT(selVal2bChanged(int)));
      connect(procVal1a,       SIGNAL(valueChanged(int)), SLOT(procVal1aChanged(int)));
      connect(procVal1b,       SIGNAL(valueChanged(int)), SLOT(procVal1bChanged(int)));
      connect(procVal2a,       SIGNAL(valueChanged(int)), SLOT(procVal2aChanged(int)));
      connect(procVal2b,       SIGNAL(valueChanged(int)), SLOT(procVal2bChanged(int)));
      connect(selChannelOp,    SIGNAL(activated(int)),    SLOT(selChannelOpSel(int)));
      connect(selChannelVala,  SIGNAL(valueChanged(int)), SLOT(selChannelValaChanged(int)));
      connect(selChannelValb,  SIGNAL(valueChanged(int)), SLOT(selChannelValbChanged(int)));
      connect(procChannelOp,   SIGNAL(activated(int)),    SLOT(procChannelOpSel(int)));
      connect(procChannelVala, SIGNAL(valueChanged(int)), SLOT(procChannelValaChanged(int)));
      connect(procChannelValb, SIGNAL(valueChanged(int)), SLOT(procChannelValbChanged(int)));
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void TransformDialog::init()
      {
      selEventOp->setCurrentIndex(cmt->data.selEventOp);
      selEventOpSel(cmt->data.selEventOp);

      for (unsigned i = 0; i < sizeof(eventTypeTable)/sizeof(*eventTypeTable); ++i) {
            if (eventTypeTable[i] == cmt->data.selType) {
                  selType->setCurrentIndex(i);
                  break;
                  }
            }

      selVal1Op->setCurrentIndex(cmt->data.selVal1);
      selVal1OpSel(cmt->data.selVal1);

      selVal2Op->setCurrentIndex(cmt->data.selVal2);
      selVal2OpSel(cmt->data.selVal2);

      selChannelOp->setCurrentIndex(cmt->data.selChannel);
      selChannelOpSel(cmt->data.selChannel);

      {
      unsigned i;
      for (i = 0; i < sizeof(oplist)/sizeof(*oplist); ++i) {
            if (oplist[i].id == cmt->data.funcOp) {
                  funcOp->setCurrentIndex(i);
                  break;
                  }
            }
      if (i == sizeof(oplist)/sizeof(*oplist))
            printf("internal error: bad OpCode\n");
      funcOpSel(i);
      }

      procEventOp->setCurrentIndex(cmt->data.procEvent);
      procEventOpSel(cmt->data.procEvent);

      procVal1Op->setCurrentIndex(cmt->data.procVal1);
      procVal1OpSel(cmt->data.procVal1);

      for (unsigned i = 0; i < sizeof(procVal2Map)/sizeof(*procVal2Map); ++i) {
            if (procVal2Map[i] == cmt->data.procVal2) {
                  procVal2Op->setCurrentIndex(i);
                  break;
                  }
            }

      selVal1a->setValue(cmt->data.selVal1a);
      selVal1b->setValue(cmt->data.selVal1b);
      selVal1aChanged(cmt->data.selVal1a);
      selVal1bChanged(cmt->data.selVal1b);

      selVal2a->setValue(cmt->data.selVal2a);
      selVal2b->setValue(cmt->data.selVal2b);

      selChannelVala->setValue(cmt->data.selChannela);
      selChannelValb->setValue(cmt->data.selChannelb);

      procVal1a->setValue(cmt->data.procVal1a);
      procVal1b->setValue(cmt->data.procVal1b);

      procVal2a->setValue(cmt->data.procVal2a);
      procVal2b->setValue(cmt->data.procVal2b);

      procChannelVala->setValue(cmt->data.procChannela);
      procChannelValb->setValue(cmt->data.procChannelb);
      }

//---------------------------------------------------------
//   setValOp
//---------------------------------------------------------

void TransformDialog::setValOp(QWidget* a, QWidget* b, ValOp op)
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

void TransformDialog::selEventOpSel(int val)
      {
      selType->setEnabled(val != All);
      cmt->data.selEventOp = ValOp(val);
      selVal1aChanged(cmt->data.selVal1a);
      selVal1bChanged(cmt->data.selVal1b);
      }

//---------------------------------------------------------
//   selTypeSel
//---------------------------------------------------------

void TransformDialog::selTypeSel(int val)
      {
      cmt->data.selType = eventTypeTable[val];
      selVal1aChanged(cmt->data.selVal1a);
      selVal1bChanged(cmt->data.selVal1b);
      }

//---------------------------------------------------------
//   selVal1OpSel
//---------------------------------------------------------

void TransformDialog::selVal1OpSel(int val)
      {
      setValOp(selVal1a, selVal1b, ValOp(val));
      cmt->data.selVal1 = ValOp(val);
      }

//---------------------------------------------------------
//   selVal2OpSel
//---------------------------------------------------------

void TransformDialog::selVal2OpSel(int val)
      {
      setValOp(selVal2a, selVal2b, ValOp(val));
      cmt->data.selVal2 = ValOp(val);
      }

//---------------------------------------------------------
//   procEventOpSel
//---------------------------------------------------------

void TransformDialog::procEventOpSel(int val)
      {
      TransformOperator op = val == 0 ? Keep : Fix;
      procType->setEnabled(op == Fix);
      cmt->data.procEvent = op;
      }

//---------------------------------------------------------
//   procEventTypeSel
//---------------------------------------------------------

void TransformDialog::procEventTypeSel(int val)
      {
      cmt->data.eventType = eventTypeTable[val];
      }

//---------------------------------------------------------
//   procVal1OpSel
//---------------------------------------------------------

void TransformDialog::procVal1OpSel(int val)
      {
      cmt->data.procVal1 = TransformOperator(val);
      switch(TransformOperator(val)) {
            case Keep:
            case Invert:
                  procVal1a->setEnabled(false);
                  procVal1b->setEnabled(false);
                  break;
            case Multiply:
            case Divide:
                  procVal1a->setEnabled(true);
                  procVal1b->setEnabled(false);
                  break;
            case Plus:
            case Minus:
            case Fix:
            case Value:
            case Flip:
                  procVal1a->setEnabled(true);
                  procVal1b->setEnabled(false);
                  break;
            case Random:
            case ScaleMap:
            case Dynamic:
                  procVal1a->setEnabled(true);
                  procVal1b->setEnabled(true);
                  break;
            }
      }

//---------------------------------------------------------
//   procVal2OpSel
//---------------------------------------------------------

void TransformDialog::procVal2OpSel(int val)
      {
      TransformOperator op = TransformOperator(procVal2Map[val]);
      cmt->data.procVal2 = op;

      switch (op) {
            case Keep:
            case Invert:
                  procVal2a->setEnabled(false);
                  procVal2b->setEnabled(false);
                  break;
            case Multiply:
            case Divide:
                  procVal2a->setEnabled(true);
                  procVal2b->setEnabled(false);
                  break;
            case Plus:
            case Minus:
            case Fix:
            case Value:
                  procVal2a->setEnabled(true);
                  procVal2b->setEnabled(false);
                  break;
            case Random:
            case Dynamic:
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

void TransformDialog::funcOpSel(int val)
      {
      TransformFunction op = oplist[val].id;

      bool isFuncOp(op == Trans);

      procEventOp->setEnabled(isFuncOp);
      procType->setEnabled(isFuncOp);
      procVal1Op->setEnabled(isFuncOp);
      procVal1a->setEnabled(isFuncOp);
      procVal1b->setEnabled(isFuncOp);
      procVal2Op->setEnabled(isFuncOp);
      procVal2a->setEnabled(isFuncOp);
      procVal2b->setEnabled(isFuncOp);
      procChannelOp->setEnabled(isFuncOp);
      procChannelVala->setEnabled(isFuncOp);
      procChannelValb->setEnabled(isFuncOp);
      if (isFuncOp) {
            procEventOpSel(cmt->data.procEvent);
            procVal1OpSel(cmt->data.procVal1);
            procVal2OpSel(cmt->data.procVal2);
            procChannelOpSel(cmt->data.procChannel);
            }
      cmt->data.funcOp = op;
      }

//---------------------------------------------------------
//   selVal1aChanged
//---------------------------------------------------------

void TransformDialog::selVal1aChanged(int val)
      {
      cmt->data.selVal1a = val;
      if ((cmt->data.selEventOp != All)
         && (cmt->data.selType == 0x90)) {
            selVal1a->setSuffix(" - " + pitch2string(val));
            }
      else
            selVal1a->setSuffix(QString(""));
      }

//---------------------------------------------------------
//   selVal1bChanged
//---------------------------------------------------------

void TransformDialog::selVal1bChanged(int val)
      {
      cmt->data.selVal1b = val;
      if ((cmt->data.selEventOp != All)
         && (cmt->data.selType == 0x90)) {
            selVal1b->setSuffix(" - " + pitch2string(val));
            }
      else
            selVal1b->setSuffix(QString(""));
      }

//---------------------------------------------------------
//   selVal2aChanged
//---------------------------------------------------------

void TransformDialog::selVal2aChanged(int val)
      {
      cmt->data.selVal2a = val;
      }

//---------------------------------------------------------
//   selVal2bChanged
//---------------------------------------------------------

void TransformDialog::selVal2bChanged(int val)
      {
      cmt->data.selVal2b = val;
      }

//---------------------------------------------------------
//   procVal1aChanged
//---------------------------------------------------------

void TransformDialog::procVal1aChanged(int val)
      {
      cmt->data.procVal1a = val;
      }

//---------------------------------------------------------
//   procVal1bChanged
//---------------------------------------------------------

void TransformDialog::procVal1bChanged(int val)
      {
      cmt->data.procVal1b = val;
      }

//---------------------------------------------------------
//   procVal2aChanged
//---------------------------------------------------------

void TransformDialog::procVal2aChanged(int val)
      {
      cmt->data.procVal2a = val;
      }

//---------------------------------------------------------
//   procVal2bChanged
//---------------------------------------------------------

void TransformDialog::procVal2bChanged(int val)
      {
      cmt->data.procVal2b = val;
      }

//---------------------------------------------------------
//   selChannelOpSel
//---------------------------------------------------------

void TransformDialog::selChannelOpSel(int val)
      {
      setValOp(selChannelVala, selChannelValb, ValOp(val));
      cmt->data.selChannel = ValOp(val);
      }

//---------------------------------------------------------
//   selChannelValaChanged
//---------------------------------------------------------

void TransformDialog::selChannelValaChanged(int val)
      {
      cmt->data.selChannela = val;
      }

//---------------------------------------------------------
//   selChannelValbChanged
//---------------------------------------------------------

void TransformDialog::selChannelValbChanged(int val)
      {
      cmt->data.selChannelb = val;
      }

//---------------------------------------------------------
//   procChannelOpSel
//---------------------------------------------------------

void TransformDialog::procChannelOpSel(int val)
      {
      cmt->data.procChannel = TransformOperator(val);
      switch(TransformOperator(val)) {
            case Keep:
            case Invert:
                  procChannelVala->setEnabled(false);
                  procChannelValb->setEnabled(false);
                  break;
            case Multiply:
            case Divide:
                  procChannelVala->setEnabled(true);
                  procChannelValb->setEnabled(false);
                  break;
            case Plus:
            case Minus:
            case Fix:
            case Value:
            case Flip:
                  procChannelVala->setEnabled(true);
                  procChannelValb->setEnabled(false);
                  break;
            case Random:
            case ScaleMap:
            case Dynamic:
                  procChannelVala->setEnabled(true);
                  procChannelValb->setEnabled(true);
                  break;
            }
      }

//---------------------------------------------------------
//   procChannelValaChanged
//---------------------------------------------------------

void TransformDialog::procChannelValaChanged(int val)
      {
      cmt->data.procChannela = val;
      }

//---------------------------------------------------------
//   procChannelValbChanged
//---------------------------------------------------------

void TransformDialog::procChannelValbChanged(int val)
      {
      cmt->data.procChannelb = val;
      }

//---------------------------------------------------------
//   getInitData
//---------------------------------------------------------

void Transform::getInitData(int* n, const unsigned char** p) const
      {
      *n = sizeof(data);
      *p = (unsigned char*)&data;
      }

//---------------------------------------------------------
//   setInitData
//---------------------------------------------------------

void Transform::setInitData(int n, const unsigned char* p)
      {
      memcpy((void*)&data, p, n);
      if (gui)
            gui->init();
      }

//---------------------------------------------------------
//   inst
//---------------------------------------------------------

static Mempi* instantiate(const char* name, const MempiHost* h)
      {
      return new Transform(name, h);
      }

extern "C" {
      static MEMPI descriptor = {
            "Transformator",
            "MusE Midi Event Transformator",
            "0.1",      // version string
            MEMPI_FILTER,
            MEMPI_MAJOR_VERSION, MEMPI_MINOR_VERSION,
            instantiate
            };

      const MEMPI* mempi_descriptor() { return &descriptor; }
      }

