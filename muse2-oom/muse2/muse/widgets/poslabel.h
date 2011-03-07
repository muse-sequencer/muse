//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: poslabel.h,v 1.2 2004/01/11 18:55:37 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __POSLABEL_H__
#define __POSLABEL_H__

#include <QLabel>

//---------------------------------------------------------
//   PosLabel
//---------------------------------------------------------

class PosLabel : public QLabel {
      bool _smpte;
      unsigned _tickValue;
      unsigned _sampleValue;
      Q_OBJECT

      void updateValue();

   protected:
      QSize sizeHint() const;

   public slots:
      void setTickValue(unsigned);
      void setSampleValue(unsigned);
      void setValue(unsigned);

   public:
      PosLabel(QWidget* parent, const char* name = 0);
      unsigned value()       const { return _smpte ? _sampleValue : _tickValue; }
      unsigned tickValue()   const { return _tickValue; }
      unsigned sampleValue() const { return _sampleValue; }
      void setSmpte(bool);
      bool smpte() const { return _smpte; }
      };


#endif


