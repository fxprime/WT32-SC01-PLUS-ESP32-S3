#include <Arduino.h> 
#include <vector>

#define LINE_COUNT 6 



#define LGFX_USE_V1
#include <LovyanGFX.hpp>


#include <lvgl.h>
#include "lv_conf.h"
/*** Setup screen resolution for LVGL ***/
static const uint16_t screenWidth = 480;
static const uint16_t screenHeight = 320;
// static lv_disp_draw_buf_t draw_buf;
// static lv_color_t buf[screenWidth * 10];

// Variables for touch x,y
#ifdef DRAW_ON_SCREEN
static int32_t x, y;
#endif

/*** Function declaration ***/
void lv_button_demo(void);





// SETUP LGFX PARAMETERS FOR WT32-SC01-PLUS
class LGFX : public lgfx::LGFX_Device
{

lgfx::Panel_ST7796      _panel_instance;
lgfx::Bus_Parallel8     _bus_instance;   
lgfx::Light_PWM         _light_instance;
lgfx::Touch_FT5x06      _touch_instance; // FT5206, FT5306, FT5406, FT6206, FT6236, FT6336, FT6436

public:
  LGFX(void)
  {
    { 
      auto cfg = _bus_instance.config();

      cfg.freq_write = 20000000;    
      cfg.pin_wr = 47; // pin number connecting WR
      cfg.pin_rd = -1; // pin number connecting RD
      cfg.pin_rs = 0;  // Pin number connecting RS(D/C)
      cfg.pin_d0 = 9;  // pin number connecting D0
      cfg.pin_d1 = 46; // pin number connecting D1
      cfg.pin_d2 = 3;  // pin number connecting D2
      cfg.pin_d3 = 8;  // pin number connecting D3
      cfg.pin_d4 = 18; // pin number connecting D4
      cfg.pin_d5 = 17; // pin number connecting D5
      cfg.pin_d6 = 16; // pin number connecting D6
      cfg.pin_d7 = 15; // pin number connecting D7
      //cfg.i2s_port = I2S_NUM_0; // (I2S_NUM_0 or I2S_NUM_1)

      _bus_instance.config(cfg);                    // Apply the settings to the bus.
      _panel_instance.setBus(&_bus_instance);       // Sets the bus to the panel.
    }

    { // Set display panel control.
      auto cfg = _panel_instance.config(); // Get the structure for display panel settings.

      cfg.pin_cs = -1;   // Pin number to which CS is connected (-1 = disable)
      cfg.pin_rst = 4;   // pin number where RST is connected (-1 = disable)
      cfg.pin_busy = -1; // pin number to which BUSY is connected (-1 = disable)

      // * The following setting values ​​are set to general default values ​​for each panel, and the pin number (-1 = disable) to which BUSY is connected, so please try commenting out any unknown items.

      cfg.memory_width = 320;  // Maximum width supported by driver IC
      cfg.memory_height = 480; // Maximum height supported by driver IC
      cfg.panel_width = 320;   // actual displayable width
      cfg.panel_height = 480;  // actual displayable height
      cfg.offset_x = 0;        // Panel offset in X direction
      cfg.offset_y = 0;        // Panel offset in Y direction
      cfg.offset_rotation = 0;  // was 2
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = true;     // was false
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true; // was false something to do with SD?

      _panel_instance.config(cfg);
    }

    { // Set backlight control. (delete if not necessary)
      auto cfg = _light_instance.config(); // Get the structure for backlight configuration.

      cfg.pin_bl = 45;     // pin number to which the backlight is connected
      cfg.invert = false;  // true to invert backlight brightness
      cfg.freq = 44100;    // backlight PWM frequency
      cfg.pwm_channel = 0; // PWM channel number to use (7??)

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance); // Sets the backlight to the panel.
    }

//*
    { // Configure settings for touch screen control. (delete if not necessary)
      auto cfg = _touch_instance.config();

      cfg.x_min = 0;   // Minimum X value (raw value) obtained from the touchscreen
      cfg.x_max = 319; // Maximum X value (raw value) obtained from the touchscreen
      cfg.y_min = 0;   // Minimum Y value obtained from touchscreen (raw value)
      cfg.y_max = 479; // Maximum Y value (raw value) obtained from the touchscreen
      cfg.pin_int = 7; // pin number to which INT is connected
      cfg.bus_shared = false; // set true if you are using the same bus as the screen
      cfg.offset_rotation = 0;

      // For I2C connection
      cfg.i2c_port = 0;    // Select I2C to use (0 or 1)
      cfg.i2c_addr = 0x38; // I2C device address number
      cfg.pin_sda = 6;     // pin number where SDA is connected
      cfg.pin_scl = 5;     // pin number to which SCL is connected
      cfg.freq = 400000;   // set I2C clock

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance); // Set the touchscreen to the panel.
    }
