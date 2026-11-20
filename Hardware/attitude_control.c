#include "attitude_control.h"
#include "MPU6050.h"
#include "HMC5883L.h"
#include <math.h>
#include <string.h>

/* Compile this entire file with --fpu=SoftVFP. The current OS port does not
 * preserve hardware FP context. Public raw/task interfaces use integers. */
static Madgwick_Filter Filter;
static Attitude_Data Data;
static Attitude_Vector3 GyroSum, GyroSquareSum, AccelSum, LastAccel;
static Attitude_Vector3 MagOffset, MagScale = {1.0f, 1.0f, 1.0f};
static Attitude_Vector3 MagMin, MagMax;
static uint32_t LastSampleUs, LastMagUs, MagCalCount;
static uint32_t LastMagPollUs;
static uint8_t HaveTime, HaveMag, Aligned, MagCalRunning;
static uint8_t HaveMagPollTime;
static uint16_t CalCount;

static uint8_t Valid(float value)
{
    return value == value && value > -1.0e20f && value < 1.0e20f;
}

static float NormSquared(const Attitude_Vector3 *v)
{
    return v->x*v->x + v->y*v->y + v->z*v->z;
}

static uint8_t NormalizeVector(Attitude_Vector3 *v)
{
    float n = NormSquared(v);
    if (!Valid(n) || n < 1.0e-12f) return 0u;
    n = 1.0f / sqrtf(n);
    v->x *= n; v->y *= n; v->z *= n;
    return 1u;
}

static uint8_t NormalizeQuaternion(Attitude_Quaternion *q)
{
    float n = q->w*q->w + q->x*q->x + q->y*q->y + q->z*q->z;
    if (!Valid(n) || n < 1.0e-12f) return 0u;
    n = 1.0f / sqrtf(n);
    q->w *= n; q->x *= n; q->y *= n; q->z *= n;
    return 1u;
}

void Attitude_MapMag(const Attitude_Vector3 *sensor, Attitude_Vector3 *body)
{
    /* This board's four-position level test requires the native XY
     * handedness: swapping X/Y and negating Z reversed magnetic yaw
     * relative to the MPU gyro. Use native axes as the candidate mapping.
     * Level turns do not establish the mounting yaw offset or tilt
     * alignment; verify roll/pitch tilt before relying on 3-D heading. */
    body->x = sensor->x;
    body->y = sensor->y;
    body->z = sensor->z;
}

void Madgwick_Init(Madgwick_Filter *filter, float beta)
{
    if (filter == 0) return;
    filter->q.w = 1.0f;
    filter->q.x = filter->q.y = filter->q.z = 0.0f;
    filter->beta = Valid(beta) && beta >= 0.0f && beta <= 1.0f ? beta : ATTITUDE_BETA_DEFAULT;
}

/* J(q)^T f(q), using the same objective as the expanded Madgwick MARG
 * equations. Reference vector is (bx,0,bz); no approximate inverse sqrt. */
static void AddGradient(const Attitude_Quaternion *q, float bx, float bz,
                        const Attitude_Vector3 *measured, Attitude_Quaternion *s)
{
    float w = q->w, x = q->x, y = q->y, z = q->z;
    float fx = bx*(1.0f-2.0f*(y*y+z*z)) + 2.0f*bz*(x*z-w*y) - measured->x;
    float fy = 2.0f*bx*(x*y-w*z) + 2.0f*bz*(y*z+w*x) - measured->y;
    float fz = 2.0f*bx*(x*z+w*y) + bz*(1.0f-2.0f*(x*x+y*y)) - measured->z;
    s->w += (-2.0f*bz*y)*fx + (-2.0f*bx*z+2.0f*bz*x)*fy + (2.0f*bx*y)*fz;
    s->x += (2.0f*bz*z)*fx + (2.0f*bx*y+2.0f*bz*w)*fy + (2.0f*bx*z-4.0f*bz*x)*fz;
    s->y += (-4.0f*bx*y-2.0f*bz*w)*fx + (2.0f*bx*x+2.0f*bz*z)*fy
          + (2.0f*bx*w-4.0f*bz*y)*fz;
    s->z += (-4.0f*bx*z+2.0f*bz*x)*fx + (-2.0f*bx*w+2.0f*bz*y)*fy + (2.0f*bx*x)*fz;
}

