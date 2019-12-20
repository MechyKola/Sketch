#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>


// struct to draw over pgm matrix
struct pgmPen { int x; int y; unsigned char color; bool active; };
typedef struct pgmPen pgmPen;


// number functions
bool checkChar(char charToCheck) {
    int num = (int)(charToCheck - (int)'0');
    
    if(num < 0 || num > 9) {
        return false;
    }

    return true;
}

int charToDigit(char charDigit) {
    int num = (int)(charDigit - (int)'0');
    
    if(!checkChar(charDigit)) {
        fprintf(stderr, "%s", "Invalid character in digit conversion attempt\n");
        fprintf(stderr, "%c", charDigit);
        fprintf(stderr, "%s", "\n");
        exit(1);
    }

    return num;
}

int freeingConvertStrToInt(char *number) {
    long accumulator = 0;
    long multiplier = 1;
    for(int i = strlen(number) - 1; i > -1; i--) {
        if(checkChar(number[i])) {
            accumulator += multiplier * charToDigit(number[i]);
        }
        multiplier *= 10;
    }

    if(accumulator > INT_MAX || accumulator < INT_MIN) {
        fprintf(stderr, "%s", "Overflow detected in string to int conversion\n");
        exit(1);
    }

    free(number);
    return accumulator;
}


// string functions
char* substringSlice(char userInput[], int startIndex, int endIndex) {
    char *numberStore = malloc(endIndex - startIndex + 1);
    memcpy(numberStore, userInput + startIndex, endIndex - startIndex);
    *(numberStore + endIndex - startIndex) = '\0';
    return numberStore;
}


// pgm -> sketch functions

// helper function checking for all types of whitespace
bool isWhitespace(char character) {
    if(character == ' ' || 
    character == '\t' || 
    character == '\r' ||
    character == '\n' ||
    character == '\v' ||
    character == '\f') {
        return true;
    }
    return false;
}

// turns 1 byte greyscale into rgba
unsigned long greyscaleAsRGBA(unsigned long greyscale) {
    unsigned long idkWhyCWantsMeToDoThis = 255;
    unsigned long color = 255 + 
    ((greyscale << 8) & (255 << 8)) + 
    ((greyscale << 16) & (255 << 16)) + 
    ((greyscale << 24) & (idkWhyCWantsMeToDoThis << 24));
    return color;
}

// accepts 32 bit integer representing rgb and writes it to characters in string
void dataBitsToString(char *toWrite, unsigned long colorValue) {
    for(int i = 0; i < 6; i++) {
        toWrite[5 - i] = ((colorValue >> (6 * i)) & 63) + 128 + 64;
        // printf("%c", toWrite[5 - i]);
    }
}

void writeColor(FILE *fileToWriteTo, char *colorData) {
    for(int k = 0; k < 6; k++) {
        fputc(colorData[k], fileToWriteTo);
    }
    // telling sketch to process data as color
    fputc(128 + 3, fileToWriteTo);
}

