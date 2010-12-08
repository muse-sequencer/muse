//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: markerview.cpp,v 1.7.2.6 2009/08/25 20:28:45 spamatica Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "al/sig.h"  // Tim.

#include "marker.h"
#include "markerview.h"
#include "xml.h"
#include "globals.h"
#include "sync.h"
#include "icons.h"
#include "song.h"
///#include "posedit.h"
#include "awl/posedit.h"

#include <QCloseEvent>
#include <QMenu>
#include <QHBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>


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
      return _marker->type() == Pos::FRAMES;
      }

//---------------------------------------------------------
//   MarkerItem
//---------------------------------------------------------

MarkerItem::MarkerItem(QTreeWidget* parent, Marker* m)
  : QTreeWidgetItem(parent)
      {
      _marker = m;
      setText(COL_NAME, m->name());
      setTick(m->tick());
      if (m->type() == Pos::FRAMES)
            setIcon(COL_LOCK, QIcon(*lockIcon));
      setLock(m->type() == Pos::FRAMES);
      }

//---------------------------------------------------------
//   setName
//---------------------------------------------------------

void MarkerItem::setName(const QString& s)
      {
      setText(COL_NAME, s);
      _marker = song->setMarkerName(_marker, s);
      }

//---------------------------------------------------------
//   setLock
//---------------------------------------------------------

void MarkerItem::setLock(bool lck)
      {
      setIcon(COL_LOCK, QIcon(lck ? *lockIcon : 0));
      _marker = song->setMarkerLock(_marker, lck);
      }

//---------------------------------------------------------
//   setTick
//---------------------------------------------------------

