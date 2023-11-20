# Lilith Engine

Lilith Engine is a 3D game engine project made with the Vulkan API. The base code was inspired by courses from the [Vulkan Tutorial website](https://vulkan-tutorial.com/Introduction) and the [Tutorial video series](https://www.youtube.com/playlist?list=PL8327DO66nu9qYVKLDmdLW_84-yE4auCR) from Brenan Galea, and new features are progressively being implemented.

## Installation

The project was developed in Visual Studio 2022. I highly recommand using VS for simplicity and compatibility. You might use older versions of Visual Studio, though it is untested, there should be no compatibility issues.

This depository includes all external libraries used, except for Vulkan SDK. You will need to install it from the [Vulkan lunarg website](https://vulkan.lunarg.com/). Make sure that the correct path is set for VulkanSDK's include and library directories in the project's properties (you can use [this guide](https://vulkan-tutorial.com/Development_environment#page_Setting-up-Visual-Studio) if you are unfamiliar with Visual Studio configuration. Don't bother with the project creation).

## How to use

Open `LilithEngine.sln` in Visual Studio, build and run the solution. You can use the keyboard to move and rotate the camera.

For now, the code is made so that you can modify the scene in the `app.cpp` file (in the `loadTextures()` and `loadGameObjects()` methods). In the future, this should be eased by using a user interface with ImGUI.

You can enable validation layers (and best practices VL) in the `lth_compile_options.hpp` file, by commenting or uncommenting the corresponding `#define` macros.

If you want to load textures, they need to be put in the `textures/` folder, and should have one of the following formats: .png, .jpeg, .tga, .bmp, .psd, .gif (non animated), .hdr, .pic, .pnm. Check the `loadTextures()` method for code references.

Same thing for loading models: put them in the `models/` folder. For now, only the .obg format can be read. Check the `loadGameObjects()` method for reference. If your models appear strangely rotated, they might not use [Vulkan's coordinate system](https://anki3d.org/vulkan-coordinate-system/).

Shaders need to be put in the `shaders/` folder. You can use  VS is configured to auto-compile shaders when building, but you can also use directly `shadersCompile.bat` to compile all of the shaders at once. However, for now it is best to only modify pre-existing shaders, as the file's name are hard-coded in the render pipeline.