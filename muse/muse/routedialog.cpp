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

#include "routedialog.h"
#include "track.h"
#include "song.h"
#include "audio.h"
#include "jackaudio.h"

//---------------------------------------------------------
//   RouteDialog
//---------------------------------------------------------

RouteDialog::RouteDialog(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      connect(routeList, SIGNAL(selectionChanged()), SLOT(routeSelectionChanged()));
      connect(newSrcList, SIGNAL(selectionChanged()), SLOT(srcSelectionChanged()));
      connect(newDstList, SIGNAL(selectionChanged()), SLOT(dstSelectionChanged()));
      connect(removeButton, SIGNAL(clicked()), SLOT(removeRoute()));
      connect(connectButton, SIGNAL(clicked()), SLOT(addRoute()));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
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

#if 0 //TODO3
      TrackList* tl = song->tracks();
      for (ciTrack i = tl->begin(); i != tl->end(); ++i) {
            if ((*i)->isMidiTrack())
                  continue;
            Track* track = *i;
            if (track->type() == Track::AUDIO_INPUT) {
                  for (int channel = 0; channel < track->channels(); ++channel)
                        newDstList->insertItem(Route(track, channel).name());
                  const RouteList* rl = track->inRoutes();
                  for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
                        Route dst(track->name(), r->channel, Route::AUDIOPORT);
                        new QListWidgetItem(routeList, r->name(), dst.name());
                        }
                  }
            else
                  newDstList->insertItem(Route(track, -1).name());
            if (track->type() == Track::AUDIO_OUTPUT) {
                  for (int channel = 0; channel < track->channels(); ++channel) {
                        Route r(track, channel);
                        newSrcList->insertItem(r.name());
                        }
                  }
            else
                  newSrcList->insertItem(Route(track, -1).name());

            const RouteList* rl = track->outRoutes();
            for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
                  QString src(track->name());
                  if (track->type() == Track::AUDIO_OUTPUT) {
                        Route s(src, r->channel, Route::AUDIOPORT);
                        src = s.name();
                        }
                  new QListWidgetItem(routeList, src, r->name());
                  }
            }

      std::list<QString> sl = audioDriver->outputPorts();
      for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i)
            newSrcList->insertItem(*i);
      sl = audioDriver->inputPorts();
      for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i)
            newDstList->insertItem(*i);
#endif
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
//TD      QListWidgetItem* item = routeList->selectedItem();
//      removeButton->setEnabled(item != 0);
      }

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

void RouteDialog::removeRoute()
      {
#if 0 //TD
      QListWidgetItem* item = routeList->selectedItem();
      if (item == 0)
            return;
      audio->msgRemoveRoute(Route(item->text(0), -1, Route::TRACK), Route(item->text(1), -1, Route::AUDIOPORT));
      delete item;
#endif
      }

//---------------------------------------------------------
//   addRoute
//---------------------------------------------------------

void RouteDialog::addRoute()
      {
#if 0 //TD
      QListWidgetItem* srcItem = newSrcList->selectedItem();
      QListWidgetItem* dstItem = newDstList->selectedItem();
      if (srcItem == 0 || dstItem == 0)
            return;
      audio->msgAddRoute(Route(srcItem->text(), -1, Route::TRACK), Route(dstItem->text(), -1, Route::AUDIOPORT));
      new QListWidgetItem(routeList, srcItem->text(), dstItem->text());
#endif
      }

//---------------------------------------------------------
//   srcSelectionChanged
//---------------------------------------------------------

void RouteDialog::srcSelectionChanged()
      {
#if 0 //TD
      QListWidgetItem* srcItem = newSrcList->selectedItem();
      QListWidgetItem* dstItem = newDstList->selectedItem();
      connectButton->setEnabled((srcItem != 0)
         && (dstItem != 0)
         && checkRoute(srcItem->text(), dstItem->text()));
#endif
      }

//---------------------------------------------------------
//   dstSelectionChanged
//---------------------------------------------------------

void RouteDialog::dstSelectionChanged()
      {
#if 0 //TD
      QListWidgetItem* dstItem = newDstList->selectedItem();
      QListWidgetItem* srcItem = newSrcList->selectedItem();
      connectButton->setEnabled((srcItem != 0)
         && (dstItem != 0)
         && checkRoute(srcItem->text(), dstItem->text()));
#endif
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void RouteDialog::closeEvent(QCloseEvent* e)
      {
      emit closed();
      e->accept();
      }
