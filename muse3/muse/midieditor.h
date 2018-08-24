//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: midieditor.h,v 1.3.2.2 2009/02/02 21:38:00 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __MIDIEDITOR_H__
#define __MIDIEDITOR_H__

#include "type_defs.h"
#include "al/sig.h"
#include "cobject.h"
// REMOVE Tim. citem. Added.
//#include "citem.h"
#include "ctrl/ctrledit.h"


#include <set>

class QGridLayout;
class QWidget;
class QPoint;

namespace MusECore {
class Track;
class Part;
class PartList;
class WavePart;
class Xml;
}

namespace MusEGui {
class EventCanvas;
class MTScale;
class ScrollScale;
class TrackInfoWidget;
//class WaveView;

//---------------------------------------------------------
//   MidiEditor
//---------------------------------------------------------

class MidiEditor : public TopWin  {
      Q_OBJECT

      MusECore::PartList* _pl;
      std::set<int> _parts;
      int _curDrumInstrument;  // currently selected instrument if drum
                               // editor

   protected:
      MusEGui::ScrollScale* hscroll;
      MusEGui::ScrollScale* vscroll;
      MusEGui::MTScale* time;
      EventCanvas* canvas;
      //WaveView* wview;
      
      TrackInfoWidget* trackInfoWidget;
      QWidget* noTrackInfo;
      MusECore::Track* selected;

      CtrlEditList ctrlEditList;
      int _raster;
      QGridLayout* mainGrid;
      QWidget* mainw;
      virtual void readStatus(MusECore::Xml&);
      virtual void writeStatus(int, MusECore::Xml&) const;
      void writePartList(int, MusECore::Xml&) const;
      void genPartlist();
      void movePlayPointerToSelectedEvent();
      
      void genTrackInfo(TrackInfoWidget* trackInfo);
      void switchInfo(int);
      void trackInfoSongChange(MusECore::SongChangedStruct_t flags);
      // Checks if track info track is valid and deletes the strip if the track is not found.
      void checkTrackInfoTrack();

   protected slots:
      void updateTrackInfo();
      
   private slots:
      void addNewParts(const std::map< const MusECore::Part*, std::set<const MusECore::Part*> >&);

   public slots:
      void songChanged(MusECore::SongChangedStruct_t type);
      void setCurDrumInstrument(int instr);

      virtual void updateHScrollRange() { };
   signals:
      void curDrumInstrumentChanged(int);

   public:
      MidiEditor(ToplevelType t, int, MusECore::PartList*,
         QWidget* parent = 0, const char* name = 0);
      ~MidiEditor();

      int rasterStep(unsigned tick) const   { return AL::sigmap.rasterStep(tick, _raster); }
      unsigned rasterVal(unsigned v)  const { return AL::sigmap.raster(v, _raster);  }
      unsigned rasterVal1(unsigned v) const { return AL::sigmap.raster1(v, _raster); }
      unsigned rasterVal2(unsigned v) const { return AL::sigmap.raster2(v, _raster); }
      int raster() const           { return _raster; }
      void setRaster(int val)      { _raster = val; }
      MusECore::PartList* parts()            { return _pl;  }
      int curDrumInstrument() const  { return _curDrumInstrument; }
      MusECore::Part* curCanvasPart();
      void setCurCanvasPart(MusECore::Part*); 
      void addPart(MusECore::Part*);
      // REMOVE Tim. citem. Added.
      bool itemsAreSelected() const;
      // Adds all selected items to the given list. Does not clear the list first.
      // Checks for duplicates, employing the 'tagged' features.
      //void getAllSelectedItems(CItemSet&) const;
      // Tags all selected item objects. Checks for duplicates, employing the 'tagged' features.
      void tagItems(bool tagAllItems = false, bool tagAllParts = false, bool range = false,
        const MusECore::Pos& = MusECore::Pos(),
        const MusECore::Pos& = MusECore::Pos()) const;
      };

} // namespace MusEGui

#endif

