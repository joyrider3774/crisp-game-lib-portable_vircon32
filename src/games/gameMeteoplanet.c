#include "../cglp.h"

int* meteoplanetTitle = "METEO PLANET";
int* meteoplanetDescription = "[Tap] Move";

int[3][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] meteoplanetCharacters = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
        "      ",
    },
    {
        " lll  ",
        "l l l ",
        "l lll ",
        "ll ll ",
        " lll  ",
    },
};
int meteoplanetCharactersCount = 3;

Options meteoplanetOptions = {100, 100, 6, true};

struct MeteoplanetFalling {
  float dist;
  float angle;
  int type;
  bool isAlive;
};
#define METEOPLANET_MAX_FALLING_COUNT 64
MeteoplanetFalling[METEOPLANET_MAX_FALLING_COUNT] meteoplanetFallings;
int meteoplanetFallingIndex;
float meteoplanetNextFallingsTicks;
float meteoplanetNextFallingAngle;
float meteoplanetAngle;
float meteoplanetTargetAngle;
float meteoplanetAnimTicks;

struct MeteoplanetStar {
  float dist;
  float angle;
};
#define METEOPLANET_STAR_COUNT 24
MeteoplanetStar[METEOPLANET_STAR_COUNT] meteoplanetStars;

void meteoplanetUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(meteoplanetFallings);
    meteoplanetFallings[0].dist = 53;
    meteoplanetFallings[0].angle = rndi(0, 4) * CGLP_PI_2;
    meteoplanetFallings[0].type = 0;
    meteoplanetFallings[0].isAlive = true;
    meteoplanetFallingIndex = 1;
    meteoplanetNextFallingsTicks = 0;
    meteoplanetNextFallingAngle = rndi(1, 4);
    meteoplanetAngle = 0;
    meteoplanetTargetAngle = 0;
    meteoplanetAnimTicks = 0;
    TIMES(METEOPLANET_STAR_COUNT, i) {
      meteoplanetStars[i].dist = rnd(10, 70);
      meteoplanetStars[i].angle = rnd(0, CGLP_PI * 2);
    }
  }
  float sd = sqrt(difficulty);
  color = LIGHT_BLACK;
  TIMES(METEOPLANET_STAR_COUNT, i) {
    MeteoplanetStar* s = &meteoplanetStars[i];
    Vector sp;
    vectorSet(&sp, 50, 50);
    addWithAngle(&sp, s->angle - meteoplanetAngle, s->dist);
    box(sp.x, sp.y, 1, 1, &scratch);
  }
  color = BLACK;
  if (input.isJustPressed) {
    play(SELECT);
    meteoplanetTargetAngle += CGLP_PI_2;
  }
  if (meteoplanetAngle < meteoplanetTargetAngle) {
    meteoplanetAngle += 0.3 * sd;
    if (meteoplanetAngle > meteoplanetTargetAngle) {
      meteoplanetAngle = meteoplanetTargetAngle;
      if (meteoplanetAngle > CGLP_PI * 2.2) {
        meteoplanetAngle = CGLP_PI_2;
        meteoplanetTargetAngle = CGLP_PI_2;
      }
    }
    meteoplanetAnimTicks += sd;
  }
  int[2] pc;
  pc[0] = 'a' + ((int)(meteoplanetAnimTicks / 3) % 2);
  pc[1] = 0;
  character(pc, 50, 42, &scratch);
  thickness = 2;
  arc(50, 50, 3, -meteoplanetAngle + CGLP_PI * 0.2, -meteoplanetAngle + CGLP_PI * 2.2, &scratch);
  meteoplanetNextFallingsTicks--;
  if (meteoplanetNextFallingsTicks < 0) {
    int cc = rndi(0, 6);
    float dist = 70;
    float fallAngle = meteoplanetNextFallingAngle * CGLP_PI_2;
    TIMES(11, i) {
      int type = abs(i - 5);
      if (type <= cc) {
        int newType;
        if (type == 0) {
          newType = 0;
        } else {
          newType = cc - type + 1;
        }
        ASSIGN_ARRAY_ITEM(meteoplanetFallings, meteoplanetFallingIndex, MeteoplanetFalling, nf);
        nf->dist = dist;
        nf->angle = fallAngle;
        nf->type = newType;
        nf->isAlive = true;
        meteoplanetFallingIndex =
            cgl_wrap(meteoplanetFallingIndex + 1, 0, METEOPLANET_MAX_FALLING_COUNT);
      }
      dist += 6;
    }
    meteoplanetNextFallingsTicks = rnd(30, 50) / sqrt(sd);
    meteoplanetNextFallingAngle += rndi(1, 4);
  }
  FOR_EACH(meteoplanetFallings, i) {
    ASSIGN_ARRAY_ITEM(meteoplanetFallings, i, MeteoplanetFalling, f);
    SKIP_IS_NOT_ALIVE(f);
    f->dist -= 0.5 * sd;
    Vector fp;
    vectorSet(&fp, 50, 50);
    addWithAngle(&fp, f->angle - meteoplanetAngle, f->dist);
    if (f->type == 0) {
      color = BLACK;
      character("c", fp.x, fp.y, &scratch);
      if (scratch.isColliding.character['a'] || scratch.isColliding.character['b']) {
        play(EXPLOSION);
        gameOver();
      }
    } else {
      color = YELLOW;
      box(fp.x, fp.y, f->type, f->type, &scratch);
      if (scratch.isColliding.character['a'] || scratch.isColliding.character['b']) {
        play(POWER_UP);
        addScore(f->type, fp.x, fp.y);
        f->isAlive = false;
        continue;
      }
    }
    if (f->dist < 5) {
      if (f->type == 0) {
        play(HIT);
        particle(fp.x, fp.y, 9, 1, 0, CGLP_PI * 2);
      }
      f->isAlive = false;
      continue;
    }
  }
}

void addGameMeteoplanet() {
  addGame(meteoplanetTitle, meteoplanetDescription, meteoplanetCharacters,
          meteoplanetCharactersCount, &meteoplanetOptions, false,
          &meteoplanetUpdate);
}
