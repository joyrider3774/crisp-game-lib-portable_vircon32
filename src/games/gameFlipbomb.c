#include "../cglp.h"

int* flipbombTitle = "FLIPBOMB";
int* flipbombDescription = "[Tap] Flip";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] flipbombCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int flipbombCharactersCount = 0;

Options flipbombOptions = {100, 100, 2, false};

struct FlipbombExplosion {
  Vector pos;
  int t;
  float a;
  bool isAlive;
};
// Bomb-hit rate grows with difficulty (interval shrinks as ~1/difficulty, and
// missing a bomb ends the run so nearly every bomb becomes an explosion), but
// each explosion's 30-tick lifetime is fixed - concurrent count is ~0.75 *
// difficulty, unbounded over a long session, so 16 isn't enough headroom.
#define FLIPBOMB_MAX_EXPLOSION_COUNT 256
FlipbombExplosion[FLIPBOMB_MAX_EXPLOSION_COUNT] flipbombExplosions;
int flipbombExplosionIndex;

struct FlipbombBall {
  Vector pos;
  Vector vel;
  int s;
  bool isAlive;
};
#define FLIPBOMB_MAX_BALL_COUNT 16
FlipbombBall[FLIPBOMB_MAX_BALL_COUNT] flipbombBalls;
int flipbombBallIndex;
float flipbombBallAppTicks;

struct FlipbombBomb {
  Vector pos;
  Vector vel;
  bool isAlive;
};
#define FLIPBOMB_MAX_BOMB_COUNT 32
FlipbombBomb[FLIPBOMB_MAX_BOMB_COUNT] flipbombBombs;
int flipbombBombIndex;
float flipbombBombAppTicks;

int flipbombMultiplier;

void flipbombUpdate() {
  Collision scratch;
  if (!ticks) {
    INIT_UNALIVED_ARRAY_FAST(flipbombExplosions);
    flipbombExplosionIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(flipbombBalls);
    flipbombBallIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(flipbombBombs);
    flipbombBombIndex = 0;
    flipbombBombAppTicks = 0;
    flipbombBallAppTicks = 0;
    flipbombMultiplier = 1;
  }
  color = BLUE;
  thickness = 3;
  line(99, 75, 75, 86, &scratch);
  if (input.isJustPressed) {
    play(LASER);
    thickness = 3;
    line(70, 89, 50, 80, &scratch);
    flipbombMultiplier = 1;
  } else {
    thickness = 3;
    line(70, 89, 50, 98, &scratch);
  }
  color = PURPLE;
  FOR_EACH(flipbombExplosions, i) {
    ASSIGN_ARRAY_ITEM(flipbombExplosions, i, FlipbombExplosion, e);
    SKIP_IS_NOT_ALIVE(e);
    float a = e->a;
    float r;
    if (e->t < 20) {
      r = e->t * 0.5;
    } else {
      r = 20 * 0.5 - (e->t - 20);
    }
    float s;
    if (e->t == 0) {
      s = 10;
    } else {
      s = r + 3;
    }
    TIMES(5, k) {
      Vector p;
      p = e->pos;
      addWithAngle(&p, a, r);
      box(p.x, p.y, s, s, &scratch);
      a += CGLP_PI * 2 / 5;
    }
    e->t++;
    e->a += 0.2;
    if (e->t >= 30) {
      e->isAlive = false;
    }
  }
  flipbombBallAppTicks -= difficulty;
  if (flipbombBallAppTicks < 0) {
    ASSIGN_ARRAY_ITEM(flipbombBalls, flipbombBallIndex, FlipbombBall, nb);
    vectorSet(&nb->pos, 99, 70);
    vectorSet(&nb->vel, -1, 0.5);
    vectorMul(&nb->vel, difficulty);
    nb->s = 0;
    nb->isAlive = true;
    flipbombBallIndex = cgl_wrap(flipbombBallIndex + 1, 0, FLIPBOMB_MAX_BALL_COUNT);
    flipbombBallAppTicks = 60;
  }
  FOR_EACH(flipbombBalls, i) {
    ASSIGN_ARRAY_ITEM(flipbombBalls, i, FlipbombBall, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    if (b->s == 0) {
      color = BLUE;
    } else {
      color = CYAN;
    }
    if (b->s == 0) {
      if (b->pos.x < 70) {
        if (input.isJustPressed) {
          vectorSet(&b->vel, 2, -4);
          rotate(&b->vel, (b->pos.x - 70) * 0.06);
          b->s = 1;
        }
      } else if (b->pos.x < 50) {
        b->s = 1;
      }
    } else {
      particle(b->pos.x, b->pos.y, 1, vectorLength(&b->vel), vectorAngle(&b->vel) + CGLP_PI, 0.1);
    }
    box(b->pos.x, b->pos.y, 3, 3, &scratch);
    if (scratch.isColliding.rect[PURPLE]) {
      b->isAlive = false;
      continue;
    }
    if (!(b->pos.x >= 0 && b->pos.x <= 99 && b->pos.y >= 0 && b->pos.y <= 99)) {
      b->isAlive = false;
      continue;
    }
  }
  flipbombBombAppTicks -= difficulty;
  if (flipbombBombAppTicks < 0) {
    Vector p;
    vectorSet(&p, rnd(0, 80), 0);
    Vector v;
    vectorSet(&v, rnd(20, 70), 70);
    vectorAdd(&v, -p.x, -p.y);
    vectorMul(&v, 1.0 / (500 / rnd(1, difficulty)));
    ASSIGN_ARRAY_ITEM(flipbombBombs, flipbombBombIndex, FlipbombBomb, nbm);
    nbm->pos = p;
    nbm->vel = v;
    nbm->isAlive = true;
    flipbombBombIndex = cgl_wrap(flipbombBombIndex + 1, 0, FLIPBOMB_MAX_BOMB_COUNT);
    flipbombBombAppTicks += rnd(30, 50);
  }
  FOR_EACH(flipbombBombs, i) {
    ASSIGN_ARRAY_ITEM(flipbombBombs, i, FlipbombBomb, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    color = RED;
    box(b->pos.x, b->pos.y, 5, 5, &scratch);
    if (scratch.isColliding.rect[CYAN] || scratch.isColliding.rect[PURPLE]) {
      play(HIT);
      ASSIGN_ARRAY_ITEM(flipbombExplosions, flipbombExplosionIndex, FlipbombExplosion, ne);
      ne->pos = b->pos;
      ne->t = 0;
      ne->a = rnd(0, CGLP_PI) * RNDPM();
      ne->isAlive = true;
      flipbombExplosionIndex =
          cgl_wrap(flipbombExplosionIndex + 1, 0, FLIPBOMB_MAX_EXPLOSION_COUNT);
      color = PURPLE;
      particle(b->pos.x, b->pos.y, 16, 3, 0, CGLP_PI * 2);
      addScore(flipbombMultiplier, b->pos.x, b->pos.y);
      flipbombMultiplier++;
      b->isAlive = false;
      continue;
    }
    if (b->pos.y > 99 || scratch.isColliding.rect[BLUE]) {
      play(EXPLOSION);
      gameOver();
    }
  }
}

void addGameFlipbomb() {
  addGame(flipbombTitle, flipbombDescription, flipbombCharacters,
          flipbombCharactersCount, &flipbombOptions, false, &flipbombUpdate);
}
