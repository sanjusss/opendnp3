if (DNP3_ASIO)
    find_package(asio REQUIRED)
    message(STATUS "Found asio")
    target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
else ()
    find_package(Boost REQUIRED)
    message(STATUS "Found Boost ${Boost_INCLUDE_DIR}")


    add_library(asio INTERFACE)
    target_include_directories(asio INTERFACE ${Boost_INCLUDE_DIR})
endif ()

find_package(Threads)
target_compile_features(asio INTERFACE cxx_std_11)
target_link_libraries(asio INTERFACE Threads::Threads)

if(WIN32)
    target_link_libraries(asio INTERFACE ws2_32 wsock32) # Link to Winsock
    target_compile_definitions(asio INTERFACE _WIN32_WINNT=0x0601) # Windows 7 and up
endif()