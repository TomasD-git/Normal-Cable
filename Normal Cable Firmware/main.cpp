#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <USB.h>
#include <USBHIDKeyboard.h>

#define CONTROL_PIN 4
#define SCRIPT_TIMEOUT_MS 600000

WebServer server(80);
USBHIDKeyboard keyboard;
TaskHandle_t scriptTaskHandle = NULL;
bool scriptRunning = false;
bool bootAutoStarted = false;
String currentScriptName = "";
String execLog[50];
int logIndex = 0;

struct Config {
    char ssid[32];
    char password[64];
    char autoScript[120];
    bool autoAttack;
    int delay;
    bool safeMode;
    char speedProfile[16];
};

Config config;

int getDelayMs() {
    if (strcmp(config.speedProfile, "slow") == 0) return 100;
    if (strcmp(config.speedProfile, "fast") == 0) return 25;
    if (strcmp(config.speedProfile, "fastest") == 0) return 5;
    return config.delay;
}

void loadSettings() {
    File f = SPIFFS.open("/settings.json", "r");
    if (!f) {
        Serial.println("[CFG] settings.json not found, using defaults");
        strlcpy(config.ssid, "ESP32-Ducky", sizeof(config.ssid));
        strlcpy(config.password, "ducky12345", sizeof(config.password));
        config.autoScript[0] = '\0';
        config.autoAttack = false;
        config.delay = 50;
        config.safeMode = false;
        strlcpy(config.speedProfile, "normal", sizeof(config.speedProfile));
        return;
    }
    DynamicJsonDocument doc(512);
    if (deserializeJson(doc, f) == DeserializationError::Ok) {
        strlcpy(config.ssid, doc["ssid"] | "ESP32-Ducky", sizeof(config.ssid));
        strlcpy(config.password, doc["password"] | "ducky12345", sizeof(config.password));
        strlcpy(config.autoScript, doc["autoScript"] | "", sizeof(config.autoScript));
        config.autoAttack = doc["autoAttack"] | false;
        config.delay = doc["delay"] | 50;
        config.safeMode = doc["safeMode"] | false;
        strlcpy(config.speedProfile, doc["speedProfile"] | "normal", sizeof(config.speedProfile));
        Serial.println("[CFG] settings.json loaded");
    } else {
        Serial.println("[CFG] Failed to parse settings.json, using defaults");
    }
    f.close();
}

void saveSettings() {
    File f = SPIFFS.open("/settings.json", "w");
    if (!f) {
        Serial.println("[CFG] Failed to open settings.json for write");
        return;
    }
    DynamicJsonDocument doc(512);
    doc["ssid"] = config.ssid;
    doc["password"] = config.password;
    doc["autoScript"] = config.autoScript;
    doc["autoAttack"] = config.autoAttack;
    doc["delay"] = config.delay;
    doc["safeMode"] = config.safeMode;
    doc["speedProfile"] = config.speedProfile;
    serializeJson(doc, f);
    f.close();
    Serial.println("[CFG] settings.json saved");
}

void setControl(bool active) {
    digitalWrite(CONTROL_PIN, active ? HIGH : LOW);
    Serial.printf("[CTRL] Pin %s\n", active ? "HIGH" : "LOW");
}

bool isValidFilename(const String& name) {
    if (name.length() == 0 || name.length() > 60) return false;
    if (name.indexOf('/') >= 0 || name.indexOf('\\') >= 0) return false;
    if (name.indexOf("../") >= 0 || name.indexOf("..\\") >= 0) return false;
    for (int i = 0; i < name.length(); i++) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) {
            return false;
        }
    }
    return true;
}

bool isValidScriptPath(const String& path) {
    if (path.length() == 0 || path.length() > 180) return false;
    if (path.startsWith("/") || path.startsWith("\\")) return false;
    if (path.indexOf("..") >= 0) return false;
    for (int i = 0; i < path.length(); i++) {
        char c = path[i];
        if (c == '/') continue;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) {
            return false;
        }
    }
    return true;
}

void logExecution(const String& scriptName) {
    execLog[logIndex] = String(millis() / 1000) + "s: " + scriptName;
    logIndex = (logIndex + 1) % 50;
}

