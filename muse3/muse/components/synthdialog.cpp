#include <QMenu>
#include <QCryptographicHash>

#include "synthdialog.h"
#include "gconfig.h"
#include "icons.h"
#include "app.h"


namespace MusEGui {

int SynthDialog::selType = SEL_TYPE_ALL;
int SynthDialog::selCategory = SEL_CAT_ALL;
int SynthDialog::curTab = TAB_ALL;
QStringList SynthDialog::filterSavedItems = QStringList();
QRect SynthDialog::geometrySave = QRect();
QByteArray SynthDialog::listSave = QByteArray();
QSet<QByteArray> SynthDialog::favs = QSet<QByteArray>();


//---------------------------------------------------------
//   SynthDialog
//    select Plugin dialog
//---------------------------------------------------------

SynthDialog::SynthDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    //    setStyleSheet("* {font-size:" + QString::number(MusEGlobal::config.fonts[0].pointSize()) + "pt}");

    setWindowTitle(tr("Select Software Synthesizer"));

    favChanged = false;

    if(!geometrySave.isNull())
        setGeometry(geometrySave);

    ui.tabBar->addTab("All");
    ui.tabBar->addTab("Favorites");

    //    ui.pList->setColumnCount(5);

    ui.pList->setRootIsDecorated(false);
    ui.pList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.pList->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.pList->setAlternatingRowColors(true);
    ui.pList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui.pList->setContextMenuPolicy(Qt::CustomContextMenu);

    ui.okB->setDefault(true);
    ui.okB->setEnabled(false);

    ui.catButtonGroup->setId(ui.rbAll, 0);
    ui.catButtonGroup->setId(ui.rbSynths, 1);
    ui.catButtonGroup->setId(ui.rbEffects, 2);

    switch(selCategory) {
    case SEL_CAT_ALL:  ui.rbAll->setChecked(true);
        break;
    case SEL_CAT_SYNTH: ui.rbSynths->setChecked(true);
        break;
    case SEL_CAT_EFFECT: ui.rbEffects->setChecked(true);
        break;
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
            break;
        }
    }

    ui.filterBox->addItems(filterSavedItems);

    fillSynths();

    ui.pList->setSortingEnabled(true);

    if(listSave.isEmpty())
    {
        ui.pList->header()->resizeSection(COL_NAME, 300);
        ui.pList->header()->resizeSection(COL_TYPE, 50);
        ui.pList->header()->resizeSection(COL_CAT, 64);
        ui.pList->header()->resizeSection(COL_AUTHOR, 120);
        ui.pList->header()->resizeSection(COL_VERSION, 64);
        ui.pList->header()->resizeSection(COL_URI, 300);

        ui.pList->sortByColumn(0, Qt::AscendingOrder);
    }
    else
        ui.pList->header()->restoreState(listSave);

    connect(ui.pList,   SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(accept()));
    connect(ui.pList,   &QTreeWidget::itemClicked, [this](){ ui.okB->setEnabled(true); } );
    connect(ui.pList,   SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(listContextMenu(const QPoint&)));
    connect(ui.cancelB, SIGNAL(clicked()), SLOT(reject()));
    connect(ui.okB,     SIGNAL(clicked()), SLOT(accept()));
    connect(ui.tabBar,  SIGNAL(currentChanged(int)), SLOT(tabChanged(int)));
    connect(ui.filterBox, SIGNAL(editTextChanged(const QString&)),SLOT(fillSynths()));
    connect(ui.catButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &SynthDialog::categoryChanged);

    ui.filterBox->setFocus();
}

void SynthDialog::filterType(int i)
{
    selType = ui.pluginType->itemData(i).toInt();
    fillSynths();
}

void SynthDialog::listContextMenu(const QPoint& )
{
    QTreeWidgetItem* item = ui.pList->currentItem();
    if (!item)
        return;

    QMenu* menu = new QMenu();

    if (curTab == TAB_ALL) {
        menu->addAction(new QAction(tr("Add to Favorites"), menu));
        if (menu->exec(QCursor::pos()))
            addToFavorites(item);
    } else {
        menu->addAction(new QAction(tr("Remove from Favorites"), menu));
        if (menu->exec(QCursor::pos()))
            removeFavorite(item);
    }

    delete menu;
}

void SynthDialog::addToFavorites(QTreeWidgetItem *item) {

    QByteArray a = QCryptographicHash::hash(item->data(COL_NAME, UDATA_NAME).toString().toUtf8()
                                            + item->text(COL_URI).toUtf8(),
                                            QCryptographicHash::Md5);
    favs.insert(a);
    item->setForeground(COL_NAME, Qt::red);

    favChanged = true;
}

void SynthDialog::removeFavorite(QTreeWidgetItem *item) {

    QByteArray a = QCryptographicHash::hash(item->data(COL_NAME, UDATA_NAME).toString().toUtf8()
                                            + item->text(COL_URI).toUtf8(),
                                            QCryptographicHash::Md5);
    favs.remove(a);

    if (curTab == TAB_ALL)
        item->setForeground(COL_NAME, palette().text().color());
    else {
        ui.pList->takeTopLevelItem(ui.pList->indexOfTopLevelItem(item));
        if (!ui.pList->currentItem())
            ui.okB->setEnabled(false);
    }

    favChanged = true;
}

