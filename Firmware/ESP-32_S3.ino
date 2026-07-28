#include "USB.h"
#include "USBHIDKeyboard.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#define USB_SWITCH_PIN   21          // usb switch gpio pin
#define USB_ENUM_DELAY   100        // ms to wait after gpio21 (Control pin) is high

const char* AP_SSID = "ESP32-S3";
const char* AP_PASS = "Password";   // da password

USBHIDKeyboard Keyboard;
WebServer      server(80);
Preferences    prefs;

String  payload  = "";
bool    running  = false;   


void typePayload(const String &script) {
  int start = 0;
  int len   = script.length();
  while (start < len) {
    int nl = script.indexOf('\n', start);
    if (nl == -1) nl = len;
    String line = script.substring(start, nl);
    line.trim();
    start = nl + 1;
    if (line.length() == 0) continue;
    if (line.startsWith("STRING ")) {
      Keyboard.print(line.substring(7));
    } else if (line.startsWith("DELAY ")) {
      delay(line.substring(6).toInt());
    } else if (line == "ENTER")  { Keyboard.write(KEY_RETURN); }
      else if (line == "TAB")    { Keyboard.write(KEY_TAB);    }
      else if (line == "SPACE")  { Keyboard.write(' ');        }
      else if (line == "UP")     { Keyboard.write(KEY_UP_ARROW);    }
      else if (line == "DOWN")   { Keyboard.write(KEY_DOWN_ARROW);  }
      else if (line == "LEFT")   { Keyboard.write(KEY_LEFT_ARROW);  }
      else if (line == "RIGHT")  { Keyboard.write(KEY_RIGHT_ARROW); }
    else if (line.startsWith("GUI ")  ||
             line.startsWith("CTRL ") ||
             line.startsWith("ALT ")) {
      uint8_t modifier = 0;
      int     keyStart = 0;
      if      (line.startsWith("GUI "))  { modifier = KEY_LEFT_GUI;   keyStart = 4; }
      else if (line.startsWith("CTRL ")) { modifier = KEY_LEFT_CTRL;  keyStart = 5; }
      else if (line.startsWith("ALT "))  { modifier = KEY_LEFT_ALT;   keyStart = 4; }
      String keyStr = line.substring(keyStart);
      keyStr.trim();
      uint8_t key = 0;
      if      (keyStr == "ENTER") key = KEY_RETURN;
      else if (keyStr == "TAB")   key = KEY_TAB;
      else if (keyStr == "SPACE") key = ' ';
      else if (keyStr.length() == 1) key = (uint8_t)keyStr.charAt(0);
      if (key != 0) {
        Keyboard.press(modifier);
        Keyboard.press(key);
        delay(50);
        Keyboard.releaseAll();
      }
    }
  }
}

void usbConnectToTarget() {
  digitalWrite(USB_SWITCH_PIN, HIGH);
}

