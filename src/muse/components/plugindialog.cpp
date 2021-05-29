#include <QMenu>
#include <QInputDialog>
#include <QStyledItemDelegate>

#include "globaldefs.h"
#include "popupmenu.h"
#include "menutitleitem.h"

#include "plugindialog.h"
//#include "ui_plugindialogbase.h"
#include "plugin.h"
#include "gconfig.h"


namespace MusEGui {

int PluginDialog::selectedPlugType = SEL_TYPE_ALL;
int PluginDialog::selectedPlugPortType = SEL_PORT_ALL;
int PluginDialog::selectedGroup = 0;
QStringList PluginDialog::sortItems = QStringList();
QRect PluginDialog::geometrySave = QRect();
QByteArray PluginDialog::listSave = QByteArray();

//---------------------------------------------------------
//   PluginItem
//---------------------------------------------------------

PluginItem::PluginItem(
      bool hasUri,
      QTreeWidget* parent
    )
  :  QTreeWidgetItem(parent),
      _hasUri(hasUri)
{
  
}


class RightAlignDelegate: public QStyledItemDelegate{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->displayAlignment = Qt::AlignRight;
    }
};


//---------------------------------------------------------
//   PluginDialog
//    select Plugin dialog
//---------------------------------------------------------

PluginDialog::PluginDialog(QWidget* parent)
  : QDialog(parent)
{
    ui.setupUi(this);
    // this dlg is called from the mixer strip so it inherits the small font size
    setStyleSheet("* {font-size:" + QString::number(MusEGlobal::config.fonts[0].pointSize()) + "pt}");
    ui.sortBox->setStyleSheet("font-size:" + QString::number(MusEGlobal::config.fonts[0].pointSize()) + "pt");

      group_info=nullptr;
      setWindowTitle(tr("MusE: Select Plugin"));

      if(!geometrySave.isNull())
        setGeometry(geometrySave);

      //QVBoxLayout* layout = new QVBoxLayout(this);

      //tabBar = new QTabBar(this);
      ui.tabBar->setToolTip(tr("Plugin categories.\nRight-click on tabs to manage.\nRight-click on plugins to add/remove from a category."));
      ui.tabBar->addTab("All");
      for (QList<QString>::iterator it=MusEGlobal::plugin_group_names.begin(); it!=MusEGlobal::plugin_group_names.end(); it++)
        ui.tabBar->addTab(*it);


      ui.pList->setColumnCount(COL_COUNT);

      // "Note: In order to avoid performance issues, it is recommended that sorting
      //   is enabled after inserting the items into the tree. Alternatively, you could
      //   also insert the items into a list before inserting the items into the tree. "

      QStringList headerLabels;
      headerLabels << tr("Name");
      headerLabels << tr("Type");
      headerLabels << tr("Aud In");
      headerLabels << tr("Aud Out");
      headerLabels << tr("Ctrl In");
      headerLabels << tr("Ctrl Out");
      headerLabels << tr("InPlace");
      headerLabels << tr("FixBlk");
      headerLabels << tr("BlkX2");
      headerLabels << tr("ID");
      headerLabels << tr("Maker");
      headerLabels << tr("Label");
      headerLabels << tr("URI/Library");
      headerLabels << tr("Copyright");

      ui.pList->setHeaderLabels(headerLabels);

      ui.pList->headerItem()->setToolTip(COL_AUDIO_IN,  tr("Audio inputs"));
      ui.pList->headerItem()->setToolTip(COL_AUDIO_OUT,  tr("Audio outputs"));
      ui.pList->headerItem()->setToolTip(COL_CTRL_IN,  tr("Control inputs"));
      ui.pList->headerItem()->setToolTip(COL_CTRL_OUT,  tr("Control outputs"));
      ui.pList->headerItem()->setToolTip(COL_INPLACE,  tr("In-place capable"));
      ui.pList->headerItem()->setToolTip(COL_FIXED_BLOCK,  tr("Requires fixed block size"));
      ui.pList->headerItem()->setToolTip(COL_POWER_2, tr("Requires power-of-2 block size"));
      ui.pList->headerItem()->setToolTip(COL_ID, tr("ID number"));

      ui.pList->setRootIsDecorated(false);
      ui.pList->setSelectionBehavior(QAbstractItemView::SelectRows);
      ui.pList->setSelectionMode(QAbstractItemView::SingleSelection);
      ui.pList->setAlternatingRowColors(true);
      ui.pList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      ui.pList->setContextMenuPolicy(Qt::CustomContextMenu);

      RightAlignDelegate *alignDelegate = new RightAlignDelegate(ui.pList);
      ui.pList->setItemDelegateForColumn(COL_AUDIO_IN, alignDelegate);
      ui.pList->setItemDelegateForColumn(COL_AUDIO_OUT, alignDelegate);
      ui.pList->setItemDelegateForColumn(COL_CTRL_IN, alignDelegate);
      ui.pList->setItemDelegateForColumn(COL_CTRL_OUT, alignDelegate);
      ui.pList->setItemDelegateForColumn(COL_ID, alignDelegate);


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
      newGroupAction= new QAction(tr("&Create new group"),ui.tabBar);
      delGroupAction= new QAction(tr("&Delete currently selected group"),ui.tabBar);
      renGroupAction= new QAction(tr("Re&name currently selected group"),ui.tabBar);
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
//      ui.pluginType->addItem("Wine VST", SEL_TYPE_WINE_VST);

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

      ui.pList->header()->setCascadingSectionResizes(true);
      ui.pList->setSortingEnabled(true);
      ui.pList->sortByColumn(COL_NAME, Qt::AscendingOrder);

      ui.pList->resizeColumnToContents(COL_NAME);
      ui.pList->resizeColumnToContents(COL_TYPE);
      ui.pList->setColumnWidth(COL_AUDIO_IN, 50);
      ui.pList->setColumnWidth(COL_AUDIO_OUT, 60);
      ui.pList->setColumnWidth(COL_CTRL_IN, 50);
      ui.pList->setColumnWidth(COL_CTRL_OUT, 60);
      ui.pList->setColumnWidth(COL_INPLACE, 50);
      ui.pList->setColumnWidth(COL_FIXED_BLOCK, 50);
      ui.pList->setColumnWidth(COL_POWER_2, 50);
      ui.pList->setColumnWidth(COL_ID, 40);
      ui.pList->setColumnWidth(COL_MAKER, 120);

      if (!listSave.isEmpty())
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
    menu->addAction(new MusEGui::MenuTitleItem(tr("Associated categories"), menu));

    if (ui.tabBar->count()==1)
    {
      QAction* tmp=menu->addAction(tr("[You need to define some categories first]"));
      tmp->setEnabled(false);
    }
    else
    {
      for (int i=1; i<ui.tabBar->count(); i++) // ignore the first tab ("All")
      {
        QAction* act=menu->addAction(ui.tabBar->tabText(i));
        act->setCheckable(true);
        act->setChecked(group_info->contains(i));
        connect(act, &QAction::toggled, [this, i]() { groupMenuEntryToggled(i); } );
      }
    }

    menu->exec(mapToGlobal(point));

    delete menu;

    if (selectedGroup!=0 && !group_info->contains(selectedGroup)) // we removed the entry from the currently visible group
      fillPlugs();

    group_info=nullptr;
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
  ui.tabBar->insertTab(selectedGroup+1, tr("New group"));
  MusEGlobal::plugin_group_names.insert(selectedGroup, tr("New group"));
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
      PluginItem* item = static_cast<PluginItem*>(ui.pList->currentItem());
      if (item)
      {
        return MusEGlobal::plugins.find(
          !item->hasUri() ? item->text(COL_URI) : QString(),
          item->hasUri()  ? item->text(COL_URI) : QString(),
          item->text(COL_LABEL));
      }
      printf("plugin not found\n");
      return nullptr;
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
            else if((*i)->isVstNativeSynth()) {
               type_name = tr("VST synth");
               plugInstanceType = SEL_TYPE_VST;
            }
            else if((*i)->isVstNativePlugin()) {
               type_name = tr("VST effect");
               plugInstanceType = SEL_TYPE_VST;
            }
            else {
               type_name = tr("ladspa");
               plugInstanceType = SEL_TYPE_LADSPA;
            }

            // last check
            if ( selectedPlugType != SEL_TYPE_ALL && plugInstanceType != selectedPlugType ) {
               continue;
            }

            PluginItem* item = new PluginItem(!(*i)->uri().isEmpty());
            item->setText(COL_TYPE,  type_name);
            if(!(*i)->uri().isEmpty())
              item->setText(COL_URI,  (*i)->uri());
            else
              item->setText(COL_URI,  (*i)->lib());

            item->setText(COL_LABEL,  (*i)->label());
            item->setText(COL_NAME,  (*i)->name());
            item->setText(COL_AUDIO_IN,  QString().setNum(ai));
            item->setText(COL_AUDIO_OUT,  QString().setNum(ao));
            item->setText(COL_CTRL_IN,  QString().setNum(ci));
            item->setText(COL_CTRL_OUT,  QString().setNum(co));
            bool flag = !((*i)->requiredFeatures() & MusECore::PluginNoInPlaceProcessing);
            item->setText(COL_INPLACE,  flag ? tr("Yes") : tr("No"));
            flag =(*i)->requiredFeatures() & MusECore::PluginFixedBlockSize;
            item->setText(COL_FIXED_BLOCK,  flag ? tr("Yes") : tr("No"));
            flag = (*i)->requiredFeatures() & MusECore::PluginPowerOf2BlockSize;
            item->setText(COL_POWER_2,  flag ? tr("Yes") : tr("No"));
            item->setText(COL_ID,  QString().setNum((*i)->id()));
            item->setText(COL_MAKER,  (*i)->maker());
            item->setText(COL_COPYRIGHT,  (*i)->copyright());
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
      MusECore::Plugin* p = nullptr;
      int rv = dialog->exec();
      if(rv)
        p = dialog->value();
      delete dialog;
      return p;
}

}

