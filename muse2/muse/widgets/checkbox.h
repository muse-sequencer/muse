//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: checkbox.h,v 1.2.2.2 2006/10/29 07:54:52 terminator356 Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CHECKBOX_H__
#define __CHECKBOX_H__

#include <qcheckbox.h>
//Added by qt3to4:
#include <QMouseEvent>

//---------------------------------------------------------
//   CheckBox
//---------------------------------------------------------

class CheckBox : public QCheckBox {
      Q_OBJECT
      Q_PROPERTY( int id READ id WRITE setId )

      int _id;

   protected:
      void mousePressEvent(QMouseEvent *e);
      void mouseReleaseEvent(QMouseEvent *e);
   
   private slots:
      void hasToggled(bool val);

   signals:
      void toggleChanged(bool, int);
      void checkboxPressed(int);
      void checkboxReleased(int);
      void checkboxRightClicked(const QPoint &, int);

   public:
      CheckBox(QWidget* parent, int i, const char* name = 0);
      int id() const       { return _id; }
      void setId(int i)    { _id = i; }
      };

#endif

