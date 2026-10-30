#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "mqtt_example";

// --- UPDATE THIS WITH YOUR PC'S LOCAL IP ADDRESS ---
#define MQTT_BROKER_URL "mqtt://192.168.1.128:1883"

static esp_mqtt_client_handle_t client = NULL;
TaskHandle_t xMqttTask;

void mqttTask(void *pvParameters) {
    int count = 0;
    char payload[32];

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(2000)); // Publish every 2 seconds
        snprintf(payload, sizeof(payload), "Hello from ESP32: %d", count++);
        int msg_id = esp_mqtt_client_publish(client, "home/sensor/telemetry", payload, 0, 1, 0);
        ESP_LOGI(TAG, "Sent publish successful, msg_id=%d", msg_id);
    }
}

/*
 * Event handler registered to receive MQTT events
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        // Subscribe to a topic once connected
        msg_id = esp_mqtt_client_subscribe(client, "home/device/command", 0);
        ESP_LOGI(TAG, "Sent subscribe successful, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA received");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(TAG, "Reported from esp-tls, tls error code = 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Reported from tls stack error = 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Captured errno = %d (%s)", event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg{};
    mqtt_cfg.broker.address.uri = MQTT_BROKER_URL;

    client = esp_mqtt_client_init(&mqtt_cfg);
    // Register the event handler
    esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_err_t err = esp_mqtt_client_start(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client");
        return;
    }

    const BaseType_t taskCreated = xTaskCreate(mqttTask, "mqttPublishTask ", 4096, nullptr, 3, &xMqttTask);
    ESP_ERROR_CHECK(taskCreated == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}
