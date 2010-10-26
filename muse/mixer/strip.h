//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: strip.h,v 1.3.2.2 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __STRIP_H__
#define __STRIP_H__

//#include <q3frame.h>
#include <QFrame>
#include <qicon.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>

#include "globaldefs.h"
//#include "route.h"

class Track;
class QLabel;
class QVBoxLayout;
class Meter;
class QToolButton;
class QGridLayout;
class ComboBox;

static const int STRIP_WIDTH = 65;

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

class Strip : public QFrame {
      Q_OBJECT

   protected:
      Track* track;
      QLabel* label;
      QVBoxLayout* layout;
      Meter* meter[MAX_CHANNELS];
      bool useSoloIconSet2;
      
      QToolButton* record;
      QToolButton* solo;
      QToolButton* mute;
      QToolButton* iR; // Input routing button
      QToolButton* oR; // Output routing button
      QGridLayout* sliderGrid;
      ComboBox* autoType;
      void setLabelText();

   private slots:
      void recordToggled(bool);
      void soloToggled(bool);
      void muteToggled(bool);

   protected slots:
      virtual void heartBeat();
      void setAutomationType(int t,int);

   public slots:
      void resetPeaks();
      virtual void songChanged(int) = 0;

   public:
      Strip(QWidget* parent, Track* t);
      ~Strip();
      void setRecordFlag(bool flag);
      Track* getTrack() const { return track; }
      void setLabelFont();
      };

#endif

