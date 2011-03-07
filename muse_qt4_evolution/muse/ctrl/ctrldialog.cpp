//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2006 by Werner Schweer and others
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

#include "ctrldialog.h"
#include "midictrl.h"
#include "track.h"
#include "miditrack.h"
#include "audiotrack.h"
#include "plugin.h"
#include "pipeline.h"
#include "ctrl/configmidictrl.h"

//---------------------------------------------------------
//   CtrlDialog
//---------------------------------------------------------

CtrlDialog::CtrlDialog(Track* track, int ci, QWidget* parent)
  : QDialog(parent)
      {
      t = track;
      currentId = ci;
      setupUi(this);
      QTreeWidgetItem* header = tw->headerItem();
      header->setTextAlignment(0, Qt::AlignLeft);
      header->setTextAlignment(1, Qt::AlignHCenter);

      tw->header()->setResizeMode(0, QHeaderView::Stretch);
      header->setToolTip(0, tr("controller name"));
      header->setToolTip(1, tr("flag if controller contains data"));

      updateController();
      otherButton->setEnabled(track->type() == Track::MIDI);
      connect(tw,
         SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
         SLOT(itemDoubleClicked(QTreeWidgetItem*, int)));
      connect(otherButton, SIGNAL(clicked()), SLOT(otherClicked()));
      }

//---------------------------------------------------------
//   updateController
//---------------------------------------------------------

void CtrlDialog::updateController()
      {
      tw->clear();
      QTreeWidgetItem* ci;
      if (t->type() == Track::MIDI) {
            //
            //    add special controll for midi tracks
            //
            ci = new QTreeWidgetItem(tw, CTRL_VELOCITY);
            ci->setText(0, "Velocity");
            if (CTRL_VELOCITY == currentId) {
                  tw->setCurrentItem(ci);
                  tw->setItemSelected(ci, true);
                  }
            if (((MidiTrack*)(t))->drumMap()) {
                  ci = new QTreeWidgetItem(tw, CTRL_SVELOCITY);
                  ci->setText(0, "Single Velocity");

            	if (CTRL_SVELOCITY == currentId) {
                        tw->setCurrentItem(ci);
                        tw->setItemSelected(ci, true);
                        }
                  }
            }
      else if (!t->isMidiTrack()) {

            //
            // present plugin parameter
            //
            Pipeline* pl = ((AudioTrack*)t)->prePipe();
            int idx = 0;
            foreach (PluginI* plugin, *pl) {
                  ci = new QTreeWidgetItem(tw, CTRL_NO_CTRL);
                  ci->setText(0, plugin->name());
                  int ncontroller = plugin->plugin()->parameter();
                  for (int i = 0; i < ncontroller; ++i) {
                        QString name(plugin->getParameterName(i));
                        int id = genACnum(idx, i, true);
                        QTreeWidgetItem* cci = new QTreeWidgetItem(ci, id);
                        cci->setText(0, name);
                        Ctrl* ctrl = t->getController(id);
                        if (ctrl) {
                              if (!ctrl->empty())
                                    cci->setText(1, "*");
                              if (id == currentId) {
                                    tw->setCurrentItem(cci);
                                    tw->setItemSelected(cci, true);
                                    }
                              }
                        else
                              printf("updateController: controller %x not found\n", id);
                        }
                  }
            pl = ((AudioTrack*)t)->postPipe();
            idx = 0;
            foreach (PluginI* plugin, *pl) {
                  ci = new QTreeWidgetItem(tw, CTRL_NO_CTRL);
                  ci->setText(0, plugin->name());
                  int ncontroller = plugin->plugin()->parameter();
                  for (int i = 0; i < ncontroller; ++i) {
                        QString name(plugin->getParameterName(i));
                        int id = genACnum(idx, i, false);
                        QTreeWidgetItem* cci = new QTreeWidgetItem(ci, id);
                        cci->setText(0, name);
                        Ctrl* ctrl = t->getController(id);
                        if (!ctrl->empty())
                              cci->setText(1, "*");
                        if (id == currentId) {
                              tw->setCurrentItem(cci);
                              tw->setItemSelected(cci, true);
                              }
                        }
                  }
            }

      ControllerNameList* cn = t->controllerNames();
      for (iControllerName i = cn->begin(); i != cn->end(); ++i) {
            ci = new QTreeWidgetItem(tw, i->id);
            ci->setText(0, i->name);
            Ctrl* ctrl = t->getController(i->id);
            if (!ctrl->empty())
                  ci->setText(1, "*");

            if (i->id == currentId) {
                  tw->setCurrentItem(ci);
                  tw->setItemSelected(ci, true);
                  }
            }
      }

//---------------------------------------------------------
//   itemDoubleClicked
//---------------------------------------------------------

void CtrlDialog::itemDoubleClicked(QTreeWidgetItem* item, int)
      {
      if (item->type() != CTRL_NO_CTRL)
            accept();
      }

//---------------------------------------------------------
//   CtrlDialog
//---------------------------------------------------------

int CtrlDialog::curId() const
      {
      QTreeWidgetItem* item = tw->currentItem();
      if (item == 0)
            return CTRL_NO_CTRL;
      return item->type();
      }

//---------------------------------------------------------
//   otherClicked
//    Add another controller to the list of "managed"
//    controllers.
//---------------------------------------------------------

void CtrlDialog::otherClicked()
      {
      QTreeWidgetItem* item = tw->currentItem();
      if (item)
            currentId = item->type();
      //
      // present the list of available controller for
      // the selected midi instrument
      //
      ConfigMidiCtrl mce((MidiTrack*)t);
      mce.exec();
      updateController();
      }

