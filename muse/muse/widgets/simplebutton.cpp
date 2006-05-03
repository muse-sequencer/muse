//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: simplebutton.cpp,v 1.12 2006/01/31 20:33:22 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "simplebutton.h"

//---------------------------------------------------------
//   SimpleButton
//---------------------------------------------------------

SimpleButton::SimpleButton(QPixmap* on, QPixmap* off, QWidget* parent)
   : QToolButton(parent)
      {
//      setStyle(new QWindowsStyle());
      setAutoRaise(true);
      QIcon icon(*off);
      icon.addPixmap(*on, QIcon::Normal, QIcon::On);
      QAction* a = new QAction(this);
      a->setIcon(icon);
      setDefaultAction(a);
      }

//---------------------------------------------------------
//   SimpleButton
//---------------------------------------------------------

SimpleButton::SimpleButton(const QString& s, QWidget* parent)
   : QToolButton(parent)
      {
      setAutoRaise(false);
      setText(s);
      }

