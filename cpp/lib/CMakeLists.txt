file(GLOB_RECURSE opendnp3_public_headers LIST_DIRECTORIES false CONFIGURE_DEPENDS ./include/opendnp3/*.h)

file(GLOB_RECURSE opendnp3_private_headers LIST_DIRECTORIES false CONFIGURE_DEPENDS ./src/*.h)

file(GLOB_RECURSE opendnp3_src LIST_DIRECTORIES false CONFIGURE_DEPENDS ./src/*.cpp)

file(GLOB_RECURSE opendnp3_tls_private_headers LIST_DIRECTORIES false CONFIGURE_DEPENDS ./src/channel/tls/*.h)
list (REMOVE_ITEM opendnp3_private_headers ${opendnp3_tls_private_headers})

file(GLOB_RECURSE opendnp3_tls_src LIST_DIRECTORIES false CONFIGURE_DEPENDS ./src/channel/tls/*.cpp)
list (REMOVE_ITEM opendnp3_src ${opendnp3_tls_src})

set(all_sources ${opendnp3_public_headers} ${opendnp3_private_headers} ${opendnp3_src})
if(DNP3_TLS)
    list(APPEND all_sources ${opendnp3_tls_private_headers} ${opendnp3_tls_src})
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${all_sources})

add_library(opendnp3 ${all_sources})
target_compile_features(opendnp3 PUBLIC cxx_std_14)
target_link_libraries(opendnp3 PRIVATE
    $<BUILD_INTERFACE:asio>
    $<$<BOOL:${DNP3_TLS}>:OpenSSL::SSL>
)
target_include_directories(opendnp3
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_target_properties(opendnp3 PROPERTIES FOLDER cpp/lib)

if(DNP3_TLS)
    target_compile_definitions(opendnp3 PRIVATE OPENDNP3_USE_TLS)
endif()

install(TARGETS opendnp3
    EXPORT opendnp3-export
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(EXPORT opendnp3-export
    NAMESPACE opendnp3::
    DESTINATION lib/cmake
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
)

clang_format(opendnp3 EXCLUDES .*/gen/.*)
clang_tidy(opendnp3)
