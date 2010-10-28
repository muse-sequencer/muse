//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: lcombo.h,v 1.1.1.1.2.3 2009/07/01 22:14:56 spamatica Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __LCOMBO_H__
#define __LCOMBO_H__

//#include <qwidget.h>
//#include <qcombobox.h>

class QString;
class QWidget;

//class Q3ListBox;
#include <QAbstractItemView>
#include <QComboBox>


//---------------------------------------------------------
//   LabelCombo
//---------------------------------------------------------

class LabelCombo : public QWidget {
      QComboBox* box;
      Q_OBJECT

   signals:
      void activated(int);

   public slots:
      void clearFocus() { box->clearFocus(); }

   public:
      LabelCombo(const QString& label, QWidget* parent,
         const char* name=0);
      void addItem(const QString& txt, const QVariant & userData = QVariant()) { box->addItem(txt, userData); }
      void insertItem(int index, const QString& txt, const QVariant & userData = QVariant()) { box->insertItem(index, txt, userData); }
      void setCurrentIndex(int i) { box->setCurrentIndex(i); }
      //void setListBox(Q3ListBox* lb) { box->setListBox(lb); } // ddskrjo
      void setView(QAbstractItemView* v) { box->setModel(v->model()); box->setView(v); } // p4.0.3
      void setFocusPolicy ( Qt::FocusPolicy fp ) { box->setFocusPolicy(fp); }
      };

#endif
