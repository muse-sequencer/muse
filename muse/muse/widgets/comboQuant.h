//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: comboQuant.h,v 1.3 2005/11/04 12:03:48 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __COMBOQUANT_H__
#define __COMBOQUANT_H__

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
      ComboQuant(QWidget* parent = 0);
      void setValue(int val);
      };

#endif

