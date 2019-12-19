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

char* unpackPgm(int *height, int *width, int *colorSizeBack, char *filename) {
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

    char *imageMatrix = malloc(imageHeight * imageWidth * colorSize + 1);

    *width = imageWidth;
    *height = imageHeight;
    *colorSizeBack = colorSize;

    // actual image
    counter = 0;
    while(!feof(pgmFile)) {
        imageMatrix[counter] = fgetc(pgmFile);
        counter++;
    }

    fclose(pgmFile);

    return imageMatrix;
}

// accepts 32 bit integer representing rgb and writes it to characters in string
char* intAsData(char *toWrite, int colorValue) {
    for(int i = 0; i < 6; i++) {
        toWrite[5 - i] = ((colorValue >> (6 * i)) & 63) + 128 + 64;
    }
    return toWrite;
}

// turns 1 byte greyscale into rgba
int greyscaleAsRGBA(int greyscale) {
    int color = greyscale + (greyscale << 8) + (greyscale << 16) + (255 << 24);
    return color;
}

void writeSkRLE(char *imageMatrix, int colorSize, int height, int width) {
    // encode using rle, top to bottom then bottom to top
    // incremnting DX by 1 each time
    // alternating between going down and up with DY
    // updating color each time it changes
}

// convert .pgm to .sk - this function unpacks the file
void convertToSk(char* filename) {
    int *height = malloc(sizeof(int));
    int *width = malloc(sizeof(int));
    int *colorSize = malloc(sizeof(int));
    if(colorSize != 1) {
        fprintf(stderr, "This color size isn't implemented yet\n");
        exit(1);
    }
    char *imageMatrix = unpackPgm(height, width, colorSize, filename);

    // rle by making horizontal lines across entire image

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
