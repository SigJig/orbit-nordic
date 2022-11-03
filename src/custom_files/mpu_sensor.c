
#include "mpu_sensor.h"
#include "../common.h"

#define LOG_MODULE_NAME mpu_sensor
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

//#define NRFX_TWI0_ENABLED (TWI0_ENABLED && !TWI0_USE_EASY_DMA)

static const nrfx_twim_t m_twim_instance = NRFX_TWIM_INSTANCE(0);
static const nrfx_twim_config_t m_twim_config = {
    // These particular gpios are used because they are close to both VDD
    // and GND.
    .scl = 4,
    .sda = 3,
    .frequency = NRF_TWIM_FREQ_400K,
    .interrupt_priority = NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY,
    .hold_bus_uninit = false,
};

#define MPU_TWI_BUFFER_SIZE 14
#define MPU_TWI_TIMEOUT 10000
#define MPU_ADDRESS 0x68

static volatile uint8_t twi_xfer_done = false;
static uint8_t twi_tx_buffer[MPU_TWI_BUFFER_SIZE];

static int
app_mpu_tx(
    const nrfx_twim_t* p_instance,
    uint8_t address,
    uint8_t* p_data,
    uint8_t length,
    bool no_stop)
{
    int err;

    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TX(
        address, p_data, length);
    err = nrfx_twim_xfer(p_instance, &xfer, 0);
    if (err != NRFX_SUCCESS) {
        return err;
    }

    return 0;
}

static int
app_mpu_rx(
    const nrfx_twim_t* p_instance,
    uint8_t address,
    uint8_t* p_data,
    uint8_t length)
{
    int err;
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_RX(
        address, p_data, length);

    err = nrfx_twim_xfer(p_instance, &xfer, 0);
    if (err != NRFX_SUCCESS) {
        return err;
    }
    return 0;
}

static int
wait_for_xfer_done(void)
{
    int timeout = MPU_TWI_TIMEOUT;
    while ((!twi_xfer_done) && --timeout) {
        // Wait...
    }
    if (timeout == 0) {
        return NRFX_ERROR_TIMEOUT;
    }
    return 0;
}

static int
app_mpu_write_single_register(uint8_t reg, uint8_t data)
{
    int err;

    uint8_t packet[2] = {reg, data};

    twi_xfer_done = false; // reset for new xfer
    err = app_mpu_tx(&m_twim_instance, MPU_ADDRESS, packet, 2, false);
    if (err) {
        return err;
    }
    err = wait_for_xfer_done();
    if (err == NRFX_ERROR_TIMEOUT) {
        return err;
    }

    return 0;
}

static int
app_mpu_write_registers(uint8_t reg, uint8_t* p_data, uint8_t length)
{
    int err;

    twi_tx_buffer[0] = reg;
    memcpy((twi_tx_buffer + 1), p_data, length);

    nrfx_twim_xfer_desc_t xfer = {0};
    xfer.address = MPU_ADDRESS;
    xfer.type = NRFX_TWIM_XFER_TX;
    xfer.primary_length = length + 1;
    xfer.p_primary_buf = twi_tx_buffer;

    twi_xfer_done = false; // reset for new xfer
    err = nrfx_twim_xfer(&m_twim_instance, &xfer, 0);
    if (err != NRFX_SUCCESS) {
        return err;
    }
    err = wait_for_xfer_done();
    if (err == NRFX_ERROR_TIMEOUT) {
        return err;
    }

    return 0;
}

static int
app_mpu_read_registers(uint8_t reg, uint8_t* p_data, uint8_t length)
{
    int err;

    twi_xfer_done = false; // reset for new xfer
    err = app_mpu_tx(&m_twim_instance, MPU_ADDRESS, &reg, 1, false);
    if (err) {
        return err;
    }
    err = wait_for_xfer_done();
    if (err == NRFX_ERROR_TIMEOUT) {
        return err;
    }

    twi_xfer_done = false; // reset for new xfer
    err = app_mpu_rx(&m_twim_instance, MPU_ADDRESS, p_data, length);
    if (err) {
        LOG_ERR("app_mpu_rx returned %08x", err);
        return err;
    }
    err = wait_for_xfer_done();
    if (err == NRFX_ERROR_TIMEOUT) {
        return err;
    }

    return 0;
}

