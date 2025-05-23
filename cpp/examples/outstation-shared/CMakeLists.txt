add_executable(outstation-shred-demo ./main.cpp)
target_link_libraries (outstation-shred-demo PRIVATE opendnp3)
set_target_properties(outstation-shred-demo PROPERTIES FOLDER cpp/examples)
install(TARGETS outstation-shred-demo RUNTIME DESTINATION bin)
