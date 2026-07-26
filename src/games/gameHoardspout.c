#include "../cglp.h"

int* hoardspoutTitle = "HOARD SPOUT";
int* hoardspoutDescription = "Gather same color sparks\n[Hold]\n Thrust";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] hoardspoutCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int hoardspoutCharactersCount = 0;

Options hoardspoutOptions = {100, 100, 3, true};

#define HOARDSPOUT_FIELD_WIDTH 100
#define HOARDSPOUT_FIELD_HEIGHT 100

struct HoardspoutPlayer {
  Vector pos;
  float angle;
  float speed;
  int color;
  float size;
};
HoardspoutPlayer hoardspoutPlayer;

struct HoardspoutSpark {
  Vector pos;
  Vector vel;
  int color;
  float size;
  bool isAlive;
};
#define HOARDSPOUT_MAX_SPARK_COUNT 64
HoardspoutSpark[HOARDSPOUT_MAX_SPARK_COUNT] hoardspoutSparks;
int hoardspoutSparkIndex;
float hoardspoutNextSparkTicks;

void hoardspoutUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&hoardspoutPlayer.pos, 50, 50);
    hoardspoutPlayer.angle = rnd(0, 2 * CGLP_PI);
    hoardspoutPlayer.speed = 1;
    hoardspoutPlayer.color = RED;
    hoardspoutPlayer.size = 3;
    INIT_UNALIVED_ARRAY_FAST(hoardspoutSparks);
    hoardspoutSparkIndex = 0;
    hoardspoutNextSparkTicks = 0;
  }
  addWithAngle(&hoardspoutPlayer.pos, hoardspoutPlayer.angle, hoardspoutPlayer.speed);
  if (input.isJustReleased) {
    play(LASER);
    if (hoardspoutPlayer.color == RED) {
      hoardspoutPlayer.color = BLUE;
    } else {
      hoardspoutPlayer.color = RED;
    }
  }
  if (input.isPressed) {
    if (hoardspoutPlayer.speed < 2) {
      hoardspoutPlayer.speed += 0.01 * difficulty;
    }
    hoardspoutPlayer.angle += 0.01 * difficulty;
    hoardspoutNextSparkTicks -= sqrt(difficulty);
  } else {
    hoardspoutPlayer.speed += (difficulty - hoardspoutPlayer.speed) * 0.1;
  }
  hoardspoutNextSparkTicks -= 0.1 * sqrt(difficulty);
  if (hoardspoutNextSparkTicks < 0) {
    Vector velocity;
    vectorSet(&velocity, 1, 0);
    rotate(&velocity, hoardspoutPlayer.angle + CGLP_PI + rnd(0, 0.2) * RNDPM());
    ASSIGN_ARRAY_ITEM(hoardspoutSparks, hoardspoutSparkIndex, HoardspoutSpark, ns);
    ns->pos = hoardspoutPlayer.pos;
    ns->vel = velocity;
    if (hoardspoutPlayer.color == RED) {
      ns->color = BLUE;
    } else {
      ns->color = RED;
    }
    ns->size = 1;
    ns->isAlive = true;
    hoardspoutSparkIndex = cgl_wrap(hoardspoutSparkIndex + 1, 0, HOARDSPOUT_MAX_SPARK_COUNT);
    hoardspoutNextSparkTicks = 9;
  }
  if (hoardspoutPlayer.pos.x < 0) {
    hoardspoutPlayer.angle = CGLP_PI - hoardspoutPlayer.angle;
    hoardspoutPlayer.pos.x = 0;
  }
  if (hoardspoutPlayer.pos.x > HOARDSPOUT_FIELD_WIDTH) {
    hoardspoutPlayer.angle = -CGLP_PI - hoardspoutPlayer.angle;
    hoardspoutPlayer.pos.x = HOARDSPOUT_FIELD_WIDTH;
  }
  if (hoardspoutPlayer.pos.y < 0) {
    hoardspoutPlayer.angle = -hoardspoutPlayer.angle;
    hoardspoutPlayer.pos.y = 0;
  }
  if (hoardspoutPlayer.pos.y > HOARDSPOUT_FIELD_HEIGHT) {
    hoardspoutPlayer.angle = -hoardspoutPlayer.angle;
    hoardspoutPlayer.pos.y = HOARDSPOUT_FIELD_HEIGHT;
  }
  hoardspoutPlayer.size -= 0.002 * difficulty;
  if (hoardspoutPlayer.size < 1) {
    play(EXPLOSION);
    gameOver();
  } else if (hoardspoutPlayer.size > 9) {
    hoardspoutPlayer.size = 9;
  }
  color = hoardspoutPlayer.color;
  thickness = hoardspoutPlayer.size * 1.5;
  barCenterPosRatio = 0.5;
  bar(hoardspoutPlayer.pos.x, hoardspoutPlayer.pos.y, hoardspoutPlayer.size * 2.5,
      hoardspoutPlayer.angle, &scratch);
  color = BLACK;
  thickness = hoardspoutPlayer.size;
  bar(hoardspoutPlayer.pos.x, hoardspoutPlayer.pos.y, hoardspoutPlayer.size + 3,
      hoardspoutPlayer.angle, &scratch);
  FOR_EACH(hoardspoutSparks, i) {
    ASSIGN_ARRAY_ITEM(hoardspoutSparks, i, HoardspoutSpark, s);
    SKIP_IS_NOT_ALIVE(s);
    vectorAdd(&s->pos, s->vel.x, s->vel.y);
    if (s->size < 5) {
      if (s->color == RED) {
        color = LIGHT_RED;
      } else {
        color = LIGHT_BLUE;
      }
    } else {
      color = s->color;
    }
    if (s->size < 5) {
      s->size += 0.2 * difficulty;
    } else {
      s->size += 0.01 * difficulty;
    }
    if (s->size > 9) {
      particle(s->pos.x, s->pos.y, 9, vectorLength(&s->vel), vectorAngle(&s->vel), 0.4);
      s->isAlive = false;
      continue;
    }
    box(s->pos.x, s->pos.y, s->size, s->size, &scratch);
    if (scratch.isColliding.rect[BLACK] && s->size >= 5) {
      if (s->color == hoardspoutPlayer.color) {
        play(POWER_UP);
        addScore(floor(hoardspoutPlayer.size), s->pos.x, s->pos.y);
        hoardspoutPlayer.size += 0.2 * difficulty;
        s->isAlive = false;
        continue;
      } else {
        play(HIT);
        particle(s->pos.x, s->pos.y, 9, 2, 0, CGLP_PI * 2);
        hoardspoutPlayer.size -= 0.4 * difficulty;
        s->isAlive = false;
        continue;
      }
    }
    if (s->pos.x < 0) {
      s->vel.x *= -1;
      s->pos.x = 0;
    }
    if (s->pos.x > HOARDSPOUT_FIELD_WIDTH) {
      s->vel.x *= -1;
      s->pos.x = HOARDSPOUT_FIELD_WIDTH;
    }
    if (s->pos.y < 0) {
      s->vel.y *= -1;
      s->pos.y = 0;
    }
    if (s->pos.y > HOARDSPOUT_FIELD_HEIGHT) {
      s->vel.y *= -1;
      s->pos.y = HOARDSPOUT_FIELD_HEIGHT;
    }
  }
}

void addGameHoardspout() {
  addGame(hoardspoutTitle, hoardspoutDescription, hoardspoutCharacters,
          hoardspoutCharactersCount, &hoardspoutOptions, false,
          &hoardspoutUpdate);
}
