#include <lvgl.h>
#include <TFT_eSPI.h>
#include <ESP32Servo.h>

#define SDA_FT6236 8
#define SCL_FT6236 3

#define TFT_HOR_RES 320
#define TFT_VER_RES 240
#define SOIL_SENSOR 33
#define SERVO_PIN 34
#define LED_PIN 2 

void *draw_buf_1;
unsigned long lastTickMillis = 0;
static lv_display_t *disp;
lv_obj_t *arc;
lv_obj_t *label;
lv_obj_t *status_label;
lv_obj_t *cover_screen;
lv_obj_t *main_screen;
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
#define DATA_UPDATE_INTERVAL 500 

void create_chart();
void update_chart_data();
void create_cover_page();
void start_button_event_handler(lv_event_t *e);
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);


void TaskReadSoil(void *pvParameters);
void TaskControlServoAndLED(void *pvParameters);
void TaskUpdateUI(void *pvParameters);

int readSoil = 0;
int min_value = 30;

TFT_eSPI tft = TFT_eSPI();
Servo myServo;

void setup() {
  Serial.begin(115200);
  myServo.attach(SERVO_PIN);
  pinMode(LED_PIN, OUTPUT); 
  lv_init();

  draw_buf_1 = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf_1, DRAW_BUF_SIZE);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  String LVGL_Arduino = "Hello do!\n";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.println(LVGL_Arduino);

  pinMode(SOIL_SENSOR, INPUT);

  
  create_cover_page();


  xTaskCreatePinnedToCore(TaskReadSoil, "ReadSoil", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskControlServoAndLED, "ControlServoAndLED", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskUpdateUI, "UpdateUI", 4096, NULL, 1, NULL, 1);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    uint16_t x = 0, y = 0;
    bool pressed = tft.getTouch(&x, &y);

    if (pressed) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = x;
        data->point.y = y;
        Serial.print("Data x ");
        Serial.println(x);
        Serial.print("Data y ");
        Serial.println(y);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void create_cover_page() {
    cover_screen = lv_obj_create(NULL);

    
    lv_obj_t *label_h1 = lv_label_create(cover_screen);
    lv_label_set_text(label_h1, "Auto Watering System");
    lv_obj_align(label_h1, LV_ALIGN_CENTER, 0, -40);  
    lv_obj_t *label_h2 = lv_label_create(cover_screen);
    lv_label_set_text(label_h2, "Luthfi Hanif");
    lv_obj_align(label_h2, LV_ALIGN_CENTER, 0, 0); 
    lv_obj_t *start_button = lv_btn_create(cover_screen);
    lv_obj_align(start_button, LV_ALIGN_CENTER, 0, 60);  
    lv_obj_add_event_cb(start_button, start_button_event_handler, LV_EVENT_CLICKED, NULL);  
    lv_obj_set_size(start_button, 150, 50);

    
    lv_obj_t *start_label = lv_label_create(start_button);
    lv_label_set_text(start_label, "Start");
    lv_obj_center(start_label);

    lv_scr_load(cover_screen);
}

void start_button_event_handler(lv_event_t *e) {
    create_chart();  
    Serial.println("ok");
    lv_scr_load(main_screen);  
}

void create_chart() {
    main_screen = lv_obj_create(NULL);

    
    arc = lv_arc_create(main_screen);
    lv_obj_set_size(arc, 150, 150);  
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 20);  

    lv_arc_set_rotation(arc, 270);  
    lv_arc_set_bg_angles(arc, 0, 360);  
    lv_arc_set_angles(arc, 0, 0);  

 
    lv_obj_set_style_arc_width(arc, 20, LV_PART_MAIN);  
    lv_obj_set_style_arc_color(arc, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);  
    label = lv_label_create(main_screen);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 20);  
    lv_label_set_text(label, "0%");  

    
    status_label = lv_label_create(main_screen);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, -90);  
    lv_label_set_text(status_label, "Idle");  
}

void loop() {

}

void TaskReadSoil(void *pvParameters) {
  while (1) {
    readSoil = map(analogRead(SOIL_SENSOR), 0, 4096, 100, 0);

    vTaskDelay(pdMS_TO_TICKS(DATA_UPDATE_INTERVAL));
  }
}

void TaskControlServoAndLED(void *pvParameters) {
  while (1) {
    if (readSoil < min_value) {
      myServo.write(180);
      digitalWrite(LED_PIN, HIGH);
    } else {
      myServo.write(0);
      digitalWrite(LED_PIN, LOW);
    }

    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

void TaskUpdateUI(void *pvParameters) {
  while (1) {
    unsigned int tickPeriod = millis() - lastTickMillis;
    lv_tick_inc(tickPeriod);
    lastTickMillis = millis();

    lv_arc_set_angles(arc, 0, (readSoil * 360) / 100);  
    char buf[4];
    snprintf(buf, sizeof(buf), "%d%%", readSoil);
    lv_label_set_text(label, buf);

    
    if (readSoil < min_value) {
      lv_label_set_text(status_label, "Watering");
      lv_obj_set_style_arc_color(arc, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);  
    } else {
      lv_label_set_text(status_label, "Idle");
      lv_obj_set_style_arc_color(arc, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);  
    }

    vTaskDelay(pdMS_TO_TICKS(DATA_UPDATE_INTERVAL));
    lv_task_handler();
  }
}