void MarkerItem::setTick(unsigned v)
      {
      if (_marker->tick() != v)
            _marker = song->setMarkerTick(_marker, v);
      QString s;
      int bar, beat;
      unsigned tick;
      ///sigmap.tickValues(v, &bar, &beat, &tick);
      AL::sigmap.tickValues(v, &bar, &beat, &tick);
      s.sprintf("%04d.%02d.%03d", bar+1, beat+1, tick);
      setText(COL_TICK, s);

      double time = double(tempomap.tick2frame(v))/double(sampleRate);
      int hour = int(time) / 3600;
      int min  = (int(time) % 3600)/60;
      int sec  = int(time) % 60;
      double rest = time - (hour*3600 + min * 60 + sec);
      switch(mtcType) {
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
      emit deleted((unsigned long)this);
      emit closed();
      e->accept();
      }

//---------------------------------------------------------
//   MarkerView
//---------------------------------------------------------

MarkerView::MarkerView(QWidget* parent)
   : TopWin(parent, "markerview", Qt::Window /*| WDestructiveClose*/)
      {
      //setAttribute(Qt::WA_DeleteOnClose);
      
      setWindowTitle(tr("MusE: Marker"));

      QAction* markerAdd = new QAction(QIcon(*flagIcon), tr("add marker"), this);
      connect(markerAdd, SIGNAL(activated()), SLOT(addMarker()));

      QAction* markerDelete = new QAction(QIcon(*deleteIcon), tr("delete marker"), this);
      connect(markerDelete, SIGNAL(activated()), SLOT(deleteMarker()));

      //---------Pulldown Menu----------------------------
      /* We probably don't need an empty menu - Orcan
      QMenu* fileMenu = new QMenu(tr("&File"));
      menuBar()->addMenu(fileMenu);
      */
      QMenu* editMenu = new QMenu(tr("&Edit"));
      menuBar()->addMenu(editMenu);
      editMenu->addAction(markerAdd);
      editMenu->addAction(markerDelete);

      //---------ToolBar----------------------------------
      tools = addToolBar(tr("marker-tools"));
      tools->addActions(undoRedo->actions());

      QToolBar* edit = addToolBar(tr("edit tools"));
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

      ///editSMPTE = new PosEdit;
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
      connect(editTick, SIGNAL(valueChanged(const Pos&)),
         SLOT(tickChanged(const Pos&)));
      connect(editSMPTE, SIGNAL(valueChanged(const Pos&)),
         SLOT(tickChanged(const Pos&)));
      connect(editSMPTE, SIGNAL(valueChanged(const Pos&)),
         editTick, SLOT(setValue(const Pos&)));
      connect(editTick, SIGNAL(valueChanged(const Pos&)),
         editSMPTE, SLOT(setValue(const Pos&)));
      connect(lock, SIGNAL(toggled(bool)),
         SLOT(lockChanged(bool)));
      connect(song, SIGNAL(markerChanged(int)),
         SLOT(markerChanged(int)));

      vbox->addWidget(table);
      vbox->addWidget(props);

      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

      //connect(song, SIGNAL(songChanged(int)), SLOT(updateList()));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      
      updateList();

      // work around for probable QT/WM interaction bug.
      // for certain window managers, e.g xfce, this window is
      // is displayed although not specifically set to show();
      // bug: 2811156  	 Softsynth GUI unclosable with XFCE4 (and a few others)
      show();
      hide();

      }

//---------------------------------------------------------
//   MArkerView
//---------------------------------------------------------

MarkerView::~MarkerView()
      {
      //printf("MarkerView::~MarkerView() before undoRedo->removeFrom(tools)\n");
      
      // undoRedo->removeFrom(tools);   // p4.0.6 Removed
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void MarkerView::readStatus(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            if (token == Xml::Error || token == Xml::End)
                  break;
            switch (token) {
                  case Xml::TagStart:
                        xml.unknown("Marker");
                        break;
                  case Xml::TagEnd:
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

void MarkerView::writeStatus(int level, Xml& xml) const
      {
      xml.tag(level++, "marker");
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
      if( i==-1 ) i = song->cpos();
      
      // Changed p3.3.43 Let Song::addMarker emit markerChanged(MARKER_ADD)
      //  and handle it in MarkerView::markerChanged(int)
      //Marker* m = song->addMarker(QString(""), i, false);
      //MarkerItem* newItem = new MarkerItem(table, m);
      //table->setSelected(newItem, true);
      //
      song->addMarker(QString(""), i, false);
      }

//---------------------------------------------------------
//   deleteMarker
//---------------------------------------------------------

void MarkerView::deleteMarker()
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item) {
            table->blockSignals(true);
            song->removeMarker(item->marker());
            table->blockSignals(false);
            // Removed p3.3.43 Let Song::removeMarker emit markerChanged(MARKER_REMOVE)
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
      MarkerList* marker = song->marker();
      MarkerItem* selitem     = (MarkerItem*)table->currentItem();
      Marker* selm     = selitem ? selitem->marker() : 0;
      // p3.3.44 Look for removed markers before added markers...
      if(selitem)
      {
        MarkerItem* mitem = (MarkerItem*)table->topLevelItem(0);
        while(mitem) 
        {
          bool found = false;
          for(iMarker i = marker->begin(); i != marker->end(); ++i) 
          {
            Marker* m = &i->second;
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
      for(iMarker i = marker->begin(); i != marker->end(); ++i) 
      {
        Marker* m = &i->second;
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
            
      table->clear();
      //MarkerList* marker = song->marker();
      for (iMarker i = marker->begin(); i != marker->end(); ++i) 
      {
            Marker* m = &i->second;
            
            // Changed p3.3.43 
            //QString tick;
            //tick.setNum(i->first);
            //new MarkerItem(table, m);
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
            
            //printf("MarkerView::markerSelectionChanged item->lock:%d\n", item->lock());
            
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
      Pos p(item->tick(), true);
      song->setPos(0, p, true, true, false);
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

void MarkerView::tickChanged(const Pos& pos)
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item) {
            item->setTick(pos.tick());
            Pos p(pos.tick(), true);
            song->setPos(0, p, true, true, false);
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
      //if (val != Song::MARKER_CUR)
      //      return;
      // p3.3.43
      switch(val)
      {
         // MARKER_CUR, MARKER_ADD, MARKER_REMOVE, MARKER_NAME,
         // MARKER_TICK, MARKER_LOCK
        case Song::MARKER_ADD:
        case Song::MARKER_REMOVE:
          updateList();      
        break; // Try falling through and let it try to select something. No, let updateList() do it...
        
        case Song::MARKER_CUR:
        {
          
          MarkerList* marker = song->marker();
          for (iMarker i = marker->begin(); i != marker->end(); ++i) {
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
      unsigned int curPos = song->cpos();//prevent compiler warning: comparison of sigend/unsigned
      unsigned int nextPos = 0xFFFFFFFF;
      MarkerList* marker = song->marker();
      for (iMarker i = marker->begin(); i != marker->end(); ++i) {
            if (i->second.tick() > curPos && i->second.tick() < nextPos)
              nextPos = i->second.tick();
            }
      if (nextPos == 0xFFFFFFFF)
          return;
      Pos p(nextPos, true);
      song->setPos(0, p, true, true, false);
        
      }
void MarkerView::prevMarker()
      {
      unsigned int curPos = song->cpos();//prevent compiler warning: comparison of sigend/unsigned
      unsigned int nextPos = 0;
      MarkerList* marker = song->marker();
      for (iMarker i = marker->begin(); i != marker->end(); ++i) {
            if (i->second.tick() < curPos && i->second.tick() > nextPos)
              nextPos = i->second.tick();
            }
/*      if (nextPos == 0)
          return;*/
      Pos p(nextPos, true);
      song->setPos(0, p, true, true, false);
      }
