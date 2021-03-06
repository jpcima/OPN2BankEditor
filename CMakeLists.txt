cmake_minimum_required(VERSION 3.2)
project(OPN2BankEditor LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang)$")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  add_definitions("-D_USE_MATH_DEFINES")
endif()

set(USE_RTMIDI "ON" CACHE STRING "Use RtMidi")
set(USE_RTAUDIO "ON" CACHE STRING "Use RtAudio")

include(FindPkgConfig)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
set(QWT_NAMES qwt-qt5 qwt) # find versions for Qt5 only
find_package(Qwt)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

if(QWT_FOUND)
  set(ENABLE_PLOTS ON)
else()
  set(ENABLE_PLOTS OFF)
endif()

message("!! Optional feature summary:")
message("!!   RtMidi: ${USE_RTMIDI}")
message("!!   RtAudio: ${USE_RTAUDIO}")

if(USE_RTAUDIO)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    pkg_check_modules(JACK "jack")
    message("!!   -- Jack driver: ${JACK_FOUND}")
    pkg_check_modules(PULSEAUDIO "libpulse-simple")
    message("!!   -- Pulseaudio driver: ${PULSEAUDIO_FOUND}")
  endif()
endif()

if(ENABLE_PLOTS)
  message("!!   Qwt: ${QWT_LIBRARY}")
endif()

message("!!   Plots: ${ENABLE_PLOTS}")

include_directories("src")

set(COMMON_SOURCES
  "src/common.cpp"
  "src/bank.cpp")
add_library(Common STATIC ${COMMON_SOURCES})
target_include_directories(Common PUBLIC "src")
target_link_libraries(Common PUBLIC Qt5::Widgets)

set(FILEFORMATS_SOURCES
  "src/FileFormats/ffmt_base.cpp"
  "src/FileFormats/ffmt_enums.cpp"
  "src/FileFormats/ffmt_factory.cpp"
  "src/FileFormats/format_deflemask_dmp.cpp"
  "src/FileFormats/format_tfi.cpp"
  "src/FileFormats/format_gems_pat.cpp"
  "src/FileFormats/format_m2v_gyb.cpp"
  "src/FileFormats/format_tomsoft_gin.cpp"
  "src/FileFormats/format_saxman_ymx.cpp"
  "src/FileFormats/format_vgm_import.cpp"
  "src/FileFormats/format_gym_import.cpp"
  "src/FileFormats/format_s98_import.cpp"
  "src/FileFormats/format_tx81z_import.cpp"
  "src/FileFormats/format_gens_y12.cpp"
  "src/FileFormats/format_opm.cpp"
  "src/FileFormats/format_mucom88_dat.cpp"
  "src/FileFormats/format_vgi.cpp"
  "src/FileFormats/format_bamboo_bti.cpp"
  "src/FileFormats/format_wohlstand_opn2.cpp"
  "src/FileFormats/ym2612_to_wopi.cpp"
  "src/FileFormats/ym2151_to_wopi.cpp"
  "src/FileFormats/text_format.cpp"
  "src/FileFormats/text_format_tokens.cpp")
add_library(FileFormats STATIC ${FILEFORMATS_SOURCES})
target_include_directories(FileFormats PUBLIC "src" PRIVATE ${ZLIB_INCLUDE_DIRS})
target_link_libraries(FileFormats PUBLIC Common PRIVATE ${ZLIB_LIBRARIES})

include(src/opl/chips/chipset.cmake)
add_library(Chips STATIC ${CHIPS_SOURCES})
target_include_directories(Chips PUBLIC "src")

set(MEASURER_SOURCES
  "src/opl/measurer.cpp")
add_library(Measurer STATIC ${MEASURER_SOURCES})
target_include_directories(Measurer PUBLIC "src")
target_link_libraries(Measurer PUBLIC Chips Common Qt5::Concurrent)
if(NOT MSVC)
  target_compile_options(Measurer PRIVATE "-fopenmp")
endif()
if(ENABLE_PLOTS)
  target_compile_definitions(Measurer PUBLIC "-DENABLE_PLOTS")
endif()

set(SOURCES
  "src/audio.cpp"
  "src/bank_editor.cpp"
  "src/operator_editor.cpp"
  "src/bank_comparison.cpp"
  "src/controlls.cpp"
  "src/proxystyle.cpp"
  "src/formats_sup.cpp"
  "src/importer.cpp"
  "src/audio_config.cpp"
  "src/register_editor.cpp"
  "src/ins_names.cpp"
  "src/main.cpp"
  "src/opl/generator.cpp"
  "src/opl/generator_realtime.cpp"
  "src/opl/realtime/ring_buffer.cpp"
  "src/piano.cpp")
if(ENABLE_PLOTS)
  list(APPEND SOURCES
    "src/delay_analysis.cpp")
endif()

set(UIS
  "src/bank_editor.ui"
  "src/operator_editor.ui"
  "src/bank_comparison.ui"
  "src/formats_sup.ui"
  "src/importer.ui"
  "src/audio_config.ui"
  "src/register_editor.ui")
if(ENABLE_PLOTS)
  list(APPEND UIS
    "src/delay_analysis.ui")
endif()

qt5_wrap_ui(FORMS ${UIS})

qt5_add_resources(RESOURCES
  "src/resources/resources.qrc")

qt5_add_translation(TRANSLATIONS
  "src/translations/opn2bankeditor_fr_FR.ts"
  "src/translations/opn2bankeditor_ru_RU.ts"
)
add_custom_target(OPN3BankEditor_translations ALL
  DEPENDS ${TRANSLATIONS})
