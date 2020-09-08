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

#include "sig.h"

#include "markerview.h"
#include "xml.h"
#include "globals.h"
#include "sync.h"
#include "icons.h"
#include "song.h"
#include "posedit.h"
#include "audio.h"
#include "gconfig.h"
#include "pos.h"

#include <cstdint>

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
#include <QTreeWidget>

namespace MusEGui {

enum { COL_TICK = 0, COL_SMPTE, COL_LOCK, COL_NAME };

//---------------------------------------------------------
//   tick
//---------------------------------------------------------

unsigned MarkerItem::tick() const
      {
      return _marker.tick();
      }

//---------------------------------------------------------
//   name
//---------------------------------------------------------

const QString MarkerItem::name() const
      {
      return _marker.name();
      }

//---------------------------------------------------------
//   lock
//---------------------------------------------------------

bool MarkerItem::lock() const
      {
      return _marker.type() == MusECore::Pos::FRAMES;
      }

MusECore::Marker MarkerItem::marker() const { return _marker; }

void MarkerItem::setMarker(const MusECore::Marker& m)
{
      _marker = m;
      setText(COL_NAME, m.name());
      setPos(m);
      setLock(m.type() == MusECore::Pos::FRAMES);
  
}

//---------------------------------------------------------
//   MarkerItem
//---------------------------------------------------------

MarkerItem::MarkerItem(QTreeWidget* parent, const MusECore::Marker& m)
  : QTreeWidgetItem(parent)
      {
      setMarker(m);
      }

MarkerItem::MarkerItem(const MusECore::Marker& m)
  : QTreeWidgetItem()
      {
      setMarker(m);
      }

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void MarkerItem::setName(const QString& s)
      {
      setText(COL_NAME, s);
      }

//---------------------------------------------------------
//   setLock
//---------------------------------------------------------

void MarkerItem::setLock(bool lck)
      {
      setIcon(COL_LOCK, lck ? QIcon(*lockIcon) : QIcon());
      }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void MarkerItem::setPos(const MusECore::Pos& v)
      {
      QString s;
      int bar, beat;
      unsigned tick;
      MusEGlobal::sigmap.tickValues(v.tick(), &bar, &beat, &tick);
      s = QString("%1.%2.%3")
          .arg(bar + 1,      4, 10, QLatin1Char('0'))
          .arg(beat + 1,     2, 10, QLatin1Char('0'))
          .arg(tick,         3, 10, QLatin1Char('0'));
      setText(COL_TICK, s);

      int hour, min, sec, frame, subframe;
      v.msf(&hour, &min, &sec, &frame, &subframe);
      
      s = QString("%1:%2:%3:%4:%5")
          .arg(hour,     2, 10, QLatin1Char('0'))
          .arg(min,      2, 10, QLatin1Char('0'))
          .arg(sec,      2, 10, QLatin1Char('0'))
          .arg(frame,    2, 10, QLatin1Char('0'))
          .arg(subframe, 2, 10, QLatin1Char('0'));
      setText(COL_SMPTE, s);
      }


//---------------------------------------------------------
//   MarkerView
//---------------------------------------------------------

MarkerView::MarkerView(QWidget* parent)
   : QWidget(parent)
      {
      setObjectName("MarkerView");

      QAction* markerAdd = new QAction(QIcon(*flagIcon), tr("Add marker"), this);
      connect(markerAdd, SIGNAL(triggered()), SLOT(addMarker()));

      QAction* markerDelete = new QAction(QIcon(*deleteIcon), tr("Delete marker"), this);
      connect(markerDelete, SIGNAL(triggered()), SLOT(deleteMarker()));    
      
      // Toolbars ---------------------------------------------------------
      QToolBar* edit = new QToolBar(tr("Edit tools"));
      edit->setIconSize(QSize(MusEGlobal::config.iconSize, MusEGlobal::config.iconSize));
      edit->setObjectName("marker edit tools");
      edit->addAction(markerAdd);
      edit->addAction(markerDelete);

      //---------------------------------------------------
      //    master
      //---------------------------------------------------

      QVBoxLayout* vbox = new QVBoxLayout(this);

      table = new QTreeWidget(this);
      table->setAllColumnsShowFocus(true);
      table->setSelectionMode(QAbstractItemView::SingleSelection);

      table->setIndentation(2);
      
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
      editTick = new PosEdit;
      editTick->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,
         QSizePolicy::Fixed));

      editSMPTE = new PosEdit;
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

      connect(editName, SIGNAL(editingFinished()),
         SLOT(nameChanged()));
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

      vbox->addWidget(edit);
      vbox->addWidget(table);
      vbox->addWidget(props);

      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
      
      updateList();

      // work around for probable QT/WM interaction bug.
      // for certain window managers, e.g xfce, this window is
      // is displayed although not specifically set to show();
      // bug: 2811156  	 Softsynth GUI unclosable with XFCE4 (and a few others)
      // Nov 21, 2012 Hey this causes the thing not to open at all, EVER, on Lubuntu and some others!
      // And we had a request to remove this from a knowledgable tester. REMOVE Tim.
      ///show();
      ///hide();
      }

