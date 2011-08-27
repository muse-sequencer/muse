//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: midieditor.cpp,v 1.2.2.2 2009/02/02 21:38:00 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include "midieditor.h"
#include "midiedit/ecanvas.h"
#include "waveedit/waveview.h"
#include "scrollscale.h"
#include "mtscale.h"
#include "xml.h"
#include "part.h"
#include "track.h"
#include "song.h"

#include <QRect>
#include <QColor>
#include <QGridLayout>

//---------------------------------------------------------
//   MidiEditor
//---------------------------------------------------------

MidiEditor::MidiEditor(int r, PartList* pl,
   QWidget* parent, const char* name) : TopWin(parent, name)
      {
      setAttribute(Qt::WA_DeleteOnClose);
      _pl = pl;
      if (_pl)
            for (iPart i = _pl->begin(); i != _pl->end(); ++i)
                  _parts.push_back(i->second->sn());
      _raster  = r;
      canvas   = 0;
      wview    = 0;
      _curDrumInstrument = -1;
      mainw    = new QWidget(this);
      
      ///mainGrid = new QGridLayout(mainw);
      mainGrid = new QGridLayout();
      mainw->setLayout(mainGrid);
      
      mainGrid->setContentsMargins(0, 0, 0, 0);
      mainGrid->setSpacing(0);  
      setCentralWidget(mainw);
      }

//---------------------------------------------------------
//   genPartlist
//---------------------------------------------------------

void MidiEditor::genPartlist()
      {
      _pl->clear();
      for (std::list<int>::iterator i = _parts.begin(); i != _parts.end(); ++i) {
            TrackList* tl = song->tracks();
            for (iTrack it = tl->begin(); it != tl->end(); ++it) {
                  PartList* pl = (*it)->parts();
                  iPart ip;
                  for (ip = pl->begin(); ip != pl->end(); ++ip) {
                        if (ip->second->sn() == *i) {
                              _pl->add(ip->second);
                              break;
                              }
                        }
                  if (ip != pl->end())
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   MidiEditor
//---------------------------------------------------------

MidiEditor::~MidiEditor()
      {
      if (_pl)
            delete _pl;
      }


//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void MidiEditor::readStatus(Xml& xml)
      {
      if (_pl == 0)
            _pl = new PartList;

      for (;;) {
            Xml::Token token = xml.parse();
            QString tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "raster")
                              _raster = xml.parseInt();
                        else if (tag == "topwin")
                              TopWin::readStatus(xml);
                        else
                              xml.unknown("MidiEditor");
                        break;
                  case Xml::TagEnd:
                        if (tag == "midieditor")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writePartList
//---------------------------------------------------------

void MidiEditor::writePartList(int level, Xml& xml) const
      {
      for (ciPart p = _pl->begin(); p != _pl->end(); ++p) {
            Part* part   = p->second;
            Track* track = part->track();
            int trkIdx   = song->tracks()->index(track);
            int partIdx  = track->parts()->index(part);
            
            if((trkIdx == -1) || (partIdx == -1))
              printf("MidiEditor::writePartList error: trkIdx:%d partIdx:%d\n", trkIdx, partIdx);
              
            xml.put(level, "<part>%d:%d</part>", trkIdx, partIdx);
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void MidiEditor::writeStatus(int level, Xml& xml) const
      {
      xml.tag(level++, "midieditor");
      TopWin::writeStatus(level, xml);
      xml.intTag(level, "raster", _raster);
      xml.tag(level, "/midieditor");
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiEditor::songChanged(int type)
      {
      
      if (type) {
            if (type & (SC_PART_REMOVED | SC_PART_MODIFIED
               | SC_PART_INSERTED | SC_TRACK_REMOVED)) {
                  genPartlist();
                  // close window if editor has no parts anymore
                  if (parts()->empty()) {
                        close();
                        return;
                        }
                  }
            if (canvas)
                  canvas->songChanged(type);
            else if (wview)
                  wview->songChanged(type);

            if (type & (SC_PART_REMOVED | SC_PART_MODIFIED
               | SC_PART_INSERTED | SC_TRACK_REMOVED)) {
                  
                    updateHScrollRange();
                  if (canvas)
                        setWindowTitle(canvas->getCaption());
                  else if (wview)
                        setWindowTitle(wview->getCaption());
                  if (type & SC_SIG)
                        time->update();
                        
              }        
            }
      }

//---------------------------------------------------------
//   setCurDrumInstrument
//---------------------------------------------------------

void MidiEditor::setCurDrumInstrument(int instr)
      {
      _curDrumInstrument = instr;
      emit curDrumInstrumentChanged(_curDrumInstrument);
      }

//---------------------------------------------------------
//   curCanvasPart
//---------------------------------------------------------

Part* MidiEditor::curCanvasPart() 
{ 
  if(canvas) 
    return canvas->part(); 
  else 
    return 0; 
}

//---------------------------------------------------------
//   curWavePart
//---------------------------------------------------------

WavePart* MidiEditor::curWavePart() 
{ 
  if(wview) 
    return wview->part(); 
  else 
    return 0; 
}

//---------------------------------------------------------
//   setCurCanvasPart
//---------------------------------------------------------

void MidiEditor::setCurCanvasPart(Part* part) 
{ 
  if(canvas) 
    canvas->setCurrentPart(part); 
}

void MidiEditor::horizontalZoomIn()
{
  int mag = hscroll->mag();
  int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
  if (zoomlvl < 23)
        zoomlvl++;

  int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);

  hscroll->setMag(newmag);

}

void MidiEditor::horizontalZoomOut()
{
  int mag = hscroll->mag();
  int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
  if (zoomlvl > 1)
        zoomlvl--;

  int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);

  hscroll->setMag(newmag);

}
