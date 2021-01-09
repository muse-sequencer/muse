#include <QMenu>
#include <QInputDialog>

#include "globaldefs.h"
#include "popupmenu.h"
#include "menutitleitem.h"

#include "synthdialog.h"
#include "gconfig.h"
#include "icons.h"


namespace MusEGui {

int SynthDialog::selType = SEL_TYPE_ALL;
int SynthDialog::selCategory = SEL_CAT_ALL;
int SynthDialog::curTab = 0;
QStringList SynthDialog::sortItems = QStringList();
QRect SynthDialog::geometrySave = QRect();
QByteArray SynthDialog::listSave = QByteArray();
QVector<QTreeWidgetItem *> SynthDialog:: favs = QVector<QTreeWidgetItem *>();


//---------------------------------------------------------
//   SynthDialog
//    select Plugin dialog
//---------------------------------------------------------

SynthDialog::SynthDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    // this dlg is called from the mixer strip so it would inherit the small font size
    setStyleSheet("* {font-size:" + QString::number(MusEGlobal::config.fonts[0].pointSize()) + "pt}");

//    group_info=nullptr;
    setWindowTitle(tr("MusE: Select Synth"));

    if(!geometrySave.isNull())
        setGeometry(geometrySave);

    ui.tabBar->addTab("All");
    ui.tabBar->addTab("Favorites");

    ui.pList->setColumnCount(5);

    // "Note: In order to avoid performance issues, it is recommended that sorting
    //   is enabled after inserting the items into the tree. Alternatively, you could
    //   also insert the items into a list before inserting the items into the tree. "

    QStringList headerLabels;
    headerLabels << tr("Name");
    headerLabels << tr("Type");
    headerLabels << tr("Category");
    headerLabels << tr("Description");
    headerLabels << tr("URI");
    //    headerLabels << tr("Maker");
    //    headerLabels << tr("Copyright");

    ui.pList->setHeaderLabels(headerLabels);

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

    ui.catButtonGroup->setId(ui.rbAll, 0);
    ui.catButtonGroup->setId(ui.rbSynths, 1);
    ui.catButtonGroup->setId(ui.rbEffects, 2);

    switch(selCategory) {
    case SEL_CAT_ALL:  ui.rbAll->setChecked(true);  break;
    case SEL_CAT_SYNTH: ui.rbSynths->setChecked(true);   break;
    case SEL_CAT_EFFECT: ui.rbEffects->setChecked(true);   break;
    }

    ui.tabBar->setCurrentIndex(curTab);
//    ui.tabBar->setMovable(true);

    ui.pluginType->addItem("All", SEL_TYPE_ALL);
    ui.pluginType->addItem("MESS", SEL_TYPE_MESS);
    ui.pluginType->addItem("DSSI", SEL_TYPE_DSSI);
    ui.pluginType->addItem("LV2", SEL_TYPE_LV2);
    ui.pluginType->addItem("VST", SEL_TYPE_VST);

    connect (ui.pluginType,SIGNAL(currentIndexChanged(int)), SLOT(filterType(int)));

