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
// REMOVE Tim. clip. Added.
#include "audio.h"

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

// void MarkerItem::init(const MusECore::Marker& m)
void MarkerItem::setMarker(const MusECore::Marker& m)
{
      _marker = m;
      setText(COL_NAME, m.name());
//       setTick(m.tick());
      setPos(m);
      //if (m.type() == MusECore::Pos::FRAMES)
      //      setIcon(COL_LOCK, QIcon(*lockIcon));
      setLock(m.type() == MusECore::Pos::FRAMES);
  
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
      setMarker(m);
      }

MarkerItem::MarkerItem(const MusECore::Marker& m)
  : QTreeWidgetItem()
      {
      setMarker(m);
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

// //---------------------------------------------------------
// //   setTick
// //---------------------------------------------------------
// 
// void MarkerItem::setTick(unsigned v)
//       {
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

      // REMOVE Tim. clip. Changed.
//       double time = double(v.frame())/double(MusEGlobal::sampleRate);
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
      editTick = new PosEdit(nullptr, MusECore::Pos::TICKS, true, MusECore::TimeFormatBBT, MusECore::TimeFormatAll);
//       editTick->setTimeFormatOptions(MusECore::TimeFormatTicksFormatted | MusECore::TimeFormatUserAll);
      editTick->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

// REMOVE Tim. clip. Changed.
//       editSMPTE = new PosEdit;
      editSMPTE = new PosEdit(nullptr, MusECore::Pos::FRAMES, true, MusECore::TimeFormatMSFS, MusECore::TimeFormatAll);
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

// REMOVE Tim. clip. Changed.
// //---------------------------------------------------------
// //   addMarker
// //---------------------------------------------------------
// void MarkerView::addMarker()
//       {
//       addMarker(-1);
//       }
//---------------------------------------------------------
//   addMarker
//---------------------------------------------------------
void MarkerView::addMarker()
      {
      MusEGlobal::song->addMarker(QString(""), MusEGlobal::song->cpos(), false);
      }
// //---------------------------------------------------------
// //   addMarker
// //---------------------------------------------------------
// void MarkerView::addMarker()
//       {
//       MusEGlobal::song->addMarker(QString(""), MusEGlobal::song->cPos());
//       }
void MarkerView::addMarker(unsigned i)
      {
//       if( i==-1 ) i = MusEGlobal::song->cpos();
      
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
  // Prevent race condition: Ignore if the change was ultimately self-sent.
//   if(//flags._sender != this &&
//      flags._flags & (SC_MARKERS_REBUILT | SC_TEMPO | SC_MASTER |
//        SC_MARKER_INSERTED | SC_MARKER_REMOVED | SC_MARKER_MODIFIED))
//     updateList();

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

// REMOVE Tim. clip. Added.
//---------------------------------------------------------
//   updateList
//---------------------------------------------------------

void MarkerView::rebuildList()
{
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      const MarkerItem* selitem = (MarkerItem*)table->currentItem();
      std::int64_t selitem_id = -1;
      if(selitem)
        selitem_id = selitem->marker().id();
      int m_id;


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
// //       int selitem_idx = -1;
      std::int64_t selitem_id = -1;
      MarkerItem* new_selitem = nullptr;
// //       int new_selitem_idx = -1;
//       std::int64_t new_selitem_id = -1;
//       // = table->indexOfTopLevelItem(selitem);
// //       MusECore::Marker* selm     = selitem ? selitem->marker() : 0;
// //       MusECore::Marker selm;
      if(selitem)
        selitem_id = selitem->marker().id();

      int m_id, mitem_id;
      MarkerItem* mitem;
//       MarkerItem *mi_above, *mi_below;
//       int mi_below_idx;
      unsigned m_frame, mitem_frame;

      // Look for removed markers before added markers...
//       if(selitem)
      {
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
          
//           //mi_below_idx = mitem_idx + 1;
//           //mi_below = (MarkerItem*)table->topLevelItem(mi_below_idx);
//           // If it is the current selected item, it no longer exists. Make the next item be selected.
//           if(mitem == selitem)
//           {
//             mi_above = (MarkerItem*)table->itemAbove(mitem);
//             mi_below = (MarkerItem*)table->itemBelow(mitem);
//             if(mi_below)
//             {
// //               selitem = mi_below;
//               new_selitem = mi_below;
// //               selm    = selitem->marker();
// //               selitem_idx = selitem->marker().id();
//               new_selitem_id = selitem->marker().id();
//             }
//             else if(mi_above)
//             {
// //               selitem = mi_above;
//               new_selitem = mi_above;
// //              selm    = selitem->marker();
// //               selitem_idx = selitem->marker().id();
//               new_selitem_id = selitem->marker().id();
//             }
//             else
//             {
// //               selitem = nullptr;
//               new_selitem = nullptr;
// //               selitem_idx = -1;
//               new_selitem_id = -1;
//             }
//           }

          delete mitem;

//           if(!mi_below)
//             break;


//           for(MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) 
//           {
// //             MusECore::Marker* m = &i->second;
// //             if(m == mitem->marker()) 
//             const MusECore::Marker& m = i->second;
// //             if(m == mitem->marker()) 
//             if(m.id() == mitem->marker().id()) 
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
        }
      }

      MarkerItem* found_item;
      int found_item_idx;
      int insert_idx;
      bool prev_frame_found;
      bool next_frame_found;
      unsigned prev_frame;
      unsigned next_frame;
//       bool current_changed = false;

      // Look for added markers...
      for(MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) 
      {
//         MusECore::Marker* m = &i->second;
        const MusECore::Marker& m = i->second;
        m_id = m.id();
        m_frame = m.frame();

//         bool found = false;
//         MarkerItem* item = (MarkerItem*)table->topLevelItem(0);
//         while(item)
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

//           if(insert_idx == -1 && m_frame <= mitem_frame)
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

// //           if(mitem->marker() == m) 
//           if(mitem->marker().id() == m.id())
//           {
//             found = true;
//             break;
//           }
//           mitem = (MarkerItem*)table->itemBelow(mitem);
        }

        // Did we find an item matching the marker ID?
        if(found_item)
        {
          const MusECore::Marker& found_m = found_item->marker();
          const unsigned found_f = found_m.frame();
//           if((m_frame < found_f && prev_frame_found && (found_f - m_frame) > (found_f - prev_frame)) ||
//              (m_frame > found_f && next_frame_found && (m_frame - found_f) > (next_frame - found_f)))
          // Can we get away with simply modifying the existing item's marker properties with the new ones?
          // That means is the new requested position CLOSER than a previous (or next) item's marker positon?
          if(m_frame == found_f ||
//              (m_frame < found_f && (!prev_frame_found || (found_f - m_frame) < (found_f - prev_frame))) ||
//              (m_frame > found_f && (!next_frame_found || (m_frame - found_f) < (next_frame - found_f))))
             (m_frame < found_f && (!prev_frame_found || m_frame >= prev_frame)) ||
             (m_frame > found_f && (!next_frame_found || m_frame <= next_frame)))
          {
            found_item->setMarker(m);
            if(m_id == selitem_id)
              new_selitem = found_item;
            continue;
          }

//           // If it is the current selected item, it no longer exists. Make the next item be selected.
//           if(found_item == selitem)
//           {
//             mi_above = (MarkerItem*)table->itemAbove(found_item);
//             mi_below = (MarkerItem*)table->itemBelow(found_item);
//             if(mi_below)
//             {
// //               selitem = mi_below;
//               new_selitem = mi_below;
// //               selm    = selitem->marker();
// //               selitem_idx = selitem->marker().id();
//               new_selitem_id = selitem->marker().id();
//             }
//             else if(mi_above)
//             {
// //               selitem = mi_above;
//               new_selitem = mi_above;
// //              selm    = selitem->marker();
// //               selitem_idx = selitem->marker().id();
//               new_selitem_id = selitem->marker().id();
//             }
//             else
//             {
// //               selitem = nullptr;
//               new_selitem = nullptr;
// //               selitem_idx = -1;
//               new_selitem_id = -1;
//             }
//           }

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
//         MarkerItem* new_item = new MarkerItem(table, m);
        MarkerItem* new_item = new MarkerItem(m);
        table->blockSignals(true);
        if(insert_idx != -1)
          table->insertTopLevelItem(insert_idx, new_item);
        else
          table->addTopLevelItem(new_item);
        table->blockSignals(false);

        if(m_id == selitem_id)
          new_selitem = new_item;
//         {
//           table->blockSignals(true);
//           table->setCurrentItem(new_item);
//           table->blockSignals(false);
// //           current_changed = true;
//         }
// //         else if(new_selitem && new_selitem_id != selitem_id)
// //           table->setCurrentItem(new_selitem);
          
//         // Anything new found in the marker list?
//         if(!found)
//           selm = m;

      }

      if(new_selitem)
      {
        table->blockSignals(true);
        table->setCurrentItem(new_selitem);
        table->blockSignals(false);
        //current_changed = true;
      }

//       if(current_changed)
        markerSelectionChanged();

//       if(selitem)
//         table->setCurrentItem(selitem);



//       // Block signals added. Triggers itemSelectionChanged() causing crash. Tim. 
//       table->blockSignals(true);
//       table->clear();
//       table->blockSignals(false);
//       
//       //MusECore::MarkerList* marker = MusEGlobal::song->marker();
//       for (MusECore::iMarker i = marker->begin(); i != marker->end(); ++i) 
//       {
// //             MusECore::Marker* m = &i->second;
// //             const MusECore::Marker& m = i->second;
//             MusECore::Marker& m = i->second;
//             
//             // Changed p3.3.43 
//             MarkerItem* item = new MarkerItem(table, m);
// //             if(m == selm)
//             if(m.id() == selm.id())
//             {
// //               m->setCurrent(true);
//               m.setCurrent(true);
//               table->setCurrentItem(item);
//             }
//             else  
//             {
// //               m->setCurrent(false);
//               m.setCurrent(false);
//             }
//       }
}

// REMOVE Tim. clip. Changed.
// //---------------------------------------------------------
// //   markerSelected
// //---------------------------------------------------------
// 
// void MarkerView::markerSelectionChanged()
//       {
//       MarkerItem* item = (MarkerItem*)table->currentItem();
//       editTick->blockSignals(true);
//       editSMPTE->blockSignals(true);
//       editName->blockSignals(true);
//       lock->blockSignals(true);
//       if (item == 0) {  // never triggered
//             editTick->setValue(0);
//             editSMPTE->setValue(0);
//             editName->setText(QString(""));
//             lock->setChecked(false);
//             editSMPTE->setEnabled(false);
//             editTick->setEnabled(false);
//             lock->setEnabled(false);
//             editName->setEnabled(false);
//             }
//       else {
//             editTick->setValue(item->tick());
//             editSMPTE->setValue(item->tick());
//             editName->setText(item->name());
//             editName->setEnabled(true);
//             lock->setChecked(item->lock());
//             lock->setEnabled(true);
//             
//             editSMPTE->setEnabled(item->lock());
//             editTick->setEnabled(!item->lock());
//             }
//       editTick->blockSignals(false);
//       editSMPTE->blockSignals(false);
//       editName->blockSignals(false);
//       lock->blockSignals(false);
//       }

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

// REMOVE Tim. clip. Changed.
// void MarkerView::clicked(QTreeWidgetItem* i)
//       {
//       MarkerItem* item = (MarkerItem*)i;
//       if (item == 0) {
//             table->clearSelection();
//             return;
//             }
//       MusECore::Pos p(item->tick(), true);
//       MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, false);
//       }

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
        // REMOVE Tim. clip. Added. Diagnostics.
        fprintf(stderr, "MarkerView::tickChanged: %u\n", pos.posValue());

      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item) {
// REMOVE Tim. clip. Changed.
//             if (item->marker().tick() != pos.tick())
            {
//               item->setTick(pos.tick());
//               MusEGlobal::song->setMarkerTick(item->marker(), pos.tick());
              MusEGlobal::song->setMarkerPos(item->marker(), pos);
            }
//             MusECore::Pos p(pos.tick(), true);
//             MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, false);
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

// bool MarkerView::compareMarkers(const MusECore::Marker& m1, const MusECore::Marker& m2) const
// {
//   // If m1 type is ticks, compare frame. If list type is frames, compare tick.
//   switch(m1.type())
//   {
//     case MusECore::Pos::TICKS:
//       switch(m2.type())
//       {
//         case MusECore::Pos::TICKS:
//             return m2.tick() <= m1.tick();
//         break;
// 
//         case MusECore::Pos::FRAMES:
//             return m2.frame() <= m1.frame();
//         break;
//       }
//     break;
// 
//     case MusECore::Pos::FRAMES:
//             return m2.frame() <= m1.frame();
//     break;
//   }
//   return false;
// }

} // namespace MusEGui
