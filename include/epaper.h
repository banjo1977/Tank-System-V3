#ifndef EPAPER_H
#define EPAPER_H

#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h> // 4.2" b/w
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <Fonts/FreeSans9pt7b.h>     // Clear sans-serif font
#include <Fonts/FreeSansBold9pt7b.h> // Clear sans-serif font in bold
#include <TimeLib.h>                 // For handling time functions
#include <WiFi.h>                    // For WiFi status
#include <math.h>                    // For cos(), sin(), radians()


#define D_RST_PIN 26
#define D_DC_PIN 27
#define D_CS_PIN 15
#define D_BZ_PIN 25

#define NUM_BARS 6

void epaper_init();

void epaper_update();
void epaper_refresh();
void epaper_setValue(int idx, uint8_t value);

// Renamed draw* functions to epaper_*
void epaper_barGraphs();
void epaper_statusArea();
void epaper_buzzerIcon(int16_t x, int16_t y, bool status);
void epaper_setBuzzerIcon(bool status);
void epaper_wifiIcon(int16_t x, int16_t y, int signalStrength);
void epaper_Arc(int16_t x, int16_t y, int16_t width, int16_t height, int16_t startAngle, int16_t endAngle, int numSegments);

#endif