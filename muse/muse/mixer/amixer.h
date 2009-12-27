//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: amixer.h,v 1.27.2.2 2009/10/18 06:13:00 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __AMIXER_H__
#define __AMIXER_H__

#include <qlayout.h>
#include <qpixmap.h>
#include <qlistbox.h>
#include <qtooltip.h>
#include <qframe.h>

#include "cobject.h"
#include "synth.h"
#include "node.h"
#include "routedialog.h"

class Xml;
class QWidget;
class QHBoxLayout;
class QScrollView;
class AudioTrack;
class Meter;
class Track;
class QLabel;
class Slider;
class QToolButton;
class Knob;
class DoubleLabel;
class ComboBox;
class RouteDialog;
class QPopupMenu;
class Strip;

#define EFX_HEIGHT     16

//---------------------------------------------------------
//   AudioMixerApp
//---------------------------------------------------------

class AudioMixerApp : public QMainWindow {
      QScrollView* view;
      QWidget* central;
      QHBoxLayout* lbox;
      Strip* master;
      QHBoxLayout* layout;
      QPopupMenu* menuView;
      RouteDialog* routingDialog;
      int routingId;
      int oldAuxsSize;

      Q_OBJECT

      virtual void closeEvent(QCloseEvent*);
      void addStrip(Track*, int);
      void showRouteDialog(bool);

      enum UpdateAction {
            NO_UPDATE, UPDATE_ALL, UPDATE_MIDI, STRIP_INSERTED, STRIP_REMOVED
            };
      void updateMixer(UpdateAction);

   signals:
      void closed();

   private slots:
      void songChanged(int);
      //void configChanged()    { songChanged(-1); }
      void configChanged();
      void toggleRouteDialog();
      void routingDialogClosed();

   public:
      AudioMixerApp(QWidget* parent);
      void clear();
      };

#endif

