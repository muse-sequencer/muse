#ifndef PLUGINDIALOG_H
#define PLUGINDIALOG_H

#include <QDialog>
#include "globals.h"
#include "ui_plugindialogbase.h"

class QAbstractButton;

namespace Ui {
class PluginDialogBase;
}

namespace MusECore {
class Plugin;
}

namespace MusEGui {



class PluginDialog : public QDialog {
      Q_OBJECT
      enum SelectedPluginPortTypes { SEL_PORT_SM, SEL_PORT_S, SEL_PORT_M, SEL_PORT_ALL };
      enum SelectedPluginTypes { SEL_TYPE_DSSI, SEL_TYPE_LADSPA, SEL_TYPE_LV2, SEL_TYPE_VST, SEL_TYPE_WINE_VST, SEL_TYPE_ALL};

   public:
      explicit PluginDialog(QWidget* parent=0);
      static MusECore::Plugin* getPlugin(QWidget* parent);
      MusECore::Plugin* value();

   public slots:
      void accept();
      void reject();

   private slots:
      void enableOkB();
      void pluginTypeSelectionChanged(QAbstractButton*);
      void tabChanged(int);
      void tabMoved(int,int);
      void fillPlugs();
      void filterType(int);

      void newGroup();
      void delGroup();
      void renameGroup();
      void plistContextMenu(const QPoint&);
      void groupMenuEntryToggled(int i);

   private:
//      QComboBox* sortBox;
//      QTabBar* tabBar;
//      QTreeWidget* pList;
//      QRadioButton* allPlug;
//      QRadioButton* onlyM;
//      QRadioButton* onlyS;
//      QRadioButton* onlySM;
//      QPushButton *okB;

      QAction* newGroupAction;
      QAction* delGroupAction;
      QAction* renGroupAction;


      void saveSettings();

      static int selectedPlugPortType;
      static int selectedPlugType;
      static int selectedGroup; // 0 means "show all"
      static QStringList sortItems;
      static QRect geometrySave;
      static QByteArray listSave;

      QSet<int>* group_info; //holds the group-set of the plugin which shall be affected by the plistContextMenu.

      Ui::PluginDialogBase ui;
};

}













#endif // PLUGINDIALOGBASE_H
