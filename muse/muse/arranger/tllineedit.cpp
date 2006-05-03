//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: tllineedit.cpp,v 1.8 2006/01/12 14:49:13 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "tllineedit.h"

//---------------------------------------------------------
//   TLLineEdit
//---------------------------------------------------------

TLLineEdit::TLLineEdit(const QString& contents, QWidget* parent)
   : QLineEdit(contents, parent)
      {
      setReadOnly(true);
      setFrame(false);
      setAlignment(Qt::AlignLeft);
      setCursorPosition(0);
      connect(this, SIGNAL(editingFinished()), SLOT(contentHasChanged()));
      }

//---------------------------------------------------------
//   contentHasChanged
//---------------------------------------------------------

void TLLineEdit::contentHasChanged()
      {
      setReadOnly(true);
      setFrame(false);
      if (isModified())
            emit contentChanged(text());
      setModified(false);
      }

//---------------------------------------------------------
//   mouseDoubleClickEvent
//---------------------------------------------------------

void TLLineEdit::mouseDoubleClickEvent(QMouseEvent*)
      {
      setReadOnly(false);
      setFocus();
      setFrame(true);
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void TLLineEdit::mousePressEvent(QMouseEvent* ev)
      {
      QLineEdit::mousePressEvent(ev);
      emit mousePress();
      }

