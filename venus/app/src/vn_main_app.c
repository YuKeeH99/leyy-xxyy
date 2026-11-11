#include <stdbool.h>
#include <stdint.h>

#include "ql_application.h"
#include "ql_nw.h"
#include "ql_power.h"
#include "ql_rtos.h"

#include "vn_plat.h"
#include "vn_audio_app.h"
#include "vn_charge.h"
#include "vn_display.h"
#include "vn_factory.h"
#include "vn_gpio.h"
#include "vn_net.h"
#include "vn_param.h"
#include "vn_radar.h"
#include "vn_utils.h"
#include "vn_wakeup_chip.h"
#include "vn_wifi.h"
#include "vn_wifi_app.h"
#include "vn_xiaozhi_app.h"

#define VN_MAIN_LOG_PERIOD_COUNT          5000U
#define VN_MAIN_TOUCH_PERIOD_MS           10U
#define VN_MAIN_NET_PERIOD_MS             2000U
#define VN_MAIN_POWER_PERIOD_MS           2000U
#define VN_MAIN_RADAR_PERIOD_MS           100U
#define VN_MAIN_RADAR_NOTICE_DELAY_MS     2000U
#define VN_MAIN_POWER_DOWN_DELAY_MS       10000U
#define VN_MAIN_WAKEUP_RESET_PERIOD_COUNT 3600000U
#define VN_MAIN_WAKEUP_DISABLE_COUNT      100U
#define VN_MAIN_WAKEUP_ENABLE_COUNT       300U
#define VN_MAIN_RADAR_TRIGGER_DISTANCE_CM 150U
#define VN_MAIN_RADAR_NOTICE_DISTANCE_CM  60
#define VN_MAIN_RADAR_SAMPLE_COUNT        10U
#define VN_MAIN_RADAR_SAMPLE_STORAGE      12U
#define VN_MAIN_BOOT_VOLTAGE_LIMIT_V      3.6
#define VN_MAIN_POWER_DOWN_VOLTAGE_V      3.5f
#define VN_MAIN_BATTERY_EMPTY_VOLTAGE_V   3.2
#define VN_MAIN_RADAR_NOTICE_EMOJI_INDEX  11U

ql_task_t Vn_AudioTaskRef = NULL;
ql_task_t Vn_XiaozhiTaskRef = NULL;

typedef struct
{
    Vn_Key_TouchStatus_Typedef LastTouchStatus;
    uint32_t KeyPressedTime;
    bool ShortPressTriggered;
    bool LongPressTriggered;
    uint32_t SHORT_PRESS_THRESHOLD;
    uint32_t LONG_PRESS_THRESHOLD;
} Vn_KeyState_Typedef;

static Vn_KeyState_Typedef Vn_KeyState = {
    .LastTouchStatus = VN_KEY_TOUCH_STATUS_VALID,
    .KeyPressedTime = 0U,
    .ShortPressTriggered = false,
    .LongPressTriggered = false,
    .SHORT_PRESS_THRESHOLD = 500U,
    .LONG_PRESS_THRESHOLD = 5000U,
};

static uint8_t Vn_BatteryErrorMessage[] = "警告！！检测电量异常";
static uint8_t Vn_BatteryLowMessage[] = "警告！！低电量，功能受限";
static uint8_t Vn_PowerDownMessage[] = "低电量警告，请充电";
static uint8_t Vn_RadarNoticeMessage[] = "哎呀，别靠我太近";

static void Vn_WakeupWorldCallback(void)
{
    VN_LOG_I("wakeup world callback");
    Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_WAKEUP_WORD);
}

static void Vn_WakeupRadarCallback(void)
{
    VN_LOG_I("wakeup radar callback");
    Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_RADAR_DETECTED);
}

static void Vn_Main_CheckBootBattery(void)
{
    Vn_ChargeStatus_Typedef charge_status;
    float voltage;
    int32_t ret;

    for (;;)
    {
        VN_LOG_I("Low battery power checking ..");

        charge_status = VN_CHARGE_STATUS_ERROR;
        voltage = 0.0f;
        ret = Vn_Charge_GetStatus(&charge_status);
        VN_LOG_D("charge status:%d, ret %d", charge_status, ret);

        if ((charge_status == VN_CHARGE_STATUS_START) ||
            (charge_status == VN_CHARGE_STATUS_STOP))
        {
            VN_LOG_I("chargeing, skip");
            return;
        }

        ret = Vn_Charge_GetBatteryVoltage(&voltage);
        if (ret == VN_RET_OK)
        {
            if (voltage > VN_MAIN_BOOT_VOLTAGE_LIMIT_V)
            {
                VN_LOG_I("battery voltage %.1f, ret = %d, skip", voltage, ret);
                return;
            }

            Vn_DisplayMsg_SendMsgBox(Vn_BatteryLowMessage, 3U);
        }
        else
        {
            Vn_DisplayMsg_SendMsgBox(Vn_BatteryErrorMessage, 3U);
            ql_rtos_task_sleep_ms(3000U);
        }

        ql_rtos_task_sleep_ms(1000U);
    }
}