void usbDisconnectFromTarget() {
  digitalWrite(USB_SWITCH_PIN, LOW);
}
// Webstite code
const char PAGE_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32-S3 bad usb thing</title>
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      background: #0f0f11;
      color: #e2e8f0;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 2rem 1rem;
    }

    .card {
      background: #1a1a1f;
      border: 1px solid #2d2d35;
      border-radius: 12px;
      padding: 2rem;
      width: 100%;
      max-width: 680px;
    }

    h1 {
      font-size: 1.25rem;
      font-weight: 600;
      color: #f1f5f9;
      margin-bottom: 0.25rem;
    }

    .subtitle {
      font-size: 0.8rem;
      color: #64748b;
      margin-bottom: 1.5rem;
    }

    /* Status badge */
    .status-row {
      display: flex;
      align-items: center;
      gap: 0.6rem;
      margin-bottom: 1.5rem;
      padding: 0.6rem 0.9rem;
      border-radius: 8px;
      background: #0f0f11;
      border: 1px solid #2d2d35;
    }
    .dot {
      width: 10px; height: 10px;
      border-radius: 50%;
      flex-shrink: 0;
    }
    .dot.idle    { background: #475569; }
    .dot.ready   { background: #22c55e; box-shadow: 0 0 6px #22c55e88; }
    .dot.running { background: #f59e0b; box-shadow: 0 0 6px #f59e0b88; animation: pulse 1s infinite; }
    @keyframes pulse { 0%,100%{opacity:1} 50%{opacity:.4} }
    #status-text { font-size: 0.85rem; color: #94a3b8; }

    /* Textarea */
    label { font-size: 0.8rem; color: #94a3b8; display: block; margin-bottom: 0.4rem; }
    textarea {
      width: 100%;
      height: 220px;
      background: #0f0f11;
      border: 1px solid #2d2d35;
      border-radius: 8px;
      color: #e2e8f0;
      font-family: "JetBrains Mono", "Fira Code", monospace;
      font-size: 13px;
      padding: 0.8rem;
      resize: vertical;
      outline: none;
      line-height: 1.6;
    }
    textarea:focus { border-color: #4f46e5; }

    /* Buttons */
    .btn-row {
      display: flex;
      gap: 0.75rem;
      margin-top: 1rem;
      flex-wrap: wrap;
    }
    button {
      flex: 1;
      min-width: 120px;
      padding: 0.65rem 1rem;
      border-radius: 8px;
      border: none;
      font-size: 0.9rem;
      font-weight: 500;
      cursor: pointer;
      transition: opacity .15s, transform .1s;
    }
    button:active { transform: scale(.97); }
    button:disabled { opacity: .4; cursor: not-allowed; }

    #btn-save {
      background: #1e293b;
      color: #94a3b8;
      border: 1px solid #2d2d35;
    }
    #btn-save:hover:not(:disabled) { background: #273449; }

    #btn-run {
      background: #4f46e5;
      color: #fff;
    }
    #btn-run:hover:not(:disabled) { background: #4338ca; }

    #btn-abort {
      background: #7f1d1d;
      color: #fca5a5;
      border: 1px solid #991b1b;
      display: none;
    }
    #btn-abort:hover:not(:disabled) { background: #991b1b; }

    .toast {
      position: fixed;
      bottom: 1.5rem;
      left: 50%;
      transform: translateX(-50%) translateY(40px);
      background: #1e293b;
      border: 1px solid #2d2d35;
      color: #e2e8f0;
      padding: 0.6rem 1.2rem;
      border-radius: 8px;
      font-size: 0.85rem;
      opacity: 0;
      transition: all .3s;
      pointer-events: none;
    }
    .toast.show { opacity: 1; transform: translateX(-50%) translateY(0); }

    .hint {
      margin-top: 1.5rem;
      font-size: 0.75rem;
      color: #475569;
      line-height: 1.7;
    }
    .hint code {
      background: #0f0f11;
      padding: 0.1em 0.35em;
      border-radius: 4px;
      color: #94a3b8;
      font-family: monospace;
    }
  </style>
</head>
<body>
  <div class="card">
    <h1>ESP32-S3 badusb thing</h1>
    <p class="subtitle">Bad USB</p>

    <div class="status-row">
      <div class="dot idle" id="status-dot"></div>
      <span id="status-text">USB switch idle — safe path active</span>
    </div>

    <label for="payload-area">Payload script</label>
    <textarea id="payload-area" spellcheck="false">__PAYLOAD__</textarea>

    <div class="btn-row">
      <button id="btn-save"  onclick="savePayload()">Save</button>
      <button id="btn-run"   onclick="runPayload()">Run Payload</button>
      <button id="btn-abort" onclick="abortPayload()"Abort</button>
    </div>

    <div class="hint">
      <strong>Commands:</strong><br>
      <code>STRING text</code> &nbsp;
      <code>DELAY ms</code> &nbsp;
      <code>ENTER</code> &nbsp;
      <code>TAB</code> &nbsp;
      <code>GUI r</code> &nbsp;
      <code>CTRL c</code> &nbsp;
      <code>ALT F4</code> &nbsp;
      <code>UP</code> / <code>DOWN</code> / <code>LEFT</code> / <code>RIGHT</code>
    </div>
  </div>

  <div class="toast" id="toast"></div>

  <script>
    function toast(msg, dur=2200) {
      const t = document.getElementById('toast');
      t.textContent = msg;
      t.classList.add('show');
      setTimeout(() => t.classList.remove('show'), dur);
    }

    function setStatus(state) {
      const dot  = document.getElementById('status-dot');
      const txt  = document.getElementById('status-text');
      const run  = document.getElementById('btn-run');
      const save = document.getElementById('btn-save');
      const abort= document.getElementById('btn-abort');

      dot.className = 'dot ' + state;

      if (state === 'idle') {
        txt.textContent  = 'USB switch idle';
        run.disabled     = false;
        save.disabled    = false;
        abort.style.display = 'none';
        run.style.display   = 'block';
      } else if (state === 'ready') {
        txt.textContent  = 'USB connected';
        run.disabled     = true;
        save.disabled    = true;
      } else if (state === 'running') {
        txt.textContent  = 'Payload executing..';
        abort.style.display = 'block';
        run.style.display   = 'none';
      }
    }

    async function savePayload() {
      const script = document.getElementById('payload-area').value;
      const res = await fetch('/save', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'payload=' + encodeURIComponent(script)
      });
      if (res.ok) toast('Payload saved');
      else        toast('Save failed');
    }

    async function runPayload() {
      await savePayload();
      setStatus('ready');
      const res = await fetch('/run', { method: 'POST' });
      if (res.ok) {
        setStatus('running');
        pollStatus();
      } else {
        toast('Failed to start');
        setStatus('idle');
      }
    }

    async function abortPayload() {
      await fetch('/abort', { method: 'POST' });
      toast('Abort sent');
      setStatus('idle');
    }

    function pollStatus() {
      const iv = setInterval(async () => {
        try {
          const r = await fetch('/status');
          const j = await r.json();
          if (j.running === false) {
            clearInterval(iv);
            setStatus('idle');
            toast('Payload complete ✓', 3000);
          }
        } catch(e) { /* keep polling */ }
      }, 800);
    }
  </script>
</body>
</html>
)rawhtml";
//  HTTP handlrels

