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
#include "shortcuts.h"

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
      setIcon(COL_LOCK, m->type() == AL::FRAMES ? *lockIcon : QIcon());
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
      lockChange = false;
      setWindowTitle(tr("MusE: Marker"));

      //---------Actions----------------------------
      QAction* markerAdd = new QAction(QIcon(*flagIcon), tr("add marker"), this);
      markerAdd->setToolTip(tr("Add Marker"));
      connect(markerAdd, SIGNAL(triggered()), SLOT(addMarker()));

      QAction* markerDelete = getAction("delete", this);
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
      connect(table, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), SLOT(currentChanged(QTreeWidgetItem*)));
      connect(table, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));
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
      connect(song, SIGNAL(markerChanged(int)), SLOT(markerChanged(int)));

      vbox->addWidget(table);
      vbox->addWidget(props);

      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

//      connect(song, SIGNAL(songChanged(int)), SLOT(updateList()));
      updateList();
      markerChanged(Song::MARKER_CUR);    // select current marker
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
//      new MarkerItem(table, m);
      song->addMarker(QString(""), pos);
      table->sortItems(0, Qt::AscendingOrder);
      }

//---------------------------------------------------------
//   deleteMarker
//---------------------------------------------------------

void MarkerView::deleteMarker()
      {
      MarkerItem* item = (MarkerItem*)table->currentItem();
      if (item) {
            AL::Marker* marker = item->marker();
            delete item;
            song->removeMarker(marker);
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
            new MarkerItem(table, m);
            }
      }

//---------------------------------------------------------
//   currentChanged
//---------------------------------------------------------

void MarkerView::currentChanged(QTreeWidgetItem* i)
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

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void MarkerView::selectionChanged()
      {
#if 0
      QList<QTreeWidgetItem*> sel = table->selectedItems();
      if (!sel.empty()) {
            MarkerItem* item = (MarkerItem*)(sel[0]);
            }
#endif
      }

//---------------------------------------------------------
//   clicked
//---------------------------------------------------------

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
            lockChange = true;
            item->setTick(pos.tick());
            Pos p(pos.tick(), AL::TICKS);
            song->setPos(0, p, true, true, false);
            lockChange = false;
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
//   markerChanged
//---------------------------------------------------------

void MarkerView::markerChanged(int val)
      {
      if (lockChange)
            return;
      switch (val) {
            case Song::MARKER_ADD:
            case Song::MARKER_REMOVE:
                  updateList();
                  // fall through
            case Song::MARKER_CUR:
                  {
                  AL::MarkerList* marker = song->marker();
                  for (AL::iMarker i = marker->begin(); i != marker->end(); ++i) {
                        if (i->second.current()) {
                              int n = table->topLevelItemCount();
                              for (int k = 0; k < n; ++k) {
                                    MarkerItem* item = (MarkerItem*)(table->topLevelItem(k));
                                    if (item->marker() == &i->second) {
                                          table->setCurrentItem(item);
                                          return;
                                          }
                                    }
                              }
                        }
                  }
                  break;
            case Song::MARKER_NAME:
            case Song::MARKER_TICK:
            case Song::MARKER_LOCK:
                  break;
            }
      }

