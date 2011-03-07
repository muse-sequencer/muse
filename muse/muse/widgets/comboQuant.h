//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: comboQuant.h,v 1.1.1.1 2003/10/27 18:54:30 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __COMBOQUANT_H__
#define __COMBOQUANT_H__

#include <qwidget.h>
#include <qcombobox.h>

//---------------------------------------------------------
//   ComboQuant
//---------------------------------------------------------

class ComboQuant : public QComboBox {
      Q_OBJECT

   private slots:
      void activated(int);

   signals:
      void valueChanged(int);

   public:
      ComboQuant(QWidget* parent = 0, const char* name = 0);
      void setValue(int val);
      };

#endif

