
#include "epaper.h"



// === BAR GRAPH CONFIGURATION ===
#define BAR_HEIGHT 36
#define BAR_SPACING_Y 8
#define MAX_BAR_WIDTH (display.width() - 10)
#define BAR_START_X 5
#define BAR_START_Y 5
#define TICK_INTERVAL_PCT 10
#define TICK_LENGTH 8
#define TICK_THICKNESS 3 // Tick thickness



GxIO_Class io(SPI, /*CS=*/D_CS_PIN, /*DC=*/D_DC_PIN, /*RST=*/D_RST_PIN);
GxEPD_Class display(io, /*RST=*/D_RST_PIN, /*BUSY=*/D_BZ_PIN);


// === STATUS AREA CONFIGURATION ===
#define STATUS_HEIGHT 28 // Height for the status area
#define STATUS_BASE_Y (display.height() - STATUS_HEIGHT)

const char *barLabels[NUM_BARS] = {
    "Fuel Stbd",
    "Fuel Port",
    "Black Water",
    "Water Port Aft",
    "Water Port Fwd",
    "Water Stbd"    
    };

uint8_t barValues[NUM_BARS] = {0, 0, 0, 0, 0, 0}; // Initial values for the bars

// Buzzer status and WiFi signal strength (Example data only)
bool buzzerStatus = true;
int wifiSignalLevel = -80; // Example signal level (in dBm)
char lastUpdateTime[20];   // For storing the last update time



void epaper_init(){
    SPI.begin(13, -1, 14, 15); // Use your custom SPI wiring!
    display.init(115200);
    delay(100);

    // Simulating time for testing purposes (You can replace this with real-time function)
    setTime(0, 8, 23, 4, 5, 25); // 04/05/25 00:08:23

    epaper_barGraphs();
    epaper_statusArea();

    display.update();
    display.powerDown();    
}


void epaper_barGraphs()
{
    display.setRotation(0);
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeSansBold9pt7b); // Use the new bold sans-serif font

    for (int i = 0; i < NUM_BARS; i++)
    {
        int16_t y0 = BAR_START_Y + i * (BAR_HEIGHT + BAR_SPACING_Y);
        int16_t x0 = BAR_START_X;

        // Draw horizontal axis ticks along the top of this bar with custom thickness
        for (int pct = 0; pct <= 100; pct += TICK_INTERVAL_PCT)
        {
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
        int16_t tbx, tby;
        uint16_t tbw, tbh;
        display.getTextBounds(labelWithValue, 0, 0, &tbx, &tby, &tbw, &tbh);

        int16_t lx, ly;

        // If the value is <= 50%, display the text on the right side of the screen in black text on white background
        if (barValues[i] <= 50)
        {
            lx = display.width() / 2 + 5;                // Start text just after the middle of the screen
            ly = y0 + (BAR_HEIGHT + tbh - 20) / 2 - tby; // Vertically center text

            // Draw a white background for the text (black text on white)
            display.fillRect(lx - 5, ly - tby - 2, tbw + 10, tbh + 4, GxEPD_WHITE); // White background for text
            display.setTextColor(GxEPD_BLACK);                                      // Black text
            display.setCursor(lx, ly);
            display.print(labelWithValue);
        }
        // If the value is >= 51%, display the text on the left side of the bar in white text on black background
        else
        {
            lx = x0 + 8;                                 // Start text just after the left margin of the screen.
            ly = y0 + (BAR_HEIGHT + tbh - 20) / 2 - tby; // Vertically center the text inside the bar

            // Draw black background for the text (white text on black)
            display.fillRect(x0, y0, barW, BAR_HEIGHT, GxEPD_BLACK); // Ensure the bar is filled black
            display.setTextColor(GxEPD_WHITE);                       // White text on black background
            display.setCursor(lx, ly);
            display.print(labelWithValue);
        }
    }
}

