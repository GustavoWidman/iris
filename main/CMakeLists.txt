idf_component_register(SRCS "app_main.cpp" INCLUDE_DIRS "." "../.private" REQUIRES esp_matter bt esp_wifi nvs_flash esp_timer)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
