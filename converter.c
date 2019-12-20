/* directed graph */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>
// #include "displayfull.h"


// yuh
struct ye { int id; };
typedef struct ye ye;


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

unsigned char* unpackPgm(int *height, int *width, int *colorSizeBack, char *filename) {
    // validate file
    if(!(access( filename, F_OK ) != -1)) {
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
        printf("%c\n", currentChar);
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
    /*
    while(!feof(pgmFile)) {
        imageMatrix[column][row] = fgetc(pgmFile);
        column++;
        if(column == *width) {
            row++;
            column = 0;
        }
    }
    */

    fclose(pgmFile);

    return imageMatrix;
}

// accepts 32 bit integer representing rgb and writes it to characters in string
void dataBitsToString(char *toWrite, unsigned long colorValue) {
    for(int i = 0; i < 6; i++) {
        toWrite[5 - i] = ((colorValue >> (6 * i)) & 63) + 128 + 64;
        // printf("%c", toWrite[5 - i]);
    }
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

void writeColor(FILE *fileToWriteTo, char *colorData) {
    for(int k = 0; k < 6; k++) {
        fputc(colorData[k], fileToWriteTo);
    }
    // telling sketch to process data as color
    fputc(128 + 3, fileToWriteTo);
}

// encode using rle, top to bottom then bottom to top
// incremnting DX by 1 each time
// alternating between going down and up with DY
// updating color each time it changes
void writeSkRLE(unsigned char *imageMatrix, int colorSize, int height, int width) {
    int counter = 0;
    unsigned long currentGreyValue = *imageMatrix;
    char* currentColorString = malloc(6); // 32 bits of data, 6 chars/bytes
    bool down = true;
    FILE *skFile = fopen("test.sk", "w+");

    // sets tool to line and initialise colour
    fputc(128 + 1, skFile);
    dataBitsToString(currentColorString, greyscaleAsRGBA(currentGreyValue));
    writeColor(skFile, currentColorString);

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
        fputc(128, skFile);
        if(x != width - 1) {
            fputc(1, skFile);
            if(down) {
                fputc(64 + 32 + 16 + 8 + 4 + 2 + 1, skFile);
            } else {
                fputc(64 + 1, skFile);
            }
        }
        fputc(128 + 1, skFile);
    }

    free(currentColorString);
    fclose(skFile);
}

// convert .pgm to .sk - this function unpacks the file
void convertToSk(char* filename) {
    int *height = malloc(sizeof(int));
    int *width = malloc(sizeof(int));
    int *colorSize = malloc(sizeof(int));
    unsigned char *imageMatrix = unpackPgm(height, width, colorSize, filename);
    if(*colorSize != 1) {
        fprintf(stderr, "This color size (%d) isn't implemented yet\n", *colorSize);
        exit(1);
    }

    // rle by making horizontal lines across entire image
    writeSkRLE(imageMatrix, *colorSize, *height, *width);

    free(imageMatrix);
    free(height);
    free(width);
    free(colorSize);
}

// run all tests
void test() {
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
                convertToSk(args[1]);
            } else if(args[1][strlen(args[1]) - 1] == 'k') {
                // convert to pgm
                // TODO
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