static const char*
twim_err_to_string(nrfx_twim_evt_type_t type)
{
    switch (type) {
    case NRFX_TWIM_EVT_DONE:
        return "NRFX_TWIM_EVT_DONE";
    case NRFX_TWIM_EVT_ADDRESS_NACK:
        return "NRFX_TWIM_EVT_ADDRESS_NACK";
    case NRFX_TWIM_EVT_DATA_NACK:
        return "NRFX_TWIM_EVT_DATA_NACK";
    case NRFX_TWIM_EVT_OVERRUN:
        return "NRFX_TWIM_EVT_OVERRUN";
    case NRFX_TWIM_EVT_BUS_ERROR:
        return "NRFX_TWIM_EVT_BUS_ERROR";
    }

    return "UNKNOWN";
}

static void
twim_handler(const nrfx_twim_evt_t* p_event, void* p_context)
{
    LOG_INF("twim callback \n");

    switch (p_event->type) {
    case NRFX_TWIM_EVT_DONE: {
        twi_xfer_done = 1;
        break;
    }
    default: {
        LOG_ERR("twim handler error: %s\n", twim_err_to_string(p_event->type));
        break;
    }
    }
}

static int
twi_init(void)
{
    IRQ_CONNECT(
        DT_IRQN(DT_NODELABEL(i2c0)),
        DT_IRQ(DT_NODELABEL(i2c0), priority),
        nrfx_isr,
        nrfx_twim_0_irq_handler,
        0);
    irq_enable(DT_IRQN(DT_NODELABEL(i2c0)));

    int err = nrfx_twim_init(
        &m_twim_instance, &m_twim_config, twim_handler, NULL);

    if (err != NRFX_SUCCESS) {
        LOG_ERR("twim init failed (%i)\n", err);

        return err;
    }

    nrfx_twim_enable(&m_twim_instance);

    return 0;
}

static int
app_mpu_config(void)
{
    app_mpu_config_t mpu_config = {
        .smplrt_div = 19,
        .sync_dlpf_config.dlpf_cfg = 1,
        .sync_dlpf_config.ext_sync_set = 0,
        .gyro_config.fs_sel = GFS_2000DPS,
        .accel_config.afs_sel = AFS_2G,
        .accel_config.za_st = 0,
        .accel_config.ya_st = 0,
        .accel_config.xa_st = 0,
    };

    uint8_t* data = (uint8_t*)&mpu_config;

    app_mpu_write_registers(MPU_REG_SMPLRT_DIV, data, sizeof(data));

    return 0;
}

int
mpu_sensor_init(void)
{
    LOG_INF("initializing mpu sensor \n");

    int err = twi_init();

    if (err) {
        LOG_ERR(INIT_UNABLE, "mpu sensor");
        LOG_INF("status code %i\n", err);

        return err;
    }

    if ((err = app_mpu_write_single_register(
             MPU_REG_SIGNAL_PATH_RESET, 0x07))) {

        LOG_ERR("mpu reset failed with code %i\n", err);
    }

    if ((err = app_mpu_write_single_register(MPU_REG_PWR_MGMT_1, 0x1))) {
        LOG_ERR("mpu pwr mgmt config failed with code %i\n", err);
    }

    return app_mpu_config();
}

int
read_accel_values(accel_values_t* p_accel_values)
{
    int err;
    uint8_t raw[6];

    err = app_mpu_read_registers(MPU_REG_ACCEL_XOUT_H, raw, sizeof(raw));

    if (err) {
        LOG_ERR("could not read accellerometer data %d\n", err);

        return err;
    }

    p_accel_values->x = ((raw[0] << 8) + raw[1]);
    p_accel_values->y = ((raw[2] << 8) + raw[3]);
    p_accel_values->z = ((raw[4] << 8) + raw[5]);

    return 0;
}

int
read_gyro_values(void)
{
    return 0;
}