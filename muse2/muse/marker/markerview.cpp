//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: markerview.cpp,v 1.7.2.6 2009/08/25 20:28:45 spamatica Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#include "al/sig.h"  // Tim.

#include "marker.h"
#include "markerview.h"
#include "xml.h"
#include "globals.h"
#include "app.h"
#include "sync.h"
#include "icons.h"
#include "song.h"
#include "awl/posedit.h"

#include <QCloseEvent>
#include <QGroupBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QHBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace MusEGui {

enum { COL_TICK = 0, COL_SMPTE, COL_LOCK, COL_NAME };

//---------------------------------------------------------
//   tick
//---------------------------------------------------------

unsigned MarkerItem::tick() const
      {
      return _marker->tick();
      }

//---------------------------------------------------------
//   name
//---------------------------------------------------------

const QString MarkerItem::name() const
      {
      return _marker->name();
      }

//---------------------------------------------------------
//   lock
//---------------------------------------------------------

bool MarkerItem::lock() const
      {
      return _marker->type() == MusECore::Pos::FRAMES;
      }

//---------------------------------------------------------
//   MarkerItem
//---------------------------------------------------------

MarkerItem::MarkerItem(QTreeWidget* parent, MusECore::Marker* m)
  : QTreeWidgetItem(parent)
      {
      _marker = m;
      setText(COL_NAME, m->name());
      setTick(m->tick());
      if (m->type() == MusECore::Pos::FRAMES)
            setIcon(COL_LOCK, QIcon(*lockIcon));
      setLock(m->type() == MusECore::Pos::FRAMES);
      }

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void MarkerItem::setName(const QString& s)
      {
      setText(COL_NAME, s);
      _marker = MusEGlobal::song->setMarkerName(_marker, s);
      }

//---------------------------------------------------------
//   setLock
//---------------------------------------------------------

void MarkerItem::setLock(bool lck)
      {
      setIcon(COL_LOCK, QIcon(lck ? *lockIcon : 0));
      _marker = MusEGlobal::song->setMarkerLock(_marker, lck);
      }

//---------------------------------------------------------
//   setTick
//---------------------------------------------------------

