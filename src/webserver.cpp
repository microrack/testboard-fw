#include "webserver.h"
#include "esp_log.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "modules.h"

static const char* TAG = "webserver";

// WiFi credentials - will be loaded from filesystem
String wifi_ssid = "";
String wifi_password = "";

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
void handleGetResults();
void handleNotFound();
bool load_wifi_credentials();
bool connect_to_wifi();

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
    server.on("/results", HTTP_GET, handleGetResults);
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

    return true;
}

void enable_wifi() {
    ESP_LOGI(TAG, "Enabling WiFi AP mode");
    WiFi.softAP("TestBoard_AP", "12345678");
    IPAddress IP = WiFi.softAPIP();
    ESP_LOGI(TAG, "WiFi AP started");
    ESP_LOGI(TAG, "SSID: TestBoard_AP");
    ESP_LOGI(TAG, "Password: 12345678");
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
    
    // Disconnect from WiFi network if connected
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        ESP_LOGI(TAG, "WiFi station disconnected");
    }
    
    // Stop AP if running
    WiFi.softAPdisconnect(true);
    ESP_LOGI(TAG, "WiFi AP stopped");
    
    // Turn off WiFi completely
    WiFi.mode(WIFI_OFF);
    ESP_LOGI(TAG, "WiFi turned off");
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
        if (!init_modules_from_fs()) {
            ESP_LOGE(TAG, "Failed to initialize modules from filesystem");
        }
    } else {
        ESP_LOGE(TAG, "No configuration data received");
        server.send(400, "text/plain", "No configuration data received");
    }
}

void handleGetResults() {
    ESP_LOGI(TAG, "GET /results - Downloading test results");
    
    // Check if results file exists
    if (!LittleFS.exists("/results")) {
        ESP_LOGE(TAG, "Results file not found");
        server.send(404, "text/plain", "No test results available");
        return;
    }
    
    // Open results file
    File resultsFile = LittleFS.open("/results", "r");
    if (!resultsFile) {
        ESP_LOGE(TAG, "Failed to open results file");
        server.send(500, "text/plain", "Failed to read test results");
        return;
    }
    
    // Set headers for file download
    server.sendHeader("Content-Disposition", "attachment; filename=results.txt");
    server.sendHeader("Content-Type", "text/plain");
    
    // Send file content
    String resultsContent = resultsFile.readString();
    resultsFile.close();
    
    server.send(200, "text/plain", resultsContent);
    ESP_LOGI(TAG, "Test results sent successfully");
}

bool load_wifi_credentials() {
    ESP_LOGI(TAG, "Loading WiFi credentials from filesystem");
    
    File wifiFile = LittleFS.open("/wifi", "r");
    if (!wifiFile) {
        ESP_LOGE(TAG, "WiFi credentials file not found");
        return false;
    }
    
    String content = wifiFile.readString();
    wifiFile.close();
    
    // Parse SSID and password from file
    // Expected format: SSID=your_ssid\nPASSWORD=your_password
    int ssidPos = content.indexOf("SSID=");
    int passwordPos = content.indexOf("PASSWORD=");
    
    if (ssidPos == -1 || passwordPos == -1) {
        ESP_LOGE(TAG, "Invalid WiFi credentials format");
        return false;
    }
    
    // Extract SSID (from SSID= to end of line)
    int ssidStart = ssidPos + 5;
    int ssidEnd = content.indexOf('\n', ssidStart);
    if (ssidEnd == -1) ssidEnd = content.length();
    wifi_ssid = content.substring(ssidStart, ssidEnd);
    
    // Extract password (from PASSWORD= to end of line or end of file)
    int passwordStart = passwordPos + 9;
    int passwordEnd = content.indexOf('\n', passwordStart);
    if (passwordEnd == -1) passwordEnd = content.length();
    wifi_password = content.substring(passwordStart, passwordEnd);
    
    if (wifi_ssid.length() == 0) {
        ESP_LOGE(TAG, "Empty SSID in WiFi credentials");
        return false;
    }
    
    ESP_LOGI(TAG, "WiFi credentials loaded - SSID: %s", wifi_ssid.c_str());
    return true;
}

bool connect_to_wifi() {
    ESP_LOGI(TAG, "Attempting to connect to WiFi network: %s", wifi_ssid.c_str());
    
    // Set WiFi mode to station
    WiFi.mode(WIFI_STA);
    
    // Begin connection
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
    
    // Wait for connection with timeout
    int attempts = 0;
    const int max_attempts = 20; // 10 seconds timeout
    
    while (WiFi.status() != WL_CONNECTED && attempts < max_attempts) {
        delay(500);
        ESP_LOGI(TAG, "Connecting to WiFi... Attempt %d/%d", attempts + 1, max_attempts);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        IPAddress IP = WiFi.localIP();
        ESP_LOGI(TAG, "WiFi connected successfully!");
        ESP_LOGI(TAG, "SSID: %s", wifi_ssid.c_str());
        ESP_LOGI(TAG, "IP address: %s", IP.toString().c_str());
        ESP_LOGI(TAG, "MAC address: %s", WiFi.macAddress().c_str());
        
        // Start server
        server.begin();
        ESP_LOGI(TAG, "Web server started");
        
        // Signal that WiFi is enabled
        wifi_enabled = true;
        
        return true;
    } else {
        ESP_LOGE(TAG, "Failed to connect to WiFi network: %s", wifi_ssid.c_str());
        return false;
    }
}

void handleNotFound() {
    ESP_LOGW(TAG, "404 - Not found: %s", server.uri().c_str());
    server.send(404, "text/plain", "Not found");
} 