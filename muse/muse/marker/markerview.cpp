//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: markerview.cpp,v 1.7.2.6 2009/08/25 20:28:45 spamatica Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "marker.h"
#include "markerview.h"
#include "xml.h"
#include "globals.h"
#include "sync.h"
#include "icons.h"
#include "song.h"
#include "posedit.h"

#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <qlayout.h>
#include <qsizegrip.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qaction.h>
#include <qgroupbox.h>
#include <qlineedit.h>

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

MarkerItem::MarkerItem(QListView* parent, Marker* m)
  : QListViewItem(parent)
      {
      _marker = m;
      setText(COL_NAME, m->name());
      setTick(m->tick());
      if (m->type() == Pos::FRAMES)
            setPixmap(COL_LOCK, *lockIcon);
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
      setPixmap(COL_LOCK, lck ? *lockIcon : 0);
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
      sigmap.tickValues(v, &bar, &beat, &tick);
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
   : TopWin(parent, "markerview", WType_TopLevel /*| WDestructiveClose*/)
      {
      setCaption(tr("MusE: Marker"));

      //---------Actions----------------------------
      QAction* markerAdd = new QAction(tr("add marker"),
         QIconSet(*flagIcon), tr("Add Marker"),
        0, this, "add marker");
      connect(markerAdd, SIGNAL(activated()), SLOT(addMarker()));

      QAction* markerDelete = new QAction(tr("delete marker"),
         QIconSet(*deleteIcon), tr("Delete Marker"),
         0, this, "delete marker");
      connect(markerDelete, SIGNAL(activated()), SLOT(deleteMarker()));

      //---------Pulldown Menu----------------------------
      QPopupMenu* fileMenu = new QPopupMenu(this);
      menuBar()->insertItem(tr("&File"), fileMenu);
      QPopupMenu* editMenu = new QPopupMenu(this);
      menuBar()->insertItem(tr("&Edit"), editMenu);
      markerAdd->addTo(editMenu);
      markerDelete->addTo(editMenu);

      //---------ToolBar----------------------------------
      tools = new QToolBar(this, "marker-tools");
      undoRedo->addTo(tools);

      QToolBar* edit = new QToolBar(this, "edit tools");
      markerAdd->addTo(edit);
      markerDelete->addTo(edit);

      //---------------------------------------------------
      //    master
      //---------------------------------------------------

      QWidget* w = new QWidget(this, "main");
      setCentralWidget(w);
      QVBoxLayout* vbox = new QVBoxLayout(w);

      table = new QListView(w);
      table->setAllColumnsShowFocus(true);
      table->setSelectionMode(QListView::Single);
      table->setSorting(COL_TICK, true);

      table->addColumn(tr("Bar:Beat:Tick"));
      table->addColumn(tr("Hr:Mn:Sc:Fr:Sf"));
      table->addColumn(tr("Lock"));
      table->addColumn(tr("Text"));
      table->setColumnWidth(3, 200);
      table->setColumnWidthMode(3, QListView::Maximum);
      connect(table, SIGNAL(selectionChanged()),
         SLOT(markerSelectionChanged()));
      connect(table, SIGNAL(clicked(QListViewItem*)),
         SLOT(clicked(QListViewItem*)));

      QGroupBox* props = new QGroupBox(4, Horizontal, tr("Marker Properties"), w);

      editTick = new PosEdit(props);
      editTick->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,
         QSizePolicy::Fixed));

      editSMPTE = new PosEdit(props);
      editSMPTE->setSmpte(true);
      editSMPTE->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,
         QSizePolicy::Fixed));

      lock = new QToolButton(props);
      lock->setPixmap(*lockIcon);
      lock->setToggleButton(true);

      editName = new QLineEdit(props);
      editName->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
         QSizePolicy::Preferred));
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

      connect(song, SIGNAL(songChanged(int)), SLOT(updateList()));
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
      undoRedo->removeFrom(tools);
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
      Marker* m = song->addMarker(QString(""), i, false);
      MarkerItem* newItem = new MarkerItem(table, m);
      table->setSelected(newItem, true);
      }

//---------------------------------------------------------
//   deleteMarker
//---------------------------------------------------------

void MarkerView::deleteMarker()
      {
      MarkerItem* item = (MarkerItem*)table->selectedItem();
      if (item) {
            song->removeMarker(item->marker());
            delete item;
            }
      }

//---------------------------------------------------------
//   updateList
//---------------------------------------------------------

void MarkerView::updateList()
      {
      table->clear();
      MarkerList* marker = song->marker();
      for (iMarker i = marker->begin(); i != marker->end(); ++i) {
            Marker* m = &i->second;
            QString tick;
            tick.setNum(i->first);
            new MarkerItem(table, m);
            }
      }

//---------------------------------------------------------
//   markerSelected
//---------------------------------------------------------

void MarkerView::markerSelectionChanged()
      {
      MarkerItem* item = (MarkerItem*)table->selectedItem();
      if (item == 0) {  // never triggered
            editTick->setValue(0);
            editSMPTE->setValue(0);
            editName->setText(QString(""));
            lock->setOn(false);
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
            lock->setOn(item->lock());
            lock->setEnabled(true);
            editSMPTE->setEnabled(item->lock());
            editTick->setEnabled(!item->lock());
            }
      }

void MarkerView::clicked(QListViewItem* i)
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
      MarkerItem* item = (MarkerItem*)table->selectedItem();
      if (item)
            item->setName(s);
      }

//---------------------------------------------------------
//   tickChanged
//---------------------------------------------------------

void MarkerView::tickChanged(const Pos& pos)
      {
      MarkerItem* item = (MarkerItem*)table->selectedItem();
      if (item) {
            item->setTick(pos.tick());
            Pos p(pos.tick(), true);
            song->setPos(0, p, true, true, false);
            table->sort();
            }
      }

//---------------------------------------------------------
//   lockChanged
//---------------------------------------------------------

void MarkerView::lockChanged(bool lck)
      {
      MarkerItem* item = (MarkerItem*)table->selectedItem();
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
      if (val != Song::MARKER_CUR)
            return;
      MarkerList* marker = song->marker();
      for (iMarker i = marker->begin(); i != marker->end(); ++i) {
            if (i->second.current()) {
                  MarkerItem* item = (MarkerItem*)table->firstChild();
                  while (item) {
                        if (item->marker() == &i->second) {
                              table->setSelected(item, true);
                              return;
                              }
                        item = (MarkerItem*)item->nextSibling();
                        }
                  }
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
