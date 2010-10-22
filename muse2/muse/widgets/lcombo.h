//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: lcombo.h,v 1.1.1.1.2.3 2009/07/01 22:14:56 spamatica Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __LCOMBO_H__
#define __LCOMBO_H__

#include <qwidget.h>
#include <qcombobox.h>

class QString;
//class Q3ListBox;
#include <QAbstractItemView>


//---------------------------------------------------------
//   LabelCombo
//---------------------------------------------------------

class LabelCombo : public QWidget {
      QComboBox* box;
      Q_OBJECT

   signals:
      void activated(int);

   public slots:
      void clearFocus();
   public:
      LabelCombo(const QString& label, QWidget* parent,
         const char* name=0);
      void insertItem(const QString& txt, int index=-1);
      void setCurrentItem(int i) { box->setCurrentItem(i); }
      //void setListBox(Q3ListBox* lb) { box->setListBox(lb); } // ddskrjo
      void setView(QAbstractItemView* v) { box->setModel(v->model()); box->setView(v); } // p4.0.3
      void setFocusPolicy ( Qt::FocusPolicy fp );
  
      };

#endif
