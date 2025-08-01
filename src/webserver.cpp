#include "webserver.h"
#include "esp_log.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "webserver";

// WiFi credentials
const char* ssid = "microrack";
const char* password = "11223344";

// Web server
WebServer server(80);

// HTML page content
String htmlContent;

// Task handle for web server
TaskHandle_t webserverTaskHandle = NULL;

// WiFi state tracking
volatile bool wifi_enabled = false;

// Forward declarations
void handleRoot();
void handleGetConfig();
void handlePostConfig();
void handleNotFound();

// Web server task function
void webserver_task(void* parameter) {
    ESP_LOGI(TAG, "Web server task started");
    
    while (true) {
        // Check if WiFi is enabled
        if (wifi_enabled) {
            // WiFi is enabled, handle client requests
            server.handleClient();
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent watchdog issues
    }
}

bool init_webserver() {
    ESP_LOGI(TAG, "Initializing web server");
    
    // Load HTML content from filesystem
    File htmlFile = LittleFS.open("/index.html", "r");
    if (!htmlFile) {
        ESP_LOGE(TAG, "Failed to open index.html");
        return false;
    }
    htmlContent = htmlFile.readString();
    htmlFile.close();
    
    // Configure web server routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/config", HTTP_GET, handleGetConfig);
    server.on("/config", HTTP_POST, handlePostConfig);
    server.onNotFound(handleNotFound);
    
    // Create web server task
    BaseType_t result = xTaskCreatePinnedToCore(
        webserver_task,           // Task function
        "webserver_task",         // Task name
        8192,                     // Stack size (bytes)
        NULL,                     // Task parameters
        1,                        // Task priority
        &webserverTaskHandle,     // Task handle
        0                         // Core to run on (Core 0)
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create web server task");
        return false;
    }

    ESP_LOGI(TAG, "Web server task created successfully");
    
    // Start WiFi initially
    enable_wifi();

    return true;
}

void enable_wifi() {
    ESP_LOGI(TAG, "Enabling WiFi");
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    ESP_LOGI(TAG, "WiFi AP started");
    ESP_LOGI(TAG, "SSID: %s", ssid);
    ESP_LOGI(TAG, "Password: %s", password);
    ESP_LOGI(TAG, "IP address: %s", IP.toString().c_str());

    // Start server
    server.begin();
    ESP_LOGI(TAG, "Web server started");
    
    // Signal that WiFi is enabled
    wifi_enabled = true;
}

void disable_wifi() {
    ESP_LOGI(TAG, "Disabling WiFi");
    
    // Signal that WiFi is disabled
    wifi_enabled = false;
    
    WiFi.softAPdisconnect(true);
    ESP_LOGI(TAG, "WiFi AP stopped");
}

void handleRoot() {
    ESP_LOGI(TAG, "GET / - Serving main page");
    server.send(200, "text/html", htmlContent);
}

void handleGetConfig() {
    ESP_LOGI(TAG, "GET /config - Downloading configuration");
    
    File configFile = LittleFS.open("/config", "r");
    if (!configFile) {
        ESP_LOGE(TAG, "Failed to open config for reading");
        server.send(404, "text/plain", "Configuration file not found");
        return;
    }
    
    String configContent = configFile.readString();
    configFile.close();
    
    server.sendHeader("Content-Disposition", "attachment; filename=config");
    server.send(200, "text/plain", configContent);
    ESP_LOGI(TAG, "Configuration sent successfully");
}

void handlePostConfig() {
    ESP_LOGI(TAG, "POST /config - Uploading configuration");
    
    if (server.hasArg("plain")) {
        String newConfig = server.arg("plain");
        
        // Write new configuration to filesystem
        File configFile = LittleFS.open("/config", "w");
        if (!configFile) {
            ESP_LOGE(TAG, "Failed to open config for writing");
            server.send(500, "text/plain", "Failed to save configuration");
            return;
        }
        
        configFile.print(newConfig);
        configFile.close();
        
        ESP_LOGI(TAG, "Configuration updated successfully");
        server.send(200, "text/plain", "Configuration updated successfully");
    } else {
        ESP_LOGE(TAG, "No configuration data received");
        server.send(400, "text/plain", "No configuration data received");
    }
}

void handleNotFound() {
    ESP_LOGW(TAG, "404 - Not found: %s", server.uri().c_str());
    server.send(404, "text/plain", "Not found");
} 