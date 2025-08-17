#include <QMenu>
#include <QCryptographicHash>
#include <QLineEdit>
#include <QMessageBox>

#include "synthdialog.h"
#include "gconfig.h"
//#include "icons.h"
#include "app.h"


namespace MusEGui {

int SynthDialog::selType = SEL_TYPE_ALL;
int SynthDialog::selCategory = SEL_CAT_SYNTH;
int SynthDialog::curTab = TAB_ALL;
QStringList SynthDialog::filterSavedItems = QStringList();
QRect SynthDialog::geometrySave = QRect();
QByteArray SynthDialog::listSave = QByteArray();
QSet<QByteArray> SynthDialog::favs = QSet<QByteArray>();
QList<QByteArray> SynthDialog::recents = QList<QByteArray>();


//---------------------------------------------------------
//   SynthDialog
//    select Plugin dialog
//---------------------------------------------------------

SynthDialog::SynthDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    setWindowTitle(tr("Select Software Synthesizer"));

    favChanged = false;

    if(!geometrySave.isNull())
        setGeometry(geometrySave);

    ui.tabBar->addTab("All");
    ui.tabBar->addTab("Favorites");

    ui.pList->setAlternatingRowColors(true);

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

    ui.pluginType->addItem(tr("All"), SEL_TYPE_ALL);
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

    // "In order to avoid performance issues, it is recommended that sorting is enabled after inserting the items into the tree."
    ui.pList->setSortingEnabled(false);
    fillSynths();
    ui.pList->setSortingEnabled(true);

    if(listSave.isEmpty())
    {
        ui.pList->header()->resizeSection(COL_NAME, 300);
        ui.pList->header()->resizeSection(COL_LABEL, 120);
        ui.pList->header()->resizeSection(COL_TYPE, 50);
        ui.pList->header()->resizeSection(COL_CAT, 64);
        ui.pList->header()->resizeSection(COL_AUTHOR, 120);
        ui.pList->header()->resizeSection(COL_VERSION, 64);
        // TODO: Right alignment so that the end of the text is always shown
        //        instead of the http://www part. Or text middle elide.
        ui.pList->header()->resizeSection(COL_URI, 120);
        ui.pList->header()->resizeSection(COL_DESCRIPTION, 300);

        ui.pList->sortByColumn(COL_NAME, Qt::AscendingOrder);
    }
    else
        ui.pList->header()->restoreState(listSave);

    connect(ui.pList,   SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(accept()));
    connect(ui.pList,   &QTreeWidget::currentItemChanged, this, &SynthDialog::onCurrentItemChanged );
    connect(ui.pList,   SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(listContextMenu(const QPoint&)));
    connect(ui.cancelB, SIGNAL(clicked()), SLOT(reject()));
    connect(ui.okB,     SIGNAL(clicked()), SLOT(accept()));
    connect(ui.tabBar,  SIGNAL(currentChanged(int)), SLOT(tabChanged(int)));
    connect(ui.filterBox, SIGNAL(editTextChanged(const QString&)),SLOT(fillSynths()));
    connect(ui.catButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &SynthDialog::categoryChanged);

    ui.pList->setCurrentItem(ui.pList->topLevelItem(0));
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
        QAction *add = new QAction(tr("Add to Favorites"), menu);
        QAction *remove = new QAction(tr("Remove from Favorites"), menu);
        isFavItem(item) ? add->setEnabled(false) : remove->setEnabled(false);
        menu->addAction(add);
        menu->addAction(remove);
        QAction *sel = menu->exec(QCursor::pos());
        if (sel == add)
            addToFavorites(item);
        else if (sel == remove)
            removeFavorite(item);
    } else {
        menu->addAction(new QAction(tr("Remove from Favorites"), menu));
        if (menu->exec(QCursor::pos()))
            removeFavorite(item);
    }

    delete menu;
}

void SynthDialog::addToFavorites(QTreeWidgetItem *item) {

    QByteArray hash = QCryptographicHash::hash(item->data(COL_NAME, UDATA_NAME).toString().toUtf8()
                                               + item->text(COL_URI).toUtf8(),
                                               QCryptographicHash::Md5);
    favs.insert(hash);
    item->setForeground(COL_NAME, Qt::red);
    QFont fb(font());
    fb.setItalic(true);
    item->setFont(COL_NAME, fb);

    favChanged = true;
    ui.pbRemoveFav->setEnabled(true);
    ui.pbAddFav->setEnabled(false);
}

