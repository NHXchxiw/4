#ifndef __ATTITUDE_CONTROL_H
#define __ATTITUDE_CONTROL_H

#include <stdint.h>

#define ATTITUDE_SAMPLE_HZ       1000u /* Nominal rate after wiring the control task. */
#define ATTITUDE_BETA_DEFAULT    0.051f
#define ATTITUDE_CALIB_SAMPLES   (2u * ATTITUDE_SAMPLE_HZ)
#define ATTITUDE_ACCEL_MIN_G     0.85f
#define ATTITUDE_ACCEL_MAX_G     1.15f
#define ATTITUDE_MAG_TIMEOUT_US  100000u
#define ATTITUDE_MAG_POLL_US     10000u /* 100 Hz polling for the 75 Hz HMC. */
#define ATTITUDE_MIN_DT_US       500u   /* Allow jitter around a 1 ms period. */
#define ATTITUDE_DEG_TO_RAD      0.017453292519943295f
#define ATTITUDE_RAD_TO_DEG      57.29577951308232f

/* Native MPU axes, right handed. Level az=+1 g gives roll=pitch=0.
 * q rotates body vectors into the reference frame (+Z up).
 * Positive angles follow the right hand rule; yaw=0 is magnetic north.
 * This is NOT NED/FRD: convert all three sensors together if NED is needed. */
typedef struct { float x, y, z; } Attitude_Vector3;
typedef struct { float w, x, y, z; } Attitude_Quaternion;
typedef struct { float roll, pitch, yaw; } Attitude_Euler;

typedef struct
{
    Attitude_Quaternion q;
    float beta;
} Madgwick_Filter;

/* Board-specific HMC-to-MPU alignment. Native axes match this board's
 * level yaw turn direction; mounting yaw and tilt still need validation. */
void Attitude_MapMag(const Attitude_Vector3 *sensor, Attitude_Vector3 *body);

void Madgwick_Init(Madgwick_Filter *filter, float beta);
/* Gyro: rad/s; accel: g (or any common unit); mag: any common unit.
 * mag==0 -> 6-axis IMU; invalid accel -> gyro only. dt is seconds. */
void Madgwick_Update(Madgwick_Filter *filter, const Attitude_Vector3 *gyro,
                     const Attitude_Vector3 *accel, const Attitude_Vector3 *mag, float dt);
void Attitude_QuaternionToEuler(const Attitude_Quaternion *q, Attitude_Euler *euler);

enum
{
    ATTITUDE_STATUS_MPU_OK      = 0x0001u,
    ATTITUDE_STATUS_MAG_OK      = 0x0002u,
    ATTITUDE_STATUS_CALIBRATED  = 0x0004u,
    ATTITUDE_STATUS_MAG_USED    = 0x0008u,
    ATTITUDE_STATUS_STATIONARY  = 0x0010u,
    ATTITUDE_STATUS_I2C_ERROR   = 0x0020u,
    ATTITUDE_STATUS_TIMING      = 0x0040u,
    ATTITUDE_STATUS_MAG_CAL     = 0x0080u,
    ATTITUDE_STATUS_MAG_CAL_OK  = 0x0100u,
    ATTITUDE_STATUS_READY       = 0x0200u
};

typedef struct
{
    Attitude_Quaternion q;
    Attitude_Euler angle;            /* degrees; yaw [-180,180] */
    Attitude_Vector3 gyro_dps, accel_g, mag_gauss, gyro_bias_dps;
    int16_t mag_raw[3];              /* HMC native axes, before calibration */
    float beta, dt;
    uint32_t samples, i2c_errors;
    uint16_t status, calibration_samples;
} Attitude_Data;

/* A single task owns acquisition/filter state. At 1 kHz, this should be the
 * control task. Other tasks copy snapshots under a short scheduler lock. */
uint8_t Attitude_Init(void);
uint8_t Attitude_Step(uint32_t now_us);
void Attitude_GetData(Attitude_Data *data);
void Attitude_SetBeta(float beta);
void Attitude_StartGyroCalibration(void);
void Attitude_StartMagCalibration(void);
uint8_t Attitude_FinishMagCalibration(void);
/* Native HMC units (counts), diagonal hard/soft iron correction in RAM. */
uint8_t Attitude_SetMagCalibration(const Attitude_Vector3 *offset, const Attitude_Vector3 *scale);
void Attitude_GetMagCalibration(Attitude_Vector3 *offset, Attitude_Vector3 *scale);

#endif
