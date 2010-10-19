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
      ttoolbar.h  
      ttoolbutton.h  
      tb1.h  
      lcombo.h  
      swidget.h  
      tempolabel.h  
      tools.h  
      popupmenu.h  
      fontsel.h  
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
      posedit.h  
      pitchedit.h  
      pitchlabel.h  
      poslabel.h  
      filedialog.h  
      sigedit.h  
      shortcutconfig.h  
      shortcutcapturedialog.h  
      combobox.h  
      aboutbox_impl.h 
      aboutbox.h  
      songinfo.h  
      didyouknow.h  
      checkbox.h  
      vscale.h  
      action.h
      )

#QT4_WRAP_UI (widget_ui_headers
QT4_WRAP_UI3 (widget_ui_headers
      midisync.ui  
      gensetbase.ui  
      mixdowndialogbase.ui  
      transposebase.ui  
      transformbase.ui  
      appearancebase.ui  
      itransformbase.ui  
      commentbase.ui  
      synthconfigbase.ui  
      gatetimebase.ui  
      velocitybase.ui  
      metronomebase.ui  
      mtrackinfobase.ui  
      wtrackinfobase.ui  
      editctrl7dialogbase.ui  
      editmetadialogbase.ui  
      editnotedialogbase.ui  
      editsysexdialogbase.ui  
      cliplisteditorbase.ui  
      mittransposebase.ui  
      fdialogbuttons.ui  
      shortcutconfigbase.ui  
      shortcutcapturedialogbase.ui  
      aboutbox.ui  
      songinfo.ui  
      didyouknow.ui  
      configmidifilebase.ui
      )

# add_custom_command(OUTPUT ui_midisync.h
#      COMMAND ${QT_UIC_EXECUTABLE}
#      ARGS -o ${CMAKE_CURRENT_BINARY_DIR}/ui_midisync.h ${CMAKE_CURRENT_SOURCE_DIR}/midisync.ui
#      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/midisync.ui
#      )

# ADD_FILE_DEPENDENCIES(moc_midisync.cxx ${CMAKE_CURRENT_BINARY_DIR}/ui_midisync.h)

add_library ( widgets STATIC 
      ${widget_ui_headers}
      ${widget_mocs}
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
      scrollscale.cpp  # scrollscale.h  
      view.cpp  # view.h  
      nentry.cpp # nentry.h  
      mtscale.cpp # mtscale.h  
      sigscale.cpp # sigscale.h  
      hitscale.cpp # hitscale.h  
      intlabel.cpp # intlabel.h  
      doublelabel.cpp # doublelabel.h  
      canvas.cpp # canvas.h  
      noteinfo.cpp # noteinfo.h  
      dentry.cpp # dentry.h  
      siglabel.cpp # siglabel.h  
      ttoolbar.cpp # ttoolbar.h  
      ttoolbutton.cpp # ttoolbutton.h  
      tb1.cpp # tb1.h  
      lcombo.cpp # lcombo.h  
      swidget.cpp # swidget.h  
      tempolabel.cpp # tempolabel.h  
      tools.cpp # tools.h  
      fontsel.cpp # fontsel.h  
      ctrlcombo.cpp # ctrlcombo.h  
      sliderbase.cpp # sliderbase.h  
      slider.cpp # slider.h  
      midisyncimpl.cpp # midisyncimpl.h  
      header.cpp # header.h  
      knob.cpp # knob.h  
      mlabel.cpp # mlabel.h  
      genset.cpp # genset.h  
      mixdowndialog.cpp # mixdowndialog.h  
      spinboxFP.cpp # spinboxFP.h  
      spinbox.cpp # spinbox.h  
      comboQuant.cpp # comboQuant.h  
      bigtime.cpp # bigtime.h  
      comment.cpp # comment.h  
      splitter.cpp # splitter.h  
      gatetime.cpp # gatetime.h  
      velocity.cpp # velocity.h  
      metronome.cpp # metronome.h  
      posedit.cpp section.h # posedit.h 
      pitchedit.cpp # pitchedit.h  
      pitchlabel.cpp # pitchlabel.h  
      poslabel.cpp # poslabel.h  
      filedialog.cpp # filedialog.h  
      sigedit.cpp # sigedit.h  
      shortcutconfig.cpp # shortcutconfig.h  
      shortcutcapturedialog.cpp #shortcutcapturedialog.h  
      combobox.cpp # combobox.h  
      checkbox.cpp # checkbox.h  
      aboutbox_impl.cpp # aboutbox_impl.h  aboutbox.h
      didyouknow.h
      popupmenu.cpp # popupmenu.h  
      listitem.h  
      menutitleitem.h  
      )

# AM_CXXFLAGS +=-DSVNVERSION=\"$(shell svnversion)\"
EXEC_PROGRAM( svnversion
      ARGS ${PROJECT_SOURCE_DIR}
      OUTPUT_VARIABLE SVNVER )

set_target_properties( widgets
   PROPERTIES COMPILE_FLAGS "-DSVNVERSION='\"${SVNVER}\"' -include ${PROJECT_BINARY_DIR}/all.h ${MUSECXXFLAGS} -I../ -I${PROJECT_SOURCE_DIR}/synti "
   )