void epaper_statusArea()
{
    // Draw the status area (black background)
    display.fillRect(0, STATUS_BASE_Y, display.width(), STATUS_HEIGHT, GxEPD_BLACK);

    // Draw Buzzer icon (example with status)
    epaper_buzzerIcon(10, STATUS_BASE_Y + 6, buzzerStatus);

    // Draw WiFi Signal Icon (WiFi signal strength)
    epaper_wifiIcon(50, STATUS_BASE_Y + 0, wifiSignalLevel);

    // Set the font to bold for the update text
    display.setFont(&FreeSansBold9pt7b); // Bold font for "Update:" and date/time

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
void epaper_buzzerIcon(int16_t x, int16_t y, bool status)
{
    int iconSize = 16; // Keep the icon size the same

    // Draw a larger speaker box with thicker lines
    display.drawRect(x, y, iconSize, iconSize, GxEPD_WHITE); // Speaker box with thicker lines

    if (status)
    {
        // Draw sound waves (icon for "on") with thicker lines
        display.drawLine(x + iconSize, y + 3, x + iconSize + 10, y + 6, GxEPD_WHITE);
        display.drawLine(x + iconSize, y + 6, x + iconSize + 10, y + 9, GxEPD_WHITE);
        display.drawLine(x + iconSize, y + 9, x + iconSize + 10, y + 12, GxEPD_WHITE);
    }
    else
    {
        // Draw a cross (icon for "off") with thicker lines
        display.drawLine(x, y, x + iconSize, y + iconSize, GxEPD_WHITE); // Cross from top left to bottom right
        display.drawLine(x + iconSize, y, x, y + iconSize, GxEPD_WHITE); // Cross from top right to bottom left
    }
}

// Function to draw a more intuitive WiFi Icon with thicker lines
void epaper_wifiIcon(int16_t x, int16_t y, int signalStrength)
{
    int numBars = 0;
    if (signalStrength > -70)
        numBars = 3; // Strong signal
    else if (signalStrength > -80)
        numBars = 2; // Moderate signal
    else if (signalStrength > -90)
        numBars = 1; // Weak signal

    int barSpacing = 6; // Wider spacing between bars
    int barWidth = 3;   // Thicker bars
    int barHeight = 6;  // Slightly taller bars

    // Draw WiFi bars with thicker lines
    for (int i = 0; i < 3; i++)
    {
        if (i < numBars)
        {
            display.fillRect(x + (i * barSpacing), y + (3 - i) * barHeight, barWidth, barHeight, GxEPD_WHITE); // Solid bars
        }
        else
        {
            display.drawRect(x + (i * barSpacing), y + (3 - i) * barHeight, barWidth, barHeight, GxEPD_WHITE); // Empty bars
        }
    }

    // Draw arcs manually using lines
    if (numBars > 0)
    {
        epaper_Arc(x, y, 16, 16, 0, 180, 3); // Approximate arc for strongest signal
    }
    if (numBars > 1)
    {
        epaper_Arc(x, y, 20, 20, 0, 180, 4); // Approximate arc for moderate signal
    }
    if (numBars > 2)
    {
        epaper_Arc(x, y, 24, 24, 0, 180, 5); // Approximate arc for strongest signal
    }
}

// Function to approximate an arc using lines
void epaper_Arc(int16_t x, int16_t y, int16_t width, int16_t height, int16_t startAngle, int16_t endAngle, int numSegments)
{
    // This function approximates an arc using line segments
    for (int i = startAngle; i < endAngle; i += (endAngle - startAngle) / numSegments)
    {
        int16_t x1 = x + cos(radians(i)) * width / 2;
        int16_t y1 = y - sin(radians(i)) * height / 2;
        int16_t x2 = x + cos(radians(i + (endAngle - startAngle) / numSegments)) * width / 2;
        int16_t y2 = y - sin(radians(i + (endAngle - startAngle) / numSegments)) * height / 2;
        display.drawLine(x1, y1, x2, y2, GxEPD_WHITE); // Draw the line segment
    }
}



void epaper_update(){
    epaper_barGraphs();
    epaper_statusArea();
    display.update();

    // Update the time every loop 
    second(); // Updates the real-time clock from TimeLib
    minute();
    hour();
    day();
    month();
    year();
}
void epaper_refresh(){
    display.fillScreen(GxEPD_WHITE); // Clear the screen
}


void epaper_setValue(int idx, uint8_t value){
    if(idx >= 0 && idx < NUM_BARS)
        barValues[idx] = value;
}