void typeString(const String& text) {
    int delayMs = getDelayMs();
    for (char c : text) {
        keyboard.print(c);
        delay(delayMs);
    }
}

void pressKey(const String& k) {
    if      (k == "ENTER")       keyboard.press(KEY_RETURN);
    else if (k == "SPACE")       keyboard.press(KEY_SPACE);
    else if (k == "TAB")         keyboard.press(KEY_TAB);
    else if (k == "BACKSPACE")   keyboard.press(KEY_BACKSPACE);
    else if (k == "DELETE")      keyboard.press(KEY_DELETE);
    else if (k == "UP")          keyboard.press(KEY_UP_ARROW);
    else if (k == "DOWN")        keyboard.press(KEY_DOWN_ARROW);
    else if (k == "LEFT")        keyboard.press(KEY_LEFT_ARROW);
    else if (k == "RIGHT")       keyboard.press(KEY_RIGHT_ARROW);
    else if (k == "HOME")        keyboard.press(KEY_HOME);
    else if (k == "END")         keyboard.press(KEY_END);
    else if (k == "PAGEUP")      keyboard.press(KEY_PAGE_UP);
    else if (k == "PAGEDOWN")    keyboard.press(KEY_PAGE_DOWN);
    else if (k == "ESCAPE")      keyboard.press(KEY_ESC);
    else if (k == "INSERT")      keyboard.press(KEY_INSERT);
    else if (k == "CAPS")        keyboard.press(KEY_CAPS_LOCK);
    else if (k == "NUMLOCK")     keyboard.press(KEY_NUM_LOCK);
    else if (k == "SCROLLLOCK")  keyboard.press(KEY_SCROLL_LOCK);
    else if (k == "PRINTSCREEN") keyboard.press(KEY_PRINT_SCREEN);
    else if (k == "PAUSE")       keyboard.press(KEY_PAUSE);
    else if (k == "F1")          keyboard.press(KEY_F1);
    else if (k == "F2")          keyboard.press(KEY_F2);
    else if (k == "F3")          keyboard.press(KEY_F3);
    else if (k == "F4")          keyboard.press(KEY_F4);
    else if (k == "F5")          keyboard.press(KEY_F5);
    else if (k == "F6")          keyboard.press(KEY_F6);
    else if (k == "F7")          keyboard.press(KEY_F7);
    else if (k == "F8")          keyboard.press(KEY_F8);
    else if (k == "F9")          keyboard.press(KEY_F9);
    else if (k == "F10")         keyboard.press(KEY_F10);
    else if (k == "F11")         keyboard.press(KEY_F11);
    else if (k == "F12")         keyboard.press(KEY_F12);
    int delayMs = getDelayMs();
    delay(delayMs);
    keyboard.releaseAll();
}

void pressMod(const String& mod, bool down) {
    if      (mod == "CTRL" || mod == "CONTROL") {
        if (down) keyboard.press(KEY_LEFT_CTRL);
        else keyboard.release(KEY_LEFT_CTRL);
    } else if (mod == "SHIFT") {
        if (down) keyboard.press(KEY_LEFT_SHIFT);
        else keyboard.release(KEY_LEFT_SHIFT);
    } else if (mod == "ALT") {
        if (down) keyboard.press(KEY_LEFT_ALT);
        else keyboard.release(KEY_LEFT_ALT);
    } else if (mod == "WINDOWS" || mod == "GUI") {
        if (down) keyboard.press(KEY_LEFT_GUI);
        else keyboard.release(KEY_LEFT_GUI);
    }
}

