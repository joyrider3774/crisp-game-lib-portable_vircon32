#ifndef CGLP_H
#define CGLP_H

// =============================================================================
// Vircon32 port notes (read this before touching call sites in games)
// =============================================================================
//  - "typedef struct {...} Name;" is rejected by the Vircon32 parser -> every
//    type below is a named "struct Name {...};" instead (no typedef needed;
//    the tag name works directly as a type name at use sites).
//  - Vircon32 functions can only pass/return ONE WORD (32 bits) across a call
//    boundary - no struct/array by value. The original API returned
//    "Collision" (a ~270-word struct!) BY VALUE from rect()/box()/line()/
//    bar()/arc()/text()/character(), and took "Options" (4 words) by value
//    in addGame(). Both are reworked to the officially-recommended Vircon32
//    idiom: the result becomes an OUT-POINTER passed as the LAST parameter,
//    and the function returns void. Concretely:
//        old: Collision c = rect(x, y, w, h);
//        new: Collision c; rect(x, y, w, h, &c);
//    Every game that calls these functions needs its call sites updated to
//    this two-statement form - see games/gamePinClimb.c for a worked example.
//  - char* strings become int* (a "char" is just a full 32-bit word here;
//    there is no byte-packed C string).
//  - "unsigned char"/"unsigned int" do not exist -> plain "int".
//  - "static" (both for functions and file-scope globals) is not accepted by
//    the compiler and is also meaningless here anyway, since a Vircon32
//    program is always a single translation unit with no linker. It is
//    simply dropped everywhere. Practical consequence: every global/function
//    name in every included game must be unique program-wide (no more
//    "static" to hide per-file names) - ported games get their file's
//    original name used as a prefix, e.g. "pinclimb_title".
//  - No variadics ("..."), so consoleLog() drops its old printf-style
//    format/varargs and just takes a single string.
// =============================================================================

#include "math.h"
#include "machineDependent.h"
#include "vector.h"

#define MAX_GAME_COUNT 49

#define FPS 60

#define COLOR_COUNT 15
#define TRANSPARENT -1
#define WHITE 0
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define PURPLE 5
#define CYAN 6
#define BLACK 7
#define LIGHT_RED 8
#define LIGHT_GREEN 9
#define LIGHT_YELLOW 10
#define LIGHT_BLUE 11
#define LIGHT_PURPLE 12
#define LIGHT_CYAN 13
#define LIGHT_BLACK 14

#define CHARACTER_WIDTH 6
#define CHARACTER_HEIGHT 6

#define SOUND_EFFECT_TYPE_COUNT 9
#define COIN 0
#define LASER 1
#define EXPLOSION 2
#define POWER_UP 3
#define HIT 4
#define JUMP 5
#define SELECT 6
#define RANDOM 7
#define CLICK 8

#define TEXT_PATTERN_COUNT 94
#define MAX_CHARACTER_PATTERN_COUNT 26
#define ASCII_CHARACTER_COUNT 127
#define MAX_CACHED_CHARACTER_PATTERN_COUNT 128

// PI constants (no "f" literal suffix in this dialect - float is already
// the only floating type, so plain literals are used everywhere)
#define CGLP_PI 3.141592
#define CGLP_PI_2 1.570796
#define CGLP_PI_4 0.785398
// Aliases so ported game source can keep using the standard names.
#define M_PI CGLP_PI
#define M_PI_2 CGLP_PI_2
#define M_PI_4 CGLP_PI_4

struct Collisions {
  bool[COLOR_COUNT] rect;
  bool[ASCII_CHARACTER_COUNT] text;
  bool[ASCII_CHARACTER_COUNT] character;
};

struct Collision {
  Collisions isColliding;
};

struct ButtonState {
  bool isPressed;
  bool isJustPressed;
  bool isJustReleased;
};

struct Input {
  ButtonState left;
  ButtonState right;
  ButtonState up;
  ButtonState down;
  ButtonState b;
  ButtonState a;
  bool isPressed;
  bool isJustPressed;
  bool isJustReleased;
  Vector pos;
};

struct Options {
  int viewSizeX;
  int viewSizeY;
  int soundSeed;
  bool isDarkColor;
};

struct CharacterOptions {
  bool isMirrorX;
  bool isMirrorY;
  int rotation;
};

struct GameHiScore {
  int[20] title;
  int hiScore;
};

struct Game;

// Vircon32 port note: "void (*fp)(...)" is NOT valid syntax here (the
// dialect's "<type> <n>" rule applies to function pointers too) - the
// function *signature* must be typedef'd first, then used as "Name* var".
// Assigning to one of these also requires "&function" (not bare
// "function" like standard C).
typedef void(void) UpdateFunc;
typedef void(Game*) ResetGameFunc;
typedef void() SaveDataFunc;

