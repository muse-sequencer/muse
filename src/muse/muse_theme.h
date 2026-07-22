//=========================================================
//  MusE
//  Linux Music Editor
//
//  muse_theme.h
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

#ifndef __MUSE_THEME_H__
#define __MUSE_THEME_H__

#include <optional>

#include <oclero/qlementine/style/Theme.hpp>

#include <QString>
#include <QJsonDocument>

namespace MusEGui {

//---------------------------------------------------------
//   MuseTheme
//    Extends oclero::qlementine::Theme (the ~120-field palette Qlementine
//    uses to style *standard* Qt widgets) with loading of MusE's own
//    custom-widget colors (the ones in MusEGlobal::config / gconfig.h -
//    canvases, knobs, meters, track labels, etc. - that Qlementine's
//    QStyle has no knowledge of, since those widgets paint themselves).
//
//    Both color sets live in the SAME JSON file: Qlementine's fields at
//    the document root (parsed by the base class as usual, via
//    Theme::fromJsonDoc()), and MusE's own fields nested under a
//    "museColors" object, parsed by loadMuseColors() below directly into
//    MusEGlobal::config - the same struct every custom-painted widget
//    already reads from. This does NOT introduce a second, parallel color
//    store: it's a new *file format* (JSON) for the data that used to
//    live in a theme's .cfc file, not a new runtime *model*.
//---------------------------------------------------------

class MuseTheme : public oclero::qlementine::Theme {
public:
  using oclero::qlementine::Theme::Theme;

  // Parses both the Qlementine base fields AND the "museColors" object
  //  (loading the latter straight into MusEGlobal::config as a side
  //  effect). Returns std::nullopt if the file doesn't exist or isn't
  //  valid JSON/doesn't parse as a Theme.
  // NOTE: for a combined one-file theme only. Prefer loadColorPalette*()
  //  below for the split theme/palette setup (themes/ vs themes/muse_custom/).
  static std::optional<MuseTheme> fromJsonPath(const QString& jsonPath);
  static std::optional<MuseTheme> fromJsonDoc(const QJsonDocument& jsonDoc);

  // Loads a standalone MusE color-palette JSON file directly into
  //  MusEGlobal::config - e.g. themes/muse_custom/<name>.json. Unlike
  //  fromJsonPath() above, these files have MusE's color fields directly
  //  at the document root (no "museColors" wrapper, no Qlementine chrome
  //  fields) - they're a palette, not a Theme. Returns false if the file
  //  doesn't exist or isn't valid JSON.
  static bool loadColorPaletteFromJsonPath(const QString& jsonPath);
  static bool loadColorPaletteFromJsonDoc(const QJsonDocument& jsonDoc);

private:
  // Reads museColorsObj's keys into MusEGlobal::config's QColor fields
  //  (see gconfig.h) by name, plus the "partColors" array. Keys not
  //  present in museColorsObj leave the corresponding MusEGlobal::config
  //  field untouched (whatever it already was - typically the compiled-in
  //  default, unless a previous theme load already changed it).
  static void loadMuseColors(const QJsonObject& museColorsObj);
};

} // namespace MusEGui

#endif // __MUSE_THEME_H__
