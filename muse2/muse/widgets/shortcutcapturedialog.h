//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/shortcutcapturedialog.h $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
// Author: Mathias Lundgren <lunar_shuttle@users.sourceforge.net>, (C) 2003
//
// Copyright: Mathias Lundgren (lunar_shuttle@users.sourceforge.net) (C) 2003
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
// C++ Interface: shortcutcapturedialog
//
// Description:
// Dialog window for capturing keyboard shortcuts
//
//

#include "shortcuts.h"
#include "filedialog.h"
#include "ui_shortcutcapturedialogbase.h"

class QKeyEvent;

namespace MusEGui {

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
      ShortcutCaptureDialog(QWidget* parent, int index=0);
      ~ShortcutCaptureDialog();
      };

} // namespace MusEGui
