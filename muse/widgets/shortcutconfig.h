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

class QCloseEvent;

#include "ui_shortcutconfigbase.h"

#define SHRT_CATEGORY_COL 0
enum
      {
      SHRT_SHRTCUT_COL = 0,
      SHRT_DESCR_COL,
      };

class SCListViewItem : public QTreeWidgetItem {
      private:
      int index;

      public:
      SCListViewItem(QTreeWidget* parent, int i)
            : QTreeWidgetItem(parent), index(i) { }
      int getIndex() { return index; }

};


class ShortcutConfig : public QDialog, public Ui::ShortcutConfigBase {
      Q_OBJECT
      private:
      int current_category;
      void updateSCListView(int category);
      void updateSCListView() { updateSCListView(current_category); }
      void closeEvent(QCloseEvent *e);

      private slots:
      void categorySelChanged(QTreeWidgetItem*, int);
      void shortcutSelChanged(QTreeWidgetItem*, int);
      void assignShortcut();
      void clearShortcut();
      void assignAll();


      public:
      ShortcutConfig(QWidget* parent);
      bool _config_changed;

};

#endif
