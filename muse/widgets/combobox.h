//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: combobox.h,v 1.3 2004/02/29 12:12:36 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __COMBOBOX_H__
#define __COMBOBOX_H__

#include <QToolButton>

class QMenu;
class QSignalMapper;

//---------------------------------------------------------
//   ComboBox
//---------------------------------------------------------

class ComboBox : public QToolButton {
      Q_OBJECT

      int _currentItem;
      QList<int> itemlist;

      QMenu* menu;
      virtual void mousePressEvent(QMouseEvent*);
      virtual void wheelEvent(QWheelEvent*);

      QSignalMapper* autoTypeSignalMapper;

   private slots:
      void activatedIntern(int id);

   signals:
      void activated(int id);

   public:
      ComboBox(QWidget* parent = 0, const char* name = 0);
      ~ComboBox();
      void setCurrentItem(int);
      void addAction(const QString& s, int id = -1);
      };

#endif



