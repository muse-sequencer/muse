 //=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tools.cpp,v 1.8 2005/12/21 17:03:08 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include "tools.h"
#include "icons.h"

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

ToolB toolList[TOOLS] = {
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

EditToolBar::EditToolBar(QMainWindow* parent, int tools)
   : QToolBar(tr("Edit Tools"))
      {
      actionGroup = new QActionGroup(parent);
      actionGroup->setExclusive(true);

      bool first = true;
      for (unsigned i = 0; i < sizeof(toolList)/sizeof(*toolList); ++i) {
            if ((tools & (1 << i))==0)
                  continue;
            ToolB* t = &toolList[i];

            QAction* a = new QAction(QIcon(**(t->icon)), "tool", this);
            a->setData(1 << i);
            a->setCheckable(true);
            actionGroup->addAction(a);
            addAction(a);
            a->setToolTip(tr(t->tip));
            a->setWhatsThis(tr(t->ltip));
            if (first) {
                  a->setChecked(true);
                  first = false;
                  }
            }
      connect(actionGroup, SIGNAL(triggered(QAction*)), SLOT(toolChanged(QAction*)));
      }

//---------------------------------------------------------
//   toolChanged
//---------------------------------------------------------

void EditToolBar::toolChanged(QAction* action)
      {
      emit toolChanged(action->data().toInt());
      }

//---------------------------------------------------------
//   set
//---------------------------------------------------------

void EditToolBar::set(int id)
      {
      QList<QAction*> actions = actionGroup->actions();
      int n = actions.size();

      for (int i = 0; i < n; ++i) {
            QAction* action = actions.at(i);
            if (action->data().toInt() == id) {
                  action->setChecked(true);
                  return;
                  }
            }
      }

//---------------------------------------------------------
//   curTool
//---------------------------------------------------------

int EditToolBar::curTool()
      {
      QList<QAction*> actions = actionGroup->actions();
      int n = actions.size();

      for (int i = 0; i < n; ++i) {
            QAction* a = actions.at(i);
            if (a->isChecked())
                  return a->data().toInt();
            }
      return -1;
      }

