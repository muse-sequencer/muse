  ## QT4_WRAP_UI3(outfiles inputfile ... )   
  ##
  ## Adapted from QT4_WRAP_UI in FindQt4.cmake module, for MusE, by Tim.
  ##
  ##
  
  MACRO (QT4_WRAP_UI3 outfiles )
    QT4_EXTRACT_OPTIONS(ui_files ui_options ${ARGN})

    FOREACH (it ${ui_files})
      GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)
      GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
      # SET(outfile_h ${CMAKE_CURRENT_BINARY_DIR}/ui_${outfile}.h)
      SET(outfile_h ${CMAKE_CURRENT_BINARY_DIR}/${outfile}.h)
      SET(outfile_cpp ${CMAKE_CURRENT_BINARY_DIR}/ui_${outfile}.cpp)
      ADD_CUSTOM_COMMAND(OUTPUT ${outfile_h} ${outfile_cpp}
        COMMAND ${QT_UIC3_EXECUTABLE}
        ARGS ${ui_options} -o ${outfile_h} ${infile}
        COMMAND ${QT_UIC3_EXECUTABLE}
        ARGS ${ui_options} -o ${outfile_cpp} -impl ${outfile_h} ${infile}
        COMMAND ${QT_MOC_EXECUTABLE}
        ARGS ${outfile_h} >> ${outfile_cpp}
        MAIN_DEPENDENCY ${infile})
      SET(${outfiles} ${${outfiles}} ${outfile_h} ${outfile_cpp})
    ENDFOREACH (it)

  ENDMACRO (QT4_WRAP_UI3)

 
