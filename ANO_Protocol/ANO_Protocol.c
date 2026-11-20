#include "ANO_Protocol.h"
#include "USART.h"
#include "MPU6050.h"
#include "ucos_ii.h"

#define ANO_FRAME_HEAD              0xAAu
#define ANO_MAX_DATA_LEN            40u
#define ANO_RX_BUFFER_SIZE          256u
#define ANO_BULK_READ_MAX           256u
#define ANO_READS_PER_TASK          2u
#define ANO_MAG_CAL_TIMEOUT_US      30000000u
#define ANO_RX_FRAME_TIMEOUT_US     100000u
#define ANO_INVALID_PARAM_VALUE     (-2147483647L - 1L)

volatile ANO_PID_Config ANO_PID_Groups[3] =
{
    {0u, 0u, 0u},
    {0u, 0u, 0u},
    {0u, 0u, 0u}
};

volatile ANO_Protocol_Diagnostics ANO_Protocol_Debug;

static volatile uint8_t RxBuffer[ANO_RX_BUFFER_SIZE];
static volatile uint8_t RxWrite;
static volatile uint8_t RxRead;
static volatile uint8_t RxDiscardPending;
static uint8_t RxFrame[ANO_MAX_DATA_LEN + 6u];
static uint8_t RxFrameLength;
static uint32_t RxLastByteUs;
static uint16_t PendingReadId;
static uint16_t PendingReadCount;
static uint8_t MagCalibrationActive;
static uint32_t MagCalibrationStartUs;

static void CalculateChecksum(const uint8_t *bytes, uint8_t length,
                              uint8_t *sum, uint8_t *add)
{
    uint8_t i;
    *sum = 0u;
    *add = 0u;
    for (i = 0u; i < length; i++)
    {
        *sum = (uint8_t)(*sum + bytes[i]);
        *add = (uint8_t)(*add + *sum);
    }
}

static uint16_t ReadU16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

static int32_t ReadS32(const uint8_t *data)
{
    uint32_t value = (uint32_t)data[0]
                   | ((uint32_t)data[1] << 8)
                   | ((uint32_t)data[2] << 16)
                   | ((uint32_t)data[3] << 24);
    return (int32_t)value;
}

static void WriteU16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
}

static void WriteS32(uint8_t *data, int32_t value)
{
    uint32_t bits = (uint32_t)value;
    data[0] = (uint8_t)bits;
    data[1] = (uint8_t)(bits >> 8);
    data[2] = (uint8_t)(bits >> 16);
    data[3] = (uint8_t)(bits >> 24);
}

static void SendFrame(uint8_t address, uint8_t id,
                      const uint8_t *payload, uint8_t length)
{
    uint8_t frame[ANO_MAX_DATA_LEN + 6u];
    uint8_t sum;
    uint8_t add;
    uint8_t i;
    uint8_t frame_length;

    if (length > ANO_MAX_DATA_LEN) return;
    frame[0] = ANO_FRAME_HEAD;
    frame[1] = address;
    frame[2] = id;
    frame[3] = length;
    for (i = 0u; i < length; i++) frame[4u + i] = payload[i];

    frame_length = (uint8_t)(4u + length);
    CalculateChecksum(frame, frame_length, &sum, &add);
    frame[frame_length] = sum;
    frame[frame_length + 1u] = add;

    if (Serial_SendBuffer(frame, (uint16_t)(frame_length + 2u)))
        ANO_Protocol_Debug.tx_frames++;
    else
        ANO_Protocol_Debug.tx_errors++;
}

static uint8_t GetPIDPointer(uint16_t id, volatile uint16_t **value)
{
    uint16_t index;
    uint8_t group;
    uint8_t component;

    if (id < ANO_PARAM_PID_1_P || id > ANO_PARAM_PID_3_D) return 0u;
    index = (uint16_t)(id - ANO_PARAM_PID_1_P);
    group = (uint8_t)(index / 3u);
    component = (uint8_t)(index % 3u);
    if (component == 0u) *value = &ANO_PID_Groups[group].p;
    else if (component == 1u) *value = &ANO_PID_Groups[group].i;
    else *value = &ANO_PID_Groups[group].d;
    return 1u;
}

static int32_t ReadParameter(uint16_t id)
{
    volatile uint16_t *value;
    if (!GetPIDPointer(id, &value)) return (int32_t)ANO_INVALID_PARAM_VALUE;
    return (int32_t)(*value);
}

