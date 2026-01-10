list(APPEND ULIGHT_MODULE_EXAMPLES 
    modules_test
)

foreach(module_example IN LISTS ULIGHT_MODULE_EXAMPLES)
    add_executable(${module_example} ${module_example}.cpp)
    target_link_libraries(${module_example} PRIVATE ulight::module)
    target_compile_features(${module_example} PRIVATE cxx_std_23)
endforeach()
