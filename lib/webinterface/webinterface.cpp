#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "webinterface.h"
#include "movements.h"
#include "operations.h"
#include "dispenser.h"

///*
#define AP_SSID "DipCoater"
#define AP_PW "michaelsohn"
#define mdns "dipcoater"
#define WIFI_CORE 0
#define TASK_CORE 1
//*/

// Establish the dispenser task and semaphore
static TaskHandle_t dispenserTaskHandle = NULL;
static SemaphoreHandle_t dispenseDoneSem = NULL;
static volatile int dispenserNumSubstrates = 0;

struct CycleParams {
    float diptime1, drytime1, mmpermin;
    float diptime2, drytime2;
    float diptime3, drytime3;
    int cycles;
    int numsub;
};

static volatile bool startRequested = false;
static volatile bool estopRequested = false;
static volatile bool systemBusy = false;
static CycleParams pendingParams;
static TaskHandle_t motorTaskHandle = NULL;

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Dip Coater Control</title>
  <style>
    body { font-family: sans-serif; max-width: 420px; margin: 40px auto; padding: 0 16px; }
    h1 { font-size: 1.3em; }
    label { display: block; margin-top: 14px; font-size: 0.9em; color: #333; }
    input { width: 100%; padding: 8px; font-size: 1em; box-sizing: border-box; margin-top: 4px; }
    button { width: 100%; padding: 12px; font-size: 1.1em; margin-top: 20px; border: none; border-radius: 4px; cursor: pointer; }
    table { width: 100%; border-collapse: collapse; margin-top: 10px; }
    th, td { padding: 6px; text-align: center; }
    th { font-size: 0.85em; color: #333; }
    input { width: 100%; padding: 6px; box-sizing: border-box; }
    #startBtn { background: #2e7d32; color: white; }
    #startBtn:disabled { background: #999; }
    #estopBtn { background: #c62828; color: white; }
    #status { margin-top: 16px; font-size: 0.9em; color: #555; text-align: center; }
  </style>
</head>
<body>
  <h1>Dip Coater Control</h1>
  <tr>________________________________________________</tr>
    <table>
    <tr><th>_______________</th><th>Solution 1</th><th>Solution 2</th><th>Solution 3</th></tr>
    <tr>
        <td>Dip time (s)</td>
        <td><input type="number" id="diptime1" step="0.1" min="0" value="1"></td>
        <td><input type="number" id="diptime2" step="0.1" min="0" value="1"></td>
        <td><input type="number" id="diptime3" step="0.1" min="0" value="1"></td>
    </tr>
    <tr>
        <td>Dry time (s)</td>
        <td><input type="number" id="drytime1" step="0.1" min="0" value="1"></td>
        <td><input type="number" id="drytime2" step="0.1" min="0" value="1"></td>
        <td><input type="number" id="drytime3" step="0.1" min="0" value="1"></td>
    </tr>
    </table>

  <label>Withdrawal speed (mm/min)</label>
  <input type="number" id="mmpermin" step="1" min="1" value="600">

  <label>Number of cycles</label>
  <input type="number" id="cycles" step="1" min="1" value="1">
  <label>Number of substrates (beakers to dispense into)</label>
  <select id="numsub">
    <option value="1" selected>1</option>
    <option value="2">2</option>
    <option value="3">3</option>
  </select>
  
  <tr>________________________________________________</tr>

  <button id="startBtn" onclick="startCycle()">Start</button>
  <button id="estopBtn" onclick="estop()">EMERGENCY STOP</button>

  <div id="status">Idle</div>

    <script>
    async function startCycle() {
        const params = new URLSearchParams({
        diptime1: diptime1.value, drytime1: drytime1.value, mmpermin: mmpermin.value,
        diptime2: diptime2.value, drytime2: drytime2.value,
        diptime3: diptime3.value, drytime3: drytime3.value,
        cycles: cycles.value, numsub: numsub.value
        });
        const res = await fetch('/start?' + params.toString());
        status.innerText = await res.text();
        poll();
    }
    async function estop() {
        const res = await fetch('/estop');
        status.innerText = await res.text();
        poll();
    }
    async function poll() {
        const res = await fetch('/status');
        const data = await res.json();
        status.innerText = data.busy ? 'Running...' : 'Idle';
        startBtn.disabled = data.busy;
        if (data.busy) setTimeout(poll, 1000);
    }
    poll();
    </script>
</body>
</html>
)rawliteral";

WebServer server(80);

// This handles the root page
static void handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

// This gets the input from the web interface
static void handleStart() {
    if (systemBusy) {
        server.send(409, "text/plain", "Busy: a cycle is already running");
        return;
    }
    const char* required[] = {"diptime1","drytime1","mmpermin",
                               "diptime2","drytime2",
                               "diptime3","drytime3","cycles","numsub"};
    for (auto name : required) {
        if (!server.hasArg(name)) {
            server.send(400, "text/plain", String("Missing parameter: ") + name);
            return;
        }
    }

    pendingParams.diptime1  = server.arg("diptime1").toFloat();
    pendingParams.drytime1  = server.arg("drytime1").toFloat();
    pendingParams.mmpermin  = server.arg("mmpermin").toFloat();
    pendingParams.diptime2  = server.arg("diptime2").toFloat();
    pendingParams.drytime2  = server.arg("drytime2").toFloat();
    pendingParams.diptime3  = server.arg("diptime3").toFloat();
    pendingParams.drytime3  = server.arg("drytime3").toFloat();
    pendingParams.cycles    = server.arg("cycles").toInt();
    pendingParams.numsub    = server.arg("numsub").toInt();
    if (pendingParams.cycles < 1) pendingParams.cycles = 1;
    if (pendingParams.numsub < 1) pendingParams.numsub = 1;
    if (pendingParams.numsub > 3) pendingParams.numsub = 3;

    estopRequested = false;
    abortMotion = false;
    startRequested = true;
    server.send(200, "text/plain", "Started");
}

// This sets the emergency stop flag
static void handleEstop() {
    estopRequested = true;   // tells the running cycle loop to abort further cycles
    abortMotion = true;
    server.send(200, "text/plain", "Emergency stop triggered");
}

// This sets the status flag
static void handleStatus() {
    String json = "{\"busy\":";
    json += systemBusy ? "true" : "false";
    json += "}";
    server.send(200, "application/json", json);
}

// This sets up the web server
void setupWebServer() {
    server.on("/", handleRoot);
    server.on("/start", handleStart);
    server.on("/estop", handleEstop);
    server.on("/status", handleStatus);
    server.begin();
    Serial.println("Web server started");
}

// This allows people to access the link.
void handleWebServer() {
    server.handleClient();
}

// Establish Task Handle for motor task (for FreeRTOS)
// *** EDIT THIS FOR THE ACTUAL DIPPING PROCESS ***
static void motorTask(void *param) {
    for (;;) {
        if (abortMotion) {
            estop();
            xQueueReset(dispenseDoneSem);
            estopRequested = false;
            systemBusy = false;
        }
        else if (startRequested) {
            startRequested = false;
            systemBusy = true;

            CycleParams p = pendingParams;

            if (p.cycles > 0) {
                enablemotor();
                //turnoff();
            }
            // *** EDIT THIS FOR THE ACTUAL DIPPING PROCESS ***
            for (int i = 0; i < p.cycles && !estopRequested; i++) {
                // Write the entire code sequence for the dipcoating cycle
                dipSolution1(p.diptime1, p.drytime1, p.mmpermin);
                if (estopRequested) break;
                dipSolution2(p.diptime2, p.drytime2);
                if (estopRequested) break;

                // Notify the dispenser task to dispense
                dispenserNumSubstrates = p.numsub;
                xTaskNotifyGive(dispenserTaskHandle);

                dipSolution3(p.diptime3, p.drytime3);
                // Never block indefinitely here — an e-stop has to be able to
                // reclaim the task even if the dispenser is stuck.
                const TickType_t dispenseTimeout = pdMS_TO_TICKS(30000); // set above your longest legit dispense
                if (xSemaphoreTake(dispenseDoneSem, dispenseTimeout) != pdTRUE) {
                    // Dispenser didn't report done in time — force it to abort
                    // and do one bounded reap so its eventual give doesn't leak
                    // into the next Start.
                    abortMotion = true;
                    xSemaphoreTake(dispenseDoneSem, pdMS_TO_TICKS(5000));
                }

                if (estopRequested) break;
            }

            if (!estopRequested) {
                turnoff();
                disablemotor();
            }

            systemBusy = false;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

// Establish Task Handle for dispenser task (for FreeRTOS)
static void dispenserTask(void *param) {
    for (;;) {
        // sleeps here until motorTask wakes it up for a dispense pass
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        dispense(dispenserNumSubstrates);
        xSemaphoreGive(dispenseDoneSem);
    }
}

// Create FreeRTOS task for motor
void startMotorTask() {
    xTaskCreatePinnedToCore(
        motorTask,
        "motorTask",
        8192,
        NULL,
        1,
        &motorTaskHandle,
        WIFI_CORE   // core 0: keeps the web server (core 1, default) responsive during a run
    );
}

// Create FreeRTOS task for dispenser
void startDispenserTask() {
    dispenseDoneSem = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(
        dispenserTask,
        "dispenserTask",
        4096,
        NULL,
        1,
        &dispenserTaskHandle,
        WIFI_CORE   
    );
}

// Establish wifi connection
void setupwifi() {
    WiFi.mode(WIFI_AP);
    bool apStarted = WiFi.softAP(AP_SSID, AP_PW);
    if (!apStarted) {
        Serial.println("Failed to start AP");
        return;
    } else {
        Serial.println("AP started");
        Serial.println("Connect to the WiFi network: " + String(AP_SSID));
        Serial.println("Access Web Interface via http://" + WiFi.softAPIP().toString());
    }
   
    if (!MDNS.begin(mdns)) {
        Serial.println("Failed to start MDNS");
    } else {
        Serial.println("or Access the web interface at http://dipcoater.local");
    }       // set domain name to dipcoater.local
}