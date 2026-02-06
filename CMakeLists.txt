cmake_minimum_required(VERSION 3.16)

project(Hearts VERSION 2.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.4.2 REQUIRED COMPONENTS
    Widgets
    LinguistTools
    Multimedia
    Svg
    SvgWidgets
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ALLEGRO REQUIRED IMPORTED_TARGET
    allegro-5>=5.2.8
    allegro_audio-5>=5.2.8
    allegro_acodec-5>=5.2.8
)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp          mainwindow.h          mainwindow.ui
    sounds.h                sounds.cpp
    deck.h                  deck.cpp
    cardscene.h             cardscene.cpp
    define.h
    turnarrow.h             turnarrow.cpp
    background.h            background.cpp
    config.h                config.cpp
    engine.h                engine.cpp
    cpus.h
    draggablescoregroup.h   draggablescoregroup.cpp
    statistics.cpp          statistics.h          statistics.ui
    client.cpp              client.h              resourcepaths.h
    resources.qrc
)

qt_add_executable(Hearts
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

# Traductions
set(TS_FILES
    translations/Hearts_en_CA.ts
    translations/Hearts_fr_CA.ts
    translations/Hearts_ru_RU.ts
)

qt_add_translations(Hearts
    TS_FILES ${TS_FILES}
    RESOURCE_PREFIX "/translations"
)

target_link_libraries(Hearts PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Svg
    Qt6::SvgWidgets
    PkgConfig::ALLEGRO
)

# Propriétés bundle (macOS / Windows)
set_target_properties(Hearts PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER    com.example.Hearts
    MACOSX_BUNDLE_BUNDLE_VERSION    ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# ────────────────────────────────────────────────
# Installation
# ────────────────────────────────────────────────

install(TARGETS Hearts
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Ressources → /usr/share/hearts/ (ou /usr/local/share/hearts)
install(DIRECTORY
    "${CMAKE_CURRENT_SOURCE_DIR}/backgrounds"
    "${CMAKE_CURRENT_SOURCE_DIR}/icons"
    "${CMAKE_CURRENT_SOURCE_DIR}/sounds"
    "${CMAKE_CURRENT_SOURCE_DIR}/SVG-cards"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/hearts
    FILES_MATCHING
    PATTERN "*"
    PATTERN "*~" EXCLUDE
    PATTERN ".git*" EXCLUDE
)

# Traductions installées
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/Hearts_en_CA.qm"
    "${CMAKE_CURRENT_BINARY_DIR}/Hearts_fr_CA.qm"
    "${CMAKE_CURRENT_BINARY_DIR}/Hearts_ru_RU.qm"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/hearts/translations
)

# .desktop
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/hearts.desktop.in"
    "${CMAKE_CURRENT_BINARY_DIR}/hearts.desktop"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/hearts.desktop"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

# Icône SVG
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/icons/hearts.svg"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    RENAME hearts.svg
)

qt_finalize_executable(Hearts)
