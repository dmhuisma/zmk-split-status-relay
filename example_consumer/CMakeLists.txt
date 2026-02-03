zephyr_library()

if (CONFIG_ZMK_SPLIT_STATUS_RELAY_CONSUMER)
  
  zephyr_library_sources(src/split_status_relay_consumer.c)

  zephyr_include_directories(include)
  zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)

endif()