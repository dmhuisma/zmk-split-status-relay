
#define DT_DRV_COMPAT zmk_split_status_relay_central

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

// device configuration data
struct srcc_config {
    const struct device *asdc_channel;
};

// device runtime data
struct srcc_data {
    
};

static int srcc_init(const struct device *dev) {
    return 0;
}

//
// Define config structs for each instance
//

#define SSRC_CFG_DEFINE(n)                                                      \
    static const struct srcc_config config_##n = {                              \
        .asdc_channel = DEVICE_DT_GET(DT_INST_PHANDLE(n, asdc_channel))         \
    };

DT_INST_FOREACH_STATUS_OKAY(SSRC_CFG_DEFINE)

#define SSRC_DEVICE_DEFINE(n)                                                   \
    static struct srcc_data srcc_data_##n;                                      \
    DEVICE_DT_INST_DEFINE(n, srcc_init, NULL, &srcc_data_##n,                   \
                          &config_##n, POST_KERNEL,                             \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(SSRC_DEVICE_DEFINE)