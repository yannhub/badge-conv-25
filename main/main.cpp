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
#include "esp_log.h"
#include "lgfx_custom.h"
#include "display_manager.h"

#include "state.h"
#include <memory>

#include "views/view_qrcode.h"
#include "views/view_badge.h"
#include "views/view_game.h"
#include "views/view_program.h"
#include "views/view_settings.h"
#include "views/view_plasma.h"
#include "user_info.h"

// #include "battery_monitor.hpp"
// #include "views/view_battery.h"

#define TAG "BADGE"

LGFX lcd;

AppState appState;
DisplayManager displayManager(lcd, appState);

// static BatteryMonitor batteryMonitor(
//     ADC_UNIT_1,
//     ADC_CHANNEL_6, // GPIO34
//     2.0f,          // gain diviseur (R1=R2 => x2)
//     3,             // 3 piles AAA
//     1.00f,         // V min NiMH
//     1.40f          // V max NiMH
// );

void display_loop_task(void *pvParameter)
{
  while (true)
  {
    displayManager.displayLoop();
  }
}

extern "C" void app_main(void)
{
  Config::initNVS();
  Config::loadFromNVS();
  ESP_LOGI(TAG, "Initialisation de l'écran...");
  lcd.init();
  srand((unsigned int)time(NULL));
  displayManager.init();

  // Ajout des vues
  displayManager.addView(std::make_unique<ViewBadge>(appState, lcd));
  displayManager.addView(std::make_unique<ViewPlasma>(appState, lcd));
  displayManager.addView(std::make_unique<ViewQRCode>());
  displayManager.addView(std::make_unique<ViewProgram>(appState, lcd));
  displayManager.addView(std::make_unique<ViewGame>(appState, lcd));
  // displayManager.addView(std::make_unique<ViewBattery>(&batteryMonitor));
  displayManager.setSettingsView(std::make_unique<ViewSettings>(lcd, displayManager));

  // Génération du QR code de badge d'accès (après allocation des vues)
  user_info_generate_qrcode(user_info.accessBadgeToken.c_str());

  // Lancement de la tâche d'affichage
  xTaskCreate(display_loop_task, "display_loop", 4096, NULL, 2, NULL);
}