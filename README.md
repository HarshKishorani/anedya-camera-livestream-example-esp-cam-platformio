[<img src="https://img.shields.io/badge/Anedya-Documentation-blue?style=for-the-badge">](https://docs.anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp-cam)

# ESP32 Camera — WebRTC Camera Livestream with Anedya (Arduino)

Arduino port of the [ESP-IDF example](../anedya-camera-livestream-example-esp-cam). Same
devices, same protocol, same browser viewer — written with the Arduino API
(`setup()` / `loop()`, `WiFi.h`, PubSubClient, ArduinoJson, `esp_camera`)
instead of raw ESP-IDF.

## 📟 Supported Boards

| Board | Chip | Flash / PSRAM | PlatformIO environment |
|---|---|---|---|
| Seeed Studio XIAO ESP32S3 **Sense** | ESP32-S3 | 8 MB / 8 MB **octal** | `seeed_xiao_esp32s3` |
| DFRobot FireBeetle 2 ESP32-S3 (N16R8) | ESP32-S3 | 16 MB / 8 MB **octal** | `dfrobot_firebeetle2_esp32s3` |
| DFRobot Romeo ESP32-S3 | ESP32-S3 | 16 MB / 8 MB **octal** | `dfrobot_romeo_esp32s3` |
| ESP32-CAM style — AI Thinker and clones | ESP32 | 4 MB / 4 MB **quad** | `esp32cam` |

Pick one at build time; there is nothing to edit in the source:

```bash
pio run -e seeed_xiao_esp32s3          -t upload -t monitor
pio run -e dfrobot_firebeetle2_esp32s3 -t upload -t monitor
pio run -e dfrobot_romeo_esp32s3       -t upload -t monitor
pio run -e esp32cam                    -t upload -t monitor
```

The two DFRobot S3 boards share one camera pinout, so they share a pin map
(`CAMERA_MODEL_DFROBOT_ESP32S3`) — they differ only in their board manifests.

The environment supplies the camera pin map (`-DCAMERA_MODEL_*`) and the
partition table, and ESP-IDF picks up the matching `sdkconfig.defaults.<target>`
for flash size and PSRAM mode. This mirrors how the ESP-IDF example switches
boards with `idf.py set-target`.

## ✨ Features

- **Live JPEG streaming** — camera frames sent over a WebRTC DataChannel
- **Anedya Commands signaling** — SDP offer/answer over MQTT, no signaling server
- **Anedya TURN relay** — works through firewalls, credentials come with the offer
- **Configurable profile** — frame size, JPEG quality, FPS, buffer count in one header
- **DataChannel test mode** — prove signaling works without a camera attached
- **Zero-copy frame path** — the camera frame buffer is handed straight to the
  send pipeline and released only after transmission

---

## ⚠️ Read this first: why PlatformIO and not the Arduino IDE

**This is Arduino code, but it does not build in the Arduino IDE.** That is not a
style choice — it is a hard limitation:

| Requirement | Arduino IDE | This project |
|---|---|---|
| Pull `espressif/esp_peer` (WebRTC) | ✗ no ESP-IDF Component Manager | ✓ `src/idf_component.yml` |
| `CONFIG_MBEDTLS_SSL_DTLS_SRTP=y` | ✗ core libs are precompiled, and this option is **off** | ✓ `sdkconfig.defaults` |
| `CONFIG_MBEDTLS_X509_CREATE_C=y` (esp_peer self-signs its DTLS cert) | ✗ | ✓ |
| Octal PSRAM + custom partition table | ~ partly | ✓ |

The Arduino core ships as prebuilt `.a` libraries with a fixed `sdkconfig`. It
enables `MBEDTLS_SSL_PROTO_DTLS` but **not** `MBEDTLS_SSL_DTLS_SRTP`, so
esp_peer cannot link there at all.

PlatformIO's hybrid mode — `framework = arduino, espidf` — builds ESP-IDF from
source **with Arduino as a component**. You write ordinary Arduino code and
still get managed components and `sdkconfig` control. Sketch files are `.cpp`
rather than `.ino`; that is the only difference in how you write code.

> If you ever want a genuine `.ino` in the Arduino IDE, the only route is to
> rebuild the core yourself with
> [esp32-arduino-lib-builder](https://github.com/espressif/esp32-arduino-lib-builder)
> and `CONFIG_MBEDTLS_SSL_DTLS_SRTP=y` added to `configs/defconfig.common`.

---

## 🏗 How It Works

### Signaling via Anedya Commands + MQTT

WebRTC peers must exchange SDP offers and answers before media can flow. This
example uses Anedya Commands as the signaling channel and Anedya MQTT as the
delivery mechanism.

```
Browser Viewer
  │  1. Fetch TURN credentials (Anedya REST API)
  │  2. POST /commands/send  command="webrtc_offer"
  │     data = base64(deflate-raw({sdp, turn}))
  ▼
Anedya Cloud  (Commands + MQTT broker + TURN relay)
  │  3. Push to $anedya/device/<id>/commands
  ▼
XIAO ESP32S3
  │  4. Decode offer, extract SDP + TURN credentials
  │  5. Publish status "processing" with ackdata = base64(deflate-raw(answer SDP))
  ▼
Browser Viewer
  │  6. Poll /commands/getDetails → read ackdata → setRemoteDescription
  │  7. ICE negotiation completes
  │  8. JPEG frames flow over the DataChannel → rendered in <img>
```

Command status flow: `received` → `processing` (carries the answer) →
`success` | `failure`. `success`/`failure` are terminal in Anedya, so the
firmware only sends them once WebRTC has actually connected or failed.

### JPEG over DataChannel

This project does not use WebRTC RTP video tracks. Camera JPEG frames are sent
as binary messages on a DataChannel labeled `jpeg-test`; the browser updates an
`<img>` per frame. Simple to inspect from both C++ and JavaScript.

---

## 📁 Repository Layout

```
.
├── platformio.ini            — pioarduino platform, one [env:] per board
├── sdkconfig.defaults        — shared: DTLS-SRTP, PSK, 1 kHz tick
├── sdkconfig.defaults.esp32s3 — XIAO: 8 MB flash, octal PSRAM, USB console
├── sdkconfig.defaults.esp32  — ESP32-CAM: 4 MB flash, quad PSRAM + cache erratum
├── partitions.esp32s3.csv    — 8 MB flash, single factory app
├── partitions.csv            — 4 MB flash, single factory app
├── include/
│   ├── app_config.h          — ★ WiFi + Anedya credentials + all tuning
│   ├── camera_pins.h         — camera pin maps, selected by -DCAMERA_MODEL_*
│   ├── anedya_signaling.h
│   └── webrtc_peer.h
├── src/
│   ├── idf_component.yml     — esp_peer, esp32-camera (ESP-IDF managed components)
│   ├── CMakeLists.txt
│   ├── main.cpp              — setup()/loop(): camera capture + MQTT pump
│   ├── anedya_signaling.cpp  — PubSubClient MQTT, Commands-based signaling
│   └── webrtc_peer.cpp       — esp_peer peer connection, DataChannel send pipeline
└── test-peer/
    └── index.html            — browser viewer (open directly, no server needed)
```

### What changed vs. the ESP-IDF example

| ESP-IDF version | Arduino version |
|---|---|
| `anedya/anedya-esp` SDK | PubSubClient + ArduinoJson against the same MQTT endpoints |
| `menuconfig` / `Kconfig.projbuild` | `include/app_config.h` |
| `idf.py set-target esp32/esp32s3` | `pio run -e esp32cam` / `-e seeed_xiao_esp32s3` |
| `boards.h` + `CONFIG_CAMERA_BOARD_*` | `camera_pins.h` + `-DCAMERA_MODEL_*` per env |
| `protocol_examples_common` `example_connect()` | `WiFi.begin()` |
| `app_main()` + `jpeg_stream_task` FreeRTOS task | `setup()` + `loop()` |
| `esp_peer` peer-loop task | unchanged — still its own core-pinned task |

The esp_peer layer is a near-line-for-line port; the wire protocol is byte
identical, so **the same browser viewer drives either firmware.**

---

## 🚀 Getting Started

### What You Need

**Hardware** — either board

- Seeed Studio XIAO ESP32S3 **Sense** + USB-C cable. The Sense expansion board
  carries the OV2640 camera; the plain XIAO ESP32S3 has no camera. Native USB,
  so no programmer needed.
- An ESP32-CAM style board (AI Thinker, DFRobot, clones) **+ a USB-to-serial
  programmer** (FTDI, CP2102). These modules have no USB port of their own.

> [!IMPORTANT]
> Both boards need PSRAM — a single HVGA JPEG frame buffer is far larger than
> the free internal DRAM, so without it `esp_camera_init()` fails outright.
> ESP32-CAM boards sold without PSRAM will not work with this example.

**Software / Accounts**
- [PlatformIO](https://platformio.org/install) (VS Code extension or `pip install platformio`)
- An [Anedya](https://anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp-cam) account

---

### Step 1: Create Your Anedya Project

1. Sign in at the [Anedya Console](https://accounts.anedya.io/ui/login).
2. Create a project.
3. Create a node for your camera and pre-authorize it with a UUID.
4. Note these values:

| Value | Where to find it | Used by |
|---|---|---|
| Device ID | Node details → Device ID | firmware (`app_config.h`) |
| Connection Key | Node details → Connection Key | firmware (`app_config.h`) |
| Node ID | Node details → Node ID | browser viewer |
| Platform API key | Project → API keys | browser viewer |

> [!TIP]
> See [Anedya Project Setup](https://docs.anedya.io/getting-started/project-setup/)
> for a walkthrough of the console.

---

### Step 2: Configure the Firmware

Everything lives in [include/app_config.h](include/app_config.h):

```c
#define WIFI_SSID     "your-ssid"
#define WIFI_PASSWORD "your-password"

#define ANEDYA_DEVICE_ID      "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
#define ANEDYA_CONNECTION_KEY "your-connection-key"
#define ANEDYA_REGION_CODE    "ap-in-1"
```

The Node ID is **not** needed in firmware — it goes in the browser viewer.

---

### Step 3: Build & Flash

Choose the environment for your board:

```bash
cd anedya-camera-livestream-example-esp-cam-arduino

# Seeed XIAO ESP32S3 Sense
pio run -e seeed_xiao_esp32s3 -t upload -t monitor

# ESP32-CAM style board (AI Thinker, DFRobot, clones)
pio run -e esp32cam -t upload -t monitor
```

The first build downloads the pioarduino toolchain and the `esp_peer` /
`esp32-camera` managed components — expect several minutes. Later builds are fast.

**Flashing an ESP32-CAM board:** wire the programmer (5V, GND, U0T→RX, U0R→TX),
connect **IO0 to GND**, press reset to enter the bootloader, upload, then
**disconnect IO0** and reset again. IO0 doubles as the camera's XCLK pin, so the
camera will not run while it is strapped to ground.

A healthy boot looks like:

```
I (xxx) webrtc: Board:     Seeed XIAO ESP32S3 Sense
I (xxx) webrtc: PSRAM:     yes (8192 KB)
I (xxx) webrtc: Sensor:    OV2640 (PID=0x2640)
I (xxx) webrtc: WiFi connected, IP 192.168.1.42
I (xxx) anedya_signaling: Connected to Anedya broker
I (xxx) anedya_signaling: Anedya signaling ready — waiting for a viewer
I (xxx) webrtc_peer: esp_peer opened, starting main loop task
```

---

### Step 4: Connect a Viewer

Open [test-peer/index.html](test-peer/index.html) in a browser, then:

1. Click **Settings**
2. Enter your **Node ID** and **Platform API key**
3. Click **Start Stream**

The viewer fetches TURN credentials from Anedya, sends the offer as a command,
and waits for the answer. Once the DataChannel opens, frames appear.

---

## 🎛 Camera Stream Settings

Defaults target balanced quality at 20 FPS ([app_config.h](include/app_config.h)):

| Setting | Value |
|---|---|
| Frame size | HVGA (480 × 320) |
| JPEG quality | 25 |
| Frame buffer count | 2 |
| Target FPS | 20 |

Max-FPS / lower-quality preset:

```c
#define CAMERA_STREAM_FRAME_SIZE   FRAMESIZE_QVGA
#define CAMERA_STREAM_FRAME_NAME   "QVGA"
#define CAMERA_STREAM_JPEG_QUALITY 20
#define CAMERA_STREAM_FB_COUNT     3
```

Delivered FPS in the browser is bounded by DataChannel bandwidth
(≈ 600 kbps ÷ frame bytes), not by this setting alone.

---

## 🔌 DataChannel Test Mode

Uncomment `#define DATACHANNEL_TEST_MODE` in `app_config.h` to skip camera init and
send `ping N from esp32` over the DataChannel instead. Use it to separate
"signaling/WebRTC is broken" from "the camera is broken".

---

## 🧵 Threading Model

| Task | Core | Priority | Does |
|---|---|---|---|
| `loopTask` (Arduino) | 1 | 1 | `loop()`: camera capture + MQTT/TLS |
| `peer_loop` | 1 | 18 | every `esp_peer_*` call, 10 ms tick |
| WiFi / lwIP | 0 | high | radio + TCP/IP |

esp_peer is **cooperative and not thread-safe**, so every `esp_peer_*` call has
to happen on one task, and that task must tick tightly at high priority or the
ICE/DTLS handshake gets preempted mid-flight. That is why the WebRTC layer is
its own pinned task rather than part of `loop()`.

Consequences, both handled in the code:
- `anedyaSignalingWriteAnswer()` and `anedyaSignalingConclude()` are called from
  the peer task, so they **queue** the MQTT publish for `loop()` instead of
  publishing inline (PubSubClient is not thread-safe, and a blocking TLS write
  from a priority-18 task would stall the handshake).
- The active command id is shared between the two tasks behind a mutex.

---

## 🔧 Hardware

| Property | Seeed XIAO ESP32S3 Sense | DFRobot ESP32-S3 | ESP32-CAM style |
|---|---|---|---|
| Chip | ESP32-S3 (dual LX7, 240 MHz) | ESP32-S3 (dual LX7, 240 MHz) | ESP32 (dual LX6, 160 MHz) |
| Flash | 8 MB | 16 MB | 4 MB |
| PSRAM | 8 MB **octal** | 8 MB **octal** | 4 MB **quad** |
| USB | native USB Serial/JTAG | native USB Serial/JTAG | none — external programmer |
| Camera | OV2640 (Sense board) | OV2640 (FPC connector) | OV2640 / OV3660 |

The firmware is ~1.33 MB, so the 8 MB partition table is reused unchanged on
the 16 MB DFRobot boards — they simply leave the upper 8 MB unpartitioned.

> [!IMPORTANT]
> PSRAM mode is not interchangeable. The XIAO needs `CONFIG_SPIRAM_MODE_OCT=y`,
> ESP32-CAM needs `CONFIG_SPIRAM_MODE_QUAD=y` — the wrong one silently reports
> "no PSRAM found" and camera init then fails. Each board's
> `sdkconfig.defaults.<target>` sets this, so it is only a concern if you add a
> new board.

> [!NOTE]
> The original ESP32 has a silicon erratum where PSRAM access can corrupt data
> while certain libc routines run from flash. `sdkconfig.defaults.esp32` enables
> `CONFIG_SPIRAM_CACHE_WORKAROUND` and relocates those routines into IRAM. That
> is why the ESP32 build has noticeably less internal RAM free than the S3 one.

### Camera pin maps

| Signal | XIAO ESP32S3 Sense | DFRobot ESP32-S3 | ESP32-CAM |
|---|---|---|---|
| PWDN | — | — | 32 |
| RESET | — | — | — |
| XCLK | 10 | 45 | 0 |
| SIOD (SDA) | 40 | 1 | 26 |
| SIOC (SCL) | 39 | 2 | 27 |
| D7 | 48 | 48 | 35 |
| D6 | 11 | 46 | 34 |
| D5 | 12 | 8 | 39 |
| D4 | 14 | 7 | 36 |
| D3 | 16 | 4 | 21 |
| D2 | 18 | 41 | 19 |
| D1 | 17 | 40 | 18 |
| D0 | 15 | 39 | 5 |
| VSYNC | 38 | 6 | 25 |
| HREF | 47 | 42 | 23 |
| PCLK | 13 | 5 | 22 |

DFRobot pin values are from arduino-esp32's `CameraWebServer/camera_pins.h`,
where they are spelled `CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3` /
`_Romeo_ESP32S3`.

Both maps live in [include/camera_pins.h](include/camera_pins.h). To add a
board: add an `#elif` block there, an `[env:...]` in `platformio.ini` with a
matching `-DCAMERA_MODEL_*`, and — if its flash size or PSRAM mode differs — a
`sdkconfig.defaults.<target>` file.

---

## 🩺 Troubleshooting

| Symptom | Cause / fix |
|---|---|
| No serial output — XIAO | It has no UART bridge. `sdkconfig.defaults.esp32s3` sets `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` so IDF logs and `Serial` share the USB port. If the port vanishes after flashing, double-tap reset to enter the bootloader. |
| No serial output — ESP32-CAM | Console is UART0 at 115200 through the programmer. Check U0T→RX / U0R→TX are not swapped, and that IO0 is **disconnected** from GND (strapped low it stays in the bootloader). |
| ESP32-CAM won't enter bootloader | IO0 must be tied to GND *before* reset, and the 5V pin needs a supply that can hold ~500 mA. Many programmers cannot, which shows up as `Brownout detector was triggered`. |
| `Camera init failed: 0x105` | PSRAM not up, or the camera is not seated. Check the boot log line `PSRAM: yes (…)`. On ESP32-CAM confirm the board actually has PSRAM — the cheapest clones ship without it. |
| `Anedya broker connect failed, rc=-2` | TLS/DNS. Check WiFi, and check the NTP sync line — an unset clock makes the broker certificate look expired. |
| Offer arrives but nothing happens | Raise the log level: `esp_log_level_set("anedya_signaling", ESP_LOG_DEBUG);` in `setup()`. |
| `Offer too large` | The compressed offer exceeded `OFFER_DEFLATE_MAX_BYTES`. The viewer already refuses offers over ~950 base64 bytes. |
| Stream connects then dies after ~90 s | The relay-path tuning in `app_config.h` (`WEBRTC_AGENT_RECV_TIMEOUT_MS`, `WEBRTC_SEND_CACHE_SIZE`) was lowered. See the comments there — the defaults exist for the TURN relay's 200–1700 ms RTT. |
| `JPEG send would block` warnings | Normal back pressure when the link cannot keep up. Lower `CAMERA_STREAM_FPS` or raise `CAMERA_STREAM_JPEG_QUALITY`. |

---

## 📚 References

**Anedya**
- [Anedya Overview](https://docs.anedya.io/anedya-overview/)
- [Anedya MQTT Endpoints](https://docs.anedya.io/device/mqtt-endpoints/)
- [Anedya Commands](https://docs.anedya.io/features/commands/commands-intro/)
- [Update status of a command](https://docs.anedya.io/device/api/commands-update-status/)
- [Anedya ESP32 Arduino examples](https://github.com/anedyaio/anedya-example-esp32)

**Arduino / ESP32 / WebRTC**
- [pioarduino platform-espressif32](https://github.com/pioarduino/platform-espressif32)
- [Arduino as an ESP-IDF component](https://docs.espressif.com/projects/arduino-esp32/en/latest/esp-idf_component.html)
- [espressif/esp_peer](https://components.espressif.com/components/espressif/esp_peer)
- [espressif/esp32-camera](https://components.espressif.com/components/espressif/esp32-camera)
- [WebRTC Overview](https://webrtc.org/getting-started/overview)

**Other examples**
- [ESP-IDF version of this example](../anedya-camera-livestream-example-esp-cam)
- [Anedya Camera Livestream with Raspberry Pi](https://github.com/anedyaio/anedya-camera-livestream-example)

---

## License

Apache License 2.0, matching the ESP-IDF example this is ported from.
