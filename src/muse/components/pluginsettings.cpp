#include "pluginsettings.h"
#include "ui_pluginsettings.h"

#include "song.h"
#include "icons.h"
#include "app.h"
#include "globals.h"


namespace MusEGui {

PluginSettings::PluginSettings(MusECore::PluginIBase *plugin, bool globalScaleRevert, QWidget *parent)
    : QDialog(parent),
    ui(new Ui::PluginSettings)
{
    ui->setupUi(this);
    ui->pbInfo->setIcon(*infoSVGIcon);

    ui->cbFixedSpeed->setChecked(plugin->quirks()._fixedSpeed);
    ui->cbFixedSpeed->setEnabled(plugin->usesTransportSource());

    ui->cbTransportAffectsLatency->setChecked(plugin->quirks()._transportAffectsAudioLatency);
    ui->cbTransportAffectsLatency->setEnabled(plugin->usesTransportSource());

    ui->cbOverrideLatency->setChecked(plugin->quirks()._overrideReportedLatency);
    ui->sbOverrideLatency->setValue(plugin->quirks()._latencyOverrideValue);
    ui->sbOverrideLatency->setEnabled(plugin->cquirks()._overrideReportedLatency);

    ui->labelRevertScalingGlobal->setText(QString(tr("Global setting: ") + (globalScaleRevert ? tr("On") : tr("Off"))));
    if (plugin->quirks().getFixNativeUIScaling() == MusECore::PluginQuirks::GLOBAL)
        ui->rbRevertScalingFollowGlobal->setChecked(true);
    else if (plugin->quirks().getFixNativeUIScaling() == MusECore::PluginQuirks::ON)
        ui->rbRevertScalingOn->setChecked(true);
    else
        ui->rbRevertScalingOff->setChecked(true);

    settings = &plugin->quirks();
}

PluginSettings::~PluginSettings()
{
    delete ui;
}

void PluginSettings::on_buttonBox_accepted()
{
    bool routeChanged(false);

    if (ui->cbFixedSpeed->isChecked() != settings->_fixedSpeed) {
        settings->_fixedSpeed = ui->cbFixedSpeed->isChecked();
        routeChanged = true;
    }
    if (ui->cbTransportAffectsLatency->isChecked() != settings->_transportAffectsAudioLatency) {
        settings->_transportAffectsAudioLatency = ui->cbTransportAffectsLatency->isChecked();
        routeChanged = true;
    }
    if (ui->cbOverrideLatency->isChecked() != settings->_overrideReportedLatency) {
        settings->_overrideReportedLatency = ui->cbOverrideLatency->isChecked();
        if (!ui->cbOverrideLatency->isChecked())
            settings->_latencyOverrideValue = 0;
        routeChanged = true;
    } else if (ui->cbOverrideLatency->isChecked() &&
            ui->sbOverrideLatency->value() != settings->_latencyOverrideValue) {
        settings->_latencyOverrideValue = ui->sbOverrideLatency->value();
        routeChanged = true;
    } else if (!ui->cbOverrideLatency->isChecked() &&
               settings->_latencyOverrideValue != 0) {
        settings->_latencyOverrideValue = 0;
        routeChanged = true;
    }

    if (routeChanged)
        MusEGlobal::song->update(SC_ROUTE);

    MusECore::PluginQuirks::NatUISCaling scaleMode;
    if (ui->rbRevertScalingFollowGlobal->isChecked())
        scaleMode = MusECore::PluginQuirks::GLOBAL;
    else if (ui->rbRevertScalingOn->isChecked())
        scaleMode = MusECore::PluginQuirks::ON;
    else
        scaleMode = MusECore::PluginQuirks::OFF;

    if (scaleMode != settings->getFixNativeUIScaling())
        settings->setFixNativeUIScaling(scaleMode);
}

void PluginSettings::on_buttonBox_rejected()
{
//    close();
}

void PluginSettings::on_cbOverrideLatency_toggled(bool checked)
{
    ui->sbOverrideLatency->setEnabled(checked);
}

void PluginSettings::on_pbInfo_clicked()
{
    QString s("https://github.com/muse-sequencer/muse/wiki/HiDPI");
    MusEGlobal::muse->launchBrowser(s);
}


} // namespace
