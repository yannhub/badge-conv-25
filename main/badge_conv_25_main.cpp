#include "lvgl.h"
#include "LovyanGFX.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// --- LGFX config custom (copié de hello_world_main.cpp) ---
class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9341 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;

public:
    LGFX(void)
    {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = HSPI_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.freq_read = 16000000;
            cfg.spi_3wire = false;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = 14;
            cfg.pin_mosi = 13;
            cfg.pin_miso = 12;
            cfg.pin_dc = 2;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = 15;
            cfg.pin_rst = -1;
            cfg.pin_busy = -1;
            cfg.memory_width = 320;
            cfg.memory_height = 240;
            cfg.panel_width = 320;
            cfg.panel_height = 240;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = false;
            cfg.invert = false;
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;
            _panel_instance.config(cfg);
        }
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl = 21;
            cfg.invert = false;
            cfg.freq = 44100;
            cfg.pwm_channel = 7;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }
        setPanel(&_panel_instance);
    }
};
static LGFX lcd;

// Fonction de flush adaptée LVGL 9.x
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;
    lcd.startWrite();
    lcd.setAddrWindow(area->x1, area->y1, w, h);
    lcd.pushImage(area->x1, area->y1, w, h, (uint16_t *)px_map);
    lcd.endWrite();
    lv_display_flush_ready(disp);
}

extern "C" void app_main()
{
    lv_init();
    lcd.init();
    lcd.setRotation(5);

    // Allocation du buffer LVGL
    int buf_pixels = lcd.width() * 40; // 40 lignes, adapte selon ta RAM
    void *buf1 = heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA);
    ESP_LOGI("LVGL", "Buffer address: %p", buf1);
#include "esp_heap_caps.h"
    bool is_dma_capable = esp_ptr_dma_capable(buf1);
    ESP_LOGI("LVGL", "DMA capable: %d", is_dma_capable);
    lv_display_t *disp = lv_display_create(lcd.width(), lcd.height());
    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    lv_display_set_buffers(disp, buf1, NULL, buf_pixels, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Définir le fond de l'écran en noir
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), 0);

    // Exemple LVGL : label centré avec texte simple et police par défaut
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "12345");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0); // Police LVGL par défaut

    while (1)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}