void MarkerItem::setTick(unsigned v)
      {
      if (_marker->tick() != v)
            _marker = MusEGlobal::song->setMarkerTick(_marker, v);
      QString s;
      int bar, beat;
      unsigned tick;
      AL::sigmap.tickValues(v, &bar, &beat, &tick);
      s.sprintf("%04d.%02d.%03d", bar+1, beat+1, tick);
      setText(COL_TICK, s);

      double time = double(MusEGlobal::tempomap.tick2frame(v))/double(MusEGlobal::sampleRate);
      int hour = int(time) / 3600;
      int min  = (int(time) % 3600)/60;
      int sec  = int(time) % 60;
      double rest = time - (hour*3600 + min * 60 + sec);
      switch(MusEGlobal::mtcType) {
            case 0:     // 24 frames sec
                  rest *= 24;
                  break;
            case 1:     // 25
                  rest *= 25;
                  break;
            case 2:     // 30 drop frame
                  rest *= 30;
                  break;
            case 3:     // 30 non drop frame
                  rest *= 30;
                  break;
            }
      int frame = int(rest);
      int subframe = int((rest-frame)*100);
      s.sprintf("%02d:%02d:%02d:%02d:%02d",
         hour, min, sec, frame, subframe);
      setText(COL_SMPTE, s);
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MarkerView::closeEvent(QCloseEvent* e)
      {
      emit isDeleting(static_cast<TopWin*>(this));
      emit closed();
      e->accept();
      }

//---------------------------------------------------------
//   MarkerView
//---------------------------------------------------------

MarkerView::MarkerView(QWidget* parent)
   : TopWin(TopWin::MARKER, parent, "markerview", Qt::Window)
      {
      setWindowTitle(tr("MusE: Marker"));

      QAction* markerAdd = new QAction(QIcon(*flagIcon), tr("add marker"), this);
      connect(markerAdd, SIGNAL(activated()), SLOT(addMarker()));

      QAction* markerDelete = new QAction(QIcon(*deleteIcon), tr("delete marker"), this);
      connect(markerDelete, SIGNAL(activated()), SLOT(deleteMarker()));

      //---------Pulldown Menu----------------------------
      QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
      
      editMenu->addAction(markerAdd);
      editMenu->addAction(markerDelete);
      
      
      QMenu* settingsMenu = menuBar()->addMenu(tr("Window &Config"));
      settingsMenu->addAction(subwinAction);
      settingsMenu->addAction(shareAction);
      settingsMenu->addAction(fullscreenAction);
      
      
      // Toolbars ---------------------------------------------------------
      QToolBar* edit = addToolBar(tr("edit tools"));
      edit->setObjectName("marker edit tools");
      edit->addAction(markerAdd);
      edit->addAction(markerDelete);

      //---------------------------------------------------
      //    master
      //---------------------------------------------------

      QWidget* w = new QWidget;
      setCentralWidget(w);
      QVBoxLayout* vbox = new QVBoxLayout(w);

      table = new QTreeWidget(w);
      table->setAllColumnsShowFocus(true);
      table->setSelectionMode(QAbstractItemView::SingleSelection);
      
      QStringList columnnames;
      columnnames << tr("Bar:Beat:Tick")
		  << tr("Hr:Mn:Sc:Fr:Sf")
		  << tr("Lock")
		  << tr("Text");

      table->setHeaderLabels(columnnames);
      table->setColumnWidth(2, 40);      
      table->header()->setStretchLastSection(true);

      connect(table, SIGNAL(itemSelectionChanged()),
         SLOT(markerSelectionChanged()));
      connect(table, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
         SLOT(clicked(QTreeWidgetItem*)));

      QGroupBox* props = new QGroupBox(tr("Marker Properties"));
      QHBoxLayout *hbox = new QHBoxLayout;

      ///editTick = new PosEdit;
      editTick = new Awl::PosEdit;
      editTick->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,
         QSizePolicy::Fixed));

      editSMPTE = new Awl::PosEdit;
      editSMPTE->setSmpte(true);
      editSMPTE->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,
         QSizePolicy::Fixed));

      lock = new QToolButton;
      lock->setIcon(*lockIcon);
      lock->setCheckable(true);

      editName = new QLineEdit;
      editName->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
					  QSizePolicy::Preferred));

      hbox->addWidget(editTick);
      hbox->addWidget(editSMPTE);
      hbox->addWidget(lock);
      hbox->addWidget(editName);
      props->setLayout(hbox);

      connect(editName, SIGNAL(textChanged(const QString&)),
         SLOT(nameChanged(const QString&)));
      connect(editTick, SIGNAL(valueChanged(const MusECore::Pos&)),
         SLOT(tickChanged(const MusECore::Pos&)));
      connect(editSMPTE, SIGNAL(valueChanged(const MusECore::Pos&)),
         SLOT(tickChanged(const MusECore::Pos&)));
      connect(editSMPTE, SIGNAL(valueChanged(const MusECore::Pos&)),
         editTick, SLOT(setValue(const MusECore::Pos&)));
      connect(editTick, SIGNAL(valueChanged(const MusECore::Pos&)),
         editSMPTE, SLOT(setValue(const MusECore::Pos&)));
      connect(lock, SIGNAL(toggled(bool)),
         SLOT(lockChanged(bool)));
      connect(MusEGlobal::song, SIGNAL(markerChanged(int)),
         SLOT(markerChanged(int)));

      vbox->addWidget(table);
      vbox->addWidget(props);

      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      
      updateList();

      finalizeInit();

      // work around for probable QT/WM interaction bug.
      // for certain window managers, e.g xfce, this window is
      // is displayed although not specifically set to show();
      // bug: 2811156  	 Softsynth GUI unclosable with XFCE4 (and a few others)
      show();
      hide();
      }

//---------------------------------------------------------
//   MarkerView
//---------------------------------------------------------

