set(QGISPYTHON_SRCS qgispython.cpp qgspythonutilsimpl.cpp)

add_library (qgispython SHARED ${QGISPYTHON_SRCS})

# require c++17
target_compile_features(qgispython PRIVATE cxx_std_17)

target_include_directories(qgispython PUBLIC
  ${CMAKE_SOURCE_DIR}/src/python
  ${CMAKE_BINARY_DIR}/src/python
)

#generate unversioned libs for android
if (NOT ANDROID)
  set_target_properties(qgispython PROPERTIES
    VERSION ${COMPLETE_VERSION}
    SOVERSION ${COMPLETE_VERSION}
  )
endif()

target_link_libraries(qgispython
  qgis_core
  Python::Python
  ${OPENPTY_LIBRARY}
)

GENERATE_EXPORT_HEADER(
   qgispython
   BASE_NAME PYTHON
   EXPORT_FILE_NAME qgis_python.h
 )

if (QGISPOSTFIX)
  set_target_properties(qgispython PROPERTIES
    RUNTIME_OUTPUT_NAME qgispython${QGISPOSTFIX}
    LIBRARY_OUTPUT_NAME qgispython${QGISPOSTFIX}
  )
endif()

install(TARGETS qgispython
  RUNTIME DESTINATION ${QGIS_BIN_DIR}
  LIBRARY DESTINATION ${QGIS_LIB_DIR}
)
