//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: midieditor.cpp,v 1.2.2.2 2009/02/02 21:38:00 terminator356 Exp $
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

#include "midieditor.h"
#include "globals.h"
#include "midiedit/ecanvas.h"
#include "waveedit/waveview.h"
#include "scrollscale.h"
#include "mtscale.h"
#include "xml.h"
#include "part.h"
#include "track.h"
#include "song.h"

#include "trackinfo_layout.h"
#include "icons.h"
#include "astrip.h"
#include "mstrip.h"
#include "gconfig.h"
#include "app.h"
#include "rasterizer.h"

#include <QRect>
#include <QColor>
#include <QGridLayout>
#include <QPainter>
#include <QPixmap>
#include <QList>

// For debugging output: Uncomment the fprintf section.
#define ERROR_MIDIEDITOR(dev, format, args...)  fprintf(dev, format, ##args)
#define DEBUG_MIDIEDITOR(dev, format, args...) // fprintf(dev, format, ##args)

namespace MusEGui {

//---------------------------------------------------------
//   MidiEditor
//---------------------------------------------------------

MidiEditor::MidiEditor(ToplevelType t, int r, MusECore::PartList* pl,
   QWidget* parent, const char* name) : TopWin(t, parent, name)
      {
      _pl = pl;
      if (_pl)
            for (MusECore::iPart i = _pl->begin(); i != _pl->end(); ++i)
                  _parts.insert(i->second->sn());

      QList<Rasterizer::Column> rast_cols;
      rast_cols << 
        Rasterizer::TripletColumn <<
        Rasterizer::NormalColumn <<
        Rasterizer::DottedColumn;
      _rasterizerModel = new RasterizerModel(
        MusEGlobal::globalRasterizer, this, -1, rast_cols);

      _raster = _rasterizerModel->checkRaster(r);
      _canvasXOrigin = 0;
      _minXMag = -25;
      _maxXMag = 2;

      canvas   = 0;
      
      trackInfoWidget = 0;
      selected = 0;
      
      //wview    = 0;
      _curDrumInstrument = -1;
      mainw    = new QWidget(this);
      
      ///mainGrid = new QGridLayout(mainw);
      mainGrid = new QGridLayout();
      mainw->setLayout(mainGrid);
      
      mainGrid->setContentsMargins(0, 0, 0, 0);
      mainGrid->setSpacing(0);  
      setCentralWidget(mainw);
      
      connect(MusEGlobal::song, SIGNAL(newPartsCreated(const std::map< const MusECore::Part*, std::set<const MusECore::Part*> >&)), SLOT(addNewParts(const std::map< const MusECore::Part*, std::set<const MusECore::Part*> >&)));
      }

int MidiEditor::rasterStep(unsigned tick) const   { return MusEGlobal::sigmap.rasterStep(tick, _raster); }
unsigned MidiEditor::rasterVal(unsigned v)  const { return MusEGlobal::sigmap.raster(v, _raster);  }
unsigned MidiEditor::rasterVal1(unsigned v) const { return MusEGlobal::sigmap.raster1(v, _raster); }
unsigned MidiEditor::rasterVal2(unsigned v) const { return MusEGlobal::sigmap.raster2(v, _raster); }

//---------------------------------------------------------
//   genPartlist
//---------------------------------------------------------

void MidiEditor::genPartlist()
      {
      if(!_pl)
        return;

      _pl->clear();
      for (std::set<int>::iterator i = _parts.begin(); i != _parts.end(); ++i) {
            MusECore::TrackList* tl = MusEGlobal::song->tracks();
            for (MusECore::iTrack it = tl->begin(); it != tl->end(); ++it) {
                  MusECore::PartList* pl = (*it)->parts();
                  MusECore::iPart ip;
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
//   genTrackInfo
//---------------------------------------------------------

void MidiEditor::genTrackInfo(TrackInfoWidget* trackInfo)
      {
      trackInfo->addWidget(nullptr, 1);
      }

//---------------------------------------------------------
//   switchInfo
//---------------------------------------------------------

void MidiEditor::switchInfo(int n)
      {
      const int idx = 1;
      if(n == idx) {
//             MidiStrip* w = (MidiStrip*)(trackInfoWidget->getWidget(idx));
            Strip* w = (Strip*)(trackInfoWidget->getWidget(idx));
            if (w == 0 || selected != w->getTrack()) {
                  if (w)
                  {
                        //fprintf(stderr, "MidiEditor::switchInfo deleting strip\n");
                        delete w;
                        //w->deleteLater();
                  }
                  if(selected->isMidiTrack())
                    w = new MidiStrip(trackInfoWidget, static_cast <MusECore::MidiTrack*>(selected));
                  else
                    w = new AudioStrip(trackInfoWidget, static_cast <MusECore::AudioTrack*>(selected));
                  // Leave broadcasting changes to other selected tracks off.
                  
                  // Set focus yielding to the canvas.
                  if(MusEGlobal::config.smartFocus)
                  {
                    w->setFocusYieldWidget(canvas);
                    //w->setFocusPolicy(Qt::WheelFocus);
                  }

                  // We must marshall song changed instead of connecting to the strip's song changed
                  //  otherwise it crashes when loading another song because track is no longer valid
                  //  and the strip's songChanged() seems to be called before Pianoroll songChanged()
                  //  gets called and has a chance to stop the crash.
                  //connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), w, SLOT(songChanged(MusECore::SongChangedStruct_t)));
                  
                  connect(MusEGlobal::muse, SIGNAL(configChanged()), w, SLOT(configChanged()));
                  w->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
                  trackInfoWidget->addWidget(w, idx);
                  w->show();
                  //setTabOrder(midiTrackInfo, w);
                  }
            }
      if (trackInfoWidget->curIdx() == n)
            return;
      trackInfoWidget->raiseWidget(n);
      }

//---------------------------------------------------------
//   trackInfoSongChange
//---------------------------------------------------------

void MidiEditor::trackInfoSongChange(MusECore::SongChangedStruct_t flags)
{
  if(!selected)
    return;
  
  Strip* w = static_cast<Strip*>(trackInfoWidget->getWidget(1));
  if(w)
    w->songChanged(flags);
}

//---------------------------------------------------------
//   updateTrackInfo
//---------------------------------------------------------

void MidiEditor::updateTrackInfo()
{
      MusECore::Part* part = curCanvasPart();
      if(part)
        selected = part->track();
      else
        selected = 0;
      
      if (selected == 0) {
            switchInfo(0);
            return;
            }
//       if (selected->isMidiTrack()) 
            switchInfo(1);
}

//---------------------------------------------------------
//   checkTrackInfoTrack
//---------------------------------------------------------

void MidiEditor::checkTrackInfoTrack()
{
  const int idx = 1;
  {
    Strip* w = static_cast<Strip*>(trackInfoWidget->getWidget(idx));
    if(w)
    {
      MusECore::Track* t = w->getTrack();
      if(t)
      {
        MusECore::TrackList* tl = MusEGlobal::song->tracks();
        MusECore::iTrack it = tl->find(t);
        if(it == tl->end())
        {
          delete w;
          trackInfoWidget->addWidget(0, idx);
          selected = 0;
          switchInfo(0);
        } 
      }   
    } 
  }
}
        
//---------------------------------------------------------
//   movePlayPointerToSelectedEvent
//---------------------------------------------------------

void MidiEditor::movePlayPointerToSelectedEvent()
{
    const MusECore::EventList & evl = curCanvasPart()->events();
    int tickPos = -1;
    for (MusECore::ciEvent ev=evl.begin(); ev!=evl.end(); ev++) {
       if (ev->second.selected()) {
        tickPos = ev->second.tick();
        printf("found selected event, moving to pos %d\n", tickPos);
        break;
       }
    }
    if (tickPos > -1)
    {
        MusECore::Pos p0(curCanvasPart()->tick() + tickPos, true);
        MusEGlobal::song->setPos(MusECore::Song::CPOS, p0);
    }
}

//---------------------------------------------------------
//   addPart
//---------------------------------------------------------

void MidiEditor::addPart(MusECore::Part* p)
{
  if(!_pl || !p)
    return;
  _pl->add(p);
  _parts.insert(p->sn());
}

//---------------------------------------------------------
//   itemsAreSelected
//---------------------------------------------------------

bool MidiEditor::itemsAreSelected() const
{
  bool res = false;
  if(canvas && canvas->itemsAreSelected())
    res = true;
  for(ciCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i)
    if((*i)->itemsAreSelected())
      res = true;
  return res;
}

//---------------------------------------------------------
//   tagItems
//---------------------------------------------------------

void MidiEditor::tagItems(MusECore::TagEventList* tag_list, const MusECore::EventTagOptionsStruct& options) const
{
  const bool tagAllItems = options._flags & MusECore::TagAllItems;
  const bool tagAllParts = options._flags & MusECore::TagAllParts;
  const bool range       = options._flags & MusECore::TagRange;
  const MusECore::Pos& p0 = options._p0;
  const MusECore::Pos& p1 = options._p1;
  
  // If tagging all items, don't bother with the controller editors below,
  //  since everything that they could tag will already be tagged.
  if(tagAllItems)
  {
    const MusECore::Part* part;
    MusECore::Pos pos, part_pos, part_endpos;
    if(tagAllParts)
    {
      if(_pl)
      {
        for(MusECore::ciPart ip = _pl->begin(); ip != _pl->end(); ++ip)
        {
          part = ip->second;
          if(range)
          {
            part_pos = *part;
            part_endpos = part->end();
            // Optimize: Is the part within the range?
            // p1 should be considered outside (one past) the very last position in the range.
            if(part_endpos <= p0 || part_pos >= p1)
              continue;
          }
          const MusECore::EventList& el = part->events();
          for(MusECore::ciEvent ie = el.cbegin(); ie != el.cend(); ++ie)
          {
            const MusECore::Event& e = ie->second;
            if(range)
            {
              // Don't forget to add the part's position.
              pos = e.pos() + part_pos;
              // If the event position is before p0, keep looking...
              if(pos < p0)
                continue;
              // If the event position is at or after p1 then we are done.
              // p1 should be considered outside (one past) the very last position in the range.
              if(pos >= p1)
                break;
            }
            tag_list->add(part, e);
          }
        }
      }
    }
    else
    {
      if(canvas && canvas->part())
      {
        part = canvas->part();
        if(range)
        {
          part_pos = *part;
          part_endpos = part->end();
          // Optimize: Is the part within the range?
          // p1 should be considered outside (one past) the very last position in the range.
          if(part_endpos <= p0 || part_pos >= p1)
            return;
        }
        const MusECore::EventList& el = part->events();
        for(MusECore::ciEvent ie = el.cbegin(); ie != el.cend(); ++ie)
        {
          const MusECore::Event& e = ie->second;
          if(range)
          {
            // Don't forget to add the part's position.
            pos = e.pos() + part_pos;
            // If the event position is before p0, keep looking...
            if(pos < p0)
              continue;
            // If the event position is at or after p1 then we are done.
            // p1 should be considered outside (one past) the very last position in the range.
            if(pos >= p1)
              break;
          }
          tag_list->add(part, e);
        }
      }
    }
  }
  else
  {
    MusECore::EventTagOptionsStruct opts = options;
    opts.removeFlags(MusECore::TagAllItems);
    // These two steps use the tagging features to mark the objects (events)
    //  as having been visited already, to avoid duplicates in the list.
    if(canvas)
      canvas->tagItems(tag_list, opts);
    for(ciCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i)
      (*i)->tagItems(tag_list, opts);
  }
}


//---------------------------------------------------------
//   MidiEditor
//---------------------------------------------------------

MidiEditor::~MidiEditor()
      {
      DEBUG_MIDIEDITOR(stderr, "MidiEditor dtor\n");
      if (_pl)
            delete _pl;
      }


//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void MidiEditor::readStatus(MusECore::Xml& xml)
      {
      if (_pl == 0)
            _pl = new MusECore::PartList;

      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            QString tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "raster")
                              _raster = _rasterizerModel->checkRaster(xml.parseInt());
                        else if (tag == "topwin")
                              TopWin::readStatus(xml);
                        else
                              xml.unknown("MidiEditor");
                        break;
                  case MusECore::Xml::TagEnd:
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

void MidiEditor::writePartList(int level, MusECore::Xml& xml) const
      {
      if(!_pl)
        return;
      for (MusECore::ciPart p = _pl->begin(); p != _pl->end(); ++p) {
            MusECore::Part* part   = p->second;
            MusECore::Track* track = part->track();
            int trkIdx   = MusEGlobal::song->tracks()->index(track);
            int partIdx  = track->parts()->index(part);
            
            if((trkIdx == -1) || (partIdx == -1))
              printf("MidiEditor::writePartList error: trkIdx:%d partIdx:%d\n", trkIdx, partIdx);
              
            xml.put(level, "<part>%d:%d</part>", trkIdx, partIdx);
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void MidiEditor::writeStatus(int level, MusECore::Xml& xml) const
      {
      xml.tag(level++, "midieditor");
      TopWin::writeStatus(level, xml);
      xml.intTag(level, "raster", _raster);
      xml.tag(level, "/midieditor");
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiEditor::songChanged(MusECore::SongChangedStruct_t type)
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

            if (type & (SC_PART_REMOVED | SC_PART_MODIFIED
               | SC_PART_INSERTED | SC_TRACK_REMOVED)) {
                  
                  updateHScrollRange();
                  
                  if (canvas)
                        setWindowTitle(canvas->getCaption());
                  if (time && type & SC_SIG)
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

MusECore::Part* MidiEditor::curCanvasPart() 
{ 
  if(canvas) 
    return canvas->part(); 
  else 
    return 0; 
}

//---------------------------------------------------------
//   setCurCanvasPart
//---------------------------------------------------------

void MidiEditor::setCurCanvasPart(MusECore::Part* part) 
{ 
  if(canvas) 
    canvas->setCurrentPart(part); 
}

void MidiEditor::addNewParts(const std::map< const MusECore::Part*, std::set<const MusECore::Part*> >& param)
{
  if(!_pl)
    return;
  
  using std::map;
  using std::set;
  
  for (map< const MusECore::Part*, set<const MusECore::Part*> >::const_iterator it = param.begin(); it!=param.end(); it++)
    if (_pl->index(it->first) != -1)
      for (set<const MusECore::Part*>::const_iterator it2=it->second.begin(); it2!=it->second.end(); it2++)
        addPart(const_cast<MusECore::Part*>(*it2)); // FIXME make this const-correct!
}

void MidiEditor::setHScrollOffset(const int value) {
    int min, max;
    hscroll->range(&min, &max);
    int pos = qMin(qMax(min, value), max);
    hscroll->setOffset(pos);
}

} // namespace MusEGui
