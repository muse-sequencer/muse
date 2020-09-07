//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: combobox.cpp,v 1.4 2004/05/06 15:08:07 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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

#include "combobox.h"

// NOTE: To cure circular dependencies these includes are at the bottom.
#include <QWidget>
#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QString>

namespace MusEGui {

//---------------------------------------------------------
//   ComboBox
//---------------------------------------------------------

ComboBox::ComboBox(QWidget* parent, const char* name)
   : QToolButton(parent)
      {
      setObjectName(name);
      _currentItem = nullptr;
      menu = new QMenu(this);
      connect(menu, &QMenu::triggered, [this](QAction* act) { activatedIntern(act); } );
      }

ComboBox::~ComboBox()
      {
      delete menu;
      }

//---------------------------------------------------------
//   findAction
//---------------------------------------------------------

QAction* ComboBox::findAction(int id) const
{
  QList<QAction*> l = menu->actions();
  foreach(QAction* act, l)
  {
    if(act && act->data().isValid() && act->data().toInt() == id)
      return act;
  }
  return nullptr;
}

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void ComboBox::mousePressEvent(QMouseEvent* /*ev*/)
      {
      menu->exec(QCursor::pos());
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void ComboBox::wheelEvent(QWheelEvent* ev)
      {
      QList<QAction*> l = menu->actions();
      const int len = l.size();
      if(len == 0)
        return;

      const int i = l.indexOf(_currentItem);
      if(i >= 0)
      {
        const QPoint pixelDelta = ev->pixelDelta();
        const QPoint angleDegrees = ev->angleDelta() / 8;
        int delta = 0;
        if(!pixelDelta.isNull())
          delta = pixelDelta.y();
        else if(!angleDegrees.isNull())
          delta = angleDegrees.y() / 15;
        else
          return;

        if (delta > 0 && i > 0)
              activatedIntern(l.at(i - 1));
        else if (delta < 0 && i < len - 1)
              activatedIntern(l.at(i + 1));
      }
      else
      {
        activatedIntern(l.at(0));
      }
      }

//---------------------------------------------------------
//   activated
//---------------------------------------------------------

void ComboBox::activatedIntern(QAction* act)
      {
      if(!act)
        return;
      setText(act->text());
      int id = -1;
      if(act->data().isValid())
        id = act->data().toInt();
      _currentItem = act;
      emit activated(id);
      emit activated(act);
      }

//---------------------------------------------------------
//   setCurrentItem
//---------------------------------------------------------

void ComboBox::setCurrentItem(int id)
      {
      QAction* act = findAction(id);
      _currentItem = act;
      if(act)
        setText(act->text());
      else
        setText(QString());
      }

//---------------------------------------------------------
//   insertItem
//---------------------------------------------------------

void ComboBox::addAction(const QString& s, int id)
      {
      QAction *act = menu->addAction(s);
      act->setData(id);
      }

      
//---------------------------------------------------------
//   CompactComboBox
//---------------------------------------------------------

CompactComboBox::CompactComboBox(QWidget* parent, const QIcon& icon, const char* name)
   : CompactToolButton(parent, icon, name)
      {
      _currentItem = nullptr;
      menu = new QMenu(this);
      connect(menu, &QMenu::triggered, [this](QAction* act) { activatedIntern(act); } );
      }

CompactComboBox::~CompactComboBox()
      {
      delete menu;
      }

//---------------------------------------------------------
//   findAction
//---------------------------------------------------------

QAction* CompactComboBox::findAction(int id) const
{
  QList<QAction*> l = menu->actions();
  foreach(QAction* act, l)
  {
    if(act && act->data().isValid() && act->data().toInt() == id)
      return act;
  }
  return nullptr;
}

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void CompactComboBox::mousePressEvent(QMouseEvent* /*ev*/)
      {
      menu->exec(QCursor::pos());
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void CompactComboBox::wheelEvent(QWheelEvent* ev)
      {
      QList<QAction*> l = menu->actions();
      const int len = l.size();
      if(len == 0)
        return;

      const int i = l.indexOf(_currentItem);
      if(i >= 0)
      {
        const QPoint pixelDelta = ev->pixelDelta();
        const QPoint angleDegrees = ev->angleDelta() / 8;
        int delta = 0;
        if(!pixelDelta.isNull())
          delta = pixelDelta.y();
        else if(!angleDegrees.isNull())
          delta = angleDegrees.y() / 15;
        else
          return;

        if (delta > 0 && i > 0)
              activatedIntern(l.at(i - 1));
        else if (delta < 0 && i < len - 1)
              activatedIntern(l.at(i + 1));
      }
      else
      {
        activatedIntern(l.at(0));
      }
      }

//---------------------------------------------------------
//   activated
//---------------------------------------------------------

void CompactComboBox::activatedIntern(QAction* act)
      {
      if(!act)
        return;
      setText(act->text());
      int id = -1;
      if(act->data().isValid())
        id = act->data().toInt();
      _currentItem = act;
      emit activated(id);
      emit activated(act);
      }

//---------------------------------------------------------
//   setCurrentItem
//---------------------------------------------------------

void CompactComboBox::setCurrentItem(int id)
      {
      QAction* act = findAction(id);
      _currentItem = act;
      if(act)
        setText(act->text());
      else
        setText(QString());
      }

//---------------------------------------------------------
//   insertItem
//---------------------------------------------------------

void CompactComboBox::addAction(const QString& s, int id)
      {
      QAction *act = menu->addAction(s);
      act->setData(id);
      }
      
} // namespace MusEGui