// edits passed in values, returns pointer to char array of greyscale values
unsigned char* unpackPgm(int *height, int *width, int *colorSizeBack, char *filename) {
    // validate file
    if(!(access(filename, F_OK) != -1)) {
        fprintf(stderr, "File not found\n");
        exit(1);
    }

    FILE *pgmFile = fopen(filename, "rb");
    fseek(pgmFile, 0L, SEEK_END);
    if(ftell(pgmFile) < 3) {
        fprintf(stderr, "File too small - check if file is valid\n");
        exit(1);
    }
    rewind(pgmFile);

    // verify pgm file by checking first characters are 'P' and '5'
    if(fgetc(pgmFile) != 'P' || fgetc(pgmFile) != '5') {
        fprintf(stderr, "Non pgm file supplied");
        exit(1);
    }

    char currentChar = fgetc(pgmFile);

    // skip whitespace
    while(!feof(pgmFile) && isWhitespace(currentChar)) {
        currentChar = fgetc(pgmFile);
    }

    // get width but not use it
    int imageWidth = 200;
    while(!feof(pgmFile) && !isWhitespace(currentChar)) {
        // printf("%c\n", currentChar);
        currentChar = fgetc(pgmFile);
    }

    // skip whitespace
    while(!feof(pgmFile) && isWhitespace(currentChar)) {
        currentChar = fgetc(pgmFile);
    }

    // get height but not use it
    int imageHeight = 200;
    while(!feof(pgmFile) && !isWhitespace(currentChar)) {
        // printf("%c\n", currentChar);
        currentChar = fgetc(pgmFile);
    }

    // skip whitespace
    while(!feof(pgmFile) && isWhitespace(currentChar)) {
        currentChar = fgetc(pgmFile);
    }

    // get max value of grey and setting byte size for colours
    char maxValString[] = "xxxxx";
    int counter = 0;
    while(!feof(pgmFile) && !isWhitespace(currentChar)) {
        maxValString[counter] = currentChar;
        currentChar = fgetc(pgmFile);
    }

    int maxVal = freeingConvertStrToInt(substringSlice(maxValString, 0, counter));
    int colorSize = 1;
    if(maxVal > 255) {
        colorSize = 2;
    }

    unsigned char *imageMatrix = malloc(imageHeight * imageWidth * colorSize + 1);

    *width = imageWidth;
    *height = imageHeight;
    *colorSizeBack = colorSize;

    // actual image - turn rows into columns
    for(int i = 0; i < imageHeight; i++) {
        for(int j = 0; j < imageWidth; j++) {
            imageMatrix[j * imageHeight + i] = (unsigned char) fgetc(pgmFile);
        }
    }

    fclose(pgmFile);

    return imageMatrix;
}

// encode using rle, top to bottom then bottom to top
// incremnting DX by 1 each time
// alternating between going down and up with DY
// updating color each time it changes
void writeSkRLE(unsigned char *imageMatrix, int colorSize, int height, int width, char *filename) {
    unsigned long currentGreyValue = *imageMatrix;
    char* currentColorString = malloc(6); // 32 bits of data, 6 chars/bytes

    // rename file
    filename[strlen(filename) - 3] = 's';
    filename[strlen(filename) - 2] = 'k';
    filename[strlen(filename) - 1] = '\0';

    FILE *skFile = fopen(filename, "w+");

    // sets tool to line and initialise colour
    fputc(128 + 1, skFile);
    dataBitsToString(currentColorString, greyscaleAsRGBA(currentGreyValue));
    writeColor(skFile, currentColorString);

    bool down = true;
    int counter = 0;
    // x coordinate of the current coordinate
    for(int x = 0; x < width; x++) {
        // checks if the current reader direction is down or not
        // changes execution based on that
        if(x % 2 == 0) {
            down = true;
            for(int y = 0; y < height; y++) {
                if((currentGreyValue == imageMatrix[height * x + y]) &&
                counter < 31) {
                    counter++;
                } else {
                    fputc(64 + counter, skFile);

                    currentGreyValue = imageMatrix[height * x + y];
                    dataBitsToString(currentColorString, greyscaleAsRGBA(currentGreyValue));
                    writeColor(skFile, currentColorString);
                    counter = 1;
                }
            }
            // handle longer lines
            fputc(64 + counter, skFile);
        } else {
            down = false;
            for(int y = height - 1; y > -1; y--) {
                if((currentGreyValue == imageMatrix[height * x + y]) &&
                counter > -32) {
                    counter--;
                } else {
                    // convert into 6 bit two's complement
                    fputc(64 + 32 + 32 + counter, skFile);

                    currentGreyValue = imageMatrix[height * x + y];
                    dataBitsToString(currentColorString, greyscaleAsRGBA(currentGreyValue));
                    writeColor(skFile, currentColorString);
                    counter = -1;
                }
            }
            fputc(64 + 32 + 32 + counter, skFile);
        }
        counter = 0;
        // hardcoded DX1, changing from line to none and back
        if(x != width - 1) {
            fputc(128, skFile);
            fputc(1, skFile);
            if(down) {
                fputc(64 + 63, skFile);
            } else {
                fputc(64 + 1, skFile);
            }
            fputc(128 + 1, skFile);
        }
    }

    free(currentColorString);
    fclose(skFile);
    printf("File %s has been written.\n", filename);
}

