/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <cmath>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "LovyanGFX.hpp"
#if 0 // Touchscreen temporairement désactivé
#include "XPT2046_Touchscreen.h"
#endif
#if 0 // Preferences temporairement désactivé
#include "Preferences.h"
#endif
#include "esp_log.h"
#define TAG "BADGE"

// --- LGFX config custom (garde ta config) ---
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
      cfg.rgb_order = true;
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
LGFX lcd;

// --- Touchscreen config ---

// Gestion du touchscreen désactivée temporairement

// Gestion Preferences désactivée temporairement
int likes = 0;
int screenW, screenH;
LGFX_Sprite spr(&lcd);

// Timing animation
float t = 0.0f;
// Variables pour l'effet de scintillement organique
float neon_flicker = 1.0f;
float neon_flicker_smooth = 1.0f; // Ajout pour le lissage
int neon_dim_frames = 0;
float neon_dim_target = 1.0f;

// Colors (neon-ish)
uint16_t colBackground;
uint16_t colNeonViolet;
uint16_t colCyan;
uint16_t colYellow;
uint16_t colHeart;

// Offset pour l'animation des scanlines
int scanline_offset = 0;
// Animation glow progressive
int anim_glow_level = 0;
int anim_glow_direction = 1;

// --- Utilitaires ---
void drawScanlines();
void drawNameWithGlow(const char *name1, const char *name2);
void drawLikeCounter();

void display_loop_task(void *pvParameter);
extern "C" void app_main(void)
{
  ESP_LOGI(TAG, "Initialisation de l'écran...");
  lcd.init();
  srand((unsigned int)time(NULL)); // Init aléatoire
  lcd.setBrightness(255);
  lcd.setRotation(5); // mode portrait custom
  screenW = lcd.width();
  screenH = lcd.height();

  // Colors
  colBackground = lcd.color565(8, 6, 20);
  colNeonViolet = lcd.color565(199, 120, 255);
  colCyan = lcd.color565(0, 224, 255);
  colYellow = lcd.color565(255, 195, 0);
  colHeart = lcd.color565(255, 40, 180);

  // Touch init désactivé

  // Sprite double buffering
  spr.setColorDepth(16);
  spr.createSprite(screenW, screenH);

  // Load likes désactivé

  // Initial draw
  spr.fillSprite(colBackground);
  drawScanlines();
  drawNameWithGlow("YANN", "SERGENT");
  drawLikeCounter();
  spr.pushSprite(0, 0);

  // Création de la tâche d'affichage à 60 fps
  xTaskCreate(display_loop_task, "display_loop", 4096, NULL, 2, NULL);
}

// Tâche d'affichage à 60 fps
void display_loop_task(void *pvParameter)
{
  const TickType_t frame_delay = pdMS_TO_TICKS(16); // 60 fps = 16 ms
  unsigned long lastFrame = 0;
  while (true)
  {
    unsigned long now = lgfx::v1::millis();
    if (now - lastFrame < 16)
    {
      vTaskDelay(1 / portTICK_PERIOD_MS);
      continue;
    }
    lastFrame = now;

    t = lgfx::v1::millis() * 0.001f;

    // --- Animation néon organique ---
    // Flicker rapide (variation frame à frame, plus intense)
    float flicker_fast = 0.75f + 0.35f * ((float)rand() / RAND_MAX) * ((float)rand() / RAND_MAX); // plus de flicker, distribution non linéaire

    // Baisse occasionnelle, moins fréquente et plus variable
    if (neon_dim_frames <= 0 && ((float)rand() / RAND_MAX) < 0.016f)
    {                                    // 1.6% de chance par frame
      neon_dim_frames = 6 + rand() % 10; // durée entre 6 et 15 frames
      // Baisse variable, parfois très légère, parfois plus marquée
      float base_dim = 0.65f + 0.25f * ((float)rand() / RAND_MAX) * ((float)rand() / RAND_MAX); // entre 0.65 et 0.9, distribution non linéaire
      neon_dim_target = base_dim;
    }
    if (neon_dim_frames > 0)
    {
      // La baisse elle-même varie à chaque frame
      float frame_dim = neon_dim_target + 0.08f * (((float)rand() / RAND_MAX) - 0.5f); // petite variation
      flicker_fast *= frame_dim;
      neon_dim_frames--;
    }
    // --- Lissage du flicker ---
    float smoothing = 0.72f; // Ajuste entre 0.7 et 0.95 pour plus ou moins de lissage
    neon_flicker_smooth = smoothing * neon_flicker_smooth + (1.0f - smoothing) * flicker_fast;
    neon_flicker = neon_flicker_smooth;

    // Animation scanlines
    scanline_offset = (scanline_offset + 1) % 10;

    // Variation de la luminosité globale avec neon_flicker
    int brightness = (int)(neon_flicker * 255.0f);
    brightness = brightness < 32 ? 32 : (brightness > 255 ? 255 : brightness); // Clamp pour éviter écran trop sombre
    lcd.setBrightness(brightness);

    spr.fillSprite(colBackground);
    drawScanlines();
    // Header "TAP TO LIKE"
    spr.setTextDatum(TC_DATUM);
    spr.setTextFont(1);
    spr.setTextSize(2);
    spr.setTextColor(colCyan);
    spr.drawString("TAP TO LIKE", screenW / 2, 20);
    // Heart
    spr.fillCircle(screenW / 2, 54, 16, colHeart);
    // Nom avec glow sur deux lignes (animation)
    drawNameWithGlow("YANN", "SERGENT");
    // Séparateur
    spr.fillRect(36, 180, screenW - 80, 3, colCyan);
    // Team
    spr.setTextDatum(TC_DATUM);
    spr.setTextFont(1);
    spr.setTextSize(2);
    spr.setTextColor(colCyan);
    spr.drawString("open|core", screenW / 2, 200);
    // Ville + rôle
    spr.setTextDatum(TC_DATUM);
    spr.setTextFont(1);
    spr.setTextSize(2);
    spr.setTextColor(colYellow);
    spr.drawString("LYON", screenW / 2, 230);
    spr.drawString("DEV FRONTEND", screenW / 2, 260);
    // Compteur likes
    drawLikeCounter();
    spr.pushSprite(0, 0);
    vTaskDelay(frame_delay);
  }
}

