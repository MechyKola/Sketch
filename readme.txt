This readme was created with a 100 word limit:

Converter.c uses lines going {top to bottom, one to the right,
back to the top, one to the right} repeatedly until the file is converted,
using DY to draw lines without using absolute coordinates (run length encoding).
Not all functions allow custom height and width, so only 
200x200 files with 1 byte greyscale values are fully supported.
To pass tests separating whitespace must be 1 character in pgm files.
If fractal.pgm and bands.pgm are present, tests will convert pgm to sk,
back to pgm and back to sk, checking file sizes match.
Automatically detects pgm and sk file extensions.
