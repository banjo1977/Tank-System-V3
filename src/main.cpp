// Signal K application template file.
//
// This is a line by line rebuild of the tank system
// V3.x - line by line rebuild, data only.
// V3.1 - Display on paper white display
// V3.2 - Add touch sensitive pads to control buzzer and display refresh.  ALso add clock (time form signalk)
// V3.3 - Add IP address to boot status message and correct date to include 2 digit year.
// And boot status message (IP address, software version, etc.)
// V3.3.2- Amend touch logic - debounce and improve response.
//  Need to:
// validate function of touch sensors on real hardware (or swap for switches)
// Amend the timing 
// V3.4 - roll out, correct logic, analogue input faults. 
// V3.5 - Corrected black water tank calibration.  Added status page readout of input and calibration pairs
// V3.6 - Amended tank display order to make easier to read.
// V4.0 - Added two RGB LEDs, externally addressibe with colour and brightness, as well as making buzzer addressible.


// Functionality for two touch-sensitive pads:
// Pad 1 - update the display now (ie, don't wait 60 seconds).
// Pad 2 - Toggle the Buzzer function 'on' and 'off' and update the icon on the display, then refresh the display.



#include <memory>
#include "sensesp.h"
#include "sensesp/sensors/analog_input.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/transforms/linear.h"
#include "sensesp_app_builder.h"
#include "sensesp/sensors/digital_output.h"
#include "sensesp/signalk/signalk_listener.h"
#include "sensesp/signalk/signalk_value_listener.h"
#include "sensesp/transforms/repeat.h"
#include "epaper.h"
#include "sensesp/controllers/smart_switch_controller.h"
#include "sensesp/sensors/digital_input.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/system/lambda_consumer.h"
#include "sensesp/transforms/repeat.h"
#include "sensesp_app_builder.h"
#include "sensesp/signalk/signalk_listener.h"
#include "sensesp/signalk/signalk_put_request_listener.h"
#include "sensesp/transforms/click_type.h"
#include "sensesp/transforms/debounce.h"
#include "sensesp/transforms/press_repeater.h"
#include "sensesp_app.h"
#include "sensesp/system/rgb_led.h"
#include "sensesp/ui/status_page_item.h"
#include <time.h>
#include "sensesp/net/networking.h"
#include "esp_task_wdt.h"

using namespace sensesp;

const char* SOFTWARE_VERSION = "v4-0"; // Update as needed
#define BUZ_CTRL_PIN 12 // Touch Pad 1
#define DISPLAY_CTRL_PIN 4 // Touch Pad 2
#define TOUCH_THRESHOLD 17 // Define threshold for touch sensitivity
#define LED_PIN 2  // Change to your board's LED GPIO if different


float bV[NUM_BARS] = {0, 0, 0, 0, 0, 0};
float display_bV[NUM_BARS] = {0, 0, 0, 0, 0, 0};
int refresh_counter = 0;

const int BUZZER_PIN = 19; // Pin 19 for buzzer control.
bool buzzer_enabled = true;      // user preference (icon reflects this)
bool buzzerStatus = true;        // icon: shows enabled/disabled (true = enabled icon)
unsigned long bw_over90_start = 0;
unsigned long buzzer_beep_until = 0; // Timer for short beep after manual enable

bool buzzer_active = false;      // indicates alarm-driven sounding (not used for icon)
static bool buzzer_sounding = false; // actual physical output state (true = sounding)
static bool buzzer_requested = false; // external/local request to sound, gated by buzzer_enabled
std::shared_ptr<sensesp::DigitalOutput> buzzer_switch;

struct RgbLedDriver {
    const uint8_t red_pin;
    const uint8_t green_pin;
    const uint8_t blue_pin;
    const uint8_t red_channel;
    const uint8_t green_channel;
    const uint8_t blue_channel;
    String command;
    float brightness;
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    RgbLedDriver(uint8_t rp, uint8_t gp, uint8_t bp, uint8_t rc, uint8_t gc, uint8_t bc)
        : red_pin(rp), green_pin(gp), blue_pin(bp), red_channel(rc), green_channel(gc), blue_channel(bc),
          command("off"), brightness(1.0f), red(0), green(0), blue(0) {}
};

RgbLedDriver led1{21, 22, 23, 0, 1, 2};
RgbLedDriver led2{18, 16, 17, 3, 4, 5};

void configureRgbLed(RgbLedDriver& led) {
    ledcSetup(led.red_channel, 5000, 8);
    ledcSetup(led.green_channel, 5000, 8);
    ledcSetup(led.blue_channel, 5000, 8);

    ledcAttachPin(led.red_pin, led.red_channel);
    ledcAttachPin(led.green_pin, led.green_channel);
    ledcAttachPin(led.blue_pin, led.blue_channel);
}

