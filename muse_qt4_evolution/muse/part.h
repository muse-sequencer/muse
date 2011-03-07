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

#ifndef __PART_H__
#define __PART_H__

#include "event.h"

class Track;
class MidiTrack;
class WaveTrack;

namespace AL {
      class Xml;
      };

using AL::Xml;

//---------------------------------------------------------
//   ClonePart
//---------------------------------------------------------

struct ClonePart {
      EventList* el;
      int id;
      ClonePart(EventList* e, int i) : el(e), id(i) {}
      };

typedef std::list<ClonePart> CloneList;
typedef CloneList::iterator iClone;
extern CloneList cloneList;

//---------------------------------------------------------
//   CtrlCanvas
//---------------------------------------------------------

struct CtrlCanvas {
	int ctrlId;
	int height;
      };

typedef std::list<CtrlCanvas> CtrlCanvasList;
typedef CtrlCanvasList::iterator iCtrlCanvas;
typedef CtrlCanvasList::const_iterator ciCtrlCanvas;

//---------------------------------------------------------
//   Part
//---------------------------------------------------------

class Part : public AL::PosLen {
      QString _name;
      bool _selected;
      bool _mute;
      int _colorIndex;

      // editor presets:
      int _raster, _quant;
      double _xmag;
      CtrlCanvasList ctrlCanvasList;

      // auto fill:
      int _fillLen;	// = 0 if no auto fill

   protected:
      Track* _track;
      EventList* _events;

   public:
      Part(Track*);

      CtrlCanvasList* getCtrlCanvasList() { return &ctrlCanvasList; }

      const QString& name() const      { return _name; }
      void setName(const QString& s)   { _name = s; }

      bool selected() const            { return _selected; }
      void setSelected(bool f)         { _selected = f; }

      bool mute() const                { return _mute; }
      void setMute(bool b)             { _mute = b; }

      Track* track() const             { return _track; }
      void setTrack(Track*t)           { _track = t; }

      int colorIndex() const           { return _colorIndex; }
      void setColorIndex(int idx)      { _colorIndex = idx; }

      int raster() const               { return _raster; }
      void setRaster(int val)          { _raster = val;  }

      int quant() const                { return _quant;  }
      void setQuant(int val)           { _quant = val;   }

      double xmag() const              { return _xmag;   }
      void setXmag(double val)         { _xmag = val;    }

      EventList* events() const        { return _events; }

      void clone(EventList* e);
      iEvent addEvent(Event& p);
      iEvent addEvent(Event& p, unsigned);

      int fillLen() const              { return _fillLen; }
      void setFillLen(int val)         { _fillLen = val;  }

      void read(QDomNode, bool isMidiPart);
      void write(Xml&);
      void dump(int n = 0) const;

      bool isClone() const;
      void deref();
      void ref();
      };

//---------------------------------------------------------
//   PartList
//---------------------------------------------------------

typedef std::multimap<unsigned, Part*, std::less<unsigned> > PL;
typedef PL::iterator iPart;
typedef PL::const_iterator ciPart;

class PartList : public PL {
   public:
      Part* findPart(unsigned tick);
      iPart add(Part*);
      void remove(Part* part);
      int index(Part*);
      Part* find(int idx);
      };

extern const char* partColorNames[];
#endif

