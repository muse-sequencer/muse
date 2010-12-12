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

include(${PROJECT_SOURCE_DIR}/pch.txt)

# FIXME: Can't build shared widgets library while using QT3 support for the widgets code. 
# QT3's QWidgetPlugin no longer supported in QT4. Our code would have to be native QT4...
# Rules seem OK but get compile errors.

# IF (APPLE)
#    message("Not yet able to build musewidgetsplugin under OS X.")
# ELSE (APPLE)
# 
#    #
#    #  build libmusewidgetsplugin.so
#    #
#    QT4_WRAP_CPP (moc_widgetsplugin_headers
#       posedit.h poslabel.h   sigedit.h     pitchedit.h pitchlabel.h tempolabel.h 
#       slider.h  sliderbase.h doublelabel.h dentry.h    checkbox.h   combobox.h
#       )
# 
#    add_library ( musewidgetsplugin SHARED
#        sclif.cpp  scldiv.cpp mmath.cpp scldraw.cpp  dimap.cpp drange.cpp 
#        
#        posedit.cpp poslabel.cpp sigedit.cpp pitchedit.cpp pitchlabel.cpp tempolabel.cpp 
#        slider.cpp sliderbase.cpp doublelabel.cpp dentry.cpp checkbox.cpp combobox.cpp
#       
#        musewidgetsplug.cpp 
#        sigedit.h 
#        ${PROJECT_SOURCE_DIR}/muse/xml.cpp ${PROJECT_SOURCE_DIR}/muse/tempo.cpp 
#        ${PROJECT_SOURCE_DIR}/muse/sig.cpp ${PROJECT_SOURCE_DIR}/muse/pos.cpp
#        
#        ${moc_widgetsplugin_headers}
#        ${PROJECT_BINARY_DIR}/all-pic.h.pch
#        )
# 
#    set_target_properties( musewidgetsplugin
#       PROPERTIES COMPILE_FLAGS
#          "-D_GNU_SOURCE -D_REENTRANT -DHAVE_CONFIG_H -DQT_PLUGIN -DQT_SHARED -DQT_NO_DEBUG -include ${PROJECT_BINARY_DIR}/all-pic.h"
#       )
#    target_link_libraries( musewidgetsplugin
#       # TODO: what do we need here?
#       al pthread asound
#       )
# 
#    # install( TARGETS awlplugin DESTINATION ${QT_PLUGINS_DIR}/designer/ )
#    # install_targets ( /${CMAKE_INSTALL_LIBDIR}/${MusE_INSTALL_NAME}/plugins/ musewidgetsplugin )
#    install( TARGETS musewidgetsplugin DESTINATION ${CMAKE_INSTALL_LIBDIR}/${MusE_INSTALL_NAME}/plugins/ )
#    
# # 
# #    add_executable( ltest ltest.cpp )
# #    target_link_libraries ( ltest
# #       musewidgetsplugin
# #       ${QT_LIBRARIES}
# #       )
# # 
#
# ENDIF (APPLE)
 

QT4_WRAP_CPP (widget_mocs
      scrollscale.h  
      view.h  
      nentry.h  
      mtscale.h  
      sigscale.h  
      hitscale.h  
      intlabel.h  
      doublelabel.h  
      canvas.h  
      noteinfo.h  
      dentry.h  
      siglabel.h  
      # ttoolbar.h  
      ttoolbutton.h  
      tb1.h  
      lcombo.h  
      swidget.h  
      tempolabel.h  
      tools.h  
      popupmenu.h  
      ctrlcombo.h  
      sliderbase.h  
      slider.h  
      midisyncimpl.h  
      header.h  
      knob.h  
      mlabel.h  
      genset.h  
      mixdowndialog.h  
      spinboxFP.h  
      spinbox.h  
      comboQuant.h  
      bigtime.h  
      comment.h  
      splitter.h  
      gatetime.h  
      velocity.h  
      metronome.h  
      # posedit.h  
      pitchedit.h  
      pitchlabel.h  
      poslabel.h  
      filedialog.h  
      # sigedit.h  
      shortcutconfig.h  
      shortcutcapturedialog.h  
      combobox.h  
      aboutbox_impl.h 
      checkbox.h  
      vscale.h  
      action.h
      songinfo.h
      didyouknow.h
      )

