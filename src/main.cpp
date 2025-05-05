// Signal K application template file.
//
// This is a line by line rebuild of the tank system
// This will draw in pin connections and the analogue readings but at V 1 will
// not include display which will be in v 3.1 It will also not exploit the
// 'touch pins' function (the touch buttons)

#include <memory>

#include "sensesp.h"
#include "sensesp/sensors/analog_input.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/transforms/linear.h"
// #include "sensesp/system/lambda_consumer.h"
#include "ep_display.h"  // Include the header file for the display functions
#include "sensesp_app_builder.h"

using namespace sensesp;
// Test comment
// The setup function performs one-time application initialization.
void setup() {
  SetupLogging(ESP_LOG_DEBUG);

  epDisplayInit();

  // Construct the global SensESPApp() object
  SensESPAppBuilder builder;
  sensesp_app = (&builder)
                    // Set a custom hostname for the app.
                    ->set_hostname("contour-tanksystem-v3.1")
                    // Optionally, hard-code the WiFi and Signal K server
                    // settings. This is normally not needed.
                    //->set_wifi_client("My WiFi SSID", "my_wifi_password")
                    //->set_wifi_access_point("My AP SSID", "my_ap_password")
                    //->set_sk_server("192.168.10.3", 80)
                    ->get_app();

  // GPIO numbers to use for the analog inputs (linked to tank sensors)
  const uint8_t kAnalogInputpin_1 = 33;  // Stbd Fuel Tank
  const uint8_t kAnalogInputpin_2 = 34;  // Port Fuel Tank
  const uint8_t kAnalogInputpin_3 = 39;  // Black Water Tank
  const uint8_t kAnalogInputpin_4 = 32;  // Port Aft Fresh Water tank
  const uint8_t kAnalogInputpin_5 = 35;  // Stbd Fresh Water tank
  const uint8_t kAnalogInputpin_6 = 36;  // Port Forward Fresh Water tank

  const char* sk_path_1 = "tanks.fuel.1.currentLevel";  // Starboard Fuel Tank
  const char* sk_path_2 = "tanks.fuel.2.currentLevel";  // Port Fuel Tank
  const char* sk_path_3 = "tanks.blackWater.1.currentLevel";  // Black Water
                                                              // tank
  const char* sk_path_4 =
      "tanks.freshWater.1.currentLevel";  // Port Aft Fresh Water tank
  const char* sk_path_5 =
      "tanks.freshWater.2.currentLevel";  // Stbd Fresh Water tank
  const char* sk_path_6 =
      "tanks.freshWater.3.currentLevel";  // Port Forward Fresh Water tank

  // The "Configuration path" is combined with "/config" to formulate a URL
  // used by the RESTful API for retrieving or setting configuration data.
  // It is ALSO used to specify a path to the SPIFFS file system
  // where configuration data is saved on the MCU board. It should
  // ALWAYS start with a forward slash if specified. If left blank,
  // that indicates this sensor or transform does not have any
  // configuration to save, or that you're not interested in doing
  // run-time configuration.
  const char* kAnalogInputConfigPath_1 = "/fuel_tank_1/analog_in";
  const char* kAnalogInputConfigPath_2 = "/fuel_tank_2/analog_in";
  const char* kAnalogInputConfigPath_3 = "/blackwater_tank_1/analog_in";
  const char* kAnalogInputConfigPath_4 = "/freshwater_tank_1/analog_in";
  const char* kAnalogInputConfigPath_5 = "/freshwater_tank_2/analog_in";
  const char* kAnalogInputConfigPath_6 = "/freshwater_tank_3/analog_in";

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

  // Add an observer that prints out the current value of the analog input
  // every time it changes.
  // analog_input_1->attach([analog_input_1]() {
  //   debugD("Analog input 1 Port fuel: %f", analog_input_1->get());
  // });
  // analog_input_2->attach([analog_input_2]() {
  //  debugD("Analog input 2 Stbd fuel: %f", analog_input_2->get());
  // });
  // analog_input_3->attach([analog_input_3]() {
  //  debugD("Analog input 3 Black Water: %f", analog_input_3->get());
  //});
  //  analog_input_4->attach([analog_input_4]() {
  //  debugD("Analog input 4 fw1 Port Aft: %f", analog_input_4->get());
  //});
  //  analog_input_5->attach([analog_input_5]() {
  //  debugD("Analog input 5 fw2 Stbd: %f", analog_input_5->get());
  //});
  //  analog_input_6->attach([analog_input_6]() {
  //  debugD("Analog input 6 fw3 Port Fwd: %f", analog_input_6->get());
  //});

  // A Linear transform takes its input, multiplies it by the multiplier, then
  // adds the offset, to calculate its output. In this example, we want to see
  // the final output presented as a percentage, where empty = 0% and full =
  // 100%.
  // We work out what the multiplier is by taking the 'full tank' reading and
  // dividing by 100.

  const char* linear_config_path_1 = "/fuel_tank_1/linear";
  const char* linear_config_path_2 = "/fuel_tank_2/linear";
  const char* linear_config_path_3 = "/blackwater_tank_1/linear";
  const char* linear_config_path_4 = "/freshwater_tank_1/linear";
  const char* linear_config_path_5 = "/freshwater_tank_2/linear";
  const char* linear_config_path_6 = "/freshwater_tank_3/linear";

  const float multiplier_1 = 0.00415;
  const float multiplier_2 = 0.00415;
  const float multiplier_3 = 0.005;
  const float multiplier_4 = 0.005;
  const float multiplier_5 = 0.005;
  const float multiplier_6 = 0.00425;

  const float offset_1 = -0.220314;  // because the sensors output something
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

  /* To store the calibrated values in simple float variables,
  you can use lambda functions to update these variables whenever the values
  change*/

  input_calibration_1->connect_to(new LambdaConsumer<float>(
      [](float value) { setBarValue(FUEL_STBD, (uint8_t)value); }));
  input_calibration_2->connect_to(new LambdaConsumer<float>(
      [](float value) { setBarValue(FUEL_PORT, (uint8_t)value); }));
  input_calibration_3->connect_to(new LambdaConsumer<float>(
      [](float value) { setBarValue(BLACK_WATER, (uint8_t)value); }));
  input_calibration_4->connect_to(new LambdaConsumer<float>(
      [](float value) { setBarValue(WATER_PORT_AFT, (uint8_t)value); }));
  input_calibration_5->connect_to(new LambdaConsumer<float>(
      [](float value) { setBarValue(WATER_STBD, (uint8_t)value); }));
  input_calibration_6->connect_to(new LambdaConsumer<float>(
      [](float value) { setBarValue(WATER_PORT_AFT, (uint8_t)value); }));

  // Use RepeatSensor to call `updateTankValues` every 1 second
  auto periodic_task =
      new RepeatSensor<bool>(1000, []() { epDisplayUpdate();
    return true; });
}

void loop() { event_loop()->tick(); }
