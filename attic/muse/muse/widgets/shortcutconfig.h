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

#include "shortcutconfigbase.h"
#include <qlistview.h>

#define SHRT_CATEGORY_COL 0
enum
      {
      SHRT_DESCR_COL = 0,
      SHRT_SHRTCUT_COL
      };

class SCListViewItem : public QListViewItem {
      private:
      int index;

      public:
      SCListViewItem(QListView* parent, int i)
            : QListViewItem(parent), index(i) { }
      int getIndex() { return index; }

};


class ShortcutConfig : public ShortcutConfigBase {
      Q_OBJECT
      private:
      int current_category;
      void updateSCListView(int category);
      void updateSCListView() { updateSCListView(current_category); }
      void closeEvent(QCloseEvent *e);

      private slots:
      void categorySelChanged(QListViewItem*);
      void shortcutSelChanged(QListViewItem*);
      void assignShortcut();
      void clearShortcut();
      void assignAll();


      public:
      ShortcutConfig(QWidget* parent, const char* name = 0);
      bool _config_changed;

};

#endif
