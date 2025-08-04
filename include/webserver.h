#pragma once

// Initialize web server and WiFi access point
bool init_webserver();

// WiFi control functions
void enable_wifi();
void disable_wifi();
bool load_wifi_credentials();
bool connect_to_wifi(); 