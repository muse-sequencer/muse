//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midirack.cpp,v 1.11 2006/01/12 14:49:13 wschweer Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#include "icons.h"
#include "gconfig.h"
#include "midirack.h"
#include "track.h"
#include "song.h"
#include "midiplugin.h"
#include "audio.h"

//---------------------------------------------------------
//   MidiRack
//---------------------------------------------------------

MidiRack::MidiRack(QWidget* parent, MidiTrackBase* t)
   : QListWidget(parent)
      {
      setAttribute(Qt::WA_DeleteOnClose, true);
      track = t;
      setFont(*config.fonts[1]);

      setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

      setSelectionMode(QAbstractItemView::SingleSelection);
      for (int i = 0; i < MidiPipelineDepth; ++i) {
            QListWidgetItem* item = new QListWidgetItem;
            item->setText(t->pipeline()->name(i));
            item->setBackgroundColor(t->pipeline()->isOn(i) ? Qt::white : Qt::gray);
            addItem(item);
            }
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
	return QSize(100, h);
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiRack::songChanged(int typ)
      {
      if (typ & (SC_ROUTE | SC_RACK)) {
            for (int i = 0; i < MidiPipelineDepth; ++i) {
                  QListWidgetItem* it = item(i);
                  it->setText(track->pipeline()->name(i));
                  it->setBackgroundColor(track->pipeline()->isOn(i) ? Qt::white : Qt::gray);
                  }
            }
      }

//---------------------------------------------------------
//   menuRequested
//---------------------------------------------------------

void MidiRack::contextMenuEvent(QContextMenuEvent* ev)
      {
      QPoint pt(ev->pos());
      QListWidgetItem* item = itemAt(pt);

      if (item == 0 || track == 0)
            return;

      int idx = row(item);
      QString name;
      bool mute;

      MidiPipeline* pipe = track->pipeline();
      name  = pipe->name(idx);
      mute  = pipe->isOn(idx);

      QMenu* menu = new QMenu;

      QAction* upAction = menu->addAction(QIcon(*upIcon), tr("move up"));
      QAction* downAction = menu->addAction(QIcon(*downIcon), tr("move down"));
      QAction* removeAction = menu->addAction(tr("remove"));
      QAction* bypassAction = menu->addAction(tr("bypass"));
      QAction* showAction   = menu->addAction(tr("show gui"));
      QAction* newAction;

      bypassAction->setChecked(!pipe->isOn(idx));
      showAction->setChecked(pipe->guiVisible(idx));

      if (pipe->empty(idx)) {
            newAction = menu->addAction(tr("new"));
            upAction->setEnabled(false);
            downAction->setEnabled(false);
            removeAction->setEnabled(false);
            bypassAction->setEnabled(false);
            showAction->setEnabled(false);
            }
      else {
            newAction = menu->addAction(tr("change"));
            if (idx == 0)
                  upAction->setEnabled(false);
            if (idx == (MidiPipelineDepth-1))
                  downAction->setEnabled(false);
            }

      QAction* sel = menu->exec(mapToGlobal(pt), newAction);
      delete menu;
      if (sel == 0)
            return;

      if (sel == newAction) {
            MidiPlugin* plugin = MidiPluginDialog::getPlugin(this);
            if (plugin) {
                  MidiPluginI* plugi = plugin->instantiate(track);
                  if (plugi == 0) {
                        printf("cannot instantiate plugin <%s>\n",
                           plugin->name().toLatin1().data());
                        delete plugi;
                        }
                  else
                        audio->msgAddMidiPlugin(track, idx, plugi);
                  }
            }
      else if (sel == removeAction) {
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
            if (idx < (MidiPipelineDepth-1)) {
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
      if (it == 0 || track == 0)
            return;

      int idx = row(it);
      MidiPipeline* pipe = track->pipeline();

      if (!pipe->empty(idx)) {
            bool flag = !pipe->guiVisible(idx);
            pipe->showGui(idx, flag);
            }
      else {
            MidiPlugin* plugin = MidiPluginDialog::getPlugin(this);
            if (plugin) {
                  MidiPluginI* plugi = plugin->instantiate(track);
                  if (plugi == 0) {
                        printf("cannot instantiate plugin <%s>\n",
                           plugin->name().toLatin1().data());
                        delete plugi;
                        }
                  else {
                        audio->msgAddMidiPlugin(track, idx, plugi);
                        }
                  }
            }
      }

