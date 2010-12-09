//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: combobox.cpp,v 1.4 2004/05/06 15:08:07 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <QCursor>
#include <QMenu>
#include <QMouseEvent>
#include <QFrame>
#include <QLabel>

#include "combobox.h"

//---------------------------------------------------------
//   ComboBox
//---------------------------------------------------------

ComboBox::ComboBox(QWidget* parent, const char* name)
   : QLabel(parent)
      {
      setObjectName(name);
      _currentItem = 0;
      _id = -1;
      list = new QMenu(0);
      connect(list, SIGNAL(triggered(QAction*)), SLOT(activatedIntern(QAction*)));
      setFrameStyle(QFrame::Panel | QFrame::Raised);
      setLineWidth(2);
      }

ComboBox::~ComboBox()
      {
      delete list;
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void ComboBox::mousePressEvent(QMouseEvent*)
      {
      list->exec(QCursor::pos());
      }

//---------------------------------------------------------
//   activated
//---------------------------------------------------------

void ComboBox::activatedIntern(QAction* act)
      {
      _currentItem = act->data().toInt();
      emit activated(_currentItem, _id);
      setText(act->text());
      }

//---------------------------------------------------------
//   setCurrentItem
//---------------------------------------------------------

void ComboBox::setCurrentItem(int i)
      {
      _currentItem = i;
      // ORCAN - CHECK
      QList<QAction *> actions = list->actions();
      for (QList<QAction *>::iterator it = actions.begin(); it != actions.end(); ++it) {
            QAction* act = *it;
            if (act->data().toInt() == i) {
                  setText(act->text());
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   insertItem
//---------------------------------------------------------

void ComboBox::insertItem(const QString& s, int id)
      {
	QAction *act = list->addAction(s);
	act->setData(id);
      }

