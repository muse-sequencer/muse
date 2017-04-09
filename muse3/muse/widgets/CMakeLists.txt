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
QT5_WRAP_CPP (widget_mocs
      aboutbox_impl.h 
      arrangercolumns.h 
      action.h
      audio_converter_settings.h
      bigtime.h
      canvas.h  
      checkbox.h
      choose_sysex.h
      clipper_label.h
      colorframe.h
      comboQuant.h  
      combobox.h  
      comment.h  
      compact_knob.h
      compact_patch_edit.h
      compact_slider.h  
      copy_on_write.h
      cpu_toolbar.h
      ctrlcombo.h  
      custom_widget_actions.h
      dentry.h  
      didyouknow.h
      doublelabel.h  
      doublespinbox.h  
      editevent.h      
      elided_label.h
      filedialog.h  
      genset.h  
      mdisettings.h  
      header.h  
      hitscale.h  
      intlabel.h  
      knob.h  
      knob_and_meter.h
      lcd_widgets.h
      lcombo.h  
      line_edit.h
      menutitleitem.h
      meter.h
      meter_slider.h
      metronome.h  
      midi_audio_control.h  
      midisyncimpl.h  
      midi_warn_init_pending_impl.h
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
      plugindialog.h
      popup_double_spinbox.h
      popupmenu.h  
      # posedit.h  
      poslabel.h  
      projectcreateimpl.h
      routepopup.h  
      #rubberband_settings.h
      scroll_area.h
      scrollbar.h
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
      #src_resampler_settings.h
      swidget.h  
      tb1.h  
      tempolabel.h
      text_edit.h
      tools.h
      trackinfo_layout.h
      tracks_duplicate.h
      # ttoolbar.h  
      ttoolbutton.h  
      unusedwavefiles.h
      view.h
      vscale.h
      visibletracks.h
      vst_native_editor.h
      warn_bad_timing.h
      widget_stack.h
      #zita_resampler_settings.h
      )

##
## UI files
##
file (GLOB widgets_ui_files
      aboutbox.ui  
      arrangercolumnsbase.ui 
      appearancebase.ui 
      audio_converter_settings_base.ui
      choose_sysex_base.ui
      cliplisteditorbase.ui  
      commentbase.ui  
      configmidifilebase.ui
      copy_on_write_base.ui
      didyouknow.ui  
      editctrlbase.ui  
      editnotedialogbase.ui  
      editsysexdialogbase.ui  
      fdialogbuttons.ui  
      gensetbase.ui  
      mdisettings_base.ui  
      itransformbase.ui  
      metronomebase.ui  
      midisync.ui  
      midi_warn_init_pending.ui
      midi_audio_control_base.ui  
      mittransposebase.ui  
      mixdowndialogbase.ui  
      mtrackinfobase.ui
      pastedialogbase.ui
      pasteeventsdialogbase.ui
      plugindialogbase.ui
      projectcreate.ui
      #rubberband_settings_base.ui
      shortcutcapturedialogbase.ui  
      shortcutconfigbase.ui  
      songinfo.ui  
      #src_resampler_settings_base.ui
      synthconfigbase.ui  
      tracks_duplicate_base.ui
      transformbase.ui  
      unusedwavefiles.ui
      warn_bad_timing.ui
      #zita_resampler_settings_base.ui
      )
QT5_WRAP_UI (widget_ui_headers ${widgets_ui_files})

##
## List of source files to compile
##
file (GLOB widgets_source_files
      aboutbox_impl.cpp 
      arrangercolumns.cpp 
      audio_converter_settings.cpp
      background_painter.cpp
      bigtime.cpp
      canvas.cpp 
      checkbox.cpp 
      choose_sysex.cpp
      citem.cpp
      clipper_label.cpp
      colorframe.cpp
      comboQuant.cpp 
      combobox.cpp  
      comment.cpp 
      compact_knob.cpp
      compact_patch_edit.cpp
      compact_slider.cpp 
      copy_on_write.cpp
      cpu_toolbar.cpp
      ctrlcombo.cpp 
      custom_widget_actions.cpp
      dentry.cpp 
      dimap.cpp
      doublelabel.cpp 
      doublespinbox.cpp
      drange.cpp
      editevent.cpp
      elided_label.cpp
      filedialog.cpp 
      genset.cpp 
      mdisettings.cpp 
      header.cpp 
      hitscale.cpp 
      intlabel.cpp 
      knob.cpp 
      knob_and_meter.cpp
      lcd_widgets.cpp
      lcombo.cpp 
      line_edit.cpp
      menutitleitem.cpp
      meter.cpp
      meter_slider.cpp
      metronome.cpp 
      midi_audio_control.cpp 
      midisyncimpl.cpp 
      midi_warn_init_pending_impl.cpp
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
      plugindialog.cpp
      popup_double_spinbox.cpp
      popupmenu.cpp 
      # posedit.cpp 
      poslabel.cpp
      projectcreateimpl.cpp
      routepopup.cpp 
      #rubberband_settings.cpp
      scldiv.cpp
      scldraw.cpp
      sclif.cpp
      scroll_area.cpp
      scrollbar.cpp
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
      #src_resampler_settings.cpp
      swidget.cpp 
      tb1.cpp   
      tempolabel.cpp 
      text_edit.cpp
      tools.cpp
      trackinfo_layout.cpp
      tracks_duplicate.cpp
      # ttoolbar.cpp 
      ttoolbutton.cpp 
      unusedwavefiles.cpp
      utils.cpp
      velocity.cpp 
      view.cpp
      vscale.cpp
      visibletracks.cpp
      vst_native_editor.cpp
      warn_bad_timing.cpp
      widget_stack.cpp
      #zita_resampler_settings.cpp
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
      PROPERTIES COMPILE_FLAGS "${VST_SDK_QUIRK} -include ${PROJECT_BINARY_DIR}/all.h ${MUSECXXFLAGS} -I../ -I${PROJECT_SOURCE_DIR}/synti"
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
