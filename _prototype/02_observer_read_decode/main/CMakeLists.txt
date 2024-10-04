
set(main_src
  framequeue.cpp
  framequeue.h
  globalstate.cpp
  globalstate.h
  main.cpp
  myavpacketlist.h
  observer.h
  packetqueue.cpp
  packetqueue.h
  stringhelper.h
  subject.cpp
  subject.h
  videocontroller.cpp
  videocontroller.h
  videodecoder.cpp
  videodecoder.h
  videoreader.cpp
  videoreader.h
)

add_executable(
  ${PROJECT_NAME}
  ${main_src}
)

# Link
if (WIN32)
  target_link_libraries(
    ${PROJECT_NAME}
    ${FFMPEG_PATH_LIB}/avcodec.lib
    ${FFMPEG_PATH_LIB}/avdevice.lib
    ${FFMPEG_PATH_LIB}/avfilter.lib
    ${FFMPEG_PATH_LIB}/avformat.lib
    ${FFMPEG_PATH_LIB}/avutil.lib
    ${FFMPEG_PATH_LIB}/swresample.lib
    ${FFMPEG_PATH_LIB}/swscale.lib
    SDL2::SDL2
  )
else()
  # Linux
  target_link_libraries(
    ${PROJECT_NAME}
    PkgConfig::FFMPEG
    SDL2::SDL2
  )
endif()

if (WIN32)
  # Copy dlls
  foreach(DLL IN LISTS EXTERNAL_DLLS)
    # Get the file name of the DLL
    get_filename_component(DLL_FILENAME "${DLL}" NAME)

    # Command to copy the DLL file
    add_custom_command(
      TARGET ${PROJECT_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "${DLL}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/${DLL_FILENAME}"
      COMMENT "Copying ${DLL_FILENAME} to output directory."
    )
  endforeach()
endif()