void runScript(const String& script) {
    Serial.println("[SCRIPT] Starting");
    logExecution(currentScriptName);
    setControl(true);
    unsigned long startTime = millis();
    int pos = 0;
    int repeatCount = 1;
    while (pos < (int)script.length() && scriptRunning) {
        if (millis() - startTime > SCRIPT_TIMEOUT_MS) {
            Serial.println("[SCRIPT] Timeout reached");
            break;
        }
        int nl = script.indexOf('\n', pos);
        if (nl < 0) nl = script.length();
        String line = script.substring(pos, nl);
        pos = nl + 1;
        line.trim();
        if (line.length() == 0 || line.startsWith("REM ") || line.startsWith("//")) continue;
        int sp = line.indexOf(' ');
        String cmd = (sp > 0) ? line.substring(0, sp) : line;
        String arg = (sp > 0) ? line.substring(sp + 1) : "";
        cmd.toUpperCase();
        if (cmd == "DELAY" || cmd == "WAIT") {
            int ms = arg.toInt();
            if (ms == 0) ms = 5000;
            delay(ms);
        } else if (cmd == "REPEAT") {
            repeatCount = arg.toInt();
            if (repeatCount < 1) repeatCount = 1;
            if (repeatCount > 100) repeatCount = 100;
        } else if (cmd == "STRING" || cmd == "TYPE") {
            for (int i = 0; i < repeatCount; i++) {
                typeString(arg);
            }
            repeatCount = 1;
        } else if (cmd == "KEY") {
            for (int i = 0; i < repeatCount; i++) {
                pressKey(arg);
            }
            repeatCount = 1;
        } else if (cmd == "PRESS") {
            int plus = arg.indexOf('+');
            if (plus > 0) {
                String mod = arg.substring(0, plus);
                String key = arg.substring(plus + 1);
                mod.toUpperCase();
                key.toUpperCase();
                for (int i = 0; i < repeatCount; i++) {
                    pressMod(mod, true);
                    pressKey(key);
                    pressMod(mod, false);
                }
            } else {
                for (int i = 0; i < repeatCount; i++) {
                    pressKey(arg);
                }
            }
            repeatCount = 1;
        }
    }
    keyboard.releaseAll();
    delay(50);
    setControl(false);
    scriptRunning = false;
    scriptTaskHandle = NULL;
    currentScriptName = "";
    Serial.println("[SCRIPT] Done");
}

void sendJson(int code, const String& json) {
    server.send(code, "application/json", json);
}

String okJson(const String& msg) {
    return String("{\"status\":\"ok\",\"message\":\"") + msg + "\"}";
}

String errJson(const String& msg) {
    return String("{\"status\":\"error\",\"message\":\"") + msg + "\"}";
}

void handleRoot() {
    File f = SPIFFS.open("/index.html", "r");
    if (!f) {
        server.send(500, "text/plain", "index.html not found on SPIFFS");
        return;
    }
    server.streamFile(f, "text/html");
    f.close();
}

void handleSettings() {
    if (server.method() == HTTP_GET) {
        DynamicJsonDocument doc(512);
        doc["ssid"] = config.ssid;
        doc["password"] = config.password;
        doc["autoScript"] = config.autoScript;
        doc["autoAttack"] = config.autoAttack;
        doc["delay"] = config.delay;
        doc["safeMode"] = config.safeMode;
        doc["speedProfile"] = config.speedProfile;
        String out;
        serializeJson(doc, out);
        sendJson(200, out);
        return;
    }
    if (server.hasArg("ssid")) server.arg("ssid").toCharArray(config.ssid, sizeof(config.ssid));
    if (server.hasArg("password")) server.arg("password").toCharArray(config.password, sizeof(config.password));
    if (server.hasArg("autoScript")) server.arg("autoScript").toCharArray(config.autoScript, sizeof(config.autoScript));
    config.autoAttack = server.hasArg("autoAttack") && server.arg("autoAttack") == "true";
    if (server.hasArg("delay")) config.delay = server.arg("delay").toInt();
    config.safeMode = server.hasArg("safeMode") && server.arg("safeMode") == "true";
    if (server.hasArg("speedProfile")) server.arg("speedProfile").toCharArray(config.speedProfile, sizeof(config.speedProfile));
    saveSettings();
    sendJson(200, okJson("Settings saved"));
}

void handleListScripts() {
    DynamicJsonDocument doc(2048);
    JsonArray arr = doc.createNestedArray("scripts");
    File dir = SPIFFS.open("/scripts");
    if (dir && dir.isDirectory()) {
        File f = dir.openNextFile();
        while (f) {
            if (!f.isDirectory()) {
                String name = String(f.name());
                if (name.startsWith("/scripts/")) name = name.substring(String("/scripts/").length());
                else if (name.startsWith("/")) name = name.substring(1);
                arr.add(name);
            }
            f = dir.openNextFile();
        }
    }
    String out;
    serializeJson(doc, out);
    sendJson(200, out);
}

