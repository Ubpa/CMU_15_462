cmake_minimum_required(VERSION 2.8)

#-------------------------------------------------------------------------------
# Set include directories
#-------------------------------------------------------------------------------
include_directories(
    ${FREETYPE_INCLUDE_DIRS}
    ${CMU462_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
)

if (WIN32)
    include_directories(dirent)
endif(WIN32)

#-------------------------------------------------------------------------------
# Set link directories
#-------------------------------------------------------------------------------
link_directories(
    ${FREETYPE_LIBRARY_DIRS}
    ${CMU462_LIBRARY_DIRS}
    ${GLEW_LIBRARY_DIRS}
    ${GLFW_LIBRARY_DIRS}
)

# Set drawsvg source
set(CMU462_DRAWSVG_SOURCE
    svg.cpp
    png.cpp
    texture.cpp
    viewport.cpp
    triangulation.cpp
#    hardware_renderer.cpp
    software_renderer.cpp
    drawsvg.cpp
    main.cpp
)

if (WIN32)
    list(APPEND CMU462_DRAWSVG_SOURCE dirent/dirent.c)
endif(WIN32)

# Set drawsvg header
set(CMU462_DRAWSVG_HEADER
    svg.h
    png.h
    texture.h
    viewport.h
    triangulation.h
    hardware_renderer.h
    software_renderer.h
    drawsvg.h
)

# Import hardware renderer
option(DRAWSVG_BUILD_HARDWARE_RENDERER  "Build hardware implementation"  ON)
include(hardware/hardware.cmake)

# Import drawsvg reference
include(reference/reference.cmake)

#-------------------------------------------------------------------------------
# Add executable
#-------------------------------------------------------------------------------
add_executable( drawsvg
    ${CMU462_DRAWSVG_SOURCE}
    ${CMU462_DRAWSVG_HEADER}
)

# Link drawsvg executable (static link reference solution)
target_link_libraries( drawsvg drawsvg_hdwr drawsvg_ref
    ${FREETYPE_LIBRARIES}
    ${OPENGL_LIBRARIES}
    CMU462 ${CMU462_LIBRARIES}
    glew ${GLEW_LIBRARIES}
    glfw ${GLFW_LIBRARIES}
)

if (APPLE) # Frameworks required on osx
  target_link_libraries(drawsvg  
    ${COCOA_LIBRARIES}
    ${IOKIT_LIBRARIES}
    ${COREVIDEO_LIBRARIES}
  )
else(UNIX)  #LINUX
target_link_libraries( drawsvg drawsvg_hdwr drawsvg_ref
    ${CMU462_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${OPENGL_LIBRARIES}
    -fopenmp 
    -lXi 
    -lXxf86vm 
    -lXinerama 
    -lXcursor 
    -lXfixes 
    -lXrandr 
    -lXext 
    -lXrender 
    -lX11 
    -lpthread 
    -lxcb 
    -lXau
)
endif()

# Put executable in build directory root
set(EXECUTABLE_OUTPUT_PATH ..)

# Install to project root
install(TARGETS drawsvg DESTINATION ${drawsvg_SOURCE_DIR})

# Copy Freetype DLLs to the build directory
if(WIN32)
    foreach(FREETYPE_DLL ${FREETYPE_RUNTIMELIBS})
        add_custom_command(TARGET drawsvg POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FREETYPE_DLL}"
                $<TARGET_FILE_DIR:drawsvg>)
    endforeach(FREETYPE_DLL)
endif(WIN32)
