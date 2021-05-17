#ifndef SYNTHDIALOG_H
#define SYNTHDIALOG_H

#include "ui_synthdialogbase.h"
#include <QDialog>
#include <QTreeWidgetItem>
#include <QAbstractButton>

#include "globaldefs.h"
#include "synth.h"
#include "globals.h"

namespace MusECore {
class Plugin;
}

namespace MusEGui {

class SynthDialog : public QDialog {
    Q_OBJECT

    enum SynthCategory { SEL_CAT_ALL = 0, SEL_CAT_SYNTH = 1, SEL_CAT_EFFECT = 2 };
    enum SynthType { SEL_TYPE_MESS, SEL_TYPE_DSSI, SEL_TYPE_LV2, SEL_TYPE_VST, SEL_TYPE_ALL};
    enum Tab { TAB_ALL = 0, TAB_FAV = 1 };
    enum Col { COL_NAME = 0, COL_TYPE, COL_CAT, COL_AUTHOR, COL_VERSION, COL_URI };
    enum UserData { UDATA_INDEX = Qt::UserRole, UDATA_HAS_URI = Qt::UserRole + 1, UDATA_NAME = Qt::UserRole + 2 };

public:
    explicit SynthDialog(QWidget* parent = nullptr);
    static MusECore::Synth* getSynth(QWidget* parent);
    static int getSynthIndex(QWidget* parent);
    MusECore::Synth* value();

    static void writeFavConfiguration(int level, MusECore::Xml& xml);
    static void readFavConfiguration(MusECore::Xml& xml);
    static void writeRecentsConfiguration(int level, MusECore::Xml& xml);
    static void readRecentsConfiguration(MusECore::Xml& xml);
    static bool isFav(MusECore::Synth *synth);
    static QVector<int> getFavsIdx();
    static QVector<int> getRecentsIdx();
    static void addRecent(MusECore::Synth *synth);

public slots:
    void accept();
    void reject();

private slots:
    void tabChanged(int);
    void fillSynths();
    void filterType(int);

    void listContextMenu(const QPoint&);
    void categoryChanged(QAbstractButton *button);
    void onCurrentItemChanged();
    void on_pbAddFav_clicked();
    void on_pbRemoveFav_clicked();

    void on_pbInfo_clicked();

private:
    bool favChanged;
    void saveSettings();
    void addToFavorites(QTreeWidgetItem *item);
    void removeFavorite(QTreeWidgetItem *item);
    bool isFavItem(QTreeWidgetItem *item);

    static QByteArray getHash(MusECore::Synth *synth);

    static int selCategory;
    static int selType;
    static int curTab;
    static QStringList filterSavedItems;
    static QRect geometrySave;
    static QByteArray listSave;
    static QSet<QByteArray> favs;
    static QList<QByteArray> recents;

    static const int RECENTS_SIZE = 10;

    Ui::SynthDialogBase ui;
};

}


#endif // SYNTHDIALOGBASE_H
