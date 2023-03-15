/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "dshot_esc_encoder.h"

#if CONFIG_IDF_TARGET_ESP32H2
#define DSHOT_ESC_RESOLUTION_HZ 32000000 // 32MHz resolution, DSHot protocol needs a relative high resolution
#else
#define DSHOT_ESC_RESOLUTION_HZ 40000000 // 40MHz resolution, DSHot protocol needs a relative high resolution
#endif
#define MOTOR1_GPIO     4
#define MOTOR2_GPIO    27
#define MOTOR3_GPIO    14
#define MOTOR4_GPIO    12

static const char *TAG = "example";

void app_main(void)
{
    ESP_LOGI(TAG, "Create RMT TX channel");

    //MOTOR 1 RMT Channel Configuration
    rmt_channel_handle_t esc_chan = NULL;
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select a clock that can provide needed resolution
        .gpio_num = MOTOR1_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = DSHOT_ESC_RESOLUTION_HZ,
        .trans_queue_depth = 10, // set the number of transactions that can be pending in the background
    };

    //MOTOR 2 RMT Channel Configuration
    rmt_channel_handle_t esc_chan1 = NULL;
    rmt_tx_channel_config_t tx_chan_config1 = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select a clock that can provide needed resolution
        .gpio_num = MOTOR2_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = DSHOT_ESC_RESOLUTION_HZ,
        .trans_queue_depth = 10, // set the number of transactions that can be pending in the background
    };

    //MOTOR 3 RMT Channel Configuration
    rmt_channel_handle_t esc_chan2 = NULL;
    rmt_tx_channel_config_t tx_chan_config2 = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select a clock that can provide needed resolution
        .gpio_num = MOTOR3_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = DSHOT_ESC_RESOLUTION_HZ,
        .trans_queue_depth = 10, // set the number of transactions that can be pending in the background
    };

    //MOTOR 4 RMT Channel Configuration
    rmt_channel_handle_t esc_chan3 = NULL;
    rmt_tx_channel_config_t tx_chan_config3 = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select a clock that can provide needed resolution
        .gpio_num = MOTOR4_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = DSHOT_ESC_RESOLUTION_HZ,
        .trans_queue_depth = 10, // set the number of transactions that can be pending in the background
    };
    
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &esc_chan));
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config1, &esc_chan1));
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config2, &esc_chan2));
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config3, &esc_chan3));

    ESP_LOGI(TAG, "Install Dshot ESC encoder");
    rmt_encoder_handle_t dshot_encoder = NULL;
    dshot_esc_encoder_config_t encoder_config = {
        .resolution = DSHOT_ESC_RESOLUTION_HZ,
        .baud_rate = 300000, // DSHOT300 protocol
        .post_delay_us = 50, // extra delay between each frame
    };

    ESP_ERROR_CHECK(rmt_new_dshot_esc_encoder(&encoder_config, &dshot_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(esc_chan)); //motor 1
    ESP_ERROR_CHECK(rmt_enable(esc_chan1)); //motor 2
    ESP_ERROR_CHECK(rmt_enable(esc_chan2)); //motor 1
    ESP_ERROR_CHECK(rmt_enable(esc_chan3)); //motor 2
    
    rmt_transmit_config_t tx_config = {
        .loop_count = -1, // infinite loop
    };
    dshot_esc_throttle_t throttle = {
        .throttle = 0,
        .telemetry_req = false, // telemetry is not supported in this example
    };

    ESP_LOGI(TAG, "Start ESC by sending zero throttle for a while...");
    ESP_ERROR_CHECK(rmt_transmit(esc_chan, dshot_encoder, &throttle, sizeof(throttle), &tx_config));
    ESP_ERROR_CHECK(rmt_transmit(esc_chan1, dshot_encoder, &throttle, sizeof(throttle), &tx_config));
    ESP_ERROR_CHECK(rmt_transmit(esc_chan2, dshot_encoder, &throttle, sizeof(throttle), &tx_config));
    ESP_ERROR_CHECK(rmt_transmit(esc_chan3, dshot_encoder, &throttle, sizeof(throttle), &tx_config));
    vTaskDelay(pdMS_TO_TICKS(5000));

    //Reverse Direction for Motor 2
    throttle.throttle = 21;
    ESP_ERROR_CHECK(rmt_transmit(esc_chan1, dshot_encoder, &throttle, sizeof(throttle), &tx_config));
    ESP_ERROR_CHECK(rmt_disable(esc_chan1));
    ESP_ERROR_CHECK(rmt_enable(esc_chan1));

    //Reverse Direction for Motor 4
    ESP_ERROR_CHECK(rmt_transmit(esc_chan3, dshot_encoder, &throttle, sizeof(throttle), &tx_config));
    ESP_ERROR_CHECK(rmt_disable(esc_chan3));
    ESP_ERROR_CHECK(rmt_enable(esc_chan3));
    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI(TAG, "Increase throttle, no telemetry");
    for (uint32_t thro = 100; thro < 1000; thro += 10) {
        throttle.throttle = thro;
        ESP_ERROR_CHECK(rmt_transmit(esc_chan, dshot_encoder, &throttle, sizeof(throttle), &tx_config));
        ESP_ERROR_CHECK(rmt_transmit(esc_chan1, dshot_encoder, &throttle, sizeof(throttle), &tx_config));
        ESP_ERROR_CHECK(rmt_transmit(esc_chan2, dshot_encoder, &throttle, sizeof(throttle), &tx_config));
        ESP_ERROR_CHECK(rmt_transmit(esc_chan3, dshot_encoder, &throttle, sizeof(throttle), &tx_config));
        // the previous loop transfer is till undergoing, we need to stop it and restart,
        // so that the new throttle can be updated on the output
        ESP_ERROR_CHECK(rmt_disable(esc_chan));
        ESP_ERROR_CHECK(rmt_disable(esc_chan1));
        ESP_ERROR_CHECK(rmt_disable(esc_chan2));
        ESP_ERROR_CHECK(rmt_disable(esc_chan3));

        ESP_ERROR_CHECK(rmt_enable(esc_chan));
        ESP_ERROR_CHECK(rmt_enable(esc_chan1));
        ESP_ERROR_CHECK(rmt_enable(esc_chan2));
        ESP_ERROR_CHECK(rmt_enable(esc_chan3));
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
