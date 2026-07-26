#include "../cglp.h"

int* levitationTitle = "LEVITATION";
int* levitationDescription = "[Tap]\nLevitate / Fall";

int[2][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] levitationCharacters = {
    {
        " lll  ",
        "ll ll ",
        "ll ll ",
        "ll ll ",
        "ll ll ",
        "ll lll",
    },
    {
        " llll ",
        "ll  ll",
        "ll  ll",
        "ll  ll",
        "ll  ll",
        " llll ",
    },
};
int levitationCharactersCount = 2;

Options levitationOptions = {100, 100, 8, false};

#define LEVITATION_STATE_CRAWL 0
#define LEVITATION_STATE_ROLL 1

struct LevitationCaterpillar {
  Vector pos;
  Vector vel;
  int state;
};
LevitationCaterpillar levitationCaterpillar;

struct LevitationPlatform {
  Vector pos;
  float width;
  bool isAlive;
};
#define LEVITATION_MAX_PLATFORM_COUNT 16
LevitationPlatform[LEVITATION_MAX_PLATFORM_COUNT] levitationPlatforms;
int levitationPlatformIndex;
float levitationNextPlatformDist;
int levitationMultiplier;

void levitationUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&levitationCaterpillar.pos, 40, 40);
    vectorSet(&levitationCaterpillar.vel, 1, 0);
    levitationCaterpillar.state = LEVITATION_STATE_CRAWL;
    INIT_UNALIVED_ARRAY_FAST(levitationPlatforms);
    vectorSet(&levitationPlatforms[0].pos, 0, 60);
    levitationPlatforms[0].width = 99;
    levitationPlatforms[0].isAlive = true;
    levitationPlatformIndex = 1;
    levitationNextPlatformDist = 0;
    levitationMultiplier = 1;
  }
  if (input.isJustPressed) {
    play(SELECT);
    if (levitationCaterpillar.state == LEVITATION_STATE_CRAWL) {
      levitationCaterpillar.state = LEVITATION_STATE_ROLL;
    } else {
      levitationCaterpillar.state = LEVITATION_STATE_CRAWL;
    }
    levitationMultiplier = 1;
  }
  if (levitationCaterpillar.state == LEVITATION_STATE_CRAWL) {
    levitationCaterpillar.vel.x = -0.4 * difficulty;
    levitationCaterpillar.vel.y = 0;
  } else {
    levitationCaterpillar.vel.x = 0.2 * difficulty;
    levitationCaterpillar.vel.y += 0.2 * sqrt(difficulty);
  }
  levitationCaterpillar.vel.y *= 0.99;
  vectorAdd(&levitationCaterpillar.pos, levitationCaterpillar.vel.x, levitationCaterpillar.vel.y);
  if (levitationCaterpillar.pos.y < 0 && levitationCaterpillar.vel.y < 0) {
    levitationCaterpillar.pos.y = 0;
    levitationCaterpillar.vel.y *= -0.3;
  }
  float scrollSpeed = difficulty * 0.5;
  levitationNextPlatformDist -= scrollSpeed;
  if (levitationNextPlatformDist < 0) {
    float width = rnd(30, 50);
    ASSIGN_ARRAY_ITEM(levitationPlatforms, levitationPlatformIndex, LevitationPlatform, np);
    vectorSet(&np->pos, 100, rnd(30, 90));
    np->width = width;
    np->isAlive = true;
    levitationPlatformIndex =
        cgl_wrap(levitationPlatformIndex + 1, 0, LEVITATION_MAX_PLATFORM_COUNT);
    levitationNextPlatformDist = width + rnd(0, 9);
  }
  if (levitationCaterpillar.pos.y > 100 || levitationCaterpillar.pos.x < -3 ||
      levitationCaterpillar.pos.x > 103) {
    play(EXPLOSION);
    gameOver();
  }
  color = GREEN;
  int[2] shape;
  if (levitationCaterpillar.state == LEVITATION_STATE_CRAWL) {
    shape[0] = 'b';
  } else {
    shape[0] = 'a';
  }
  shape[1] = 0;
  character(shape, levitationCaterpillar.pos.x, levitationCaterpillar.pos.y, &scratch);
  color = LIGHT_BLACK;
  FOR_EACH(levitationPlatforms, i) {
    ASSIGN_ARRAY_ITEM(levitationPlatforms, i, LevitationPlatform, p);
    SKIP_IS_NOT_ALIVE(p);
    rect(p->pos.x, p->pos.y, p->width, 5, &scratch);
    if (scratch.isColliding.character['a'] && levitationCaterpillar.vel.y >= 0) {
      play(JUMP);
      addScore(levitationMultiplier, levitationCaterpillar.pos.x, levitationCaterpillar.pos.y);
      levitationMultiplier++;
      levitationCaterpillar.pos.y = p->pos.y - 3;
      levitationCaterpillar.vel.y *= -1.5;
    }
    p->pos.x -= scrollSpeed;
    if (p->pos.x + p->width < 0) {
      p->isAlive = false;
    }
  }
}

void addGameLevitation() {
  addGame(levitationTitle, levitationDescription, levitationCharacters,
          levitationCharactersCount, &levitationOptions, false,
          &levitationUpdate);
}
