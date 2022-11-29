add_executable(anyopt
    main.cpp
    typetable.cpp
    irbuilder.cpp
)
set_target_properties(anyopt PROPERTIES CXX_STANDARD 17)
target_compile_definitions(anyopt PUBLIC -DANYOPT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR} -DANYOPT_VERSION_MINOR=${PROJECT_VERSION_MINOR})

target_include_directories(anyopt PUBLIC ${Thorin_INCLUDE_DIRS} ../include)

target_link_libraries(anyopt PUBLIC ${Thorin_LIBRARIES})
#target_link_libraries(anyopt PUBLIC libartic)
target_link_libraries(anyopt PRIVATE nlohmann_json::nlohmann_json)

if (Thorin_HAS_LLVM_SUPPORT)
    target_compile_definitions(anyopt PUBLIC -DENABLE_LLVM)
    llvm_config(anyopt ${AnyDSL_LLVM_LINK_SHARED} core support)
endif ()
