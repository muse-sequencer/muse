#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 2006 by Werner Schweer and others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#=============================================================================

SET(FLTK_FLUID_EXECUTABLE "/usr/bin/fluid")

include_directories(
      BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/UI
      BEFORE ${CMAKE_CURRENT_BINARY_DIR}
      )

FLTK_WRAP_UI ( fltk_ui
      UI/ADnoteUI.fl
      UI/BankUI.fl
      UI/ConfigUI.fl
      UI/EffUI.fl
      UI/EnvelopeUI.fl
      UI/FilterUI.fl
      UI/LFOUI.fl
      UI/MasterUI.fl
      UI/MicrotonalUI.fl
      UI/OscilGenUI.fl
      UI/PADnoteUI.fl
      UI/PartUI.fl
      UI/PresetsUI.fl
      UI/ResonanceUI.fl
      UI/SUBnoteUI.fl
      UI/SeqUI.fl
      UI/VirKeyboard.fl
      UI/WidgetPDial.fl
      )

# ${fltk_ui_FLTK_UI_SRCS}
#      Output/JACKaudiooutput.C
#      Input/MidiIn.C
#      Input/ALSAMidiIn.C
#      Input/NULLMidiIn.C

add_library ( zynaddsubfx SHARED
      ${fltk_ui_FLTK_UI_SRCS}
      DSP/AnalogFilter.C
      DSP/FFTwrapper.C
      DSP/Filter.C
      DSP/FormantFilter.C
      DSP/SVFilter.C 
      Effects/Alienwah.C
      Effects/Chorus.C
      Effects/Distorsion.C
      Effects/DynamicFilter.C
      Effects/EQ.C
      Effects/Echo.C
      Effects/Effect.C
      Effects/EffectLFO.C
      Effects/EffectMgr.C
      Effects/Phaser.C
      Effects/Reverb.C
      Output/Recorder.C
      Misc/Bank.C
      Misc/Config.C
      Misc/Dump.C
      Misc/Master.C
      Misc/Microtonal.C
      Misc/Part.C
      Misc/Util.C
      Misc/XMLwrapper.C
      Seq/MIDIEvents.C
      Seq/MIDIFile.C
      Seq/Sequencer.C
      Params/ADnoteParameters.C
      Params/Controller.C
      Params/EnvelopeParams.C
      Params/FilterParams.C
      Params/LFOParams.C
      Params/PADnoteParameters.C
      Params/Presets.C
      Params/PresetsStore.C
      Params/SUBnoteParameters.C
      Synth/ADnote.C
      Synth/Envelope.C
      Synth/LFO.C
      Synth/OscilGen.C
      Synth/PADnote.C
      Synth/Resonance.C
      Synth/SUBnote.C
      main.cpp
      )

# - tell cmake to name target zynaddsubfx.so instead of 
#   libzynaddsubfx.so
#

set_target_properties ( zynaddsubfx
   PROPERTIES PREFIX ""
   COMPILE_FLAGS "-O6 -fvisibility=hidden -DASM_F2I_YES -DOS_LINUX -DFFTW_VERSION_3"
   )

target_link_libraries( zynaddsubfx
      synti jack asound pthread fftw3 mxml
      m z fltk
      )

install_targets ( /${CMAKE_INSTALL_LIBDIR}/${MusE_INSTALL_NAME}/synthi/ zynaddsubfx )

## needs cmake version > 2.4.3
## INSTALL(
##      DIRECTORY banks
##      DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${MusE_INSTALL_NAME}/presets/zynaddsubfx
##      )


