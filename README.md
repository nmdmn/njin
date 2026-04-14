# Njin

<!--toc:start-->
- [Njin](#njin)
  - [Installation](#installation)
    - [macOS](#macos)
      - [Environment setup](#environment-setup)
      - [Troubleshooting](#troubleshooting)
      - [Generate cmake and build (the usual)](#generate-cmake-and-build-the-usual)
      - [Sym. link the compile-commands](#sym-link-the-compile-commands)
      - [Launch with debug layer](#launch-with-debug-layer)
<!--toc:end-->

Quick platform agnostic vulkan renderer using GLFW3, GLM, VMA.

## Installation

Not sure what is needed on the other platforms yet, what is specific for
Linux or Windows

### macOS

```sh
brew install molten-vk
brew install vulkan-tools
brew install vulkan-validationlayers
```

#### Environment setup

Do NOT add this to ".(z)profile"! Add to ".zshrc, .bashrc" so neovim's
embedded terminal will see these. ".(z)profile" meant for login shells
(opening a fresh terminal app, SSH in, etc)

```sh
export VK_LAYER_PATH=/opt/homebrew/opt/vulkan-validationlayers/share/vulkan/explicit_layer.d
export VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json
export DYLD_LIBRARY_PATH=/opt/homebrew/lib:$DYLD_LIBRARY_PATH
```

This will needed for DAP as well, example:
".vscode/launch.json"

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "njin",
      "type": "codelldb",
      "request": "launch",
      "program": "${workspaceFolder}/build/njin",
      "args": [],
      "cwd": "${workspaceFolder}/build",
      "env": {
        "VK_LAYER_PATH": "/opt/homebrew/opt/vulkan-validationlayers/share/vulkan/explicit_layer.d",
        "VK_ICD_FILENAMES": "/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json",
        "DYLD_LIBRARY_PATH": "/opt/homebrew/lib:$DYLD_LIBRARY_PATH"
      }
    }
  ]
}
```

#### Troubleshooting

```sh
# Check if MoltenVK is installed
find /usr/local /opt/homebrew -name "libMoltenVK*" 2>/dev/null

# Check if vulkan loader is installed
find /usr/local /opt/homebrew -name "libvulkan*" 2>/dev/null

# Check if the ICD manifest exists
find /opt/homebrew -name "MoltenVK_icd.json" 2>/dev/null

# Locate validation layer lib
find /opt/homebrew -name "libVkLayer_khronos_validation.dylib" 2>/dev/null
```

#### Generate cmake and build (the usual)

```sh
mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j8
```

Run tests!

```sh
ctest --output-on-failure
```

#### Sym. link the compile-commands

```sh
ln -s build/compile_commands.json compile_commands.json
```

#### Launch with debug layer

```sh
VK_LOADER_DEBUG=all ./njin 2>&1 | head -50 
```
