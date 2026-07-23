#include "../cglp.h"

char* thunderTitle = "THUNDER";
char* thunderDescription = "[Tap]   Turn\n[Arrow] Move";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] thunderCharacters = {
    {
        "      ",
        "      ",
        "   l  ",
        "  lll ",
        "  l l ",
        "      ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "      ",
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
    }};
int thunderCharactersCount = 3;

Options thunderOptions = {100, 100, 9, true};

struct ThunderLine {
  Vector from;
  Vector to;
  Vector vel;
  float ticks;
  ThunderLine* prevLine;
  bool isActive;
  bool isAlive;
};
#define THUNDER_MAX_LINE_COUNT 128
ThunderLine[THUNDER_MAX_LINE_COUNT] thunderLines;
int thunderLineIndex;
int thunderActiveTicks;
struct ThunderStar {
  Vector pos;
  float vy;
  bool isAlive;
};
#define THUNDER_MAX_STAR_COUNT 32
ThunderStar[THUNDER_MAX_STAR_COUNT] thunderStars;
int thunderStarIndex;
struct ThunderPlayer {
  float x;
  float vx;
};
ThunderPlayer thunderPlayer;
int thunderMultiplier;

void thunderUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY(thunderLines);
    thunderLineIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(thunderStars);
    thunderStarIndex = 0;
    thunderActiveTicks = -1;
    thunderPlayer.x = 40;
    thunderPlayer.vx = 1;
    thunderMultiplier = 1;
  }
  COUNT_IS_ALIVE(thunderLines, lc);
  if (lc == 0) {
    ASSIGN_ARRAY_ITEM(thunderLines, thunderLineIndex, ThunderLine, l);
    vectorSet(&l->from, rnd(30, 70), 0);
    vectorSet(&l->to, l->from.x, l->from.y);
    vectorSet(&l->vel, 0.5 * difficulty, 0);
    rotate(&l->vel, M_PI / 2);
    l->ticks = ceil(30.0 / difficulty);
    l->prevLine = NULL;
    l->isActive = false;
    l->isAlive = true;
    thunderLineIndex = cgl_wrap(thunderLineIndex + 1, 0, THUNDER_MAX_LINE_COUNT);
  }
  color = LIGHT_BLUE;
  rect(0, 90, 100, 10, &scratch);
  thunderActiveTicks--;
  FOR_EACH(thunderLines, i) {
    ASSIGN_ARRAY_ITEM(thunderLines, i, ThunderLine, l);
    SKIP_IS_NOT_ALIVE(l);
    if (l->isActive) {
      color = YELLOW;
      line(l->from.x, l->from.y, l->to.x, l->to.y, &scratch);
      l->isAlive = thunderActiveTicks >= 0;
      continue;
    }
    l->ticks--;
    if (thunderActiveTicks > 0) {
      if (l->ticks > 0) {
        ASSIGN_ARRAY_ITEM(thunderStars, thunderStarIndex, ThunderStar, s);
        vectorSet(&s->pos, l->to.x, l->to.y);
        s->vy = -l->to.y * 0.02;
        s->isAlive = true;
        thunderStarIndex = cgl_wrap(thunderStarIndex + 1, 0, THUNDER_MAX_STAR_COUNT);
      }
      l->isAlive = false;
      continue;
    }
    if (l->ticks > 0) {
      vectorAdd(&l->to, l->vel.x, l->vel.y);
      if (thunderActiveTicks < 0 && (l->to.y > 90 || lc >= THUNDER_MAX_LINE_COUNT)) {
        play(EXPLOSION);
        ThunderLine* al = l;
        color = YELLOW;
        for (int j = 0; j < 99; j++) {
          particle(al->to.x, al->to.y, 30, 2, 0, M_PI * 2);
          al->isActive = true;
          al = al->prevLine;
          if (al == NULL) {
            break;
          }
        }
        thunderActiveTicks = ceil(20.0 / sqrt(difficulty));
        thunderMultiplier = 1;
      }
    } else if (l->ticks == 0) {
      play(HIT);
      color = BLACK;
      particle(l->to.x, l->to.y, 9, 1, 0, M_PI * 2);
      for (int j = 0; j < rndi(1, 4); j++) {
        ASSIGN_ARRAY_ITEM(thunderLines, thunderLineIndex, ThunderLine, nl);
        vectorSet(&nl->from, l->to.x, l->to.y);
        vectorSet(&nl->to, l->to.x, l->to.y);
        vectorSet(&nl->vel, l->vel.x, l->vel.y);
        vectorMul(&nl->vel, 1.0 / vectorLength(&nl->vel));
        rotate(&nl->vel, rnd(-0.7, 0.7));
        vectorMul(&nl->vel, rnd(0.3, 1) * sqrt(difficulty));
        nl->ticks = ceil(rnd(20, 40) / difficulty);
        nl->prevLine = l;
        nl->isActive = false;
        nl->isAlive = true;
        thunderLineIndex = cgl_wrap(thunderLineIndex + 1, 0, THUNDER_MAX_LINE_COUNT);
      }
    }
    color = LIGHT_BLACK;
    hasCollision = false;
    thickness = 2;
    line(l->from.x, l->from.y, l->to.x, l->to.y, &scratch);
    thickness = 3;
    hasCollision = true;
  }
  bool isJustPressed = input.isJustPressed;
  if (isJustPressed && ((input.left.isJustPressed && thunderPlayer.vx < 0) ||
                        (input.right.isJustPressed && thunderPlayer.vx > 0))) {
    isJustPressed = false;
  }
  if (isJustPressed || (thunderPlayer.x < 0 && thunderPlayer.vx < 0) ||
      (thunderPlayer.x > 99 && thunderPlayer.vx > 0)) {
    play(LASER);
    thunderPlayer.vx *= -1;
  }
  thunderPlayer.x += thunderPlayer.vx * sqrt(difficulty);
  color = BLACK;
  characterOptions.isMirrorX = thunderPlayer.vx < 0;
  int[2] pc;
  pc[0] = 'b' + (ticks / 10) % 2;
  pc[1] = 0;
  character(pc, thunderPlayer.x, 87, &scratch);
  if (scratch.isColliding.rect[YELLOW]) {
    play(RANDOM);
    gameOver();
  }
  characterOptions.isMirrorX = false;
  color = YELLOW;
  FOR_EACH(thunderStars, i) {
    ASSIGN_ARRAY_ITEM(thunderStars, i, ThunderStar, s);
    SKIP_IS_NOT_ALIVE(s);
    s->vy += 0.1 * difficulty;
    s->pos.y += s->vy;
    if (s->pos.y > 89 && s->vy > 0) {
      s->vy *= -0.5;
      if (s->vy > -0.1) {
        s->isAlive = false;
        continue;
      }
    }
    character("a", s->pos.x, s->pos.y, &scratch);
    bool* c = scratch.isColliding.character;
    if (c['b'] || c['c']) {
      play(COIN);
      addScore(thunderMultiplier, s->pos.x, s->pos.y);
      thunderMultiplier++;
      s->isAlive = false;
    }
  }
}

void addGameThunder() {
  addGame(thunderTitle, thunderDescription, thunderCharacters,
          thunderCharactersCount, &thunderOptions, false, &thunderUpdate);
}
