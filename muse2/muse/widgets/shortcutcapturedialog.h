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
#include "shortcutcapturedialogbase.h"
//Added by qt3to4:
#include <QKeyEvent>

class ShortcutCaptureDialog : public ShortcutCaptureDialogBase
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
      ShortcutCaptureDialog(QWidget* parent, const char* name = 0, int index=0);
      ~ShortcutCaptureDialog();
      };

