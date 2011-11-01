//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/shortcutcapturedialog.cpp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
// Author: Mathias Lundgren <lunar_shuttle@users.sourceforge.net>, (C) 2003
//
// Copyright: Mathias Lundgren (lunar_shuttle@users.sourceforge.net) (C) 2003
//
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
//
// C++ Implementation: shortcutcapturedialog
//
// Description:
// Dialog window for capturing keyboard shortcuts

#include "shortcutcapturedialog.h"
#include "shortcuts.h"

#include <QKeyEvent>
#include <QKeySequence>
#include <QInputEvent>
#include <QChar>
#include <QApplication>

namespace MusEGui {

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
      bool shift, alt, ctrl, meta, conflict = false, realkey = false;
      QString msgString = "";
      int temp_key;
      Qt::KeyboardModifiers mods = ((QInputEvent*)e)->modifiers();
      shift = mods & Qt::ShiftModifier;
      ctrl  = mods & Qt::ControlModifier;
      alt   = mods & Qt::AltModifier;
      meta  = mods & Qt::MetaModifier;
      //printf("Key total: %d, alt: %d, ctrl: %d shift: %d\n",e->key(), alt, ctrl, shift);
      temp_key = e->key();
      
      QChar keychar(temp_key);
      //bool ispunct = keychar.isPunct();
      //bool issymbol = keychar.isSymbol();
      //printf("Key:%x, alt:%d, ctrl:%d shift:%d ispunct:%d issymbol:%d text:%s\n",
      //  e->key(), alt, ctrl, shift, ispunct, issymbol, e->text().toLatin1().constData());
      
      temp_key += (shift ? (int)Qt::SHIFT : 0);    // (int) Tim
      temp_key += (ctrl  ? (int)Qt::CTRL  : 0);    //
      temp_key += (alt   ? (int)Qt::ALT   : 0);    //
      temp_key += (meta  ? (int)Qt::META  : 0);
      //printf("Final key assembled: %d\n",temp_key);

      // Check if this is a "real" key that completes a valid shortcut:
      int k = e->key();
      if (k < 256 || k == Qt::Key_Enter || k == Qt::Key_Return || (k >= Qt::Key_F1 && k <= Qt::Key_F12) || k == Qt::Key_Home || k == Qt::Key_PageUp
          || k == Qt::Key_PageDown || k == Qt::Key_End || k == Qt::Key_Insert || k == Qt::Key_Delete 
          || k == Qt::Key_Up || k == Qt::Key_Down || k == Qt::Key_Left || k == Qt::Key_Right) {
            key = temp_key;
            realkey = true;
            QKeySequence q = QKeySequence(key);
            //QKeySequence q = QKeySequence(k, mods);
            QString keyString = q;
            if (keyString != QString::null)
                  nshrtLabel->setText(q);

            // Check against conflicting shortcuts
            for (int i=0; i < SHRT_NUM_OF_ELEMENTS; i++) {
                  if (shortcuts[i].key == key &&  // use the same key
                      (( shortcuts[i].type  & (shortcuts[shortcutindex].type | INVIS_SHRT)) ||
                         shortcuts[i].type & GLOBAL_SHRT ||
                         shortcuts[shortcutindex].type & GLOBAL_SHRT)) { // affect the same scope
                      msgString = tr("Shortcut conflicts with %1").arg(qApp->translate("shortcuts",shortcuts[i].descr));
                      conflict = true;
                      break;
                      }
                  }
            }
            messageLabel->setText(msgString);
            okButton->setEnabled(conflict == false && realkey);
            if (!realkey)
                  nshrtLabel->setText(tr("Undefined"));


      }

void ShortcutCaptureDialog::apply()
      {
      //return the shortcut to configurator widget:
      done(key);
      }

} // namespace MusEGui
