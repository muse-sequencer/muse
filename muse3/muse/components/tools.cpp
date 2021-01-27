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
#include <QAction>
#include <QPixmap>
#include <QWidget>
#include <QIcon>

#include "icons.h"
#include "shortcuts.h"
#include "action.h"
#include "globals.h"
#include "app.h"

namespace MusEGui {

static const char* infoPointer = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Pointer tool:\n"
                                                                           "With the pointer tool you can:\n"
                                                                           "  select parts\n"
                                                                           "  move parts\n"
                                                                           "  copy parts");
static const char* infoPencil = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Pencil tool:\n"
                                                                          "With the pencil tool you can:\n"
                                                                          "  create new parts\n"
                                                                          "  modify length of parts");
static const char* infoDel = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Delete tool:\n"
                                                                       "With the delete tool you can delete parts");
static const char* infoCut = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Cut tool:\n"
                                                                       "With the cut tool you can split a part");
static const char* infoGlue = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Glue tool:\n"
                                                                        "With the glue tool you can glue two parts");
static const char* infoDraw = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Drawing tool");
static const char* infoMute = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Muting tool:\n"
                                                                        "Click on part to mute/unmute");
static const char* infoAutomation = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Manipulate automation");
static const char* infoCursor = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Cursor (tracker mode) tool:\n"
                                                                          "With the cursor tool you can:\n"
                                                                          "  navigate with arrow keys\n"
                                                                          "  use VBNM to place notes\n"
                                                                          "  change step with 0 and 9");
static const char* infoRange = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Range tool");
static const char* infoPan = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Panning tool");
static const char* infoZoom = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Zoom tool");
static const char* infoStretch = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Stretch tool");
static const char* infoSamplerate = QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Sample rate tool");

QVector<ToolB> toolList = {
    {&pointerIconSVG, QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Pointer"),        infoPointer },
    {&pencilIconSVG,  QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Pencil"),         infoPencil  },
    {&deleteIconSVG,  QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Eraser"),         infoDel     },
    {&cutterIconSVG,  QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Cutter"),         infoCut     },
    {&glueIconSVG,    QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Glue"),           infoGlue    },
    {&cursorIconSVG,  QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Range"),          infoRange   },
    {&handIconSVG,    QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Pan"),            infoPan     },
    {&zoomIconSVG,    QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Zoom"),           infoZoom    },
    {&drawIconSVG,    QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Draw"),           infoDraw    },
    {&mutePartsIconSVG, QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Mute parts"),   infoMute    },
    {&drawIconSVG,    QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Edit automation"),infoAutomation},
    {&cursorIconSVG,  QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Cursor"),         infoCursor},
    {&audioStretchIconSVG,  QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Stretch"),  infoStretch},
    {&audioResampleIconSVG, QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Sample rate"), infoSamplerate}
};

QMap<int,int> toolShortcuts;


//---------------------------------------------------------
//   EditToolBar
//---------------------------------------------------------

EditToolBar::EditToolBar(QWidget* parent, int tools, const char*)
    : QToolBar(tr("Edit Tools"), parent)
{
    setObjectName("Edit Tools");
    QActionGroup* actionGroup = new QActionGroup(parent);  // Parent needed.
    actionGroup->setExclusive(true);

    initShortcuts();

    bool first = true;
    int n = 0;
    for (unsigned i = 0; i < static_cast<unsigned>(toolList.size()); ++i) {
        if ((tools & (1 << i))==0)
            continue;
        ToolB* t = &toolList[i];

        Action* a = new Action(actionGroup, 1 << i, tr(t->tip).toLatin1().data(), true);
        actions.append(a);
        a->setIcon(**(t->icon));
        a->setShortcut(shortcuts[toolShortcuts[1 << i]].key);
        a->setToolTip(tr(t->tip) + " (" + a->shortcut().toString() + ")");
        a->setWhatsThis(tr(t->ltip));
        if (first) {
            a->setChecked(true);
            first = false;
        }
        ++n;
    }
    actionGroup->setVisible(true);
    //action->addTo(this);
    // Note: Does not take ownership.
    addActions(actionGroup->actions());

    connect(actionGroup, SIGNAL(triggered(QAction*)), SLOT(toolChanged(QAction*)));
    // TODO kybos (to be checked, setShortcut is apparently causing crashes...)
    connect(MusEGlobal::muse, &MusE::configChanged, this, &EditToolBar::configChanged);
}

void EditToolBar::initShortcuts() {
    toolShortcuts[PointerTool] = SHRT_TOOL_POINTER;
    toolShortcuts[PencilTool]  = SHRT_TOOL_PENCIL;
    toolShortcuts[RubberTool]  = SHRT_TOOL_RUBBER;
    toolShortcuts[CutTool]     = SHRT_TOOL_SCISSORS;
    toolShortcuts[GlueTool]    = SHRT_TOOL_GLUE;
    toolShortcuts[RangeTool]   = SHRT_TOOL_RANGE;
    toolShortcuts[PanTool]     = SHRT_TOOL_PAN;
    toolShortcuts[ZoomTool]    = SHRT_TOOL_ZOOM;
    toolShortcuts[DrawTool]    = SHRT_TOOL_LINEDRAW;
    toolShortcuts[MuteTool]    = SHRT_TOOL_MUTE;
    toolShortcuts[AutomationTool] = SHRT_TOOL_LINEDRAW;
    toolShortcuts[CursorTool]  = SHRT_TOOL_CURSOR;
    toolShortcuts[StretchTool]  = SHRT_TOOL_STRETCH;
    toolShortcuts[SamplerateTool]  = SHRT_TOOL_SAMPLERATE;
}

void EditToolBar::configChanged() {

    for (const auto& a : actions) {
        if (MusEGui::toolShortcuts.contains(a->id())) {
            a->setShortcut(shortcuts[toolShortcuts[a->id()]].key);
            int idx = a->toolTip().lastIndexOf('(');
            if (idx != -1)
                a->setToolTip(a->toolTip().left(idx + 1) + a->shortcut().toString() + ")");
        }
        else
            printf("Error: EditToolBar configChanged: Tool ID doesn't exist: %d\n", a->id());
    }
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
}

//---------------------------------------------------------
//   set
//---------------------------------------------------------

void EditToolBar::set(int id)
{
    for (const auto& action : actions) {
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
    for (const auto& action : actions) {
        if (action->isChecked())
            return action->id();
    }
    return -1;
}

} // namespace MusEGui
