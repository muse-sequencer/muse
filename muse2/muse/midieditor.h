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
#include "pos.h" // for XTick

#include <set>

class QGridLayout;
class QWidget;
class QPoint;

namespace MusECore {
class Part;
class PartList;
class WavePart;
class Xml;
}

namespace MusEGui {
class CtrlEdit;
class EventCanvas;
class MTScale;
class ScrollScale;
//class WaveView;

using MusECore::XTick;

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

      std::list<CtrlEdit*> ctrlEditList;

      //raster=n raster on n ticks
      //raster=1 means raster on full ticks (for int-ticks, this does nothing)
      //raster=0 means raster on measure
      //raster<0 would mean "don't even touch the subtick" (for XTicks and int-ticks, this does nothing)
      int _raster;
      enum {NO_RASTER=-1};

      QGridLayout* mainGrid;
      QWidget* mainw;
      virtual void readStatus(MusECore::Xml&);
      virtual void writeStatus(int, MusECore::Xml&) const;
      void writePartList(int, MusECore::Xml&) const;
      void genPartlist();

   private slots:
      void addNewParts(const std::map< MusECore::Part*, std::set<MusECore::Part*> >&);

   public slots:
      void songChanged(MusECore::SongChangedFlags_t type);
      void setCurDrumInstrument(int instr);

      virtual void updateHScrollRange() { };
   signals:
      void curDrumInstrumentChanged(int);

   public:
      MidiEditor(ToplevelType t, int, MusECore::PartList*,
         QWidget* parent = 0, const char* name = 0);
      ~MidiEditor();

      // this is ugly. better use XTick everywhere (flo)
      XTick rasterStep(XTick tick) const { return _raster==NO_RASTER ? tick : XTick(AL::sigmap.rasterStep(tick.to_uint(), _raster)); }
      XTick rasterVal(XTick tick)  const { return _raster==NO_RASTER ? tick : XTick(AL::sigmap.raster(tick.to_uint(), _raster));  }
      XTick rasterVal1(XTick tick) const { return _raster==NO_RASTER ? tick : XTick(AL::sigmap.raster1(tick.to_uint(), _raster)); }
      XTick rasterVal2(XTick tick) const { return _raster==NO_RASTER ? tick : XTick(AL::sigmap.raster2(tick.to_uint(), _raster)); }
      int raster() const           { return _raster; }
      void setRaster(int val)      { _raster = val; }
      void disableRaster()         { _raster = -1; }
      MusECore::PartList* parts()            { return _pl;  }
      int curDrumInstrument() const  { return _curDrumInstrument; }
      MusECore::Part* curCanvasPart();
      void setCurCanvasPart(MusECore::Part*); 
      void addPart(MusECore::Part*);
      };

} // namespace MusEGui

#endif

