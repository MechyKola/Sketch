// Basic program skeleton for a Sketch File (.sk) Viewer
#include "displayfull.h"
#include "sketch.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Allocate memory for a drawing state and initialise it
state *newState() {
  state *newStatePointer = malloc(sizeof(state));
  newStatePointer->start = 0;
  newStatePointer->end = false;
  newStatePointer->data = 0;
  newStatePointer->tool = LINE;
  newStatePointer->tx = 0;
  newStatePointer->ty = 0;
  newStatePointer->x = 0;
  newStatePointer->y = 0;

  return newStatePointer; // this is a placeholder only
}

// Release all memory associated with the drawing state
void freeState(state *s) {
  free(s);
}

// binary to int helper function, for unisigned binary
int binaryToInt(byte b, int bits) {
  int multiplier = 1;
  int accumulator = 0;

  for(int i = 0; i < bits; i++) {
    accumulator += ((b >> i) & 1) * multiplier;
    multiplier *= 2;
  }

  return accumulator;
}

// Extract an opcode from a byte (two most significant bits).
int getOpcode(byte b) {
  int accumulator = 0;
  accumulator += (b >> 6) & 1;
  accumulator += ((b >> 7) & 1) * 2;
  return accumulator;
}

// Extract an operand (-32..31) from the rightmost 6 bits of a byte.
int getOperand(byte b) {
  int value = binaryToInt(b, 5);
  if(((b >> 5) & 1) == 1) {
    value -= 32;
  }

  return value;
}

void processDX(state *s, byte b) {
  s->tx += getOperand(b);
}

void processDY(display *d, state *s, byte b) {
  s->ty += getOperand(b);
  switch(s->tool) {
    case LINE:
      line(d, s->x, s->y, s->tx, s->ty);
      break;
    case BLOCK:
      block(d, s->x, s->y, s->tx - s->x, s->ty - s->y);
      break;
  }
  s->y = s->ty;
  s->x = s->tx;
}

void processTOOL(display *d, state *s, byte b) {
  switch(binaryToInt(b, 6)) {
    case NONE:
      s->tool = NONE;
      break;
    case LINE:
      s->tool = LINE;
      break;
    case BLOCK:
      s->tool = BLOCK;
      break;
    case COLOUR:
      colour(d, s->data);
      break;
    case TARGETX:
      s->tx = s->data;
      break;
    case TARGETY:
      s->ty = s->data;
      break;
    case SHOW:
      show(d);
      break;
    case PAUSE:
      pause(d, s->data);
      break;
    case NEXTFRAME:
      show(d);
      break;
  }
  s->data = 0;
}

void processData(state *s, byte b) {
  s->data = s->data << 6;
  s->data += binaryToInt(b, 6);
}

// Execute the next byte of the command sequence.
void obey(display *d, state *s, byte op) {
  switch(getOpcode(op)) {
    case DX:
      processDX(s, op);
      break;
    case DY:
      processDY(d, s, op);
      break;
    case TOOL:
      processTOOL(d, s, op);
      break;
    case DATA:
      processData(s, op);
      break;
  }
}

// Draw a frame of the sketch file. For basic and intermediate sketch files
// this means drawing the full sketch whenever this function is called.
// For advanced sketch files this means drawing the current frame whenever
// this function is called.
bool processSketch(display *d, void *data, const char pressedKey) {
  char *filename = getName(d);
  FILE *in = fopen(filename, "rb");
  byte b;

  if(data == NULL) {
    return (pressedKey == 27);
  }
  state *s = (state*) data;

  
  // handle nextframe 
  if(s->start != 0) {
    printf("\n\n%d\n\n", s->start);
    for(int i = 0; i < s->start + 1; i++) {
      b = fgetc(in);
    }
  }
  

  while(! feof(in)) {
    b = fgetc(in);
    obey(d, s, b);

    if(getOpcode(b) == TOOL && getOperand(b) == NEXTFRAME) {
      printf("after NEXTFRAME thing\n");
      printf("%d\n", s->start);
      s->end = false;
      s->data = 0;
      s->tool = LINE;
      s->tx = 0;
      s->ty = 0;
      s->x = 0;
      s->y = 0;
      return 0;
    }
    
    s->start++;
  }
  show(d);
  fclose(in);

  // reset state
  s->start = 0;
  s->data = 0;
  s->tool = LINE;
  s->tx = 0;
  s->ty = 0;
  s->x = 0;
  s->y = 0;

    //TO DO: OPEN, PROCESS/DRAW A SKETCH FILE BYTE BY BYTE, THEN CLOSE IT
    //NOTE: CHECK DATA HAS BEEN INITIALISED... if (data == NULL) return (pressedKey == 27);
    //NOTE: TO GET ACCESS TO THE DRAWING STATE USE... state *s = (state*) data;
    //NOTE: TO GET THE FILENAME... char *filename = getName(d);
    //NOTE: DO NOT FORGET TO CALL show(d); AND TO RESET THE DRAWING STATE APART FROM
    //      THE 'START' FIELD AFTER CLOSING THE FILE

  return (pressedKey == 27);
}

// View a sketch file in a 200x200 pixel window given the filename
void view(char *filename) {
  display *d = newDisplay(filename, 200, 200);
  state *s = newState();
  run(d, s, processSketch);
  freeState(s);
  freeDisplay(d);
}

// Include a main function only if we are not testing (make sketch),
// otherwise use the main function of the test.c file (make test).
#ifndef TESTING
int main(int n, char *args[n]) {
  if (n != 2) { // return usage hint if not exactly one argument
    printf("Use ./sketch file\n");
    exit(1);
  } else view(args[1]); // otherwise view sketch file in argument
  return 0;
}
#endif
