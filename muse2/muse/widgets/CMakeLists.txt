#=============================================================================
#  MusE
#  Linux Music Editor
#  $Id:$
#
#  Copyright (C) 1999-2011 by Werner Schweer and others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the
#  Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#=============================================================================

set (SubDirs 
      function_dialogs 
      )

subdirs (${SubDirs})

##
## Expand Qt macros in source files
##
QT4_WRAP_CPP (widget_mocs
      aboutbox_impl.h 
      arrangercolumns.h 
      action.h
      bigtime.h  
      canvas.h  
      checkbox.h  
      comboQuant.h  
      combobox.h  
      comment.h  
      ctrlcombo.h  
      custom_widget_actions.h
      dentry.h  
      didyouknow.h
      doublelabel.h  
      doublespinbox.h  
      filedialog.h  
      genset.h  
      mdisettings.h  
      header.h  
      hitscale.h  
      intlabel.h  
      knob.h  
      lcombo.h  
      menutitleitem.h
      meter.h
      metronome.h  
      midi_audio_control.h  
      midisyncimpl.h  
      mixdowndialog.h  
      mlabel.h  
      mtscale.h  
      mtscale_flo.h  
      mtrackinfo.h
      nentry.h  
      noteinfo.h  
      pastedialog.h
      pasteeventsdialog.h
      pitchedit.h  
      pitchlabel.h  
      pixmap_button.h
      popupmenu.h  
      # posedit.h  
      poslabel.h  
      projectcreateimpl.h
      routepopup.h  
      scrollscale.h  
      shortcutcapturedialog.h  
      shortcutconfig.h  
      # sigedit.h  
      siglabel.h  
      sigscale.h  
      sig_tempo_toolbar.h 
      slider.h  
      sliderbase.h  
      songinfo.h
      songpos_toolbar.h
      spinbox.h  
      spinboxFP.h  
      splitter.h  
      swidget.h  
      tb1.h  
      tempolabel.h  
      tools.h  
      tracks_duplicate.h
      # ttoolbar.h  
      ttoolbutton.h  
      unusedwavefiles.h
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
      arrangercolumnsbase.ui 
      appearancebase.ui 
      cliplisteditorbase.ui  
      commentbase.ui  
      configmidifilebase.ui
      didyouknow.ui  
      editnotedialogbase.ui  
      editsysexdialogbase.ui  
      fdialogbuttons.ui  
      gensetbase.ui  
      mdisettings_base.ui  
      itransformbase.ui  
      metronomebase.ui  
      midisync.ui  
      midi_audio_control_base.ui  
      mittransposebase.ui  
      mixdowndialogbase.ui  
      mtrackinfobase.ui
      pastedialogbase.ui
      pasteeventsdialogbase.ui
      projectcreate.ui
      shortcutcapturedialogbase.ui  
      shortcutconfigbase.ui  
      songinfo.ui  
      synthconfigbase.ui  
      tracks_duplicate_base.ui
      transformbase.ui  
      unusedwavefiles.ui
      )
QT4_WRAP_UI (widget_ui_headers ${widgets_ui_files})

##
## List of source files to compile
##
file (GLOB widgets_source_files
      aboutbox_impl.cpp 
      arrangercolumns.cpp 
      bigtime.cpp 
      canvas.cpp 
      checkbox.cpp 
      citem.cpp
      comboQuant.cpp 
      combobox.cpp  
      comment.cpp 
      ctrlcombo.cpp 
      custom_widget_actions.cpp
      dentry.cpp 
      dimap.cpp
      doublelabel.cpp 
      doublespinbox.cpp
      drange.cpp
      filedialog.cpp 
      genset.cpp 
      mdisettings.cpp 
      header.cpp 
      hitscale.cpp 
      intlabel.cpp 
      knob.cpp 
      lcombo.cpp 
      menutitleitem.cpp
      meter.cpp
      metronome.cpp 
      midi_audio_control.cpp 
      midisyncimpl.cpp 
      mixdowndialog.cpp 
      mlabel.cpp 
      mmath.cpp
      mtrackinfo.cpp
      mtscale.cpp 
      mtscale_flo.cpp 
      nentry.cpp 
      noteinfo.cpp 
      pastedialog.cpp
      pasteeventsdialog.cpp
      pitchedit.cpp 
      pitchlabel.cpp 
      pixmap_button.cpp
      popupmenu.cpp 
      # posedit.cpp 
      poslabel.cpp
      projectcreateimpl.cpp 
      routepopup.cpp 
      scldiv.cpp
      scldraw.cpp
      sclif.cpp
      scrollscale.cpp  
      shortcutcapturedialog.cpp 
      shortcutconfig.cpp 
      # sigedit.cpp 
      siglabel.cpp 
      sigscale.cpp 
      sig_tempo_toolbar.cpp 
      slider.cpp 
      sliderbase.cpp 
      songpos_toolbar.cpp 
      spinbox.cpp 
      spinboxFP.cpp 
      splitter.cpp  
      swidget.cpp 
      tb1.cpp   
      tempolabel.cpp 
      tools.cpp 
      tracks_duplicate.cpp
      # ttoolbar.cpp 
      ttoolbutton.cpp 
      unusedwavefiles.cpp
      utils.cpp
      velocity.cpp 
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
