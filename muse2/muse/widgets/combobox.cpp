//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: combobox.cpp,v 1.4 2004/05/06 15:08:07 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <QMenu>
#include <QSignalMapper>
#include <QWheelEvent>

#include "combobox.h"

//---------------------------------------------------------
//   ComboBox
//---------------------------------------------------------

ComboBox::ComboBox(QWidget* parent, const char* name)
   : QToolButton(parent)
      {
      setObjectName(name);
      _currentItem = 0;

      menu = new QMenu(this);

      autoTypeSignalMapper = new QSignalMapper(this);
      connect(autoTypeSignalMapper, SIGNAL(mapped(int)), this, SLOT(activatedIntern(int)));
      }

ComboBox::~ComboBox()
      {
      delete menu;
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
      int i = itemlist.indexOf(_currentItem);
      int len = itemlist.count();
      if (ev->delta() > 0 && i > 0)
            activatedIntern(_currentItem-1);
      else if (ev->delta() < 0 && -1 < i && i < len - 1)
            activatedIntern(_currentItem+1);
      }

//---------------------------------------------------------
//   activated
//---------------------------------------------------------

void ComboBox::activatedIntern(int id)
      {
      setCurrentItem(id);
      emit activated(id);
      }

//---------------------------------------------------------
//   setCurrentItem
//---------------------------------------------------------

void ComboBox::setCurrentItem(int id)
      {
      QAction* act = (QAction*) autoTypeSignalMapper->mapping(id);
      _currentItem = id;
      setText(act->text());
      }

//---------------------------------------------------------
//   insertItem
//---------------------------------------------------------

void ComboBox::addAction(const QString& s, int id)
      {
      QAction *act = menu->addAction(s);
      connect(act, SIGNAL(triggered()), autoTypeSignalMapper, SLOT(map()));
      autoTypeSignalMapper->setMapping(act, id);
      itemlist << id;
      }