MarkerView::~MarkerView()
      {
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void MarkerView::readStatus(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
                  break;
            switch (token) {
                  case MusECore::Xml::TagStart:
                        if (tag=="topwin")
                            TopWin::readStatus(xml);
                        else
                            xml.unknown("Marker");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "marker")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void MarkerView::writeStatus(int level, MusECore::Xml& xml) const
      {
      xml.tag(level++, "marker");
      TopWin::writeStatus(level, xml);
      xml.tag(level, "/marker");
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void MarkerView::readConfiguration(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "topwin")
                              TopWin::readConfiguration(MARKER, xml);
                        else
                              xml.unknown("MarkerView");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "marker")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void MarkerView::writeConfiguration(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "marker");
      TopWin::writeConfiguration(MARKER, level, xml);
      xml.tag(level, "/marker");
      }

//---------------------------------------------------------
//   addMarker
//---------------------------------------------------------
void MarkerView::addMarker()
      {
      addMarker(-1);
      }
void MarkerView::addMarker(int i)
      {
      if( i==-1 ) i = MusEGlobal::song->cpos();
      
      // Changed p3.3.43 Let MusECore::Song::addMarker emit markerChanged(MARKER_ADD)
      //  and handle it in MarkerView::markerChanged(int)
      //MusECore::Marker* m = MusEGlobal::song->addMarker(QString(""), i, false);
      //MarkerItem* newItem = new MarkerItem(table, m);
      //table->setSelected(newItem, true);
      //
      MusEGlobal::song->addMarker(QString(""), i, false);
      }

//---------------------------------------------------------
//   deleteMarker
//---------------------------------------------------------

void MarkerView::deleteMarker()
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item) {
            table->blockSignals(true);
            MusEGlobal::song->removeMarker(item->marker());
            table->blockSignals(false);
            // Removed p3.3.43 Let MusECore::Song::removeMarker emit markerChanged(MARKER_REMOVE)
            //  and handle it in MarkerView::markerChanged(int)
            //delete item;
            }
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MarkerView::songChanged(int flags)
{
  // Is it simply a midi controller value adjustment? Forget it.
  if(flags == SC_MIDI_CONTROLLER)
    return;
    
  updateList();
}

//---------------------------------------------------------
//   updateList
//---------------------------------------------------------

void MarkerView::updateList()
{
      // Added p3.3.43 Manage selected item, due to clearing of table...
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      MarkerItem* selitem     = (MarkerItem*)table->currentItem();
      MusECore::Marker* selm     = selitem ? selitem->marker() : 0;
      // p3.3.44 Look for removed markers before added markers...
      if(selitem)
      {
        MarkerItem* mitem = (MarkerItem*)table->topLevelItem(0);
        while(mitem) 
        {
          bool found = false;
          for(MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) 
          {
            MusECore::Marker* m = &i->second;
            if(m == mitem->marker()) 
            {
              found = true;
              break;
            }
          }
          // Anything removed from the marker list?
          if(!found)
          {
            // If it is the current selected item, it no longer exists. Make the next item be selected.
            if(mitem == selitem)
            {
              MarkerItem* mi = (MarkerItem*)table->itemBelow(selitem);
              if(mi)
              {
                selitem = mi;
                selm    = selitem->marker();
              }  
            }  
          }  
          mitem = (MarkerItem*)table->itemBelow(mitem);
        }
      }  
      // Look for added markers...
      for(MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) 
      {
        MusECore::Marker* m = &i->second;
        bool found = false;
        MarkerItem* item = (MarkerItem*)table->topLevelItem(0);
        while(item) 
        {
          if(item->marker() == m) 
          {
            found = true;
            break;
          }
          item = (MarkerItem*)table->itemBelow(item);
        }
        // Anything new found in the marker list?
        if(!found)
          selm = m;
      }
            
      // Block signals added. Triggers itemSelectionChanged() causing crash. Tim. 
      table->blockSignals(true);
      table->clear();
      table->blockSignals(false);
      
      //MusECore::MarkerList* marker = MusEGlobal::song->marker();
      for (MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) 
      {
            MusECore::Marker* m = &i->second;
            
            // Changed p3.3.43 
            MarkerItem* item = new MarkerItem(table, m);
            if(m == selm)
            {
              m->setCurrent(true);
              table->setCurrentItem(item);
            }
            else  
            {
              m->setCurrent(false);
            }
      }
}