void Madgwick_Update(Madgwick_Filter *filter, const Attitude_Vector3 *gyro,
                     const Attitude_Vector3 *accel, const Attitude_Vector3 *mag, float dt)
{
    Attitude_Quaternion q, derivative, step = {0.0f, 0.0f, 0.0f, 0.0f};
    Attitude_Vector3 a, m;
    float hx, hy, hz, bx, n;
    if (filter == 0 || gyro == 0 || !Valid(dt) || dt <= 0.0f || dt > 0.05f
        || !Valid(gyro->x) || !Valid(gyro->y) || !Valid(gyro->z)) return;
    q = filter->q;
    if (!NormalizeQuaternion(&q)) { Madgwick_Init(filter, filter->beta); q = filter->q; }
    derivative.w = 0.5f*(-q.x*gyro->x-q.y*gyro->y-q.z*gyro->z);
    derivative.x = 0.5f*( q.w*gyro->x+q.y*gyro->z-q.z*gyro->y);
    derivative.y = 0.5f*( q.w*gyro->y-q.x*gyro->z+q.z*gyro->x);
    derivative.z = 0.5f*( q.w*gyro->z+q.x*gyro->y-q.y*gyro->x);
    if (accel != 0)
    {
        a = *accel;
        if (NormalizeVector(&a))
        {
            AddGradient(&q, 0.0f, 1.0f, &a, &step);
            if (mag != 0)
            {
                m = *mag;
                if (NormalizeVector(&m))
                {
                    /* Rotate measured magnetic field into the earth frame. */
                    hx = (1.0f-2.0f*(q.y*q.y+q.z*q.z))*m.x
                       + 2.0f*(q.x*q.y-q.w*q.z)*m.y + 2.0f*(q.x*q.z+q.w*q.y)*m.z;
                    hy = 2.0f*(q.x*q.y+q.w*q.z)*m.x
                       + (1.0f-2.0f*(q.x*q.x+q.z*q.z))*m.y + 2.0f*(q.y*q.z-q.w*q.x)*m.z;
                    hz = 2.0f*(q.x*q.z-q.w*q.y)*m.x + 2.0f*(q.y*q.z+q.w*q.x)*m.y
                       + (1.0f-2.0f*(q.x*q.x+q.y*q.y))*m.z;
                    bx = sqrtf(hx*hx+hy*hy);
                    if (bx > 0.05f) AddGradient(&q, bx, hz, &m, &step);
                }
            }
            n = step.w*step.w + step.x*step.x + step.y*step.y + step.z*step.z;
            /* At exact alignment gradient=0. Never divide by zero. */
            if (Valid(n) && n > 1.0e-12f)
            {
                n = filter->beta / sqrtf(n);
                derivative.w -= n*step.w; derivative.x -= n*step.x;
                derivative.y -= n*step.y; derivative.z -= n*step.z;
            }
        }
    }
    q.w += derivative.w*dt; q.x += derivative.x*dt;
    q.y += derivative.y*dt; q.z += derivative.z*dt;
    if (NormalizeQuaternion(&q)) filter->q = q;
}

void Attitude_QuaternionToEuler(const Attitude_Quaternion *q, Attitude_Euler *euler)
{
    float sin_pitch;
    if (q == 0 || euler == 0) return;
    sin_pitch = 2.0f*(q->w*q->y-q->z*q->x);
    if (sin_pitch > 1.0f) sin_pitch = 1.0f;
    if (sin_pitch < -1.0f) sin_pitch = -1.0f;
    euler->roll = atan2f(2.0f*(q->w*q->x+q->y*q->z),
                           1.0f-2.0f*(q->x*q->x+q->y*q->y))*ATTITUDE_RAD_TO_DEG;
    euler->pitch = asinf(sin_pitch)*ATTITUDE_RAD_TO_DEG;
    euler->yaw = atan2f(2.0f*(q->w*q->z+q->x*q->y),
                          1.0f-2.0f*(q->y*q->y+q->z*q->z))*ATTITUDE_RAD_TO_DEG;
}