static void Vn_Main_ProcessTouch(void)
{
    static uint32_t touch_check_timestamp;
    Vn_Key_TouchStatus_Typedef current_status = VN_KEY_TOUCH_STATUS_VALID;
    uint32_t now = VN_RTOS_SYSTEM_MS;
    uint32_t pressed_time;

    if (Vn_DiffTime(touch_check_timestamp, now) >= VN_MAIN_TOUCH_PERIOD_MS)
    {
        touch_check_timestamp = now;
        (void)Vn_Key_GetTouchStatus(&current_status);

        if (Vn_KeyState.LastTouchStatus != current_status)
        {
            if (current_status == VN_KEY_TOUCH_STATUS_VALID)
            {
                Vn_KeyState.LongPressTriggered = false;
                Vn_KeyState.KeyPressedTime = now;
                VN_LOG_D("Key pressed down");
            }
            else
            {
                pressed_time = Vn_DiffTime(Vn_KeyState.KeyPressedTime, now);
                if (!Vn_KeyState.LongPressTriggered)
                {
                    if (pressed_time < Vn_KeyState.SHORT_PRESS_THRESHOLD)
                    {
                        Vn_KeyState.ShortPressTriggered = true;
                        VN_LOG_I("Short press detected");
                    }
                    else
                    {
                        VN_LOG_I("Long press detected (on release), KeyPressedTime: %u ms",
                                 Vn_KeyState.KeyPressedTime);
                    }
                }
                VN_LOG_D("Key released");
            }

            Vn_KeyState.LastTouchStatus = current_status;
        }

        if ((current_status == VN_KEY_TOUCH_STATUS_VALID) &&
            !Vn_KeyState.LongPressTriggered)
        {
            pressed_time = Vn_DiffTime(Vn_KeyState.KeyPressedTime, now);
            if (pressed_time >= Vn_KeyState.LONG_PRESS_THRESHOLD)
            {
                Vn_KeyState.KeyPressedTime = now + Vn_KeyState.LONG_PRESS_THRESHOLD;
                VN_LOG_I("Long press detected (sustained), duration: %u ms", pressed_time);
                Vn_KeyState.LongPressTriggered = true;
            }
        }
    }

    if (Vn_KeyState.ShortPressTriggered)
    {
        Vn_KeyState.ShortPressTriggered = false;
        Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_TOUCH);
    }
    else if (Vn_KeyState.LongPressTriggered)
    {
        Vn_KeyState.LongPressTriggered = false;
        Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_NET_TYPE_CHANGE);
    }
}

static uint8_t Vn_Main_CellularSignalLevel(int rssi)
{
    if (rssi >= -79)
    {
        return 4U;
    }
    if (rssi >= -100)
    {
        return 3U;
    }
    if (rssi >= -120)
    {
        return 2U;
    }
    return 1U;
}

static uint8_t Vn_Main_WifiSignalLevel(int32_t rssi)
{
    if (rssi >= -34)
    {
        return 8U;
    }
    if (rssi >= -43)
    {
        return 7U;
    }
    if (rssi >= -50)
    {
        return 6U;
    }
    return 5U;
}

static void Vn_Main_CheckNetwork(void)
{
    static uint32_t net_status_check_timestamp;
    QL_NW_SIGNAL_STRENGTH_INFO_T signal_strength = {0};
    Vn_Xiaozhi_Typedef *xiaozhi;
    int32_t rssi;
    uint32_t now = VN_RTOS_SYSTEM_MS;

    if (Vn_DiffTime(net_status_check_timestamp, now) < VN_MAIN_NET_PERIOD_MS)
    {
        return;
    }
    net_status_check_timestamp = now;

    xiaozhi = Vn_Xiaozhi_GetInstance();
    if (xiaozhi->NetType == VN_NET_TYPE_CELLULAR)
    {
        if (xiaozhi->IsNetConnected)
        {
            if (ql_nw_get_signal_strength(&signal_strength) == QL_NW_SUCCESS)
            {
                rssi = signal_strength.LTE_SignalStrength.rssi;
                VN_LOG_I("signal strength: %d", rssi);
                Vn_DisplayMsg_SendLcdSignal(Vn_Main_CellularSignalLevel(rssi));
            }
            else
            {
                Vn_DisplayMsg_SendLcdSignal(0U);
                VN_LOG_E("get signal strength failed");
            }
        }
    }
    else if (Vn_Wifi_GetRssi(&rssi) == VN_RET_OK)
    {
        Vn_DisplayMsg_SendLcdSignal(Vn_Main_WifiSignalLevel(rssi));
        VN_LOG_I("rssi: %d", rssi);
    }
}

