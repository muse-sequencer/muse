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
#include "al/pos.h"
#include "ctrllistedit.h"
#include "song.h"
#include "part.h"
#include "ctrl.h"

//---------------------------------------------------------
//   ListEdit
//---------------------------------------------------------

ListEdit::ListEdit(QWidget*)
   : QWidget(0)
      {
      setWindowTitle(tr("MusE: List Edit"));

      QHBoxLayout* hbox = new QHBoxLayout;
      setLayout(hbox);

      QSplitter* split = new QSplitter;
      split->setOpaqueResize(true);
      hbox->addWidget(split);

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
      
      ctrlPanel = new CtrlListEditor;
      stack->addWidget(ctrlPanel);

      connect(list, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
         SLOT(itemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
      connect(list, SIGNAL(itemExpanded(QTreeWidgetItem*)), SLOT(itemExpanded(QTreeWidgetItem*)));
      connect(list, SIGNAL(itemCollapsed(QTreeWidgetItem*)), SLOT(itemExpanded(QTreeWidgetItem*)));
      list->resizeColumnToContents(0);
      resize(900, 300);
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
      QWidget* ew = ctrlPanel;
#if 0
      Element* el = ((ElementItem*)i)->element();
      setWindowTitle(QString("MuseScore: List Edit: ") + el->name());
      ShowElementBase* ew = 0;
      switch (el->type()) {
            case PAGE:    ew = pagePanel;    break;
            case SYSTEM:  ew = systemPanel;  break;
            case MEASURE: ew = measurePanel; break;
            case CHORD:   ew = chordPanel;   break;
            case NOTE:    ew = notePanel;    break;
            case REST:    ew = restPanel;    break;
            case CLEF:    ew = clefPanel;    break;
            case TIMESIG: ew = timesigPanel; break;
            case KEYSIG:  ew = keysigPanel;  break;
            case SEGMENT: ew = segmentView;  break;
            case HAIRPIN: ew = hairpinView;  break;
            case BAR_LINE: ew = barLineView; break;
            case FINGERING:
            case TEXT:
                  ew = textView;
                  break;
            case ACCIDENTAL:
            default:
                  ew = elementView;
                  break;
            }
      ew->setElement(el);
#endif
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
      for (iTrack i = tl->begin(); i != tl->end(); ++i,++idx) {
            Track* t = *i;
            QTreeWidgetItem* item = new QTreeWidgetItem;
            item->setText(0, t->name());
            list->insertTopLevelItem(idx, item);
            PartList* pl = t->parts();
            if (!pl->empty()) {
                  QTreeWidgetItem* pitem = new QTreeWidgetItem(item);
                  pitem->setText(0, tr("Parts"));
                  for (iPart i = pl->begin(); i != pl->end(); ++i) {
                        QTreeWidgetItem* ppitem = new QTreeWidgetItem(pitem);
                        ppitem->setText(0, i->second->name());
                        }
                  }
            CtrlList* cl = t->controller();
            if (!cl->empty()) {
                  QTreeWidgetItem* citem = new QTreeWidgetItem(item);
                  citem->setText(0, tr("Controller"));
                  for (iCtrl i = cl->begin(); i != cl->end(); ++i) {
                        QTreeWidgetItem* ccitem = new QTreeWidgetItem(citem);
                        ccitem->setText(0, i->second->name());
                        }
                  }
            }
      }

//---------------------------------------------------------
//   selectItem
//---------------------------------------------------------

void ListEdit::selectItem(const AL::Pos&, Track* track, Ctrl*)
      {
      stack->setCurrentWidget(ctrlPanel);
      buildList();
      for (int i = 0;; ++i) {
            QTreeWidgetItem* item = list->topLevelItem(i);
            if (item == 0) {
                  printf("MusE::ListEdit: Element not found\n");
                  break;
                  }
            if (item->text(0) == track->name()) {
                  list->setItemExpanded(item, true);
                  list->setCurrentItem(item);
                  list->scrollToItem(item);
                  break;
                  }
            }
      }

