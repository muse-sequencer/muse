//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: spinboxFP.h,v 1.1.1.1.2.1 2008/05/21 00:28:54 terminator356 Exp $
//    (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SPINBOXFP_H__
#define __SPINBOXFP_H__

//#include <QSpinBox>
#include <QDoubleSpinBox>

//class QValidator;
//class QDoubleValidator;

//---------------------------------------------------------
//   SpinBoxFP
//---------------------------------------------------------

//class SpinBoxFP : public QSpinBox {
class SpinBoxFP : public QDoubleSpinBox {
      Q_OBJECT
      //Q_PROPERTY( int decimals READ decimals WRITE setDecimals )

      //int _decimals; 
      //QDoubleValidator* validator;
      
   signals:
      void valueChanged(int);
      
   private slots:
      void valueChange(double);
   
   protected: 
      //virtual QString textFromValue(int) const;
      //virtual int valueFromText(const QString&) const;
      //virtual QValidator::State validate(QString&, int&) const;

   public:
      SpinBoxFP(QWidget* parent=0);
      SpinBoxFP(int minValue, int maxValue, int step = 1, QWidget* parent=0);
      //SpinBoxFP(double minValue, double maxValue, double step = 1.0, QWidget* parent=0);
      
      void setValue(int val);
      int intValue();
      
      void setDecimals(int);
      //int decimals() const { return _decimals; }
      };

#endif

