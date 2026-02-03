# ZMK Split Status Relay

This is a [ZMK](https://zmk.dev) module that relays status information across an [arbitrary split data channel](https://github.com/dmhuisma/zmk_arbitrary_split_data_channel). This allows peripheral devices to access information that would otherwise only be available on the central (battery level for all devices, current layer, etc...).

It currently implements the BLE split transport. Other transports (wire, ESB) are currently not implemented but it might be possible to do so by implementing them in the src/(type of transport) subdirectories.

The following information is relayed to all peripherals:

- Connection status of split devices
- Battery
- Highest active layer
- WPM
- Endpoint transport (WiFi, BLE)

# How to use

> [!NOTE]
> Note: this has not been tested on ZMK 0.4 yet.

Include this project on your ZMK's west manifest in config/west.yml. Also include the arbitrary split data channel module (it is a required dependency):

```diff
  [...]
  remotes:
+    - name: dmhuisma
+      url-base: https://github.com/dmhuisma
  projects:
+    - name: zmk_arbitrary_split_data_channel
+      remote: dmhuisma
+      revision: main
+    - name: zmk-split-status-relay
+      remote: dmhuisma
+      revision: main
  [...]
```

Add the node to your overlay file, including the ASDC node. Add it as a dependency in another module. An example of a consumer module is included in the "example_consumer" directory.

``` c
/{
    sdc0: split_data_channel {
        compatible = "zmk,arbitrary-split-data-channel";
        channel-id = <1>;
        status = "okay";
    };

    ssrc0: split_status_relay {
        compatible = "zmk,split-status-relay";
        asdc-channel = <&sdc0>;
        status = "okay";
    };

    ssrcc0: split_status_relay_consumer {
        compatible = "zmk,split-status-relay-consumer";
        ssrc = <&ssrc0>;
        status = "okay";
    };
}
```
