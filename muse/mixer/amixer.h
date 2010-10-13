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
#include <q3listbox.h>
#include <qtooltip.h>
#include <q3frame.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <QLabel>
#include <Q3PopupMenu>
#include <QCloseEvent>
#include <Q3Action>

#include "cobject.h"
#include "synth.h"
#include "node.h"
#include "routedialog.h"

class Xml;
class QWidget;
class Q3HBoxLayout;
class Q3ScrollView;
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
class Q3PopupMenu;
class Strip;

struct MixerConfig;

#define EFX_HEIGHT     16

typedef std::list<Strip*> StripList;

//---------------------------------------------------------
//   AudioMixerApp
//---------------------------------------------------------

class AudioMixerApp : public Q3MainWindow {
      //QString name;
      MixerConfig* cfg;
      StripList stripList;
      Q3ScrollView* view;
      QWidget* central;
      Q3HBoxLayout* lbox;
      //Strip* master;
      Q3HBoxLayout* layout;
      Q3PopupMenu* menuView;
      RouteDialog* routingDialog;
      int routingId;
      int oldAuxsSize;

      Q3Action* showMidiTracksId;
      Q3Action* showDrumTracksId;
      Q3Action* showInputTracksId;
      Q3Action* showOutputTracksId;
      Q3Action* showWaveTracksId;
      Q3Action* showGroupTracksId;
      Q3Action* showAuxTracksId;
      Q3Action* showSyntiTracksId;

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
      //void showTracksChanged(QAction*);
      void showMidiTracksChanged(bool);
      void showDrumTracksChanged(bool);
      void showWaveTracksChanged(bool);
      void showInputTracksChanged(bool);
      void showOutputTracksChanged(bool);
      void showGroupTracksChanged(bool);
      void showAuxTracksChanged(bool);
      void showSyntiTracksChanged(bool);

   public:
      //AudioMixerApp(QWidget* parent);
      AudioMixerApp(QWidget* parent, MixerConfig* c);
      //void write(Xml&, const char* name);
      //void write(int level, Xml& xml, const char* name);
      void write(int level, Xml& xml);
      void clear();
      };

#endif