void handleRoot() {
  String page = String(PAGE_HTML);
  String safe = payload;
  safe.replace("&", "&amp;");
  safe.replace("<", "&lt;");
  safe.replace(">", "&gt;");
  page.replace("__PAYLOAD__", safe);
  server.send(200, "text/html", page);
}

void handleSave() {
  if (server.hasArg("payload")) {
    payload = server.arg("payload");
    prefs.putString("payload", payload);
    server.send(200, "text/plain", "ok");
  } else {
    server.send(400, "text/plain", "missing payload");
  }
}

void handleRun() {
  if (running) {
    server.send(409, "text/plain", "alredy running");
    return;
  }
  running = true;
  server.send(200, "text/plain", "ok");
  usbConnectToTarget();
  delay(USB_ENUM_DELAY);    
  typePayload(payload);
  usbDisconnectFromTarget();
  running = false;
}

void handleAbort() {
  Keyboard.releaseAll();
  usbDisconnectFromTarget();
  running = false;
  server.send(200, "text/plain", "aborted");
}

void handleStatus() {
  String json = "{\"running\":";
  json += running ? "true" : "false";
  json += "}";
  server.send(200, "application/json", json);
}


// Setup
void setup() {
  pinMode(USB_SWITCH_PIN, OUTPUT);
  digitalWrite(USB_SWITCH_PIN, LOW);
  prefs.begin("badusb", false);
  payload = prefs.getString("payload",
    "DELAY 500\nGUI r\nDELAY 600\nSTRING notepad\nENTER\n"
    "DELAY 1000\nSTRING Hello WORld!!\nENTER");
  Keyboard.begin();
  USB.begin();
  WiFi.softAP(AP_SSID, AP_PASS);
  server.on("/",       HTTP_GET,  handleRoot);
  server.on("/save",   HTTP_POST, handleSave);
  server.on("/run",    HTTP_POST, handleRun);
  server.on("/abort",  HTTP_POST, handleAbort);
  server.on("/status", HTTP_GET,  handleStatus);
  server.begin();
}
// LOOP LOOP
void loop() {
  if (!running) {
    server.handleClient();
  }
}
