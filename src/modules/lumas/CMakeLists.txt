if(NOT MOD_LUMAS)
    return()
endif()

add_executable(luma luma.c)
# luma requires libmlt to work, so we set the output directory for luma to the location of this library
set_target_properties(luma PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/src/framework")
target_link_libraries(luma mlt)
function(create_lumas dir w h bpp)
    set(${dir}
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma01.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma02.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma03.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma04.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma05.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma06.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma07.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma08.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma09.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma10.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma11.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma12.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma13.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma14.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma15.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma16.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma17.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma18.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma19.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma20.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma21.pgm
        ${CMAKE_CURRENT_BINARY_DIR}/${dir}/luma22.pgm
        )
    add_custom_command(OUTPUT ${${dir}} DEPENDS luma
        COMMAND ${CMAKE_COMMAND} -E make_directory ${dir}
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma01.pgm
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma02.pgm -bands ${h}
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma03.pgm -hmirror 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma04.pgm -bands ${h} -vmirror 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma05.pgm -offset 32768 -dmirror 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma06.pgm -offset 32768 -dmirror 1 -flip 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma07.pgm -offset 32768 -dmirror 1 -quart 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma08.pgm -offset 32768 -dmirror 1 -quart 1 -flip 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma09.pgm -bands 12 -rband 0
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma10.pgm -bands 12 -rband 0 -rotate 1 -flop 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma11.pgm -bands 12 -rband 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma12.pgm -bands 12 -rband 1 -vmirror 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma13.pgm -bands 12 -rband 1 -rotate 1 -flop 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma14.pgm -bands 12 -rband 1 -rotate 1 -flop 1 -vmirror 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma15.pgm -offset 32768 -dmirror 1 -hmirror 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma16.pgm -type 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma17.pgm -type 1 -bands 2 -rband 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma18.pgm -type 2
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma19.pgm -type 2 -quart 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma20.pgm -type 2 -quart 1 -flip 1
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma21.pgm -type 2 -quart 1 -bands 2
        COMMAND luma -w ${w} -h ${h} -bpp ${bpp} ${dir}/luma22.pgm -type 3)
    add_custom_target(${dir} ALL DEPENDS ${${dir}})
    install(FILES ${${dir}} DESTINATION ${CMAKE_INSTALL_DATADIR}/mlt/lumas/${dir})
endfunction()
create_lumas(PAL 720 576 16)
create_lumas(NTSC 720 480 16)
create_lumas(16_9 1920 1080 16)
create_lumas(9_16 1080 1920 16)
create_lumas(square 1080 1080 16)
