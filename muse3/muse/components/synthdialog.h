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

public:
    explicit SynthDialog(QWidget* parent = nullptr);
    static MusECore::Synth* getSynth(QWidget* parent);
    static int getSynthIndex(QWidget* parent);
    MusECore::Synth* value();

public slots:
    void accept();
    void reject();

private slots:
    void enableOkB();
    void tabChanged(int);
    void fillPlugs();
    void filterType(int);

    void plistContextMenu(const QPoint&);
//    void groupMenuEntryToggled(int i);
    void categoryChanged(QAbstractButton *button);

private:
    void saveSettings();
    void addToFavorites(QTreeWidgetItem *item);

    static int selCategory;
    static int selType;
    static int curTab; // 0 means "show all"
    static QStringList sortItems;
    static QRect geometrySave;
    static QByteArray listSave;

    static QVector<QTreeWidgetItem *> favs;
//    QSet<int>* group_info; //holds the group-set of the plugin which shall be affected by the plistContextMenu.

    Ui::SynthDialogBase ui;
};

}


#endif // SYNTHDIALOGBASE_H
