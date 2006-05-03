//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Description:
//  Configuration settings for the midi-editors.
//
//  Copyright (C) 2004 Mathias Lundgren <lunar_shuttle@users.sourceforge.net>
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

#ifndef __ESETTINGS_H__
#define __ESETTINGS_H__

#include <unistd.h>
#include <fcntl.h>

#define ET_LISTEDIT                 0x1000000
#define ET_MASTEREDIT               0x4000000
#define ET_PIANO_ROLL              0x10000000
#define ET_DRUMEDIT                0x20000000
#define ET_WAVEEDIT                0x40000000

#define MAXNOOFCTRLEDITSETTINGS    16

#define ES_UNINIT                  -1 /* Uninitialized parameter */

#define ESETTINGS_DEBUG            0

#include "debug.h"
#define DBG_ESETTINGS_ON                         0
#define DBG_ESETTINGS debugMsg && DBG_ESETTINGS_ON

namespace AL {
      class Xml;
      };
using AL::Xml;

class CtrlEdit;

//-----------------------------------------------
// CtrlEditSettings class
// stores ctrleditsettings
//-----------------------------------------------
class CtrlEditSettings
      {
   private:
      int height;
      int controller;

   public:
      //const MidiController* controller;
      CtrlEditSettings() {}
      CtrlEditSettings(int h, int c) { height = h; controller = c; }
      CtrlEditSettings(const CtrlEditSettings& c) { height = c.height; controller = c.controller; }
      int getHeight() { return height; }
      int getController() { return controller; }
      void readStatus(QDomNode);
      void writeStatus(Xml& xml);

      bool operator==(const CtrlEditSettings& c) const { return (c.height == height && c.controller == controller); }
      bool operator!=(const CtrlEditSettings& c) const { return !(c == *this); }
      };

//-----------------------------------------------
// EditorSettings
// base class for midieditorsettings
//-----------------------------------------------
class EditorSettings
      {
   protected:
      int _raster;
      int _width, _height, _x, _y;

   public:
      EditorSettings(int r=96, int w=600, int h=400, int x=-1, int y=-1) : _raster(r), _width(w), _height(h), _x(x), _y(y) { }
      virtual ~EditorSettings() {}
      int raster() const    { return _raster; }
      void setRaster(int r) { _raster = r; }
      int* rasterPtr()      { return &_raster; }
      int width() const     { return _width; }
      int height() const    { return _height; }
      void setWidth(int w)  { _width = w; }
      void setHeight(int h) { _height = h; }
      int x() const         { return _x; }
      void setX(int x)      { _x = x; }
      int y() const         { return _y; }
      void setY(int y)      { _y = y; }

      virtual void readStatus(QDomNode);
      virtual void writeStatus(Xml&) const;
      virtual bool operator==(const EditorSettings& e) const {
            return ((e._raster == _raster) && (e._width == _width) && (e._height == _height)/* && (e._x == _x) && (e._y == _y)*/);
            }
      virtual bool operator!=(const EditorSettings& e) const { return !(e == *this); }
      virtual EditorSettings* clone() { return new EditorSettings(_raster, _width, _height, _x, _y); }
      virtual void dump() { printf("%p: EditorSettings: r:%d w:%d h:%d x:%d y:%d\n", this, _raster, _width, _height, _x, _y); }
      };

//---------------------------------------------------------
//   GraphEditorSettings
//---------------------------------------------------------
class GraphEditorSettings : public EditorSettings
      {
   protected:
      //Values considering scroll + zoom
      double _xmag, _ymag;
      QPoint _pos;

   public:
      GraphEditorSettings(int r=96, int w=600, int h=400, int x=-1, int y=-1,
         double xmag=0.05, double ymag=1.0, QPoint pos = QPoint(0, 0))
         : EditorSettings(r, w, h, x, y), _xmag(xmag), _ymag(ymag), _pos(pos)
            {
            }
      virtual ~GraphEditorSettings() {}

      double xmag() const { return _xmag; }
      double ymag() const { return _ymag; }
      void setXmag(double x) { _xmag = x; }
      void setYmag(double y) { _xmag = y; }
      QPoint pos() const { return _pos; }
      void setPos(const QPoint& p) { _pos = p; }
      int ypos() const { return _pos.y(); }
      void setYpos(int y) { _pos.setY(y); }

      virtual bool operator==(const EditorSettings& e) const {
            const GraphEditorSettings& f = (GraphEditorSettings&) e;
            return ((f._raster == _raster) && (f._width == _width) && (f._height == _height)// && (f._x == _x) && (f._y == _y)
                  && (f._xmag == _xmag)
                  && (f._ymag == _ymag)
                  && (f._pos == _pos));
            }
      virtual bool operator!=(const EditorSettings& e) const { return !(e == *this); }
      virtual EditorSettings* clone() { return new GraphEditorSettings(_raster, _width, _height, _x, _y, _xmag, _ymag, _pos); }
      virtual void readStatus(QDomNode);
      virtual void writeStatus(Xml&) const;
      virtual void dump() {
            printf("%p: GraphEditorSettings: r:%d w:%d h:%d x:%d y:%d mag:%f:%f pos:%d ypos:%d\n",
               this, _raster, _width, _height, _x, _y, _xmag, _ymag, _pos.x(), _pos.y());
            }
      };

