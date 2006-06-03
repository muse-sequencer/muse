//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __ARRANGER_H__
#define __ARRANGER_H__

#include "widgets/tools.h"
#include "track.h"

namespace AL {
      class Xml;
      class Pos;
      };
using AL::Xml;
class AL::Pos;

class Track;
class TrackInfo;
class TLWidget;
class TLSWidget;
class PartCanvas;
class Part;
class SimpleButton;
class Strip;
class ArrangerTrack;
class Splitter;
class TlsvLayout;
class TLayout;

//---------------------------------------------------------
//   TrackListWidget
//---------------------------------------------------------

class TrackListWidget : public QWidget {
      Q_OBJECT

      void mousePressEvent(QMouseEvent*);
      void wheelEvent(QWheelEvent*);
      virtual void paintEvent(QPaintEvent*);

   signals:
      void mouseWheel(QWheelEvent*);

   public:
      TrackListWidget(QWidget* parent = 0);
      };

//---------------------------------------------------------
//   Arranger
//---------------------------------------------------------

class Arranger : public QWidget {
      Q_OBJECT

      QAction* infoDockAction;
      QAction* mixerDockAction;

      TrackInfo* trackInfos[Track::TRACK_TYPES];
      Tool tool;
      QWidget* trackList;
      PartCanvas* canvas;
      TLayout* tl;               // tracklist layout
      TrackListWidget* tlsv;
      TlsvLayout* tlsvLayout;
      QDialog* configTrackEditor;
      QStackedWidget* trackInfo;
      QScrollArea* infoView;
      Track* _curTrack;             // current selected track
      SimpleButton* gmute;
      SimpleButton* gsolo;
      SimpleButton* gar;
      SimpleButton* gaw;
      Strip* strip;
      QStackedWidget* info;

      Splitter* split;
      QDockWidget* infoDock;
      QDockWidget* mixerDock;

      int startH;    // start value for resize track height

      bool trackInfoVisible;
      bool mixerStripVisible;

      void updateIndex();
      TrackInfo* createTrackInfo();
      int tlIndex(Track*) const;
      int tlIndex(ArrangerTrack* t) const;
      void initSubtrack(Track* t, ArrangerTrack*);
      ArrangerTrack* atrack(int idx);
      void insertTrack1(Track*);

   private slots:
      void startDrag(int idx);
      void drag(int idx, int);
      void setTLViewPos(int, int);
      void appendSubtrack(TLWidget*);
      void removeSubtrack(TLSWidget*);
      void configTrackList();
      void toggleTrackInfo(bool);
      void toggleMixerStrip(bool);
      void setGMute();
      void setGSolo();
      void setGar();
      void setGaw();
      void offGMute();
      void offGSolo();
      void offGar();
      void offGaw();
      void setSelectedTrack(Track*);
      void moveTrack(Track* src, Track* dst);
      void kbdMovementUpdate(Track* t, Part* p);
      void mouseWheel(QWheelEvent*);
      void setPos(int, const AL::Pos&);
      void addMarker(const AL::Pos&);
      void removeMarker(const AL::Pos&);

   public slots:
      void insertTrack(Track*);
      void removeTrack(Track*);
      void setTool(int t);
      void updateConfiguration();
      void startLoadSong();

   signals:
      void configChanged();
      void editPart(Part*);
      void cursorPos(const AL::Pos&,bool);

   public:
      Arranger(QMainWindow* parent = 0);
      void readStatus(QDomNode);
      void writeStatus(Xml&);
      Track* curTrack() const { return _curTrack; }
      void endLoadSong();
      static int trackNameWidth;

   protected:
      virtual void keyPressEvent(QKeyEvent* e);
      };

#endif