//*/
    setPanel(&_panel_instance); // Sets the panel to use.
  }
};

LGFX tft;

// create buffer for display
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY);
    if (!touched) { data->state = LV_INDEV_STATE_REL; }
    else {
      data->state = LV_INDEV_STATE_PR;
      data->point.x = touchX;
      data->point.y = touchY;

      #if DEBUG_TOUCH !=0
      Serial.print( "Data x " ); Serial.println( touchX );
      Serial.print( "Data y " ); Serial.println( touchY );
      #endif
    }
}





static std::vector<int> points[LINE_COUNT];
static int colors[] = { TFT_RED, TFT_GREEN, TFT_BLUE, TFT_CYAN, TFT_MAGENTA, TFT_YELLOW };
static int xoffset, yoffset, point_count;

int getBaseColor(int x, int y)
{
  return ((x^y)&3 || ((x-xoffset)&31 && y&31) ? TFT_BLACK : ((!y || x == xoffset) ? TFT_WHITE : TFT_DARKGREEN));
}

void setup(void)
{

  tft.begin();
  tft.setRotation(3);
  tft.setBrightness(255);

  lv_init();

  #if LV_USE_LOG != 0
    lv_log_register_print_cb( my_print ); /* register print function for debugging */
  #endif

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touch_read;
  lv_indev_drv_register(&indev_drv);

  
    
  if (tft.width() < tft.height()) tft.setRotation(tft.getRotation() ^ 1);

  yoffset = tft.height() >> 1;
  xoffset = tft.width()  >> 1;
  point_count = tft.width() + 1;

  for (int i = 0; i < LINE_COUNT; i++)
  {
    points[i].resize(point_count);
  }

  tft.startWrite();
  tft.setAddrWindow(0, 0, tft.width(), tft.height());
  for (int y = 0; y < tft.height(); y++)
  {
    for (int x = 0; x < tft.width(); x++)
    {
      tft.writeColor(getBaseColor(x, y - yoffset), 1);
    }
  }
  tft.endWrite();
}

static int32_t x, y;
void loop(void)
{
  /* FPS calculation */
  static int prev_sec;
  static int fps;
  ++fps;
  int sec = millis() / 1000;
  if (prev_sec != sec)
  {
    prev_sec = sec;
    tft.setCursor(0,0);
    tft.printf("fps:%03d", fps);
    fps = 0;
  }

  static int count;

  // Setting up random data for graph
  for (int i = 0; i < LINE_COUNT; i++)
  {
    points[i][count % point_count] = (sinf((float)count / (10 + 30 * i))+sinf((float)count / (13 + 37 * i))) * (tft.height() >> 2);
  }

  ++count;

  // startWrite/endWrite is used to draw buffer at once=smooth graphics
  tft.startWrite();
  int index1 = count % point_count;
  for (int x = 0; x < point_count-1; x++)
  {
    int index0 = index1;
    index1 = (index0 +1) % point_count;
    for (int i = 0; i < LINE_COUNT; i++)
    {
      int y = points[i][index0];
      if (y != points[i][index1])
      {
        tft.writePixel(x, y + yoffset, getBaseColor(x, y));
      }
    }

    for (int i = 0; i < LINE_COUNT; i++)
    {
      int y = points[i][index1];
      tft.writePixel(x, y + yoffset, colors[i]);
    }
  }
  tft.endWrite();
  
  /* Touch draw with rectangles 
   * Have only this if you just want to only draw on screen
  */
  if (tft.getTouch(&x, &y)) {
    tft.fillRect(x-2, y-2, 5, 5, count*7);
    tft.setCursor(380,0);
    tft.printf("Touch:(%03d,%03d)", x,y);
  }
}