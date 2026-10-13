idf_component_register(SRCS "app_main.cpp" INCLUDE_DIRS "." "../.private" REQUIRES esp_matter bt esp_wifi nvs_flash esp_timer)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")

option(IRIS_EXPERIMENTAL_RELATIVE_BALANCE "Enable unvalidated relative channel balance (identity otherwise)" OFF)
if(IRIS_EXPERIMENTAL_RELATIVE_BALANCE)
 target_compile_definitions(${COMPONENT_LIB} PRIVATE IRIS_EXPERIMENTAL_RELATIVE_BALANCE=1)
endif()
