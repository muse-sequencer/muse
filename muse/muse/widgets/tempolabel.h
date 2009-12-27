//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tempolabel.h,v 1.1.1.1 2003/10/27 18:55:05 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TEMPOLABEL_H__
#define __TEMPOLABEL_H__

#include <qlabel.h>
#include <qspinbox.h>

//---------------------------------------------------------
//   TempoLabel
//---------------------------------------------------------

class TempoLabel : public QLabel {
      double _value;

      Q_OBJECT

   protected:
      QSize sizeHint() const;

   public slots:
      void setValue(int);
      void setValue(double);

   public:
      TempoLabel(QWidget*, const char* name = 0);
      };

//---------------------------------------------------------
//   TempoEdit
//---------------------------------------------------------

class TempoEdit : public QSpinBox {
      Q_OBJECT

   protected:
      QSize sizeHint() const;
      virtual QString mapValueToText(int);
      virtual int mapTextToValue(bool*);

   private slots:
      void tempoChanged(int);

   public slots:
      void setValue(double);

   signals:
      void valueChanged(double);

   public:
      TempoEdit(QWidget*, const char* name = 0);
      };

#endif

