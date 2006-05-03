//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: configmidictrl.cpp,v 1.3 2005/10/31 14:55:04 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "configmidictrl.h"
#include "track.h"
#include "miditrack.h"
#include "midictrl.h"
#include "instruments/minstrument.h"

//---------------------------------------------------------
//   ConfigMidiCtrl
//---------------------------------------------------------

ConfigMidiCtrl::ConfigMidiCtrl(MidiTrack* t)
      {
      setupUi(this);
      track = t;

      //---------------------------------------------------
      // populate list of managed controllers
      //---------------------------------------------------

      ControllerNameList* cn = track->controllerNames();
      for (iControllerName i = cn->begin(); i != cn->end(); ++i)
            managedController->addItem(i->name);

      //---------------------------------------------------
      // populate list of available controllers
      //---------------------------------------------------

      MidiChannel* mc = 0;
      if (track->type() == Track::MIDI)
            mc = track->channel();
      else if (track->type() == Track::MIDI_CHANNEL)
            mc = (MidiChannel*)track;
      if (mc) {
            MidiOutPort* mp = mc->port();
            portName->setText(mp->name());
            //
            // populate popup with all controllers available for
            // current instrument
            //
            MidiControllerList* mcl = mp->instrument()->controller();
            for (iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci) {
                  iControllerName i;
                  for (i = cn->begin(); i != cn->end(); ++i) {
                        if (i->name == (*ci)->name())
                              break;
                        }
                  if (i == cn->end())
                        availableController->addItem((*ci)->name());
                  }
            }
      delete cn;
      buttonAdd->setEnabled(false);
      buttonRemove->setEnabled(false);
      connect(buttonAdd, SIGNAL(clicked()), SLOT(addClicked()));
      connect(buttonRemove, SIGNAL(clicked()), SLOT(removeClicked()));
      connect(availableController, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), SLOT(availableSelected(QListWidgetItem*)));
      connect(managedController,   SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), SLOT(managedSelected(QListWidgetItem*)));
      }

//---------------------------------------------------------
//   addClicked
//---------------------------------------------------------

void ConfigMidiCtrl::addClicked()
      {
      QListWidgetItem* n = availableController->currentItem();
      if (n == 0)
            return;
      QString s(n->text());
      for (int i = 0; i < managedController->count(); ++i) {
            if (s == managedController->item(i)->text())
                  return;
            }
      managedController->addItem(s);
      managedController->setCurrentItem(managedController->item(managedController->count()-1));
      delete n;
      buttonAdd->setEnabled(false);
      }

//---------------------------------------------------------
//   removeClicked
//---------------------------------------------------------

void ConfigMidiCtrl::removeClicked()
      {
      QListWidgetItem* n = managedController->currentItem();
      if (n == 0)
            return;
      availableController->addItem(n->text());
      delete n;
      buttonRemove->setEnabled(false);
      }

//---------------------------------------------------------
//   availableSelected
//---------------------------------------------------------

void ConfigMidiCtrl::availableSelected(QListWidgetItem* item)
      {
      buttonAdd->setEnabled(item != 0);
      }

//---------------------------------------------------------
//   managedSelected
//---------------------------------------------------------

void ConfigMidiCtrl::managedSelected(QListWidgetItem* item)
      {
      buttonRemove->setEnabled(item != 0);
      }

//---------------------------------------------------------
//   done
//---------------------------------------------------------

void ConfigMidiCtrl::done(int code)
      {
      if (!code) {
            QDialog::done(code);
            return;
            }

      MidiChannel* mc = 0;
      if (track->type() == Track::MIDI)
            mc = track->channel();
      else if (track->type() == Track::MIDI_CHANNEL)
            mc = (MidiChannel*)track;

      if (mc) {
            MidiOutPort* port = mc->port();
            ControllerNameList* cn  = track->controllerNames();
            MidiInstrument* instr   = port->instrument();
            MidiControllerList* mcl = instr->controller();

            //
            // search for new, added controller
            //
            int n = managedController->count();
            for (int i = 0; i < n; ++i) {
                  QString name(managedController->item(i)->text());
                  iControllerName ii = cn->begin();
                  for (; ii != cn->end(); ++ii) {
                        if (ii->name == name)
                              break;
                        }
                  if (ii == cn->end()) {
                        // add controller "name" to list of managed controller
                        //
                        for (iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci) {
                              if ((*ci)->name() == name) {
                                    Ctrl* ctrl = new Ctrl(*ci);
                                    track->addController(ctrl);
                                    break;
                                    }
                              }
                        }
                  }

            //
            // search for removed controller
            //
            for (iControllerName ii = cn->begin(); ii != cn->end(); ++ii) {
                  int i;
                  for (i = 0; i < n; ++i) {
                        if (managedController->item(i)->text() == ii->name)
                              break;
                        }
                  if (i == n)
                        track->removeController(ii->id);
                  }
            delete cn;
            }
      QDialog::done(code);
      }

