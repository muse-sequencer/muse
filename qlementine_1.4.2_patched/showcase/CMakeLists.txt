set(SHOWCASE_NAME "showcase")

# Dependency: qlementine-icons
include(FetchContent)
FetchContent_Declare(qlementine-icons
  GIT_REPOSITORY "https://github.com/oclero/qlementine-icons.git"
  GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(qlementine-icons)
set_target_properties(qlementine-icons
  PROPERTIES
    FOLDER dependencies
)

if(APPLE)
  set(APP_ICON_MACOS "${CMAKE_SOURCE_DIR}/branding/icon/icon.icns")
  set_source_files_properties(${APP_ICON_MACOS}
    PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources"
  )
endif()

set(SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ShowcaseWindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ShowcaseWindow.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/showcase.qrc
)

qt_add_executable(${SHOWCASE_NAME}
  WIN32 MACOSX_BUNDLE
  ${SOURCES}
)

target_link_libraries(${SHOWCASE_NAME}
  PUBLIC
    qlementine
    qlementine-icons
)

install(
  TARGETS ${SHOWCASE_NAME}
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set_target_properties(${SHOWCASE_NAME}
  PROPERTIES
    INTERNAL_CONSOLE OFF
    EXCLUDE_FROM_ALL OFF
    FOLDER "tools"
    CMAKE_AUTOMOC ON
    CMAKE_AUTORCC ON
    CMAKE_AUTOUIC ON
    MACOSX_BUNDLE_GUI_IDENTIFIER "oclero.qlementine.icons.${SHOWCASE_NAME}"
    MACOSX_BUNDLE_BUNDLE_NAME "Showcase"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_ICON_FILE "icon.icns"
    MACOSX_BUNDLE_COPYRIGHT ${PROJECT_COPYRIGHT}
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "${XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED}"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY}"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "${XCODE_ATTRIBUTE_CODE_SIGN_STYLE}"
    XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS OFF
)
