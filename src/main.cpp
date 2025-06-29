// Signal K application template file.
//
// This is a line by line rebuild of the tank system
// V3.x - line by line rebuild, data only.
// V4.x - Display on paper white display
// V5.x - Add touch sensitive pads to control buzzer and display refresh.  ALso add clock (time form signalk)
// V5.0.6 - Add IP address to boot status message and correct date to include 2 digit year.
// And boot status message (IP address, software version, etc.)
// V5.0.7 - Amend touch logic - debounce and improe response.
//  Need to:
// validate function of touch sensors on real hardware (or swap for switches)
// Amend the timing 

// Functionality for two touch-sensitive pads:
// Pad 1 - update the display now (ie, don't wait 120 seconds).
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
#include <time.h>
#include "sensesp/net/networking.h"


const char* SOFTWARE_VERSION = "v5.0.7"; // Update as needed
#define BUZ_CTRL_PIN 12 // Touch Pad 1
#define DISPLAY_CTRL_PIN 4 // Touch Pad 2
#define TOUCH_THRESHOLD 17 // Define threshold for touch sensitivity
#define LED_PIN 2  // Change to your board's LED GPIO if different


float bV[NUM_BARS] = {0, 0, 0, 0, 0, 0};
int refresh_counter = 0;

const int BUZZER_PIN = 19; // Pin 19 for buzzer control.
bool buzzer_enabled = true;
bool buzzerStatus = false; // variabe to control the buzzer icon. 
unsigned long bw_over90_start = 0;
unsigned long buzzer_beep_until = 0; // Timer for buzzer beep duration

bool buzzer_active = true;
auto buzzer_switch = std::make_shared<sensesp::DigitalOutput>(BUZZER_PIN); // inverted logic so must be 'On' for off and 'Off' for on....
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;
// Debounce state for touch pads
static bool last_buz_state = false;
static bool last_disp_state = false;


