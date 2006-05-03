//
// C++ Interface: shortcutcapturedialog
//
// Description:
// Dialog window for capturing keyboard shortcuts
//
// Author: Mathias Lundgren <lunar_shuttle@users.sourceforge.net>, (C) 2003
//
// Copyright: Mathias Lundgren (lunar_shuttle@users.sourceforge.net) (C) 2003
//
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

