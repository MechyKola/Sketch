Converter.c uses lines going {top to bottom, one along to the right,
then back to the top, one to the right} repeatedly until the file is converted.
This allows using the DY function to draw lines without using data
to represent absolute coordinates etc.
Some functions allow custom height and width but not all,
only 200x200 files with 1 byte greyscale values are fully supported.
To pass tests separating whitespace must be 1 character in pgm files.
If fractal.pgm and bands.pgm are present,
tests will try to convert to sk, back to pgm and back to sk.
