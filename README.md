# C Asteroids
It is a basic clone of the Atari's game Asteroids made in C99 for Windows only using nothing more than [Graphic Device Interface](https://en.wikipedia.org/wiki/Graphics_Device_Interface) for rendering, compatible with *MinGW* and *MSVC*.

## How to build
MinGW:
	- gcc Asteroids.c -o Asteroids.exe -l gdi32

MSVC:
	- cl Asteroids.c /o Asteroids.exe gdi32.lib user32.lib

