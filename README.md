# nexplit-keypad

ZMK firmware configuration for the Nexplit Keypad, a standalone 19-key numpad built around a single nice!nano v2. It's designed as a **companion device for [KSN-2](https://github.com/Kesaros44/ksn2-firmware)** — KSN-2 is a 70% split without an integrated numpad, so this keypad sits next to it and fills that gap. It isn't tied to KSN-2 in firmware (there's no split link between them; it just shows up as its own separate Bluetooth device), so it also works fine as a standalone numpad with any host.

* Keyboard Maintainer: [AJG](https://github.com/Kesaros44)
* Hardware Supported: Nexplit Keypad, nice!nano v2 (nRF52840), BLE

## Hardware

- **MCU:** nice!nano v2 (nRF52840), wireless (BLE), **not split** — one board, one firmware image
- **Matrix:** 6 rows x 4 columns, 19 physical keys (`diode-direction = "col2row"`, `wakeup-source` set on the matrix so the board can wake from sleep on a keypress)
- **LED (status):** one indicator LED, driven by a custom driver (see below)
- **Backlight:** PWM backlight, off by default at boot (`CONFIG_ZMK_BACKLIGHT` on, `BL_TOG`/`BL_CYCLE` bound on the settings layer)
- **Bluetooth:** `CONFIG_BT_MAX_CONN` / `CONFIG_BT_MAX_PAIRED` both raised to 5 — more paired-host slots than the KSN boards ship with by default, useful since a numpad is the kind of accessory that gets paired to several machines
- **Battery:** reporting enabled

### Pinout

| Row | GPIO |
| --- | --- |
| row1 | P1.00 |
| row2 | P0.24 |
| row3 | P0.22 |
| row4 | P0.20 |
| row5 | P0.17 |
| row6 | P0.08 |

| Column | GPIO |
| --- | --- |
| col1 | P0.31 |
| col2 | P0.02 |
| col3 | P1.15 |
| col4 | P1.13 |

| Function | GPIO |
| --- | --- |
| Indicator LED | P0.29 (active-low) |
| Backlight PWM | P0.06 |

### LED driver

`config/src/keypad_indicator.c` drives the status LED directly: solid on while connected to the host, blinking every 500ms while not. It replaced the `zmk-poor-mans-led-indicator` module's BLE widget (still imported in `config/west.yml` for other pieces, but its LED widget is explicitly disabled with `CONFIG_INDICATOR_LED_WIDGET=n` in `keypad.conf`) for the same reason as KSN-2's `ksn2_conn_status.c`: that widget only re-evaluates on a Bluetooth profile-switch event, so the LED could get stuck showing "connected" after an actual disconnect that wasn't triggered by a profile switch. This driver instead re-checks on every profile-changed event *and* polls once a second as a backstop, so it can't get stuck either way.

It's guarded with `DT_NODE_EXISTS(DT_NODELABEL(indicator_led))`, so the same source file compiles cleanly (as a no-op) for the `settings_reset` build target too, which shares this repo's `build.yaml` but doesn't apply `keypad.overlay` and therefore has no `indicator_led` node.

Wired into the build the same way as the KSN boards: `config/zephyr/module.yml` registers `config/` as a Zephyr module, `config/CMakeLists.txt` adds `src/keypad_indicator.c` to the app sources.

### Keymap (`config/boards/shields/keypad/keypad.keymap`)

Two layers:

- **`default_layer`** — standard numpad layout (7/8/9, 4/5/6, 1/2/3, 0/., plus, minus, multiply, slash, enter, backspace, delete). The top-left key is `&lt 1 KP_NUM` — tap for Num Lock, hold to reach `settings_layer`.
- **`settings_layer`** (held via the top-left key) — Bluetooth profile select (`BT_SEL 0`–`4`), `BT_CLR`, and backlight toggle/cycle on the top two keys.

## Building

CI (`.github/workflows/build.yml`) uses ZMK's standard reusable workflow (`zmkfirmware/zmk/.github/workflows/build-user-config.yml@main`). `build.yaml` defines two targets — there's no left/right split here, just one board plus the bond-reset image:

```yaml
include:
  - board: nice_nano//zmk
    shield: keypad
  - board: nice_nano//zmk
    shield: settings_reset
    artifact-name: settings_reset
```

Pushing to this repo builds `keypad` and `settings_reset` as GitHub Actions artifacts — grab the `.uf2` files from the workflow run.

To build locally with `west`:

```sh
west init -l config
west update
west build -p -b nice_nano_v2 -- -DSHIELD=keypad -DZMK_EXTRA_MODULES=$(pwd)/config
```

## Flashing

Double-tap reset on the nice!nano to enter its UF2 bootloader, then drag `keypad-nice_nano-zmk.uf2` onto the mounted `NICENANO` drive.

## Re-pairing / clearing Bluetooth bonds

Flash the `settings_reset` artifact to wipe stored BLE bonds, then reflash the normal `keypad` firmware and re-pair.

## Notes

- The custom LED driver went through two real fixes during development (see git history): it wasn't being compiled into the build at first (missing from `CMakeLists.txt`), and once it was, it needed a guard so it wouldn't break the `settings_reset` build, which shares this repo's `build.yaml` but has no `indicator_led` node to attach to. Both are fixed as of the current source — this note is here so a future edit to `keypad.overlay` or `CMakeLists.txt` doesn't reintroduce either problem.
- `reference/` in this repo holds the original schematic and keymap reference images this firmware was built from.

## Schematic source

`config/boards/shields/keypad/` was derived from the schematics in `reference/` (회로도1–3, 키맵). If the physical board is ever revised, the GPIO pin comments above are the first place to check against updated net labels.

---

# nexplit-keypad (한국어)

nice!nano v2 한 대로 만든 독립형 19키 넘패드, Nexplit Keypad용 ZMK 펌웨어 설정입니다. **[KSN-2](https://github.com/Kesaros44/ksn2-firmware)와 함께 사용하도록 설계된 보조 기기**입니다 — KSN-2는 넘패드가 내장되지 않은 70% 스플릿 보드라서, 이 키패드가 옆에 놓여 그 빈자리를 채워줍니다. 펌웨어상으로 KSN-2와 묶여 있는 건 아니고(둘 사이에 스플릿 링크는 없으며, 그냥 별도의 블루투스 기기로 인식됩니다), 그래서 어떤 호스트에서든 독립형 넘패드로도 문제없이 사용할 수 있습니다.

* 키보드 관리자: [AJG](https://github.com/Kesaros44)
* 지원 하드웨어: Nexplit Keypad, nice!nano v2 (nRF52840), BLE

## 하드웨어

- **MCU:** nice!nano v2 (nRF52840), 무선(BLE), **스플릿 아님** — 보드 1개, 펌웨어 이미지 1개
- **매트릭스:** 6행 x 4열, 물리 키 19개(`diode-direction = "col2row"`, 키 입력으로 슬립에서 깨어나도록 매트릭스에 `wakeup-source` 설정됨)
- **LED (상태):** 인디케이터 LED 1개, 커스텀 드라이버로 구동(아래 참고)
- **백라이트:** PWM 백라이트, 부팅 시 기본 꺼짐(`CONFIG_ZMK_BACKLIGHT` 켜짐, 설정 레이어에 `BL_TOG`/`BL_CYCLE` 배치)
- **블루투스:** `CONFIG_BT_MAX_CONN` / `CONFIG_BT_MAX_PAIRED` 모두 5로 상향 — KSN 보드들의 기본값보다 페어링 가능한 호스트 슬롯이 많음. 넘패드는 여러 기기에 페어링해두고 쓰는 액세서리라는 특성상 이렇게 설정함
- **배터리:** 보고 활성화

### 핀아웃

| Row | GPIO |
| --- | --- |
| row1 | P1.00 |
| row2 | P0.24 |
| row3 | P0.22 |
| row4 | P0.20 |
| row5 | P0.17 |
| row6 | P0.08 |

| Column | GPIO |
| --- | --- |
| col1 | P0.31 |
| col2 | P0.02 |
| col3 | P1.15 |
| col4 | P1.13 |

| 기능 | GPIO |
| --- | --- |
| 인디케이터 LED | P0.29 (active-low) |
| 백라이트 PWM | P0.06 |

### LED 드라이버

`config/src/keypad_indicator.c`가 상태 LED를 직접 구동합니다: 호스트에 연결되어 있으면 상시 점등, 아니면 500ms마다 점멸. 이것은 `zmk-poor-mans-led-indicator` 모듈의 BLE 위젯(다른 용도로는 여전히 `config/west.yml`에 임포트되어 있지만, `keypad.conf`에서 `CONFIG_INDICATOR_LED_WIDGET=n`으로 그 LED 위젯만 명시적으로 비활성화됨)을 대체한 것인데, 이유는 KSN-2의 `ksn2_conn_status.c`와 동일합니다: 그 위젯은 블루투스 프로필 전환 이벤트에서만 재평가되기 때문에, 프로필 전환이 아닌 방식으로 실제 연결이 끊겼을 때 LED가 "연결됨" 상태로 고정될 수 있었습니다. 이 드라이버는 프로필 변경 이벤트마다 즉시 재확인하는 것에 더해 1초마다 폴링하는 안전망까지 두어서, 어느 경우로도 고정되지 않게 했습니다.

`DT_NODE_EXISTS(DT_NODELABEL(indicator_led))`로 가드되어 있어서, 이 소스 파일은 이 저장소의 `build.yaml`을 공유하지만 `keypad.overlay`가 적용되지 않아 `indicator_led` 노드가 없는 `settings_reset` 빌드 타겟에서도 (아무 동작 안 하며) 깔끔하게 컴파일됩니다.

빌드에 연결되는 방식은 KSN 보드들과 동일합니다: `config/zephyr/module.yml`이 `config/`를 Zephyr 모듈로 등록하고, `config/CMakeLists.txt`가 `src/keypad_indicator.c`를 앱 소스에 추가합니다.

### 키맵 (`config/boards/shields/keypad/keypad.keymap`)

레이어 2개:

- **`default_layer`** — 표준 넘패드 배열(7/8/9, 4/5/6, 1/2/3, 0/., 더하기, 빼기, 곱하기, 나누기, 엔터, 백스페이스, 삭제). 왼쪽 위 키는 `&lt 1 KP_NUM` — 탭하면 Num Lock, 누르고 있으면 `settings_layer`로 진입.
- **`settings_layer`** (왼쪽 위 키를 눌러서 진입) — 블루투스 프로필 선택(`BT_SEL 0`–`4`), `BT_CLR`, 그리고 위쪽 두 키에 백라이트 토글/사이클.

## 빌드

CI(`.github/workflows/build.yml`)는 ZMK 표준 재사용 워크플로우(`zmkfirmware/zmk/.github/workflows/build-user-config.yml@main`)를 사용합니다. `build.yaml`은 타겟 2개를 정의합니다 — 여기는 좌/우 스플릿이 없어서 보드 1개와 본딩 초기화용 이미지뿐입니다:

```yaml
include:
  - board: nice_nano//zmk
    shield: keypad
  - board: nice_nano//zmk
    shield: settings_reset
    artifact-name: settings_reset
```

이 저장소에 push하면 `keypad`, `settings_reset` 펌웨어가 GitHub Actions artifact로 빌드됩니다.

`west`로 로컬 빌드하려면:

```sh
west init -l config
west update
west build -p -b nice_nano_v2 -- -DSHIELD=keypad -DZMK_EXTRA_MODULES=$(pwd)/config
```

## 플래싱

nice!nano의 리셋 버튼을 더블탭해서 UF2 부트로더로 진입한 뒤, 마운트된 `NICENANO` 드라이브에 `keypad-nice_nano-zmk.uf2`를 드래그하면 됩니다.

## 재페어링 / 블루투스 본딩 초기화

`settings_reset` artifact를 플래시하면 저장된 BLE 본딩이 초기화됩니다. 그 다음 정상 `keypad` 펌웨어를 다시 플래시하고 재페어링하세요.

## 참고

- 커스텀 LED 드라이버는 개발 과정에서 실제로 두 번 수정됐습니다(git 히스토리 참고): 처음엔 빌드에 아예 컴파일되지 않았고(`CMakeLists.txt`에 빠져 있었음), 그걸 고친 뒤에는 `indicator_led` 노드가 없는 `settings_reset` 빌드가 깨지지 않도록 가드가 필요했습니다. 현재 소스 기준으로 둘 다 고쳐져 있습니다 — 나중에 `keypad.overlay`나 `CMakeLists.txt`를 수정할 때 두 문제 중 하나가 다시 생기지 않도록 이 메모를 남겨둡니다.
- 이 저장소의 `reference/`에는 이 펌웨어의 기반이 된 원본 회로도와 키맵 참고 이미지가 들어 있습니다.

## 회로도 출처

`config/boards/shields/keypad/`는 `reference/`에 있는 회로도(회로도1–3, 키맵)에서 도출했습니다. 실물 보드가 개정되면, 위 GPIO 핀 정보를 갱신된 넷 레이블과 가장 먼저 대조해봐야 합니다.
