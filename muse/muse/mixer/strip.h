//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: strip.h,v 1.21 2006/01/12 14:49:13 wschweer Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __STRIP_H__
#define __STRIP_H__

#include "globaldefs.h"
#include "gui.h"

class Track;
class Meter;
class SimpleButton;
class Mixer;

static const QSize buttonSize(STRIP_WIDTH/2-2, STRIP_WIDTH/3);
static const QSize entrySize(STRIP_WIDTH/2-2, 17);

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

class Strip : public QFrame {
      Q_OBJECT

   protected:
      Mixer* mixer;
      Track* track;
      QLabel* label;
      QVBoxLayout* layout;

      SimpleButton* solo;
      SimpleButton* mute;
      void updateLabel();
      bool _align;      // align elements for mixer app

      void recordToggled(bool);
      void addAutomationButtons();

   public slots:
      void resetPeaks();
      virtual void songChanged(int) = 0;
      virtual void controllerChanged(int) {}
      void configChanged();

   public:
      Strip(Mixer* m, Track* t, bool align);
      ~Strip();
      Track* getTrack() const { return track; }
      };

#endif