using namespace sensesp;

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

    Serial.print("Software version: ");
    Serial.println(SOFTWARE_VERSION);

    Serial.print("WiFi SSID: ");
    Serial.println(WiFi.SSID());

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    epaper_init();

    pinMode(BUZ_CTRL_PIN, INPUT); // Set up the buzzer control pad
    pinMode(DISPLAY_CTRL_PIN, INPUT); // Set up the display control pad
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH); // Ensure buzzer is OFF (HIGH) before anything else
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); // Turn LED OFF (for most boards, LOW = off)

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
    const float multiplier_3 = 0.005;
    const float multiplier_4 = 0.005;
    const float multiplier_5 = 0.005;
    const float multiplier_6 = 0.00425;

    const float offset_1 = -0.220314; // because the sensors output something
                                      // more than zero at empty position
    const float offset_2 = -0.220314;
    const float offset_3 = -0.220314;
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
        [](float value)
        { bV[0] = value; }));
    input_calibration_2->connect_to(new LambdaConsumer<float>(
        [](float value)
        { bV[1] = value; }));
    input_calibration_3->connect_to(new LambdaConsumer<float>(
        [](float value)
        { bV[2] = value; }));
    input_calibration_4->connect_to(new LambdaConsumer<float>(
        [](float value)
        { bV[3] = value; }));
    input_calibration_5->connect_to(new LambdaConsumer<float>(
        [](float value)
        { bV[4] = value; }));
    input_calibration_6->connect_to(new LambdaConsumer<float>(
        [](float value)
        { bV[5] = value; }));

    
    controllerBuz->connect_to(buzzer_switch);

    auto* sk_listener_buzz = new StringSKPutRequestListener(sk_path_buzz);
    
    sk_listener_buzz->connect_to(new Repeat<bool, bool>(10000))
      ->connect_to(new SKOutputBool(sk_path_buzz, config_path_sk_output));

    buzzer_switch->connect_to(new Repeat<bool, bool>(10003))
      ->connect_to(new SKOutputBool(sk_path_buzz, config_path_sk_output));
      buzzer_switch->set(true); // Pin HIGH, buzzer OFF

    // Use RepeatSensor to call `updateTankValues` every 30 second
    event_loop()->onRepeat(
        60000,
        []()
        {
            // Update barValues from bV (convert ratio to percentage)
            for (int i = 0; i < NUM_BARS; i++)
            {
                float pct = bV[i] * 100.0f;
                if (pct < 0)
                    pct = 0;
                if (pct > 100)
                    pct = 100;
                epaper_setValue(i, static_cast<uint8_t>(pct));
            }

            // Perform a full screen refresh every 30 minutes
            if (refresh_counter++ > (30)) 
            {
                refresh_counter = 0;
                epaper_refresh();
            }
            epaper_update();

            // Log the updated bar values to the terminal in tabular format
            Serial.println("  FS  FP  BW   PFFW   SFW   PAFW");
            Serial.printf("%4.2f %4.2f %4.2f %4.2f %4.2f %4.2f\n", bV[0], bV[1], bV[2], bV[3], bV[4], bV[5]);
        });

        event_loop()->onRepeat(
        1000,
        []()
        {
            float blackwater_pct = bV[2] * 100.0f;
            unsigned long now = millis();
            // Check if the Black Water tank is over 90% for more than 10 seconds
            if (buzzer_enabled && bV[2] > 90.0f) {
                if (bw_over90_start == 0) {
                    bw_over90_start = millis(); // Start timer
                } 
                else if (millis() - bw_over90_start > 10000) { // 10 seconds
                    if (!buzzer_active) {
                        buzzer_switch->set(false); // Activate buzzer (inverted logic)
                        buzzer_active = true;
                    }
                }
            } 
            else {
                bw_over90_start = 0; // Reset timer
                if (buzzer_active) {
                    buzzer_switch->set(true); // Deactivate buzzer (inverted logic)
                    buzzer_active = false;
                }
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
                    if (buzzer_enabled) {
                        buzzer_enabled = false;
                        buzzer_switch->set(true); // Immediately turn off the buzzer (inverted logic)
                        buzzer_active = false;
                        buzzerStatus = false;
                        Serial.println("Buzzer turned OFF by touch!");
                        epaper_update();
                        epaper_refresh();
                        refresh_counter = 0;
                        Serial.println("Display updated to reflect Buzzer state change!");
                    } else {
                        buzzer_enabled = true;
                        buzzer_switch->set(false); // Turn on the buzzer (inverted logic)
                        buzzer_beep_until = millis() + 200; // 200ms beep
                        buzzer_active = true;
                        buzzerStatus = true;
                        Serial.println("Buzzer turned ON by touch!");
                        epaper_update();
                        epaper_refresh();
                        refresh_counter = 0;
                        Serial.println("Display updated to reflect Buzzer state change!");
                    }
                }
        
                // Display pad pressed (rising edge)
                if (disp_now && !last_disp_state) {
                    epaper_update();
                    epaper_refresh();
                    refresh_counter = 0;
                    Serial.println("Display updated by touch!");
                    //buzzer_switch->set(false); // Activate buzzer (inverted logic)
                    //buzzer_beep_until = millis() + 200; // 200ms beep
                }
        
                // Both pads pressed (rising edge)
                static unsigned long both_pressed_start = 0;
                if (buz_now && disp_now) {
                    Serial.println("Both buttons pressed!");
                    buzzer_switch->set(false); // Activate buzzer (inverted logic)
                    if (both_pressed_start == 0) {
                        both_pressed_start = millis();
                    } else if (millis() - both_pressed_start >= 5000) {
                        Serial.println("Both buttons held for 5 seconds. Restarting ESP32...");
                        digitalWrite(LED_PIN, HIGH); // Turn LED ON (for most boards, HIGH = on)
                        ESP.restart();
                    }
                } else {
                    both_pressed_start = 0;
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
}

void loop() { event_loop()->tick(); }
