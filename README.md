# spock
A Vulkan rendering engine written in C++

## Dependencies
You will need a copy of the Vulkan SDK in order to build, which can (hopefully)
be obtained from your distribution's package manager.  Alternatively, you can
download and install it from the [LunarG](https://vulkan.lunarg.com) website,
in which case you will need to add the following line to your
`.{bash,zsh,...}rc`:
```
source /path/to/sdk/setup-env.sh
```

If building for a platform that supports a windowing system, you will also need
[GLFW](https://www.glfw.org/) to be somewhere on your system.

## Installation
```
git clone https://github.com/wiggywombathead/spock.git
```

## Building
Makefile options:
* `BUILD`
  * `debug` : enables validation layers and various logging output
  * else : no validation layers
* `WS`
  * `null` : builds for NullWS
  * else : uses GLFW to handle all windowing
