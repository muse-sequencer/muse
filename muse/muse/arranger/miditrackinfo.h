//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: miditrackinfo.h,v 1.2 2004/10/07 17:53:47 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDITRACKINFO_H__
#define __MIDITRACKINFO_H__


#include "mtrackinfobase.h"

class MidiTrack;

//---------------------------------------------------------
//   MidiTrackInfo
//---------------------------------------------------------

class MidiTrackInfo : public MidiTrackInfoBase {
       Q_OBJECT

      MidiTrack* track;
      int volume, pan, program;

   private slots:
      void programChanged();

   public:
      MidiTrackInfo(QWidget* parent);
      void setTrack(MidiTrack* t) { track = t; }
      };

#endif
