//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tools.cpp,v 1.2 2004/04/28 21:56:13 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include "tools.h"

#include <QActionGroup>

#include "icons.h"
#include "action.h"

const char* infoPointer = QT_TRANSLATE_NOOP("@default", "select Pointer Tool:\n"
      "with the pointer tool you can:\n"
      "  select parts\n"
      "  move parts\n"
      "  copy parts");
const char* infoPencil = QT_TRANSLATE_NOOP("@default", "select Pencil Tool:\n"
      "with the pencil tool you can:\n"
      "  create new parts\n"
      "  modify length of parts");
const char* infoDel = QT_TRANSLATE_NOOP("@default", "select Delete Tool:\n"
      "with the delete tool you can delete parts");
const char* infoCut = QT_TRANSLATE_NOOP("@default", "select Cut Tool:\n"
      "with the cut tool you can split a part");
const char* infoGlue = QT_TRANSLATE_NOOP("@default", "select Glue Tool:\n"
      "with the glue tool you can glue two parts");
const char* infoScore = QT_TRANSLATE_NOOP("@default", "select Score Tool:\n");
const char* infoQuant = QT_TRANSLATE_NOOP("@default", "select Quantize Tool:\n"
      "insert display quantize event");
const char* infoDraw = QT_TRANSLATE_NOOP("@default", "select Drawing Tool");
const char* infoMute = QT_TRANSLATE_NOOP("@default", "select Muting Tool:\n"
      "click on part to mute/unmute");

ToolB toolList[] = {
      {&pointerIcon,  QT_TRANSLATE_NOOP("@default", "pointer"),     infoPointer },
      {&pencilIcon,   QT_TRANSLATE_NOOP("@default", "pencil"),      infoPencil  },
      {&deleteIcon,   QT_TRANSLATE_NOOP("@default", "eraser"),      infoDel     },
      {&cutIcon,      QT_TRANSLATE_NOOP("@default", "cutter"),      infoCut     },
      {&note1Icon,    QT_TRANSLATE_NOOP("@default", "score"),       infoScore   },
      {&glueIcon,     QT_TRANSLATE_NOOP("@default", "glue"),        infoGlue    },
      {&quantIcon,    QT_TRANSLATE_NOOP("@default", "quantize"),    infoQuant   },
      {&drawIcon,     QT_TRANSLATE_NOOP("@default", "draw"),        infoDraw    },
      {&editmuteIcon, QT_TRANSLATE_NOOP("@default", "mute parts"),  infoMute    },
      };

//---------------------------------------------------------
//   EditToolBar
//---------------------------------------------------------

//EditToolBar::EditToolBar(QMainWindow* parent, int tools, const char*)
EditToolBar::EditToolBar(QWidget* parent, int tools, const char*)
   : QToolBar(tr("Edit Tools"), parent)
      {
      QActionGroup* action = new QActionGroup(parent);  // Parent needed.
      action->setExclusive(true);

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
            //a->setIconSet(QIcon(**(t->icon)));
            a->setIcon(QIcon(**(t->icon)));
            a->setToolTip(tr(t->tip));
            a->setWhatsThis(tr(t->ltip));
            if (first) {
                  a->setChecked(true);
                  first = false;
                  }
            ++n;
            }
      action->setVisible(true);
      //action->addTo(this);
      // Note: Does not take ownership.
      addActions(action->actions());
      
      connect(action, SIGNAL(selected(QAction*)), SLOT(toolChanged(QAction*)));   
      }

//---------------------------------------------------------
//   toolChanged
//---------------------------------------------------------

void EditToolBar::toolChanged(QAction* action)
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
                  action->setChecked(true);
                  toolChanged(action);
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
            if (action->isChecked())
                  return action->id();
            }
      return -1;
      }

