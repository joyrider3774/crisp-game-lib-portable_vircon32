#ifndef MENU_H
#define MENU_H

// Vircon32 port note: addGame() used to take "Options options" BY VALUE
// (4 words) - not allowed across a Vircon32 function boundary, so it now
// takes "Options* options" instead. getGame() used to return "Game" BY
// VALUE (~7 words) - instead it now returns a pointer straight into the
// internal games[] array (simpler than an out-pointer copy, and avoids
// copying a struct that's never mutated by the caller anyway).

extern int gameCount;
void addGame(int* title, int* description,
             int[CHARACTER_WIDTH][CHARACTER_HEIGHT + 1]* characters,
             int charactersCount, Options* options, bool usesMouse,
             UpdateFunc* update);
Game* getGame(int index);
void addMenu();
void addGames();

#endif