void convertPgmToSk(char *filename) {
    int *height = malloc(sizeof(int));
    int *width = malloc(sizeof(int));
    int *colorSize = malloc(sizeof(int));
    unsigned char *imageMatrix = unpackPgm(height, width, colorSize, filename);
    if(*colorSize != 1) {
        fprintf(stderr, "This color size (%d) isn't implemented yet\n", *colorSize);
        exit(1);
    }

    // rle by making horizontal lines across entire image
    writeSkRLE(imageMatrix, *colorSize, *height, *width, filename);

    free(imageMatrix);
    free(height);
    free(width);
    free(colorSize);
}


// sk -> pgm

// binary to int helper function, for unisigned binary
int binaryToInt(unsigned char b, int bits) {
  int multiplier = 1;
  int accumulator = 0;

  for(int i = 0; i < bits; i++) {
    accumulator += ((b >> i) & 1) * multiplier;
    multiplier *= 2;
  }

  return accumulator;
}

// Extract an opcode from a byte (two most significant bits).
int getOpcode(unsigned char b) {
  int accumulator = 0;
  accumulator += (b >> 6) & 1;
  accumulator += ((b >> 7) & 1) * 2;
  return accumulator;
}

// Extract an operand (-32..31) from the rightmost 6 bits of a byte.
int getOperand(unsigned char b) {
  int value = binaryToInt(b, 5);
  if(((b >> 5) & 1) == 1) {
    value -= 32;
  }

  return value;
}

void applyLine(unsigned char *matrix, pgmPen *pen, int height, int width, int length) {
    if(pen->active) {
        if(length > 0) {
            for(int i = pen->y; i < (pen->y + length); i++) {
                matrix[(i * width) + pen->x] = pen->color;
            }
        } else {
            for(int i = pen->y; i > (pen->y + length); i--) {
                matrix[(i * width) + pen->x] = pen->color;
            }
        }
    }
    pen->y += length;
}

// all color encounters are 6 bits of data followed by a color command
unsigned char getColor(FILE *sketch, unsigned char byte) {
    unsigned long color = getOperand(byte);
    // first byte already processed by convert function
    for(int i = 0; i < 5; i++) {
        color = color << 6;
        color += binaryToInt(fgetc(sketch), 6);
    }
    // process color command
    fgetc(sketch);
    color = (color >> 16) & 255;
    // printf("%ld\n", color);
    return color;
}

void writePgm(int height, int width, unsigned char *matrix) {
    FILE *pgmFile = fopen("test.pgm", "w+");
    fprintf(pgmFile, "P5 %d %d %d\n", width, height, 255);
    for(int i = 0; i < 200 * 200; i++) {
        fputc(matrix[i], pgmFile);
    }

    fclose(pgmFile);
    printf("File pgm has been written.\n");
}

// mostly just creates matrix
void convertSkToPgm(char *filename) {
    FILE *sk = fopen(filename, "rb");
    unsigned char *matrix = malloc(200 * 200 + 1);
    pgmPen *pen = malloc(sizeof(pgmPen));
    pen->color = 0;
    pen->x = 0;
    pen->y = 0;
    pen->active = 0;
    int opCode = 0;
    int operand = 0;

    unsigned char currentByte = fgetc(sk);

    while(!feof(sk)) {
        opCode = getOpcode(currentByte);
        operand = getOperand(currentByte);
        switch(opCode) {
            // dx - increment column
            case 0:
                pen->x += operand;
                break;
            // dy - draw line
            case 1:
                applyLine(matrix, pen, 200, 200, operand);
                break;
            // tool - line or none
            case 2:
                if(operand == 0) {
                    pen->active = false;
                } else {
                    pen->active = true;
                }
                break;
            // data aka color
            case 3:
                pen->color = getColor(sk, currentByte);
                break;
        }
        currentByte = fgetc(sk);
    }

    writePgm(200, 200, matrix);
    free(matrix);
    free(pen);
}


// Testing