//校准完成时，用加速度和磁力计直接沟造初始四元数加速收敛
static void Align(const Attitude_Vector3 *a, const Attitude_Vector3 *m)
{
    float roll = atan2f(a->y, a->z);
    float pitch = atan2f(-a->x, sqrtf(a->y*a->y+a->z*a->z));
    float yaw = 0.0f, mx, my, cr, sr, cp, sp, cy, sy;
    if (m != 0)
    {
        mx = m->x*cosf(pitch) + m->y*sinf(roll)*sinf(pitch) + m->z*cosf(roll)*sinf(pitch);
        my = m->y*cosf(roll) - m->z*sinf(roll);
        if (mx*mx+my*my > 1.0e-8f) yaw = atan2f(-my, mx);
    }
    cr = cosf(roll*0.5f); sr = sinf(roll*0.5f);
    cp = cosf(pitch*0.5f); sp = sinf(pitch*0.5f);
    cy = cosf(yaw*0.5f); sy = sinf(yaw*0.5f);
    Filter.q.w = cr*cp*cy+sr*sp*sy;
    Filter.q.x = sr*cp*cy-cr*sp*sy;
    Filter.q.y = cr*sp*cy+sr*cp*sy;
    Filter.q.z = cr*cp*sy-sr*sp*cy;
    (void)NormalizeQuaternion(&Filter.q);
}

void Attitude_SetBeta(float beta)
{
    if (Valid(beta) && beta >= 0.0f && beta <= 1.0f) Filter.beta = beta;
    Data.beta = Filter.beta;
}


void Attitude_StartGyroCalibration(void)
{
    CalCount = 0u;
    memset(&GyroSum, 0, sizeof(GyroSum));
    memset(&GyroSquareSum, 0, sizeof(GyroSquareSum));
    memset(&AccelSum, 0, sizeof(AccelSum));
    Data.calibration_samples = 0u;
    Data.status &= ~(ATTITUDE_STATUS_CALIBRATED | ATTITUDE_STATUS_READY);
    Aligned = 0u;
}

uint8_t Attitude_Init(void)
{
    memset(&Data, 0, sizeof(Data));
    HaveTime = HaveMag = Aligned = MagCalRunning = 0u;
    HaveMagPollTime = 0u;
    LastMagPollUs = 0u;
    MagOffset.x = MagOffset.y = MagOffset.z = 0.0f;
    MagScale.x = MagScale.y = MagScale.z = 1.0f;
    Madgwick_Init(&Filter, ATTITUDE_BETA_DEFAULT);
    Data.q = Filter.q;
    Data.beta = Filter.beta;
    Attitude_StartGyroCalibration();
    if (!MPU6050_InitChecked()) { Data.status |= ATTITUDE_STATUS_I2C_ERROR; return 0u; }
    Data.status |= ATTITUDE_STATUS_MPU_OK;
    if (HMC5883L_Init()) Data.status |= ATTITUDE_STATUS_MAG_OK;
    return 1u;
}

void Attitude_StartMagCalibration(void)
{
    MagMin.x = MagMin.y = MagMin.z = 32767.0f;
    MagMax.x = MagMax.y = MagMax.z = -32768.0f;
    MagCalCount = 0u;
    MagCalRunning = 1u;
    Data.status &= ~ATTITUDE_STATUS_MAG_CAL_OK;
    Data.status |= ATTITUDE_STATUS_MAG_CAL;
}

uint8_t Attitude_SetMagCalibration(const Attitude_Vector3 *offset, const Attitude_Vector3 *scale)
{
    if (offset == 0 || scale == 0 || !Valid(offset->x) || !Valid(offset->y) || !Valid(offset->z)
        || !Valid(scale->x) || !Valid(scale->y) || !Valid(scale->z)
        || scale->x < 0.1f || scale->x > 10.0f || scale->y < 0.1f || scale->y > 10.0f
        || scale->z < 0.1f || scale->z > 10.0f) return 0u;
    MagOffset = *offset; MagScale = *scale;
    HaveMag = 0u; /* Do not fuse a cached sample using the previous calibration. */
    Data.status |= ATTITUDE_STATUS_MAG_CAL_OK;
    return 1u;
}

uint8_t Attitude_FinishMagCalibration(void)
{
    Attitude_Vector3 radius, offset, scale;
    float mean;
    MagCalRunning = 0u;
    Data.status &= ~ATTITUDE_STATUS_MAG_CAL;
    radius.x = (MagMax.x-MagMin.x)*0.5f;
    radius.y = (MagMax.y-MagMin.y)*0.5f;
    radius.z = (MagMax.z-MagMin.z)*0.5f;
    /* Require enough distinct conversions and real coverage of all 3 axes. */
    if (MagCalCount < 300u || radius.x < 100.0f || radius.y < 100.0f || radius.z < 100.0f) return 0u;
    mean = (radius.x+radius.y+radius.z)/3.0f;
    offset.x = (MagMax.x+MagMin.x)*0.5f;
    offset.y = (MagMax.y+MagMin.y)*0.5f;
    offset.z = (MagMax.z+MagMin.z)*0.5f;
    scale.x = mean/radius.x; scale.y = mean/radius.y; scale.z = mean/radius.z;
    return Attitude_SetMagCalibration(&offset, &scale);
}

