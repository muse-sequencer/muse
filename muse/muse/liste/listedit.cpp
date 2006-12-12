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

#include "listedit.h"
#include "ctrllistedit.h"
#include "partlistedit.h"
#include "tracklistedit.h"
#include "song.h"
#include "part.h"
#include "ctrl.h"

//---------------------------------------------------------
//   operator==
//---------------------------------------------------------

bool ListType::operator==(const ListType& t) const
      {
      return id == t.id && track == t.track
         && part == t.part && ctrl->id() == t.ctrl->id();
      }

//---------------------------------------------------------
//   ListEdit
//---------------------------------------------------------

ListEdit::ListEdit(QWidget*)
   : TopWin()
      {
      setWindowTitle(tr("MusE: List Edit"));

      QSplitter* split = new QSplitter;
      split->setOpaqueResize(true);
      setCentralWidget(split);

      list = new QTreeWidget;
      list->setColumnCount(1);
      list->setSelectionMode(QAbstractItemView::SingleSelection);
      list->setRootIsDecorated(true);
      list->setColumnCount(1);
      list->setHeaderLabels(QStringList("Element"));
      list->setSortingEnabled(false);
      list->setUniformRowHeights(true);
      split->addWidget(list);

      stack = new QStackedWidget;
      split->addWidget(stack);

      ctrlPanel = new CtrlListEditor(this);
      stack->addWidget(ctrlPanel);
      partPanel = new PartListEditor(this);
      stack->addWidget(partPanel);
      trackPanel = new TrackListEditor(this);
      stack->addWidget(trackPanel);

      connect(list, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
         SLOT(itemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
      connect(list, SIGNAL(itemExpanded(QTreeWidgetItem*)), SLOT(itemExpanded(QTreeWidgetItem*)));
      connect(list, SIGNAL(itemCollapsed(QTreeWidgetItem*)), SLOT(itemExpanded(QTreeWidgetItem*)));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      list->resizeColumnToContents(0);
      resize(900, 400);
      }

//---------------------------------------------------------
//   itemExpanded
//---------------------------------------------------------

void ListEdit::itemExpanded(QTreeWidgetItem*)
      {
      list->resizeColumnToContents(0);
      }

//---------------------------------------------------------
//   itemChanged
//---------------------------------------------------------

void ListEdit::itemChanged(QTreeWidgetItem* i, QTreeWidgetItem*)
      {
      if (i == 0)
            return;
      ListWidget* ew = ctrlPanel;
      lt = i->data(0, Qt::UserRole).value<ListType>();
      switch(lt.id) {
            case LIST_TRACK:
                  ew = trackPanel;
                  break;
            case LIST_PART:
                  ew = partPanel;
                  break;
            case LIST_CTRL:
                  ew = ctrlPanel;
                  break;
            default:
                  return;
            }
      ew->setup(lt);
      stack->setCurrentWidget(ew);
      }

//---------------------------------------------------------
//   buildList
//---------------------------------------------------------

void ListEdit::buildList()
      {
      list->clear();
      TrackList* tl = song->tracks();
      int idx = 0;
      ListType lt;

      for (iTrack i = tl->begin(); i != tl->end(); ++i,++idx) {
            QTreeWidgetItem* item = new QTreeWidgetItem;
            Track* t = *i;
            item->setText(0, t->name());
            lt.id    = LIST_TRACK;
            lt.track = t;
            item->setData(0, Qt::UserRole, QVariant::fromValue(lt));
            list->insertTopLevelItem(idx, item);

            PartList* pl = t->parts();
            if (!pl->empty()) {
                  QTreeWidgetItem* pitem = new QTreeWidgetItem(item);
                  pitem->setFlags(pitem->flags() & ~Qt::ItemIsSelectable);
                  pitem->setText(0, tr("Parts"));
                  lt.id = LIST_NONE;
                  pitem->setData(0, Qt::UserRole, QVariant::fromValue(lt));
                  for (iPart pi = pl->begin(); pi != pl->end(); ++pi) {
                        lt.id   = LIST_PART;
                        lt.part = pi->second;
                        QTreeWidgetItem* ppitem = new QTreeWidgetItem(pitem);
                        ppitem->setData(0, Qt::UserRole, QVariant::fromValue(lt));
                        ppitem->setText(0, pi->second->name());
                        }
                  }

            CtrlList* cl = t->controller();
            lt.part = 0;
            if (!cl->empty()) {
                  QTreeWidgetItem* citem = new QTreeWidgetItem(item);
                  citem->setText(0, tr("Controller"));
                  citem->setFlags(citem->flags() & ~Qt::ItemIsSelectable);
                  lt.id = LIST_NONE;
                  citem->setData(0, Qt::UserRole, QVariant::fromValue(lt));
                  for (iCtrl ci = cl->begin(); ci != cl->end(); ++ci) {
                        QTreeWidgetItem* ccitem = new QTreeWidgetItem(citem);
                        ccitem->setText(0, ci->second->name());
                        lt.id   = LIST_CTRL;
                        lt.ctrl = ci->second;
                        ccitem->setData(0, Qt::UserRole, QVariant::fromValue(lt));
                        }
                  }
            }
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void ListEdit::songChanged(int flags)
      {
      if (flags & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_PART_INSERTED
         | SC_PART_REMOVED)) {
            buildList();
            selectItem();
            }
      }

//---------------------------------------------------------
//   findItem
//---------------------------------------------------------

QTreeWidgetItem* ListEdit::findItem(const ListType& lt, QTreeWidgetItem* item)
      {
      if (item->flags() & Qt::ItemIsSelectable) {
            if (lt == item->data(0, Qt::UserRole).value<ListType>())
                  return item;
            }
      for (int n = 0; n < item->childCount(); ++n) {
            QTreeWidgetItem* i = findItem(lt, item->child(n));
            if (i)
                  return i;
            }
      return 0;
      }

//---------------------------------------------------------
//   selectItem
//---------------------------------------------------------

void ListEdit::selectItem(const AL::Pos& p, Track* track, Part* part, Ctrl* ctrl)
      {
      _pos = p;
      stack->setCurrentWidget(ctrlPanel);
      if (ctrl)
            lt.id = LIST_CTRL;
      else if (part)
            lt.id = LIST_PART;
      else if (track)
            lt.id = LIST_TRACK;
      else
            return;
      lt.track = track;
      lt.part  = part;
      lt.ctrl  = ctrl;
      selectItem();
      }

void ListEdit::selectItem()
      {
      stack->setCurrentWidget(ctrlPanel);
      buildList();
      for (int i = 0;; ++i) {
            QTreeWidgetItem* item = list->topLevelItem(i);
            if (item == 0) {
                  printf("MusE::ListEdit: Element not found\n");
                  break;
                  }
            item = findItem(lt, item);
            if (item) {
                  list->setItemExpanded(item, true);
                  list->setCurrentItem(item);
                  list->scrollToItem(item);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void ListEdit::read(QDomNode node)
      {
      QString trackName;
      Track* track = 0;
      Part* part = 0;
      Ctrl* ctrl = 0;

      for (node = node.firstChild(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            if (tag == "Track") {
                  track = song->findTrack(e.text());
                  if (track == 0) {
                        printf("MusE::ListEdit::read: track not found\n");
                        }
                  }
            else if (tag == "Pos")
                  _pos.read(node);
            else if (tag == "Ctrl") {
                  int ctrlId = e.text().toInt();
                  ctrl = track->getController(ctrlId);
                  if (ctrl == 0) {
                        printf("MusE::ListEdit::read: controller not found: track %p\n", track);
                        printf("MusE::ListEdit::read: controller %d not found\n", ctrlId);
                        return;
                        }
                  }
            else
      		AL::readProperties(this, node);
            }
      selectItem(_pos, track, part, ctrl);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void ListEdit::write(Xml& xml) const
      {
      xml.stag(metaObject()->className());
      xml.writeProperties(this);

      xml.tag("Track", lt.track->name());
      if (lt.ctrl) {
            xml.tag("Ctrl", lt.ctrl->id());
            _pos.write(xml, "Pos");
            }
      xml.etag(metaObject()->className());
      }

