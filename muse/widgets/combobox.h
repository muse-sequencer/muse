//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: combobox.h,v 1.3 2004/02/29 12:12:36 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __COMBOBOX_H__
#define __COMBOBOX_H__

#include <qlabel.h>
//Added by qt3to4:
#include <Q3PopupMenu>
#include <QMouseEvent>

class Q3PopupMenu;

//---------------------------------------------------------
//   ComboBox
//---------------------------------------------------------

class ComboBox : public QLabel {
      Q_OBJECT
      Q_PROPERTY( int id READ id WRITE setId )

      int _id;
      int _currentItem;
      Q3PopupMenu* list;
      virtual void mousePressEvent(QMouseEvent*);

   private slots:
      void activatedIntern(int);

   signals:
      void activated(int val, int id);

   public:
      ComboBox(QWidget* parent, const char* name = 0);
      ~ComboBox();
      void setCurrentItem(int);
      void insertItem(const QString& s, int id = -1, int idx=-1);
      int id() const       { return _id; }
      void setId(int i)    { _id = i; }
      };

#endif



