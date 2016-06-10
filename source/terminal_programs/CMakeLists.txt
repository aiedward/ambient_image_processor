cmake_minimum_required(VERSION 2.8.11)

cuda_add_executable(tgv2_deshade_application
  ${CMAKE_CURRENT_LIST_DIR}/tgv2_deshade_main.cpp
  ${SOURCE_ALL_FILES}
  ${CUDA_CPP_FILES}
)

target_link_libraries(tgv2_deshade_application
  Qt5::Core
  Qt5::Widgets
  -lfftw3
)

list(APPEND ITK_APPLICATIONS
  tgv2_deshade_application
  )

include(${CMAKE_CURRENT_LIST_DIR}/image_metrics/CMakeLists.txt)