void SynthDialog::removeFavorite(QTreeWidgetItem *item) {

    QByteArray hash = QCryptographicHash::hash(item->data(COL_NAME, UDATA_NAME).toString().toUtf8()
                                               + item->text(COL_URI).toUtf8(),
                                               QCryptographicHash::Md5);
    favs.remove(hash);

    if (curTab == TAB_ALL) {
        item->setForeground(COL_NAME, palette().text().color());
        item->setFont(COL_NAME, font());
        ui.pbRemoveFav->setEnabled(false);
        ui.pbAddFav->setEnabled(true);
    } else {
        auto selItem = ui.pList->itemBelow(ui.pList->currentItem());
        if (!selItem)
            selItem = ui.pList->itemAbove(ui.pList->currentItem());
        ui.pList->takeTopLevelItem(ui.pList->indexOfTopLevelItem(item));
        if (selItem)
            ui.pList->setCurrentItem(selItem);
        else
            ui.pList->setCurrentItem(ui.pList->topLevelItem(0));
    }

    favChanged = true;
}

QByteArray SynthDialog::getHash(MusECore::Synth *synth)
{
    QString urifile = synth->uri().isEmpty() ? synth->completeBaseName() : synth->uri();
    return QCryptographicHash::hash(synth->name().toUtf8() + urifile.toUtf8(),
                                               QCryptographicHash::Md5);
}


bool SynthDialog::isFavItem(QTreeWidgetItem *item)
{
    if (!item)
        return false;

    QByteArray hash = QCryptographicHash::hash(item->data(COL_NAME, UDATA_NAME).toString().toUtf8()
                                               + item->text(COL_URI).toUtf8(),
                                               QCryptographicHash::Md5);
    return favs.contains(hash);
}

bool SynthDialog::isFav(MusECore::Synth *synth)
{
    return favs.contains(getHash(synth));
}

QVector<int> SynthDialog::getFavsIdx()
{
    QVector<int> si;
    int i = 0;
    for (const auto& it : MusEGlobal::synthis) {
        if (isFav(it))
            si.push_back(i);
        ++i;
    }
    return si;
}

QVector<int> SynthDialog::getRecentsIdx()
{
    QVector<int> si;
    for (const auto& it : recents) {
        int i = 0;
        bool found = false;
        for (const auto& its : MusEGlobal::synthis) {
            if (it == getHash(its)) {
                found = true;
                break;
            } else {
                ++i;
            }
        }
        if (found)
            si.push_back(i);
    }
    return si;
}

void SynthDialog::addRecent(MusECore::Synth *synth)
{
    QByteArray hash = getHash(synth);

    if (recents.contains(hash)) {
        if (recents.indexOf(hash) == 0)
            return;
        else
            recents.removeOne(hash);
    }

    recents.prepend(hash);

    while (recents.size() > RECENTS_SIZE)
        recents.removeLast();
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

        return MusEGlobal::synthis.find(
          MusEPlugin::PluginType(item->data(COL_TYPE, UDATA_TYPE).toInt()),
          base,
          uri,
          item->data(COL_LABEL, UDATA_LABEL).toString());
    }
    fprintf(stderr, "Synth not found\n");
    return nullptr;
}

//---------------------------------------------------------
//   saveSettings
//---------------------------------------------------------

void SynthDialog::saveSettings()
{
//    if (!ui.filterBox->currentText().isEmpty()) {
        bool found = false;
        for (const auto& it : filterSavedItems)
            if(it == ui.filterBox->currentText()) {
                found = true;
                break;
            }
        if(!found)
            filterSavedItems.push_front(ui.filterBox->currentText());
//    }

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
}

void SynthDialog::onCurrentItemChanged()
{
    QTreeWidgetItem* item = ui.pList->currentItem();
    if (!item) {
        ui.okB->setEnabled(false);
        ui.pbRemoveFav->setEnabled(false);
        ui.pbAddFav->setEnabled(false);
        return;
    }

    ui.okB->setEnabled(true);

    if (curTab == TAB_ALL) {
        if (isFavItem(item)) {
            ui.pbRemoveFav->setEnabled(true);
            ui.pbAddFav->setEnabled(false);
        } else {
            ui.pbRemoveFav->setEnabled(false);
            ui.pbAddFav->setEnabled(true);
        }
    } else {
        ui.pbRemoveFav->setEnabled(true);
        ui.pbAddFav->setEnabled(false);
    }
}

