// Imports
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

// Define WiFi credentials (replace with actual)
const char *ssid = "SSID";
const char *password = "PASSWORD";
// Static IP configuration
IPAddress ip(192, 168, 1, 222);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);
ESP8266WebServer server(80);

// Structure that holds the attributes of a node
struct Node {
    int RELAY_PIN;
    int BUTTON_PIN;
    String NAME;
    bool STATE;
    bool BUTTON_STATE;
};
// Length of nodes array
#define NODES_LENGTH 3
// Nodes array
Node nodes[NODES_LENGTH];

// Update interval
const int DELAY = 200;

// JSON serialize helper
String serializeRes(StaticJsonDocument<500> r) {
    String serRes;
    serializeJson(r, serRes);
    return serRes;
}

// Handle routes
void handleRoot() {
    String response = "welcome to Automator (Akash's bedroom)";
    server.send(200, "text/plain", response);
}
void handleNotFound() {
    String response = "unknown route";
    server.send(404, "text/plain", response);
}
void handleSet() {
    if (server.method() == HTTP_POST) {
        StaticJsonDocument<JSON_ARRAY_SIZE(NODES_LENGTH) + NODES_LENGTH * JSON_OBJECT_SIZE(2) + 80> res;
        for (int n = 0; n < NODES_LENGTH; n++)  {
            if (server.hasArg(nodes[n].NAME)) {
                // Create nested object
                JsonObject resObj = res.createNestedObject();
                resObj["name"] = nodes[n].NAME;

                if (server.arg(nodes[n].NAME) == "0") {
                    digitalWrite(nodes[n].RELAY_PIN, HIGH);
                    nodes[n].STATE = false;
                    resObj["state"] = 0;
                } else if (server.arg(nodes[n].NAME) == "1") {
                    digitalWrite(nodes[n].RELAY_PIN, LOW);
                    nodes[n].STATE = true;
                    resObj["state"] = 1;
                } else {
                    resObj["error"] = "unknown state identifier";
                }
            }
        }

        // Stringify and send JSON
        server.send(200, "application/json", serializeRes(res));
    } else {
        StaticJsonDocument<JSON_ARRAY_SIZE(1) + 80> res;
        res["error"] = "GET not allowed, use POST";
        server.send(405, "application/json", serializeRes(res));
    }
}
void handleGet() {
    if (server.method() == HTTP_GET) {
        StaticJsonDocument<JSON_ARRAY_SIZE(NODES_LENGTH) + NODES_LENGTH * JSON_OBJECT_SIZE(2) + 80> res;
        for (int n = 0; n < NODES_LENGTH; n++) {
            Node node = nodes[n];

            // Create nested object
            JsonObject resObj = res.createNestedObject();
            // Append data to nested object
            resObj["name"] = node.NAME;
            resObj["state"] = (node.STATE) ? 0 : 1;
        }

        // Stringify and send JSON
        server.send(200, "application/json", serializeRes(res));
    } else {
        StaticJsonDocument<JSON_ARRAY_SIZE(1) + 80> res;
        res["error"] = "POST not allowed, use GET";
        server.send(405, "application/json", serializeRes(res));
    }
}

// Setup functions
void setupNodes() {
    // Add values to `nodes`
    nodes[0] = (Node){D5, D1, "fan", false, false};
    nodes[1] = (Node){D6, D2, "light", false, false};
    nodes[2] = (Node){D7, D3, "outlet", false, false};

    // Setup pins
    for (int n = 0; n < NODES_LENGTH; n++) {
        // Set pin operation modes
        pinMode(nodes[n].RELAY_PIN, OUTPUT);
        pinMode(nodes[n].BUTTON_PIN, INPUT_PULLUP);

        // Set pin state to off/low
        digitalWrite(nodes[n].RELAY_PIN, LOW);
        digitalWrite(nodes[n].BUTTON_PIN, LOW);
    }
}
void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.config(ip, gateway, subnet, dns);
    WiFi.begin(ssid, password);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp8266")) {
        Serial.println("MDNS responder started");
    }
}
void setupRoutes() {
    server.on("/", handleRoot);

    server.on("/set", handleSet);
    server.on("/get", handleGet);

    server.onNotFound(handleNotFound);
}

void updateNodes() {
    for (int n = 0; n < NODES_LENGTH; n++) {
        if (digitalRead(nodes[n].BUTTON_PIN) == LOW) {
            Serial.print(nodes[n].NAME + ": " + nodes[n].STATE);
            if (nodes[n].BUTTON_STATE == false) {
                nodes[n].BUTTON_STATE = true;
                if (nodes[n].STATE) {
                    digitalWrite(nodes[n].RELAY_PIN, HIGH);
                    nodes[n].STATE = false;
                } else {
                    digitalWrite(nodes[n].RELAY_PIN, LOW);
                    nodes[n].STATE = true;
                }
            }
        } else {
            nodes[n].BUTTON_STATE = false;
        }
    }
}

void setup(void) {
    Serial.begin(115200);

    setupNodes();
    setupWiFi();
    setupRoutes();

    server.begin();

    Serial.println("HTTP server started");
}

void loop(void) {
    // Update local state
    updateNodes();
    // Handle remote requests
    server.handleClient();

    delay(DELAY);
}
