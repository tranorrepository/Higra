function(REGISTER_PYTHON_MODULE_FILES PYTHON_FILES)
    foreach (FILE ${PYTHON_FILES})
        configure_file(${FILE} ${CMAKE_CURRENT_BINARY_DIR}/${FILE} COPYONLY)
    endforeach ()
endfunction(REGISTER_PYTHON_MODULE_FILES)
