# vkwaifu

![alt text](https://i.imgur.com/NtYtvkU.png)

<sub>Art by [XANAX025](https://twitter.com/XANAX025)</sub>

## Overview
Displays your waifu with the power of Vulkan and post-processing effects!

### Dependencies
- [volk](https://github.com/zeux/volk) Vulkan Loader
- [glfw](https://github.com/glfw/glfw) Window Handling
- [meson](https://mesonbuild.com/)

### Compilation
After cloning ``vkwaifu``, run this inside the ``vkwaifu`` directory:
```
meson build
meson compile -C build
```

### Project Structure
```
├── dep: dependencies
│   ├── inc: include dependencies
│   └── lib: library dependencies
├── src: Project Source Code
└── inc: Project Headers
```

### License
[Simplified BSD License](LICENSE)