//---------------------------------------------------------
//   ExtEditorSettings
// Common settings for drumeditor and pianoroll
//---------------------------------------------------------

class ExtEditorSettings : public GraphEditorSettings
      {
   private:

   protected:
      int _quant;
      int _applyTo;
      static bool _steprec;
      static bool _midiin;
      int _numOfCtrlEdits;

   public:
      ExtEditorSettings(int r=96, int w=600, int h=400, int x=-1, int y=-1, double xm=0.05, double ym=1.0, QPoint pos = QPoint(0,0), int q=96, int apply=0);
      virtual ~ExtEditorSettings();
      //ExtEditorSettings(const ExtEditorSettings& e) : EditorSettings(e._raster) , _quant(e._quant) { }
      static void readStatic(QDomNode);
      static void writeStatic(Xml& xml);
      CtrlEditSettings* ctrlEdits[MAXNOOFCTRLEDITSETTINGS];

      virtual bool operator==(const EditorSettings& e) const { //örk...
            const ExtEditorSettings& f = (ExtEditorSettings&) e;
            bool ctrlEditsEqual = true;
            for (int i=0; i<MAXNOOFCTRLEDITSETTINGS; i++) {
                  if (ctrlEdits[i] != f.ctrlEdits[i])
                        ctrlEditsEqual = false;
                  }
            return ((f._raster == _raster) && (f._width == _width) && (f._height == _height)// && (f._x == _x) && (f._y == _y)
                  && (f._xmag == _xmag)
                  && (f._ymag == _ymag)
                  && (f._pos == _pos)
                  && (f._quant == _quant) && (f._applyTo == _applyTo)
                  && (ctrlEditsEqual));
            };
      virtual bool operator!=(const EditorSettings& e) const { const ExtEditorSettings& f = (ExtEditorSettings&) e; return !(f == *this); }

      virtual EditorSettings* clone();
      virtual void readStatus(QDomNode);
      virtual void writeStatus(Xml&) const;
      virtual void dump() {
            printf("%p: ExtEditorSettings: r:%d q:%d w:%d h:%d x:%d y:%d mag=%f:%f xpos=%d ypos=%d applyTo=%d\n",
            this, _raster, _quant, _width, _height, _x, _y, _xmag, _ymag, _pos.x(), _pos.y(), _applyTo);
            }
      void setControlEditSettings(int pos, CtrlEditSettings* c);
      void setControlEditSize(int s) { _numOfCtrlEdits = s; }
      int getControlEditSize() { return _numOfCtrlEdits; }
      CtrlEditSettings* getControlEditSettings(int pos) { return ctrlEdits[pos]; }

      int quant() const { return _quant; }
      void setQuant(int q) { _quant = q; }

      static bool steprec() { return _steprec; }
      static void setSteprec(bool b) { _steprec = b; }
      static bool midiin() { return _midiin; }
      static void setMidiin(bool b) { _midiin = b; }
      };


//---------------------------------------------------------
//   DrumEditorSettings
// Settings for drumeditor
//---------------------------------------------------------
class DrumEditorSettings : public ExtEditorSettings
      {
      int _dlistWidth, _dcanvasWidth;

      // Default initialization values
      static int _quantInit, _rasterInit;
      static int _widthInit, _heightInit;

   public:
      DrumEditorSettings(int r=ES_UNINIT, int w=ES_UNINIT, int h=ES_UNINIT,
         int x=-1, int y=-1,
         double xm=0.05, double ym=1.0, QPoint pos=QPoint(0,0), int q=ES_UNINIT, int apply=0, int dl=50, int dw=300);
      virtual ~DrumEditorSettings() {}

      static void readStatic(QDomNode);
      static void writeStatic(Xml& xml);
      int dlistWidth() const   { return _dlistWidth; }
      int dcanvasWidth() const { return _dcanvasWidth; }
      void setdlistWidth(int d)      { _dlistWidth = d; }
      void setdcanvasWidth(int d)    { _dcanvasWidth = d; }

      virtual bool operator==(const EditorSettings& e) const { //öööörk...
            DrumEditorSettings& f = (DrumEditorSettings&) e;
            bool ctrlEditsEqual = true;
            for (int i=0; i<MAXNOOFCTRLEDITSETTINGS; i++) {
                  if (ctrlEdits[i] != f.ctrlEdits[i])
                        ctrlEditsEqual = false;
                  }
            return ((f._raster == _raster) && (f._width == _width) && (f._height == _height)// && (f._x == _x) && (f._y == _y)
                  && (f._xmag == _xmag)
                  && (f._ymag == _ymag)
                  && (f._pos == _pos)
                  && (f._quant == _quant) && (f._applyTo == _applyTo)
                  && (ctrlEditsEqual)
                  && (f._dlistWidth == _dlistWidth) && (f._dcanvasWidth == _dcanvasWidth));
            }
      virtual bool operator!=(const EditorSettings& e) const { DrumEditorSettings& f = (DrumEditorSettings&) e; return !(f==*this); }
      virtual EditorSettings* clone();
      virtual void readStatus(QDomNode);
      virtual void writeStatus(Xml&) const;
      virtual void dump();

      static void setStaticInitValues(int widthinit, int heightinit, int rasterinit, int quantinit);
      };


