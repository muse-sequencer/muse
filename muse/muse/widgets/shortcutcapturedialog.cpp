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
      shift = e->state() & ShiftButton;
      ctrl  = e->state() & ControlButton;
      alt   = e->state() & AltButton;
      //printf("Key total: %d, alt: %d, ctrl: %d shift: %d\n",e->key(), alt, ctrl, shift);
      temp_key = e->key();
      temp_key += (shift ? Qt::SHIFT : 0);
      temp_key += (ctrl  ? Qt::CTRL  : 0);
      temp_key += (alt   ? Qt::ALT   : 0);
      //printf("Final key assembled: %d\n",temp_key);

      // Check if this is a "real" key that completes a valid shortcut:
      int k = e->key();
      if (k < 256 || k == Key_Enter || k == Key_Return || (k >= Key_F1 && k <= Key_F12) || k == Key_Home || k == Key_PageUp
          || k == Key_PageDown || k == Key_End || k == Key_Insert || k == Key_Delete) {
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