static uint8_t WriteParameter(uint16_t id, int32_t new_value)
{
    volatile uint16_t *value;
    if (new_value < 0 || new_value > 65535L) return 0u;
    if (!GetPIDPointer(id, &value)) return 0u;
    *value = (uint16_t)new_value;
    return 1u;
}

static void SendParameter(uint16_t id)
{
    uint8_t payload[6];
    WriteU16(payload, id);
    WriteS32(&payload[2], ReadParameter(id));
    SendFrame(ANO_HOST_ADDRESS, 0xE2u, payload, 6u);
}

static void SendAck(uint8_t received_id, uint8_t sum, uint8_t add)
{
    uint8_t payload[3];
    payload[0] = received_id;
    payload[1] = sum;
    payload[2] = add;
    SendFrame(ANO_BROADCAST_ADDRESS, 0x00u, payload, 3u);
}

static void HandleParameterRead(const uint8_t *payload, uint8_t length)
{
    if (length == 2u)
    {
        SendParameter(ReadU16(payload));
    }
    else if (length == 4u)
    {
        PendingReadId = ReadU16(payload);
        PendingReadCount = ReadU16(&payload[2]);
        if (PendingReadCount > ANO_BULK_READ_MAX)
            PendingReadCount = ANO_BULK_READ_MAX;
    }
}

static void HandleParameterWrite(const uint8_t *frame, uint8_t length)
{
    const uint8_t *payload = &frame[4];
    uint16_t first_id;
    uint16_t count;
    uint16_t i;

    if (length < 6u || ((length - 2u) & 3u) != 0u) return;
    first_id = ReadU16(payload);
    count = (uint16_t)((length - 2u) / 4u);
    for (i = 0u; i < count; i++)
        (void)WriteParameter((uint16_t)(first_id + i), ReadS32(&payload[2u + i * 4u]));
}

static void HandleCommand(const uint8_t *payload, uint8_t length, uint32_t now_us)
{
    uint8_t cid;
    uint8_t command;

    if (length != 11u) return;
    cid = payload[0];
    if (cid != 0x01u || payload[1] != 0x00u) return;
    command = payload[2];

    if (command == 0x02u)
    {
        Attitude_StartGyroCalibration();
    }
    else if (command == 0x04u)
    {
        /* Host retries must not toggle calibration off. Collect for 30 s. */
        if (!MagCalibrationActive)
        {
            Attitude_StartMagCalibration();
            MagCalibrationStartUs = now_us;
            MagCalibrationActive = 1u;
        }
    }
}

static uint8_t HandleFrame(uint8_t *frame, uint8_t total_length, uint32_t now_us)
{
    uint8_t length = frame[3];
    uint8_t sum;
    uint8_t add;
    uint8_t sum_index = (uint8_t)(4u + length);

    if (total_length != (uint8_t)(length + 6u)) return 0u;
    CalculateChecksum(frame, sum_index, &sum, &add);
    if (sum != frame[sum_index] || add != frame[sum_index + 1u])
    {
        ANO_Protocol_Debug.checksum_errors++;
        return 0u;
    }
    if (frame[1] != ANO_DEVICE_ADDRESS && frame[1] != ANO_BROADCAST_ADDRESS) return 0u;
    ANO_Protocol_Debug.rx_frames++;

    if (frame[2] == 0xE1u)
    {
        HandleParameterRead(&frame[4], length);
    }
    else if (frame[2] == 0xE2u)
    {
        if (length >= 6u && ((length - 2u) & 3u) == 0u)
        {
            HandleParameterWrite(frame, length);
            SendAck(frame[2], frame[sum_index], frame[sum_index + 1u]);
        }
    }
    else if (frame[2] == 0xE0u)
    {
        if (length == 11u)
        {
            HandleCommand(&frame[4], length, now_us);
            SendAck(frame[2], frame[sum_index], frame[sum_index + 1u]);
        }
    }
    return 1u;
}

static void DiscardFirstRxByte(void)
{
    uint8_t i;
    RxFrameLength--;
    for (i = 0u; i < RxFrameLength; i++) RxFrame[i] = RxFrame[i + 1u];
}

