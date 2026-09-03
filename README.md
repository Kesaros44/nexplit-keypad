# nexplit-keypad

ZMK firmware for the Nexplit Keypad, a standalone 19-key numpad built around a single nice!nano v2. Designed as a companion for [KSN-2](https://github.com/Kesaros44/ksn2-firmware) (which has no integrated numpad), but works as a standalone numpad with any host — there's no split link between them.

* Keyboard Maintainer: [AJG](https://github.com/Kesaros44)
* Hardware Supported: Nexplit Keypad, nice!nano v2 (nRF52840), BLE

## Hardware

- **MCU:** nice!nano v2 (nRF52840), wireless (BLE), not split — one board, one firmware image
- **Matrix:** 6 rows × 4 columns, 19 physical keys, `col2row`
- **LED (status):** one indicator LED, driven by a custom driver
- **Backlight:** designed into the schematic (PWM), but parts aren't populated and the feature is disabled in firmware — no backlight on the physical board
- **Bluetooth:** up to 5 paired hosts (`CONFIG_BT_MAX_CONN`/`CONFIG_BT_MAX_PAIRED` raised from the KSN boards' default)
- **Battery:** reporting enabled

## Keymap (`config/boards/shields/keypad/keypad.keymap`)

Two layers:

- **`default_layer`** — standard numpad layout (7/8/9, 4/5/6, 1/2/3, 0/., plus, minus, multiply, slash, enter). Top-left key (`&lt 1 BSPC`) taps Backspace, hold to reach `settings_layer`. The Delete-position key instead runs a macro (Win+R → `calc` → Enter) to launch the calculator. The Num Lock key is a plain tap (`&kp KP_NUM`).
- **`settings_layer`** (held via the top-left key) — Bluetooth profile select (`BT_SEL 0`–`4`), `BT_CLR`, and backlight toggle/cycle (no-op — no backlight hardware).

## Building

GitHub Actions builds on every push — grab the `.uf2` files (`keypad`, `settings_reset`) from the workflow run's artifacts.

Local build with `west`:

```sh
west init -l config
west update
west build -p -b nice_nano_v2 -- -DSHIELD=keypad -DZMK_EXTRA_MODULES=$(pwd)/config
```

## Flashing

Double-tap reset on the nice!nano to enter the UF2 bootloader, then drag `keypad-nice_nano-zmk.uf2` onto the `NICENANO` drive.

## Re-pairing / clearing Bluetooth bonds

Flash the `settings_reset` artifact to wipe stored BLE bonds, then reflash the normal `keypad` firmware and re-pair.

---

# nexplit-keypad (한국어)

nice!nano v2 한 대로 만든 독립형 19키 넘패드, Nexplit Keypad용 ZMK 펌웨어 설정입니다. [KSN-2](https://github.com/Kesaros44/ksn2-firmware)(넘패드가 내장되지 않은 보드)의 보조 기기로 설계되었지만, 둘 사이에 스플릿 링크는 없어서 어떤 호스트에서든 독립형 넘패드로 사용할 수 있습니다.

* 키보드 관리자: [AJG](https://github.com/Kesaros44)
* 지원 하드웨어: Nexplit Keypad, nice!nano v2 (nRF52840), BLE

## 하드웨어

- **MCU:** nice!nano v2 (nRF52840), 무선(BLE), 스플릿 아님 — 보드 1개, 펌웨어 이미지 1개
- **매트릭스:** 6행 × 4열, 물리 키 19개, `col2row`
- **LED (상태):** 인디케이터 LED 1개, 커스텀 드라이버로 구동
- **백라이트:** 회로도에는 설계되어 있지만(PWM) 부품이 실장되지 않았고 펌웨어 기능도 꺼져 있음 — 실제 보드에는 백라이트 없음
- **블루투스:** 최대 5개 호스트 페어링(`CONFIG_BT_MAX_CONN`/`CONFIG_BT_MAX_PAIRED`를 KSN 보드 기본값보다 상향)
- **배터리:** 보고 활성화

## 키맵 (`config/boards/shields/keypad/keypad.keymap`)

레이어 2개:

- **`default_layer`** — 표준 넘패드 배열(7/8/9, 4/5/6, 1/2/3, 0/., 더하기, 빼기, 곱하기, 나누기, 엔터). 왼쪽 위 키(`&lt 1 BSPC`)는 탭하면 백스페이스, 누르고 있으면 `settings_layer`로 진입. Delete 위치의 키는 대신 매크로(Win+R → `calc` → Enter)로 계산기를 실행. Num Lock 키는 순수 탭키(`&kp KP_NUM`).
- **`settings_layer`** (왼쪽 위 키를 눌러서 진입) — 블루투스 프로필 선택(`BT_SEL 0`–`4`), `BT_CLR`, 백라이트 토글/사이클(백라이트 하드웨어 자체가 없어서 실제 동작은 없음).

## 빌드

GitHub Actions가 push마다 자동으로 빌드합니다 — 워크플로우 실행의 아티팩트에서 `.uf2` 파일(`keypad`, `settings_reset`)을 받으면 됩니다.

`west`로 로컬 빌드:

```sh
west init -l config
west update
west build -p -b nice_nano_v2 -- -DSHIELD=keypad -DZMK_EXTRA_MODULES=$(pwd)/config
```

## 플래싱

nice!nano의 리셋 버튼을 더블탭해서 UF2 부트로더로 진입한 뒤, 마운트된 `NICENANO` 드라이브에 `keypad-nice_nano-zmk.uf2`를 드래그하면 됩니다.

## 재페어링 / 블루투스 본딩 초기화

`settings_reset` artifact를 플래시하면 BLE 본딩이 초기화됩니다. 그 다음 정상 `keypad` 펌웨어를 다시 플래시하고 재페어링하세요.