    for (int i=0; i < ui.pluginType->count(); i++) {
        if (selType == ui.pluginType->itemData(i).toInt()) {
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
        ui.pList->header()->resizeSection(0, 240);
        ui.pList->header()->resizeSection(1, 40);
        ui.pList->header()->resizeSection(2, 64);
        ui.pList->header()->resizeSection(4, 200);
        ui.pList->header()->resizeSection(5, 100);

        ui.pList->header()->setSectionResizeMode(1, QHeaderView::Fixed);
        ui.pList->header()->setSectionResizeMode(2, QHeaderView::Fixed);

        ui.pList->sortByColumn(0, Qt::AscendingOrder);
    }
    else
        ui.pList->header()->restoreState(listSave);

    connect(ui.pList,   SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(accept()));
    connect(ui.pList,   SIGNAL(itemClicked(QTreeWidgetItem*,int)), SLOT(enableOkB()));
    connect(ui.pList,   SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(plistContextMenu(const QPoint&)));
    connect(ui.cancelB, SIGNAL(clicked()), SLOT(reject()));
    connect(ui.okB,     SIGNAL(clicked()), SLOT(accept()));
    connect(ui.tabBar,  SIGNAL(currentChanged(int)), SLOT(tabChanged(int)));
    //connect(tabBar,  SIGNAL(tabMoved(int,int)), SLOT(tabMoved(int,int)));
    connect(ui.sortBox, SIGNAL(editTextChanged(const QString&)),SLOT(fillPlugs()));
    connect(ui.catButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &SynthDialog::categoryChanged);

    ui.sortBox->setFocus();
}

void SynthDialog::filterType(int i)
{
    selType = ui.pluginType->itemData(i).toInt();

    fillPlugs();
}

void SynthDialog::plistContextMenu(const QPoint& point)
{
    QTreeWidgetItem* item = ui.pList->currentItem();
    if (item)
    {
//        group_info = &MusEGlobal::plugin_groups.get(item->text(1), item->text(2));
        QMenu* menu = new QMenu();
//        QMenu* menu = new MusEGui::PopupMenu(this, true);
        menu->addAction(new QAction(tr("Add to Favorites"), menu));
//        menu->addAction(new MusEGui::MenuTitleItem(tr("Add to Favorites"), menu));

//            for (int i=1; i<ui.tabBar->count(); i++) // ignore the first tab ("All")
//            {
//                QAction* act=menu->addAction(ui.tabBar->tabText(i));
//                act->setCheckable(true);
//                act->setChecked(group_info->contains(i));
//                connect(act, &QAction::toggled, [this, i]() { groupMenuEntryToggled(i); } );
//            }

        if (menu->exec(mapToGlobal(point)))
            addToFavorites(item);

        delete menu;
    }
}

void SynthDialog::addToFavorites(QTreeWidgetItem *item) {
    favs.append(item);
}

//---------------------------------------------------------
//   enableOkB
//---------------------------------------------------------

void SynthDialog::enableOkB()
{
    ui.okB->setEnabled(true);
}

//---------------------------------------------------------
//   value
//---------------------------------------------------------

MusECore::Synth* SynthDialog::value()
{
    //    QTreeWidgetItem* item = static_cast<PluginItem*>(ui.pList->currentItem());
    QTreeWidgetItem* item = ui.pList->currentItem();
    if (item) {
        return MusEGlobal::synthis.find(
                    item->text(4).isNull() ? item->text(3) : QString(),
                    !item->text(4).isNull() ? item->text(4) : QString(),
                    item->text(0));
    }
    printf("Synth not found\n");
    return nullptr;
}

//---------------------------------------------------------
//   saveSettings
//---------------------------------------------------------

void SynthDialog::saveSettings()
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

void SynthDialog::accept()
{
    saveSettings();
    QDialog::accept();
}

//---------------------------------------------------------
//   reject
//---------------------------------------------------------

void SynthDialog::reject()
{
    saveSettings();
    QDialog::reject();
}

void SynthDialog::tabChanged(int index)
{
    curTab = index;
    fillPlugs();
}

void SynthDialog::fillPlugs()
{

    ui.pList->clear();
    ui.okB->setEnabled(false);

    QString type_name, cat_name;

    if (curTab == 1 && favs.isEmpty())
        return;

    int cnt = -1;
    for (const auto& it : MusEGlobal::synthis)
    {
        cnt++;
//        if (curTab == 1 && favs.contains(it))

        QString sb_txt = ui.sortBox->currentText().toLower();
        if (!(sb_txt.isEmpty() || it->name().toLower().contains(sb_txt)))
            continue;

        if (it->synthType() == MusECore::Synth::MESS_SYNTH) {
            if ((selCategory != SEL_CAT_SYNTH && selCategory != SEL_CAT_ALL)
                    || (selType != SEL_TYPE_MESS && selType != SEL_TYPE_ALL))
                continue;

            type_name = "MESS";
            cat_name = "Synth";

        } else if (it->synthType() == MusECore::Synth::DSSI_SYNTH) {
            if ((selCategory != SEL_CAT_SYNTH && selCategory != SEL_CAT_ALL)
                    || (selType != SEL_TYPE_DSSI && selType != SEL_TYPE_ALL))
                continue;

            type_name = "DSSI";
            cat_name = "Synth";

        } else if (it->synthType() == MusECore::Synth::LV2_SYNTH) {
            if ((selCategory != SEL_CAT_SYNTH && selCategory != SEL_CAT_ALL)
                    || (selType != SEL_TYPE_LV2 && selType != SEL_TYPE_ALL))
                continue;

            type_name = "LV2";
            cat_name = "Synth";

        } else if (it->synthType() == MusECore::Synth::VST_NATIVE_SYNTH) {
            if ((selCategory != SEL_CAT_SYNTH && selCategory != SEL_CAT_ALL)
                    || (selType != SEL_TYPE_VST && selType != SEL_TYPE_ALL))
                continue;

            type_name = "VST";
            cat_name = "Synth";

        } else if (it->synthType() == MusECore::Synth::LV2_EFFECT) {
            if ((selCategory != SEL_CAT_EFFECT && selCategory != SEL_CAT_ALL)
                    || (selType != SEL_TYPE_LV2 && selType != SEL_TYPE_ALL))
                continue;

            type_name = "LV2";
            cat_name = "Effect";

        } else if (it->synthType() == MusECore::Synth::VST_NATIVE_EFFECT) {
            if ((selCategory != SEL_CAT_EFFECT && selCategory != SEL_CAT_ALL)
                    || (selType != SEL_TYPE_VST && selType != SEL_TYPE_ALL))
                continue;

            type_name = "VST";
            cat_name = "Effect";
        }

        QTreeWidgetItem* item = new QTreeWidgetItem(ui.pList);
        item->setText(0,  it->name());
        item->setText(1,  type_name);
        item->setText(2,  cat_name);
        item->setText(3,  it->completeBaseName());

        if(!it->uri().isEmpty())
            item->setText(4, it->uri());

        item->setData(0, Qt::UserRole, cnt);
    }
}

//---------------------------------------------------------
//   getPlugin
//---------------------------------------------------------

MusECore::Synth *SynthDialog::getSynth(QWidget* parent)
{
    SynthDialog* dialog = new SynthDialog(parent);
    MusECore::Synth* p = nullptr;
    int rv = dialog->exec();
    if(rv)
        p = dialog->value();
    delete dialog;
    return p;
}

int SynthDialog::getSynthIndex(QWidget* parent)
{
    SynthDialog* dialog = new SynthDialog(parent);
    int rc = -1;
    int rv = dialog->exec();
    if(rv)
    rc = dialog->ui.pList->currentItem()->data(0, Qt::UserRole).toInt();
    delete dialog;
    return rc;
}

void SynthDialog::categoryChanged(QAbstractButton *button)
{
    selCategory = ui.catButtonGroup->id(button);
    fillPlugs();
}


}