// --- Fonctions utilitaires ---
void drawScanlines()
{
  for (int y = 0; y < screenH; y += 10)
  {
    int animY = y + scanline_offset;
    if (animY < screenH)
    {
      uint16_t col = lcd.color565(12, 8, 30);
      spr.drawFastHLine(0, animY, screenW, col);
    }
  }
}

void drawNeonText(const char *txt1, const char *txt2, int x, int y1, int y2)
{

  // Couleur de base du néon (rose/violet)
  uint8_t baseR = 255;
  uint8_t baseG = 60;
  uint8_t baseB = 200;

  // Flicker organique seul (sans pulse)
  float flicker = neon_flicker;
  float organic = flicker;
  organic = fmaxf(organic, 0.32f); // ne descend jamais sous 0.32 (plus lumineux)

  spr.setTextDatum(MC_DATUM);
  spr.setTextFont(4);

  // Halo : nombre de couches dépend dynamiquement du flicker
  int layers = (int)(neon_flicker * 12.0f); // 0 à 12 couches selon l'intensité
  for (int i = layers; i > 0; i--)
  {
    float fade = powf(0.8f, i) * organic;
    float alpha = fade * 0.5f * flicker;
    uint8_t r = (uint8_t)(baseR * alpha + 8 * (1.0f - alpha));
    uint8_t g = (uint8_t)(baseG * alpha + 6 * (1.0f - alpha));
    uint8_t b = (uint8_t)(baseB * alpha + 20 * (1.0f - alpha));
    uint16_t color = lcd.color565(r, g, b);
    spr.setTextColor(color);
    spr.setTextSize(1.5f + 0.02f * i);
    // Dessine les deux couches
    spr.drawString(txt1, x + i, y1);
    spr.drawString(txt1, x - i, y1);
    spr.drawString(txt1, x, y1 + i);
    spr.drawString(txt1, x, y1 - i);
    spr.drawString(txt2, x + i, y2);
    spr.drawString(txt2, x - i, y2);
    spr.drawString(txt2, x, y2 + i);
    spr.drawString(txt2, x, y2 - i);
  }

  // Texte principal plus vif pour les deux couches
  spr.setTextSize(1.5f);
  uint16_t brightColor = lcd.color565(
      baseR * (0.3f + 0.6f * organic),
      baseG * (0.3f + 0.6f * organic),
      baseB * (0.3f + 0.6f * organic));
  spr.setTextColor(brightColor);
  spr.drawString(txt1, x, y1);
  spr.drawString(txt2, x, y2);
}

void drawNameWithGlow(const char *name1, const char *name2)
{
  int x = screenW / 2;
  int y1 = 110;
  int y2 = 145;
  // Dessine les deux noms avec effet glow en même temps
  drawNeonText(name1, name2, x, y1, y2);
}

void drawLikeCounter()
{
  spr.setTextDatum(TR_DATUM);
  spr.setTextFont(1);
  spr.setTextSize(2);
  spr.setTextColor(colHeart);
  char buf[16];
  snprintf(buf, sizeof(buf), "\xe2\x99\xa5 %d", likes); // coeur unicode
  spr.drawString(buf, screenW - 8, 8);
}