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

##
## Expand Qt macros in source files
##
QT4_WRAP_CPP (widget_mocs
      aboutbox_impl.h 
      action.h
      bigtime.h  
      canvas.h  
      checkbox.h  
      comboQuant.h  
      combobox.h  
      comment.h  
      ctrlcombo.h  
      dentry.h  
      didyouknow.h
      doublelabel.h  
      filedialog.h  
      gatetime.h  
      genset.h  
      header.h  
      hitscale.h  
      intlabel.h  
      knob.h  
      lcombo.h  
      meter.h
      metronome.h  
      midisyncimpl.h  
      mixdowndialog.h  
      mlabel.h  
      mtscale.h  
      mtrackinfo.h
      nentry.h  
      noteinfo.h  
      pitchedit.h  
      pitchlabel.h  
      popupmenu.h  
      # posedit.h  
      poslabel.h  
      projectcreateimpl.h
      scrollscale.h  
      shortcutcapturedialog.h  
      shortcutconfig.h  
      # sigedit.h  
      siglabel.h  
      sigscale.h  
      slider.h  
      sliderbase.h  
      songinfo.h
      spinbox.h  
      spinboxFP.h  
      splitter.h  
      swidget.h  
      tb1.h  
      tempolabel.h  
      tools.h  
      # ttoolbar.h  
      ttoolbutton.h  
      unusedwavefiles.h
      velocity.h
      quantize.h  
      move.h  
      remove.h  
      deloverlaps.h  
      setlen.h  
      transpose.h  
      verticalmeter.h
      view.h
      vscale.h
      visibletracks.h
      )

##
## UI files
##
file (GLOB widgets_ui_files
      aboutbox.ui  
      appearancebase.ui 
      cliplisteditorbase.ui  
      commentbase.ui  
      configmidifilebase.ui
      didyouknow.ui  
      editnotedialogbase.ui  
      editsysexdialogbase.ui  
      fdialogbuttons.ui  
      gatetimebase.ui  
      gensetbase.ui  
      itransformbase.ui  
      metronomebase.ui  
      midisync.ui  
      mittransposebase.ui  
      mixdowndialogbase.ui  
      mtrackinfobase.ui
      projectcreate.ui
      shortcutcapturedialogbase.ui  
      shortcutconfigbase.ui  
      songinfo.ui  
      synthconfigbase.ui  
      transformbase.ui  
      transposebase.ui
      unusedwavefiles.ui
      velocitybase.ui
      quantbase.ui
      movebase.ui
      removebase.ui
      deloverlapsbase.ui
      transposebase.ui
      setlenbase.ui
      )
QT4_WRAP_UI (widget_ui_headers ${widgets_ui_files})

##
## List of source files to compile
##
file (GLOB widgets_source_files
      aboutbox_impl.cpp 
      bigtime.cpp 
      canvas.cpp 
      checkbox.cpp 
      citem.cpp
      comboQuant.cpp 
      combobox.cpp  
      comment.cpp 
      ctrlcombo.cpp 
      dentry.cpp 
      dimap.cpp
      doublelabel.cpp 
      drange.cpp
      filedialog.cpp 
      gatetime.cpp 
      genset.cpp 
      header.cpp 
      hitscale.cpp 
      intlabel.cpp 
      knob.cpp 
      lcombo.cpp 
      meter.cpp
      metronome.cpp 
      midisyncimpl.cpp 
      mixdowndialog.cpp 
      mlabel.cpp 
      mmath.cpp
      mtrackinfo.cpp
      mtscale.cpp 
      nentry.cpp 
      noteinfo.cpp 
      pitchedit.cpp 
      pitchlabel.cpp 
      popupmenu.cpp 
      # posedit.cpp 
      poslabel.cpp
      projectcreateimpl.cpp 
      scldiv.cpp
      scldraw.cpp
      sclif.cpp
      scrollscale.cpp  
      shortcutcapturedialog.cpp 
      shortcutconfig.cpp 
      # sigedit.cpp 
      siglabel.cpp 
      sigscale.cpp 
      slider.cpp 
      sliderbase.cpp 
      spinbox.cpp 
      spinboxFP.cpp 
      splitter.cpp  
      swidget.cpp 
      tb1.cpp   
      tempolabel.cpp 
      tools.cpp 
      ttoolbar.cpp 
      ttoolbutton.cpp 
      unusedwavefiles.cpp
      utils.cpp
      velocity.cpp 
      quantize.cpp 
      move.cpp 
      remove.cpp 
      deloverlaps.cpp 
      setlen.cpp 
      transpose.cpp 
      verticalmeter.cpp
      view.cpp
      vscale.cpp
      visibletracks.cpp
      )

##
## Define target
##
add_library ( widgets ${MODULES_BUILD} 
      ${widget_ui_headers}
      ${widget_mocs}
      ${widgets_source_files}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${widgets_source_files}
      ${widgets_ui_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties( widgets
      # PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h ${MUSECXXFLAGS} -I../ -I${PROJECT_SOURCE_DIR}/synti "
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h ${MUSECXXFLAGS} -I../ -I${PROJECT_SOURCE_DIR}/synti"
      OUTPUT_NAME muse_widgets
      )

##
## Linkage
##
target_link_libraries ( widgets
      ${QT_LIBRARIES}
      icons
      )

##
## Install location
##
if ( ${MODULES_BUILD} STREQUAL SHARED )
      install(TARGETS widgets
            DESTINATION ${MusE_MODULES_DIR}
            )
endif ( ${MODULES_BUILD} STREQUAL SHARED )