void SynthDialog::fillSynths()
{
    ui.pList->clear();
    ui.okB->setEnabled(false);
    ui.pbAddFav->setEnabled(false);
    ui.pbRemoveFav->setEnabled(false);

    QString type_name, cat_name;

    if (curTab == TAB_FAV && favs.isEmpty())
        return;

    int index = -1;
    QFont fb(font());
    fb.setItalic(true);
    for (const auto& it : MusEGlobal::synthis)
    {
        index++;
        if (curTab == TAB_FAV && !isFav(it)) {
            continue;
        }

        QString sb_txt = ui.filterBox->currentText().toLower();
        if (!(sb_txt.isEmpty() || it->name().toLower().contains(sb_txt)))
            continue;

        if((selCategory == SEL_CAT_EFFECT && !(it->pluginClass() & MusEPlugin::PluginClassEffect)) ||
           (selCategory == SEL_CAT_SYNTH && !(it->pluginClass() & MusEPlugin::PluginClassInstrument)))
          continue;

        switch(it->pluginType())
        {
          case MusEPlugin::PluginTypeMESS:
            if(selType != SEL_TYPE_MESS && selType != SEL_TYPE_ALL)
                continue;
          break;

          case MusEPlugin::PluginTypeDSSI:
          case MusEPlugin::PluginTypeDSSIVST:
            if(selType != SEL_TYPE_DSSI && selType != SEL_TYPE_ALL)
                continue;
          break;

          case MusEPlugin::PluginTypeLV2:
            if(selType != SEL_TYPE_LV2 && selType != SEL_TYPE_ALL)
                continue;
          break;

          case MusEPlugin::PluginTypeLinuxVST:
            if(selType != SEL_TYPE_VST && selType != SEL_TYPE_ALL)
                continue;
          break;

          case MusEPlugin::PluginTypeLADSPA:
          case MusEPlugin::PluginTypeVST:
          case MusEPlugin::PluginTypeMETRONOME:
          case MusEPlugin::PluginTypeNone:
          case MusEPlugin::PluginTypeUnknown:
            continue;
          break;
        }

        type_name = QApplication::translate("MusEPlugin", MusEPlugin::pluginTypeToString(it->pluginType()));
        cat_name  = QApplication::translate("MusEPlugin", MusEPlugin::pluginClassToString(it->pluginClass()));

        QTreeWidgetItem* item = new QTreeWidgetItem(ui.pList);
        item->setText(COL_NAME, it->name());
        item->setText(COL_LABEL, it->label());
        item->setText(COL_TYPE, type_name);
        item->setText(COL_CAT, cat_name);
        item->setText(COL_AUTHOR, it->maker());
        item->setText(COL_VERSION, it->version());
        item->setText(COL_URI, it->uri().isEmpty() ? it->completeBaseName() : it->uri());
        item->setText(COL_DESCRIPTION, it->description());

        item->setData(COL_NAME, UDATA_INDEX, index);
        item->setData(COL_NAME, UDATA_HAS_URI, !it->uri().isEmpty());
        item->setData(COL_NAME, UDATA_NAME, it->name());
        item->setData(COL_LABEL, UDATA_LABEL, it->label());
        item->setData(COL_TYPE, UDATA_TYPE, it->pluginType());

        if (curTab == TAB_ALL && isFav(it)) {
            item->setForeground(COL_NAME, Qt::red);
            item->setFont(COL_NAME, fb);
        }
    }

    ui.pList->setCurrentItem(ui.pList->topLevelItem(0));
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
                favs.insert(QByteArray::fromHex(xml.parse1().toUtf8()));
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

void SynthDialog::writeRecentsConfiguration(int level, MusECore::Xml& xml)
{
    xml.tag(level++, "synthDialogRecents");

    for (const auto& it : recents)
        xml.strTag(level, "hash", QLatin1String(it.toHex()));

    xml.etag(--level, "synthDialogRecents");
}

void SynthDialog::readRecentsConfiguration(MusECore::Xml& xml)
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
            if (tag=="hash") {
                // parallel MusE instances can create duplicates in .cfg, so better check
                auto hash = QByteArray::fromHex(xml.parse1().toUtf8());
                if (!recents.contains(hash))
                    recents.append(hash);
            } else
                xml.unknown("readSynthRecentsConfiguration");
            break;

        case MusECore::Xml::TagEnd:
            if (tag == "synthDialogRecents")
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

void SynthDialog::on_pbInfo_clicked()
{
    QMessageBox::information(this, "Effects as synth tracks",
        "Multi-channel effects can be inserted as pseudo-synth tracks "
        "if more than 2 channels and/or MIDI are needed."
        "\nIn all other cases the Effect rack in the mixer strip should be used.");
}



} // namespace GUI