void applyRgbLed(RgbLedDriver& led) {
    const float brightness = constrain(led.brightness, 0.0f, 1.0f);
    const uint8_t scaled_red = static_cast<uint8_t>(led.red * brightness);
    const uint8_t scaled_green = static_cast<uint8_t>(led.green * brightness);
    const uint8_t scaled_blue = static_cast<uint8_t>(led.blue * brightness);

    ledcWrite(led.red_channel, scaled_red);
    ledcWrite(led.green_channel, scaled_green);
    ledcWrite(led.blue_channel, scaled_blue);
}

void setRgbLedCommand(RgbLedDriver& led, const String& command) {
    String cmd = command;
    cmd.trim();
    cmd.toLowerCase();

    led.command = cmd;

    if (cmd == "red") {
        led.red = 255;
        led.green = 0;
        led.blue = 0;
    } else if (cmd == "green") {
        led.red = 0;
        led.green = 255;
        led.blue = 0;
    } else if (cmd == "blue") {
        led.red = 0;
        led.green = 0;
        led.blue = 255;
    } else if (cmd == "white") {
        led.red = 255;
        led.green = 255;
        led.blue = 255;
    } else if (cmd == "yellow") {
        led.red = 255;
        led.green = 255;
        led.blue = 0;
    } else if (cmd == "cyan") {
        led.red = 0;
        led.green = 255;
        led.blue = 255;
    } else if (cmd == "magenta") {
        led.red = 255;
        led.green = 0;
        led.blue = 255;
    } else {
        led.red = 0;
        led.green = 0;
        led.blue = 0;
    }

    applyRgbLed(led);
}

void setRgbLedBrightness(RgbLedDriver& led, float brightness) {
    led.brightness = constrain(brightness, 0.0f, 1.0f);
    applyRgbLed(led);
}

// Tank diagnostics for the status page
sensesp::StatusPageItem<float> tank1_raw_adc("Stbd Fuel raw ADC", 0.0f, "Tank Diagnostics", 1000);
sensesp::StatusPageItem<float> tank1_calibrated("Stbd Fuel calibrated", 0.0f, "Tank Diagnostics", 1001);
sensesp::StatusPageItem<float> tank1_percent("Stbd Fuel %", 0.0f, "Tank Diagnostics", 1002);
sensesp::StatusPageItem<float> tank2_raw_adc("Port Fuel raw ADC", 0.0f, "Tank Diagnostics", 1003);
sensesp::StatusPageItem<float> tank2_calibrated("Port Fuel calibrated", 0.0f, "Tank Diagnostics", 1004);
sensesp::StatusPageItem<float> tank2_percent("Port Fuel %", 0.0f, "Tank Diagnostics", 1005);
sensesp::StatusPageItem<float> tank3_raw_adc("Black Water raw ADC", 0.0f, "Tank Diagnostics", 1006);
sensesp::StatusPageItem<float> tank3_calibrated("Black Water calibrated", 0.0f, "Tank Diagnostics", 1007);
sensesp::StatusPageItem<float> tank3_percent("Black Water %", 0.0f, "Tank Diagnostics", 1008);
sensesp::StatusPageItem<float> tank4_raw_adc("Port Aft Water raw ADC", 0.0f, "Tank Diagnostics", 1009);
sensesp::StatusPageItem<float> tank4_calibrated("Port Aft Water calibrated", 0.0f, "Tank Diagnostics", 1010);
sensesp::StatusPageItem<float> tank4_percent("Port Aft Water %", 0.0f, "Tank Diagnostics", 1011);
sensesp::StatusPageItem<float> tank5_raw_adc("Stbd Water raw ADC", 0.0f, "Tank Diagnostics", 1012);
sensesp::StatusPageItem<float> tank5_calibrated("Stbd Water calibrated", 0.0f, "Tank Diagnostics", 1013);
sensesp::StatusPageItem<float> tank5_percent("Stbd Water %", 0.0f, "Tank Diagnostics", 1014);
sensesp::StatusPageItem<float> tank6_raw_adc("Port Fwd Water raw ADC", 0.0f, "Tank Diagnostics", 1015);
sensesp::StatusPageItem<float> tank6_calibrated("Port Fwd Water calibrated", 0.0f, "Tank Diagnostics", 1016);
sensesp::StatusPageItem<float> tank6_percent("Port Fwd Water %", 0.0f, "Tank Diagnostics", 1017);

// Non-blocking restart scheduling
static unsigned long both_pressed_start = 0;
static bool both_pressed_buzzer_activated = false;
static bool restart_scheduled = false;
static unsigned long restart_scheduled_at = 0;
static const unsigned long RESTART_HOLD_MS = 5000;   // hold duration to request restart
static const unsigned long RESTART_FEEDBACK_MS = 500; // feedback period before actual restart

// Debounce state for touch pads
static bool last_buz_state = false;
static bool last_disp_state = false;
unsigned long last_epaper_update = 0;
const unsigned long epaper_update_delay = 500; // Minimum 500ms between updates