static void Vn_Main_CheckPower(void)
{
    static uint32_t power_check_timestamp;
    static bool power_down_started;
    static uint32_t power_down_timestamp;
    Vn_ChargeStatus_Typedef charge_status;
    float voltage;
    uint8_t show_percent;
    int32_t ret;
    uint32_t now = VN_RTOS_SYSTEM_MS;

    if (Vn_DiffTime(power_check_timestamp, now) < VN_MAIN_POWER_PERIOD_MS)
    {
        return;
    }
    power_check_timestamp = now;

    charge_status = VN_CHARGE_STATUS_ERROR;
    voltage = 0.0f;
    ret = Vn_Charge_GetStatus(&charge_status);
    VN_LOG_D("charge status:%d, ret %d", charge_status, ret);

    ret = Vn_Charge_GetBatteryVoltage(&voltage);
    if (ret != VN_RET_OK)
    {
        return;
    }

    show_percent = 0U;
    if (voltage >= VN_MAIN_BATTERY_EMPTY_VOLTAGE_V)
    {
        show_percent = (uint8_t)((voltage - VN_MAIN_BATTERY_EMPTY_VOLTAGE_V) * 100.0f);
    }
    Vn_DisplayMsg_SendMsgBattery(
        (uint8_t)(charge_status == VN_CHARGE_STATUS_START), show_percent);
    VN_LOG_D("battery voltage:%.3f, show_percent=%d%%", voltage, show_percent);

    if ((voltage > VN_MAIN_POWER_DOWN_VOLTAGE_V) && power_down_started)
    {
        power_down_started = false;
    }

    if ((charge_status != VN_CHARGE_STATUS_START) &&
        (voltage < VN_MAIN_POWER_DOWN_VOLTAGE_V))
    {
        if (!power_down_started)
        {
            Vn_DisplayMsg_SendMsgBox(Vn_PowerDownMessage, 10U);
            power_down_started = true;
            power_down_timestamp = now;
            VN_LOG_W("========battery low, waiting time out %d seconds...==========",
                     (int)VN_MAIN_POWER_DOWN_DELAY_MS);
        }

        if (Vn_DiffTime(power_down_timestamp, now) >= VN_MAIN_POWER_DOWN_DELAY_MS)
        {
            VN_LOG_W("========battery low, power down====================");
            ql_power_down(1U);
        }
    }
}

static void Vn_Main_CheckRadar(void)
{
    static uint32_t radar_check_timestamp;
    static bool note_started;
    static uint32_t note_timestamp;
    static int32_t distance;
    static int32_t distance_avg;
    static int32_t distances[VN_MAIN_RADAR_SAMPLE_STORAGE];
    static uint32_t calculate_count;
    bool had_target = false;
    uint32_t now = VN_RTOS_SYSTEM_MS;

    if (Vn_DiffTime(radar_check_timestamp, now) < VN_MAIN_RADAR_PERIOD_MS)
    {
        return;
    }
    radar_check_timestamp = now;

    if ((Vn_Radar_GetTragetDistance(&distance, &had_target) == VN_RET_OK) && had_target)
    {
        if (distance_avg < 10)
        {
            distance_avg = distance;
        }
        distance_avg = (distance + distance_avg) / 2;

        if (distance_avg <= VN_MAIN_RADAR_NOTICE_DISTANCE_CM)
        {
            if (note_started &&
                (Vn_DiffTime(now, note_timestamp) > VN_MAIN_RADAR_NOTICE_DELAY_MS))
            {
                note_started = false;
                Vn_DisplayMsg_SendMsgBox(Vn_RadarNoticeMessage, 5U);
                VN_LOG_I("==================>radar trigger note");
                Vn_AudioApp_PlaySound(VN_AUDIO_SOUND_INDEX_POPUP);
                Vn_DisplayMsg_SendOledEmoji(VN_MAIN_RADAR_NOTICE_EMOJI_INDEX);
            }

            if (!note_started)
            {
                distance_avg = 0;
                note_started = true;
                note_timestamp = now;
            }
        }
        else
        {
            note_started = false;
        }

        distances[calculate_count] = distance;
        ++calculate_count;
    }

    if (calculate_count >= VN_MAIN_RADAR_SAMPLE_COUNT)
    {
        VN_LOG_I("distance: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d; distance_avg: %d",
                 distances[0], distances[1], distances[2], distances[3], distances[4],
                 distances[5], distances[6], distances[7], distances[8], distances[9],
                 distance_avg);
        calculate_count = 0U;
    }
}

