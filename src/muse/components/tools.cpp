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
#include "globals.h"
#include "app.h"

namespace MusEGui {

//---------------------------------------------------------
//   EditToolBar
//---------------------------------------------------------

// Icon / tooltip / whatsthis
const QVector<ToolB> EditToolBar::toolList = {
    {&pointerIconSVG, QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Pointer"),
        QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Pointer tool:\n"
                                                  "With the pointer tool you can:\n"
                                                  "  select parts\n"
                                                  "  move parts\n"
                                                  "  copy parts")
    },
    {&pencilIconSVG,  QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Pencil"),
        QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Pencil tool:\n"
                                                  "With the pencil tool you can:\n"
                                                  "  create new parts\n"
                                                  "  modify length of parts")
    },
    {&deleteIconSVG,  QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Eraser"),
        QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Delete tool:\n"
                                                  "With the delete tool you can delete parts")
    },
    {&cutterIconSVG,  QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Cutter"),
        QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Cut tool:\n"
                                                  "With the cut tool you can split a part")
    },
    {&glueIconSVG,    QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Glue"),
        QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Glue tool:\n"
                                                  "With the glue tool you can glue two parts")},
    {&cursorIconSVG,  QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Range"),
        QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Range tool")
    },
    {&handIconSVG,    QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Pan"),
        QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Panning tool")
    },
    {&zoomIconSVG,    QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Zoom"),
        QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Zoom tool")
    },
    {&drawIconSVG,    QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Draw"),
        QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Drawing tool")
    },
    {&mutePartsIconSVG, QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Mute Parts"),
        QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Muting tool:\n"
                                                  "Click on part to mute/unmute")
    },
    {&drawIconSVG,    QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Edit Automation"),
        QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Manipulate automation")
    },
    {&cursorIconSVG,  QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Cursor"),
        QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Cursor (tracker mode) tool:\n"
                                               "With the cursor tool you can:\n"
                                               "  navigate with arrow keys\n"
                                               "  use VBNM to place notes\n"
                                               "  change step with 0 and 9")
    },
    {&audioStretchIconSVG,  QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Stretch"),
        QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Stretch tool")
    },
    {&audioResampleIconSVG, QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Sample Rate"),
        QT_TRANSLATE_NOOP("MusEGui::EditToolBar", "Select Sample rate tool")
    }
};

const QMap<int,int> EditToolBar::toolShortcuts = {
    {PointerTool   , SHRT_TOOL_POINTER},
    {PencilTool    , SHRT_TOOL_PENCIL},
    {RubberTool    , SHRT_TOOL_RUBBER},
    {CutTool       , SHRT_TOOL_SCISSORS},
    {GlueTool      , SHRT_TOOL_GLUE},
    {RangeTool     , SHRT_TOOL_RANGE},
    {PanTool       , SHRT_TOOL_PAN},
    {ZoomTool      , SHRT_TOOL_ZOOM},
    {DrawTool      , SHRT_TOOL_LINEDRAW},
    {MuteTool      , SHRT_TOOL_MUTE},
    {AutomationTool, SHRT_TOOL_LINEDRAW},
    {CursorTool    , SHRT_TOOL_CURSOR},
    {StretchTool   , SHRT_TOOL_STRETCH},
    {SamplerateTool, SHRT_TOOL_SAMPLERATE}
};

EditToolBar::EditToolBar(QWidget* parent, int tools, const char*)
    : QToolBar(tr("Edit Tools"), parent)
{
    setObjectName("Edit Tools");
    actionGroup = new QActionGroup(parent);  // Parent needed.
    actionGroup->setExclusive(true);

    bool first = true;
    for (unsigned i = 0; i < static_cast<unsigned>(toolList.size()); ++i) {
        if ((tools & (1 << i))==0)
            continue;
        const ToolB* t = &toolList[static_cast<int>(i)];

        QAction* a = new QAction(tr(t->tip).toLatin1().data(), actionGroup);
        a->setData(1 << i);
        a->setCheckable(true);
        a->setIcon(**(t->icon));
        a->setShortcut(shortcuts[toolShortcuts[1 << i]].key);
        a->setToolTip(tr(t->tip) + " (" + a->shortcut().toString() + ")");
        a->setWhatsThis(tr(t->ltip));
        if (first) {
            a->setChecked(true);
            first = false;
        }
    }
    actionGroup->setVisible(true);
    //action->addTo(this);
    // Note: Does not take ownership.
    addActions(actionGroup->actions());

    connect(actionGroup, SIGNAL(triggered(QAction*)), SLOT(toolChanged(QAction*)));
}

void EditToolBar::configChanged() {

    for (const auto& a : actionGroup->actions()) {
        if (toolShortcuts.contains(a->data().toInt())) {
            a->setShortcut(shortcuts[toolShortcuts[a->data().toInt()]].key);
            int idx = a->toolTip().lastIndexOf('(');
            if (idx != -1)
                a->setToolTip(a->toolTip().left(idx + 1) + a->shortcut().toString() + ")");
        }
        else
            printf("Error: EditToolBar configChanged: Tool ID doesn't exist: %d\n", a->data().toInt());
    }
}

//---------------------------------------------------------
//   toolChanged
//---------------------------------------------------------

void EditToolBar::toolChanged(QAction* action)
{
    emit toolChanged(action->data().toInt());
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
    for (const auto& action : actionGroup->actions()) {
        if (action->data().toInt() == id) {
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
    for (const auto& action : actionGroup->actions()) {
        if (action->isChecked())
            return action->data().toInt();
    }
    return -1;
}

} // namespace MusEGui
