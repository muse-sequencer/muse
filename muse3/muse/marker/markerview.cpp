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

#include "sig.h"  // Tim.

#include "markerview.h"
#include "xml.h"
#include "globals.h"
#include "app.h"
#include "sync.h"
#include "icons.h"
#include "song.h"
#include "posedit.h"

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
// REMOVE Tim. clip. Changed.
//       return _marker->tick();
      return _marker.tick();
      }

//---------------------------------------------------------
//   name
//---------------------------------------------------------

const QString MarkerItem::name() const
      {
// REMOVE Tim. clip. Changed.
//       return _marker->name();
      return _marker.name();
      }

//---------------------------------------------------------
//   lock
//---------------------------------------------------------

bool MarkerItem::lock() const
      {
// REMOVE Tim. clip. Changed.
//       return _marker->type() == MusECore::Pos::FRAMES;
      return _marker.type() == MusECore::Pos::FRAMES;
      }

//---------------------------------------------------------
//   MarkerItem
//---------------------------------------------------------

// REMOVE Tim. clip. Changed.
// MarkerItem::MarkerItem(QTreeWidget* parent, MusECore::Marker* m)
//   : QTreeWidgetItem(parent)
//       {
//       _marker = m;
//       setText(COL_NAME, m->name());
//       setTick(m->tick());
//       if (m->type() == MusECore::Pos::FRAMES)
//             setIcon(COL_LOCK, QIcon(*lockIcon));
//       setLock(m->type() == MusECore::Pos::FRAMES);
//       }
MarkerItem::MarkerItem(QTreeWidget* parent, const MusECore::Marker& m)
  : QTreeWidgetItem(parent)
      {
      _marker = m;
      setText(COL_NAME, m.name());
      setTick(m.tick());
      if (m.type() == MusECore::Pos::FRAMES)
            setIcon(COL_LOCK, QIcon(*lockIcon));
      setLock(m.type() == MusECore::Pos::FRAMES);
      }

// REMOVE Tim. clip. Changed.
// //---------------------------------------------------------
// //   setName
// //---------------------------------------------------------
// 
// void MarkerItem::setName(const QString& s)
//       {
//       setText(COL_NAME, s);
//       _marker = MusEGlobal::song->setMarkerName(_marker, s);
//       }
// 
// //---------------------------------------------------------
// //   setLock
// //---------------------------------------------------------
// 
// void MarkerItem::setLock(bool lck)
//       {
//       setIcon(COL_LOCK, lck ? QIcon(*lockIcon) : QIcon());
//       _marker = MusEGlobal::song->setMarkerLock(_marker, lck);
//       }
// 
// //---------------------------------------------------------
// //   setTick
// //---------------------------------------------------------
// 
// void MarkerItem::setTick(unsigned v)
//       {
//       if (_marker->tick() != v)
//             _marker = MusEGlobal::song->setMarkerTick(_marker, v);
//       QString s;
//       int bar, beat;
//       unsigned tick;
//       MusEGlobal::sigmap.tickValues(v, &bar, &beat, &tick);
//       s = QString("%1.%2.%3")
//           .arg(bar + 1,      4, 10, QLatin1Char('0'))
//           .arg(beat + 1,     2, 10, QLatin1Char('0'))
//           .arg(tick,         3, 10, QLatin1Char('0'));
//       setText(COL_TICK, s);
// 
//       double time = double(MusEGlobal::tempomap.tick2frame(v))/double(MusEGlobal::sampleRate);
//       int hour = int(time) / 3600;
//       int min  = (int(time) % 3600)/60;
//       int sec  = int(time) % 60;
//       double rest = time - (hour*3600 + min * 60 + sec);
//       switch(MusEGlobal::mtcType) {
//             case 0:     // 24 frames sec
//                   rest *= 24;
//                   break;
//             case 1:     // 25
//                   rest *= 25;
//                   break;
//             case 2:     // 30 drop frame
//                   rest *= 30;
//                   break;
//             case 3:     // 30 non drop frame
//                   rest *= 30;
//                   break;
//             }
//       int frame = int(rest);
//       int subframe = int((rest-frame)*100);
//       s = QString("%1:%2:%3:%4:%5")
//           .arg(hour,     2, 10, QLatin1Char('0'))
//           .arg(min,      2, 10, QLatin1Char('0'))
//           .arg(sec,      2, 10, QLatin1Char('0'))
//           .arg(frame,    2, 10, QLatin1Char('0'))
//           .arg(subframe, 2, 10, QLatin1Char('0'));
//       setText(COL_SMPTE, s);
//       }

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
//   setTick
//---------------------------------------------------------

