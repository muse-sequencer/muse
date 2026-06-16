IF(NOT EXISTS "/home/ad/doks/git/muse/src/install_manifest.txt")
  MESSAGE(FATAL_ERROR "Cannot find install manifest: \"/home/ad/doks/git/muse/src/install_manifest.txt\"")
ENDIF(NOT EXISTS "/home/ad/doks/git/muse/src/install_manifest.txt")

SET(DESTDIR $ENV{DESTDIR})
FILE(READ "/home/ad/doks/git/muse/src/install_manifest.txt" files)
STRING(REGEX REPLACE "\n" ";" files "${files}")
FOREACH(file ${files})
  MESSAGE(STATUS "Uninstalling \"${DESTDIR}${file}\"")
  IF(EXISTS "${DESTDIR}${file}")
    EXEC_PROGRAM(
      "/usr/bin/cmake" ARGS "-E remove \"${DESTDIR}${file}\""
      OUTPUT_VARIABLE rm_out
      RETURN_VALUE rm_retval
      )
    IF("${rm_retval}" STREQUAL 0)
    ELSE("${rm_retval}" STREQUAL 0)
      MESSAGE(FATAL_ERROR "Problem when removing \"${DESTDIR}${file}\"")
    ENDIF("${rm_retval}" STREQUAL 0)
  ELSE(EXISTS "${DESTDIR}${file}")
    MESSAGE(STATUS "File \"${DESTDIR}${file}\" does not exist.")
  ENDIF(EXISTS "${DESTDIR}${file}")
ENDFOREACH(file)