struct Game {
  int* title;
  int* description;
  int[CHARACTER_WIDTH][CHARACTER_HEIGHT + 1]* characters;
  int charactersCount;
  Options options;
  bool usesMouse;
  UpdateFunc* update;
};

// Vircon32 note: "#pragma pack" does not exist here (there is no byte
// packing at all - everything is one word), so it is simply omitted; the
// struct layout below is equivalent in spirit to the original packed one.
struct SaveData {
  int magic; // always first
  GameHiScore[MAX_GAME_COUNT] hiScores;
  int crc;
};

extern SaveData saveData;
extern int currentGameIndex;
extern ResetGameFunc* onResetGame;
extern SaveDataFunc* onSaveData;
extern int ticks;
extern float score;
extern float difficulty;
extern int color;
extern float thickness;
extern float barCenterPosRatio;
extern CharacterOptions characterOptions;
extern bool hasCollision;
extern float tempo;
extern Input input;
extern Input currentInput;
extern bool isInMenu;
extern bool isInGameOver;

// Collision-producing draw calls: out-pointer "result" is always the LAST
// parameter and must point to caller-owned storage (see port notes above).
void rect(float x, float y, float w, float h, Collision* result);
void box(float x, float y, float w, float h, Collision* result);
void line(float x1, float y1, float x2, float y2, Collision* result);
void bar(float x, float y, float length, float angle, Collision* result);
void arc(float centerX, float centerY, float radius, float angleFrom, float angleTo, Collision* result);
void text(int* msg, float x, float y, Collision* result);
void character(int* msg, float x, float y, Collision* result);

void play(int type);
void addScore(float value, float x, float y);
float rnd(float low, float high);
int rndi(int low, int high);
void gameOver();
void particle(float x, float y, float count, float speed, float angle, float angleWidth);

float clamp(float v, float low, float high);
float cgl_wrap(float v, float low, float high);
int* intToChar(int v);
void consoleLog(int* msg);
void enableSound();
void disableSound();
void toggleSound();
void goToMenu();
void restartGame(int gameIndex);

struct ColorRgb {
  int r;
  int g;
  int b;
};

extern ColorRgb[COLOR_COUNT] colorRgbs;

void initGame();
void setButtonState(bool left, bool right, bool up, bool down, bool b, bool a);
void setMousePos(float x, float y);
void updateFrame();

// Iterate over an `array` with variable `index`
#define FOR_EACH(array, index) \
  for (int index = 0; index < sizeof(array) / sizeof(array[0]); index++)
// Assign the `index` th item in the `array` to an `item` variable of `type`
#define ASSIGN_ARRAY_ITEM(array, index, type, item) type* item = &array[index]
// Skip (continue) if the member `isAlive` of variable `item` is false.
#define SKIP_IS_NOT_ALIVE(item) \
  if (!item->isAlive) continue
// Iterate `count` times with variable `index`
#define TIMES(count, index) for (int index = 0; index < count; index++)
// Count the number of items in the array for which the `isAlive` member is
// true and assigns it to a variable defined as the int variable `counter`
#define COUNT_IS_ALIVE(array, counter)                          \
  int counter = 0;                                              \
  do {                                                          \
    for (int i = 0; i < sizeof(array) / sizeof(array[0]); i++) {\
      if (array[i].isAlive) {                                   \
        counter++;                                              \
      }                                                         \
    }                                                            \
  } while (0)
// Set the `isAlive` member to false for all items in the `array`
#define INIT_UNALIVED_ARRAY(array)                              \
  do {                                                          \
    for (int i = 0; i < sizeof(array) / sizeof(array[0]); i++) {\
      array[i].isAlive = false;                                 \
    }                                                            \
  } while (0)
// Same as INIT_UNALIVED_ARRAY, but zeroes the whole array via a single
// memset call (Vircon32's memset is the hardware "sets" fill instruction
// - see misc.h - not a software loop) instead of looping per element to
// touch only isAlive. Zeroing every field this way is only equivalent in
// observable behavior when SKIP_IS_NOT_ALIVE() guards every read of a
// dead item's other fields (true throughout this port's games - a
// revived item always has every field it uses explicitly reassigned) AND
// the element type has NO pointer field anywhere in it, including nested
// structs: Vircon32's NULL is -1, not 0, so zeroing a pointer field this
// way leaves it as a garbage, non-null-looking address rather than a
// safe null, and `if (ptr == NULL)` would then wrongly treat it as valid
// and dereference it. Verify the element struct by hand (recursively,
// through any nested struct fields) before using this for a new array -
// do not assume, check.
#define INIT_UNALIVED_ARRAY_FAST(array) memset(array, 0, sizeof(array))
// Return 1 or -1 randomly
#define RNDPM() (rndi(0, 2) * 2 - 1)

#include "menu.h"

#endif
