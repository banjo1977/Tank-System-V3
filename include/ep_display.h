#ifndef EP_DISPLAY_H
#define EP_DISPLAY_H

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
#endif // EP_DISPLAY_H