# `rGuiStyler`

A simple and easy-to-use [raygui](https://github.com/raysan5/raygui) styles editor.

Useful for tools style customization. The best tool companion for [rGuiLayout](https://raylibtech.itch.io/rguilayout) and [rGuiIcons](https://raylibtech.itch.io/rguiicons).

`rGuiStyler` can be used for free as a [WebAssembly online tool](https://raylibtech.itch.io/rguistyler) and it can also be downloaded as a **standalone tool** for `Windows`, `Linux` and `macOS` with some extra features.

**NOTE: Latest `rGuiStyler 6.0` release is intended to be used with [`raygui 4.5+`](https://github.com/raysan5/raygui) release.**

## Features

 - **Global and control specific styles edition**
 - **Style preview** in real time with individual test controls
 - **Style templates** available to start customizing new styles
 - Selectable controls state: **NORMAL, FOCUSED, PRESSED, DISABLED**
 - Save and load as binary style file `.rgs` (with font embedded!)
 - Export style as an embeddable **code file** (`.h`) (with font embedded!)
 - Export style as a `.png` **controls table image** for showcase
 - Embed style as png image chunk: `rGSf` (rgs file data)
 - Import, configure and preview **style fonts** (`.ttf`/`.otf`)
 - Load custom font charset for the style (Unicode codepoints)
 - Color palette for quick color save/selection
 - **+14 custom style examples** included for reference
 - Command-line support for `.rgs`/`.h`/`.png` batch conversion
 - **Free and open source** 

## Screenshot

![rGuiStyler](screenshots/rguistyler_v600_shot01.png)
 
## Usage

The tool is quite intuitive, the expected steps to follow are: 
 1. Choose the control to edit from the Controls ListView (`DEFAULT` referes to global style for all controls)
 2. Choose the property to edit from Properties ListView
 3. Select a value for that property (color, number...) on Control Property Edit window
 4. Font can be loaded and atlas generated automatically
 
NOTE: Changes are previewed in real time in the same tool! 

Once the desired style has been created, press the `Export Style` button to save it as a `.rgs`**binary style file**. Style can also be exported as an embeddable `.h` **code file** or a `.png` **controls table image** (intended for preview and style showcase)

`rGuiStyler Standalone` comes with command-line support for batch conversion. For usage help:

 > rguistyler.exe --help

## License

`rGuiStyler` source code is distributed as **open source**, licensed under an unmodified [zlib/libpng license](LICENSE). 

`rGuiStyler` binaries are completely free for anyone willing to compile them directly from source.

`rGuiStyler Standalone` desktop tool is distributed as freeware. 

In any case, consider some donation to help the author keep working on software for games development.

*Copyright (c) 2017-2025 raylib technologies ([@raylibtech](https://twitter.com/raylibtech)) / Ramon Santamaria ([@raysan5](https://twitter.com/raysan5))*
