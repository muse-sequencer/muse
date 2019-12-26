//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2003-2020 Werner Schweer (ws@seh.de) and others
//
//  wave_helper.h
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

#ifndef __WAVE_HELPER_H__
#define __WAVE_HELPER_H__

#include "event.h"
#include "wave.h"
#include "time_stretch.h"
#include "audio_convert/audio_converter_settings_group.h"

namespace MusECore {

bool sndFileCheckCopyOnWrite(const SndFileR sndFile);
void sndFileApplyUndoFile(const Event& original, const QString* tmpfile, unsigned startframe, unsigned endframe);
// If audioConverterSettings and stretchList are given, they are assigned.
SndFileR sndFileGetWave(const QString& name, bool readOnlyFlag, bool openFlag = true, bool showErrorBox = true, 
                 const AudioConverterSettingsGroup* audioConverterSettings = NULL, const StretchList* stretchList = NULL);

}

#endif