file (GLOB UI_FILES
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
      shortcutcapturedialogbase.ui  
      shortcutconfigbase.ui  
      songinfo.ui  
      synthconfigbase.ui  
      transformbase.ui  
      transposebase.ui  
      velocitybase.ui  
      )

QT4_WRAP_UI (widget_ui_headers ${UI_FILES})      

# add_custom_command(OUTPUT ui_midisync.h
#      COMMAND ${QT_UIC_EXECUTABLE}
#      ARGS -o ${CMAKE_CURRENT_BINARY_DIR}/ui_midisync.h ${CMAKE_CURRENT_SOURCE_DIR}/midisync.ui
#      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/midisync.ui
#      )

# ADD_FILE_DEPENDENCIES(moc_midisync.cxx ${CMAKE_CURRENT_BINARY_DIR}/ui_midisync.h)

file (GLOB SOURCE_FILES
      utils.cpp 
      citem.cpp
      vscale.cpp
      drange.cpp
      sclif.cpp
      dimap.cpp dimap.h  
      scldiv.cpp scldiv.h  
      mmath.cpp mmath.h  
      scldraw.cpp scldraw.h  
      action.h  
      scrollscale.cpp  
      view.cpp    
      nentry.cpp 
      mtscale.cpp 
      sigscale.cpp 
      hitscale.cpp 
      intlabel.cpp 
      doublelabel.cpp 
      canvas.cpp 
      noteinfo.cpp 
      dentry.cpp 
      siglabel.cpp 
      ttoolbar.cpp 
      ttoolbutton.cpp 
      tb1.cpp   
      lcombo.cpp 
      swidget.cpp 
      tempolabel.cpp 
      tools.cpp 
      ctrlcombo.cpp 
      sliderbase.cpp 
      slider.cpp 
      midisyncimpl.cpp 
      header.cpp 
      knob.cpp 
      mlabel.cpp 
      genset.cpp 
      mixdowndialog.cpp 
      spinboxFP.cpp 
      spinbox.cpp 
      comboQuant.cpp 
      bigtime.cpp 
      comment.cpp 
      splitter.cpp  
      gatetime.cpp 
      velocity.cpp 
      metronome.cpp 
      # posedit.cpp 
      section.h 
      pitchedit.cpp 
      pitchlabel.cpp 
      poslabel.cpp 
      filedialog.cpp 
      # sigedit.cpp 
      shortcutconfig.cpp 
      shortcutcapturedialog.cpp 
      combobox.cpp  
      checkbox.cpp 
      aboutbox_impl.cpp 
      popupmenu.cpp 
      menutitleitem.h  
      )

add_library ( widgets STATIC 
      ${widget_ui_headers}
      ${widget_mocs}
      ${SOURCE_FILES}
      )

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${SOURCE_FILES} ${UI_FILES} CACHE INTERNAL "")

#add_dependencies(widgets translations)

# AM_CXXFLAGS +=-DSVNVERSION=\"$(shell svnversion)\"
# EXEC_PROGRAM( svnversion
#      ARGS ${PROJECT_SOURCE_DIR}
#      OUTPUT_VARIABLE SVNVER )

set_target_properties( widgets
   # PROPERTIES COMPILE_FLAGS "-DSVNVERSION='\"${SVNVER}\"' -include ${PROJECT_BINARY_DIR}/all.h ${MUSECXXFLAGS} -I../ -I${PROJECT_SOURCE_DIR}/synti "
   PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h ${MUSECXXFLAGS} -I../ -I${PROJECT_SOURCE_DIR}/synti "
   )



