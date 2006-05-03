//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: simplebutton.h,v 1.7 2005/11/05 11:05:25 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SIMPLE_BUTTON_H__
#define __SIMPLE_BUTTON_H__

//---------------------------------------------------------
//   SimpleButton
//---------------------------------------------------------

class SimpleButton : public QToolButton {
      Q_OBJECT

      int _id;
      virtual QSize minimumSizeHint() const { return QSize(0, 0); }

   public:
      SimpleButton(QPixmap* on, QPixmap* off, QWidget* parent = 0);
      SimpleButton(const QString& s, QWidget* parent = 0);
      };

#endif

