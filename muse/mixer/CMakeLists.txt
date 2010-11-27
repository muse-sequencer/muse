#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 2002-2006 by Werner Schweer and others
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

QT4_WRAP_CPP ( mixer_mocs amixer.h strip.h astrip.h mstrip.h meter.h rack.h panknob.h auxknob.h routedialog.h )
QT4_WRAP_UI ( mixer_uis routedialogbase.ui )

add_library ( mixer STATIC 
      ${mixer_mocs}
      ${mixer_uis}
      amixer.cpp  
      meter.cpp 
      rack.cpp 
      panknob.cpp 
      auxknob.cpp 
      routedialog.cpp 
      strip.cpp astrip.cpp mstrip.cpp
      )

set_target_properties ( mixer
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
   )

