// Signal K application template file.
//
// This is a line by line rebuild of the tank system
// This will draw in pin connections and the analogue readings but at V 1 will
// not include display which will be in v 3.1 

#include <memory>

#include "sensesp.h"
#include "sensesp/sensors/analog_input.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/transforms/linear.h"
// #include "sensesp/system/lambda_consumer.h"
// #include "ep_display.h"  // Include the header file for the display functions
#include "sensesp_app_builder.h"

/*******************************************************/

#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h>      // 4.2" b/w
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <Fonts/FreeSans9pt7b.h>            // Clear sans-serif font
#include <Fonts/FreeSansBold9pt7b.h>        // Clear sans-serif font in bold
#include <TimeLib.h>                      // For handling time functions
#include <WiFi.h>                          // For WiFi status
#include <math.h>  // For cos(), sin(), radians()


#define D_RST_PIN   26
#define D_DC_PIN    27
#define D_CS_PIN    15
#define D_BZ_PIN    25

// === BAR GRAPH CONFIGURATION ===
#define NUM_BARS            6
#define BAR_HEIGHT          36
#define BAR_SPACING_Y       8
#define MAX_BAR_WIDTH       (display.width() - 10)
#define BAR_START_X         5
#define BAR_START_Y         5
#define TICK_INTERVAL_PCT   10
#define TICK_LENGTH         8
#define TICK_THICKNESS      3  // Tick thickness

#define WATER_PORT_FWD 0
#define WATER_PORT_AFT 1
#define WATER_STBD 2
#define FUEL_PORT 3
#define FUEL_STBD 4
#define BLACK_WATER 5


// === STATUS AREA CONFIGURATION ===
#define STATUS_HEIGHT        28   // Height for the status area
#define STATUS_BASE_Y        (display.height() - STATUS_HEIGHT)

// forward prototype

void epDisplayInit();
// function to update the display after every second
void epDisplayUpdate();
void drawBarGraphs();
void drawStatusArea();
void drawBuzzerIcon(int16_t x, int16_t y, bool status);
void drawWiFiIcon(int16_t x, int16_t y, int signalStrength);
void drawArc(int16_t x, int16_t y, int16_t width, int16_t height, int16_t startAngle, int16_t endAngle, int numSegments);

void setBarValue(int index, uint8_t value) ;

GxIO_Class io(SPI, /*CS=*/ D_CS_PIN, /*DC=*/ D_DC_PIN, /*RST=*/ D_RST_PIN);
GxEPD_Class display(io, /*RST=*/ D_RST_PIN, /*BUSY=*/ D_BZ_PIN);

uint16_t sec_counter = 0;

const char* barLabels[NUM_BARS] = {
  "Water Port Fwd",
  "Water Port Aft",
  "Water Stbd",
  "Fuel Port",
  "Fuel Stbd",
  "Black Water"
};
uint8_t barValues[NUM_BARS] = { 0, 0, 0, 0, 0, 0};

// Buzzer status and WiFi signal strength (Example data only)
bool buzzerStatus = true;
int wifiSignalLevel = -80;  // Example signal level (in dBm)
char lastUpdateTime[20];    // For storing the last update time


// Added flag to track whether the bar value is increasing or decreasing
bool increasing[NUM_BARS] = {false, false, false, false, false, false};  // Initially, set to decreasing for all bars



/*******************************************************/



using namespace sensesp;
// Test comment
// The setup function performs one-time application initialization.
void setup() {
  SetupLogging(ESP_LOG_DEBUG);

  //Init the display
  epDisplayInit();    
  Serial.println("Display initiated");
  

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
      [](float value) { setBarValue(WATER_PORT_FWD, (uint8_t)value); }));


  // Use RepeatSensor to call `updateTankValues` every 1 second
  event_loop()->onRepeat(
  1000,
  []() { 
    epDisplayUpdate();
    Serial.println("Graph Update!");
 }
);

}

void loop() { event_loop()->tick(); }


/**********************************************************/


void epDisplayInit(){
    display.init(115200);
  
    // Simulating time for testing purposes (You can replace this with real-time function)
    setTime(0, 8, 23, 4, 5, 25);  // 04/05/25 00:08:23
  
    drawBarGraphs();
    drawStatusArea();
    
    display.update();
    display.powerDown();
}