bool SynthDialog::isFav(MusECore::Synth *synth)
{
    QString urifile = synth->uri().isEmpty() ? synth->completeBaseName() : synth->uri();
    QByteArray hash = QCryptographicHash::hash(synth->name().toUtf8() + urifile.toUtf8(),
                                               QCryptographicHash::Md5);

    return favs.contains(hash);
}

//---------------------------------------------------------
//   value
//---------------------------------------------------------

MusECore::Synth* SynthDialog::value()
{
    QTreeWidgetItem* item = ui.pList->currentItem();

    if (item) {
        QString uri, base;
        bool hasURI = item->data(COL_NAME, UDATA_HAS_URI).toBool();
        if (hasURI)
            uri = item->text(COL_URI);
        else
            base = item->text(COL_URI);

        return MusEGlobal::synthis.find(uri, base, item->data(COL_NAME, UDATA_NAME).toString());
    }
    printf("Synth not found\n");
    return nullptr;
}

//---------------------------------------------------------
//   saveSettings
//---------------------------------------------------------

void SynthDialog::saveSettings()
{
    if (!ui.filterBox->currentText().isEmpty()) {
        bool found = false;
        foreach (QString item, filterSavedItems)
            if(item == ui.filterBox->currentText()) {
                found = true;
                break;
            }
        if(!found)
            filterSavedItems.push_front(ui.filterBox->currentText());
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
    if (!ui.pList->currentItem())
        return;

    saveSettings();
    if (favChanged)
        MusEGlobal::muse->populateAddTrack();

    QDialog::accept();
}

//---------------------------------------------------------
//   reject
//---------------------------------------------------------

void SynthDialog::reject()
{
    saveSettings();
    if (favChanged)
        MusEGlobal::muse->populateAddTrack();

    QDialog::reject();
}

void SynthDialog::tabChanged(int index)
{
    curTab = index;
    fillSynths();

    ui.pbAddFav->setEnabled(curTab == TAB_ALL);
}

void SynthDialog::fillSynths()
{

    ui.pList->clear();
    ui.okB->setEnabled(false);

    QString type_name, cat_name;

    if (curTab == TAB_FAV && favs.isEmpty())
        return;

    int index = -1;
    for (const auto& it : MusEGlobal::synthis)
    {
        index++;
        if (curTab == TAB_FAV && !isFav(it)) {
            continue;
        }

        QString sb_txt = ui.filterBox->currentText().toLower();
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
        item->setText(COL_NAME, it->description());
        item->setText(COL_TYPE, type_name);
        item->setText(COL_CAT, cat_name);
        item->setText(COL_AUTHOR, it->maker());
        item->setText(COL_VERSION, it->version());
        item->setText(COL_URI, it->uri().isEmpty() ? it->completeBaseName() : it->uri());

        item->setData(COL_NAME, UDATA_INDEX, index);
        item->setData(COL_NAME, UDATA_HAS_URI, !it->uri().isEmpty());
        item->setData(COL_NAME, UDATA_NAME, it->name());

        if (curTab == TAB_ALL && isFav(it)) {
            item->setForeground(COL_NAME, Qt::red);
        }
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
        rc = dialog->ui.pList->currentItem()->data(COL_NAME, UDATA_INDEX).toInt();
    delete dialog;

    return rc;
}

void SynthDialog::categoryChanged(QAbstractButton *button)
{
    selCategory = ui.catButtonGroup->id(button);
    fillSynths();
}

void SynthDialog::writeFavConfiguration(int level, MusECore::Xml& xml)
{
    xml.tag(level++, "synthDialogFavorites");

    for (const auto& it : favs)
        xml.strTag(level, "hash", QLatin1String(it.toHex()));

    xml.etag(--level, "synthDialogFavorites");
}

void SynthDialog::readFavConfiguration(MusECore::Xml& xml)
{
    for (;;)
    {
        MusECore::Xml::Token token = xml.parse();
        if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
            break;

        const QString& tag = xml.s1();
        switch (token)
        {
        case MusECore::Xml::TagStart:
            if (tag=="hash")
                favs.insert(QByteArray::fromHex(xml.parse1().toLatin1()));
            else
                xml.unknown("readSynthFavConfiguration");
            break;

        case MusECore::Xml::TagEnd:
            if (tag == "synthDialogFavorites")
                return;
            break;

        default:
            break;
        }
    }
}

void SynthDialog::on_pbAddFav_clicked()
{
    if (curTab == TAB_FAV)
        return;

    QTreeWidgetItem *item = ui.pList->currentItem();
    if (item)
        addToFavorites(item);
}

void SynthDialog::on_pbRemoveFav_clicked()
{
    QTreeWidgetItem *item = ui.pList->currentItem();
    if (item)
        removeFavorite(item);
}


} // namespace GUI