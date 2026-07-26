#include "../cglp.h"

int* wavybirdTitle = "WAVY BIRD";
int* wavybirdDescription = "[Tap] Flap";
int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] wavybirdCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int wavybirdCharactersCount = 1;

Options wavybirdOptions = {100, 100, 3, false};

#define WAVYBIRD_GRAVITY 0.1
#define WAVYBIRD_JUMP_FORCE 1.5
#define WAVYBIRD_SHOCKWAVE_SPEED 2
#define WAVYBIRD_MAX_SHOCKWAVE_RADIUS 30
#define WAVYBIRD_PILLAR_WIDTH 8
#define WAVYBIRD_MIN_PILLAR_HEIGHT 20
#define WAVYBIRD_MAX_PILLAR_HEIGHT 60

struct WavybirdBird {
  Vector pos;
  Vector vel;
  Vector size;
  float angle;
};
WavybirdBird wavybirdBird;

struct WavybirdPillar {
  Vector pos;
  Vector size;
  bool isAlive;
};
#define WAVYBIRD_MAX_PILLAR_COUNT 64
WavybirdPillar[WAVYBIRD_MAX_PILLAR_COUNT] wavybirdPillars;
int wavybirdPillarIndex;

struct WavybirdShockwave {
  Vector pos;
  float radius;
  float maxRadius;
  float angle;
  bool isAlive;
};
#define WAVYBIRD_MAX_SHOCKWAVE_COUNT 16
WavybirdShockwave[WAVYBIRD_MAX_SHOCKWAVE_COUNT] wavybirdShockwaves;
int wavybirdShockwaveIndex;

float wavybirdNextPillarDist;
float wavybirdMultiplier;

void wavybirdUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&wavybirdBird.pos, 20, 50);
    vectorSet(&wavybirdBird.vel, 0, 0);
    vectorSet(&wavybirdBird.size, 5, 3);
    wavybirdBird.angle = 0;
    INIT_UNALIVED_ARRAY_FAST(wavybirdPillars);
    wavybirdPillarIndex = 0;
    wavybirdNextPillarDist = 0;
    INIT_UNALIVED_ARRAY_FAST(wavybirdShockwaves);
    wavybirdShockwaveIndex = 0;
    wavybirdMultiplier = 1;
  }

  color = PURPLE;
  FOR_EACH(wavybirdShockwaves, si) {
    ASSIGN_ARRAY_ITEM(wavybirdShockwaves, si, WavybirdShockwave, s);
    SKIP_IS_NOT_ALIVE(s);
    s->radius += WAVYBIRD_SHOCKWAVE_SPEED * difficulty;
    thickness = 3;
    arc(s->pos.x, s->pos.y, s->radius, s->angle - 0.3, s->angle + 0.3, &scratch);
    if (s->radius >= s->maxRadius) {
      s->isAlive = false;
      continue;
    }
  }

  wavybirdBird.pos.y += wavybirdBird.vel.y * difficulty;
  wavybirdBird.vel.y += WAVYBIRD_GRAVITY;
  wavybirdBird.angle += (1.2 - wavybirdBird.angle) * 0.02 * difficulty;
  if (input.isJustPressed) {
    play(CLICK);
    ASSIGN_ARRAY_ITEM(wavybirdShockwaves, wavybirdShockwaveIndex, WavybirdShockwave, ns);
    vectorSet(&ns->pos, wavybirdBird.pos.x, wavybirdBird.pos.y);
    ns->radius = 0;
    ns->maxRadius = WAVYBIRD_MAX_SHOCKWAVE_RADIUS;
    ns->angle = wavybirdBird.angle - 0.4 * difficulty;
    ns->isAlive = true;
    wavybirdShockwaveIndex = cgl_wrap(wavybirdShockwaveIndex + 1, 0, WAVYBIRD_MAX_SHOCKWAVE_COUNT);
    wavybirdBird.vel.y = -WAVYBIRD_JUMP_FORCE;
    wavybirdBird.angle -= 0.6 * difficulty;
  }
  wavybirdBird.angle = clamp(wavybirdBird.angle, -1.2, 1.2);
  if (wavybirdBird.pos.y < 0 || wavybirdBird.pos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  color = RED;
  thickness = wavybirdBird.size.y;
  bar(wavybirdBird.pos.x, wavybirdBird.pos.y, wavybirdBird.size.x, wavybirdBird.angle, &scratch);

  color = CYAN;
  FOR_EACH(wavybirdPillars, pillarIdx) {
    ASSIGN_ARRAY_ITEM(wavybirdPillars, pillarIdx, WavybirdPillar, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.x -= difficulty;
    box(p->pos.x, p->pos.y, p->size.x, p->size.y, &scratch);
    if (scratch.isColliding.rect[PURPLE]) {
      play(POWER_UP);
      addScore(floor(wavybirdMultiplier), p->pos.x, p->pos.y);
      wavybirdMultiplier += 1;
      p->isAlive = false;
      continue;
    } else if (scratch.isColliding.rect[RED]) {
      play(EXPLOSION);
      gameOver();
    }
    if (p->pos.x < -WAVYBIRD_PILLAR_WIDTH) {
      p->isAlive = false;
      continue;
    }
  }

  wavybirdNextPillarDist -= difficulty;
  if (wavybirdNextPillarDist < 0) {
    play(LASER);
    float height = rnd(WAVYBIRD_MIN_PILLAR_HEIGHT, WAVYBIRD_MAX_PILLAR_HEIGHT);
    float y = rnd(0, 110 - height);
    int blockCount = floor(height / WAVYBIRD_PILLAR_WIDTH);
    TIMES(blockCount, bi) {
      ASSIGN_ARRAY_ITEM(wavybirdPillars, wavybirdPillarIndex, WavybirdPillar, np);
      vectorSet(&np->pos, 100 + WAVYBIRD_PILLAR_WIDTH, y);
      vectorSet(&np->size, WAVYBIRD_PILLAR_WIDTH, WAVYBIRD_PILLAR_WIDTH);
      np->isAlive = true;
      wavybirdPillarIndex = cgl_wrap(wavybirdPillarIndex + 1, 0, WAVYBIRD_MAX_PILLAR_COUNT);
      y += WAVYBIRD_PILLAR_WIDTH;
    }
    wavybirdNextPillarDist += rnd(10, 40);
  }
  wavybirdMultiplier = clamp(wavybirdMultiplier - 0.03 * difficulty, 1, 99);
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar((int)floor(wavybirdMultiplier)));
  text(multText, 2, 9, &scratch);
}

void addGameWavybird() {
  addGame(wavybirdTitle, wavybirdDescription, wavybirdCharacters, wavybirdCharactersCount,
          &wavybirdOptions, false, &wavybirdUpdate);
}
