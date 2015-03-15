#include <QMenu>
#include <QSignalMapper>
#include <QInputDialog>

#include "popupmenu.h"
#include "menutitleitem.h"

#include "plugindialog.h"
#include "ui_plugindialogbase.h"
#include "plugin.h"


namespace MusEGui {

int PluginDialog::selectedPlugType = SEL_TYPE_ALL;
int PluginDialog::selectedPlugPortType = SEL_PORT_ALL;
int PluginDialog::selectedGroup = 0;
QStringList PluginDialog::sortItems = QStringList();
QRect PluginDialog::geometrySave = QRect();
QByteArray PluginDialog::listSave = QByteArray();

//---------------------------------------------------------
//   PluginDialog
//    select Plugin dialog
//---------------------------------------------------------

PluginDialog::PluginDialog(QWidget* parent)
  : QDialog(parent)
{
    ui.setupUi(this);

      group_info=NULL;
      setWindowTitle(tr("MusE: select plugin"));

      if(!geometrySave.isNull())
        setGeometry(geometrySave);

      //QVBoxLayout* layout = new QVBoxLayout(this);

      //tabBar = new QTabBar(this);
      ui.tabBar->setToolTip(tr("Plugin categories.\nRight-click on tabs to manage.\nRight-click on plugins to add/remove from a category."));
      ui.tabBar->addTab("All");
      for (QList<QString>::iterator it=MusEGlobal::plugin_group_names.begin(); it!=MusEGlobal::plugin_group_names.end(); it++)
        ui.tabBar->addTab(*it);


      //pList  = new QTreeWidget(this);

      ui.pList->setColumnCount(12);
      // "Note: In order to avoid performance issues, it is recommended that sorting
      //   is enabled after inserting the items into the tree. Alternatively, you could
      //   also insert the items into a list before inserting the items into the tree. "
      QStringList headerLabels;
      headerLabels << tr("Type");
      headerLabels << tr("Lib");
      headerLabels << tr("Label");
      headerLabels << tr("Name");
      headerLabels << tr("AI");
      headerLabels << tr("AO");
      headerLabels << tr("CI");
      headerLabels << tr("CO");
      headerLabels << tr("IP");
      headerLabels << tr("id");
      headerLabels << tr("Maker");
      headerLabels << tr("Copyright");

      ui.pList->setHeaderLabels(headerLabels);

      ui.pList->headerItem()->setToolTip(4,  tr("Audio inputs"));
      ui.pList->headerItem()->setToolTip(5,  tr("Audio outputs"));
      ui.pList->headerItem()->setToolTip(6,  tr("Control inputs"));
      ui.pList->headerItem()->setToolTip(7,  tr("Control outputs"));
      ui.pList->headerItem()->setToolTip(8,  tr("In-place capable"));
      ui.pList->headerItem()->setToolTip(9,  tr("ID number"));

      ui.pList->setRootIsDecorated(false);
      ui.pList->setSelectionBehavior(QAbstractItemView::SelectRows);
      ui.pList->setSelectionMode(QAbstractItemView::SingleSelection);
      ui.pList->setAlternatingRowColors(true);
      ui.pList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      ui.pList->setContextMenuPolicy(Qt::CustomContextMenu);


      ui.okB->setDefault(true);
      ui.okB->setFixedWidth(80);
      ui.okB->setEnabled(false);
      ui.cancelB->setFixedWidth(80);

      switch(selectedPlugPortType) {
            case SEL_PORT_SM:  ui.onlySM->setChecked(true);  break;
            case SEL_PORT_S:   ui.onlyS->setChecked(true);   break;
            case SEL_PORT_M:   ui.onlyM->setChecked(true);   break;
            case SEL_PORT_ALL: ui.allPlug->setChecked(true); break;
            }

      ui.tabBar->setCurrentIndex(selectedGroup);
      ui.tabBar->setContextMenuPolicy(Qt::ActionsContextMenu);
      newGroupAction= new QAction(tr("&create new group"),ui.tabBar);
      delGroupAction= new QAction(tr("&delete currently selected group"),ui.tabBar);
      renGroupAction= new QAction(tr("re&name currently selected group"),ui.tabBar);
      ui.tabBar->addAction(newGroupAction);
      ui.tabBar->addAction(delGroupAction);
      ui.tabBar->addAction(renGroupAction);

      if (selectedGroup==0)
      {
                delGroupAction->setEnabled(false);
                renGroupAction->setEnabled(false);
            }
      //ui.tabBar->setMovable(true); //not yet. need to find a way to forbid moving the zeroth tab


      ui.pluginfilterGroup->setToolTip(tr("Select which types of plugins should be visible in the list.<br>"
                             "Note that using mono plugins on stereo tracks is not a problem, two will be used in parallel.<br>"
                             "Also beware that the 'all' alternative includes plugins that may not be useful in an effect rack."));

      ui.pluginType->addItem("All", SEL_TYPE_ALL);
      ui.pluginType->addItem("DSSI", SEL_TYPE_DSSI);
      ui.pluginType->addItem("LADSPA", SEL_TYPE_LADSPA);
      ui.pluginType->addItem("LV2", SEL_TYPE_LV2);
      ui.pluginType->addItem("VST", SEL_TYPE_VST);
      ui.pluginType->addItem("Wine VST", SEL_TYPE_WINE_VST);
      connect (ui.pluginType,SIGNAL(currentIndexChanged(int)), SLOT(filterType(int)));

      for (int i=0; i < ui.pluginType->count(); i++) {
        if (selectedPlugType == ui.pluginType->itemData(i).toInt()) {
          ui.pluginType->setCurrentIndex(i);
          //printf("set current index to %d\n",i);
          break;
        }
      }
      ui.sortBox->addItems(sortItems);

      fillPlugs();

      ui.pList->setSortingEnabled(true);

      if(listSave.isEmpty())
      {
        int sizes[] = { 80, 110, 110, 110, 30, 30, 30, 30, 30, 50, 110, 110 };
        for (int i = 0; i < 12; ++i) {
              if (sizes[i] <= 50)     // hack alert!
                    ui.pList->header()->setSectionResizeMode(i, QHeaderView::Fixed);
              ui.pList->header()->resizeSection(i, sizes[i]);
        }
        ui.pList->sortByColumn(3, Qt::AscendingOrder);
      }
      else
        ui.pList->header()->restoreState(listSave);

      connect(ui.pList,   SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(accept()));
      connect(ui.pList,   SIGNAL(itemClicked(QTreeWidgetItem*,int)), SLOT(enableOkB()));
      connect(ui.pList,   SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(plistContextMenu(const QPoint&)));
      connect(ui.cancelB, SIGNAL(clicked()), SLOT(reject()));
      connect(ui.okB,     SIGNAL(clicked()), SLOT(accept()));
      connect(ui.portFilterGroup, SIGNAL(buttonClicked(QAbstractButton*)), SLOT(pluginTypeSelectionChanged(QAbstractButton*)));
      connect(ui.tabBar,  SIGNAL(currentChanged(int)), SLOT(tabChanged(int)));
      //connect(tabBar,  SIGNAL(tabMoved(int,int)), SLOT(tabMoved(int,int))); //not yet. need to find a way to forbid moving the zeroth tab
      connect(ui.sortBox, SIGNAL(editTextChanged(const QString&)),SLOT(fillPlugs()));
      connect(newGroupAction, SIGNAL(triggered()), SLOT(newGroup()));
      connect(delGroupAction, SIGNAL(triggered()), SLOT(delGroup()));
      connect(renGroupAction, SIGNAL(triggered()), SLOT(renameGroup()));
      ui.sortBox->setFocus();
}

void PluginDialog::filterType(int i)
{
    selectedPlugType = ui.pluginType->itemData(i).toInt();

    fillPlugs();

}

void PluginDialog::plistContextMenu(const QPoint& point)
{
  QTreeWidgetItem* item = ui.pList->currentItem();
  if (item)
  {
    group_info = &MusEGlobal::plugin_groups.get(item->text(1), item->text(2));
    QMenu* menu = new MusEGui::PopupMenu(this, true);
    QSignalMapper* mapper = new QSignalMapper(this);
    menu->addAction(new MusEGui::MenuTitleItem(tr("Associated categories"), menu));

    if (ui.tabBar->count()==1)
    {
      QAction* tmp=menu->addAction(tr("You need to define some categories first."));
      tmp->setEnabled(false);
    }
    else
    {
      for (int i=1; i<ui.tabBar->count(); i++) // ignore the first tab ("All")
      {
        QAction* act=menu->addAction(ui.tabBar->tabText(i));
        act->setCheckable(true);
        act->setChecked(group_info->contains(i));
        connect(act,SIGNAL(toggled(bool)), mapper, SLOT(map()));
        mapper->setMapping(act, i);
      }
      connect(mapper, SIGNAL(mapped(int)), this, SLOT(groupMenuEntryToggled(int)));
    }

    menu->exec(mapToGlobal(point));

    delete mapper;
    delete menu;

    if (selectedGroup!=0 && !group_info->contains(selectedGroup)) // we removed the entry from the currently visible group
      fillPlugs();

    group_info=NULL;
  }
}

void PluginDialog::groupMenuEntryToggled(int index)
{
  if (group_info)
  {
    if (group_info->contains(index))
      group_info->remove(index);
    else
      group_info->insert(index);
  }
  else
  {
    fprintf(stderr,"THIS SHOULD NEVER HAPPEN: groupMenuEntryToggled called but group_info is NULL!\n");
  }
}



//---------------------------------------------------------
//   enableOkB
//---------------------------------------------------------

void PluginDialog::enableOkB()
{
  ui.okB->setEnabled(true);
}


void PluginDialog::newGroup()
{
  MusEGlobal::plugin_groups.shift_right(selectedGroup+1, ui.tabBar->count());
  ui.tabBar->insertTab(selectedGroup+1, tr("new group"));
  MusEGlobal::plugin_group_names.insert(selectedGroup, tr("new group"));
}

void PluginDialog::delGroup()
{
  if (selectedGroup!=0)
  {
    MusEGlobal::plugin_groups.erase(selectedGroup);
    MusEGlobal::plugin_groups.shift_left(selectedGroup+1, ui.tabBar->count());
    ui.tabBar->removeTab(selectedGroup);
    MusEGlobal::plugin_group_names.removeAt(selectedGroup-1);
  }
}

void PluginDialog::renameGroup()
{
  if (selectedGroup!=0)
  {
    bool ok;
    QString newname = QInputDialog::getText(this, tr("Enter the new group name"),
                                        tr("Enter the new group name"), QLineEdit::Normal,
                                        ui.tabBar->tabText(selectedGroup), &ok);
    if (ok)
    {
      ui.tabBar->setTabText(selectedGroup, newname);
      MusEGlobal::plugin_group_names.replace(selectedGroup-1, newname);
    }
  }
}

//---------------------------------------------------------
//   value
//---------------------------------------------------------

MusECore::Plugin* PluginDialog::value()
{
      QTreeWidgetItem* item = ui.pList->currentItem();
      if (item)
        return MusEGlobal::plugins.find(item->text(1), item->text(2));
      printf("plugin not found\n");
      return 0;
}

//---------------------------------------------------------
//   saveSettings
//---------------------------------------------------------

void PluginDialog::saveSettings()
{
  if (!ui.sortBox->currentText().isEmpty()) {
        bool found = false;
        foreach (QString item, sortItems)
            if(item == ui.sortBox->currentText()) {
                found = true;
                break;
                }
        if(!found)
          sortItems.push_front(ui.sortBox->currentText());
        }

  QHeaderView* hdr = ui.pList->header();
  if(hdr)
    listSave = hdr->saveState();

  geometrySave = geometry();
}

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void PluginDialog::accept()
{
      saveSettings();
      QDialog::accept();
}

//---------------------------------------------------------
//   reject
//---------------------------------------------------------

void PluginDialog::reject()
{
      saveSettings();
      QDialog::reject();
}

//---------------------------------------------------------
//    pluginTypeSelectionChanged
//---------------------------------------------------------

void PluginDialog::pluginTypeSelectionChanged(QAbstractButton* ab)
{
      if (ab == ui.allPlug)
            selectedPlugPortType = SEL_PORT_ALL;
      else if (ab == ui.onlyM)
            selectedPlugPortType = SEL_PORT_M;
      else if (ab == ui.onlyS)
            selectedPlugPortType = SEL_PORT_S;
      else if (ab == ui.onlySM)
            selectedPlugPortType = SEL_PORT_SM;
      fillPlugs();
}

void PluginDialog::tabChanged(int index)
{
  renGroupAction->setEnabled(index!=0);
  delGroupAction->setEnabled(index!=0);

  selectedGroup=index;
  fillPlugs();
}

void PluginDialog::tabMoved(int from, int to)
{
//all the below doesn't work :/
/*  static bool recurse=false;

  if (!recurse)
  {
    if (from==0 && to!=0) {recurse=true; tabBar->moveTab(to, from);}
    if (from!=0 && to==0) {recurse=true; tabBar->moveTab(from, to);}
  }
  recurse=false;*/


   //if ((from==0 && to!=0) || (from!=0 && to==0)) { tabBar->setMovable(false); tabBar->setMovable(true); }
   printf("**** %i -> %i\n", from, to);

  //FINDMICH TODO
}

void PluginDialog::fillPlugs()
{
   QString type_name;
   ui.pList->clear();
   ui.okB->setEnabled(false);
   for (MusECore::iPlugin i = MusEGlobal::plugins.begin(); i != MusEGlobal::plugins.end(); ++i)
      if (selectedGroup==0 || MusEGlobal::plugin_groups.get(*i).contains(selectedGroup))
      {
         unsigned long ai = (*i)->inports();
         unsigned long ao = (*i)->outports();
         unsigned long ci = (*i)->controlInPorts();
         unsigned long co = (*i)->controlOutPorts();
         bool found = false;
         QString sb_txt = ui.sortBox->currentText().toLower();
         if(sb_txt.isEmpty() || (*i)->label().toLower().contains(sb_txt) || (*i)->name().toLower().contains(sb_txt))
            found = true;

         bool addFlag = false;
         switch (selectedPlugPortType) {
         case SEL_PORT_SM: // stereo & mono
            if ((ai == 1 || ai == 2) && (ao == 1 || ao ==2)) {
               addFlag = true;
            }
            break;
         case SEL_PORT_S: // stereo
            if ((ai == 1 || ai == 2) &&  ao ==2) {
               addFlag = true;
            }
            break;
         case SEL_PORT_M: // mono
            if (ai == 1  && ao == 1) {
               addFlag = true;
            }
            break;
         case SEL_PORT_ALL: // all
            addFlag = true;
            break;
         }
         if (found && addFlag) {
            int plugInstanceType;
            if((*i)->isDssiSynth() || (*i)->isDssiPlugin()) {
               if ((*i)->lib() == "dssi-vst") {
                  type_name = tr("Wine VST");
                  plugInstanceType = SEL_TYPE_WINE_VST;
               } else {
                  if ((*i)->isDssiSynth())
                     type_name = tr("dssi synth");
                  else
                     type_name = tr("dssi effect");
                  plugInstanceType = SEL_TYPE_DSSI;
               }
            }
            else if((*i)->isLV2Synth()) {
               type_name = tr("LV2 synth");
               plugInstanceType = SEL_TYPE_LV2;
            }
            else if((*i)->isLV2Plugin()) {
               type_name = tr("LV2 effect");
               plugInstanceType = SEL_TYPE_LV2;
            }
            else {
               type_name = tr("ladspa");
               plugInstanceType = SEL_TYPE_LADSPA;
            }

            // last check
            if ( selectedPlugType != SEL_TYPE_ALL && plugInstanceType != selectedPlugType ) {
               continue;
            }

            QTreeWidgetItem* item = new QTreeWidgetItem;
            item->setText(0,  type_name);
            item->setText(1,  (*i)->lib());
            item->setText(2,  (*i)->label());
            item->setText(3,  (*i)->name());
            item->setText(4,  QString().setNum(ai));
            item->setText(5,  QString().setNum(ao));
            item->setText(6,  QString().setNum(ci));
            item->setText(7,  QString().setNum(co));
            item->setText(8,  QString().setNum((*i)->inPlaceCapable()));
            item->setText(9,  QString().setNum((*i)->id()));
            item->setText(10,  (*i)->maker());
            item->setText(11, (*i)->copyright());
            ui.pList->addTopLevelItem(item);
         }
      }
}

//---------------------------------------------------------
//   getPlugin
//---------------------------------------------------------

MusECore::Plugin* PluginDialog::getPlugin(QWidget* parent)
{
      PluginDialog* dialog = new PluginDialog(parent);
      MusECore::Plugin* p = 0;
      int rv = dialog->exec();
      if(rv)
        p = dialog->value();
      delete dialog;
      return p;
}

}

