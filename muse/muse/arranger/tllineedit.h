//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: tllineedit.h,v 1.3 2006/01/06 22:48:09 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TLLINEEDIT_H__
#define __TLLINEEDIT_H__

//---------------------------------------------------------
//   TLLineEdit
//---------------------------------------------------------

class TLLineEdit : public QLineEdit {
      Q_OBJECT

      virtual void mouseDoubleClickEvent(QMouseEvent*);
      virtual void mousePressEvent(QMouseEvent*);

   private slots:
      void contentHasChanged();

   signals:
      void contentChanged(QString s);
      void mousePress();

   public:
      TLLineEdit(const QString& contents, QWidget* parent = 0);
      };

#endif



