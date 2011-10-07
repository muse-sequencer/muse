//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: routedialog.cpp,v 1.5.2.2 2007/01/04 00:35:17 terminator356 Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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

#include <QCloseEvent>
#include <QDialog>
#include <QListWidgetItem>
#include <QTreeWidgetItem>

#include "routedialog.h"
#include "track.h"
#include "song.h"
#include "audio.h"
#include "driver/jackaudio.h"

namespace MusEGui {

//---------------------------------------------------------
//   RouteDialog
//---------------------------------------------------------

RouteDialog::RouteDialog(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      connect(routeList, SIGNAL(itemSelectionChanged()), SLOT(routeSelectionChanged()));
      connect(newSrcList, SIGNAL(itemSelectionChanged()), SLOT(srcSelectionChanged()));
      connect(newDstList, SIGNAL(itemSelectionChanged()), SLOT(dstSelectionChanged()));
      connect(removeButton, SIGNAL(clicked()), SLOT(removeRoute()));
      connect(connectButton, SIGNAL(clicked()), SLOT(addRoute()));
      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      routingChanged();
      }

//---------------------------------------------------------
//   routingChanged
//---------------------------------------------------------

void RouteDialog::routingChanged()
      {
      //---------------------------------------------------
      //  populate lists
      //---------------------------------------------------

      routeList->clear();
      newSrcList->clear();
      newDstList->clear();

      MusECore::TrackList* tl = MusEGlobal::song->tracks();
      for (MusECore::ciTrack i = tl->begin(); i != tl->end(); ++i) {
            if ((*i)->isMidiTrack())
                  continue;
            // p3.3.38
            //WaveTrack* track = (WaveTrack*)(*i);
            MusECore::AudioTrack* track = (MusECore::AudioTrack*)(*i);
            if (track->type() == MusECore::Track::AUDIO_INPUT) {
                  for (int channel = 0; channel < track->channels(); ++channel)
                        newDstList->addItem(MusECore::Route(track, channel).name());
                  const MusECore::RouteList* rl = track->inRoutes();
                  for (MusECore::ciRoute r = rl->begin(); r != rl->end(); ++r) {
                        //MusECore::Route dst(track->name(), true, r->channel);
                        MusECore::Route dst(track->name(), true, r->channel, MusECore::Route::TRACK_ROUTE);
                        new QTreeWidgetItem(routeList, QStringList() << r->name() << dst.name());
                        }
                  }
            else if (track->type() != MusECore::Track::AUDIO_AUX)
                  newDstList->addItem(MusECore::Route(track, -1).name());
            if (track->type() == MusECore::Track::AUDIO_OUTPUT) {
                  for (int channel = 0; channel < track->channels(); ++channel) {
                        MusECore::Route r(track, channel);
                        newSrcList->addItem(r.name());
                        }
                  }
            else
                  newSrcList->addItem(MusECore::Route(track, -1).name());

            const MusECore::RouteList* rl = track->outRoutes();
            for (MusECore::ciRoute r = rl->begin(); r != rl->end(); ++r) {
                  QString src(track->name());
                  if (track->type() == MusECore::Track::AUDIO_OUTPUT) {
                        MusECore::Route s(src, false, r->channel);
                        src = s.name();
                        }
                  new QTreeWidgetItem(routeList, QStringList() << src << r->name());
                  }
            }
      if (!MusEGlobal::checkAudioDevice()) return;
      std::list<QString> sl = MusEGlobal::audioDevice->outputPorts();
      for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i)
            newSrcList->addItem(*i);
      sl = MusEGlobal::audioDevice->inputPorts();
      for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i)
            newDstList->addItem(*i);
      routeSelectionChanged();   // init remove button
      srcSelectionChanged();     // init select button
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void RouteDialog::songChanged(int v)
      {
      if (v & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_ROUTE)) {
            routingChanged();
            }
      }

//---------------------------------------------------------
//   routeSelectionChanged
//---------------------------------------------------------

void RouteDialog::routeSelectionChanged()
      {
      QTreeWidgetItem* item = routeList->currentItem();
      removeButton->setEnabled(item != 0);
      }

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

void RouteDialog::removeRoute()
      {
      QTreeWidgetItem* item = routeList->currentItem();
      if (item == 0)
            return;
      MusEGlobal::audio->msgRemoveRoute(MusECore::Route(item->text(0), false, -1), MusECore::Route(item->text(1), true, -1));
      MusEGlobal::audio->msgUpdateSoloStates();
      MusEGlobal::song->update(SC_SOLO);
      delete item;
      }

//---------------------------------------------------------
//   addRoute
//---------------------------------------------------------

void RouteDialog::addRoute()
      {
      QListWidgetItem* srcItem = newSrcList->currentItem();
      QListWidgetItem* dstItem = newDstList->currentItem();
      if (srcItem == 0 || dstItem == 0)
            return;
      MusEGlobal::audio->msgAddRoute(MusECore::Route(srcItem->text(), false, -1), MusECore::Route(dstItem->text(), true, -1));
      MusEGlobal::audio->msgUpdateSoloStates();
      MusEGlobal::song->update(SC_SOLO);
      new QTreeWidgetItem(routeList, QStringList() << srcItem->text() << dstItem->text());
      }

//---------------------------------------------------------
//   srcSelectionChanged
//---------------------------------------------------------

void RouteDialog::srcSelectionChanged()
      {
      QListWidgetItem* srcItem = newSrcList->currentItem();
      QListWidgetItem* dstItem = newDstList->currentItem();
      connectButton->setEnabled((srcItem != 0)
         && (dstItem != 0)
         && MusECore::checkRoute(srcItem->text(), dstItem->text()));
      }

//---------------------------------------------------------
//   dstSelectionChanged
//---------------------------------------------------------

void RouteDialog::dstSelectionChanged()
      {
      QListWidgetItem* dstItem = newDstList->currentItem();
      QListWidgetItem* srcItem = newSrcList->currentItem();
      connectButton->setEnabled((srcItem != 0)
         && (dstItem != 0)
         && MusECore::checkRoute(srcItem->text(), dstItem->text()));
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void RouteDialog::closeEvent(QCloseEvent* e)
      {
      emit closed();
      e->accept();
      }

} // namespace MusEGui
