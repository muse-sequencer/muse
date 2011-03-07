#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 2006-2008 by Werner Schweer and others
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


FLTK_WRAP_UI (
      FLTKUI
      ADnoteUI.fl
      BankUI.fl
      ConfigUI.fl
      EffUI.fl
      EnvelopeUI.fl
      FilterUI.fl
      LFOUI.fl
      MasterUI.fl
      MicrotonalUI.fl
      OscilGenUI.fl
      PADnoteUI.fl
      PartUI.fl
      PresetsUI.fl
      ResonanceUI.fl
      SUBnoteUI.fl
      SeqUI.fl
      VirKeyboard.fl
      WidgetPDial.fl
      )

add_library (
      zyngui STATIC
      ${FLTKUI_FLTK_UI_SRCS}
      )

set_target_properties ( zyngui
      PROPERTIES
      COMPILE_FLAGS "-O3 -fPIC -fvisibility=hidden -DASM_F2I_YES -DOS_LINUX -DFFTW_VERSION_3"
      )


