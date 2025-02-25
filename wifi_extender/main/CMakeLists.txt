idf_component_register(SRCS "net_tools.c" "main.c"
                    PRIV_REQUIRES spi_flash nvs_flash esp_wifi esp_event
                    INCLUDE_DIRS "")
