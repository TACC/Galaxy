cmake_minimum_required (VERSION 3.9)

include_directories(${gxy_framework_SOURCE_DIR}
                    ${gxy_renderer_SOURCE_DIR}
                    ${gxy_renderer_BINARY_DIR}
                    ${gxy_sampler_SOURCE_DIR}
                    ${OSPRAY_INCLUDE_DIRS}
                    ${gxy_data_SOURCE_DIR}
                    ${gxy_ospray_SOURCE_DIR}
                    ${Galaxy_BINARY_DIR}/src)

set(Galaxy_LIBRARIES
    gxy_framework
    gxy_renderer
    gxy_data
    gxy_ospray
    ${OSPRAY_LIBRARIES}
    ${MPI_C_LIBRARIES})

if (GXY_WRITE_IMAGES)

  add_executable(tester tester.cpp RungeKutta.cpp TraceToPathLines.cpp)
  target_link_libraries(tester ${VTK_LIBRARIES} ${Galaxy_LIBRARIES} ${MPI_C_LIBRARIES})
  set(BINS tester)

  add_executable(sampletrace sampletrace.cpp RungeKutta.cpp TraceToPathLines.cpp Interpolator.cpp)
  target_link_libraries(sampletrace gxy_sampler ${VTK_LIBRARIES} ${Galaxy_LIBRARIES} ${MPI_C_LIBRARIES})
  set(BINS sampletrace ${BINS})

  install(TARGETS ${BINS} DESTINATION bin)

endif (GXY_WRITE_IMAGES)

