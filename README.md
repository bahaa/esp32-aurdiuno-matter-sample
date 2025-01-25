# ESP32 Arduino Matter Sample with PlatformIO

This is sample of how to use ESP32 Arduino Matter library with PlatformIO project. You might need to use
[pioarduino](https://github.com/pioarduino/pioarduino-vscode-ide) fork for this to work.

This needs ESP32 Arduino 3.x or later, and PlatformIO don't support this version out of the box.

## `platformio.ini` Modifications
First you'll notice we used the URL of `platform-espressif32` directly. This is to make sure we use the platform version
that supports Arduino core 3.x.

```ini
platform = https://github.com/pioarduino/platform-espressif32/releases/download/53.03.11/platform-espressif32.zip
```

Second, we added `-DCHIP_HAVE_CONFIG_H` to compiler flags so CHIP and ESP Matter would build successfully:
```ini
build_flags =
    -DCHIP_HAVE_CONFIG_H
```

## Building

Add `secrets.h` to `include` directory. Copy this to the file and  replace `<WIFI_SSID>` and `<WIFI_PASSWORD>` with your actual Wi-Fi SSID and 
password. 

```c++
#pragma once

constexpr auto WIFI_SSID = "<WIFI_SSID>";
constexpr auto WIFI_PASSWORD = "<WIFI_PASSWORD>";
```