void handleGetScript() {
    if (!server.hasArg("name")) { sendJson(400, errJson("Missing name")); return; }
    String name = server.arg("name");
    if (name.startsWith("/")) name = name.substring(1);
    if (name.indexOf('/') >= 0) {
        if (!isValidScriptPath(name)) { sendJson(400, errJson("Invalid path")); return; }
    } else {
        if (!isValidFilename(name)) { sendJson(400, errJson("Invalid filename")); return; }
    }
    String path = "/scripts/" + name;
    File f = SPIFFS.open(path, "r");
    if (!f) { sendJson(404, errJson("Not found")); return; }
    DynamicJsonDocument doc(8192);
    doc["content"] = f.readString();
    f.close();
    String out;
    serializeJson(doc, out);
    sendJson(200, out);
}

void handleSaveScript() {
    if (!server.hasArg("name") || !server.hasArg("content")) {
        sendJson(400, errJson("Missing name or content")); return;
    }
    String name = server.arg("name");
    if (name.startsWith("/")) name = name.substring(1);
    if (name.indexOf('/') >= 0) {
        if (!isValidScriptPath(name)) { sendJson(400, errJson("Invalid path")); return; }
    } else {
        if (!isValidFilename(name)) { sendJson(400, errJson("Invalid filename")); return; }
    }
    String path = "/scripts/" + name;
    int lastSlash = path.lastIndexOf('/');
    if (lastSlash > 8) {
        String dirPath = path.substring(0, lastSlash);
        if (!SPIFFS.exists(dirPath)) {
            String accum = "";
            int p = 1;
            while (p <= dirPath.length()) {
                int next = dirPath.indexOf('/', p);
                String part;
                if (next == -1) part = dirPath.substring(p);
                else part = dirPath.substring(p, next);
                accum += "/" + part;
                if (!SPIFFS.exists(accum)) SPIFFS.mkdir(accum);
                if (next == -1) break;
                p = next + 1;
            }
        }
    }
    File f = SPIFFS.open(path, "w");
    if (!f) { sendJson(500, errJson("Failed to write")); return; }
    f.print(server.arg("content"));
    f.close();
    sendJson(200, okJson("Saved"));
}

void handleDeleteScript() {
    if (!server.hasArg("name")) { sendJson(400, errJson("Missing name")); return; }
    String name = server.arg("name");
    if (name.startsWith("/")) name = name.substring(1);
    if (name.indexOf('/') >= 0) {
        if (!isValidScriptPath(name)) { sendJson(400, errJson("Invalid path")); return; }
    } else {
        if (!isValidFilename(name)) { sendJson(400, errJson("Invalid filename")); return; }
    }
    String path = "/scripts/" + name;
    if (!SPIFFS.exists(path)) { sendJson(404, errJson("Not found")); return; }
    SPIFFS.remove(path);
    sendJson(200, okJson("Deleted"));
}

void handleRunScript() {
    if (!server.hasArg("name")) { sendJson(400, errJson("Missing name")); return; }
    String name = server.arg("name");
    if (name.startsWith("/")) name = name.substring(1);
    if (name.indexOf('/') >= 0) {
        if (!isValidScriptPath(name)) { sendJson(400, errJson("Invalid path")); return; }
    } else {
        if (!isValidFilename(name)) { sendJson(400, errJson("Invalid filename")); return; }
    }
    if (scriptRunning) { sendJson(400, errJson("Script already running")); return; }
    String path = "/scripts/" + name;
    File f = SPIFFS.open(path, "r");
    if (!f) { sendJson(404, errJson("Not found")); return; }
    String content = f.readString();
    f.close();
    currentScriptName = name;
    scriptRunning = true;
    xTaskCreate([](void* param) {
        String* s = (String*)param;
        runScript(*s);
        delete s;
        vTaskDelete(NULL);
    }, "run", 8192, new String(content), 1, &scriptTaskHandle);
    sendJson(200, okJson("Running"));
}

void handleStopScript() {
    scriptRunning = false;
    setControl(false);
    currentScriptName = "";
    sendJson(200, okJson("Stopped"));
}