// Add at the top with other globals:
unsigned long boot_time = 0;
const unsigned long ALARM_STARTUP_DELAY = 30000; // 30 seconds before alarm checks

void setBuzzerOutput(bool on);

// Helper to control buzzer output; `on = true` means buzzer sounding.
// Hardware: buzzer is active LOW (LOW = ON), HIGH = OFF.
void updateBuzzerState() {
    setBuzzerOutput(buzzer_enabled && buzzer_requested);
}

void requestBuzzer(bool requested) {
    buzzer_requested = requested;
    updateBuzzerState();
}

void setBuzzerOutput(bool on) {
    const bool effective_on = buzzer_enabled && on;

    // Avoid redundant operations
    if (buzzer_sounding == effective_on) {
        return;
    }
    buzzer_sounding = effective_on;

    if (effective_on) {
        // Activate buzzer (active low)
        if (buzzer_switch) {
            buzzer_switch->set(false); // mirror physical pin state (LOW)
        }
        digitalWrite(BUZZER_PIN, LOW);
    } else {
        // Deactivate buzzer (safe off = HIGH)
        if (buzzer_switch) {
            buzzer_switch->set(true); // mirror physical pin state (HIGH)
        }
        digitalWrite(BUZZER_PIN, HIGH);
    }
}

void set_time_from_signalk(String sk_time) {
    Serial.print("Received SK time: ");
    Serial.println(sk_time);

    struct tm tm = {0};
    // Parse ISO 8601 with numeric timezone offset
    sk_time.trim();
    if (strptime(sk_time.c_str(), "%Y-%m-%dT%H:%M:%S", &tm) != NULL) {
        time_t t = mktime(&tm);
        struct timeval now = { .tv_sec = t };
        settimeofday(&now, NULL);
        //Serial.print("System time set to: ");
        //Serial.println(asctime(&tm));
    } else {
        Serial.println("Failed to parse SK time string!");
    }
}

