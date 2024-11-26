# Press F Ultra

**Press F Ultra** is a Fairchild Channel F emulator for Nintendo 64, utilizing the **[libpressf](https://github.com/celerizer/libpressf)** emulation library.

![image](https://github.com/celerizer/Press-F-Ultra/assets/33245078/146d3645-fd53-48a7-9996-04c933c66a20)

## Usage

### Nintendo 64 Flashcart

- Download the [latest release](https://github.com/celerizer/Press-F-Ultra/releases/).
- Place the Channel F BIOS images and any additional cartridge ROMs in a "press-f" directory on the root of the SD Card. Make sure the two BIOS images have these exact filenames:
  - `sl31253.bin`
  - `sl31254.bin` 
- Boot `Press-F.z64` and choose a game to play.

### Emulator

- Add the files `sl31253.bin` and `sl31254.bin` to the `roms` directory, as well as any additional cartridge ROM files.
- Build `Press-F.z64` per the instructions below.
- Load in the Ares emulator.

## Controls

| | Nintendo 64 | Channel F |
|-|-|-|
| ![d](https://github.com/celerizer/Press-F-Ultra/assets/33245078/ce131dfb-45ac-42d0-8182-c9a89062795d) | Control Pad | Directional movement |
| ![cleft](https://github.com/celerizer/Press-F-Ultra/assets/33245078/ece48821-183b-439c-b8ee-479369a0b392) | C-Left | Rotate counter-clockwise |
| ![cright](https://github.com/celerizer/Press-F-Ultra/assets/33245078/13b81712-a6c8-4191-8f10-e62809a6f6bb) | C-Right | Rotate clockwise |
| ![cup](https://github.com/celerizer/Press-F-Ultra/assets/33245078/0a519cf0-652b-4133-9283-0a6abb6b623a) | C-Up | Pull up |
| ![cdown](https://github.com/celerizer/Press-F-Ultra/assets/33245078/a72d495b-f850-448b-9985-7a1687e2b4cc) | C-Down | Plunge down |
| ![a](https://github.com/celerizer/Press-F-Ultra/assets/33245078/f634cf7e-4705-42b0-a607-14b9e057ea8a) | A Button | 1 / TIME |
| ![b](https://github.com/celerizer/Press-F-Ultra/assets/33245078/4a1dcb97-483d-47db-9ac5-12171940dcd4) | B Button | 2 / MODE |
| ![z](https://github.com/celerizer/Press-F-Ultra/assets/33245078/8ac5415c-8cfd-4d43-9dd3-0c278163eafc) | Z Trigger | 3 / HOLD |
| ![s](https://github.com/celerizer/Press-F-Ultra/assets/33245078/bf7ad340-bcd0-44b0-a4a9-c0557e24e44b) | START Button | 4 / START |

## Building

- Set up a [libdragon environment](https://github.com/DragonMinded/libdragon/wiki/Installing-libdragon).
- Clone the project and the core emulation submodule:
```sh
git clone https://github.com/celerizer/Press-F-Ultra.git --recurse-submodules
```
- Run `make`.
