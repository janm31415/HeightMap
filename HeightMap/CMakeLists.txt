
set(IMGUI
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imconfig.h
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui.h
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_impl_sdl2.h
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_impl_sdlrenderer.h
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_internal.h
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imstb_rectpack.h
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imstb_textedit.h
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imstb_truetype.h
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_draw.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_demo.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_impl_sdl2.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_impl_sdlrenderer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_tables.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/imgui_widgets.cpp
)

set(HDRS
view.h
    )
	
set(SRCS
main.cpp
view.cpp
)

if (WIN32)
set(CMAKE_C_FLAGS_DEBUG "/W4 /MP /GF /RTCu /Od /MDd /Zi")
set(CMAKE_CXX_FLAGS_DEBUG "/W4 /MP /GF /RTCu /Od /MDd /Zi")
set(CMAKE_C_FLAGS_RELEASE "/W4 /MP /GF /O2 /Ob2 /Oi /Ot /MD /Zi")
set(CMAKE_CXX_FLAGS_RELEASE "/W4 /MP /GF /O2 /Ob2 /Oi /Ot /MD /Zi")
endif(WIN32)

include ("../jtk/jtk/jtk.cmake")

# general build definitions
add_definitions(-DNOMINMAX)
add_definitions(-D_UNICODE)
add_definitions(-DUNICODE)
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
#add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLEW)

add_executable(HeightMap WIN32 ${HDRS} ${SRCS} ${IMGUI})
source_group("Header Files" FILES ${hdrs})
source_group("Source Files" FILES ${srcs})
source_group("ImGui" FILES ${IMGUI})

 target_include_directories(HeightMap
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../imgui_sdl/
    ${CMAKE_CURRENT_SOURCE_DIR}/../SDL2/include/
    )	
	
target_link_libraries(HeightMap
    PRIVATE	
    SDL2
    SDL2main  
    )	