// A replacement for the library assert function.
void assert(int line, bool b) {
  if (b) return;
  printf("The test on line %d fails.\n", line);
  exit(1);
}

void testSubStringSlicing() {
    char *newString = substringSlice("abc", 1, 1);
    assert(__LINE__, strcmp(newString, "") == 0);
    free(newString);

    newString = substringSlice("abc", 0, 3);
    assert(__LINE__, strcmp(newString, "abc") == 0);
    free(newString);

    newString = substringSlice("abc", 0, 1);
    assert(__LINE__, strcmp(newString, "a") == 0);
    free(newString);

    newString = substringSlice("abc", 1, 2);
    assert(__LINE__, strcmp(newString, "b") == 0);
    free(newString);

    newString = substringSlice("abc", 2, 3);
    assert(__LINE__, strcmp(newString, "c") == 0);
    free(newString);
}

void testCharDigits() {
    // check int detection
    assert(__LINE__, checkChar('0'));
    assert(__LINE__, checkChar('1'));
    assert(__LINE__, checkChar('2'));
    assert(__LINE__, checkChar('3'));
    assert(__LINE__, checkChar('4'));
    assert(__LINE__, checkChar('5'));
    assert(__LINE__, checkChar('6'));
    assert(__LINE__, checkChar('7'));
    assert(__LINE__, checkChar('8'));
    assert(__LINE__, checkChar('9'));

    // check non int detection
    assert(__LINE__, ~checkChar('-'));
    assert(__LINE__, ~checkChar('z'));
    assert(__LINE__, ~checkChar('!'));
    assert(__LINE__, ~checkChar(';'));
    assert(__LINE__, ~checkChar('`'));
    assert(__LINE__, ~checkChar('#'));
}

void testInt() {
    // string to int conversion
    char *testingInt = malloc(2 * sizeof(char));
    strcpy(testingInt, "0");
    assert(__LINE__, freeingConvertStrToInt(testingInt) == 0);

    testingInt = malloc(4 * sizeof(char));
    strcpy(testingInt, "125");
    assert(__LINE__, freeingConvertStrToInt(testingInt) == 125);

    testingInt = malloc(8 * sizeof(char));
    strcpy(testingInt, "3452489");
    assert(__LINE__, freeingConvertStrToInt(testingInt) == 3452489);

    testingInt = malloc(5 * sizeof(char));
    strcpy(testingInt, "0000");
    assert(__LINE__, freeingConvertStrToInt(testingInt) == 0);

    testingInt = malloc(6 * sizeof(char));
    strcpy(testingInt, "00809");
    assert(__LINE__, freeingConvertStrToInt(testingInt) == 809);
}

void testWhitespace() {
    // check whitespace detection
    assert(__LINE__, isWhitespace(' '));
    assert(__LINE__, isWhitespace('\t'));
    assert(__LINE__, isWhitespace('\r'));
    assert(__LINE__, isWhitespace('\n'));
    assert(__LINE__, isWhitespace('\v'));
    assert(__LINE__, isWhitespace('\f'));

    // check non whitespace detection
    assert(__LINE__, ~isWhitespace('-'));
    assert(__LINE__, ~isWhitespace('z'));
    assert(__LINE__, ~isWhitespace('!'));
    assert(__LINE__, ~isWhitespace(';'));
    assert(__LINE__, ~isWhitespace('0'));
    assert(__LINE__, ~isWhitespace('#'));

}

void testGreyscale() {
    assert(__LINE__, greyscaleAsRGBA(0) == 255);
    assert(__LINE__, greyscaleAsRGBA(255) == 4294967295);
}

void testBitsToString() {
    char *data = malloc(6);
    dataBitsToString(data, 0);
    assert(__LINE__, data[0] == (char)192);
    assert(__LINE__, data[1] == (char)192);
    assert(__LINE__, data[2] == (char)192);
    assert(__LINE__, data[3] == (char)192);
    assert(__LINE__, data[4] == (char)192);
    assert(__LINE__, data[5] == (char)192);
    dataBitsToString(data, 4294967295);
    assert(__LINE__, data[0] == (char)195);
    assert(__LINE__, data[1] == (char)255);
    assert(__LINE__, data[2] == (char)255);
    assert(__LINE__, data[3] == (char)255);
    assert(__LINE__, data[4] == (char)255);
    assert(__LINE__, data[5] == (char)255);
    free(data);
}

