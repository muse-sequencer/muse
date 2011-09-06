//=========================================================
//  MusE
//  Linux Music Editor
//    software synthesizer helper library
//    $Id: mono.cpp,v 1.2 2004/04/15 13:46:18 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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

#include "mono.h"

//---------------------------------------------------------
//   playNote
//---------------------------------------------------------

bool MessMono::playNote(int channel, int pitch, int velo)
      {
      if (velo == 0) {
            if (pitchStack.empty())
                  return false;
            if (pitchStack.back().pitch == pitch) {
                  pitchStack.pop_back();
                  if (pitchStack.empty()) {
                        note(channel, pitch, 0);
                        return false;
                        }
                  PitchVelo pv = pitchStack.back();
                  note(pv.channel, pv.pitch, pv.velo);  // change pitch
                  return false;
                  }
            for (std::list<PitchVelo>::iterator i = pitchStack.begin();
               i != pitchStack.end(); ++i) {
                  if ((*i).pitch == pitch) {
                        pitchStack.erase(i);
                        return false;
                        }
                  }
            // no noteon found
            // emergency stop:
            note(channel, pitch, velo);
            return false;
            }
      pitchStack.push_back(PitchVelo(channel, pitch, velo));
      note(channel, pitch, velo);
      return false;
      }