// function to update the display after every second
void epDisplayUpdate(){
    if(sec_counter >= 120){
        sec_counter = 0;
        display.fillScreen(GxEPD_WHITE); // Clear the screen
    }
    else{
        sec_counter++;
    }

    drawBarGraphs();
    drawStatusArea();
    display.update(); // Update the screen

    // Update the time every loop based on the arbitrary start time
    second(); // Updates the real-time clock from TimeLib
    minute();
    hour();
    day();
    month();
    year();
}

void drawBarGraphs() {
    display.setRotation(0);
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeSansBold9pt7b);  // Use the new bold sans-serif font
  
    for (int i = 0; i < NUM_BARS; i++) {
      int16_t y0 = BAR_START_Y + i * (BAR_HEIGHT + BAR_SPACING_Y);
      int16_t x0 = BAR_START_X;
  
      // Draw horizontal axis ticks along the top of this bar with custom thickness
      for (int pct = 0; pct <= 100; pct += TICK_INTERVAL_PCT) {
        int16_t x_tick = x0 + (MAX_BAR_WIDTH * pct) / 100;
        display.fillRect(x_tick - TICK_THICKNESS / 2, y0 - (TICK_LENGTH / 2), TICK_THICKNESS, TICK_LENGTH, GxEPD_BLACK);
      }
  
      // Draw filled black bar representing the value
      int16_t barW = (MAX_BAR_WIDTH * barValues[i]) / 100;
      display.fillRect(x0, y0, barW, BAR_HEIGHT, GxEPD_BLACK);
  
      // Draw white outline around the full-length area (optional)
      display.drawRect(x0, y0, MAX_BAR_WIDTH, BAR_HEIGHT, GxEPD_BLACK);
  
      // Prepare the label with the numeric value and percentage
      char labelWithValue[50];
      snprintf(labelWithValue, sizeof(labelWithValue), "%s - %d%%", barLabels[i], barValues[i]);
  
      // Calculate the text bounds for the new label
      int16_t tbx, tby; uint16_t tbw, tbh;
      display.getTextBounds(labelWithValue, 0, 0, &tbx, &tby, &tbw, &tbh);
  
      int16_t lx, ly;
  
      // If the value is <= 50%, display the text on the right side of the screen in black text on white background
      if (barValues[i] <= 50) {
        lx = display.width() / 2 + 5;  // Start text just after the middle of the screen
        ly = y0 + (BAR_HEIGHT + tbh - 20) / 2 - tby;  // Vertically center text
  
        // Draw a white background for the text (black text on white)
        display.fillRect(lx - 5, ly - tby - 2, tbw + 10, tbh + 4, GxEPD_WHITE);  // White background for text
        display.setTextColor(GxEPD_BLACK);  // Black text
        display.setCursor(lx, ly);
        display.print(labelWithValue);
      }
      // If the value is >= 51%, display the text on the left side of the bar in white text on black background
      else {
        lx = x0 + 8;  // Start text just after the left margin of the screen.
        ly = y0 + (BAR_HEIGHT + tbh - 20) / 2 - tby;  // Vertically center the text inside the bar
  
        // Draw black background for the text (white text on black)
        display.fillRect(x0, y0, barW, BAR_HEIGHT, GxEPD_BLACK);  // Ensure the bar is filled black
        display.setTextColor(GxEPD_WHITE);  // White text on black background
        display.setCursor(lx, ly);
        display.print(labelWithValue);
      }
    }
  }
  
  
  void drawStatusArea() {
    // Draw the status area (black background)
    display.fillRect(0, STATUS_BASE_Y, display.width(), STATUS_HEIGHT, GxEPD_BLACK);
  
    // Draw Buzzer icon (example with status)
    drawBuzzerIcon(10, STATUS_BASE_Y + 6, buzzerStatus);
  
    // Draw WiFi Signal Icon (WiFi signal strength)
    drawWiFiIcon(50, STATUS_BASE_Y + 0, wifiSignalLevel);
  
    // Set the font to bold for the update text
    display.setFont(&FreeSansBold9pt7b);  // Bold font for "Update:" and date/time
  
    // Prepare the time/date text and a hyphen
    snprintf(lastUpdateTime, sizeof(lastUpdateTime), "%02d:%02d:%02d - %0d/%0d/%0d", hour(), minute(), second(), day(), month(), year());
  
    // Set text color to white
    display.setTextColor(GxEPD_WHITE);  
  
    // Position for the last update text
    display.setCursor(167, STATUS_BASE_Y + 18);  
    display.print("Update: ");
    display.print(lastUpdateTime);
  }
  
  
  // Function to draw a more intuitive Buzzer Icon with thicker lines
  void drawBuzzerIcon(int16_t x, int16_t y, bool status) {
    int iconSize = 16;  // Keep the icon size the same
  
    // Draw a larger speaker box with thicker lines
    display.drawRect(x, y, iconSize, iconSize, GxEPD_WHITE);  // Speaker box with thicker lines
  
    if (status) {
      // Draw sound waves (icon for "on") with thicker lines
      display.drawLine(x + iconSize, y + 3, x + iconSize + 10, y + 6, GxEPD_WHITE);
      display.drawLine(x + iconSize, y + 6, x + iconSize + 10, y + 9, GxEPD_WHITE);
      display.drawLine(x + iconSize, y + 9, x + iconSize + 10, y + 12, GxEPD_WHITE);
    } else {
      // Draw a cross (icon for "off") with thicker lines
      display.drawLine(x, y, x + iconSize, y + iconSize, GxEPD_WHITE);  // Cross from top left to bottom right
      display.drawLine(x + iconSize, y, x, y + iconSize, GxEPD_WHITE);  // Cross from top right to bottom left
    }
  }
  
  // Function to draw a more intuitive WiFi Icon with thicker lines
  void drawWiFiIcon(int16_t x, int16_t y, int signalStrength) {
    int numBars = 0;
    if (signalStrength > -70) numBars = 3;    // Strong signal
    else if (signalStrength > -80) numBars = 2; // Moderate signal
    else if (signalStrength > -90) numBars = 1; // Weak signal
  
    int barSpacing = 6;  // Wider spacing between bars
    int barWidth = 3;    // Thicker bars
    int barHeight = 6;   // Slightly taller bars
  
    // Draw WiFi bars with thicker lines
    for (int i = 0; i < 3; i++) {
      if (i < numBars) {
        display.fillRect(x + (i * barSpacing), y + (3 - i) * barHeight, barWidth, barHeight, GxEPD_WHITE);  // Solid bars
      } else {
        display.drawRect(x + (i * barSpacing), y + (3 - i) * barHeight, barWidth, barHeight, GxEPD_WHITE);  // Empty bars
      }
    }
  
    // Draw arcs manually using lines
    if (numBars > 0) {
      drawArc(x, y, 16, 16, 0, 180, 3); // Approximate arc for strongest signal
    }
    if (numBars > 1) {
      drawArc(x, y, 20, 20, 0, 180, 4); // Approximate arc for moderate signal
    }
    if (numBars > 2) {
      drawArc(x, y, 24, 24, 0, 180, 5); // Approximate arc for strongest signal
    }
  }
  
  // Function to approximate an arc using lines
  void drawArc(int16_t x, int16_t y, int16_t width, int16_t height, int16_t startAngle, int16_t endAngle, int numSegments) {
    // This function approximates an arc using line segments
    for (int i = startAngle; i < endAngle; i += (endAngle - startAngle) / numSegments) {
      int16_t x1 = x + cos(radians(i)) * width / 2;
      int16_t y1 = y - sin(radians(i)) * height / 2;
      int16_t x2 = x + cos(radians(i + (endAngle - startAngle) / numSegments)) * width / 2;
      int16_t y2 = y - sin(radians(i + (endAngle - startAngle) / numSegments)) * height / 2;
      display.drawLine(x1, y1, x2, y2, GxEPD_WHITE); // Draw the line segment
    }
  }

  void setBarValue(int index, uint8_t value) {
    if (index >= 0 && index < NUM_BARS) {
      barValues[index] = value;
    }
  }