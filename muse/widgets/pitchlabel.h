//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pitchlabel.h,v 1.1.1.1 2003/10/27 18:54:49 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __PITCHLABEL_H__
#define __PITCHLABEL_H__

#include <qlabel.h>

//---------------------------------------------------------
//   PitchLabel
//---------------------------------------------------------

class PitchLabel : public QLabel {
      bool _pitchMode;
      int _value;
      Q_OBJECT

   protected:
      QSize sizeHint() const;

   public slots:
      void setValue(int);
      void setInt(int);
      void setPitch(int);

   public:
      PitchLabel(QWidget* parent, const char* name = 0);
      int value() const { return _value; }
      void setPitchMode(bool val);
      bool pitchMode() const { return _pitchMode; }
      };


#endif