//---------------------------------------------------------
//   PianorollSettings
// Settings for pianoroll
//---------------------------------------------------------
class PianorollSettings : public ExtEditorSettings
      {
      int _quantStrength, _quantLimit, _colorMode;
      bool _quantLen;
      int _pianoWidth;

      // Default initialization values
      static int _quantInit;
      static int _rasterInit;
      static int _widthInit;
      static int _heightInit;

      public:
      PianorollSettings(int r=ES_UNINIT, int w=ES_UNINIT, int h=ES_UNINIT, 
         int x=-1, int y=-1, double xm=0.05, double ym = 1.0,
         QPoint pos = QPoint(0, 245), int q=ES_UNINIT, int apply=0,
         int qs=80, int qlim=50, int cmode=0, bool qlen=false, int pw=40);
      virtual ~PianorollSettings() {}

      static void readStatic(QDomNode);
      static void writeStatic(Xml& xml);
      int quantStrength() const { return _quantStrength; }
      void setQuantStrength(int c) { _quantStrength = c; }
      int quantLimit() const { return _quantLimit; }
      void setQuantLimit(int q) { _quantLimit = q; }
      int colorMode() const { return _colorMode; }
      void setColorMode(int c) { _colorMode = c; }
      int applyTo() const { return _applyTo; }
      void setApplyTo(int a) { _applyTo = a; }
      bool quantLen() const { return _quantLen; }
      void setQuantLen(bool b) { _quantLen = b; }
      int pianoWidth() const { return _pianoWidth; }
      void setPianoWidth(int w) { _pianoWidth = w; }

      virtual bool operator==(const EditorSettings& e) const { //öööörkk...
            PianorollSettings& f = (PianorollSettings&) e;
            bool ctrlEditsEqual = true;
            for (int i=0; i<MAXNOOFCTRLEDITSETTINGS; i++) {
                  if (ctrlEdits[i] != f.ctrlEdits[i])
                        ctrlEditsEqual = false;
                  }
            return ((f._raster == _raster) && (f._width == _width) && (f._height == _height)// && (f._x == _x) && (f._y == _y)
                  && (f._xmag == _xmag)
                  && (f._ymag == _ymag)
                  && (f._pos == _pos)
                  && (f._quant == _quant) && (f._applyTo == _applyTo)
                  && (ctrlEditsEqual)
                  && (f._quantStrength == _quantStrength) && (f._quantLimit == _quantLimit) && (f._colorMode == _colorMode)
                  && (f._quantLen == _quantLen) && (f._pianoWidth == _pianoWidth));
            }
      virtual bool operator!=(const EditorSettings& e) const { PianorollSettings& f = (PianorollSettings&) e; return !(f==*this); }
      virtual void readStatus(QDomNode);
      virtual void writeStatus(Xml&) const;
      virtual EditorSettings* clone();
      virtual void dump();

      static void setStaticInitValues(int widthinit, int heightinit, int rasterinit, int quantinit);
      };

//---------------------------------------------------------
//   SettingsList
//
// Multimap containing a list of editor settings
// There is one big list with settings for all editors and
// different parts. When opening an editor for a specific
// part, the part-id is matched against the correct editortype
// If one exists, the editor gets when calling getSettings(),
// otherwise, a new one is created.
// The setting has a final update on destruction, and if it
// has changed, it is stored to the settingslist
//---------------------------------------------------------
typedef std::multimap<unsigned, EditorSettings*>::iterator iSettingsList;
typedef std::multimap<unsigned, EditorSettings*>::const_iterator ciSettingsList;

class SettingsList : public std::multimap<unsigned, EditorSettings*>
      {
   public:
      SettingsList();
      ~SettingsList();
      void set(unsigned id, EditorSettings* e);
      EditorSettings* get(unsigned);
      void removeSettings(unsigned id);
      void readElem(QDomNode);
      void readStatus(QDomNode);
      void writeStatus(Xml&) const;
      void dump();
    void reset();
      };

#endif
