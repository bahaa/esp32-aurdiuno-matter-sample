#include <Matter.h>
#include <WiFi.h>
#include <Preferences.h>

#include "secrets.h"
#include "config.h"

// List of Matter Endpoints for this Node
// On/Off Light Endpoint
MatterOnOffLight on_off_light;

// it will keep last OnOff state stored, using Preferences
Preferences preferences;
constexpr auto *on_off_pref_key = "OnOff";

// set your board USER BUTTON pin here
constexpr auto button_pin = BOOT_PIN; // Set your pin here. Using BOOT Button.

// Button control
uint32_t button_time_stamp = 0; // debouncing control
bool button_state = false; // false = released | true = pressed
constexpr auto debounce_time = 250; // button debouncing time (ms)
constexpr auto decommissioning_timeout = 5000; // keep the button pressed for 5s, or longer, to decommission

// Matter Protocol Endpoint Callback
bool set_light_on_off(const bool state) {
  Serial.printf("User Callback :: New Light State = %s\r\n", state ? "ON" : "OFF");
  if (state) {
    digitalWrite(BUILTIN_LED_PIN, HIGH);
  } else {
    digitalWrite(BUILTIN_LED_PIN, LOW);
  }
  // store last OnOff state for when the Light is restarted / power goes off
  preferences.putBool(on_off_pref_key, state);
  // This callback must return the success state to Matter core
  return true;
}

void setup() {
  // Initialize the USER BUTTON (Boot button) GPIO that will act as a toggle switch
  pinMode(button_pin, INPUT_PULLUP);
  // Initialize the LED (light) GPIO and Matter End Point
  pinMode(BUILTIN_LED_PIN, OUTPUT);

  Serial.begin(115200);

  // We start by connecting to a Wi-Fi network
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  // Manually connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\r\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(500);

  // Initialize Matter EndPoint
  preferences.begin("MatterPrefs", false);
  const auto last_on_off_state = preferences.getBool(on_off_pref_key, false);

  on_off_light.begin(last_on_off_state);
  on_off_light.onChange([](const bool state) {
    Serial.printf("User Callback :: New Light State = %s\r\n", state ? "ON" : "OFF");
    if (state) {
      digitalWrite(BUILTIN_LED_PIN, HIGH);
    } else {
      digitalWrite(BUILTIN_LED_PIN, LOW);
    }
    // store last OnOff state for when the Light is restarted / power goes off
    preferences.putBool(on_off_pref_key, state);
    // This callback must return the success state to Matter core
    return true;
  });

  on_off_light.onIdentify([](bool cond) {
    digitalWrite(BUILTIN_LED_PIN, HIGH);
    delay(500);
    digitalWrite(BUILTIN_LED_PIN, LOW);
    delay(500);
    digitalWrite(BUILTIN_LED_PIN, HIGH);
    delay(500);
    digitalWrite(BUILTIN_LED_PIN, LOW);
    delay(500);
    return true;
  });

  // Matter beginning - Last step, after all EndPoints are initialized
  ArduinoMatter::begin();
  // This may be a restart of an already commissioned Matter accessory
  if (ArduinoMatter::isDeviceCommissioned()) {
    Serial.println("Matter Node is commissioned and connected to Wi-Fi. Ready for use.");
    Serial.printf("Initial state: %s\r\n", on_off_light.getOnOff() ? "ON" : "OFF");
    on_off_light.updateAccessory(); // configure the Light based on initial state
  }
}

void loop() {
  // Check Matter Light Commissioning state, which may change during execution of loop()
  if (!ArduinoMatter::isDeviceCommissioned()) {
    Serial.println("");
    Serial.println("Matter Node is not commissioned yet.");
    Serial.println("Initiate the device discovery in your Matter environment.");
    Serial.println("Commission it to your Matter hub with the manual pairing code or QR code");
    Serial.printf("Manual pairing code: %s\r\n", ArduinoMatter::getManualPairingCode().c_str());
    Serial.printf("QR code URL: %s\r\n", ArduinoMatter::getOnboardingQRCodeUrl().c_str());
    // waits for Matter Light Commissioning.
    uint32_t timeCount = 0;
    while (!ArduinoMatter::isDeviceCommissioned()) {
      delay(100);
      if (timeCount++ % 50 == 0) {
        // 50*100ms = 5 sec
        Serial.println("Matter Node not commissioned yet. Waiting for commissioning.");
      }
    }
    Serial.printf("Initial state: %s\r\n", on_off_light.getOnOff() ? "ON" : "OFF");
    on_off_light.updateAccessory(); // configure the Light based on initial state
    Serial.println("Matter Node is commissioned and connected to Wi-Fi. Ready for use.");
  }

  // A button is also used to control the light
  // Check if the button has been pressed
  if (digitalRead(button_pin) == LOW && !button_state) {
    // deals with button debouncing
    button_time_stamp = millis(); // record the time while the button is pressed.
    button_state = true; // pressed.
  }

  // Onboard User Button is used as a Light toggle switch or to decommission it
  const auto time_diff = millis() - button_time_stamp;
  if (button_state && time_diff > debounce_time && digitalRead(button_pin) == HIGH) {
    button_state = false; // released
    // Toggle button is released - toggle the light
    Serial.println("User button released. Toggling Light!");
    on_off_light.toggle(); // Matter Controller also can see the change
  }

  // Onboard User Button is kept pressed for longer than 5 seconds in order to decommission matter node
  if (button_state && time_diff > decommissioning_timeout) {
    Serial.println("Decommissioning the Light Matter Accessory. It shall be commissioned again.");
    on_off_light.setOnOff(false); // turn the light off
    ArduinoMatter::decommission();
    button_time_stamp = millis(); // avoid running decommissioning again, reboot takes a second or so
  }
}
