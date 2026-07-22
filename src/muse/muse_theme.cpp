//=========================================================
//  MusE
//  Linux Music Editor
//
//  muse_theme.cpp
//  Copyright (C) 2026 by the MusE development team
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

#include "muse_theme.h"

#include "gconfig.h"
#include "globals.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>
#include <QPair>
#include <cstdio>

namespace MusEGui {

//---------------------------------------------------------
//   colorFieldTable
//    Name -> MusEGlobal::GlobalConfigValues member-pointer, one entry per
//     QColor field in gconfig.h (excluding palette[] - the user's
//     quick-pick swatch tray, not really theme data - and partColors[],
//     handled separately below since it's an array). Generated from
//     gconfig.h; keep in sync if fields are added/removed/renamed there.
//---------------------------------------------------------

namespace {

using ColorMember = QColor MusEGlobal::GlobalConfigValues::*;

const QVector<QPair<QString, ColorMember>>& colorFieldTable()
{
  static const QVector<QPair<QString, ColorMember>> table = {
  { "transportHandleColor", &MusEGlobal::GlobalConfigValues::transportHandleColor },
  { "bigTimeForegroundColor", &MusEGlobal::GlobalConfigValues::bigTimeForegroundColor },
  { "bigTimeBackgroundColor", &MusEGlobal::GlobalConfigValues::bigTimeBackgroundColor },
  { "waveEditBackgroundColor", &MusEGlobal::GlobalConfigValues::waveEditBackgroundColor },
  { "trackBg", &MusEGlobal::GlobalConfigValues::trackBg },
  { "selectTrackBg", &MusEGlobal::GlobalConfigValues::selectTrackBg },
  { "selectTrackFg", &MusEGlobal::GlobalConfigValues::selectTrackFg },
  { "selectTrackCurBg", &MusEGlobal::GlobalConfigValues::selectTrackCurBg },
  { "trackSectionDividerColor", &MusEGlobal::GlobalConfigValues::trackSectionDividerColor },
  { "midiTrackLabelBg", &MusEGlobal::GlobalConfigValues::midiTrackLabelBg },
  { "drumTrackLabelBg", &MusEGlobal::GlobalConfigValues::drumTrackLabelBg },
  { "newDrumTrackLabelBg", &MusEGlobal::GlobalConfigValues::newDrumTrackLabelBg },
  { "waveTrackLabelBg", &MusEGlobal::GlobalConfigValues::waveTrackLabelBg },
  { "outputTrackLabelBg", &MusEGlobal::GlobalConfigValues::outputTrackLabelBg },
  { "inputTrackLabelBg", &MusEGlobal::GlobalConfigValues::inputTrackLabelBg },
  { "groupTrackLabelBg", &MusEGlobal::GlobalConfigValues::groupTrackLabelBg },
  { "auxTrackLabelBg", &MusEGlobal::GlobalConfigValues::auxTrackLabelBg },
  { "synthTrackLabelBg", &MusEGlobal::GlobalConfigValues::synthTrackLabelBg },
  { "midiTrackBg", &MusEGlobal::GlobalConfigValues::midiTrackBg },
  { "drumTrackBg", &MusEGlobal::GlobalConfigValues::drumTrackBg },
  { "newDrumTrackBg", &MusEGlobal::GlobalConfigValues::newDrumTrackBg },
  { "waveTrackBg", &MusEGlobal::GlobalConfigValues::waveTrackBg },
  { "outputTrackBg", &MusEGlobal::GlobalConfigValues::outputTrackBg },
  { "inputTrackBg", &MusEGlobal::GlobalConfigValues::inputTrackBg },
  { "groupTrackBg", &MusEGlobal::GlobalConfigValues::groupTrackBg },
  { "auxTrackBg", &MusEGlobal::GlobalConfigValues::auxTrackBg },
  { "synthTrackBg", &MusEGlobal::GlobalConfigValues::synthTrackBg },
  { "partCanvasBg", &MusEGlobal::GlobalConfigValues::partCanvasBg },
  { "partCanvasCoarseRasterColor", &MusEGlobal::GlobalConfigValues::partCanvasCoarseRasterColor },
  { "partCanvasBeatRasterColor", &MusEGlobal::GlobalConfigValues::partCanvasBeatRasterColor },
  { "partCanvasFineRasterColor", &MusEGlobal::GlobalConfigValues::partCanvasFineRasterColor },
  { "ctrlGraphFg", &MusEGlobal::GlobalConfigValues::ctrlGraphFg },
  { "ctrlGraphSel", &MusEGlobal::GlobalConfigValues::ctrlGraphSel },
  { "rulerBg", &MusEGlobal::GlobalConfigValues::rulerBg },
  { "rulerFg", &MusEGlobal::GlobalConfigValues::rulerFg },
  { "midiCanvasBg", &MusEGlobal::GlobalConfigValues::midiCanvasBg },
  { "midiControllerViewBg", &MusEGlobal::GlobalConfigValues::midiControllerViewBg },
  { "drumListBg", &MusEGlobal::GlobalConfigValues::drumListBg },
  { "drumListFont", &MusEGlobal::GlobalConfigValues::drumListFont },
  { "drumListSel", &MusEGlobal::GlobalConfigValues::drumListSel },
  { "drumListSelFont", &MusEGlobal::GlobalConfigValues::drumListSelFont },
  { "rulerCurrent", &MusEGlobal::GlobalConfigValues::rulerCurrent },
  { "midiCanvasFineColor", &MusEGlobal::GlobalConfigValues::midiCanvasFineColor },
  { "midiCanvasBeatColor", &MusEGlobal::GlobalConfigValues::midiCanvasBeatColor },
  { "midiCanvasBarColor", &MusEGlobal::GlobalConfigValues::midiCanvasBarColor },
  { "midiItemColor", &MusEGlobal::GlobalConfigValues::midiItemColor },
  { "midiItemSelectedColor", &MusEGlobal::GlobalConfigValues::midiItemSelectedColor },
  { "dummyPartColor", &MusEGlobal::GlobalConfigValues::dummyPartColor },
  { "midiDividerColor", &MusEGlobal::GlobalConfigValues::midiDividerColor },
  { "pianoCurrentKey", &MusEGlobal::GlobalConfigValues::pianoCurrentKey },
  { "pianoPressedKey", &MusEGlobal::GlobalConfigValues::pianoPressedKey },
  { "pianoSelectedKey", &MusEGlobal::GlobalConfigValues::pianoSelectedKey },
  { "waveNonselectedPart", &MusEGlobal::GlobalConfigValues::waveNonselectedPart },
  { "wavePeakColor", &MusEGlobal::GlobalConfigValues::wavePeakColor },
  { "waveRmsColor", &MusEGlobal::GlobalConfigValues::waveRmsColor },
  { "wavePeakColorSelected", &MusEGlobal::GlobalConfigValues::wavePeakColorSelected },
  { "waveRmsColorSelected", &MusEGlobal::GlobalConfigValues::waveRmsColorSelected },
  { "partWaveColorPeak", &MusEGlobal::GlobalConfigValues::partWaveColorPeak },
  { "partWaveColorRms", &MusEGlobal::GlobalConfigValues::partWaveColorRms },
  { "partMidiDarkEventColor", &MusEGlobal::GlobalConfigValues::partMidiDarkEventColor },
  { "partMidiLightEventColor", &MusEGlobal::GlobalConfigValues::partMidiLightEventColor },
  { "sliderBarColor", &MusEGlobal::GlobalConfigValues::sliderBarColor },
  { "sliderBackgroundColor", &MusEGlobal::GlobalConfigValues::sliderBackgroundColor },
  { "panSliderColor", &MusEGlobal::GlobalConfigValues::panSliderColor },
  { "gainSliderColor", &MusEGlobal::GlobalConfigValues::gainSliderColor },
  { "auxSliderColor", &MusEGlobal::GlobalConfigValues::auxSliderColor },
  { "audioVolumeSliderColor", &MusEGlobal::GlobalConfigValues::audioVolumeSliderColor },
  { "midiVolumeSliderColor", &MusEGlobal::GlobalConfigValues::midiVolumeSliderColor },
  { "audioVolumeHandleColor", &MusEGlobal::GlobalConfigValues::audioVolumeHandleColor },
  { "midiVolumeHandleColor", &MusEGlobal::GlobalConfigValues::midiVolumeHandleColor },
  { "audioControllerSliderColor", &MusEGlobal::GlobalConfigValues::audioControllerSliderColor },
  { "audioPropertySliderColor", &MusEGlobal::GlobalConfigValues::audioPropertySliderColor },
  { "midiControllerSliderColor", &MusEGlobal::GlobalConfigValues::midiControllerSliderColor },
  { "midiPropertySliderColor", &MusEGlobal::GlobalConfigValues::midiPropertySliderColor },
  { "midiPatchReadoutColor", &MusEGlobal::GlobalConfigValues::midiPatchReadoutColor },
  { "knobFontColor", &MusEGlobal::GlobalConfigValues::knobFontColor },
  { "audioMeterPrimaryColor", &MusEGlobal::GlobalConfigValues::audioMeterPrimaryColor },
  { "midiMeterPrimaryColor", &MusEGlobal::GlobalConfigValues::midiMeterPrimaryColor },
  { "meterBackgroundColor", &MusEGlobal::GlobalConfigValues::meterBackgroundColor },
  { "rackItemBackgroundColor", &MusEGlobal::GlobalConfigValues::rackItemBackgroundColor },
  { "rackItemBgActiveColor", &MusEGlobal::GlobalConfigValues::rackItemBgActiveColor },
  { "rackItemFontColor", &MusEGlobal::GlobalConfigValues::rackItemFontColor },
  { "rackItemFontActiveColor", &MusEGlobal::GlobalConfigValues::rackItemFontActiveColor },
  { "rackItemBorderColor", &MusEGlobal::GlobalConfigValues::rackItemBorderColor },
  { "rackItemFontColorHover", &MusEGlobal::GlobalConfigValues::rackItemFontColorHover },
  { "midiInstrumentBackgroundColor", &MusEGlobal::GlobalConfigValues::midiInstrumentBackgroundColor },
  { "midiInstrumentBgActiveColor", &MusEGlobal::GlobalConfigValues::midiInstrumentBgActiveColor },
  { "midiInstrumentFontColor", &MusEGlobal::GlobalConfigValues::midiInstrumentFontColor },
  { "midiInstrumentFontActiveColor", &MusEGlobal::GlobalConfigValues::midiInstrumentFontActiveColor },
  { "midiInstrumentBorderColor", &MusEGlobal::GlobalConfigValues::midiInstrumentBorderColor },
  { "markerColor", &MusEGlobal::GlobalConfigValues::markerColor },
  { "rangeMarkerColor", &MusEGlobal::GlobalConfigValues::rangeMarkerColor },
  { "positionMarkerColor", &MusEGlobal::GlobalConfigValues::positionMarkerColor },
  { "currentPositionColor", &MusEGlobal::GlobalConfigValues::currentPositionColor },
  };
  return table;
}

} // anonymous namespace

//---------------------------------------------------------
//   loadMuseColors
//---------------------------------------------------------
//   colorPaletteToJsonDoc / saveColorPaletteToJsonPath
//---------------------------------------------------------

QJsonDocument MuseTheme::colorPaletteToJsonDoc(const QString& paletteName)
{
  QJsonObject root;

  QJsonObject meta;
  meta["name"] = paletteName.isEmpty() ? QStringLiteral("MusE Custom") : paletteName;
  meta["author"] = QStringLiteral("MusE user (saved from Appearance > Colors)");
  meta["version"] = QStringLiteral("1.0");
  root["meta"] = meta;

  for (const auto& entry : colorFieldTable())
  {
    const QColor& c = MusEGlobal::config.*(entry.second);
    root[entry.first] = c.isValid() ? c.name(QColor::HexRgb) : QString();
  }

  QJsonArray partColorsArr;
  for (int i = 0; i < NUM_PARTCOLORS; ++i)
  {
    const QColor& c = MusEGlobal::config.partColors[i];
    partColorsArr.append(c.isValid() ? c.name(QColor::HexRgb) : QString());
  }
  root["partColors"] = partColorsArr;

  return QJsonDocument(root);
}

bool MuseTheme::saveColorPaletteToJsonPath(const QString& jsonPath, const QString& paletteName)
{
  QFile f(jsonPath);
  if (!f.open(QIODevice::WriteOnly))
  {
    fprintf(stderr, "MuseTheme::saveColorPaletteToJsonPath: could not open <%s> for writing: %s\n",
            qPrintable(jsonPath), qPrintable(f.errorString()));
    return false;
  }

  const QJsonDocument doc = colorPaletteToJsonDoc(paletteName);
  const qint64 written = f.write(doc.toJson(QJsonDocument::Indented));
  f.close();

  if (written < 0)
  {
    fprintf(stderr, "MuseTheme::saveColorPaletteToJsonPath: write to <%s> failed: %s\n",
            qPrintable(jsonPath), qPrintable(f.errorString()));
    return false;
  }

  if (MusEGlobal::debugMsg)
    fprintf(stderr, "MuseTheme::saveColorPaletteToJsonPath: wrote <%s>\n", qPrintable(jsonPath));
  return true;
}

//---------------------------------------------------------

void MuseTheme::loadMuseColors(const QJsonObject& museColorsObj)
{
  for (const auto& entry : colorFieldTable())
  {
    const QString& key = entry.first;
    if (!museColorsObj.contains(key))
      continue;

    const QColor c(museColorsObj.value(key).toString());
    if (!c.isValid())
    {
      fprintf(stderr, "MuseTheme::loadMuseColors: invalid color for \"%s\": <%s>\n",
              qPrintable(key), qPrintable(museColorsObj.value(key).toString()));
      continue;
    }

    MusEGlobal::config.*(entry.second) = c;
  }

  if (museColorsObj.contains("partColors"))
  {
    const QJsonArray arr = museColorsObj.value("partColors").toArray();
    const int n = qMin(arr.size(), NUM_PARTCOLORS);
    for (int i = 0; i < n; ++i)
    {
      const QColor c(arr.at(i).toString());
      if (c.isValid())
        MusEGlobal::config.partColors[i] = c;
      else
        fprintf(stderr, "MuseTheme::loadMuseColors: invalid partColors[%d]: <%s>\n",
                i, qPrintable(arr.at(i).toString()));
    }
  }
}

//---------------------------------------------------------
//   loadColorPaletteFromJsonDoc / loadColorPaletteFromJsonPath
//---------------------------------------------------------

bool MuseTheme::loadColorPaletteFromJsonDoc(const QJsonDocument& jsonDoc)
{
  if (!jsonDoc.isObject())
    return false;

  // Whole document is the color palette (no "museColors" wrapper needed -
  //  there's no Qlementine chrome fields in these files to collide with).
  loadMuseColors(jsonDoc.object());
  return true;
}

bool MuseTheme::loadColorPaletteFromJsonPath(const QString& jsonPath)
{
  QFile f(jsonPath);
  if (!f.open(QIODevice::ReadOnly))
  {
    fprintf(stderr, "MuseTheme::loadColorPaletteFromJsonPath: could not open <%s>\n", qPrintable(jsonPath));
    return false;
  }

  QJsonParseError err;
  const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
  f.close();

  if (doc.isNull())
  {
    fprintf(stderr, "MuseTheme::loadColorPaletteFromJsonPath: JSON parse error in <%s>: %s\n",
            qPrintable(jsonPath), qPrintable(err.errorString()));
    return false;
  }

  return loadColorPaletteFromJsonDoc(doc);
}

//---------------------------------------------------------
//   fromJsonDoc
//---------------------------------------------------------

std::optional<MuseTheme> MuseTheme::fromJsonDoc(const QJsonDocument& jsonDoc)
{
  // Parse the Qlementine base fields (root-level keys) first, exactly as
  //  oclero::qlementine::Theme does for any other theme.
  auto baseTheme = oclero::qlementine::Theme::fromJsonDoc(jsonDoc);
  if (!baseTheme)
    return std::nullopt;

  MuseTheme result;
  // Slice-copy the base (Qlementine) part. Theme's operator= is defaulted
  //  and public, so this copies all of Theme's fields in one go.
  static_cast<oclero::qlementine::Theme&>(result) = *baseTheme;

  if (jsonDoc.isObject())
  {
    const QJsonObject root = jsonDoc.object();
    const auto it = root.constFind("museColors");
    if (it != root.constEnd() && it->isObject())
      loadMuseColors(it->toObject());
  }

  return result;
}

//---------------------------------------------------------
//   fromJsonPath
//---------------------------------------------------------

std::optional<MuseTheme> MuseTheme::fromJsonPath(const QString& jsonPath)
{
  QFile f(jsonPath);
  if (!f.open(QIODevice::ReadOnly))
  {
    fprintf(stderr, "MuseTheme::fromJsonPath: could not open <%s>\n", qPrintable(jsonPath));
    return std::nullopt;
  }

  QJsonParseError err;
  const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
  f.close();

  if (doc.isNull())
  {
    fprintf(stderr, "MuseTheme::fromJsonPath: JSON parse error in <%s>: %s\n",
            qPrintable(jsonPath), qPrintable(err.errorString()));
    return std::nullopt;
  }

  return fromJsonDoc(doc);
}

} // namespace MusEGui
