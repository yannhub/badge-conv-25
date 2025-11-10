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

#define TAG "BADGE"

LGFX lcd;

AppState appState;
DisplayManager displayManager(lcd, appState);

void display_loop_task(void *pvParameter)
{
  while (true)
  {
    displayManager.displayLoop();
  }
}

extern "C" void app_main(void)
{
  ESP_LOGI(TAG, "Initialisation de l'écran...");
  lcd.init();
  srand((unsigned int)time(NULL));
  lcd.setBrightness(255);
  lcd.setRotation(5);
  displayManager.init();
  // Lancement de la tâche d'affichage
  xTaskCreate(display_loop_task, "display_loop", 4096, NULL, 2, NULL);
}