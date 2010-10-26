//  MusE
//  Linux Music Editor
//    $Id: noteinfo.h,v 1.3 2004/01/09 17:12:54 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __NOTE_INFO_H__
#define __NOTE_INFO_H__

//#include <q3toolbar.h>
#include <QToolBar>

class PosEdit;
class QSpinBox;
class PitchEdit;
//class QMainWindow;
class QWidget;
class Pos;

//---------------------------------------------------------
//   NoteInfo
//---------------------------------------------------------

class NoteInfo : public QToolBar {
      PosEdit* selTime;
      QSpinBox* selLen;
      PitchEdit* selPitch;
      QSpinBox* selVelOn;
      QSpinBox* selVelOff;
      bool deltaMode;

      Q_OBJECT

   public:
      enum ValType {VAL_TIME, VAL_LEN, VAL_VELON, VAL_VELOFF, VAL_PITCH };
      //NoteInfo(QMainWindow* parent);
      NoteInfo(QWidget* parent = 0);
      void setValues(unsigned, int, int, int, int);
      void setDeltaMode(bool);

   private slots:
      void lenChanged(int);
      void velOnChanged(int);
      void velOffChanged(int);
      void pitchChanged(int);
      void timeChanged(const Pos&);

   public slots:
      void setValue(ValType, int);

   signals:
      void valueChanged(NoteInfo::ValType, int);
      };
#endif

