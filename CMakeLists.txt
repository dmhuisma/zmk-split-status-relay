zephyr_library()

if (CONFIG_ZMK_SPLIT_STATUS_RELAY)

  # Event implementations
  zephyr_library_sources(src/events/ssr_peripheral_connection_state_changed.c)
  zephyr_library_sources(src/events/ssr_central_battery_state_changed.c)
  zephyr_library_sources(src/events/ssr_peripheral_battery_state_changed.c)
  zephyr_library_sources(src/events/ssr_central_layer_state_changed.c)
  zephyr_library_sources(src/events/ssr_central_wpm_state_changed.c)
  zephyr_library_sources(src/events/ssr_central_transport_changed.c)
  zephyr_library_sources(src/events/ssr_central_ble_profile_changed.c)
  zephyr_library_sources(src/events/ssr_central_usb_conn_state_changed.c)
  zephyr_library_sources(src/events/ssr_peripheral_usb_conn_state_changed.c)

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