void Attitude_GetMagCalibration(Attitude_Vector3 *offset, Attitude_Vector3 *scale)
{
    if (offset != 0) *offset = MagOffset;
    if (scale != 0) *scale = MagScale;
}

static void ReadMag(uint32_t now)
{
    HMC5883L_RawData raw;
    Attitude_Vector3 sensor;
    int8_t result;
    if ((Data.status & ATTITUDE_STATUS_MAG_OK) == 0u) return;
    if (HaveMagPollTime && (uint32_t)(now-LastMagPollUs) < ATTITUDE_MAG_POLL_US) return;
    LastMagPollUs = now;
    HaveMagPollTime = 1u;
    result = HMC5883L_ReadRaw(&raw);
    if (result < 0)
    {
        HaveMag = 0u;
        Data.i2c_errors++;
        Data.status |= ATTITUDE_STATUS_I2C_ERROR;
    }
    if (result != 1) return;
    Data.mag_raw[0] = raw.x; Data.mag_raw[1] = raw.y; Data.mag_raw[2] = raw.z;
    if (MagCalRunning)
    {
        if (raw.x < MagMin.x) MagMin.x = raw.x;
        if (raw.y < MagMin.y) MagMin.y = raw.y;
        if (raw.z < MagMin.z) MagMin.z = raw.z;
        if (raw.x > MagMax.x) MagMax.x = raw.x;
        if (raw.y > MagMax.y) MagMax.y = raw.y;
        if (raw.z > MagMax.z) MagMax.z = raw.z;
        MagCalCount++;
    }
    sensor.x = ((float)raw.x-MagOffset.x)*MagScale.x/HMC5883L_LSB_PER_GAUSS;
    sensor.y = ((float)raw.y-MagOffset.y)*MagScale.y/HMC5883L_LSB_PER_GAUSS;
    sensor.z = ((float)raw.z-MagOffset.z)*MagScale.z/HMC5883L_LSB_PER_GAUSS;
    Attitude_MapMag(&sensor, &Data.mag_gauss);
    LastMagUs = now;
    HaveMag = 1u;
}

//陀螺仪零偏校准
static void CalibrateGyro(const Attitude_Vector3 *gyro, const Attitude_Vector3 *accel)
{
    Attitude_Vector3 delta;
    float n = NormSquared(accel), inv, variance;
    delta.x = accel->x-LastAccel.x; delta.y = accel->y-LastAccel.y; delta.z = accel->z-LastAccel.z;
    LastAccel = *accel;
    /* The limits are magnitudes in g; n is the squared magnitude.
     * Allow uncalibrated accel offset/scale here, as in the fusion gate.
     * Motion is still rejected by the delta and gyro variance checks. */
    if (!Valid(n) || n < ATTITUDE_ACCEL_MIN_G*ATTITUDE_ACCEL_MIN_G
        || n > ATTITUDE_ACCEL_MAX_G*ATTITUDE_ACCEL_MAX_G || NormSquared(gyro) > 400.0f
        || (CalCount != 0u && NormSquared(&delta) > 0.0025f))
    {
        Attitude_StartGyroCalibration();
        return;
    }
    GyroSum.x += gyro->x; 
    GyroSum.y += gyro->y; 
    GyroSum.z += gyro->z;
    GyroSquareSum.x += gyro->x*gyro->x;
    GyroSquareSum.y += gyro->y*gyro->y;
    GyroSquareSum.z += gyro->z*gyro->z;
    AccelSum.x += accel->x; 
    AccelSum.y += accel->y; 
    AccelSum.z += accel->z;
    Data.calibration_samples = ++CalCount;
    if (CalCount < ATTITUDE_CALIB_SAMPLES) return;
    inv = 1.0f/(float)CalCount;
    variance = GyroSquareSum.x*inv-GyroSum.x*inv*GyroSum.x*inv
             + GyroSquareSum.y*inv-GyroSum.y*inv*GyroSum.y*inv
             + GyroSquareSum.z*inv-GyroSum.z*inv*GyroSum.z*inv;
    if (variance > 0.75f) { Attitude_StartGyroCalibration(); return; }
    Data.gyro_bias_dps.x = GyroSum.x*inv;
    Data.gyro_bias_dps.y = GyroSum.y*inv;
    Data.gyro_bias_dps.z = GyroSum.z*inv;
    Data.status |= ATTITUDE_STATUS_CALIBRATED;
    Align(&AccelSum, HaveMag && !MagCalRunning ? &Data.mag_gauss : 0);
    Aligned = 1u;
}