//---------------------------------------------------------
//   MarkerView
//---------------------------------------------------------

MarkerView::~MarkerView()
      {
      }

//---------------------------------------------------------
//   addMarker
//---------------------------------------------------------
void MarkerView::addMarker()
      {
      MusEGlobal::song->addMarker(QString(""), MusEGlobal::song->cpos(), false);
      }
void MarkerView::addMarker(unsigned i)
      {
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

void MarkerView::songChanged(MusECore::SongChangedStruct_t flags)
{
  // Try to minimize the work. Rebuild the list only if required.
  // The simpler updateList() just cannot handle this complex situation,
  //  due to position comparisons ALREADY being equal etc.
  if(flags &
    (SC_MARKERS_REBUILT | SC_TEMPO | SC_MASTER
     | SC_SIG // Required so that bar/beat/tick of listed items are shown correctly.
    ))
    rebuildList();
  // Can we get away with just an update?
  else if(flags & (SC_MARKER_INSERTED | SC_MARKER_REMOVED | SC_MARKER_MODIFIED))
    updateList();
}

//---------------------------------------------------------
//   rebuildList
//---------------------------------------------------------

void MarkerView::rebuildList()
{
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      const MarkerItem* selitem = (MarkerItem*)table->currentItem();
      MusECore::EventID_t selitem_id = -1;
      if(selitem)
        selitem_id = selitem->marker().id();
      int m_id;

      // Block signals added. Triggers itemSelectionChanged() causing crash. Tim. 
      table->blockSignals(true);
      table->clear();
      table->blockSignals(false);

      for (MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) 
      {
        const MusECore::Marker& m = i->second;
        m_id = m.id();
        MarkerItem* new_item = new MarkerItem(m);
        table->blockSignals(true);
        table->addTopLevelItem(new_item);
        if(m_id == selitem_id)
        {
          //m->setCurrent(true);
          table->setCurrentItem(new_item);
        }
        //else  
        //{
        //  m->setCurrent(false);
        //}
        table->blockSignals(false);
      }

      markerSelectionChanged();
}

//---------------------------------------------------------
//   updateList
//---------------------------------------------------------

void MarkerView::updateList()
{
      // Manage selected item, due to clearing of table...
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      const MarkerItem* selitem = (MarkerItem*)table->currentItem();
      MusECore::EventID_t selitem_id = -1;
      MarkerItem* new_selitem = nullptr;
      if(selitem)
        selitem_id = selitem->marker().id();

      int m_id, mitem_id;
      MarkerItem* mitem;
      unsigned m_frame, mitem_frame;

      // Look for removed markers before added markers...
      for(int mitem_idx = 0; ; )
      {
        mitem = (MarkerItem*)table->topLevelItem(mitem_idx);
        if(!mitem)
          break;

        MusECore::ciMarker im = marker->findId(mitem->marker().id());
        if(im != marker->cend())
        {
          ++mitem_idx;
          continue;
        }
        
        delete mitem;
      }

      MarkerItem* found_item;
      int found_item_idx;
      int insert_idx;
      bool prev_frame_found;
      bool next_frame_found;
      unsigned prev_frame;
      unsigned next_frame;

      // Look for added markers...
      for(MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) 
      {
        const MusECore::Marker& m = i->second;
        m_id = m.id();
        m_frame = m.frame();

        found_item = nullptr;
        found_item_idx = -1;
        insert_idx = -1;
        prev_frame_found = false;
        next_frame_found = false;
        prev_frame = 0;
        next_frame = 0;
        for(int mitem_idx = 0; ; ++mitem_idx)
        {
          mitem = (MarkerItem*)table->topLevelItem(mitem_idx);
          if(!mitem)
            break;

          // If all four pieces of information have been found, we're done.
          if(found_item && insert_idx != -1 && prev_frame_found && next_frame_found)
            break;

          const MusECore::Marker& mm = mitem->marker();
          mitem_id = mm.id();
          mitem_frame = mm.frame();

          if(insert_idx == -1 && m_frame < mitem_frame)
            insert_idx = mitem_idx;

          if(m_id == mitem_id)
          {
            found_item = mitem;
            found_item_idx = mitem_idx;
            prev_frame_found = true;
          }
          else
          {
            if(!prev_frame_found)
              prev_frame = mitem_frame;

            if(found_item && !next_frame_found)
            {
              next_frame = mitem_frame;
              next_frame_found = true;
              // All four pieces of information have been found, we're done.
              //break;
            }
          }
        }

        // Did we find an item matching the marker ID?
        if(found_item)
        {
          const MusECore::Marker& found_m = found_item->marker();
          const unsigned found_f = found_m.frame();
          // Can we get away with simply modifying the existing item's marker properties with the new ones?
          // That means is the new requested position CLOSER than a previous (or next) item's marker positon?
          if(m_frame == found_f ||
             (m_frame < found_f && (!prev_frame_found || m_frame >= prev_frame)) ||
             (m_frame > found_f && (!next_frame_found || m_frame <= next_frame)))
          {
            found_item->setMarker(m);
            if(m_id == selitem_id)
              new_selitem = found_item;
            continue;
          }

          // The new requested position is not CLOSER than a previous (or next) item's marker positon.
          // We must delete the existing item, and re-add it at the new position.
          table->blockSignals(true);
          delete found_item;
          table->blockSignals(false);
          if(found_item_idx <= insert_idx)
          {
            if(insert_idx > 0)
              --insert_idx;
          }
        }

        // Add a new item, based on the given marker m.
        MarkerItem* new_item = new MarkerItem(m);
        table->blockSignals(true);
        if(insert_idx != -1)
          table->insertTopLevelItem(insert_idx, new_item);
        else
          table->addTopLevelItem(new_item);
        table->blockSignals(false);

        if(m_id == selitem_id)
          new_selitem = new_item;
      }

      if(new_selitem)
      {
        table->blockSignals(true);
        table->setCurrentItem(new_selitem);
        table->blockSignals(false);
      }

      markerSelectionChanged();
}

//---------------------------------------------------------
//   markerSelectionChanged
//---------------------------------------------------------

void MarkerView::markerSelectionChanged()
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      editTick->blockSignals(true);
      editSMPTE->blockSignals(true);
      editName->blockSignals(true);
      lock->blockSignals(true);
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
            editTick->setValue(item->marker());
            editSMPTE->setValue(item->marker());
            editName->setText(item->name());
            editName->setEnabled(true);
            lock->setChecked(item->lock());
            lock->setEnabled(true);
            
            editSMPTE->setEnabled(item->lock());
            editTick->setEnabled(!item->lock());
            }
      editTick->blockSignals(false);
      editSMPTE->blockSignals(false);
      editName->blockSignals(false);
      lock->blockSignals(false);
      }

