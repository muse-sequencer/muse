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

#include "icons.h"
#include "gconfig.h"
#include "midirack.h"
#include "track.h"
#include "song.h"
#include "midiplugin.h"
#include "audio.h"
#include "muse.h"
#include "gui.h"

//---------------------------------------------------------
//   MidiRack
//---------------------------------------------------------

MidiRack::MidiRack(QWidget* parent, MidiTrackBase* t)
   : QListWidget(parent)
      {
      setUniformItemSizes(true);
      setAlternatingRowColors(true);
      setAttribute(Qt::WA_DeleteOnClose, true);
      verticalScrollBar()->setStyle(smallStyle);
      track = t;
//      setFont(config.fonts[1]);

      setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

      setSelectionMode(QAbstractItemView::SingleSelection);
      songChanged(SC_RACK);   // force update
      connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
         this, SLOT(doubleClicked(QListWidgetItem*)));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      setToolTip(tr("midi effect rack"));
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize MidiRack::sizeHint() const
      {
	QFontMetrics fm(font());
	int h = fm.lineSpacing() * MidiPipelineDepth + 1;
	return QSize(STRIP_WIDTH, h);
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiRack::songChanged(int typ)
      {
      if (typ & (SC_ROUTE | SC_RACK)) {
            clear();
            foreach(MidiPluginI* plugin, *(track->pipeline())) {
                  QListWidgetItem* item = new QListWidgetItem;
                  item->setText(plugin->name());
                  // tooltip should only be set if name does not fit
                  // (is elided)
                  item->setToolTip(plugin->name());
                  item->setBackgroundColor(plugin->on() ? Qt::white : Qt::gray);
                  addItem(item);
                  }
            }
      }

//---------------------------------------------------------
//   contextMenuEvent
//---------------------------------------------------------

void MidiRack::contextMenuEvent(QContextMenuEvent* ev)
      {
      QPoint pt(ev->pos());
      QListWidgetItem* item = itemAt(pt);
      MidiPipeline* pipe    = track->pipeline();

      QMenu* menu           = new QMenu;
      QAction* upAction     = menu->addAction(QIcon(*upIcon), tr("move up"));
      QAction* downAction   = menu->addAction(QIcon(*downIcon), tr("move down"));
      QAction* removeAction = menu->addAction(tr("remove"));
      QAction* bypassAction = menu->addAction(tr("bypass"));
      QAction* showAction   = menu->addAction(tr("show gui"));
      QAction* newAction    = menu->addAction(tr("new"));
      bypassAction->setCheckable(true);
      showAction->setCheckable(true);

      int idx = -1;
      if (item == 0) {
            upAction->setEnabled(false);
            downAction->setEnabled(false);
            removeAction->setEnabled(false);
            bypassAction->setEnabled(false);
            showAction->setEnabled(false);
            }
      else {
            idx = row(item);
            upAction->setEnabled(idx != 0);
            downAction->setEnabled(idx < (pipe->size() - 1));
            showAction->setEnabled(pipe->hasGui(idx));
            bypassAction->setEnabled(true);
            bypassAction->setChecked(!pipe->isOn(idx));
            showAction->setChecked(pipe->guiVisible(idx));
            }

      QAction* sel = menu->exec(mapToGlobal(pt), newAction);
      delete menu;
      if (sel == 0)
            return;

      if (sel == newAction) {
            selectNew();
            return;
            }
      if (sel == removeAction) {
            audio->msgAddMidiPlugin(track, idx, 0);
            }
      else if (sel == bypassAction) {
            bool flag = !pipe->isOn(idx);
            pipe->setOn(idx, flag);
            }
      else if (sel == showAction) {
            bool flag = !pipe->guiVisible(idx);
            pipe->showGui(idx, flag);
            }
      else if (sel == upAction) {
            if (idx > 0) {
                  setCurrentRow(idx-1);
                  pipe->move(idx, true);
                  }
            }
      else if (sel == downAction) {
            if (idx < (pipe->size() - 1)) {
                  setCurrentRow(idx+1);
                  pipe->move(idx, false);
                  }
            }
      song->update(SC_RACK);
      }

//---------------------------------------------------------
//   doubleClicked
//    toggle gui
//---------------------------------------------------------

void MidiRack::doubleClicked(QListWidgetItem* it)
      {
      if (track == 0)
            return;
      int idx = row(it);
      MidiPipeline* pipe = track->pipeline();
      bool flag = !pipe->guiVisible(idx);
      pipe->showGui(idx, flag);
      song->update(SC_RACK);
      }

//---------------------------------------------------------
//   mouseDoubleClickEvent
//---------------------------------------------------------

void MidiRack::mouseDoubleClickEvent(QMouseEvent* event)
      {
      QListWidgetItem* it = itemAt(event->pos());
      if (it || (track == 0)) {
            QListWidget::mouseDoubleClickEvent(event);
            return;
            }
      selectNew();
      }

//---------------------------------------------------------
//   selectNew
//---------------------------------------------------------

void MidiRack::selectNew()
      {
      MidiPlugin* plugin = MidiPluginDialog::getPlugin(this);
      if (plugin) {
            MidiPluginI* plugi = plugin->instantiate(track);
            if (plugi == 0) {
                  printf("cannot instantiate plugin <%s>\n",
                     plugin->name().toLatin1().data());
                  delete plugi;
                  }
            else
                  audio->msgAddMidiPlugin(track, track->pipeline()->size(), plugi);
            song->update(SC_RACK);
            }
      }

