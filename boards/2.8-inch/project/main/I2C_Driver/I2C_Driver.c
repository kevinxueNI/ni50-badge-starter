#include "I2C_Driver.h"

static const char *I2C_TAG = "I2C";

static i2c_master_bus_handle_t i2c_bus_handle = NULL;

/* Simple device handle cache: up to 4 devices */
#define MAX_I2C_DEVS 4
static struct {
    uint8_t addr;
    i2c_master_dev_handle_t dev;
} dev_cache[MAX_I2C_DEVS];
static int dev_cache_count = 0;

static i2c_master_dev_handle_t get_or_add_device(uint8_t addr)
{
    for (int i = 0; i < dev_cache_count; i++) {
        if (dev_cache[i].addr == addr) return dev_cache[i].dev;
    }
    if (dev_cache_count >= MAX_I2C_DEVS) {
        ESP_LOGE(I2C_TAG, "I2C device cache full");
        return NULL;
    }
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    i2c_master_dev_handle_t dev = NULL;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &dev));
    dev_cache[dev_cache_count].addr = addr;
    dev_cache[dev_cache_count].dev = dev;
    dev_cache_count++;
    return dev;
}

void I2C_Init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_handle));
    ESP_LOGI(I2C_TAG, "I2C initialized successfully");
}


// Reg addr is 8 bit
esp_err_t I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
{
    i2c_master_dev_handle_t dev = get_or_add_device(Driver_addr);
    if (!dev) return ESP_ERR_INVALID_STATE;

    uint8_t buf[Length + 1];
    buf[0] = Reg_addr;
    memcpy(&buf[1], Reg_data, Length);
    return i2c_master_transmit(dev, buf, Length + 1, I2C_MASTER_TIMEOUT_MS);
}



esp_err_t I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
    i2c_master_dev_handle_t dev = get_or_add_device(Driver_addr);
    if (!dev) return ESP_ERR_INVALID_STATE;

    return i2c_master_transmit_receive(dev, &Reg_addr, 1, Reg_data, Length, I2C_MASTER_TIMEOUT_MS);
}
