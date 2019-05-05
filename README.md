# Mattoni

Mattoni is an interactive fractal explorer.

![julia4](media/julia4.png)

### Setup

First, you must have [GCC](https://gcc.gnu.org) and [SDL2](https://www.libsdl.org/download-2.0.php) installed on your system. After you're set up, run `make` from the project's main folder to compile Mattoni. To remove the compiled executable and any generated object files, run `make clean`.

### Usage

Run `./main` from the main directory to start Mattoni. In the window that opens, you can:

+ Click and drag a box to zoom in on an area
+ Pan using `H/J/K/L` or the arrow keys
+ Zoom in with either `U` or `Return`
+ Zoom out with either `N` or `Spacebar`
+ Save a screenshot to `out/screenshot.bmp` by pressing `S`. Note that only one screenshot is ever saved at a time, so make sure you do whatever you want with your old screenshot before overwriting. Running `make clean` also deletes screenshots.

![julia1](media/julia1.png)

### Authors

Crafted with care by Marc-André Brochu and Marcel Goh, exchange students at Charles University, for HackPrague 2019.  

Named after the best drink ever.

![julia2](media/julia2.png)