void testWriteColor() {
    FILE *testFile = fopen("testingThings.test.sk", "w+");

    char toWrite[] = "things";
    writeColor(testFile, toWrite);
    fclose(testFile);
    testFile = fopen("testingThings.test.sk", "rb");
    assert(__LINE__, fgetc(testFile) == 't');
    assert(__LINE__, fgetc(testFile) == 'h');
    assert(__LINE__, fgetc(testFile) == 'i');
    assert(__LINE__, fgetc(testFile) == 'n');
    assert(__LINE__, fgetc(testFile) == 'g');
    assert(__LINE__, fgetc(testFile) == 's');
    assert(__LINE__, fgetc(testFile) == 131);
    fclose(testFile);
    assert(__LINE__, remove("testingThings.test.sk") == 0);
}

void testUnpackPgm() {
    char *filename = "testingPgmUnpack.test.pgm";
    FILE *testingFile = fopen(filename, "w+");
    fprintf(testingFile, "P5 200 200 255 o");
    fclose(testingFile);

    int *height = malloc(sizeof(int));
    int *width = malloc(sizeof(int));
    int *colorSizeBack = malloc(sizeof(int));

    unsigned char *testMatrix = unpackPgm(height, width, colorSizeBack, filename);
    assert(__LINE__, *colorSizeBack == 1);
    assert(__LINE__, *width == 200);
    assert(__LINE__, *height == 200);
    assert(__LINE__, *testMatrix == 'o');
    free(testMatrix);
    free(height);
    free(width);
    free(colorSizeBack);
    assert(__LINE__, remove(filename) == 0);
}

void testWriteSkRLE() {
    unsigned char *matrix = malloc(2);
    *matrix = (char)255;
    char filename[] = "testingSkRLE.test.pgm";

    writeSkRLE(matrix, 1, 1, 1, filename);
    FILE *sk = fopen("testingSkRLE.test.sk", "rb");
    
    // selecting line tool
    assert(__LINE__, fgetc(sk) == 129);
    // colour data
    assert(__LINE__, fgetc(sk) == 195);
    assert(__LINE__, fgetc(sk) == 255);
    assert(__LINE__, fgetc(sk) == 255);
    assert(__LINE__, fgetc(sk) == 255);
    assert(__LINE__, fgetc(sk) == 255);
    assert(__LINE__, fgetc(sk) == 255);
    // colour tool
    assert(__LINE__, fgetc(sk) == 131);
    // a single dy down
    assert(__LINE__, fgetc(sk) == 65);
    // check that end of file
    assert(__LINE__, fgetc(sk) == -1);

    fclose(sk);
    remove("testingSkRLE.test.sk");
    free(matrix);
}

// run all tests
void test() {
    testSubStringSlicing();
    testCharDigits();
    testInt();
    testWhitespace();
    testGreyscale();
    testBitsToString();
    testWriteColor();
    testUnpackPgm();
    testWriteSkRLE();
    printf("All tests passed\n");
}


// Run the program or, if there are no arguments, test it.
int main(int n, char *args[n]) {
    setbuf(stdout, NULL);

    switch(n) {
        case 1:
            test();
            break;

        case 2: {
            if(args[1][strlen(args[1]) - 1] == 'm') {
                convertPgmToSk(args[1]);
            } else if(args[1][strlen(args[1]) - 1] == 'k') {
                convertSkToPgm(args[1]);
            } else {
                fprintf(stderr, "No valid file supplied\n");
            }
            break;
        }

        default:
            fprintf(stderr, "Use ./converter myimage.pgm format\n");
            fprintf(stderr, "To turn pgm file into an sk file\n\n");
            fprintf(stderr, "Use ./converter myimage.sk format\n");
            fprintf(stderr, "To turn sk file into a pgm file\n\n");
            exit(1);
    }

    return 0;
}
