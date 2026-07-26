#include "../cglp.h"

int* revolveaTitle = "REVOLVE A";
int* revolveaDescription = "[Tap]\n Go forward";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] revolveaCharacters = {{
    " rrr  ",
    "rrRrr ",
    "rRrLr ",
    "rrLLr ",
    " rrr  ",
}};
int revolveaCharactersCount = 1;

Options revolveaOptions = {100, 100, 2, false};

#define REVOLVEA_LINE_DIST 30

struct RevolveaEnemy {
  Vector pos;
  Vector vel;
  bool isRemoved;
  bool isAlive;
};
// Spawn interval ~ 1/difficulty but enemy speed (hence lifetime) only scales
// ~sqrt(difficulty), so concurrent enemies grow ~sqrt(difficulty) - already
// tight within the first few minutes of play, not just at extreme difficulty.
#define REVOLVEA_MAX_ENEMY_COUNT 512
RevolveaEnemy[REVOLVEA_MAX_ENEMY_COUNT] revolveaEnemies;
int revolveaEnemyIndex;
float revolveaNextEnemyTicks;
int revolveaMultiplier;

struct RevolveaArrow {
  Vector pos;
  Vector vel;
  float angle;
};
RevolveaArrow revolveaArrow;

void revolveaRemoveAroundEnemy(float px, float py) {
  FOR_EACH(revolveaEnemies, i) {
    ASSIGN_ARRAY_ITEM(revolveaEnemies, i, RevolveaEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (!e->isRemoved && distanceTo(&e->pos, px, py) < REVOLVEA_LINE_DIST) {
      e->isRemoved = true;
      revolveaRemoveAroundEnemy(e->pos.x, e->pos.y);
    }
  }
}

void revolveaUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&revolveaArrow.pos, 50, 50);
    vectorSet(&revolveaArrow.vel, 1, 0);
    rotate(&revolveaArrow.vel, CGLP_PI / 4);
    revolveaArrow.angle = -CGLP_PI / 2;
    INIT_UNALIVED_ARRAY_FAST(revolveaEnemies);
    revolveaEnemyIndex = 0;
    revolveaNextEnemyTicks = 0;
    revolveaMultiplier = 1;
  }
  revolveaNextEnemyTicks--;
  if (revolveaNextEnemyTicks < 0) {
    float px = rnd(0, 99);
    float py;
    if (rnd(0, 1) < 0.5) {
      py = -3;
    } else {
      py = 103;
    }
    if (rnd(0, 1) < 0.5) {
      float tmp = px;
      px = py;
      py = tmp;
    }
    ASSIGN_ARRAY_ITEM(revolveaEnemies, revolveaEnemyIndex, RevolveaEnemy, ne);
    vectorSet(&ne->pos, px, py);
    float velAngle = angleTo(&ne->pos, rnd(10, 90), rnd(10, 90));
    vectorSet(&ne->vel, rnd(1, sqrt(difficulty)) * 0.3, 0);
    rotate(&ne->vel, velAngle);
    ne->isRemoved = false;
    ne->isAlive = true;
    revolveaEnemyIndex = cgl_wrap(revolveaEnemyIndex + 1, 0, REVOLVEA_MAX_ENEMY_COUNT);
    revolveaNextEnemyTicks = rnd(30, 40) / difficulty;
  }
  revolveaMultiplier = 1;
  FOR_EACH(revolveaEnemies, i) {
    ASSIGN_ARRAY_ITEM(revolveaEnemies, i, RevolveaEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    color = GREEN;
    if (e->isRemoved) {
      particle(e->pos.x, e->pos.y, 16, 1, 0, CGLP_PI * 2);
      addScore(revolveaMultiplier, e->pos.x, e->pos.y);
      revolveaMultiplier++;
      e->isAlive = false;
      continue;
    }
    vectorAdd(&e->pos, e->vel.x, e->vel.y);
    FOR_EACH(revolveaEnemies, j) {
      ASSIGN_ARRAY_ITEM(revolveaEnemies, j, RevolveaEnemy, ae);
      SKIP_IS_NOT_ALIVE(ae);
      if (i == j || distanceTo(&e->pos, ae->pos.x, ae->pos.y) >= REVOLVEA_LINE_DIST) {
        continue;
      }
      thickness = 3;
      line(e->pos.x, e->pos.y, ae->pos.x, ae->pos.y, &scratch);
    }
    bool eInRect =
        e->pos.x >= -5 && e->pos.x < 105 && e->pos.y >= -5 && e->pos.y < 105;
    e->isAlive = eInRect;
  }
  color = BLACK;
  FOR_EACH(revolveaEnemies, i) {
    ASSIGN_ARRAY_ITEM(revolveaEnemies, i, RevolveaEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    character("a", e->pos.x, e->pos.y, &scratch);
  }
  if (input.isJustPressed) {
    play(LASER);
    vectorSet(&revolveaArrow.vel, 1, 0);
    rotate(&revolveaArrow.vel, revolveaArrow.angle);
  }
  if ((revolveaArrow.pos.x < 3 && revolveaArrow.vel.x < 0) ||
      (revolveaArrow.pos.x > 97 && revolveaArrow.vel.x > 0)) {
    revolveaArrow.vel.x *= -1;
  }
  if ((revolveaArrow.pos.y < 3 && revolveaArrow.vel.y < 0) ||
      (revolveaArrow.pos.y > 97 && revolveaArrow.vel.y > 0)) {
    revolveaArrow.vel.y *= -1;
  }
  vectorAdd(&revolveaArrow.pos, revolveaArrow.vel.x * sqrt(difficulty) * 0.4,
            revolveaArrow.vel.y * sqrt(difficulty) * 0.4);
  revolveaArrow.angle += 0.08 * sqrt(difficulty);
  if (input.isJustPressed) {
    color = RED;
  } else {
    color = BLUE;
  }
  Vector p;
  vectorSet(&p, revolveaArrow.pos.x, revolveaArrow.pos.y);
  addWithAngle(&p, revolveaArrow.angle, 2);
  Vector q1;
  vectorSet(&q1, revolveaArrow.pos.x, revolveaArrow.pos.y);
  addWithAngle(&q1, revolveaArrow.angle + CGLP_PI, 2);
  thickness = 2;
  Collision c1;
  line(p.x, p.y, q1.x, q1.y, &c1);
  Vector q2;
  vectorSet(&q2, revolveaArrow.pos.x, revolveaArrow.pos.y);
  addWithAngle(&q2, revolveaArrow.angle + CGLP_PI / 2, 2);
  thickness = 2;
  Collision c2;
  line(p.x, p.y, q2.x, q2.y, &c2);
  Vector q3;
  vectorSet(&q3, revolveaArrow.pos.x, revolveaArrow.pos.y);
  addWithAngle(&q3, revolveaArrow.angle - CGLP_PI / 2, 2);
  thickness = 2;
  Collision c3;
  line(p.x, p.y, q3.x, q3.y, &c3);
  bool hitChar = c1.isColliding.character['a'] || c2.isColliding.character['a'] ||
                 c3.isColliding.character['a'];
  bool hitGreen = c1.isColliding.rect[GREEN] || c2.isColliding.rect[GREEN] ||
                  c3.isColliding.rect[GREEN];
  if (hitChar) {
    play(EXPLOSION);
    gameOver();
  } else if (hitGreen) {
    play(POWER_UP);
    revolveaRemoveAroundEnemy(revolveaArrow.pos.x, revolveaArrow.pos.y);
  }
}

void addGameRevolvea() {
  addGame(revolveaTitle, revolveaDescription, revolveaCharacters,
          revolveaCharactersCount, &revolveaOptions, false, &revolveaUpdate);
}
