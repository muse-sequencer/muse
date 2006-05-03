//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: combobox.h,v 1.7 2005/10/05 18:15:27 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __COMBOBOX_H__
#define __COMBOBOX_H__

class Q3PopupMenu;

//---------------------------------------------------------
//   ComboBox
//---------------------------------------------------------

class ComboBox : public QLabel {
      Q_OBJECT
      Q_PROPERTY(int id READ id WRITE setId)

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