void handleGetLog() {
    DynamicJsonDocument doc(2048);
    JsonArray arr = doc.createNestedArray("logs");
    for (int i = 0; i < 50; i++) {
        if (execLog[i].length() > 0) arr.add(execLog[i]);
    }
    String out;
    serializeJson(doc, out);
    sendJson(200, out);
}

void handleStatus() {
    DynamicJsonDocument doc(256);
    doc["running"] = scriptRunning;
    doc["currentScript"] = currentScriptName;
    doc["controlPin"] = (bool)digitalRead(CONTROL_PIN);
    doc["freeHeap"] = (int)ESP.getFreeHeap();
    doc["uptime"] = (int)(millis() / 1000);
    String out;
    serializeJson(doc, out);
    sendJson(200, out);
}

void handlePing() {
    sendJson(200, "{\"status\":\"pong\",\"device\":\"ESP32-Ducky\"}");
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n[BOOT] ESP32-C3 Ducky starting");
    pinMode(CONTROL_PIN, OUTPUT);
    setControl(false);
    if (!SPIFFS.begin(true)) {
        Serial.println("[BOOT] SPIFFS mount failed!");
        return;
    }
    Serial.println("[BOOT] SPIFFS mounted");
    if (!SPIFFS.exists("/scripts")) SPIFFS.mkdir("/scripts");
    loadSettings();
    keyboard.begin();
    USB.begin();
    Serial.println("[USB] HID keyboard ready");
    bootAutoStarted = false;
    if (config.autoAttack && config.autoScript[0] != '\0') {
        delay(100);
        String cfgPath = String(config.autoScript);
        String scriptPath = "";
        if (cfgPath.startsWith("/")) {
            if (cfgPath.startsWith("/scripts/")) {
                scriptPath = cfgPath;
            } else {
                Serial.println("[BOOT] Auto-script path must be under /scripts/ - ignoring");
            }
        } else {
            if (!isValidScriptPath(cfgPath)) {
                Serial.println("[BOOT] Invalid auto-script path - ignoring");
            } else {
                scriptPath = "/scripts/" + cfgPath;
            }
        }
        if (scriptPath.length() > 0) {
            if (SPIFFS.exists(scriptPath)) {
                Serial.printf("[BOOT] Auto-executing: %s\n", cfgPath.c_str());
                File f = SPIFFS.open(scriptPath, "r");
                if (f) {
                    String content = f.readString();
                    f.close();
                    currentScriptName = cfgPath;
                    scriptRunning = true;
                    xTaskCreate([](void* param) {
                        String* s = (String*)param;
                        runScript(*s);
                        delete s;
                        vTaskDelete(NULL);
                    }, "auto", 8192, new String(content), 1, &scriptTaskHandle);
                    bootAutoStarted = true;
                }
            } else {
                Serial.printf("[BOOT] Auto-script not found: %s\n", cfgPath.c_str());
            }
        }
    }
    if (!bootAutoStarted) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(config.ssid, config.password);
        WiFi.softAPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
        Serial.printf("[WIFI] AP  ssid=%s  ip=192.168.1.1\n", config.ssid);
        server.on("/", HTTP_GET, handleRoot);
        server.on("/api/settings", HTTP_GET, handleSettings);
        server.on("/api/settings", HTTP_POST, handleSettings);
        server.on("/api/scripts", HTTP_GET, handleListScripts);
        server.on("/api/scripts/get", HTTP_GET, handleGetScript);
        server.on("/api/scripts/save", HTTP_POST, handleSaveScript);
        server.on("/api/scripts/delete", HTTP_POST, handleDeleteScript);
        server.on("/api/scripts/run", HTTP_POST, handleRunScript);
        server.on("/api/scripts/stop", HTTP_POST, handleStopScript);
        server.on("/api/log", HTTP_GET, handleGetLog);
        server.on("/api/status", HTTP_GET, handleStatus);
        server.on("/api/ping", HTTP_GET, handlePing);
        server.begin();
        Serial.println("[HTTP] Server started  http://192.168.1.1");
    }
}

void loop() {
    if (!bootAutoStarted) {
        server.handleClient();
    }
    delay(2);
}
