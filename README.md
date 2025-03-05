# Press F PSX

**Press F PSX** is a Fairchild Channel F emulator for Sony PlayStation, utilizing the **[libpressf](https://github.com/celerizer/libpressf)** emulation library.

## Building
- Set up a [PSYQo environment]([https://github.com/DragonMinded/libdragon/wiki/Installing-libdragon](https://github.com/grumpycoders/pcsx-redux/blob/main/src/mips/psyqo/GETTING_STARTED.md)).
- Clone the project and the required submodules:
```sh
git clone https://github.com/celerizer/Press-F-PSX.git --recurse-submodules
```
- Run `make`.
- Optionally, package the output ps-exe into a disc image using BUILDCD and PSXLICENSE, available [here](https://www.psxdev.net/downloads.html).

## License

- **Press F PSX**, **nugget**, and **libpressf** are distributed under the MIT license. See LICENSE for information.
