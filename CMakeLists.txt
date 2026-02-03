zephyr_library()

if (CONFIG_ZMK_SPLIT_STATUS_RELAY)

  zephyr_syscall_header(include/split_status_relay.h)

  if (CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zephyr_library_sources(src/split_status_relay_central.c)
    if (CONFIG_ZMK_SPLIT_BLE)
      zephyr_library_sources(src/ble/split_status_relay_ble_central.c)
    endif()
  endif()
  if (CONFIG_ZMK_SPLIT AND (NOT CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
    zephyr_library_sources(src/split_status_relay_peripheral.c)
    if (CONFIG_ZMK_SPLIT_BLE)
      zephyr_library_sources(src/ble/split_status_relay_ble_peripheral.c)
    endif()
  endif()

  zephyr_include_directories(include)
  zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)

endif()