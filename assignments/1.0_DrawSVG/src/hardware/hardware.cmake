if(DRAWSVG_BUILD_HARDWARE_RENDERER)

  # Build hardware renderer
  include_directories(${CMAKE_CURRENT_SOURCE_DIR})

  # drawsvg hardware renderer source
  set(CMU462_DrawSVGHDWR_SOURCE
      hardware/hardware_renderer.cpp
  )

  # drawsvg hardware renderer lib
  add_library( drawsvg_hdwr STATIC
      ${CMU462_DrawSVGHDWR_SOURCE}
  )

  # output name
  if (UNIX)
    set_target_properties(drawsvg_hdwr PROPERTIES OUTPUT_NAME drawsvghdwr)
    if(APPLE)
      set_target_properties(drawsvg_hdwr PROPERTIES OUTPUT_NAME drawsvghdwr_osx)
    endif(APPLE)
  endif(UNIX)
  install(TARGETS drawsvg_hdwr DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/hardware)

else(DRAWSVG_BUILD_HARDWARE_RENDERER)

  add_library( drawsvg_hdwr STATIC IMPORTED)

  # Import hardware
  if (UNIX)
    set_property(TARGET drawsvg_hdwr PROPERTY IMPORTED_LOCATION
                 ${CMAKE_CURRENT_SOURCE_DIR}/hardware/libdrawsvghdwr.a)
  endif(UNIX)

  if(APPLE)
    set_property(TARGET drawsvg_hdwr PROPERTY IMPORTED_LOCATION
                 ${CMAKE_CURRENT_SOURCE_DIR}/hardware/libdrawsvghdwr_osx.a)
  endif(APPLE)

endif(DRAWSVG_BUILD_HARDWARE_RENDERER)
