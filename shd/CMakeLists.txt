function(add_shaders TARGET_NAME)
  set(SHADER_FILES ${ARGN})

  list(LENGTH SHADER_FILES FILE_COUNT)
  if(FILE_COUNT EQUAL 0)
    message(FATAL_ERROR "Cannot create shader without source files.")
  endif()

  set(SHADER_COMMANDS)
  set(SHADER_PRODUCTS)

  foreach(SHADER_SOURCE IN LISTS SHADER_FILES)
    cmake_path(ABSOLUTE_PATH SHADER_SOURCE NORMALIZE)
    cmake_path(GET SHADER_SOURCE FILENAME SHADER_NAME)
    set(SHADER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.spv")

    list(APPEND SHADER_COMMAND COMMAND)
    list(APPEND SHADER_COMMAND glslc)
    list(APPEND SHADER_COMMAND "${SHADER_SOURCE}")
    list(APPEND SHADER_COMMAND "-o")
    list(APPEND SHADER_COMMAND "${SHADER_OUTPUT}")
    list(APPEND SHADER_COMMANDS ${SHADER_COMMAND})

    list(APPEND SHADER_PRODUCTS "${SHADER_OUTPUT}")
  endforeach()

  add_custom_target(
    ${TARGET_NAME} ALL
    ${SHADER_COMMANDS}
    COMMENT "Compiling shader [${TARGET_NAME}]"
    SOURCES ${SHADER_SOURCE_FILES}
    BYPRODUCTS ${SHADER_PRODUCTS}
  )
endfunction()

add_shaders(shader shader.vert shader.frag)
