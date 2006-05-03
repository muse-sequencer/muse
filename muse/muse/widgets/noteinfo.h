//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: noteinfo.h,v 1.9 2005/10/05 18:15:27 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __NOTE_INFO_H__
#define __NOTE_INFO_H__

namespace Awl {
      class PosEdit;
      class PitchEdit;
      };
namespace AL {
      class Pos;
      }

using AL::Pos;

//---------------------------------------------------------
//   NoteInfo
//---------------------------------------------------------

class NoteInfo : public QToolBar {
      Awl::PosEdit* selTime;
      QSpinBox* selLen;
      Awl::PitchEdit* selPitch;
      QSpinBox* selVelOn;
      QSpinBox* selVelOff;
      bool deltaMode;

      Q_OBJECT

   public:
      enum ValType {VAL_TIME, VAL_LEN, VAL_VELON, VAL_VELOFF, VAL_PITCH };
      NoteInfo(QMainWindow* parent);
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

