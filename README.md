# CLI Image Editor

A command-line application developed in **C** for processing and transforming images in the **NetPBM** format. This project implements image manipulation algorithms, including rotations, cropping, and convolution-based filtering, through a custom-built dynamic memory and I/O management system.


## Technical Implementation

### Data Architecture
The system uses a `image_data` structure to manage metadata and pixel information. The core data is stored in a triple-pointer structure (`int ***area`):
* `area[0][i][j]` stores pixels for grayscale images.
* `area[0..2][i][j]` stores individual R, G, and B channels for color images.

### Memory and I/O Management
* **Dynamic Allocation**: Custom utility `aloc_triple_ptr` manages the 3D matrix allocation, ensuring that the memory footprint is tailored to the image dimensions.
* **Defensive Programming**: Every memory allocation is verified, and a deep-clearing function `free_image` is utilized to prevent fragmentation and leaks during operations.
* **Hybrid Parsing**: The `LOAD` command handles both ASCII and Binary files by parsing headers with a custom whitespace-skipping logic and utilizing direct character reading for binary data streams.

### Processing Logic
* **Convolution Filters**: The `APPLY` command implements 3x3 convolution kernels. It performs matrix multiplication across RGB channels, utilizes a `clamp` function to maintain pixel values within the [0, 255] range.
* **Rotation Engine**: Supports ±90, ±180, ±270 and ±360 degree rotations. The system dynamically reallocates memory and swaps height/width metadata for non-square rotations to maintain aspect ratio integrity.
* **Histogram & Equalization**: Implements frequency-based analysis for grayscale images, allowing for automatic contrast adjustment and visual distribution reporting.

## Command Overview

| Command | Description |
| :--- | :--- |
| **LOAD <file>** | Loads a NetPBM file into memory and resets the selection. |
| **SELECT \<x1> \<y1> \<x2> \<y2>** | Selects a specific rectangular area for processing. |
| **SELECT ALL** | Selects the entire image dimensions. |
| **HISTOGRAM \<x> \<y>** | Displays a histogram with <x> stars and <y> bins (Grayscale only). |
| **EQUALIZE** | Performs histogram equalization to improve contrast (Grayscale only). |
| **ROTATE \<angle>** | Rotates the selection or image. (accepted: ±90, ±180, ±270, ±360) |
| **CROP** | Resizes the image to the current selection. |
| **APPLY \<FILTER>** | Applies filters (EDGE, SHARPEN, BLUR, GAUSSIAN_BLUR) to color images. |
| **SAVE \<file> [ascii]** | Saves the image. Binary by default; ASCII if specified. |
| **EXIT** | Frees all resources and terminates the program. |

## Build and Execution

Either run `make` or

```bash
gcc -Wall -Wextra main.c -lm -o image_editor
```

To run,
```bash
./image_editor
```