//---------------------------------------------------------
//   markerSelected
//---------------------------------------------------------

void MarkerView::markerSelectionChanged()
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item == 0) {  // never triggered
            editTick->setValue(0);
            editSMPTE->setValue(0);
            editName->setText(QString(""));
            lock->setChecked(false);
            editSMPTE->setEnabled(false);
            editTick->setEnabled(false);
            lock->setEnabled(false);
            editName->setEnabled(false);
            }
      else {
            editTick->setValue(item->tick());
            editSMPTE->setValue(item->tick());
            editName->setText(item->name());
            editName->setEnabled(true);
            lock->setChecked(item->lock());
            lock->setEnabled(true);
            
            editSMPTE->setEnabled(item->lock());
            editTick->setEnabled(!item->lock());
            }
      }

void MarkerView::clicked(QTreeWidgetItem* i)
      {
      MarkerItem* item = (MarkerItem*)i;
      if (item == 0) {
            table->clearSelection();
            return;
            }
      MusECore::Pos p(item->tick(), true);
      MusEGlobal::song->setPos(0, p, true, true, false);
      }

//---------------------------------------------------------
//   nameChanged
//---------------------------------------------------------

void MarkerView::nameChanged(const QString& s)
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item)
            item->setName(s);
      }

//---------------------------------------------------------
//   tickChanged
//---------------------------------------------------------

void MarkerView::tickChanged(const MusECore::Pos& pos)
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item) {
            item->setTick(pos.tick());
            MusECore::Pos p(pos.tick(), true);
            MusEGlobal::song->setPos(0, p, true, true, false);
            table->sortByColumn(COL_TICK, Qt::AscendingOrder);
            }
      }

//---------------------------------------------------------
//   lockChanged
//---------------------------------------------------------

void MarkerView::lockChanged(bool lck)
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item) {
            item->setLock(lck);
            editSMPTE->setEnabled(item->lock());
            editTick->setEnabled(!item->lock());
            }
      }

//---------------------------------------------------------
//   posChanged
//    select appropriate Marker
//---------------------------------------------------------

void MarkerView::markerChanged(int val)
{
      switch(val)
      {
         // MARKER_CUR, MARKER_ADD, MARKER_REMOVE, MARKER_NAME,
         // MARKER_TICK, MARKER_LOCK
        case MusECore::Song::MARKER_ADD:
        case MusECore::Song::MARKER_REMOVE:
          updateList();      
        break; // Try falling through and let it try to select something. No, let updateList() do it...
        
        case MusECore::Song::MARKER_CUR:
        {
          
          MusECore::MarkerList* marker = MusEGlobal::song->marker();
          for (MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) {
                if (i->second.current()) {
                      MarkerItem* item = (MarkerItem*)table->topLevelItem(0);
                      while (item) {
                            if (item->marker() == &i->second) {
                                  table->setCurrentItem(item);
                                  return;
                                  }
                            item = (MarkerItem*)table->itemBelow(item);
                            }
                      }
                }
        }
        break;
        
        default:
        break;        
      }  
}

void MarkerView::nextMarker()
      {
      unsigned int curPos = MusEGlobal::song->cpos();//prevent compiler warning: comparison of sigend/unsigned
      unsigned int nextPos = 0xFFFFFFFF;
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      for (MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) {
            if (i->second.tick() > curPos && i->second.tick() < nextPos)
              nextPos = i->second.tick();
            }
      if (nextPos == 0xFFFFFFFF)
          return;
      MusECore::Pos p(nextPos, true);
      MusEGlobal::song->setPos(0, p, true, true, false);
        
      }
void MarkerView::prevMarker()
      {
      unsigned int curPos = MusEGlobal::song->cpos();//prevent compiler warning: comparison of sigend/unsigned
      unsigned int nextPos = 0;
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      for (MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) {
            if (i->second.tick() < curPos && i->second.tick() > nextPos)
              nextPos = i->second.tick();
            }

      MusECore::Pos p(nextPos, true);
      MusEGlobal::song->setPos(0, p, true, true, false);
      }

} // namespace MusEGui
