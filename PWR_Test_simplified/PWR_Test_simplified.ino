#include "user_config.h"
#include "driver/gpio.h"
#include <Wire.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "fonts.h"

// ----------- Settings -----------
#define SPI_CLOCK_HZ 4000000

// ----------- EPD (1.54" D67) -----------
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(
  GxEPD2_154_D67(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN));

U8G2_FOR_ADAFRUIT_GFX u8g2;

// === U8g2 Font Selections ===
#define FONT_DAY u8g2_font_luBS24_tf
#define FONT_DATE u8g2_font_helvB18_tf
#define FONT_TIME u8g2_font_luBS19_tf

// ---------------- Helpers ----------------
// ---------------------------------------------------------
// Helper: Draw centered text - call within display.firstPage(); do{<call here>} while(dislay.nextPage());
// ---------------------------------------------------------
void drawCenteredU8g2(const char* text, int y, const uint8_t* font,
                      uint16_t fgColor, uint16_t bgColor) {
  u8g2.setFont(font);
  u8g2.setForegroundColor(fgColor);
  u8g2.setBackgroundColor(bgColor);
  int textWidth = u8g2.getUTF8Width(text);
  int x = (EPD_WIDTH - textWidth) / 2;
  u8g2.drawUTF8(x, y, text);
}

static void epdLinesHiZ() {
  // Put all EPD/SPI lines in high-Z so nothing back-feeds after we cut rails
  pinMode(EPD_CS_PIN, INPUT);
  pinMode(EPD_DC_PIN, INPUT);
  pinMode(EPD_RST_PIN, INPUT);
  pinMode(EPD_MOSI_PIN, INPUT);
  pinMode(EPD_SCK_PIN, INPUT);
  pinMode(EPD_BUSY_PIN, INPUT);
}

static void epdInit() {
  // SPI on custom pins
  SPI.begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, EPD_CS_PIN);
  display.epd2.selectSPI(SPI, SPISettings(SPI_CLOCK_HZ, MSBFIRST, SPI_MODE0));

  // Init panel
  display.init(115200);
  display.setRotation(0);
  display.setFullWindow();

  // U8g2 wrapper
  u8g2.begin(display);
}

static void epdShowTextFull(const char* txt, int x, int y) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    //display.setFont(&DejaVu_Sans_Condensed_Bold_15);
    display.setFont(&DSEG7_Classic_Bold_36);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(x, y);
    display.print(txt);
  } while (display.nextPage());
}

static void epdShowTextFullCenteredY(
  const char* txt,
  int16_t y_baseline,
  const GFXfont* font) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    display.setFont(font);
    display.setTextColor(GxEPD_BLACK);

    int16_t x1, y1;
    uint16_t w, h;

    // Measure text bounds
    display.getTextBounds(txt, 0, y_baseline, &x1, &y1, &w, &h);

    // Center horizontally using width w
    int16_t cx = (EPD_WIDTH - w) / 2;

    display.setCursor(cx, y_baseline);
    display.print(txt);

  } while (display.nextPage());
}

// ------------- Power control -------------
class PowerControl {
public:
  void init() {
    // Release any holds from deep sleep (defensive)
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis((gpio_num_t)VBAT_PWR_PIN);
    gpio_hold_dis((gpio_num_t)EPD_PWR_PIN);

    pinMode(VBAT_PWR_PIN, OUTPUT);
    pinMode(EPD_PWR_PIN, OUTPUT);

    digitalWrite(VBAT_PWR_PIN, HIGH);  // main rail ON
    digitalWrite(EPD_PWR_PIN, HIGH);   // keep EPD rail OFF until we're ready
    delay(10);
  }

  void epdOn() {
    digitalWrite(EPD_PWR_PIN, LOW);  // EPD rail ON (Waveshare boards: LOW=ON)
    delay(10);
  }

  void powerOff() {
    // 1) Ensure the power button is released so the latch can re-arm
    while (digitalRead(PWR_BUTTON_PIN) == LOW) {
      delay(10);
    }
    delay(50);

    // 2) Turn the panel rail OFF first to prevent back-feeding
    digitalWrite(EPD_PWR_PIN, HIGH);  // EPD rail OFF
    delay(10);

    // 3) High-Z all related pins so no leakage paths exist
    epdLinesHiZ();
    delay(10);

    // 4) Finally cut the main rail
    digitalWrite(VBAT_PWR_PIN, LOW);  // main rail OFF
  }
};

PowerControl power;

void setup() {
  // Power first (main ON, EPD kept OFF)
  power.init();

  Serial.begin(115200);
  delay(100);

  // Configure power button (use INPUT_PULLUP if you don't have an external pull-up)
  pinMode(PWR_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("Device ON - Press button to power off");

  // Now enable the EPD rail and initialize the display
  power.epdOn();
  epdInit();

  // Show "ON" - font define in function
  // epdShowTextFull("ON", 50, 100);

  // Show "ON" but centered using font from fonts.h
  // epdShowTextFullCenteredY("ON", 100, &DSEG7_Classic_Bold_36);

  // show "ON", centered and using U8g2 font
  display.firstPage();
  do {
    drawCenteredU8g2("ON", 100, FONT_DAY, GxEPD_BLACK, GxEPD_WHITE);
  } while (display.nextPage());

  // Turn on LED
  digitalWrite(LED_PIN, 0);
}

void loop() {
  // Press to power off
  if (digitalRead(PWR_BUTTON_PIN) == LOW) {
    delay(50);                                 // debounce
    if (digitalRead(PWR_BUTTON_PIN) == LOW) {  //Power button low = shut down
      Serial.println("Powering OFF...");

      // Full refresh with "OFF" + hibernate; reduces ghosting and quiesces controller
      // epdShowTextFull("OFF", 50, 100);

      // Show "OFF" but centered
      // epdShowTextFullCenteredY("OFF", 100, &DSEG7_Classic_Bold_36);

      // Show "OFF", centered and using U8g2 font
      display.firstPage();
      do {
        drawCenteredU8g2("OFF", 100, FONT_DAY, GxEPD_BLACK, GxEPD_WHITE);
      } while (display.nextPage());

      // Hibernate display
      display.hibernate();
      delay(300);  // allow waveform to settle

      // Safe cut: EPD rail OFF, pins high-Z, then main rail OFF
      power.powerOff();

      // Should never reach here—device is unpowered
      while (true) { delay(1000); }
    }
  }

  delay(100);
}
