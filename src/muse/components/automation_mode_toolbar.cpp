//=========================================================
//  MusE
//  Linux Music Editor
//  automation_mode_toolbar.h
//  (C) Copyright 2022 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <QWidget>
#include <QAction>
#include <QActionGroup>

#include "automation_mode_toolbar.h"
#include "icons.h"

namespace MusEGui {

AutomationModeToolBar::AutomationModeToolBar(QWidget* parent, const char*)
    : QToolBar(tr("Audio Automation Mode"), parent)
{
    setObjectName("Audio automation mode");
    interpolateActions = new QActionGroup(parent);  // Parent needed.
    interpolateActions->setExclusive(true);

    QAction* a = new QAction(interpolateActions);
    a->setData(0);
    a->setIcon(*audioAutomationDiscreteIconSVG);
    a->setToolTip(tr("Draw Discrete Automation Points"));
    a->setCheckable(true);
    a->setChecked(true);

    a = new QAction(interpolateActions);
    a->setData(1);
    a->setIcon(*audioAutomationInterpolateIconSVG);
    a->setToolTip(tr("Draw Interpolating Automation Points"));
    a->setCheckable(true);

    interpolateActions->setVisible(true);
    // Note: Does not take ownership.
    addActions(interpolateActions->actions());

    connect(interpolateActions, &QActionGroup::triggered, [this](QAction* a) { interpolateModeChange(a); } );


    boxActions = new QActionGroup(parent);  // Parent needed.
    boxActions->setExclusive(true);

    a = new QAction(boxActions);
    a->setData(0);
    a->setIcon(*audioAutomationBoxesIconSVG);
    a->setToolTip(tr("Show Automation Point Boxes"));
    a->setCheckable(true);
    a->setChecked(true);

    a = new QAction(boxActions);
    a->setData(1);
    a->setIcon(*audioAutomationNoBoxesIconSVG);
    a->setToolTip(tr("Hide Automation Point Boxes"));
    a->setCheckable(true);

    boxActions->setVisible(true);
    // Note: Does not take ownership.
    addActions(boxActions->actions());

    connect(boxActions, &QActionGroup::triggered, [this](QAction* a) { boxModeChange(a); } );

    addSeparator();

    optimizeAction = new QAction(parent);
    optimizeAction->setData(0);
    optimizeAction->setIcon(*audioAutomationOptimizeIconSVG);
    optimizeAction->setToolTip(tr("Don't Record Redundant Straight Line Automation Points"));
    optimizeAction->setCheckable(true);
    addAction(optimizeAction);

    connect(optimizeAction, &QAction::toggled, [this](bool v) { optimizeChange(v); } );
}

//---------------------------------------------------------
//   interpolateModeChange
//---------------------------------------------------------

void AutomationModeToolBar::interpolateModeChange(QAction* action)
{
  emit interpolateModeChanged(action->data().toInt());
}

//---------------------------------------------------------
//   boxModeChange
//---------------------------------------------------------

void AutomationModeToolBar::boxModeChange(QAction* action)
{
  emit boxModeChanged(action->data().toInt());
}

//---------------------------------------------------------
//   optimizeChange
//---------------------------------------------------------

void AutomationModeToolBar::optimizeChange(bool v)
{
  emit optimizeChanged(v);
}

//---------------------------------------------------------
//   setInterpolateMode
//---------------------------------------------------------

void AutomationModeToolBar::setInterpolateMode(int id)
{
  for (const auto& action : interpolateActions->actions())
  {
    if (action->data().toInt() == id)
    {
        action->setChecked(true);
        interpolateModeChange(action);
        return;
    }
  }
}

//---------------------------------------------------------
//   setBoxMode
//---------------------------------------------------------

void AutomationModeToolBar::setBoxMode(int id)
{
  for (const auto& action : boxActions->actions())
  {
    if (action->data().toInt() == id)
    {
        action->setChecked(true);
        boxModeChange(action);
        return;
    }
  }
}

//---------------------------------------------------------
//   setBoxMode
//---------------------------------------------------------

void AutomationModeToolBar::setOptimize(bool v)
{
  if(optimizeAction->isChecked() == v)
    return;
  optimizeAction->setChecked(v);
}

//---------------------------------------------------------
//   ~AutomationModeToolBar
//---------------------------------------------------------

AutomationModeToolBar::~AutomationModeToolBar()
{
}


} // namespace MusEGui