void MarkerView::clicked(QTreeWidgetItem* i)
      {
      MarkerItem* item = (MarkerItem*)i;
      if (item == 0) {
            table->clearSelection();
            return;
            }
// Hm I don't like the idea of letting song handle this. I feel that song->setPos()
//  should only be called as a RESULT of the transport repositioning, which it ALREADY will.
      MusEGlobal::song->setPos(MusECore::Song::CPOS, item->marker(), true, true, false);
//       MusEGlobal::audio->msgSeek(item->marker());
      }

//---------------------------------------------------------
//   nameChanged
//---------------------------------------------------------

void MarkerView::nameChanged()
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item)
      {
        const QString txt = editName->text();
        // Avoid multiple identical changes from return pressed and focus lost etc.
        if(item->marker().name() != txt)
          MusEGlobal::song->setMarkerName(item->marker(), txt);
      }
      }

//---------------------------------------------------------
//   tickChanged
//---------------------------------------------------------

void MarkerView::tickChanged(const MusECore::Pos& pos)
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item) {
              MusEGlobal::song->setMarkerPos(item->marker(), pos);
            }
      }

//---------------------------------------------------------
//   lockChanged
//---------------------------------------------------------

void MarkerView::lockChanged(bool lck)
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item) {
            MusEGlobal::song->setMarkerLock(item->marker(), lck);
            }
      }

//---------------------------------------------------------
//   markerChanged
//    select appropriate Marker
//---------------------------------------------------------

void MarkerView::markerChanged(int val)
{
      switch(val)
      {
         // MARKER_CUR, MARKER_ADD, MARKER_REMOVE, MARKER_NAME,
         // MARKER_TICK, MARKER_LOCK
        case MusECore::Song::MARKER_CUR:
        {
          
          MusECore::MarkerList* marker = MusEGlobal::song->marker();
          for (MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) {
                if (i->second.current()) {
                      MarkerItem* item = (MarkerItem*)table->topLevelItem(0);
                      while (item) {
                            if (item->marker().id() == i->second.id()) {
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
      MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, false);
        
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
      MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, false);
      }

MarkerItem* MarkerView::findId(MusECore::EventID_t id) const
{
  MarkerItem* item = (MarkerItem*)table->topLevelItem(0);
  while(item) 
  {
    if(item->marker().id() == id)
      return item;
    item = (MarkerItem*)table->itemBelow(item);
  }
  return nullptr;
}

} // namespace MusEGui
