//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tools.cpp,v 1.2 2004/04/28 21:56:13 spamatica Exp $
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
#include "tools.h"

#include <QActionGroup>

#include "icons.h"
#include "action.h"
#include "shortcuts.h"

namespace MusEGui {

const char* infoPointer = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Pointer Tool:\n"
      "with the pointer tool you can:\n"
      "  select parts\n"
      "  move parts\n"
      "  copy parts");
const char* infoPencil = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Pencil Tool:\n"
      "with the pencil tool you can:\n"
      "  create new parts\n"
      "  modify length of parts");
const char* infoDel = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Delete Tool:\n"
      "with the delete tool you can delete parts");
const char* infoCut = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Cut Tool:\n"
      "with the cut tool you can split a part");
const char* infoGlue = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Glue Tool:\n"
      "with the glue tool you can glue two parts");
const char* infoScore = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Score Tool:\n");
const char* infoQuant = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Quantize Tool:\n"
      "insert display quantize event");
const char* infoDraw = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Drawing Tool");
const char* infoMute = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Muting Tool:\n"
      "click on part to mute/unmute");
const char* infoAutomation = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Manipulate automation");
const char* infoCursor = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Cursor (tracker mode) tool:\n"
      "with the cursor tool you can:\n"
      "  navigate with arrow keys\n"
      "  use VBNM to place notes\n"
      "  change step with 0 and 9");
const char* infoRange = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Range Tool");
const char* infoPan = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Panning Tool");
const char* infoZoom = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Zoom Tool");
// REMOVE Tim. samplerate. Added.
const char* infoStretch = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Stretch Tool");
const char* infoSamplerate = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "select Sample Rate Tool");

ToolB toolList[] = {
      {&pointerIcon,  QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "pointer"),        infoPointer },
      {&pencilIcon,   QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "pencil"),         infoPencil  },
      {&deleteIcon,   QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "eraser"),         infoDel     },
      {&cutIcon,      QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "cutter"),         infoCut     },
      {&glueIcon,     QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "glue"),           infoGlue    },
      {&cursorIcon,   QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "range"),          infoRange   },
      {&openHandIcon, QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "pan"),            infoPan     },
      {&zoomIcon,     QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "zoom"),           infoZoom    },
      {&note1Icon,    QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "score"),          infoScore   },
      {&quantIcon,    QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "quantize"),       infoQuant   },
      {&drawIcon,     QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "draw"),           infoDraw    },
      {&editmuteIcon, QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "mute parts"),     infoMute    },
      {&drawIcon,     QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "edit automation"),infoAutomation},
      {&cursorIcon,   QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "cursor"),         infoCursor},
// REMOVE Tim. samplerate. Added.
      {&cursorIcon,   QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "stretch"),        infoStretch},
      {&cursorIcon,   QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "sample rate"),    infoSamplerate}
      };

QMap<int,int> toolShortcuts;


const unsigned gNumberOfTools = sizeof(toolList) / sizeof(ToolB);

//---------------------------------------------------------
//   EditToolBar
//---------------------------------------------------------

EditToolBar::EditToolBar(QWidget* parent, int tools, const char*)
   : QToolBar(tr("Edit Tools"), parent)
      {
      setObjectName("Edit Tools");
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

            Action* a = new Action(action, 1<<i, tr(t->tip).toLatin1().data(), true);
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
      
      connect(action, SIGNAL(triggered(QAction*)), SLOT(toolChanged(QAction*)));

      toolShortcuts[PointerTool] = SHRT_TOOL_POINTER;
      toolShortcuts[PencilTool]  = SHRT_TOOL_PENCIL;
      toolShortcuts[RubberTool]  = SHRT_TOOL_RUBBER;
      toolShortcuts[CutTool]     = SHRT_TOOL_SCISSORS;
      toolShortcuts[GlueTool]    = SHRT_TOOL_GLUE;
      toolShortcuts[RangeTool]   = SHRT_TOOL_RANGE;
      toolShortcuts[PanTool]     = SHRT_TOOL_PAN;
      toolShortcuts[ZoomTool]    = SHRT_TOOL_ZOOM;
      //toolShortcuts[ScoreTool]   = SHRT_TOOL_
      //toolShortcuts[QuantTool]    = SHRT_TOOL_
      toolShortcuts[DrawTool]    = SHRT_TOOL_LINEDRAW;
      toolShortcuts[MuteTool]    = SHRT_TOOL_MUTE;
      toolShortcuts[AutomationTool] = SHRT_TOOL_LINEDRAW;
      toolShortcuts[CursorTool]  = SHRT_TOOL_CURSOR;
      // REMOVE Tim. samplerate. Added.
      toolShortcuts[StretchTool]  = SHRT_TOOL_STRETCH;
      toolShortcuts[SamplerateTool]  = SHRT_TOOL_SAMPLERATE;
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
      delete [] actions;
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

} // namespace MusEGui
