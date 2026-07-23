#include "menu.h"
#include "cglp.h"
#include "string.h"

#define MAX_GAMES_PER_PAGE 14
#define KEY_REPEAT_DURATION 30
int gameCount = 0;
Game[MAX_GAME_COUNT] games;
int gameIndex = 1;
int keyRepeatTicks = 0;

void menuUpdate() {
  Collision scratch;
  // None of the text()/character() calls below ever read the Collision
  // result they're passed - the menu doesn't need hit-testing for its
  // own drawing at all. Without this, every character of every visible
  // title would still run the full checkHitBox() scan (see cglp.c) -
  // registering and testing against every hitbox drawn so far this
  // frame - on *every* frame, whether or not the menu-idle skip above
  // even draws anything this frame; that gate only ever covered the GPU
  // calls, not collision detection, which happens earlier and doesn't
  // care whether shouldDrawThisFrame is true. Gets set back to true
  // automatically the moment a real game starts, via resetDrawState()
  // in initInGame().
  hasCollision = false;
  color = BLUE;
  text("[A]      [B]", 3, 3, &scratch);
  color = BLACK;
  text("   Select   Down", 3, 3, &scratch);

  if (input.b.isPressed || input.down.isPressed || input.up.isPressed) {
    keyRepeatTicks++;
  } else {
    keyRepeatTicks = 0;
  }
  if (input.b.isJustPressed || input.down.isJustPressed ||
      (keyRepeatTicks > KEY_REPEAT_DURATION &&
       (input.b.isPressed || input.down.isPressed))) {
    gameIndex++;
    gameIndex = cgl_wrap(gameIndex, 1, gameCount);
    while (games[gameIndex].update == NULL) {
      gameIndex++;
      gameIndex = cgl_wrap(gameIndex, 1, gameCount);
    }
    if (keyRepeatTicks > KEY_REPEAT_DURATION) {
      keyRepeatTicks = KEY_REPEAT_DURATION / 3 * 2;
    }
  }
  if (input.up.isJustPressed ||
      (keyRepeatTicks > KEY_REPEAT_DURATION && input.up.isPressed)) {
    gameIndex--;
    gameIndex = cgl_wrap(gameIndex, 1, gameCount);
    while (games[gameIndex].update == NULL) {
      gameIndex--;
      gameIndex = cgl_wrap(gameIndex, 1, gameCount);
    }
    if (keyRepeatTicks > KEY_REPEAT_DURATION) {
      keyRepeatTicks = KEY_REPEAT_DURATION / 3 * 2;
    }
  }

  // Calculate current page and starting index
  int currentGames = 0;
  int page = 0;
  int startIndex = 1; // Start from 1 to skip first game

  // Find which page we're on and the starting index
  int tempIndex = 1; // Start from 1
  int gamesPerPage;
  if (page == 0) {
    gamesPerPage = MAX_GAMES_PER_PAGE;
  } else {
    gamesPerPage = MAX_GAMES_PER_PAGE - 1;
  }
  while (tempIndex + gamesPerPage <= gameIndex) {
    tempIndex += gamesPerPage;
    startIndex = tempIndex;
    page++;
  }

  // Draw the games for current page
  currentGames = gamesPerPage;
  for (int i = 0; i < currentGames; i++) {
    int gamePos = startIndex + i;
    if (gamePos >= gameCount) break;

    float y = (i + 1) * 6; // Always offset by one line

    // Draw selection cursor
    if (gamePos == gameIndex) {
      color = BLUE;
      text(">", 3, y + 3, &scratch);
    }

    // Offset normal games vs category
    // i assume games without update function are category
    int offset = 0;
    color = BLACK;
    if (games[gamePos].update == NULL) {
      color = RED;
      offset = -9;
    }
    // Draw game title
    text(games[gamePos].title, 9 + offset, y + 3, &scratch);
  }

  // "page/totalPages" indicator, one line below the game list. Its Y
  // position is derived from MAX_GAMES_PER_PAGE (the same define that
  // sizes the list above, via gamesPerPage) rather than a fixed number,
  // so it always sits directly under the list regardless of how many
  // rows that define is set to show.
  int totalPages = 1;
  int totalPagesTempIndex = 1;
  int lastGameIndex = gameCount - 1;
  while (totalPagesTempIndex + gamesPerPage <= lastGameIndex) {
    totalPagesTempIndex += gamesPerPage;
    totalPages++;
  }
  float pageIndicatorY = (MAX_GAMES_PER_PAGE + 1) * 6 + 3;
  color = BLACK;
  int[16] pageText;
  strcpy(pageText, "PAGE: ");
  strcat(pageText, intToChar(page + 1));
  strcat(pageText, "/");
  strcat(pageText, intToChar(totalPages));
  text(pageText, 3, pageIndicatorY, &scratch);

  if (input.a.isJustPressed) {
    if (games[gameIndex].update != NULL) {
      restartGame(gameIndex);
    }
  }
}

void addGame(int* title, int* description,
             int[CHARACTER_WIDTH][CHARACTER_HEIGHT + 1]* characters,
             int charactersCount, Options* options, bool usesMouse,
             UpdateFunc* update) {
  if (gameCount >= MAX_GAME_COUNT) {
    return;
  }
  Game* g = &games[gameCount];
  g->title = title;
  g->description = description;
  g->characters = characters;
  g->charactersCount = charactersCount;
  g->options = *options;
  g->usesMouse = usesMouse;
  g->update = update;
  gameCount++;
}

Game* getGame(int index) { return &games[index]; }

void addMenu() {
  Options o = {100, 100, 0, false};
  addGame("", "", NULL, 0, &o, false, &menuUpdate);
}
