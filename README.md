# Press F Ultra

**Press F Ultra** is a Fairchild Channel F emulator for Nintendo 64, utilizing the **[libpressf](https://github.com/celerizer/libpressf)** emulation library.

![images](https://github.com/user-attachments/assets/83d289c0-f379-4551-8610-747b1f4f0672)

## Usage

### On a Nintendo 64 Flashcart

1. *Download the Program*
   - Download the [latest release](https://github.com/celerizer/Press-F-Ultra/releases/).  

2. *Prepare the SD Card*
   - Create a directory named `press-f` on the root of the SD Card.  
   - Place the Channel F BIOS files in this directory with the exact filenames:  
     - `sl31253.bin`  
     - `sl31254.bin`  
   - Add any additional cartridge ROM files to the same directory.  

3. *Load the Program*
   - Copy `Press-F.z64` to the SD Card.  
   - Boot the file on the flashcart and select a cartridge ROM to load.

4. *Setup as an [N64FlashCartMenu](https://github.com/Polprzewodnikowy/N64FlashcartMenu) plugin* (optional)  
   - Place `Press-F.z64` in the `menu/emulators` directory.  
   - Load Channel F ROMs with the `.chf` extension directly through N64FlashCartMenu.  

### On the Ares Emulator

- Add `sl31253.bin`, `sl31254.bin`, and any additional cartridge ROMs to the `roms` directory.  
- Compile `Press-F.z64` following the provided build instructions.  
- Open `Press-F.z64` in the Ares emulator.

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

The L Trigger and R Trigger can be used to open a ROM menu and settings menu respectively.

## Building
Open the devcontainer (rebuild required if you want to update libdragon, as it is not a submodule), or:
- Set up a [libdragon environment](https://github.com/DragonMinded/libdragon/wiki/Installing-libdragon) on the preview branch.
- Clone the project and the core emulation submodule:
```sh
git clone https://github.com/celerizer/Press-F-Ultra.git --recurse-submodules
```
- Run `make`.

## License

- **Press F Ultra** and **libpressf** are distributed under the MIT license. See LICENSE for information.
- The font **Tuffy Bold** is used under the public domain.
