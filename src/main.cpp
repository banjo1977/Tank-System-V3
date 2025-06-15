// Signal K application template file.
//
// This is a line by line rebuild of the tank system
// This will draw in pin connections and the analogue readings but at V 1 will
// not include display which will be in v 3.1
//  

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

#define BUZ_CTRL_PIN 12 // Touch Pad 1
#define DISPLAY_CTRL_PIN 4 // Touch Pad 2

float bV[NUM_BARS] = {0, 0, 0, 0, 0, 0};
int refresh_counter = 0;

const int BUZZER_PIN = 21; // Pin 21 for buzzer control.
// Timer for Black Water tank over 90%
bool buzzer_enabled = true;
bool buzzerStatus = true; // variabe to control the buzzer icon. 
unsigned long bw_over90_start = 0;
unsigned long buzzer_beep_until = 0; // Timer for buzzer beep duration
bool buzzer_active = false;
auto buzzer_switch = std::make_shared<sensesp::DigitalOutput>(BUZZER_PIN); // inverted logic so must be 'On' for off and 'Off' for on....

using namespace sensesp;

// The setup function performs one-time application initialization.
void setup()
{
    SetupLogging(ESP_LOG_DEBUG);

    epaper_init();

    pinMode(BUZ_CTRL_PIN, INPUT); // Set up the buzzer control pad
    pinMode(DISPLAY_CTRL_PIN, INPUT); // Set up the display control pad

    // Construct the global SensESPApp() object
    SensESPAppBuilder builder;
    sensesp_app = (&builder)
                      // Set a custom hostname for the app.
                      ->set_hostname("contour-tanksystem-v3.3")
                      // Optionally, hard-code the WiFi and Signal K server
                      // settings. This is normally not needed.
                      //->set_wifi_client("My WiFi SSID", "my_wifi_password")
                      //->set_wifi_access_point("My AP SSID", "my_ap_password")
                      //->set_sk_server("192.168.10.3", 80)
                      ->get_app();

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
    buzzer_switch->set(true); // Pin HIGH, buzzer OFF

    auto* sk_listener_buzz = new StringSKPutRequestListener(sk_path_buzz);
    
    sk_listener_buzz->connect_to(new Repeat<bool, bool>(10000))
      ->connect_to(new SKOutputBool(sk_path_buzz, config_path_sk_output));

    buzzer_switch->connect_to(new Repeat<bool, bool>(10003))
      ->connect_to(new SKOutputBool(sk_path_buzz, config_path_sk_output));
  

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
        500,
        []()
        {
            if(digitalRead(BUZ_CTRL_PIN) == HIGH) // Buzzer control pad pressed
            {
                if (buzzer_active) {
                    // If BUZ_CTRL_PIN is HIGH and buzzer is currently on, turn it off
                    buzzer_enabled = false;                // Disable automatic buzzer logic
                    buzzer_switch->set(true);             // Immediately turn off the buzzer (inverted logic)
                    buzzer_active = false;                 // Reset the active flag
                    buzzerStatus = false; // Update the status variable
                    Serial.println("Buzzer turned OFF by touch!");
                    epaper_update();
                    epaper_refresh();
                    refresh_counter = 0;
                    Serial.println("Display updated to reflect Buzzer state change!");
                } 
                else {
                    // If BUZ_CTRL_PIN is HIGH and buzzer is currently off, turn it on
                    buzzer_enabled = true;                 // Enable automatic buzzer logic
                    buzzer_switch->set(false);              // Turn on the buzzer (inverted logic)
                    buzzer_beep_until = millis() + 200; // 200ms beep
                    buzzer_active = true;                  // Set the active flag
                    buzzerStatus = true;           // Update the display icon
                    Serial.println("Buzzer turned ON by touch!");
                    epaper_update();
                    epaper_refresh();
                    refresh_counter = 0;
                    Serial.println("Display updated to reflect Buzzer state change!");
                }
            }

            if(digitalRead(DISPLAY_CTRL_PIN) == HIGH) //  control pad pressed
            {
                epaper_update();
                epaper_refresh();
                refresh_counter = 0;
                Serial.println("Display updated by touch!");
                buzzer_switch->set(false); // Activate buzzer (inverted logic)
                buzzer_beep_until = millis() + 200; // 200ms beep
            }
        });
        
        event_loop()->onRepeat(
            200,
            []()
            {
                static unsigned long both_pressed_start = 0;
                if (digitalRead(BUZ_CTRL_PIN) == HIGH && digitalRead(DISPLAY_CTRL_PIN) == HIGH) {
                                             Serial.println("Both buttons pressed!");
                                            buzzer_switch->set(false); // Activate buzzer (inverted logic)
                    // If both buttons are pressed, start the timer
                    if (both_pressed_start == 0) {
                        both_pressed_start = millis();
                    } else if (millis() - both_pressed_start >= 5000) {
                        Serial.println("Both buttons held for 5 seconds. Restarting ESP32...");
                        ESP.restart();
                    }
                } else {
                    both_pressed_start = 0;
                }
            }); 


            event_loop()->onRepeat(
    50,
    []() {
        if (buzzer_beep_until > 0 && millis() > buzzer_beep_until) {
            buzzer_switch->set(true); // Pin HIGH, buzzer OFF
            buzzer_beep_until = 0;
        }
    }
);
}

void loop() { event_loop()->tick(); }