install(FILES ${TRANSLATIONS}
  DESTINATION "share/opn2_bank_editor/translations")

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" AND
    NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  install(FILES "src/resources/opn2_bank_editor.desktop"
    DESTINATION "share/applications")
  foreach(size 16;32;48)
    install(FILES "src/resources/opn2_${size}.png"
      DESTINATION "share/icons/hicolor/${size}x${size}/apps"
      RENAME "opn2_bank_editor.png")
  endforeach()
endif()

add_executable(OPN2BankEditor WIN32 MACOSX_BUNDLE
  ${SOURCES} ${FORMS} ${RESOURCES})
set_target_properties(OPN2BankEditor PROPERTIES
  OUTPUT_NAME "opn2_bank_editor")
if(ENABLE_PLOTS)
  target_include_directories(OPN2BankEditor PRIVATE ${QWT_INCLUDE_DIRS})
endif()
target_link_libraries(OPN2BankEditor PRIVATE FileFormats Chips Measurer)

target_link_libraries(OPN2BankEditor PRIVATE Qt5::Widgets Qt5::Concurrent ${CMAKE_THREAD_LIBS_INIT})
if(ENABLE_PLOTS)
  target_link_libraries(OPN2BankEditor PRIVATE ${QWT_LIBRARIES})
endif()
install(TARGETS OPN2BankEditor DESTINATION "bin" RENAME "opn2_bank_editor")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(ICON "${CMAKE_SOURCE_DIR}/src/resources/res.rc")
  target_sources(OPN2BankEditor PRIVATE "${ICON}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(MACOSX_BUNDLE_ICON_FILE "opn2.icns")
  set(ICON "${CMAKE_SOURCE_DIR}/src/resources/opn2.icns")
  set_source_files_properties("${ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  target_sources(OPN2BankEditor PRIVATE "${ICON}")
endif()

if(USE_RTMIDI)
  add_library(RtMidi STATIC "src/midi/external/rtmidi/RtMidi.cpp")
  target_include_directories(RtMidi PUBLIC "src/midi/external/rtmidi")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(RtMidi PUBLIC "__LINUX_ALSA__")
    target_link_libraries(RtMidi PUBLIC "asound")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(RtMidi PUBLIC "__WINDOWS_MM__")
    target_link_libraries(RtMidi PUBLIC "winmm")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(RtMidi PUBLIC "__MACOSX_CORE__")
    find_library(COREMIDI_LIBRARY "CoreMIDI")
    target_link_libraries(RtMidi PUBLIC "${COREMIDI_LIBRARY}")
  endif()
  target_sources(OPN2BankEditor PRIVATE "src/midi/midi_rtmidi.cpp")
  target_compile_definitions(OPN2BankEditor PRIVATE "ENABLE_MIDI")
  target_link_libraries(OPN2BankEditor PRIVATE RtMidi)
endif()

if(USE_RTAUDIO)
  add_library(RtAudio STATIC "src/audio/external/rtaudio/RtAudio.cpp")
  target_include_directories(RtAudio PUBLIC "src/audio/external/rtaudio")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(RtAudio PUBLIC "__LINUX_ALSA__")
    target_link_libraries(RtAudio PUBLIC "asound")
    if(JACK_FOUND)
      target_compile_definitions(RtAudio PUBLIC "__UNIX_JACK__")
      target_include_directories(RtAudio PUBLIC ${JACK_INCLUDE_DIRS})
      link_directories(${JACK_LIBRARY_DIRS})
      target_link_libraries(RtAudio PUBLIC ${JACK_LIBRARIES})
    endif()
    if(PULSEAUDIO_FOUND)
      target_compile_definitions(RtAudio PUBLIC "__LINUX_PULSE__")
      target_include_directories(RtAudio PUBLIC ${PULSEAUDIO_INCLUDE_DIRS})
      link_directories(${PULSEAUDIO_LIBRARY_DIRS})
      target_link_libraries(RtAudio PUBLIC ${PULSEAUDIO_LIBRARIES})
    endif()
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(RtAudio PUBLIC "__WINDOWS_DS__")
    target_link_libraries(RtAudio PUBLIC "dsound" "ole32")
    target_compile_definitions(RtAudio PUBLIC "__WINDOWS_WASAPI__")
    target_link_libraries(RtAudio PUBLIC "ksguid")
    target_compile_definitions(RtAudio PUBLIC "__WINDOWS_ASIO__")
    target_include_directories(RtAudio PRIVATE
      "src/audio/external/rtaudio/include")
    target_sources(RtAudio PRIVATE
      "src/audio/external/rtaudio/include/asio.cpp"
      "src/audio/external/rtaudio/include/asiodrivers.cpp"
      "src/audio/external/rtaudio/include/asiolist.cpp"
      "src/audio/external/rtaudio/include/iasiothiscallresolver.cpp")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(RtAudio PUBLIC "__MACOSX_CORE__")
    find_library(COREAUDIO_LIBRARY "CoreAudio")
    target_link_libraries(RtAudio PUBLIC "${COREAUDIO_LIBRARY}")
  endif()
  target_sources(OPN2BankEditor PRIVATE "src/audio/ao_rtaudio.cpp")
  target_compile_definitions(OPN2BankEditor PRIVATE "ENABLE_AUDIO_TESTING")
  target_link_libraries(OPN2BankEditor PRIVATE RtAudio)
endif()

add_executable(measurer
  "utils/measurer/measurer.cpp")
target_link_libraries(measurer PRIVATE FileFormats Measurer)
