
#define DT_DRV_COMPAT zmk_split_status_relay_consumer

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zmk/events/ssr_peripheral_connection_state_changed.h>
#include <zmk/events/ssr_central_battery_state_changed.h>
#include <zmk/events/ssr_peripheral_battery_state_changed.h>
#include <zmk/events/ssr_central_layer_state_changed.h>
#include <zmk/events/ssr_central_wpm_state_changed.h>
#include <zmk/events/ssr_central_transport_changed.h>
#include <zmk/events/ssr_central_ble_profile_changed.h>
#include <zmk/events/ssr_central_usb_conn_state_changed.h>
#include <zmk/events/ssr_peripheral_usb_conn_state_changed.h>
#include <zmk/event_manager.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static int ssr_connection_listener(const zmk_event_t *eh) {
    const struct zmk_ssr_peripheral_connection_state_changed *ev = 
        as_zmk_ssr_peripheral_connection_state_changed(eh);
    if (ev) {
        LOG_DBG("SSR Consumer: Connection state slot %d %s", 
                ev->slot, ev->connected ? "connected" : "disconnected");
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int ssr_central_battery_listener(const zmk_event_t *eh) {
    const struct zmk_ssr_central_battery_state_changed *ev = 
        as_zmk_ssr_central_battery_state_changed(eh);
    if (ev) {
        LOG_DBG("SSR Consumer: Central battery %d%%", ev->battery_level);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int ssr_peripheral_battery_listener(const zmk_event_t *eh) {
    const struct zmk_ssr_peripheral_battery_state_changed *ev = 
        as_zmk_ssr_peripheral_battery_state_changed(eh);
    if (ev) {
        LOG_DBG("SSR Consumer: Peripheral battery slot %d %d%%", 
                ev->slot, ev->battery_level);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int ssr_layer_listener(const zmk_event_t *eh) {
    const struct zmk_ssr_central_layer_state_changed *ev = 
        as_zmk_ssr_central_layer_state_changed(eh);
    if (ev) {
        LOG_DBG("SSR Consumer: Layer %d (%s)", ev->layer, ev->layer_name);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int ssr_wpm_listener(const zmk_event_t *eh) {
    const struct zmk_ssr_central_wpm_state_changed *ev = 
        as_zmk_ssr_central_wpm_state_changed(eh);
    if (ev) {
        LOG_DBG("SSR Consumer: WPM %d", ev->wpm);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int ssr_transport_listener(const zmk_event_t *eh) {
    const struct zmk_ssr_central_transport_changed *ev = 
        as_zmk_ssr_central_transport_changed(eh);
    if (ev) {
        LOG_DBG("SSR Consumer: Transport %u", ev->transport);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int ssr_ble_profile_listener(const zmk_event_t *eh) {
    const struct zmk_ssr_central_ble_profile_changed *ev = 
        as_zmk_ssr_central_ble_profile_changed(eh);
    if (ev) {
        LOG_DBG("SSR Consumer: BLE profile %u connected:%d bonded:%d",
                ev->active_profile_index, ev->active_profile_connected, 
                ev->active_profile_bonded);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int ssr_central_usb_listener(const zmk_event_t *eh) {
    const struct zmk_ssr_central_usb_conn_state_changed *ev = 
        as_zmk_ssr_central_usb_conn_state_changed(eh);
    if (ev) {
        LOG_DBG("SSR Consumer: Central USB %s", ev->connected ? "connected" : "disconnected");
    }
    return ZMK_EV_EVENT_BUBBLE;
}

static int ssr_peripheral_usb_listener(const zmk_event_t *eh) {
    const struct zmk_ssr_peripheral_usb_conn_state_changed *ev = 
        as_zmk_ssr_peripheral_usb_conn_state_changed(eh);
    if (ev) {
        LOG_DBG("SSR Consumer: Peripheral USB slot %d %s", 
                ev->slot, ev->connected ? "connected" : "disconnected");
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(ssr_connection_listener, ssr_connection_listener);
ZMK_SUBSCRIPTION(ssr_connection_listener, zmk_ssr_peripheral_connection_state_changed);

ZMK_LISTENER(ssr_central_battery_listener, ssr_central_battery_listener);
ZMK_SUBSCRIPTION(ssr_central_battery_listener, zmk_ssr_central_battery_state_changed);

ZMK_LISTENER(ssr_peripheral_battery_listener, ssr_peripheral_battery_listener);
ZMK_SUBSCRIPTION(ssr_peripheral_battery_listener, zmk_ssr_peripheral_battery_state_changed);

ZMK_LISTENER(ssr_layer_listener, ssr_layer_listener);
ZMK_SUBSCRIPTION(ssr_layer_listener, zmk_ssr_central_layer_state_changed);

ZMK_LISTENER(ssr_wpm_listener, ssr_wpm_listener);
ZMK_SUBSCRIPTION(ssr_wpm_listener, zmk_ssr_central_wpm_state_changed);

ZMK_LISTENER(ssr_transport_listener, ssr_transport_listener);
ZMK_SUBSCRIPTION(ssr_transport_listener, zmk_ssr_central_transport_changed);

ZMK_LISTENER(ssr_ble_profile_listener, ssr_ble_profile_listener);
ZMK_SUBSCRIPTION(ssr_ble_profile_listener, zmk_ssr_central_ble_profile_changed);

ZMK_LISTENER(ssr_central_usb_listener, ssr_central_usb_listener);
ZMK_SUBSCRIPTION(ssr_central_usb_listener, zmk_ssr_central_usb_conn_state_changed);

ZMK_LISTENER(ssr_peripheral_usb_listener, ssr_peripheral_usb_listener);
ZMK_SUBSCRIPTION(ssr_peripheral_usb_listener, zmk_ssr_peripheral_usb_conn_state_changed);

#define SSR_CONSUMER_DEVICE_DEFINE(n)                                           \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL,                                  \
                          NULL, POST_KERNEL,                                    \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(SSR_CONSUMER_DEVICE_DEFINE)