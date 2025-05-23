#include "OtaPage.h"

// Definições (só aparecem UMA vez no projeto)
const char otaPageTemplate[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP-NOW OTA Update</title>
  <style>
    body { font-family: Arial, sans-serif; max-width: 600px; margin: 0 auto; padding: 20px; }
    .input-group { margin-bottom: 15px; }
    label { display: block; margin-bottom: 5px; font-weight: bold; }
    select, button { width: 100%; padding: 10px; box-sizing: border-box; }
    button { background-color: #4CAF50; color: white; border: none; cursor: pointer; }
    button:hover { background-color: #45a049; }
  </style>
</head>
<body>
  <h2>ESP-NOW OTA Update</h2>
  <div class="input-group">
    <label for="deviceName">Source device:</label>
    <select id="deviceName" required>
      <option value="">Select a device</option>
      %DEVICE_OPTIONS%
    </select>
  </div>

  <button onclick="updateMode()">Start Update Mode</button>

  <script>
    async function updateMode() {
      const deviceName = document.getElementById("deviceName").value;
      if (!deviceName) {
        alert("Please, select a device.");
        return;
      }

      if (!confirm(`Activate UPDATE mode for: ${deviceName}?`)) {
        return;
      }

      try {
        const response = await fetch("/updateMode", {
          method: "POST",
          headers: {
            "X-Device-Name": deviceName
          }
        });

        if (response.ok) {
          alert("Update mode activated!");
        } else {
          const errorText = await response.text();
          throw new Error(errorText);
        }
      } catch (error) {
        alert("Error on activation: " + error.message);
        console.error(error);
      }
    }
  </script>
</body>
</html>)rawliteral";

String generateHtml(const std::vector<String>& devices) { 
  String html = FPSTR(otaPageTemplate);
  
  String optionsName;
  for (const auto& device : devices) { // Usa o parâmetro
    optionsName += "<option value=\"" + device + "\">" + device + "</option>";
  }
  html.replace("%DEVICE_OPTIONS%", optionsName);
  return html;
}