//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
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

#include "markerview.h"
#include "globals.h"
#include "sync.h"
#include "icons.h"
#include "song.h"
#include "awl/posedit.h"
#include "al/al.h"
#include "al/xml.h"
#include "al/sig.h"
#include "al/tempo.h"
#include "al/marker.h"

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
      return _marker->type() == AL::FRAMES;
      }

//---------------------------------------------------------
//   MarkerItem
//---------------------------------------------------------

MarkerItem::MarkerItem(QTreeWidget* parent, AL::Marker* m)
  : QTreeWidgetItem(parent)
      {
      _marker = m;
      setText(COL_NAME, m->name());
      setTick(m->tick());
      if (m->type() == AL::FRAMES)
            setIcon(COL_LOCK, *lockIcon);
      setLock(m->type() == AL::FRAMES);
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
      setIcon(COL_LOCK, lck ? *lockIcon : QIcon());
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
      int tick;
      _marker->mbt(&bar, &beat, &tick);
      s.sprintf("%04d.%02d.%03d", bar+1, beat+1, tick);
      setText(COL_TICK, s);

      int min, sec, frame, subframe;
      _marker->msf(&min, &sec, &frame, &subframe);

      s.sprintf("%03d:%02d:%02d:%02d", min, sec, frame, subframe);
      setText(COL_SMPTE, s);
      }

//---------------------------------------------------------
//   MarkerView
//---------------------------------------------------------

MarkerView::MarkerView()
   : TopWin()
      {
      setWindowTitle(tr("MusE: Marker"));

      //---------Actions----------------------------
      QAction* markerAdd = new QAction(QIcon(*flagIcon), tr("add marker"), this);
      markerAdd->setToolTip(tr("Add Marker"));
      connect(markerAdd, SIGNAL(triggered()), SLOT(addMarker()));

      QAction* markerDelete = new QAction(QIcon(*deleteIcon), tr("delete marker"), this);
      markerDelete->setToolTip(tr("Delete Marker"));
      connect(markerDelete, SIGNAL(triggered()), SLOT(deleteMarker()));

      //---------Pulldown Menu----------------------------
      QMenuBar* mb = menuBar();

      QMenu* editMenu = mb->addMenu(tr("&Edit"));
      editMenu->addAction(markerAdd);
      editMenu->addAction(markerDelete);

      //---------ToolBar----------------------------------
      QToolBar* tools = addToolBar(tr("marker-tools"));
      tools->addAction(undoAction);
      tools->addAction(redoAction);

      QToolBar* edit = addToolBar(tr("edit tools"));
      edit->addAction(markerAdd);
      edit->addAction(markerDelete);

      //---------------------------------------------------
      //    master
      //---------------------------------------------------

      QWidget* w = new QWidget;
      setCentralWidget(w);
      QVBoxLayout* vbox = new QVBoxLayout;
      w->setLayout(vbox);

      table = new QTreeWidget;
      table->setUniformRowHeights(true);
      table->setAlternatingRowColors(true);
      table->setSelectionBehavior(QAbstractItemView::SelectRows);
      table->setSelectionMode(QTreeWidget::SingleSelection);
      table->setSortingEnabled(false);
      table->setIndentation(0);
      QStringList labels;
      labels << tr("Bar:Beat:Tick")
             << tr("Min:Sc:Fr:Sf")
             << tr("Lock")
             << tr("Text");
      table->setHeaderLabels(labels);

      table->header()->resizeSection(0, 120);
      table->header()->resizeSection(1, 120);
      table->header()->resizeSection(2, 50);
      table->header()->resizeSection(3, 200);
//      table->header()->resizeSection(mnWidthMode(3, QTreeWidget::Maximum);
      connect(table, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), SLOT(markerSelectionChanged(QTreeWidgetItem*)));
      connect(table, SIGNAL(itemClicked(QTreeWidgetItem*,int)), SLOT(clicked(QTreeWidgetItem*)));

      QGroupBox* props = new QGroupBox(tr("Marker Properties"));
      QHBoxLayout* propsLayout = new QHBoxLayout;
      props->setLayout(propsLayout);

      editTick = new Awl::PosEdit;
      propsLayout->addWidget(editTick);
      editTick->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,
         QSizePolicy::Fixed));

      editSMPTE = new Awl::PosEdit;
      propsLayout->addWidget(editSMPTE);
      editSMPTE->setSmpte(true);
      editSMPTE->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,
         QSizePolicy::Fixed));

      lock = new QToolButton;
      propsLayout->addWidget(lock);
      lock->setIcon(*lockIcon);
      lock->setCheckable(true);

      editName = new QLineEdit;
      propsLayout->addWidget(editName);
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
      }

//---------------------------------------------------------
//   MArkerView
//---------------------------------------------------------

MarkerView::~MarkerView()
      {
//      undoRedo->removeFrom(tools);
      }

//---------------------------------------------------------
//   addMarker
//---------------------------------------------------------

void MarkerView::addMarker()
      {
      addMarker(song->cPos());
      }

void MarkerView::addMarker(const AL::Pos& pos)
      {
      AL::Marker* m = song->addMarker(QString(""), pos);
      MarkerItem* newItem = new MarkerItem(table, m);
      table->setCurrentItem(newItem);
      table->setItemSelected(newItem, true);
      table->sortItems(0, Qt::AscendingOrder);
      }

//---------------------------------------------------------
//   removeMarker
//---------------------------------------------------------

void MarkerView::removeMarker(const AL::Pos&)
      {
      printf("delete marker: not implemented\n");
      }

//---------------------------------------------------------
//   deleteMarker
//---------------------------------------------------------

void MarkerView::deleteMarker()
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
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
      AL::MarkerList* marker = song->marker();
      for (AL::iMarker i = marker->begin(); i != marker->end(); ++i) {
            AL::Marker* m = &i->second;
            QString tick;
            tick.setNum(i->first);
            new MarkerItem(table, m);
            }
      }

//---------------------------------------------------------
//   markerSelected
//---------------------------------------------------------

void MarkerView::markerSelectionChanged(QTreeWidgetItem* i)
      {
      MarkerItem* item = (MarkerItem*)i;
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
      Pos p(item->tick(), AL::TICKS);
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
            Pos p(pos.tick(), AL::TICKS);
            song->setPos(0, p, true, true, false);
//            table->sort();
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

void MarkerView::markerChanged(int)
      {
#if 0
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
#endif
      }

