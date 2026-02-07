
#include <ssr_event.h>
#include <zephyr/logging/log.h>
#include <zmk/events/ssr_peripheral_connection_state_changed.h>
#include <zmk/events/ssr_central_battery_state_changed.h>
#include <zmk/events/ssr_peripheral_battery_state_changed.h>
#include <zmk/events/ssr_central_layer_state_changed.h>
#include <zmk/events/ssr_central_wpm_state_changed.h>
#include <zmk/events/ssr_central_transport_changed.h>
#include <zmk/events/ssr_central_ble_profile_changed.h>
#include <zmk/events/ssr_central_usb_conn_state_changed.h>
#include <zmk/events/ssr_peripheral_usb_conn_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

void raise_ssr_event(struct ssr_asdc_event* event) {

    // Raise individual ZMK events based on the event type
    switch (event->type) {
        case SSR_EVENT_CONNECTION_STATE: {
            struct zmk_ssr_peripheral_connection_state_changed *conn_event = (struct zmk_ssr_peripheral_connection_state_changed *)event->data;
            raise_zmk_ssr_peripheral_connection_state_changed(
                (struct zmk_ssr_peripheral_connection_state_changed){
                    .slot = conn_event->slot,
                    .connected = conn_event->connected
                });
        }
        break;
        case SSR_EVENT_CENTRAL_BATTERY_LEVEL: {
            struct zmk_ssr_central_battery_state_changed *battery_event = (struct zmk_ssr_central_battery_state_changed *)event->data;
            raise_zmk_ssr_central_battery_state_changed(
                (struct zmk_ssr_central_battery_state_changed){
                    .battery_level = battery_event->battery_level
                });
        }
        break;
        case SSR_EVENT_PERIPHERAL_BATTERY_LEVEL: {
            struct zmk_ssr_peripheral_battery_state_changed *battery_event = (struct zmk_ssr_peripheral_battery_state_changed *)event->data;
            raise_zmk_ssr_peripheral_battery_state_changed(
                (struct zmk_ssr_peripheral_battery_state_changed){
                    .slot = battery_event->slot,
                    .battery_level = battery_event->battery_level
                });
        }
        break;
        case SSR_EVENT_HIGHEST_ACTIVE_LAYER: {
            // Validate data contains at least the layer field plus null terminator
            if (event->data_length < sizeof(uint8_t) + 1) {
                LOG_ERR("SSRP: Layer event data too small: %d", event->data_length);
                break;
            }
            
            uint8_t layer = event->data[0];
            const char *layer_name = (const char *)&event->data[1];
            
            // Ensure null termination within the data bounds
            size_t max_name_len = event->data_length - sizeof(uint8_t);
            bool null_found = false;
            for (size_t i = 0; i < max_name_len; i++) {
                if (layer_name[i] == '\0') {
                    null_found = true;
                    break;
                }
            }
            
            if (!null_found) {
                LOG_ERR("SSRP: Layer name not null-terminated, data_length=%d", event->data_length);
                break;
            }
            
            raise_zmk_ssr_central_layer_state_changed(
                (struct zmk_ssr_central_layer_state_changed){
                    .layer = layer,
                    .layer_name = layer_name
                });
        }
        break;
        case SSR_EVENT_WPM: {
            struct zmk_ssr_central_wpm_state_changed *wpm_event = (struct zmk_ssr_central_wpm_state_changed *)event->data;
            raise_zmk_ssr_central_wpm_state_changed(
                (struct zmk_ssr_central_wpm_state_changed){
                    .wpm = wpm_event->wpm
                });
        }
        break;
        case SSR_EVENT_TRANSPORT: {
            struct zmk_ssr_central_transport_changed *transport_event = (struct zmk_ssr_central_transport_changed *)event->data;
            raise_zmk_ssr_central_transport_changed(
                (struct zmk_ssr_central_transport_changed){
                    .transport = transport_event->transport
                });
        }
        break;
        case SSR_EVENT_ACTIVE_BLE_PROFILE: {
            struct zmk_ssr_central_ble_profile_changed *ble_event = (struct zmk_ssr_central_ble_profile_changed *)event->data;
            raise_zmk_ssr_central_ble_profile_changed(
                (struct zmk_ssr_central_ble_profile_changed){
                    .active_profile_index = ble_event->active_profile_index,
                    .active_profile_connected = ble_event->active_profile_connected,
                    .active_profile_bonded = ble_event->active_profile_bonded
                });
        }
        break;
        case SSR_EVENT_CENTRAL_USB_CONNECTION_STATE: {
            struct zmk_ssr_central_usb_conn_state_changed *usb_event = (struct zmk_ssr_central_usb_conn_state_changed *)event->data;
            raise_zmk_ssr_central_usb_conn_state_changed(
                (struct zmk_ssr_central_usb_conn_state_changed){
                    .connected = usb_event->connected
                });
        }
        break;
        case SSR_EVENT_PERIPHERAL_USB_CONNECTION_STATE: {
            struct zmk_ssr_peripheral_usb_conn_state_changed *usb_event = (struct zmk_ssr_peripheral_usb_conn_state_changed *)event->data;
            raise_zmk_ssr_peripheral_usb_conn_state_changed(
                (struct zmk_ssr_peripheral_usb_conn_state_changed){
                    .slot = usb_event->slot,
                    .connected = usb_event->connected
                });
        }
        break;
        default:
            LOG_WRN("SSRP peripheral: Unknown event type %d", event->type);
            break;
    }
}