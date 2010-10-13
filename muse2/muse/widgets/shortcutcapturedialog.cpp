//
// C++ Implementation: shortcutcapturedialog
//
// Description:
// Dialog window for capturing keyboard shortcuts
//
// Author: Mathias Lundgren <lunar_shuttle@users.sourceforge.net>, (C) 2003
//
// Copyright: Mathias Lundgren (lunar_shuttle@users.sourceforge.net) (C) 2003
//
//
#include "shortcutcapturedialog.h"
#include "shortcuts.h"
#include <qkeysequence.h>
#include <qlabel.h>
#include <qevent.h>
#include <qpushbutton.h>
//Added by qt3to4:
#include <QKeyEvent>

ShortcutCaptureDialog::ShortcutCaptureDialog(QWidget* parent, const char* name, int index)
   : ShortcutCaptureDialogBase(parent, name, true)
      {
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
      bool shift, alt, ctrl, conflict = false, realkey = false;
      QString msgString = "";
      int temp_key;
      shift = e->state() & Qt::ShiftModifier;
      ctrl  = e->state() & Qt::ControlModifier;
      alt   = e->state() & Qt::AltModifier;
      //printf("Key total: %d, alt: %d, ctrl: %d shift: %d\n",e->key(), alt, ctrl, shift);
      temp_key = e->key();
      temp_key += (shift ? Qt::SHIFT : 0);
      temp_key += (ctrl  ? Qt::CTRL  : 0);
      temp_key += (alt   ? Qt::ALT   : 0);
      //printf("Final key assembled: %d\n",temp_key);

      // Check if this is a "real" key that completes a valid shortcut:
      int k = e->key();
      if (k < 256 || k == Qt::Key_Enter || k == Qt::Key_Return || (k >= Qt::Key_F1 && k <= Qt::Key_F12) || k == Qt::Key_Home || k == Qt::Key_PageUp
          || k == Qt::Key_PageDown || k == Qt::Key_End || k == Qt::Key_Insert || k == Qt::Key_Delete) {
            key = temp_key;
            realkey = true;
            QKeySequence q = QKeySequence(key);
            QString keyString = q;
            if (keyString != QString::null)
                  nshrtLabel->setText(q);

            // Check against conflicting shortcuts
            for (int i=0; i < SHRT_NUM_OF_ELEMENTS; i++) {
                  if (shortcuts[i].key == key && (shortcuts[i].type & (shortcuts[shortcutindex].type | GLOBAL_SHRT | INVIS_SHRT))) {
                        msgString = tr("Shortcut conflicts with ") + QString(shortcuts[i].descr);
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