// The setup function performs one-time application initialization.
void setup()
{
    SetupLogging(ESP_LOG_WARN);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH); // active LOW buzzer -> HIGH = off      

    Serial.print("Software version: ");
    Serial.println(SOFTWARE_VERSION);

    Serial.print("WiFi SSID: ");
    Serial.println(WiFi.SSID());

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
   
    // esp_task_wdt_delete(NULL); // Disable WDT temporarily
    esp_task_wdt_init(10, true);  // 10 second timeout instead of default ~5 seconds to cope with epaper
    epaper_init();

    pinMode(BUZ_CTRL_PIN, INPUT); // Set up the buzzer control pad
    pinMode(DISPLAY_CTRL_PIN, INPUT); // Set up the display control pad

    // initialize buzzer hardware & software state (synchronized)
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH); // ensure safe off level on boot (hardware HIGH = OFF)
    buzzer_enabled = true;
    buzzerStatus = buzzer_enabled; // icon shows enabled state
    buzzer_active = false;
    buzzer_requested = false;
    buzzer_sounding = false;

    // DON'T create DigitalOutput yet - just keep manual control until end of setup
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    configureRgbLed(led1);
    configureRgbLed(led2);
    setRgbLedCommand(led1, "off");
    setRgbLedCommand(led2, "off");

    // record boot time for startup alarm suppression
    boot_time = millis();

    //report wifi details on connection 
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
        Serial.print("Network connected!\nSoftware version: ");
        Serial.println(SOFTWARE_VERSION);

        Serial.print("WiFi SSID: ");
        Serial.println(WiFi.SSID());

        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        extern bool show_boot_status;
        extern String last_boot_status_ip;
        if (show_boot_status) {
            String new_ip = WiFi.localIP().toString();
            if (last_boot_status_ip != new_ip) {
                last_boot_status_ip = new_ip;
                epaper_update(); // Redraw status bar with new IP
            }
        }
    }
    });


    // Construct the global SensESPApp() object
    char hostname[64];
    snprintf(hostname, sizeof(hostname), "contour-tanksystem-%s", SOFTWARE_VERSION);

    SensESPAppBuilder builder;
    sensesp_app = (&builder)
                      // Set a custom hostname for the app.
                      ->set_hostname(hostname)
                      // Optionally, hard-code the WiFi and Signal K server
                      // settings. This is normally not needed.
                      //->set_wifi_client("My WiFi SSID", "my_wifi_password")
                      //->set_wifi_access_point("My AP SSID", "my_ap_password")
                      //->set_sk_server("192.168.10.3", 80)

                      ->get_app();
    
    // Listen for Signal K environment.time and update system clock
    auto* sk_time_listener = new SKValueListener<String>("environment.time");    sk_time_listener->connect_to(new LambdaConsumer<String>([](String sk_time) {
        set_time_from_signalk(sk_time);
    }));

    auto* sk_led1_command_listener = new SKValueListener<String>("electrical.indicators.led1.command");
    sk_led1_command_listener->connect_to(new LambdaConsumer<String>([](String command) {
        setRgbLedCommand(led1, command);
    }));

    auto* sk_led1_brightness_listener = new SKValueListener<float>("electrical.indicators.led1.brightness");
    sk_led1_brightness_listener->connect_to(new LambdaConsumer<float>([](float brightness) {
        setRgbLedBrightness(led1, brightness);
    }));

    auto* sk_led2_command_listener = new SKValueListener<String>("electrical.indicators.led2.command");
    sk_led2_command_listener->connect_to(new LambdaConsumer<String>([](String command) {
        setRgbLedCommand(led2, command);
    }));

    auto* sk_led2_brightness_listener = new SKValueListener<float>("electrical.indicators.led2.brightness");
    sk_led2_brightness_listener->connect_to(new LambdaConsumer<float>([](float brightness) {
        setRgbLedBrightness(led2, brightness);
    }));

    auto* sk_buzzer_listener = new SKValueListener<bool>("electrical.switches.alarm.buzzer");
    sk_buzzer_listener->connect_to(new LambdaConsumer<bool>([](bool enabled) {
        buzzer_enabled = enabled;
        buzzerStatus = enabled;
        updateBuzzerState();
    }));

    // GPIO numbers to use for the analog inputs (linked to tank sensors)
    const uint8_t kAnalogInputpin_1 = 33; // Stbd Fuel Tank
    const uint8_t kAnalogInputpin_2 = 34; // Port Fuel Tank
    const uint8_t kAnalogInputpin_3 = 39; // Black Water Tank
    const uint8_t kAnalogInputpin_4 = 32; // Port Aft Fresh Water tank
    const uint8_t kAnalogInputpin_5 = 35; // Stbd Fresh Water tank
    const uint8_t kAnalogInputpin_6 = 36; // Port Forward Fresh Water tank

    const char *sk_path_1 = "tanks.fuel.1.currentLevel";       // Starboard Fuel Tank
    const char *sk_path_2 = "tanks.fuel.2.currentLevel";       // Port Fuel Tank
    const char *sk_path_3 = "tanks.blackWater.1.currentLevel"; // Black Water
                                                               // tank
    const char *sk_path_4 =
        "tanks.freshWater.1.currentLevel"; // Port Aft Fresh Water tank
    const char *sk_path_5 =
        "tanks.freshWater.2.currentLevel"; // Stbd Fresh Water tank
    const char *sk_path_6 =
        "tanks.freshWater.3.currentLevel"; // Port Forward Fresh Water tank
    
    const char* sk_path_buzz = "electrical.switches.alarm.buzzer";
    const char* sk_path_buzzer_alarm = "electrical.switches.alarm.buzzerAlarm";
    
    const char* config_path_sk_output = "/signalk/path";
    const char* config_path_repeat = "/signalk/repeat";

             
    // The "Configuration path" is combined with "/config" to formulate a URL
    // used by the RESTful API for retrieving or setting configuration data.
    // It is ALSO used to specify a path to the SPIFFS file system
    // where configuration data is saved on the MCU board. It should
    // ALWAYS start with a forward slash if specified. If left blank,
    // that indicates this sensor or transform does not have any
    // configuration to save, or that you're not interested in doing
    // run-time configuration.
    const char *kAnalogInputConfigPath_1 = "/fuel_tank_1/analog_in";
    const char *kAnalogInputConfigPath_2 = "/fuel_tank_2/analog_in";
    const char *kAnalogInputConfigPath_3 = "/blackwater_tank_1/analog_in";
    const char *kAnalogInputConfigPath_4 = "/freshwater_tank_1/analog_in";
    const char *kAnalogInputConfigPath_5 = "/freshwater_tank_2/analog_in";
    const char *kAnalogInputConfigPath_6 = "/freshwater_tank_3/analog_in";

    // Define how often (in milliseconds) new samples are acquired
    const unsigned int kAnalogInputReadInterval = 1000;

    // Create a Analog Input Sensors to read fuel tank values from pins.
    // periodically.
    auto analog_input_1 = std::make_shared<AnalogInput>(
        kAnalogInputpin_1, kAnalogInputReadInterval, kAnalogInputConfigPath_1);
    auto analog_input_2 = std::make_shared<AnalogInput>(
        kAnalogInputpin_2, kAnalogInputReadInterval, kAnalogInputConfigPath_2);
    auto analog_input_3 = std::make_shared<AnalogInput>(
        kAnalogInputpin_3, kAnalogInputReadInterval, kAnalogInputConfigPath_3);
    auto analog_input_4 = std::make_shared<AnalogInput>(
        kAnalogInputpin_4, kAnalogInputReadInterval, kAnalogInputConfigPath_4);
    auto analog_input_5 = std::make_shared<AnalogInput>(
        kAnalogInputpin_5, kAnalogInputReadInterval, kAnalogInputConfigPath_5);
    auto analog_input_6 = std::make_shared<AnalogInput>(
        kAnalogInputpin_6, kAnalogInputReadInterval, kAnalogInputConfigPath_6);

    analog_input_1->connect_to(&tank1_raw_adc);
    analog_input_2->connect_to(&tank2_raw_adc);
    analog_input_3->connect_to(&tank3_raw_adc);
    analog_input_4->connect_to(&tank4_raw_adc);
    analog_input_5->connect_to(&tank5_raw_adc);
    analog_input_6->connect_to(&tank6_raw_adc);

    ConfigItem(analog_input_1)
        ->set_title("Stbd Fuel Tank Analog Input")
        ->set_description("Analog input read interval.")
        ->set_sort_order(1000);

    ConfigItem(analog_input_2)
        ->set_title("Port Fuel Tank Analog Input")
        ->set_description("Analog input read interval.")
        ->set_sort_order(1000);

    ConfigItem(analog_input_3)
        ->set_title("Black Water Tank Analog Input")
        ->set_description("Analog input read interval.")
        ->set_sort_order(1000);

    ConfigItem(analog_input_4)
        ->set_title("Port Aft Water Tank Analog Input")
        ->set_description("Analog input read interval.")
        ->set_sort_order(1000);

    ConfigItem(analog_input_5)
        ->set_title("Stbd Water Tank Analog Input")
        ->set_description("Analog input read interval.")
        ->set_sort_order(1000);

    ConfigItem(analog_input_6)
        ->set_title("Port Fwd Water Tank Analog Input")
        ->set_description("Analog input read interval.")
        ->set_sort_order(1000);

    // Add an observer that grabs current value of the analog input
    // every time it changes.
    // analog_input_1->attach([analog_input_1]() {
    // debugD("Analog input 1 Port fuel: %f", analog_input_1->get());
    //});
    // analog_input_2->attach([analog_input_2]() {
    // debugD("Analog input 2 Stbd fuel: %f", analog_input_2->get());
    //});
    // analog_input_3->attach([analog_input_3]() {
    // debugD("Analog input 3 Black Water: %f", analog_input_3->get());
    //});
    // analog_input_4->attach([analog_input_4]() {
    // debugD("Analog input 4 fw1 Port Aft: %f", analog_input_4->get());
    //});
    // analog_input_5->attach([analog_input_5]() {
    // debugD("Analog input 5 fw2 Stbd: %f", analog_input_5->get());
    //});
    // analog_input_6->attach([analog_input_6]() {
    // debugD("Analog input 6 fw3 Port Fwd: %f", analog_input_6->get());
    //});

    // A Linear transform takes its input, multiplies it by the multiplier, then
    // adds the offset, to calculate its output. In this example, we want to see
    // the final output presented as a percentage, where empty = 0% and full =
    // 100%.
    // We work out what the multiplier is by taking the 'full tank' reading and
    // dividing by 100.

    const char *linear_config_path_1 = "/fuel_tank_1/linear";
    const char *linear_config_path_2 = "/fuel_tank_2/linear";
    const char *linear_config_path_3 = "/blackwater_tank_1/linear";
    const char *linear_config_path_4 = "/freshwater_tank_1/linear";
    const char *linear_config_path_5 = "/freshwater_tank_2/linear";
    const char *linear_config_path_6 = "/freshwater_tank_3/linear";

    const float multiplier_1 = 0.00415;
    const float multiplier_2 = 0.00415;
    const float multiplier_3 = 0.000000623; // Black Water Tank: 52cm=100%, 21.3cm=40.28%
    const float multiplier_4 = 0.005;
    const float multiplier_5 = 0.005;
    const float multiplier_6 = 0.00425;

    const float offset_1 = -0.220314; // because the sensors output something
                                      // more than zero at empty position
    const float offset_2 = -0.220314;
    const float offset_3 = -0.047; // Black Water Tank: 52cm=100%, 21.3cm=40.28%
    const float offset_4 = -0.220314;
    const float offset_5 = -0.220314;
    const float offset_6 = -0.189470;

    // Create a linear transform for calibrating the raw input value.
    // Connect the analog input to the linear transform.

    auto input_calibration_1 =
        new Linear(multiplier_1, offset_1, linear_config_path_1);
    analog_input_1->connect_to(input_calibration_1);
    auto input_calibration_2 =
        new Linear(multiplier_2, offset_2, linear_config_path_2);
    analog_input_2->connect_to(input_calibration_2);
    auto input_calibration_3 =
        new Linear(multiplier_3, offset_3, linear_config_path_3);
    analog_input_3->connect_to(input_calibration_3);
    auto input_calibration_4 =
        new Linear(multiplier_4, offset_4, linear_config_path_4);
    analog_input_4->connect_to(input_calibration_4);
    auto input_calibration_5 =
        new Linear(multiplier_5, offset_5, linear_config_path_5);
    analog_input_5->connect_to(input_calibration_5);
    auto input_calibration_6 =
        new Linear(multiplier_6, offset_6, linear_config_path_6);
    analog_input_6->connect_to(input_calibration_6);

    input_calibration_1->connect_to(&tank1_calibrated);
    input_calibration_1->connect_to(new LambdaConsumer<float>([](float value) {
      tank1_percent.set(value * 100.0f);
    }));
    input_calibration_2->connect_to(&tank2_calibrated);
    input_calibration_2->connect_to(new LambdaConsumer<float>([](float value) {
      tank2_percent.set(value * 100.0f);
    }));
    input_calibration_3->connect_to(&tank3_calibrated);
    input_calibration_3->connect_to(new LambdaConsumer<float>([](float value) {
      tank3_percent.set(value * 100.0f);
    }));
    input_calibration_4->connect_to(&tank4_calibrated);
    input_calibration_4->connect_to(new LambdaConsumer<float>([](float value) {
      tank4_percent.set(value * 100.0f);
    }));
    input_calibration_5->connect_to(&tank5_calibrated);
    input_calibration_5->connect_to(new LambdaConsumer<float>([](float value) {
      tank5_percent.set(value * 100.0f);
    }));
    input_calibration_6->connect_to(&tank6_calibrated);
    input_calibration_6->connect_to(new LambdaConsumer<float>([](float value) {
      tank6_percent.set(value * 100.0f);
    }));

    // Create a ConfigItem for the linear transform.

    ConfigItem(input_calibration_1)
        ->set_title("Input Calibration - Stbd Fuel Tank")
        ->set_description("Analog input value adjustment.")
        ->set_sort_order(1100);
    ConfigItem(input_calibration_2)
        ->set_title("Input Calibration - Port Fuel Tank")
        ->set_description("Analog input value adjustment.")
        ->set_sort_order(1100);
    ConfigItem(input_calibration_3)
        ->set_title("Input Calibration - Black Water Tank")
        ->set_description("Analog input value adjustment.")
        ->set_sort_order(1100);
    ConfigItem(input_calibration_4)
        ->set_title("Input Calibration - Port Aft Water Tank")
        ->set_description("Analog input value adjustment.")
        ->set_sort_order(1100);
    ConfigItem(input_calibration_5)
        ->set_title("Input Calibration - Stbd Water Tank")
        ->set_description("Analog input value adjustment.")
        ->set_sort_order(1100);
    ConfigItem(input_calibration_6)
        ->set_title("Input Calibration - Port Fwd Water Tank")
        ->set_description("Analog input value adjustment.")
        ->set_sort_order(1100);

    
    // Subscribe to Signal K time from environment.time
    // new SKTime("environment.time");
    // Connect the calibration output to the Signal K output.
    // This will send the calibrated value to the Signal K server
    // on the specified Signal K path. As part of
    // that output, send some metadata to indicate that the "units"
    // to be used to display this value is "ratio". Also specify that
    // the display name for this value, to be used by any Signal K
    // consumer that displays it.

    // If you want to make the SK Output path configurable, you can
    // assign the SKOutputFloat to a variable and then call
    // ConfigItem on that variable. In that case, config_path needs to be
    // defined in the constructor of the SKOutputFloat.

    input_calibration_1->connect_to(new SKOutputFloat(
        sk_path_1, "", new SKMetadata("ratio", "Stbd Fuel Tank")));

    input_calibration_2->connect_to(new SKOutputFloat(
        sk_path_2, "", new SKMetadata("ratio", "Port Fuel Tank")));

    input_calibration_3->connect_to(new SKOutputFloat(
        sk_path_3, "", new SKMetadata("ratio", "Black Water Tank")));

    input_calibration_4->connect_to(new SKOutputFloat(
        sk_path_4, "", new SKMetadata("ratio", "Port Aft Water Tank")));

    input_calibration_5->connect_to(new SKOutputFloat(
        sk_path_5, "", new SKMetadata("ratio", "Stbd Water Tank")));

    input_calibration_6->connect_to(new SKOutputFloat(
        sk_path_6, "", new SKMetadata("ratio", "Port Fwd Water Tank")));

    
    SmartSwitchController* controllerBuz = new SmartSwitchController();

    /* To store the calibrated values in simple float variables,
    you can use lambda functions to update these variables whenever the values
    change*/

    input_calibration_1->connect_to(new LambdaConsumer<float>(
        [](float value) { bV[0] = value; display_bV[1] = value; })); // Stbd Fuel -> displayed second
    input_calibration_2->connect_to(new LambdaConsumer<float>(
        [](float value) { bV[1] = value; display_bV[0] = value; })); // Port Fuel -> displayed first
    input_calibration_3->connect_to(new LambdaConsumer<float>(
        [](float value) { bV[2] = value; display_bV[2] = value; })); // Black Water
    input_calibration_4->connect_to(new LambdaConsumer<float>(
        [](float value) { bV[3] = value; display_bV[3] = value; })); // Port Aft Fresh Water
    input_calibration_5->connect_to(new LambdaConsumer<float>(
        [](float value) { bV[5] = value; display_bV[5] = value; })); // Port Forward Fresh Water
    input_calibration_6->connect_to(new LambdaConsumer<float>(
        [](float value) { bV[4] = value; display_bV[4] = value; })); // Stbd Fresh Water
    
    // Create the DigitalOutput NOW - right before we need it
    // Ensure pin is firmly set to HIGH (OFF) before creating the object
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);  // Give it a moment to settle
    digitalWrite(BUZZER_PIN, HIGH);  // Set again to be absolutely sure
    
    buzzer_switch = std::make_shared<sensesp::DigitalOutput>(BUZZER_PIN);
    
    // Override any defaults the DigitalOutput might have loaded
    buzzer_sounding = true;  // Mark as ON to force state change
    setBuzzerOutput(false);   // Force pin HIGH
    delay(10);
    digitalWrite(BUZZER_PIN, HIGH);  // One more explicit set to be absolutely certain
    
    Serial.printf("DEBUG: Post-DigitalOutput creation - BUZZER_PIN state: %d\n", digitalRead(BUZZER_PIN));
    
    controllerBuz->connect_to(buzzer_switch);

    auto* sk_listener_buzz = new BoolSKPutRequestListener(sk_path_buzzer_alarm);
    sk_listener_buzz->connect_to(new LambdaConsumer<bool>([controllerBuz](bool value) {
        controllerBuz->swich_consumer_.set(value);
    }));

    buzzer_switch->connect_to(new Repeat<bool, bool>(10003))
      ->connect_to(new SKOutputBool(sk_path_buzzer_alarm, config_path_sk_output));

    // Force buzzer OFF immediately at boot to override any saved DigitalOutput state
    // This runs once before anything else can interfere
    event_loop()->onDelay(100, []() {
        digitalWrite(BUZZER_PIN, HIGH);  // Physically ensure pin is HIGH (OFF)
        buzzer_sounding = true;  // Force state change on next setBuzzerOutput call
        setBuzzerOutput(false);   // Ensure all state is synchronized to OFF
        Serial.println("DEBUG: Boot-time buzzer force-OFF executed");
    });

    // Use RepeatSensor to call `updateTankValues` every 60 second
    event_loop()->onRepeat(
        60000,
        []()
        {
            // Perform a full screen refresh every 30 minutes
            if (refresh_counter++ > (30)) 
            {
                refresh_counter = 0;
                epaper_refresh();  // This now handles everything
            } else {
                epaper_update();   // Fast partial update
            }

            // Log the updated bar values to the terminal in tabular format
            Serial.println("  FS  FP  BW   PFFW   SFW   PAFW");
            Serial.printf("%4.2f %4.2f %4.2f %4.2f %4.2f %4.2f\n", bV[0], bV[1], bV[2], bV[3], bV[4], bV[5]);
            
      
        });

        event_loop()->onRepeat(
        1000,
        []()
        {
            // Skip auto-alarm check for 30 seconds after boot (allows sensors to stabilize)
            if (millis() - boot_time < ALARM_STARTUP_DELAY) {
                return;
            }

            // Auto-alarm: only sound if user has enabled the buzzer function
            if (buzzer_enabled) {
                if (bV[2] > 0.90f) {  // Black water > 90%
                    if (bw_over90_start == 0) {
                        bw_over90_start = millis();
                    }
                    else if (millis() - bw_over90_start > 10000) { // 10 seconds
                        buzzer_active = true;
                        requestBuzzer(true); // sound buzzer (only if enabled)
                    }
                } else {
                    bw_over90_start = 0;
                    buzzer_active = false;
                    // if not in manual short beep window, ensure buzzer off
                    if (millis() > buzzer_beep_until) {
                        requestBuzzer(false);
                    }
                }
            } else {
                // buzzer disabled -> always off
                bw_over90_start = 0;
                buzzer_active = false;
                setBuzzerOutput(false);
            }
        });

        event_loop()->onRepeat(
            20, // Poll every 20 ms for better responsiveness
            []()
            {
                int buz_val = touchRead(BUZ_CTRL_PIN);
                int disp_val = touchRead(DISPLAY_CTRL_PIN);
                bool buz_now = buz_val < TOUCH_THRESHOLD;
                bool disp_now = disp_val < TOUCH_THRESHOLD;
                 // Debug: print touch values
                 // Serial.printf("Buz: %d, Disp: %d\n", buz_val, disp_val);

                // Buzzer pad pressed (rising edge)
                if (buz_now && !last_buz_state) {
                    // toggle enabled/disabled preference and update icon
                    buzzer_enabled = !buzzer_enabled;
                    buzzerStatus = buzzer_enabled; // icon follows enabled state

                    Serial.printf("Buzzer function %s by touch!\n", buzzer_enabled ? "ENABLED" : "DISABLED");

                    if (!buzzer_enabled) {
                        // ensure buzzer physically off immediately
                        requestBuzzer(false);
                    } else {
                        // provide a brief beep to confirm enable, non-blocking
                        buzzer_beep_until = millis() + 200;
                        requestBuzzer(true);
                    }

                    if (millis() - last_epaper_update > epaper_update_delay) {
                        epaper_update();
                        last_epaper_update = millis();
                        refresh_counter = 0;
                        Serial.println("Display updated to reflect Buzzer state change!");
                    }
                }

                // Display pad pressed (rising edge)
                if (disp_now && !last_disp_state) {
                    if (millis() - last_epaper_update > epaper_update_delay) {
                        epaper_update();
                        last_epaper_update = millis();
                        refresh_counter = 0;
                        Serial.println("Display updated by touch!");
                        
                        // Debug output...
                        Serial.println("\n========== TANK VALUES (0-1 ratio) ==========");
                        Serial.printf("Stbd Fuel Tank:           %f\n", bV[0]);
                        Serial.printf("Port Fuel Tank:           %f\n", bV[1]);
                        Serial.printf("Black Water Tank:         %f\n", bV[2]);
                        Serial.printf("Port Aft Fresh Water:     %f\n", bV[3]);
                        Serial.printf("Stbd Fresh Water Tank:    %f\n", bV[4]);
                        Serial.printf("Port Forward Fresh Water: %f\n", bV[5]);
                        Serial.println("===========================================\n");
                        
                    } else {
                        Serial.println("Display update skipped - too soon after last update");
                    }
                }
        
                // Both pads pressed (rising edge) -> non-blocking restart scheduling
                if (buz_now && disp_now) {
                    if (both_pressed_start == 0) {
                        both_pressed_start = millis();
                        both_pressed_buzzer_activated = false;
                        restart_scheduled = false;
                        restart_scheduled_at = 0;
                        Serial.println("Both buttons pressed!");
                    }

                    // give brief one-time buzzer feedback at first detection (not repeated)
                    if (!both_pressed_buzzer_activated) {
                        requestBuzzer(true);
                        both_pressed_buzzer_activated = true;
                    }

                    // if held long enough, schedule restart once
                    if (!restart_scheduled && millis() - both_pressed_start >= RESTART_HOLD_MS) {
                        restart_scheduled = true;
                        restart_scheduled_at = millis() + RESTART_FEEDBACK_MS;
                        Serial.println("Both buttons held long enough; scheduling restart...");
                        // indicate pending restart (LED on)
                        digitalWrite(LED_PIN, HIGH);
                    }

                    // if a restart has been scheduled and time reached, perform restart (non-blocking)
                    if (restart_scheduled && restart_scheduled_at != 0 && millis() >= restart_scheduled_at) {
                        Serial.println("Restarting now.");
                        Serial.flush();
                        // do minimal final actions then restart
                        requestBuzzer(false); // turn buzzer off to avoid stuck sound during reboot
                        ESP.restart();
                    }

                } else {
                    // released: reset timers and any pending scheduled restart
                    both_pressed_start = 0;
                    both_pressed_buzzer_activated = false;
                    restart_scheduled = false;
                    restart_scheduled_at = 0;
                    digitalWrite(LED_PIN, LOW);
                }

                // release short manual beep if time elapsed
                if (buzzer_beep_until != 0 && millis() > buzzer_beep_until) {
                    // don't turn off if auto alarm is active
                    if (!buzzer_active) {
                        requestBuzzer(false);
                    }
                    buzzer_beep_until = 0;
                }

                last_buz_state = buz_now;
                last_disp_state = disp_now;
            }
        );

    event_loop()->onRepeat(
    1000,
    []() {
        extern bool show_boot_status;
        extern unsigned long boot_status_start;
        if (show_boot_status && millis() - boot_status_start > 15000) {
            show_boot_status = false;
            epaper_update(); // Redraw status bar with normal info
        }
    }
);

    Serial.print("Free heap after app init: ");
    Serial.println(ESP.getFreeHeap());
    
    // Final verification that buzzer pin is OFF
    Serial.printf("DEBUG: BUZZER_PIN (%d) state at END of setup: %d (should be 1 for HIGH/OFF)\n", BUZZER_PIN, digitalRead(BUZZER_PIN));
    Serial.printf("DEBUG: buzzer_sounding = %d, buzzer_enabled = %d\n", buzzer_sounding, buzzer_enabled);

    // Check if any blocking delays are in setup()
    // Look for: delay(xxx), long loops, heavy processing

}

void loop() { event_loop()->tick(); }
