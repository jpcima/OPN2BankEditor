cmake_minimum_required(VERSION 3.3)

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")

add_executable(test
  "sources/main.cc"
  "sources/tinysynth.cc"
  "../src/bank.cpp"
  "../src/common.cpp"
  "../src/FileFormats/ffmt_base.cpp"
  "../src/FileFormats/format_wohlstand_opn2.cpp"
  "../src/opl/chips/mame_opna.cpp"
  "../src/opl/chips/mamefm/emu2149.c"
  "../src/opl/chips/mamefm/fm.cpp"
  "../src/opl/chips/mamefm/resampler.cpp"
  "../src/opl/chips/mamefm/ymdeltat.cpp"
  "../src/opl/chips/np2_opna.cpp"
  "../src/opl/chips/np2/fmgen_file.cpp"
  "../src/opl/chips/np2/fmgen_fmgen.cpp"
  "../src/opl/chips/np2/fmgen_fmtimer.cpp"
  "../src/opl/chips/np2/fmgen_opna.cpp"
  "../src/opl/chips/np2/fmgen_psg.cpp"
  "../src/opl/chips/nuked_opn2.cpp"
  "../src/opl/chips/nuked/ym3438.c")
target_include_directories(test PRIVATE "../src")

find_package(Qt5Core REQUIRED)
target_link_libraries(test PUBLIC Qt5::Core)