uint8_t Attitude_Step(uint32_t now_us)
{
    MPU6050_RawData raw;
    Attitude_Vector3 gyro, corrected;
    const Attitude_Vector3 *mag = 0, *accel = 0;
    uint32_t elapsed;
    float n;
    Data.status &= ~(ATTITUDE_STATUS_MAG_USED | ATTITUDE_STATUS_STATIONARY
                   | ATTITUDE_STATUS_I2C_ERROR | ATTITUDE_STATUS_TIMING);
    if (!MPU6050_ReadRaw(&raw))
    {
        Data.status &= ~(ATTITUDE_STATUS_MPU_OK | ATTITUDE_STATUS_READY);
        Data.status |= ATTITUDE_STATUS_I2C_ERROR;
        Data.i2c_errors++;
        return 0u;
    }
    Data.status |= ATTITUDE_STATUS_MPU_OK;
    Data.samples++;
    Data.accel_g.x = (float)raw.ax/MPU6050_ACCEL_LSB_PER_G;
    Data.accel_g.y = (float)raw.ay/MPU6050_ACCEL_LSB_PER_G;
    Data.accel_g.z = (float)raw.az/MPU6050_ACCEL_LSB_PER_G;
    gyro.x = (float)raw.gx/MPU6050_GYRO_LSB_PER_DPS;
    gyro.y = (float)raw.gy/MPU6050_GYRO_LSB_PER_DPS;
    gyro.z = (float)raw.gz/MPU6050_GYRO_LSB_PER_DPS;
    ReadMag(now_us);
    if (HaveMag && (uint32_t)(now_us-LastMagUs) > ATTITUDE_MAG_TIMEOUT_US) HaveMag = 0u;
    if (HaveMag && !MagCalRunning)
    {
        n = NormSquared(&Data.mag_gauss);
        /* Broad geomagnetic magnitude gate (0.1..1.0 gauss). */
        if (n > 0.01f && n < 1.0f) mag = &Data.mag_gauss;
    }
    if ((Data.status & ATTITUDE_STATUS_CALIBRATED) == 0u) CalibrateGyro(&gyro, &Data.accel_g);
    Data.gyro_dps.x = gyro.x-Data.gyro_bias_dps.x;
    Data.gyro_dps.y = gyro.y-Data.gyro_bias_dps.y;
    Data.gyro_dps.z = gyro.z-Data.gyro_bias_dps.z;
    corrected.x = Data.gyro_dps.x*ATTITUDE_DEG_TO_RAD;
    corrected.y = Data.gyro_dps.y*ATTITUDE_DEG_TO_RAD;
    corrected.z = Data.gyro_dps.z*ATTITUDE_DEG_TO_RAD;
    elapsed = HaveTime ? (uint32_t)(now_us-LastSampleUs) : 0u;
    LastSampleUs = now_us; HaveTime = 1u;
    Data.dt = (float)elapsed*0.000001f;
    if (Aligned && elapsed >= ATTITUDE_MIN_DT_US && elapsed <= 20000u)
    {
        n = NormSquared(&Data.accel_g);
        /* Large linear acceleration must not be interpreted as gravity. */
        if (n > ATTITUDE_ACCEL_MIN_G*ATTITUDE_ACCEL_MIN_G
            && n < ATTITUDE_ACCEL_MAX_G*ATTITUDE_ACCEL_MAX_G) accel = &Data.accel_g;
        if (n > 0.94f && n < 1.06f && NormSquared(&Data.gyro_dps) < 0.64f)
            Data.status |= ATTITUDE_STATUS_STATIONARY;
        Madgwick_Update(&Filter, &corrected, accel, mag, Data.dt);
        Data.status |= ATTITUDE_STATUS_READY;
        if (mag != 0 && accel != 0) Data.status |= ATTITUDE_STATUS_MAG_USED;
    }
    else if (Aligned)
    {
        Data.status &= ~ATTITUDE_STATUS_READY;
        Data.status |= ATTITUDE_STATUS_TIMING;
    }
    Data.q = Filter.q;
    Attitude_QuaternionToEuler(&Data.q, &Data.angle);
    return 1u;
}

void Attitude_GetData(Attitude_Data *data)
{
    if (data != 0) *data = Data;
}
