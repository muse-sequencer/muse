#ifndef PLUGINSETTINGS_H
#define PLUGINSETTINGS_H

#include <QDialog>

#include "plugin.h"

namespace Ui {
class PluginSettings;
}

namespace MusEGui {

class PluginSettings : public QDialog
{
    Q_OBJECT

public:
    explicit PluginSettings(MusECore::PluginIBase *plugin, bool globalScaleRevert, QWidget *parent = nullptr);
    ~PluginSettings();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

    void on_cbOverrideLatency_toggled(bool checked);

    void on_pbInfo_clicked();

private:
    Ui::PluginSettings *ui;

    MusECore::PluginQuirks *settings;
};

}
#endif // PLUGINSETTINGS_H
