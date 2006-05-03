//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2003 Mathias Lundgren <lunar_shuttle@users.sourceforge.net>
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

//
// C++ Interface: shortcutcapturedialog
//
// Description:
// Dialog window for capturing keyboard shortcuts
//

#include "shortcuts.h"
#include "filedialog.h"
#include "ui_shortcutcapturedialog.h"

class ShortcutCaptureDialog : public QDialog, public Ui::ShortcutCaptureDialogBase
      {
      Q_OBJECT
      private:
      int  shortcutindex;
      void keyPressEvent(QKeyEvent* e);
      int  key;

      private slots:
      void apply();
      void cancel() { reject(); };

      public:
      ShortcutCaptureDialog(QWidget* parent = 0, int index = 0);
      ~ShortcutCaptureDialog();
      };

