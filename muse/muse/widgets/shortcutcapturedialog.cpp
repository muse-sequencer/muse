//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2003 Mathias Lundgren (lunar_shuttle@users.sourceforge.net)
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

#include "shortcutcapturedialog.h"
#include "shortcuts.h"

ShortcutCaptureDialog::ShortcutCaptureDialog(QWidget* parent, int index)
   : QDialog(parent)
      {
      setupUi(this);
      QKeySequence q = QKeySequence(shortcuts[index].key);
      oshrtLabel->setText(q);
      connect(okButton, SIGNAL( clicked() ), this, SLOT( apply() )  );
      connect(cancelButton, SIGNAL(pressed()), this, SLOT(cancel()));
      shortcutindex = index;
      grabKeyboard();
      okButton->setText(tr("Ok"));
      cancelButton->setText(tr("Cancel"));
      }

ShortcutCaptureDialog::~ShortcutCaptureDialog()
      {
      releaseKeyboard();
      }

void ShortcutCaptureDialog::keyPressEvent(QKeyEvent* e)
      {
#if 0 //TODOB
      bool shift, alt, ctrl, conflict = false, realkey = false;
      QString msgString = "";
      int temp_key;
      shift = e->modifiers() & Qt::ShiftModifier;
      ctrl  = e->modifiers() & Qt::ControlModifier;
      alt   = e->modifiers() & Qt::AltModifier;

      //printf("Key total: %d, alt: %d, ctrl: %d shift: %d\n",e->key(), alt, ctrl, shift);
      temp_key = e->key();
      if (shift)
            temp_key += Qt::SHIFT;
      if (ctrl)
            temp_key += Qt::CTRL;
      if (alt)
            temp_key += Qt::ALT;
      //printf("Final key assembled: %d\n",temp_key);

      // Check if this is a "real" key that completes a valid shortcut:
      int k = e->key();
      if (k < 256 || k == Qt::Key_Enter || k == Qt::Key_Return || k >= Qt::Key_F1 && k <= Qt::Key_F12 || k == Qt::Key_Home || k == Qt::Key_PageUp
          || k == Qt::Key_PageDown || k == Qt::Key_End || k == Qt::Key_Insert || k == Qt::Key_Delete) {
            key = temp_key;
            realkey = true;
            QKeySequence q = QKeySequence(key);
            QString keyString = q;
            if (keyString != QString::null)
                  nshrtLabel->setText(q);

            // Check against conflicting shortcuts
            for (int i=0; i < SHRT_NUM_OF_ELEMENTS; i++) {
                  if (i != shortcutindex) { //check all other than current shortcut
                        if (shortcuts[i].key == key && (shortcuts[i].type & (shortcuts[shortcutindex].type | GLOBAL_SHRT | INVIS_SHRT))) {
                              msgString = tr("Shortcut conflicts with ") + QString(shortcuts[i].descr);
                              conflict = true;
                              break;
                              }
                        }
                  }
            }
            messageLabel->setText(msgString);
            okButton->setEnabled(conflict == false && realkey);
            if (!realkey)
                  nshrtLabel->setText(tr("Undefined"));

#endif
      }

void ShortcutCaptureDialog::apply()
      {
      //return the shortcut to configurator widget:
      done(key);
      }

