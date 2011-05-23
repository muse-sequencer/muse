//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: midieditor.h,v 1.3.2.2 2009/02/02 21:38:00 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDIEDITOR_H__
#define __MIDIEDITOR_H__

///#include "sig.h"
#include "al/sig.h"
#include "cobject.h"

class QGridLayout;
class QWidget;

class PartList;
class Xml;
class EventCanvas;
class ScrollScale;
class CtrlEdit;
class MTScale;
class WaveView;
class Part;
class WavePart;

//---------------------------------------------------------
//   MidiEditor
//---------------------------------------------------------

class MidiEditor : public TopWin  {
      Q_OBJECT

      PartList* _pl;
      std::list<int> _parts;
      int _curDrumInstrument;  // currently selected instrument if drum
                               // editor
   protected:
      ScrollScale* hscroll;
      ScrollScale* vscroll;
      MTScale* time;
      EventCanvas* canvas;
      WaveView* wview;

      std::list<CtrlEdit*> ctrlEditList;
      int _raster;
      QGridLayout* mainGrid;
      QWidget* mainw;
      virtual void readStatus(Xml&);
      virtual void writeStatus(int, Xml&) const;
      void writePartList(int, Xml&) const;
      void genPartlist();

   public slots:
      void songChanged(int type);
      void setCurDrumInstrument(int instr);
      void horizontalZoomIn();
      void horizontalZoomOut();

      virtual void updateHScrollRange() { };
   signals:
      void curDrumInstrumentChanged(int);

   public:
      MidiEditor(int, PartList*,
         QWidget* parent = 0, const char* name = 0);
      ~MidiEditor();

      ///int rasterStep(unsigned tick) const   { return sigmap.rasterStep(tick, _raster); }
      ///unsigned rasterVal(unsigned v)  const { return sigmap.raster(v, _raster);  }
      ///unsigned rasterVal1(unsigned v) const { return sigmap.raster1(v, _raster); }
      ///unsigned rasterVal2(unsigned v) const { return sigmap.raster2(v, _raster); }
      int rasterStep(unsigned tick) const   { return AL::sigmap.rasterStep(tick, _raster); }
      unsigned rasterVal(unsigned v)  const { return AL::sigmap.raster(v, _raster);  }
      unsigned rasterVal1(unsigned v) const { return AL::sigmap.raster1(v, _raster); }
      unsigned rasterVal2(unsigned v) const { return AL::sigmap.raster2(v, _raster); }
      int raster() const           { return _raster; }
      void setRaster(int val)      { _raster = val; }
      PartList* parts()            { return _pl;  }
      int curDrumInstrument() const  { return _curDrumInstrument; }
      Part* curCanvasPart();
      WavePart* curWavePart();
      void setCurCanvasPart(Part*); 
      };

#endif