void MarkerItem::setTick(unsigned v)
      {
      QString s;
      int bar, beat;
      unsigned tick;
      MusEGlobal::sigmap.tickValues(v, &bar, &beat, &tick);
      s = QString("%1.%2.%3")
          .arg(bar + 1,      4, 10, QLatin1Char('0'))
          .arg(beat + 1,     2, 10, QLatin1Char('0'))
          .arg(tick,         3, 10, QLatin1Char('0'));
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
      s = QString("%1:%2:%3:%4:%5")
          .arg(hour,     2, 10, QLatin1Char('0'))
          .arg(min,      2, 10, QLatin1Char('0'))
          .arg(sec,      2, 10, QLatin1Char('0'))
          .arg(frame,    2, 10, QLatin1Char('0'))
          .arg(subframe, 2, 10, QLatin1Char('0'));
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
      connect(markerAdd, SIGNAL(triggered()), SLOT(addMarker()));

      QAction* markerDelete = new QAction(QIcon(*deleteIcon), tr("delete marker"), this);
      connect(markerDelete, SIGNAL(triggered()), SLOT(deleteMarker()));

      //---------Pulldown Menu----------------------------
      QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
      
      editMenu->addAction(markerAdd);
      editMenu->addAction(markerDelete);
      
      
      QMenu* settingsMenu = menuBar()->addMenu(tr("Window &Config"));
      settingsMenu->addAction(subwinAction);
      settingsMenu->addAction(shareAction);
      settingsMenu->addAction(fullscreenAction);
      
      
      // Toolbars ---------------------------------------------------------
      
      // NOTICE: Please ensure that any tool bar object names here match the names assigned 
      //          to identical or similar toolbars in class MusE or other TopWin classes. 
      //         This allows MusE::setCurrentMenuSharingTopwin() to do some magic
      //          to retain the original toolbar layout. If it finds an existing
      //          toolbar with the same object name, it /replaces/ it using insertToolBar(),
      //          instead of /appending/ with addToolBar().

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

// REMOVE Tim. clip. Changed.
      //editTick = new PosEdit;
      editTick = new PosEdit(nullptr, MusECore::Pos::TICKS, false, MusECore::TimeFormatBBT, MusECore::TimeFormatAll);
//       editTick->setTimeFormatOptions(MusECore::TimeFormatTicksFormatted | MusECore::TimeFormatUserAll);
      editTick->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

// REMOVE Tim. clip. Changed.
//       editSMPTE = new PosEdit;
      editSMPTE = new PosEdit(nullptr, MusECore::Pos::FRAMES, false, MusECore::TimeFormatMSFS, MusECore::TimeFormatAll);
//       editSMPTE->setTimeFormatOptions(MusECore::TimeFormatFramesFormatted | MusECore::TimeFormatUserAll);
      editSMPTE->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

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

// REMOVE Tim. clip. Changed.
//       connect(editName, SIGNAL(textChanged(const QString&)),
//          SLOT(nameChanged(const QString&)));
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

      vbox->addWidget(table);
      vbox->addWidget(props);

      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
      
      updateList();

      finalizeInit();

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

void MarkerView::songChanged(MusECore::SongChangedStruct_t flags)
{
  // REMOVE Tim. clip. Added.
  if(flags._flags &
    (SC_MARKERS_REBUILT | SC_TEMPO | SC_MASTER |
     SC_MARKER_INSERTED | SC_MARKER_REMOVED | SC_MARKER_MODIFIED))

    updateList();
}

// REMOVE Tim. clip. Added.
// //---------------------------------------------------------
// //   updateList
// //---------------------------------------------------------
// 
// void MarkerView::updateList()
// {
//       // Added p3.3.43 Manage selected item, due to clearing of table...
//       MusECore::MarkerList* marker = MusEGlobal::song->marker();
//       MarkerItem* selitem     = (MarkerItem*)table->currentItem();
//       MusECore::Marker* selm     = selitem ? selitem->marker() : 0;
//       // p3.3.44 Look for removed markers before added markers...
//       if(selitem)
//       {
//         MarkerItem* mitem = (MarkerItem*)table->topLevelItem(0);
//         while(mitem) 
//         {
//           bool found = false;
//           for(MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) 
//           {
//             MusECore::Marker* m = &i->second;
//             if(m == mitem->marker()) 
//             {
//               found = true;
//               break;
//             }
//           }
//           // Anything removed from the marker list?
//           if(!found)
//           {
//             // If it is the current selected item, it no longer exists. Make the next item be selected.
//             if(mitem == selitem)
//             {
//               MarkerItem* mi = (MarkerItem*)table->itemBelow(selitem);
//               if(mi)
//               {
//                 selitem = mi;
//                 selm    = selitem->marker();
//               }  
//             }  
//           }  
//           mitem = (MarkerItem*)table->itemBelow(mitem);
//         }
//       }  
//       // Look for added markers...
//       for(MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) 
//       {
//         MusECore::Marker* m = &i->second;
//         bool found = false;
//         MarkerItem* item = (MarkerItem*)table->topLevelItem(0);
//         while(item) 
//         {
//           if(item->marker() == m) 
//           {
//             found = true;
//             break;
//           }
//           item = (MarkerItem*)table->itemBelow(item);
//         }
//         // Anything new found in the marker list?
//         if(!found)
//           selm = m;
//       }
//             
//       // Block signals added. Triggers itemSelectionChanged() causing crash. Tim. 
//       table->blockSignals(true);
//       table->clear();
//       table->blockSignals(false);
//       
//       //MusECore::MarkerList* marker = MusEGlobal::song->marker();
//       for (MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) 
//       {
//             MusECore::Marker* m = &i->second;
//             
//             // Changed p3.3.43 
//             MarkerItem* item = new MarkerItem(table, m);
//             if(m == selm)
//             {
//               m->setCurrent(true);
//               table->setCurrentItem(item);
//             }
//             else  
//             {
//               m->setCurrent(false);
//             }
//       }
// }

//---------------------------------------------------------
//   updateList
//---------------------------------------------------------

void MarkerView::updateList()
{
      // Manage selected item, due to clearing of table...
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      MarkerItem* selitem     = (MarkerItem*)table->currentItem();
//       MusECore::Marker* selm     = selitem ? selitem->marker() : 0;
      MusECore::Marker selm;
      if(selitem)
        selm = selitem->marker();

      // Look for removed markers before added markers...
      if(selitem)
      {
        MarkerItem* mitem = (MarkerItem*)table->topLevelItem(0);
        while(mitem) 
        {
          bool found = false;
          for(MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) 
          {
//             MusECore::Marker* m = &i->second;
//             if(m == mitem->marker()) 
            const MusECore::Marker& m = i->second;
//             if(m == mitem->marker()) 
            if(m.id() == mitem->marker().id()) 
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
//         MusECore::Marker* m = &i->second;
        const MusECore::Marker& m = i->second;
        bool found = false;
        MarkerItem* item = (MarkerItem*)table->topLevelItem(0);
        while(item) 
        {
//           if(item->marker() == m) 
          if(item->marker().id() == m.id())
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
//             MusECore::Marker* m = &i->second;
//             const MusECore::Marker& m = i->second;
            MusECore::Marker& m = i->second;
            
            // Changed p3.3.43 
            MarkerItem* item = new MarkerItem(table, m);
//             if(m == selm)
            if(m.id() == selm.id())
            {
//               m->setCurrent(true);
              m.setCurrent(true);
              table->setCurrentItem(item);
            }
            else  
            {
//               m->setCurrent(false);
              m.setCurrent(false);
            }
      }
}

//---------------------------------------------------------
//   markerSelected
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
            editTick->setValue(item->tick());
            editSMPTE->setValue(item->tick());
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
      MusECore::Pos p(item->tick(), true);
      MusEGlobal::song->setPos(0, p, true, true, false);
      }

// REMOVE Tim. clip. Changed.
// //---------------------------------------------------------
// //   nameChanged
// //---------------------------------------------------------
// 
// void MarkerView::nameChanged(const QString& s)
//       {
//       MarkerItem* item = (MarkerItem*)table->currentItem();
//       if (item)
//             item->setName(s);
//       }
// 
// //---------------------------------------------------------
// //   tickChanged
// //---------------------------------------------------------
// 
// void MarkerView::tickChanged(const MusECore::Pos& pos)
//       {
//       MarkerItem* item = (MarkerItem*)table->currentItem();
//       if (item) {
//             item->setTick(pos.tick());
//             MusECore::Pos p(pos.tick(), true);
//             MusEGlobal::song->setPos(0, p, true, true, false);
//             table->sortByColumn(COL_TICK, Qt::AscendingOrder);
//             }
//       }
// 
// //---------------------------------------------------------
// //   lockChanged
// //---------------------------------------------------------
// 
// void MarkerView::lockChanged(bool lck)
//       {
//       MarkerItem* item = (MarkerItem*)table->currentItem();
//       if (item) {
//             item->setLock(lck);
//             editSMPTE->setEnabled(item->lock());
//             editTick->setEnabled(!item->lock());
//             }
//       }

//---------------------------------------------------------
//   nameChanged
//---------------------------------------------------------

void MarkerView::nameChanged()
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item)
      {
//         item->setName(s);
        MusEGlobal::song->setMarkerName(item->marker(), editName->text());
      }
      }

//---------------------------------------------------------
//   tickChanged
//---------------------------------------------------------

void MarkerView::tickChanged(const MusECore::Pos& pos)
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item) {
            if (item->marker().tick() != pos.tick())
            {
//               item->setTick(pos.tick());
              MusEGlobal::song->setMarkerTick(item->marker(), pos.tick());
            }
//             MusECore::Pos p(pos.tick(), true);
//             MusEGlobal::song->setPos(0, p, true, true, false);
//             table->sortByColumn(COL_TICK, Qt::AscendingOrder);
            }
      }

//---------------------------------------------------------
//   lockChanged
//---------------------------------------------------------

void MarkerView::lockChanged(bool lck)
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item) {
//             item->setLock(lck);
            MusEGlobal::song->setMarkerLock(item->marker(), lck);
//             editSMPTE->setEnabled(item->lock());
//             editTick->setEnabled(!item->lock());
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
// REMOVE Tim. clip. Changed.
//         case MusECore::Song::MARKER_ADD:
//         case MusECore::Song::MARKER_REMOVE:
//           updateList();      
//         break; // Try falling through and let it try to select something. No, let updateList() do it...
        
        case MusECore::Song::MARKER_CUR:
        {
          
          MusECore::MarkerList* marker = MusEGlobal::song->marker();
          for (MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) {
                if (i->second.current()) {
                      MarkerItem* item = (MarkerItem*)table->topLevelItem(0);
                      while (item) {
// REMOVE Tim. clip. Changed.
//                             if (item->marker() == &i->second) {
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
