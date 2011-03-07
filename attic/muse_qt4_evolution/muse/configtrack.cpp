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

#include "configtrack.h"
#include "trelement.h"
#include "arranger.h"

//---------------------------------------------------------
//   ConfigTrackList
//---------------------------------------------------------

ConfigTrackList::ConfigTrackList(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      // does not work:
      configuredList->setSelectionMode(QAbstractItemView::SingleSelection);
      availableList->setSelectionMode(QAbstractItemView::SingleSelection);

      up->setEnabled(false);
      down->setEnabled(false);
      addItem->setEnabled(false);
      removeItem->setEnabled(false);
      dirty = false;
      curType = 0;
      init();

      connect(trackType, SIGNAL(activated(int)), SLOT(trackTypeChanged(int)));
      connect(availableList, SIGNAL(itemSelectionChanged()), SLOT(availableSelected()));
      connect(configuredList, SIGNAL(itemSelectionChanged()), SLOT(configuredSelected()));
      connect(up, SIGNAL(clicked()), SLOT(upClicked()));
      connect(down, SIGNAL(clicked()), SLOT(downClicked()));
      connect(addItem, SIGNAL(clicked()), SLOT(addItemClicked()));
      connect(removeItem, SIGNAL(clicked()), SLOT(removeItemClicked()));
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void ConfigTrackList::init()
      {
      trackType->clear();
      for (int tt = 0; tt < Track::TRACK_TYPES; ++tt) {
            list[tt] = glist[tt];
            trackType->addItem(Track::_clname[tt]);
            }
      trackTypeChanged(curType);
      }

//---------------------------------------------------------
//   done
//---------------------------------------------------------

void ConfigTrackList::done(int code)
      {
      if (code) {
            // OK pressed
            if (dirty)
                  saveTrackType();
            for (int tt = 0; tt < Track::TRACK_TYPES; ++tt)
                  glist[tt] = list[tt];
            emit trackConfigChanged();
            }
      else {
            dirty = false;
            init();
            }
      QDialog::done(code);
      }

//---------------------------------------------------------
//   saveTrackType
//---------------------------------------------------------

void ConfigTrackList::saveTrackType()
      {
      dirty = false;
      list[curType].clear();
      int gn = 0;
      TrElementList group;
      int n = configuredList->count();
      for (int i = 0; i < n; ++i) {
            QString s = configuredList->item(i)->text();
            int k;
            for (k = 0; k < nTrElements; ++k)
                  if (trElements[k].name == s)
                        break;
            if (trElements[k].grp != gn) {
                  list[curType].push_back(group);
                  group.clear();
                  gn = trElements[k].grp;
                  }
            group.push_back(&trElements[k]);
            }
      if (!group.empty())
            list[curType].push_back(group);
      }

//---------------------------------------------------------
//   trackTypeChanged
//---------------------------------------------------------

void ConfigTrackList::trackTypeChanged(int type)
      {
      curType = type;
      if (dirty)
            saveTrackType();

      configuredList->clear();
      TrGroupList& gl = list[type];
      for (iTrGroup i = gl.begin(); i != gl.end(); ++i) {
            for (iTrElement k = i->begin(); k != i->end(); ++k)
                  configuredList->addItem((*k)->name);
            }

      availableList->clear();
      for (int i = 0; i < nTrElements; ++i) {
            QString name(trElements[i].name);
            bool f = false;

            // is gui element available for this track type?

            for (int i = 0; i < nTrElements; ++i) {
                  const TrElement&  el = trElements[i];
                  if (el.name == name) {
                        if (el.trackMask & (1 << type)) {
                              f = true;
                              break;
                              }
                        }
                  }
            if (!f)
                  continue;

            // is gui element already configured?

            for (iTrGroup i = gl.begin(); i != gl.end(); ++i) {
                  for (iTrElement k = i->begin(); k != i->end(); ++k) {
                        if (name == (*k)->name) {
                              f = false;
                              break;
                              }
                        }
                  if (!f)
                        break;
                  }
            if (f) {
                  availableList->addItem(name);
                  }
            }
      }

//---------------------------------------------------------
//   configuredSelected
//---------------------------------------------------------

void ConfigTrackList::configuredSelected()
      {
      QListWidgetItem* item = configuredList->selectedItems().at(0);
      up->setEnabled(item != 0);
      down->setEnabled(item != 0);
      removeItem->setEnabled(item != 0);
      }

//---------------------------------------------------------
//   availableSelected
//---------------------------------------------------------

void ConfigTrackList::availableSelected()
      {
      QListWidgetItem* item = availableList->selectedItems().at(0);
      addItem->setEnabled(item != 0);
      }

//---------------------------------------------------------
//   upClicked
//---------------------------------------------------------

void ConfigTrackList::upClicked()
      {
      QListWidgetItem* item = configuredList->selectedItems().at(0);
      int n = configuredList->row(item);
      if (n <= 0)
            return;
      QString s = item->text();
      delete item;
      configuredList->insertItem(n-1, s);
      configuredList->setItemSelected(configuredList->item(n-1), true);
      dirty = true;
      }

//---------------------------------------------------------
//   downClicked
//---------------------------------------------------------

void ConfigTrackList::downClicked()
      {
      QListWidgetItem* item = configuredList->selectedItems().at(0);
      int n = configuredList->row(item);
      if (n >= int(configuredList->count()-1))
            return;
      QString s = item->text();

      delete item;
      configuredList->insertItem(n+1, s);
      configuredList->setItemSelected(configuredList->item(n+1), true);
      dirty = true;
      }

//---------------------------------------------------------
//   addItemClicked
//---------------------------------------------------------

void ConfigTrackList::addItemClicked()
      {
      QListWidgetItem* item = availableList->selectedItems().at(0);
      if (item == 0)
            return;
      QString s = item->text();
      delete item;
      configuredList->addItem(s);
      configuredList->setItemSelected(configuredList->item(configuredList->count()-1), true);
      QListWidgetItem* ci = availableList->currentItem();
      if (ci)
            availableList->setItemSelected(ci, true);
      dirty = true;
      }

//---------------------------------------------------------
//   removeItemClicked
//---------------------------------------------------------

void ConfigTrackList::removeItemClicked()
      {
      QListWidgetItem* item = configuredList->selectedItems().at(0);
      if (item == 0)
            return;
      QString s = item->text();
      delete item;
      availableList->addItem(s);
      availableList->setItemSelected(availableList->item(availableList->count()-1), true);
      QListWidgetItem* ci = configuredList->item(configuredList->currentRow());
      if (ci)
            configuredList->setItemSelected(ci, true);
      dirty = true;
      }