static void ParseByte(uint8_t byte, uint32_t now_us)
{
    uint8_t expected;

    if (RxFrameLength >= sizeof(RxFrame)) RxFrameLength = 0u;
    RxFrame[RxFrameLength++] = byte;
    RxLastByteUs = now_us;

    /* Shift by one after a bad frame: a valid AA inside the rejected bytes
     * can start the next frame. Never treat AA inside a valid payload as sync. */
    while (RxFrameLength != 0u)
    {
        if (RxFrame[0] != ANO_FRAME_HEAD)
        {
            DiscardFirstRxByte();
            continue;
        }
        if (RxFrameLength >= 2u)
        {
            ANO_Protocol_Debug.last_address = RxFrame[1];
            if (RxFrame[1] != ANO_DEVICE_ADDRESS &&
                RxFrame[1] != ANO_BROADCAST_ADDRESS)
            {
                ANO_Protocol_Debug.address_errors++;
                DiscardFirstRxByte();
                continue;
            }
        }
        if (RxFrameLength >= 3u) ANO_Protocol_Debug.last_id = RxFrame[2];
        if (RxFrameLength < 4u) return;
        ANO_Protocol_Debug.last_length = RxFrame[3];
        if (RxFrame[3] > ANO_MAX_DATA_LEN)
        {
            ANO_Protocol_Debug.length_errors++;
            DiscardFirstRxByte();
            continue;
        }
        expected = (uint8_t)(RxFrame[3] + 6u);
        if (RxFrameLength < expected) return;
        if (HandleFrame(RxFrame, expected, now_us))
        {
            do { DiscardFirstRxByte(); } while (--expected != 0u);
        }
        else
        {
            DiscardFirstRxByte();
        }
    }
}

static void ProcessPendingParameterReads(void)
{
    uint8_t i;
    for (i = 0u; i < ANO_READS_PER_TASK && PendingReadCount != 0u; i++)
    {
        SendParameter(PendingReadId);
        PendingReadId++;
        PendingReadCount--;
    }
}

static int16_t FloatToS16(float value)
{
    if (value != value) return 0;
    if (value > 32767.0f) return 32767;
    if (value < -32768.0f) return -32768;
    if (value >= 0.0f) return (int16_t)(value + 0.5f);
    return (int16_t)(value - 0.5f);
}

static uint8_t GetFusionState(uint16_t status)
{
    uint8_t state = 0u;
    if ((status & ATTITUDE_STATUS_READY) != 0u) state |= 0x01u;
    if ((status & ATTITUDE_STATUS_CALIBRATED) != 0u) state |= 0x02u;
    if ((status & ATTITUDE_STATUS_MAG_OK) != 0u) state |= 0x04u;
    if ((status & ATTITUDE_STATUS_MAG_USED) != 0u) state |= 0x08u;
    if ((status & ATTITUDE_STATUS_MAG_CAL) != 0u) state |= 0x10u;
    if ((status & ATTITUDE_STATUS_MAG_CAL_OK) != 0u) state |= 0x20u;
    return state;
}

void ANO_Protocol_Init(void)
{
    ANO_Protocol_Diagnostics empty = {0};

    RxWrite = 0u;
    RxRead = 0u;
    RxFrameLength = 0u;
    RxLastByteUs = 0u;
    PendingReadId = 0u;
    PendingReadCount = 0u;
    RxDiscardPending = 0u;
    ANO_Protocol_Debug = empty;
    MagCalibrationActive = 0u;
    MagCalibrationStartUs = 0u;
}

void ANO_Protocol_RxByte(uint8_t byte)
{
    uint8_t next = (uint8_t)(RxWrite + 1u);

    /* Keep proof of UART reception even if the parser rejects the stream.
     * The bounded capture deliberately freezes after the first 32 bytes so
     * Keil can inspect the request without racing a continuously moving ring. */
    ANO_Protocol_Debug.rx_bytes++;
    ANO_Protocol_Debug.last_rx_byte = byte;
    if (ANO_Protocol_Debug.rx_capture_count < ANO_RX_CAPTURE_SIZE)
    {
        ANO_Protocol_Debug.rx_capture[ANO_Protocol_Debug.rx_capture_count] = byte;
        ANO_Protocol_Debug.rx_capture_count++;
    }
    if (RxDiscardPending) return;
    if (next == RxRead)
    {
        ANO_Protocol_Debug.rx_overflows++;
        RxDiscardPending = 1u;
        return;
    }
    RxBuffer[RxWrite] = byte;
    RxWrite = next;
}

void ANO_Protocol_RxError(void)
{
    ANO_Protocol_Debug.uart_errors++;
    RxDiscardPending = 1u;
}

