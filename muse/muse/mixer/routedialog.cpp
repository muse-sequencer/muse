//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: routedialog.cpp,v 1.5.2.2 2007/01/04 00:35:17 terminator356 Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <qlistbox.h>
#include <qlistview.h>
#include <qtoolbutton.h>
#include "routedialog.h"
#include "track.h"
#include "song.h"
#include "audio.h"
#include "driver/jackaudio.h"

//---------------------------------------------------------
//   RouteDialog
//---------------------------------------------------------

RouteDialog::RouteDialog(QWidget* parent)
   : RouteDialogBase(parent)
      {
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

      TrackList* tl = song->tracks();
      for (ciTrack i = tl->begin(); i != tl->end(); ++i) {
            if ((*i)->isMidiTrack())
                  continue;
            WaveTrack* track = (WaveTrack*)(*i);
            if (track->type() == Track::AUDIO_INPUT) {
                  for (int channel = 0; channel < track->channels(); ++channel)
                        newDstList->insertItem(Route(track, channel).name());
                  const RouteList* rl = track->inRoutes();
                  for (ciRoute r = rl->begin(); r != rl->end(); ++r) {
                        Route dst(track->name(), true, r->channel);
                        new QListViewItem(routeList, r->name(), dst.name());
                        }
                  }
            else if (track->type() != Track::AUDIO_AUX)
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
                        Route s(src, false, r->channel);
                        src = s.name();
                        }
                  new QListViewItem(routeList, src, r->name());
                  }
            }
      if (!checkAudioDevice()) return;
      std::list<QString> sl = audioDevice->outputPorts();
      for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i)
            newSrcList->insertItem(*i);
      sl = audioDevice->inputPorts();
      for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i)
            newDstList->insertItem(*i);
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
      QListViewItem* item = routeList->selectedItem();
      removeButton->setEnabled(item != 0);
      }

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

void RouteDialog::removeRoute()
      {
      QListViewItem* item = routeList->selectedItem();
      if (item == 0)
            return;
      audio->msgRemoveRoute(Route(item->text(0), false, -1), Route(item->text(1), true, -1));
      audio->msgUpdateSoloStates();
      song->update(SC_SOLO);
      delete item;
      }

//---------------------------------------------------------
//   addRoute
//---------------------------------------------------------

void RouteDialog::addRoute()
      {
      QListBoxItem* srcItem = newSrcList->selectedItem();
      QListBoxItem* dstItem = newDstList->selectedItem();
      if (srcItem == 0 || dstItem == 0)
            return;
      audio->msgAddRoute(Route(srcItem->text(), false, -1), Route(dstItem->text(), true, -1));
      audio->msgUpdateSoloStates();
      song->update(SC_SOLO);
      new QListViewItem(routeList, srcItem->text(), dstItem->text());
      }

//---------------------------------------------------------
//   srcSelectionChanged
//---------------------------------------------------------

void RouteDialog::srcSelectionChanged()
      {
      QListBoxItem* srcItem = newSrcList->selectedItem();
      QListBoxItem* dstItem = newDstList->selectedItem();
      connectButton->setEnabled((srcItem != 0)
         && (dstItem != 0)
         && checkRoute(srcItem->text(), dstItem->text()));
      }

//---------------------------------------------------------
//   dstSelectionChanged
//---------------------------------------------------------

void RouteDialog::dstSelectionChanged()
      {
      QListBoxItem* dstItem = newDstList->selectedItem();
      QListBoxItem* srcItem = newSrcList->selectedItem();
      connectButton->setEnabled((srcItem != 0)
         && (dstItem != 0)
         && checkRoute(srcItem->text(), dstItem->text()));
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void RouteDialog::closeEvent(QCloseEvent* e)
      {
      emit closed();
      e->accept();
      }
