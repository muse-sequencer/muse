//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tools.cpp,v 1.2 2004/04/28 21:56:13 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include "tools.h"
#include <qpixmap.h>
#include <q3buttongroup.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <q3whatsthis.h>
#include <q3mainwindow.h>
//Added by qt3to4:
#include <Q3ActionGroup>

#include "icons.h"
#include "action.h"

const char* infoPointer = QT_TR_NOOP("select Pointer Tool:\n"
      "with the pointer tool you can:\n"
      "  select parts\n"
      "  move parts\n"
      "  copy parts");
const char* infoPencil = QT_TR_NOOP("select Pencil Tool:\n"
      "with the pencil tool you can:\n"
      "  create new parts\n"
      "  modify length of parts");
const char* infoDel = QT_TR_NOOP("select Delete Tool:\n"
      "with the delete tool you can delete parts");
const char* infoCut = QT_TR_NOOP("select Cut Tool:\n"
      "with the cut tool you can split a part");
const char* infoGlue = QT_TR_NOOP("select Glue Tool:\n"
      "with the glue tool you can glue two parts");
const char* infoScore = QT_TR_NOOP("select Score Tool:\n");
const char* infoQuant = QT_TR_NOOP("select Quantize Tool:\n"
      "insert display quantize event");
const char* infoDraw = QT_TR_NOOP("select Drawing Tool");
const char* infoMute = QT_TR_NOOP("select Muting Tool:\n"
      "click on part to mute/unmute");

ToolB toolList[] = {
      {&pointerIcon,  QT_TR_NOOP("pointer"),     infoPointer },
      {&pencilIcon,   QT_TR_NOOP("pencil"),      infoPencil  },
      {&deleteIcon,   QT_TR_NOOP("eraser"),      infoDel     },
      {&cutIcon,      QT_TR_NOOP("cutter"),      infoCut     },
      {&note1Icon,    QT_TR_NOOP("score"),       infoScore   },
      {&glueIcon,     QT_TR_NOOP("glue"),        infoGlue    },
      {&quantIcon,    QT_TR_NOOP("quantize"),    infoQuant   },
      {&drawIcon,     QT_TR_NOOP("draw"),        infoDraw    },
      {&editmuteIcon, QT_TR_NOOP("mute parts"),  infoMute    },
      };

//---------------------------------------------------------
//   EditToolBar
//---------------------------------------------------------

EditToolBar::EditToolBar(Q3MainWindow* parent, int tools, const char*)
   : Q3ToolBar(tr("Edit Tools"), parent)
      {
      Q3ActionGroup* action = new Q3ActionGroup(parent, "editaction", true);

      nactions = 0;
      for (unsigned i = 0; i < sizeof(toolList)/sizeof(*toolList); ++i) {
            if ((tools & (1 << i))==0)
                  continue;
            ++nactions;
            }
      actions = new Action*[nactions];
      bool first = true;
      int n = 0;
      for (unsigned i = 0; i < sizeof(toolList)/sizeof(*toolList); ++i) {
            if ((tools & (1 << i))==0)
                  continue;
            ToolB* t = &toolList[i];

            Action* a = new Action(action, 1<<i, t->tip, true);
            actions[n] = a;
            a->setIconSet(QIcon(**(t->icon)));
            a->setToolTip(tr(t->tip));
            a->setWhatsThis(tr(t->ltip));
            if (first) {
                  a->setOn(true);
                  first = false;
                  }
            ++n;
            }
      action->addTo(this);
      //connect(action, SIGNAL(selected(Q3Action*)), SLOT(toolChanged(QAction*)));
      connect(action, SIGNAL(selected(Q3Action*)), SLOT(toolChanged(Q3Action*)));   // p4.0.5
      }

//---------------------------------------------------------
//   toolChanged
//---------------------------------------------------------

void EditToolBar::toolChanged(Q3Action* action)
      {
      emit toolChanged(((Action*)action)->id());
      }

//---------------------------------------------------------
//   ~EditToolBar
//---------------------------------------------------------

EditToolBar::~EditToolBar()
      {
      delete actions;
      }

//---------------------------------------------------------
//   set
//---------------------------------------------------------

void EditToolBar::set(int id)
      {
      for (int i = 0; i < nactions; ++i) {
            Action* action = actions[i];
            if (action->id() == id) {
                  action->setOn(true);
                  return;
                  }
            }
      }

//---------------------------------------------------------
//   curTool
//---------------------------------------------------------

int EditToolBar::curTool()
      {
      for (int i = 0; i < nactions; ++i) {
            Action* action = actions[i];
            if (action->isOn())
                  return action->id();
            }
      return -1;
      }