void ANO_Protocol_Process(uint32_t now_us)
{
    uint8_t byte;
    uint8_t processed = 0u;
#if OS_CRITICAL_METHOD == 3
    OS_CPU_SR cpu_sr;
#endif

    if (RxDiscardPending)
    {
        OS_ENTER_CRITICAL();
        RxRead = RxWrite;
        RxDiscardPending = 0u;
        OS_EXIT_CRITICAL();
        RxFrameLength = 0u;
    }
    if (RxFrameLength != 0u &&
        (uint32_t)(now_us - RxLastByteUs) >= ANO_RX_FRAME_TIMEOUT_US)
        RxFrameLength = 0u;

    while (!RxDiscardPending && RxRead != RxWrite && processed < 64u)
    {
        byte = RxBuffer[RxRead];
        RxRead = (uint8_t)(RxRead + 1u);
        ParseByte(byte, now_us);
        processed++;
    }

    ProcessPendingParameterReads();
    if (MagCalibrationActive &&
        (uint32_t)(now_us - MagCalibrationStartUs) >= ANO_MAG_CAL_TIMEOUT_US)
    {
        (void)Attitude_FinishMagCalibration();
        MagCalibrationActive = 0u;
    }
}

void ANO_Protocol_SendTelemetry(const Attitude_Data *data)
{
    uint8_t payload[14];
    uint8_t state;
    int16_t value;

    if (data == 0) return;
    state = GetFusionState(data->status);

    /* ID 0x01: accel in configured raw LSB and gyro in configured raw LSB. */
    value = FloatToS16(data->accel_g.x * MPU6050_ACCEL_LSB_PER_G);
    WriteU16(&payload[0], (uint16_t)value);
    value = FloatToS16(data->accel_g.y * MPU6050_ACCEL_LSB_PER_G);
    WriteU16(&payload[2], (uint16_t)value);
    value = FloatToS16(data->accel_g.z * MPU6050_ACCEL_LSB_PER_G);
    WriteU16(&payload[4], (uint16_t)value);
    value = FloatToS16(data->gyro_dps.x * MPU6050_GYRO_LSB_PER_DPS);
    WriteU16(&payload[6], (uint16_t)value);
    value = FloatToS16(data->gyro_dps.y * MPU6050_GYRO_LSB_PER_DPS);
    WriteU16(&payload[8], (uint16_t)value);
    value = FloatToS16(data->gyro_dps.z * MPU6050_GYRO_LSB_PER_DPS);
    WriteU16(&payload[10], (uint16_t)value);
    payload[12] = 0u;
    SendFrame(ANO_BROADCAST_ADDRESS, 0x01u, payload, 13u);

    /* ID 0x02: magnetometer counts; barometer and temperature are unavailable. */
    WriteU16(&payload[0], (uint16_t)data->mag_raw[0]);
    WriteU16(&payload[2], (uint16_t)data->mag_raw[1]);
    WriteU16(&payload[4], (uint16_t)data->mag_raw[2]);
    WriteS32(&payload[6], (int32_t)ANO_INVALID_PARAM_VALUE);
    WriteU16(&payload[10], 0x8000u);
    payload[12] = 0u;
    payload[13] = (data->status & ATTITUDE_STATUS_MAG_OK) != 0u ? 2u : 0u;
    SendFrame(ANO_BROADCAST_ADDRESS, 0x02u, payload, 14u);

    /* ID 0x03: Euler angles in hundredths of a degree. */
    WriteU16(&payload[0], (uint16_t)FloatToS16(data->angle.roll * 100.0f));
    WriteU16(&payload[2], (uint16_t)FloatToS16(data->angle.pitch * 100.0f));
    WriteU16(&payload[4], (uint16_t)FloatToS16(data->angle.yaw * 100.0f));
    payload[6] = state;
    SendFrame(ANO_BROADCAST_ADDRESS, 0x03u, payload, 7u);

    /* ID 0x04: quaternion components (w, x, y, z) scaled by 10000. */
    WriteU16(&payload[0], (uint16_t)FloatToS16(data->q.w * 10000.0f));
    WriteU16(&payload[2], (uint16_t)FloatToS16(data->q.x * 10000.0f));
    WriteU16(&payload[4], (uint16_t)FloatToS16(data->q.y * 10000.0f));
    WriteU16(&payload[6], (uint16_t)FloatToS16(data->q.z * 10000.0f));
    payload[8] = state;
    SendFrame(ANO_BROADCAST_ADDRESS, 0x04u, payload, 9u);
}

uint8_t ANO_Protocol_GetPID(uint8_t group, ANO_PID_Config *config)
{
    if (group >= 3u || config == 0) return 0u;
    config->p = ANO_PID_Groups[group].p;
    config->i = ANO_PID_Groups[group].i;
    config->d = ANO_PID_Groups[group].d;
    return 1u;
}
