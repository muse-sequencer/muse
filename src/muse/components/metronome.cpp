//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: metronome.cpp,v 1.2.2.1 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include <QMenu>
#include <QDir>
#include <QFileInfoList>
#include <QToolButton>
#include <QPainter>
#include <QIcon>
#include <QMessageBox>

#include <stdio.h>
#include "metronome.h"
#include "midi_consts.h"
#include "ticksynth.h"

#include "globals.h"
#include "gconfig.h"
#include "song.h"
#include "track.h"
#include "audio.h"
#include "operations.h"
#include "pixmap_button.h"
#include "icons.h"

namespace MusEGui {

MetronomePresetItemWidget::MetronomePresetItemWidget(
  QIcon* on_icon, QIcon* off_icon, const MusECore::MetroAccentsStruct& mas,
  bool hasFixedIconSize, int margin, QWidget* parent, const char* name)
  : QFrame(parent), _onIcon(on_icon), _offIcon(off_icon),
    _hasFixedIconSize(hasFixedIconSize), _margin(margin), _accents(mas)
{
  setObjectName(name);
  setAttribute(Qt::WA_TranslucentBackground);
  _iconSize = QSize(10, 10);
}

QSize MetronomePresetItemWidget::minimumSizeHint () const
{
  return QSize(10, 10);
}

void MetronomePresetItemWidget::setMargin(int v)
{
  _margin = v;
  update();
}

void MetronomePresetItemWidget::setOffIcon(QIcon* pm)
{
  _offIcon = pm;
  update();
}

void MetronomePresetItemWidget::setOnIcon(QIcon* pm)
{
  _onIcon = pm;
  update();
}

void MetronomePresetItemWidget::setIconSize(const QSize sz)
{
  _iconSize = sz;
  updateGeometry();
}


QSize MetronomePresetItemWidget::sizeHint() const
{
  const int beats = _accents._accents.size();
  
  // TODO Ask style for margins.
  const QSize isz = iconSize();
  const int fmh = fontMetrics().lineSpacing() + 2;

  const int iw = isz.width() + 2;
  const int ih = isz.height() + 2;

  const int h = (_hasFixedIconSize && ih > fmh) ? ih : fmh;
  const int w = beats * ((_hasFixedIconSize && iw > h) ? iw : h + 2);

  const int fin_w = w + 2 * _margin;
  // For two lines: 5 margins.
  const int fin_h = 2 * h + 2 * _margin + 1;
  return QSize(fin_w, fin_h);
}

void MetronomePresetItemWidget::paintEvent(QPaintEvent* ev)
{
  ev->accept();
  QPainter p(this);

  const int beats = _accents._accents.size();

  // TODO Ask style for margins.
  const QSize isz = iconSize();
  const int fmh = fontMetrics().lineSpacing() + 2;

  const int iw = isz.width() + 2;
  const int ih = isz.height() + 2;

  const int ico_h = (_hasFixedIconSize && ih > fmh) ? ih : fmh;
  const int ico_w = ((_hasFixedIconSize && iw > ico_h) ? iw : ico_h + 2);

  QIcon::Mode mode;
  if(isEnabled())
    mode = hasFocus() ? QIcon::Selected : QIcon::Normal;
  else
    mode = QIcon::Disabled;

  QIcon::State state;
  bool acc1, acc2;
  QIcon* ico;
  QRect r;
  for(int i = 0; i < beats; ++i)
  {
    const MusECore::MetroAccent& ma = _accents._accents.at(i);
    acc1 = ma._accentType & MusECore::MetroAccent::Accent1;
    acc2 = ma._accentType & MusECore::MetroAccent::Accent2;

    r = QRect(i * ico_w, _margin, ico_w, ico_h - 1);
    state = acc1 ? QIcon::On : QIcon::Off;
    ico = acc1 ? _onIcon : _offIcon;
    if(ico)
      ico->paint(&p, r, Qt::AlignCenter, mode, state);
    
    r.moveTop(_margin + ico_h + 1);
    state = acc2 ? QIcon::On : QIcon::Off;
    ico = acc2 ? _onIcon : _offIcon;
    if(ico)
      ico->paint(&p, r, Qt::AlignCenter, mode, state);

  }
}

//---------------------------------------------------------
//   MetronomeConfig
//---------------------------------------------------------

MetronomeConfig::MetronomeConfig(QWidget* parent)
   : QDialog(parent)
{
      setupUi(this);

      accentPresetsTypeItemActivated(0);

      updateValues();

      // TODO Let's only do this once for now. It may be too slow to call with every update or songChanged signal,
      //       so for now any user-additional files in our metronome waves directory will need a restart...
      fillSoundFiles();

      // Special for these two: Need qt helper overload for these lambdas.
      connect(accentBeats, QOverload<int>::of(&QSpinBox::valueChanged), [=](int v) { accentBeatsChanged(v); } );
      connect(accentPresetTypeList, QOverload<int>::of(&QComboBox::activated), [=](int index) { accentPresetsTypeItemActivated(index); } );

      connect(buttonApply, &QPushButton::clicked, [this]() { apply(); } );
      connect(midiClick, &QCheckBox::toggled, [this](bool v) { midiClickChanged(v); } );
      connect(precountEnable, &QCheckBox::toggled, [this](bool v) { precountEnableChanged(v); } );
      connect(precountFromMastertrack, &QCheckBox::toggled, [this](bool v) { precountFromMastertrackChanged(v); } );
      connect(audioBeepRoutesButton, &QPushButton::clicked, [this]() { audioBeepRoutesClicked(); } );
      connect(volumeSlider, &QSlider::valueChanged, [this](int v) { volumeChanged(v); } );
      connect(measVolumeSlider, &QSlider::valueChanged, [this](int v) { measVolumeChanged(v); } );
      connect(beatVolumeSlider, &QSlider::valueChanged, [this](int v) { beatVolumeChanged(v); } );
      connect(accent1VolumeSlider, &QSlider::valueChanged, [this](int v) { accent1VolumeChanged(v); } );
      connect(accent2VolumeSlider, &QSlider::valueChanged, [this](int v) { accent2VolumeChanged(v); } );
      connect(radioSamples2, &QRadioButton::toggled, [this]() { switchSamples(); } );
      connect(MusEGlobal::song, &MusECore::Song::songChanged, [this](MusECore::SongChangedStruct_t type) { songChanged(type); } );
      connect(globalSettingsButton, &QRadioButton::toggled, [this]() { switchSettings(); } );
      connect(accentPresets, &QListWidget::itemActivated, [this](QListWidgetItem* item) { accentPresetsItemActivated(item); } );
      connect(addAccentPresetButton, &QToolButton::clicked, [this]() { addAccentsPresetClicked(); } );
      connect(delAccentPresetButton, &QToolButton::clicked, [this]() { delAccentsPresetClicked(); } );
      connect(useAccentPresetButton, &QToolButton::clicked, [this]() { useAccentsPresetClicked(); } );
      connect(accentsDefaultsButton, &QToolButton::clicked, [this]() { accentsResetDefaultClicked(); } );
}

void MetronomeConfig::songChanged(MusECore::SongChangedStruct_t type)
{
  if(type & SC_METRONOME)
  {
    updateValues();
  }
}

void MetronomeConfig::setAccentsSettings(int beats, const MusECore::MetroAccentsStruct& mas)
{
  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;
  
  // Make a working copy, to replace the original.
  MusECore::MetroAccentsMap* new_accents_map = new MusECore::MetroAccentsMap(*metro_settings->metroAccentsMap);

  // Manipulate the working copy...
  if(mas.isBlank())
  {
    // Don't allow adding if there are no accents set. Erase any existing found.
    MusECore::MetroAccentsMap::iterator imap = new_accents_map->find(beats);
    if(imap != new_accents_map->end())
      new_accents_map->erase(imap);
  }
  else
  {
    std::pair<MusECore::MetroAccentsMap::iterator, bool> res =
      new_accents_map->insert(std::pair<const int, MusECore::MetroAccentsStruct>(beats, mas));
    if(!res.second)
      res.first->second = mas;
  }

  MusECore::PendingOperationList operations;
  // Takes ownership of the original which it deletes at the end of the operation.
  operations.add(MusECore::PendingOperationItem(
    &metro_settings->metroAccentsMap, new_accents_map,
    MusECore::PendingOperationItem::ModifyMetronomeAccentMap));
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
} 

void MetronomeConfig::accentPresetsItemActivated(QListWidgetItem* /*item*/)
{
  updateAccentPresetAddButton();
  updateAccentPresetDelButton();
}

void MetronomeConfig::accentPresetsTypeItemActivated(int /*idx*/)
{
  const int beats = accentBeats->value();
  fillAccentPresets(beats);
//   configureAccentButtons(beats);
//   updateAccentButtons(beats);
  updateAccentPresetAddButton();
  updateAccentPresetDelButton();
}

void MetronomeConfig::getAccents(int beats, MusECore::MetroAccentsStruct* mas) const
{
  QLayoutItem* item;
  IconButton* button;
  const int count1 = accent1ButtonsLayout->count();
  const int count2 = accent2ButtonsLayout->count();
  for(int i = 0; i < beats; ++i)
  {
    MusECore::MetroAccent ma;
    if(i < count1)
    {
      item = accent1ButtonsLayout->itemAt(i);
      if(item && !item->isEmpty())
      {
        button = static_cast<IconButton*>(item->widget());
        if(button && button->isChecked())
          ma._accentType |= MusECore::MetroAccent::Accent1;
      }
    }
    if(i < count2)
    {
      item = accent2ButtonsLayout->itemAt(i);
      if(item && !item->isEmpty())
      {
        button = static_cast<IconButton*>(item->widget());
        if(button && button->isChecked())
          ma._accentType |= MusECore::MetroAccent::Accent2;
      }
    }
    mas->_accents.push_back(ma);
  }
}

void MetronomeConfig::changeAccents()
{
  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

  if(!metro_settings->metroAccentsMap)
    return;

  const int beats = accentBeats->value();
  if(beats <= 0)
    return;

  MusECore::MetroAccentsStruct mas(MusECore::MetroAccentsStruct::User);
  getAccents(beats, &mas);
  setAccentsSettings(beats, mas);
}

void MetronomeConfig::clearAccents(MusECore::MetroAccent::AccentType row)
{
  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

  if(!metro_settings->metroAccentsMap)
    return;

  const int beats = accentBeats->value();
  if(beats <= 0)
    return;

  MusECore::MetroAccentsStruct mas(MusECore::MetroAccentsStruct::User);
  getAccents(beats, &mas);
  mas.blank(row);
  setAccentsSettings(beats, mas);
}

//---------------------------------------------------------
//    fillSoundFiles
//---------------------------------------------------------

void MetronomeConfig::fillSoundFiles()
{
    MusECore::MetronomeSettings* metro_settings = 
      MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

    QDir metroPath(MusEGlobal::museGlobalShare+"/metronome");
    QStringList filters;
    filters.append("*.wav");
    QStringList klickfiles = metroPath.entryList(filters);

    measSampleCombo->clear();
    beatSampleCombo->clear();
    accent1SampleCombo->clear();
    accent2SampleCombo->clear();
    
    measSampleCombo->addItems(klickfiles);
    beatSampleCombo->addItems(klickfiles);
    accent1SampleCombo->addItems(klickfiles);
    accent2SampleCombo->addItems(klickfiles);

    measSampleCombo->setCurrentIndex(klickfiles.indexOf(metro_settings->measSample));
    beatSampleCombo->setCurrentIndex(klickfiles.indexOf(metro_settings->beatSample));
    accent1SampleCombo->setCurrentIndex(klickfiles.indexOf(metro_settings->accent1Sample));
    accent2SampleCombo->setCurrentIndex(klickfiles.indexOf(metro_settings->accent2Sample));
}

void MetronomeConfig::updateValues()
{
      MusECore::MetronomeSettings* metro_settings = 
        MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

      volumeSlider->blockSignals(true);
      measVolumeSlider->blockSignals(true);
      beatVolumeSlider->blockSignals(true);
      accent1VolumeSlider->blockSignals(true);
      accent2VolumeSlider->blockSignals(true);
      radioSamples2->blockSignals(true);
      radioSamples4->blockSignals(true);
      songSettingsButton->blockSignals(true);
      globalSettingsButton->blockSignals(true);

      measureNote->blockSignals(true);
      measureVelocity->blockSignals(true);
      beatNote->blockSignals(true);
      beatVelocity->blockSignals(true);
      accent1Note->blockSignals(true);
      accent1Velocity->blockSignals(true);
      accent2Note->blockSignals(true);
      accent2Velocity->blockSignals(true);
      midiChannel->blockSignals(true);
      midiPort->blockSignals(true);
      
      precountBars->blockSignals(true);
      precountEnable->blockSignals(true);
      precountFromMastertrack->blockSignals(true);
      precountSigZ->blockSignals(true);
      precountSigN->blockSignals(true);
      precountOnPlay->blockSignals(true);
      precountMuteMetronome->blockSignals(true);
      precountPrerecord->blockSignals(true);
      precountPreroll->blockSignals(true);

      midiClick->blockSignals(true);
      audioBeep->blockSignals(true);

      if (MusEGlobal::metroUseSongSettings)
          songSettingsButton->setChecked(true);
      else
          globalSettingsButton->setChecked(true);

      volumeSlider->setValue(metro_settings->audioClickVolume*100);
      measVolumeSlider->setValue(metro_settings->measClickVolume*100);
      beatVolumeSlider->setValue(metro_settings->beatClickVolume*100);
      accent1VolumeSlider->setValue(metro_settings->accent1ClickVolume*100);
      accent2VolumeSlider->setValue(metro_settings->accent2ClickVolume*100);
      if (metro_settings->clickSamples == metro_settings->origSamples)
          radioSamples2->setChecked(true);
      else
          radioSamples4->setChecked(true);

      volumeLabel->setText(QString::number(int(metro_settings->audioClickVolume*99)));
      measVolumeLabel->setText(QString::number(int(metro_settings->measClickVolume*99)));
      beatVolumeLabel->setText(QString::number(int(metro_settings->beatClickVolume*99)));
      accent1VolumeLabel->setText(QString::number(int(metro_settings->accent1ClickVolume*99)));
      accent2VolumeLabel->setText(QString::number(int(metro_settings->accent2ClickVolume*99)));

      measureNote->setValue(metro_settings->measureClickNote);
      measureVelocity->setValue(metro_settings->measureClickVelo);
      beatNote->setValue(metro_settings->beatClickNote);
      beatVelocity->setValue(metro_settings->beatClickVelo);
      accent1Note->setValue(metro_settings->accentClick1);
      accent1Velocity->setValue(metro_settings->accentClick1Velo);
      accent2Note->setValue(metro_settings->accentClick2);
      accent2Velocity->setValue(metro_settings->accentClick2Velo);
      midiChannel->setValue(metro_settings->clickChan+1);
      midiPort->setValue(metro_settings->clickPort+1);

      precountBars->setValue(metro_settings->preMeasures);
      precountEnable->setChecked(metro_settings->precountEnableFlag);
      precountFromMastertrack->setChecked(metro_settings->precountFromMastertrackFlag);
      precountSigZ->setValue(metro_settings->precountSigZ);
      precountSigN->setValue(metro_settings->precountSigN);
      precountOnPlay->setChecked(metro_settings->precountOnPlay);
      precountMuteMetronome->setChecked(metro_settings->precountMuteMetronome);
      precountPrerecord->setChecked(metro_settings->precountPrerecord);
      precountPreroll->setChecked(metro_settings->precountPreroll);
      
      midiClick->setChecked(metro_settings->midiClickFlag);
      audioBeep->setChecked(metro_settings->audioClickFlag);


      midiClickChanged(metro_settings->midiClickFlag);
      precountEnableChanged(metro_settings->precountEnableFlag);
      switchSamples(); // to disable gui elements
      // TODO Let's only do this once in the constructor for now. It may be too slow to call with every update or
      //       songChanged signal, so for now any user-additional files in our metronome waves directory will need a restart...
      //fillSoundFiles();

      const int beats = accentBeats->value();
      configureAccentButtons(beats);
      updateAccentButtons(beats);
      updateAccentPresetAddButton();
      updateAccentPresetDelButton();


      volumeSlider->blockSignals(false);
      measVolumeSlider->blockSignals(false);
      beatVolumeSlider->blockSignals(false);
      accent1VolumeSlider->blockSignals(false);
      accent2VolumeSlider->blockSignals(false);
      radioSamples2->blockSignals(false);
      radioSamples4->blockSignals(false);
      songSettingsButton->blockSignals(false);
      globalSettingsButton->blockSignals(false);

      measureNote->blockSignals(false);
      measureVelocity->blockSignals(false);
      beatNote->blockSignals(false);
      beatVelocity->blockSignals(false);
      accent1Note->blockSignals(false);
      accent1Velocity->blockSignals(false);
      accent2Note->blockSignals(false);
      accent2Velocity->blockSignals(false);
      midiChannel->blockSignals(false);
      midiPort->blockSignals(false);
      
      precountBars->blockSignals(false);
      precountEnable->blockSignals(false);
      precountFromMastertrack->blockSignals(false);
      precountSigZ->blockSignals(false);
      precountSigN->blockSignals(false);
      precountOnPlay->blockSignals(false);
      precountMuteMetronome->blockSignals(false);
      precountPrerecord->blockSignals(false);
      precountPreroll->blockSignals(false);

      midiClick->blockSignals(false);
      audioBeep->blockSignals(false);
}

//---------------------------------------------------------
//   audioBeepRoutesClicked
//---------------------------------------------------------

void MetronomeConfig::audioBeepRoutesClicked()
{
      if(MusEGlobal::song->outputs()->size() == 0)
        return;
        
      QMenu* pup = new QMenu;
      
      MusECore::OutputList* ol = MusEGlobal::song->outputs();

      int nn = 0;
      for(MusECore::iAudioOutput iao = ol->begin(); iao != ol->end(); ++iao)
      {
        QAction* action = pup->addAction((*iao)->name());
        action->setCheckable(true);
        action->setData(nn);
        if((*iao)->sendMetronome())
          action->setChecked(true);
        ++nn;  
      }  
      
      QAction* clickaction = pup->exec(QCursor::pos());
      if (clickaction)
      {
        nn = 0;
        for(MusECore::iAudioOutput iao = ol->begin(); iao != ol->end(); ++iao)
        {
          if (nn == clickaction->data())
          {
            MusEGlobal::audio->msgSetSendMetronome(*iao, clickaction->isChecked());
            break;
          }
          ++nn;
        }
      }
      
      delete pup;
      audioBeepRoutesButton->setDown(false);     // pup->exec() catches mouse release event
}

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void MetronomeConfig::accept()
      {
      apply();
      QDialog::accept();
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void MetronomeConfig::apply()
{
      MusECore::MetronomeSettings* metro_settings = 
        MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

      metro_settings->measureClickNote   = measureNote->value();
      metro_settings->measureClickVelo   = measureVelocity->value();
      metro_settings->beatClickNote      = beatNote->value();
      metro_settings->beatClickVelo      = beatVelocity->value();
      metro_settings->accentClick1       = accent1Note->value();
      metro_settings->accentClick1Velo   = accent1Velocity->value();
      metro_settings->accentClick2       = accent2Note->value();
      metro_settings->accentClick2Velo   = accent2Velocity->value();
      metro_settings->clickChan          = midiChannel->value() - 1;
      metro_settings->clickPort          = midiPort->value() - 1;
      metro_settings->preMeasures        = precountBars->value();

      metro_settings->precountEnableFlag = precountEnable->isChecked();
      metro_settings->precountFromMastertrackFlag = precountFromMastertrack->isChecked();
      metro_settings->precountSigZ      = precountSigZ->value();
      metro_settings->precountSigN      = precountSigN->value();
      metro_settings->precountOnPlay    = precountOnPlay->isChecked();
      metro_settings->precountMuteMetronome = precountMuteMetronome->isChecked();
      metro_settings->precountPrerecord = precountPrerecord->isChecked();
      metro_settings->precountPreroll   = precountPreroll->isChecked();

      metro_settings->midiClickFlag      = midiClick->isChecked();
      metro_settings->audioClickFlag     = audioBeep->isChecked();

      metro_settings->measSample = measSampleCombo->currentText();
      metro_settings->beatSample = beatSampleCombo->currentText();
      metro_settings->accent1Sample = accent1SampleCombo->currentText();
      metro_settings->accent2Sample = accent2SampleCombo->currentText();

      MusECore::PendingOperationList operations;
      MusECore::metronome->initSamplesOperation(operations);
      MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

//---------------------------------------------------------
//   reject
//---------------------------------------------------------

void MetronomeConfig::reject()
      {
      QDialog::reject();
      }

//---------------------------------------------------------
//   midiClickChanged
//---------------------------------------------------------

void MetronomeConfig::midiClickChanged(bool flag)
      {
      measureNote->setEnabled(flag);
      measureVelocity->setEnabled(flag);
      beatNote->setEnabled(flag);
      beatVelocity->setEnabled(flag);
      accent1Note->setEnabled(flag && radioSamples4->isChecked());
      accent1Velocity->setEnabled(flag && radioSamples4->isChecked());
      accent2Note->setEnabled(flag && radioSamples4->isChecked());
      accent2Velocity->setEnabled(flag && radioSamples4->isChecked());
      midiChannel->setEnabled(flag);
      midiPort->setEnabled(flag);
      }

void MetronomeConfig::precountEnableChanged(bool flag)
      {
      precountBars->setEnabled(flag);
      precountFromMastertrack->setEnabled(flag);
      precountSigZ->setEnabled(flag && !precountFromMastertrack->isChecked());
      precountSigN->setEnabled(flag && !precountFromMastertrack->isChecked());
      precountOnPlay->setEnabled(flag);
      precountMuteMetronome->setEnabled(flag);
      //precountPrerecord->setEnabled(flag); // Not supported yet.
      //precountPreroll->setEnabled(flag); // Not supported yet.
      }

void MetronomeConfig::precountFromMastertrackChanged(bool flag)
      {
      precountSigZ->setEnabled(!flag);
      precountSigN->setEnabled(!flag);
      }

// these values are directly applied, not using the Apply button, it just seems more usable this way.
void MetronomeConfig::volumeChanged(int volume) {
      MusECore::MetronomeSettings* metro_settings = 
        MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;
      metro_settings->audioClickVolume=volume/100.0;
      volumeLabel->setText(QString::number(int(metro_settings->audioClickVolume*99)));
}
void MetronomeConfig::measVolumeChanged(int volume) {
      MusECore::MetronomeSettings* metro_settings = 
        MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;
      metro_settings->measClickVolume=volume/100.0;
      measVolumeLabel->setText(QString::number(int(metro_settings->measClickVolume*99)));
}
void MetronomeConfig::beatVolumeChanged(int volume) {
      MusECore::MetronomeSettings* metro_settings = 
        MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;
      metro_settings->beatClickVolume=volume/100.0;
      beatVolumeLabel->setText(QString::number(int(metro_settings->beatClickVolume*99)));
}
void MetronomeConfig::accent1VolumeChanged(int volume) {
      MusECore::MetronomeSettings* metro_settings = 
        MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;
      metro_settings->accent1ClickVolume=volume/100.0;
      accent1VolumeLabel->setText(QString::number(int(metro_settings->accent1ClickVolume*99)));
}
void MetronomeConfig::accent2VolumeChanged(int volume) {
      MusECore::MetronomeSettings* metro_settings = 
        MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;
      metro_settings->accent2ClickVolume=volume/100.0;
      accent2VolumeLabel->setText(QString::number(int(metro_settings->accent2ClickVolume*99)));
}

void MetronomeConfig::switchSamples() {
    MusECore::MetronomeSettings* metro_settings = 
      MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

    if (radioSamples2->isChecked()) {
        metro_settings->clickSamples = metro_settings->origSamples;
        accent1VolumeSlider->setEnabled(false);
        accent2VolumeSlider->setEnabled(false);
        accent1Label->setEnabled(false);
        accent2Label->setEnabled(false);
        accent1Note->setEnabled(false);
        accent1Velocity->setEnabled(false);
        accent2Note->setEnabled(false);
        accent2Velocity->setEnabled(false);
        accentsTab->setEnabled(false);
    }
    else {
        metro_settings->clickSamples = metro_settings->newSamples;
        accent1VolumeSlider->setEnabled(true);
        accent2VolumeSlider->setEnabled(true);
        accent1Label->setEnabled(true);
        accent2Label->setEnabled(true);
        accent1Note->setEnabled(midiClick->isChecked());
        accent1Velocity->setEnabled(midiClick->isChecked());
        accent2Note->setEnabled(midiClick->isChecked());
        accent2Velocity->setEnabled(midiClick->isChecked());
        accentsTab->setEnabled(true);
       }
}

void MetronomeConfig::switchSettings()
{
  //const bool new_val = !MusEGlobal::metroUseSongSettings;
  const bool new_val = songSettingsButton->isChecked();
  MusECore::PendingOperationList operations;
  operations.add(MusECore::PendingOperationItem(
    &MusEGlobal::metroUseSongSettings, new_val,
    MusECore::PendingOperationItem::SwitchMetronomeSettings));
  // Let it do a songChanged update.
  // Let the songChanged handler update all the values and buttons...
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

void MetronomeConfig::accentBeatsChanged(int beats)
{
  fillAccentPresets(beats);
  configureAccentButtons(beats);
  updateAccentButtons(beats);
  updateAccentPresetAddButton();
  updateAccentPresetDelButton();
}

void MetronomeConfig::configureAccentButtons(int beats)
{
  const int count1 = accent1ButtonsLayout->count();
  const int count2 = accent2ButtonsLayout->count();

  // Nothing to do?
  if(beats == 0 && count1 == 0 && count2 == 0)
    return;
  const int button_count = beats + 1;
  if(count1 == button_count && count2 == button_count)
    return;

  QLayoutItem* item;
  QWidget* w;
  QList<QWidget*> wl1, wl2;

  if(count1 != button_count)
  {
    for(int i = 0; i < count1; ++i)
    {
      item = accent1ButtonsLayout->itemAt(i);
      if(item /*&& !item->isEmpty()*/)
      {
        w = item->widget();
        if(w)
          wl1.append(w);
      }
    }
    const int wl1_sz = wl1.size();
    for(int i = 0; i < wl1_sz; ++i)
    {
      delete wl1.at(i);
    }
    for(int i = 0; i < beats; ++i)
    {
      IconButton* b = new IconButton(ledGreenIcon, ledDarkGreenIcon, 0, 0, false, true);
      b->setCheckable(true);
      connect(b, &IconButton::clicked, [this]() { changeAccents(); } );
      accent1ButtonsLayout->addWidget(b);
    }
    // Add one more button - the 'clear all' button.
    if(beats > 0)
    {
      IconButton* b = new IconButton(icon_select_deselect_all, icon_select_deselect_all, 0, 0, false, true);
      connect(b, &IconButton::clicked, [this]() { clearAccents(MusECore::MetroAccent::Accent1); } );
      accent1ButtonsLayout->addWidget(b);
    }
  }

  if(count2 != button_count)
  {
    for(int i = 0; i < count2; ++i)
    {
      item = accent2ButtonsLayout->itemAt(i);
      if(item /*&& !item->isEmpty()*/)
      {
        w = item->widget();
        if(w)
          wl2.append(w);
      }
    }
    const int wl2_sz = wl2.size();
    for(int i = 0; i < wl2_sz; ++i)
    {
      //disconnect(wl2.at(i));
      delete wl2.at(i);
    }
    for(int i = 0; i < beats; ++i)
    {
      IconButton* b = new IconButton(ledGreenIcon, ledDarkGreenIcon, 0, 0, false, true);
      b->setCheckable(true);
      connect(b, &IconButton::clicked, [this]() { changeAccents(); } );
      accent2ButtonsLayout->addWidget(b);
    }
    // Add one more button - the 'clear all' button.
    if(beats > 0)
    {
      IconButton* b = new IconButton(icon_select_deselect_all, icon_select_deselect_all, 0, 0, false, true);
      connect(b, &IconButton::clicked, [this]() { clearAccents(MusECore::MetroAccent::Accent2); } );
      accent2ButtonsLayout->addWidget(b);
    }
  }
}

void MetronomeConfig::updateAccentPresetAddButton()
{
  const int beats = accentBeats->value();
  if(beats <= 0 || accentPresetTypeList->currentIndex() != UserPresetType)
  {
    addAccentPresetButton->setEnabled(false);
    return;
  }

  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

  if(!metro_settings->metroAccentsMap)
  {
    addAccentPresetButton->setEnabled(false);
    return;
  }

  const MusECore::MetroAccentsMap::const_iterator imap = metro_settings->metroAccentsMap->find(beats);
  if(imap == metro_settings->metroAccentsMap->cend())
  {
    addAccentPresetButton->setEnabled(false);
    return;
  }

  MusECore::MetroAccentsStruct mas = imap->second;

  // Don't allow adding if there are no accents set.
  if(mas._type != MusECore::MetroAccentsStruct::User || mas.isBlank())
  {
    addAccentPresetButton->setEnabled(false);
    return;
  }

  MusECore::MetroAccentsPresetsMap::const_iterator ipm = MusEGlobal::metroAccentPresets.find(beats);
  if(ipm == MusEGlobal::metroAccentPresets.cend())
  {
    addAccentPresetButton->setEnabled(true);
  }
  else
  {
    // Change the type to user preset just in case.
    mas._type = MusECore::MetroAccentsStruct::UserPreset;
    addAccentPresetButton->setEnabled(
      ipm->second.find(mas, MusECore::MetroAccentsStruct::AllTypes) == ipm->second.end());
  }
}

void MetronomeConfig::updateAccentPresetDelButton()
{
  QListWidgetItem* item = accentPresets->currentItem();
  delAccentPresetButton->setEnabled(item && item->data(PresetTypeRole).toInt() == MusECore::MetroAccentsStruct::UserPreset);
}

void MetronomeConfig::updateAccentButtons(int beats)
{
  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

  const int count1 = accent1ButtonsLayout->count();
  const int count2 = accent2ButtonsLayout->count();

  const MusECore::MetroAccents* accents = nullptr;
  int accents_sz = 0;
  if(metro_settings->metroAccentsMap)
  {
    const MusECore::MetroAccentsMap::const_iterator imap = metro_settings->metroAccentsMap->find(beats);
    if(imap != metro_settings->metroAccentsMap->cend())
    {
      const MusECore::MetroAccentsStruct& mas = imap->second;
      accents = &mas._accents;
      accents_sz = accents->size();
    }
  }
  
  QLayoutItem* item;
  QWidget* w;
  IconButton* button;

  for(int i = 0; i < beats; ++i)
  {
    if(i < count1)
    {
      item = accent1ButtonsLayout->itemAt(i);
      if(item /*&& !item->isEmpty()*/)
      {
        w = item->widget();
        if(w)
        {
          button = static_cast<IconButton*>(w);
          button->blockSignals(true);
          button->setChecked(accents && i < accents_sz && (accents->at(i)._accentType & MusECore::MetroAccent::Accent1));
          button->blockSignals(false);
        }
      }
    }
    if(i < count2)
    {
      item = accent2ButtonsLayout->itemAt(i);
      if(item /*&& !item->isEmpty()*/)
      {
        w = item->widget();
        if(w)
        {
          button = static_cast<IconButton*>(w);
          button->blockSignals(true);
          button->setChecked(accents && i < accents_sz && (accents->at(i)._accentType & MusECore::MetroAccent::Accent2));
          button->blockSignals(false);
        }
      }
    }
  }  
}

bool MetronomeConfig::addAccentPreset(int beats, const MusECore::MetroAccentsStruct& mas)
{
  // Accept either user presets or factory presets.
  if(mas._type != MusECore::MetroAccentsStruct::UserPreset && mas._type != MusECore::MetroAccentsStruct::FactoryPreset)
    return false;
  QListWidgetItem* new_item = new QListWidgetItem();
  MetronomePresetItemWidget* new_widget =
    new MetronomePresetItemWidget(
      ledGreenIcon, ledDarkGreenIcon, mas, true, 4, accentPresets, "MetronomePresetItemWidget");
  new_item->setData(BeatsRole, beats);
  new_item->setData(PresetIdRole, qlonglong(mas.id()));
  new_item->setData(PresetTypeRole, mas._type);

  // Find the LAST user preset item and insert after it, or else just append:
  const int sz = accentPresets->count();
  int idx = sz - 1;
  for( ; idx >= 0; --idx)
  {
    const QListWidgetItem* it = accentPresets->item(idx);
    if(it && it->data(PresetTypeRole).toInt() == MusECore::MetroAccentsStruct::UserPreset)
      break;
  }
  accentPresets->blockSignals(true);
  // If no user presets were found or the one found is the last item, just append.
  if(idx == -1 || (idx == sz - 1))
    accentPresets->addItem(new_item);
  else
    accentPresets->insertItem(idx + 1, new_item);
  accentPresets->setItemWidget(new_item, new_widget);
  new_item->setSizeHint(new_widget->sizeHint());
  accentPresets->blockSignals(false);
  return true;
}

void MetronomeConfig::fillAccentPresets(int beats)
{
  accentPresets->blockSignals(true);
  accentPresets->clear();
  accentPresets->blockSignals(false);

  const MusECore::MetroAccentsPresetsMap::const_iterator ipm = MusEGlobal::metroAccentPresets.find(beats);
  if(ipm == MusEGlobal::metroAccentPresets.cend())
    return;

  const int type_idx = accentPresetTypeList->currentIndex();
  if(type_idx != FactoryPresetType && type_idx != UserPresetType)
    return;

  const MusECore::MetroAccentsPresets& mp = ipm->second;
  const int map_sz = mp.size();
  // Sort the visual items: Put the factory presets before the user presets.
  // TODO: Actually, user presets are more important since the user made them,
  //        so maybe put them first, but then the factory presets would appear
  //        below them, which seems kinda counter-intuitive...
  //       Hm, but when we add a new user preset it wants to go to the end of the list...
  if(type_idx == FactoryPresetType)
  {
    for(int i = 0; i != map_sz; ++i)
    {
      const MusECore::MetroAccentsStruct& mas = mp.at(i);
      // Don't bother adding if there are no accents set.
      if(mas._type == MusECore::MetroAccentsStruct::FactoryPreset && !mas.isBlank())
        addAccentPreset(beats, mas);
    }
  }
  else if(type_idx == UserPresetType)
  {
    for(int i = 0; i != map_sz; ++i)
    {
      const MusECore::MetroAccentsStruct& mas = mp.at(i);
      // Don't bother adding if there are no accents set.
      if(mas._type == MusECore::MetroAccentsStruct::UserPreset && !mas.isBlank())
        addAccentPreset(beats, mas);
    }
  }
}

void MetronomeConfig::addAccentsPresetClicked()
{
  const int beats = accentBeats->value();
  if(beats <= 0 || accentPresetTypeList->currentIndex() != UserPresetType)
    return;

  MusECore::MetroAccentsStruct mas(MusECore::MetroAccentsStruct::User);
  getAccents(beats, &mas);

  // Don't bother adding if there are no accents set.
  // (This is an error because the add button should not be enabled if the accents are blank.)
  if(mas.isBlank())
    return;

  MusECore::MetroAccentsPresetsMap::iterator ipm = MusEGlobal::metroAccentPresets.find(beats);
  if(ipm == MusEGlobal::metroAccentPresets.end())
  {
    std::pair<MusECore::MetroAccentsPresetsMap::iterator, bool> res =
      MusEGlobal::metroAccentPresets.insert(
        std::pair<const int, MusECore::MetroAccentsPresets>(beats, MusECore::MetroAccentsPresets()));
    ipm = res.first;
  }

  if(ipm->second.find(mas, MusECore::MetroAccentsStruct::AllTypes) == ipm->second.end())
  {
    // Change the type to a user preset.
    mas._type = MusECore::MetroAccentsStruct::UserPreset;
    ipm->second.push_back(mas);
    addAccentPreset(beats, mas);
    updateAccentPresetAddButton();
    updateAccentPresetDelButton();
  }
}

void MetronomeConfig::delAccentsPresetClicked()
{
  QListWidgetItem* item = accentPresets->currentItem();
  if(!item)
    return;

  // Looking only for user presets.
  if(item->data(PresetTypeRole).toInt() != MusECore::MetroAccentsStruct::UserPreset)
    return;

  const int beats = item->data(BeatsRole).toInt();
  MusECore::MetroAccentsPresetsMap::iterator ipm = MusEGlobal::metroAccentPresets.find(beats);
  if(ipm != MusEGlobal::metroAccentPresets.end())
  {
    MusECore::MetroAccentsPresets& mp = ipm->second;
    const std::uint64_t id = item->data(PresetIdRole).toLongLong();
    MusECore::MetroAccentsPresets::iterator imap = mp.findId(id);
    if(imap != mp.end())
    {
      // Looking only for user presets.
      //if(imap->_type == MusECore::MetroAccentsStruct::UserPreset)
      {
        // Erase the preset.
        mp.erase(imap);
        // Erase the presets map as well if there are no more presets.
        if(mp.empty())
          MusEGlobal::metroAccentPresets.erase(ipm);
      }
    }
  }
  delete item;
  updateAccentPresetAddButton();
  updateAccentPresetDelButton();
}

void MetronomeConfig::useAccentsPresetClicked()
{
  QListWidgetItem* item = accentPresets->currentItem();
  if(!item)
    return;

  const int beats = item->data(BeatsRole).toInt();
  if(beats <= 0)
    return;

  const MusECore::MetroAccentsPresetsMap::const_iterator ipm = MusEGlobal::metroAccentPresets.find(beats);
  if(ipm == MusEGlobal::metroAccentPresets.cend())
    return;
  const MusECore::MetroAccentsPresets& mp = ipm->second;
  const std::uint64_t id = item->data(PresetIdRole).toLongLong();
  const MusECore::MetroAccentsPresets::const_iterator imap = mp.findId(id);
  if(imap == mp.cend())
    return;

  MusECore::MetroAccentsStruct mas = *imap;
  // Change the type to user.
  mas._type = MusECore::MetroAccentsStruct::User;

  setAccentsSettings(beats, mas);
}

void MetronomeConfig::accentsResetDefaultClicked()
{
  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

  if(!metro_settings->metroAccentsMap)
    return;

  MusECore::MetroAccentsStruct::MetroAccentsType type;
  if(accentPresetTypeList->currentIndex() == FactoryPresetType)
    type = MusECore::MetroAccentsStruct::FactoryPreset;
  else if(accentPresetTypeList->currentIndex() == UserPresetType)
    type = MusECore::MetroAccentsStruct::UserPreset;
  else
    return;

  QMessageBox::StandardButton b = QMessageBox::warning(this, tr("Reset accents:"),
      tr("Resets all accents to the defaults (first in list)\n of the current preset category (Factory or User).\nProceed?"),
      QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
  if(b != QMessageBox::Ok)
    return;

  // Make a new map, to replace the original.
  MusECore::MetroAccentsMap* new_accents_map = new MusECore::MetroAccentsMap();
  // Fill with defaults.
  MusEGlobal::metroAccentPresets.defaultAccents(new_accents_map, type);

  MusECore::PendingOperationList operations;
  // Takes ownership of the original which it deletes at the end of the operation.
  operations.add(MusECore::PendingOperationItem(
    &metro_settings->metroAccentsMap, new_accents_map,
    MusECore::PendingOperationItem::ModifyMetronomeAccentMap));
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

} // namespace MusEGui

