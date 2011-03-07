//
// C++ Interface: shortcutconfig
//
// Description:
// Dialog for configuring keyboard shortcuts
//
// Author: Mathias Lundgren <lunar_shuttle@users.sourceforge.net>, (C) 2003
//
// Copyright: Mathias Lundgren (lunar_shuttle@users.sourceforge.net) (C) 2003
//
//
#ifndef __SHORTCUTCONFIG_H
#define __SHORTCUTCONFIG_H

#include "ui_shortcutconfig.h"
#include "shortcuts.h"

#define SHRT_CATEGORY_COL 0
enum {
      SHRT_DESCR_COL = 0,
      SHRT_SHRTCUT_COL
      };

//---------------------------------------------------------
//   ShortcutConfig
//---------------------------------------------------------

class ShortcutConfig : public QDialog, public Ui::ShortcutConfigBase {
      Q_OBJECT
      private:
      int current_category;
      void updateSCListView(int category);
      void updateSCListView() { updateSCListView(current_category); }
      void closeEvent(QCloseEvent *e);

   private slots:
      void categorySelChanged(QTreeWidgetItem*);
      void shortcutSelChanged(QTreeWidgetItem*);
      void assignShortcut();
      void clearShortcut();
      void assignAll();

   public:
      ShortcutConfig(QWidget* parent = 0);
      bool _config_changed;
      static const shortcut_cg shortcut_category[];
      };

#endif

