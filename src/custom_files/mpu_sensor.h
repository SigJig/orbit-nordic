
#ifndef MPUSENSOR_H
#define MPUSENSOR_H

#include "mpu6050_registers.h"
#include <drivers/i2c.h>
#include <logging/log.h>
#include <nrf52840.h>
#include <nrfx_twim.h>
#include <zephyr.h>
#include <zephyr/irq.h>

/* Enum defining Accelerometer's Full Scale range posibillities in Gs. */
enum accel_range {
    AFS_2G = 0, // 2 G
    AFS_4G,     // 4 G
    AFS_8G,     // 8 G
    AFS_16G     // 16 G
};

/* Enum defining Gyroscope's Full Scale range posibillities in Degrees Pr
 * Second. */
enum gyro_range {
    GFS_250DPS = 0, // 250 deg/s
    GFS_500DPS,     // 500 deg/s
    GFS_1000DPS,    // 1000 deg/s
    GFS_2000DPS     // 2000 deg/s
};

/* MPU driver digital low pass fileter and external Frame Synchronization
 * (FSYNC) pin sampling configuration structure */
typedef struct {
    uint8_t dlpf_cfg : 3; // 3-bit unsigned value. Configures the Digital Low
                          // Pass Filter setting.
    uint8_t ext_sync_set : 3; // 3-bit unsigned value. Configures the external
                              // Frame Synchronization (FSYNC) pin sampling.
    uint8_t : 2;
} sync_dlpf_config_t;

/* MPU driver gyro configuration structure. */
typedef struct {

    uint8_t : 3;
    uint8_t fs_sel : 2; // FS_SEL 2-bit unsigned value. Selects the full scale
                        // range of gyroscopes.
    uint8_t : 3;
} gyro_config_t;

/* MPU driver accelerometer configuration structure. */
typedef struct {
    uint8_t : 3;
    uint8_t afs_sel : 2; // 2-bit unsigned value. Selects the full scale range
                         // of accelerometers.
    uint8_t za_st : 1; // When set to 1, the Z- Axis accelerometer performs self
                       // test.
    uint8_t ya_st : 1; // When set to 1, the Y- Axis accelerometer performs self
                       // test.
    uint8_t xa_st : 1; // When set to 1, the X- Axis accelerometer performs self
                       // test.
} accel_config_t;

/* MPU driver general configuration structure. */
typedef struct {
    uint8_t smplrt_div; // Divider from the gyroscope output rate used to
                        // generate the Sample Rate for the MPU-9150. Sample
                        // Rate = Gyroscope Output Rate / (1 + SMPLRT_DIV)
    sync_dlpf_config_t
        sync_dlpf_config; // Digital low pass fileter and external Frame
                          // Synchronization (FSYNC) configuration structure
    gyro_config_t gyro_config;   // Gyro configuration structure
    accel_config_t accel_config; // Accelerometer configuration structure
} app_mpu_config_t;

typedef struct accel_values_t {
    int16_t x, y, z;
} accel_values_t;

typedef struct gyro_values_t {
    int16_t x, y, z;
} gyro_values_t;

int mpu_sensor_init(void);
int read_accel_values(accel_values_t* p_accel_values);
int read_gyro_values(void);

#endif // MPUSENSOR_H