# C Asteroids
It is a basic single file clone of the Atari's game Asteroids made with C99 for Windows using nothing but [GDI](https://en.wikipedia.org/wiki/Graphics_Device_Interface) for rendering, compatible with *MinGW* and *MSVC*.

## How to build
MinGW:
	- gcc Asteroids.c -o Asteroids.exe -l gdi32

MSVC:
	- cl Asteroids.c /o Asteroids.exe gdi32.lib user32.lib