static void Vn_Main_ServiceWakeupReset(uint32_t time_count)
{
    static bool reset_in_progress;
    static bool is_enabled;
    static uint32_t disable_start_time;
    static uint32_t enable_start_time;
    int32_t ret;

    if (!reset_in_progress)
    {
        if ((time_count % VN_MAIN_WAKEUP_RESET_PERIOD_COUNT) == 0U)
        {
            VN_LOG_I("reset wakeup chip to avoid miss wakeup");
            (void)Vn_WakeupChip_DisableTrigger();
            (void)Vn_LDO_Control(VN_LDO_WAKEUP_CHIP, VN_LDO_STATUS_DISABLE);
            reset_in_progress = true;
            disable_start_time = time_count;
        }
        else
        {
            return;
        }
    }

    if (Vn_DiffTime(disable_start_time, time_count) < VN_MAIN_WAKEUP_DISABLE_COUNT)
    {
        return;
    }

    if (!is_enabled)
    {
        (void)Vn_LDO_Control(VN_LDO_WAKEUP_CHIP, VN_LDO_STATUS_ENABLE);
        enable_start_time = time_count;
        is_enabled = true;
    }

    if (Vn_DiffTime(enable_start_time, time_count) >= VN_MAIN_WAKEUP_ENABLE_COUNT)
    {
        is_enabled = false;
        reset_in_progress = false;
        VN_LOG_I("re-enable wakeup chip trigger");
        ret = Vn_WakeupChip_EnableTrigger(Vn_WakeupWorldCallback);
        if (ret != VN_RET_OK)
        {
            VN_LOG_E("re-enable wakeup chip failed");
        }
    }
}

void Vn_Main_App(void *arg)
{
    (void)arg;
    static uint32_t time_count;
    uint32_t distance = VN_MAIN_RADAR_TRIGGER_DISTANCE_CM;
    int32_t ret;

    VN_LOG_I("vn main app start");

    Vn_Log_Init();
    (void)Vn_LDO_Init();
    Vn_Param_Init();
    Vn_Display_Init();

    Vn_Main_CheckBootBattery();

    ret = Vn_Radar_Init();
    VN_LOG_I("radar init ret: %d", ret);
    (void)Vn_Radar_EnableTriggerWakeUp(Vn_WakeupRadarCallback);

    ret = Vn_Radar_SetTriggerDistance(distance);
    if (ret == VN_RET_OK)
    {
        VN_LOG_I("set radar trigger distance to %d cm", distance);
    }
    else
    {
        VN_LOG_E("set radar trigger distance failed");
    }

    (void)Vn_Key_TouchInit();
    (void)Vn_WakeupChip_Init();
    (void)Vn_WakeupChip_EnableTrigger(Vn_WakeupWorldCallback);
    (void)Vn_Charge_Init();
    (void)Vn_Wifi_AppInit();

    (void)ql_rtos_task_create(&Vn_AudioTaskRef, 0x5000U, 109U, "Audio_App",
                              Vn_Audio_App, NULL);

    if (Vn_Factory_IsOtaWifiMode())
    {
        Vn_Factory_RunOtaWifi();
        ql_rtos_task_sleep_ms(30000U);
    }

    (void)ql_rtos_task_create(&Vn_XiaozhiTaskRef, 0xF000U, 107U, "Xiaozhi_App",
                              Vn_Xiaozhi_App, NULL);

    for (;;)
    {
        ++time_count;
        if ((time_count % VN_MAIN_LOG_PERIOD_COUNT) == 0U)
        {
            VN_LOG_I("free heap size: %u bytes", ql_rtos_get_free_heap_size());
            VN_LOG_I("cpu usage rate: %u%%", ql_get_cpu_usage_rate());
        }

        Vn_Main_ProcessTouch();
        Vn_Main_CheckNetwork();
        Vn_Main_CheckPower();
        Vn_Main_CheckRadar();
        Vn_Main_ServiceWakeupReset(time_count);

        ql_rtos_task_sleep_ms(1U);
    }
}

application_init(Vn_Main_App, "Vn_Main_App", 20, 110);
