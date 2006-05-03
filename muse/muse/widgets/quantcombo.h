//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: quantcombo.h,v 1.1 2006/01/24 21:19:28 wschweer Exp $
//  (C) Copyright 2006 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __QUANTCOMBO_H__
#define __QUANTCOMBO_H__

//---------------------------------------------------------
//   QuantCombo
//---------------------------------------------------------

class QuantCombo : public QComboBox {
      Q_OBJECT

   private slots:
      void _quantChanged(int);

   public slots:
      void setQuant(int);

   signals:
      void quantChanged(int);

   public:
      QuantCombo(QWidget* parent = 0);
      int quant() const;
      };

#endif

