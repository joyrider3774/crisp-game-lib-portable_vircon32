#include "../cglp.h"

int* orbitgrazerTitle = "ORBIT GRAZER";
int* orbitgrazerDescription = "[Tap]\n Reverse orbit";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] orbitgrazerCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int orbitgrazerCharactersCount = 0;

Options orbitgrazerOptions = {100, 100, 0, false};

struct OrbitgrazerTrailPoint {
  Vector pos;
  int life;
  bool isAlive;
};
#define ORBITGRAZER_PLAYER_TRAIL_MAX_COUNT 32
OrbitgrazerTrailPoint[ORBITGRAZER_PLAYER_TRAIL_MAX_COUNT] orbitgrazerPlayerTrail;
int orbitgrazerPlayerTrailIndex;

// Concurrent asteroid trail points ~ life(30)/spawnEvery(4) ~ 7.5 - headroom applied.
#define ORBITGRAZER_ASTEROID_TRAIL_MAX_COUNT 16
struct OrbitgrazerAsteroid {
  Vector pos;
  Vector vel;
  float size;
  float rotation;
  float rotSpeed;
  float breathPhase;
  OrbitgrazerTrailPoint[ORBITGRAZER_ASTEROID_TRAIL_MAX_COUNT] trail;
  int trailIndex;
  bool isAlive;
};
// Spawn interval and per-asteroid lifetime both scale as 1/speed, so
// concurrent count stays roughly constant (~4-5) regardless of difficulty -
// generous headroom applied anyway.
#define ORBITGRAZER_MAX_ASTEROID_COUNT 32
OrbitgrazerAsteroid[ORBITGRAZER_MAX_ASTEROID_COUNT] orbitgrazerAsteroids;
int orbitgrazerAsteroidIndex;

float orbitgrazerCenterX;
float orbitgrazerCenterY;
float orbitgrazerOrbitRadius;
float orbitgrazerOrbitAngle;
int orbitgrazerOrbitDir;
Vector orbitgrazerPlayerPos;
int orbitgrazerInvincibleTicks;
float orbitgrazerPlayerStretch;

void orbitgrazerAddPlayerTrailPoint(float x, float y) {
  ASSIGN_ARRAY_ITEM(orbitgrazerPlayerTrail, orbitgrazerPlayerTrailIndex, OrbitgrazerTrailPoint, t);
  t->pos.x = x;
  t->pos.y = y;
  t->life = 24;
  t->isAlive = true;
  orbitgrazerPlayerTrailIndex = cgl_wrap(orbitgrazerPlayerTrailIndex + 1, 0, ORBITGRAZER_PLAYER_TRAIL_MAX_COUNT);
}

void orbitgrazerAddAsteroidTrailPoint(OrbitgrazerAsteroid* a, float x, float y) {
  ASSIGN_ARRAY_ITEM(a->trail, a->trailIndex, OrbitgrazerTrailPoint, t);
  t->pos.x = x;
  t->pos.y = y;
  t->life = 30;
  t->isAlive = true;
  a->trailIndex = cgl_wrap(a->trailIndex + 1, 0, ORBITGRAZER_ASTEROID_TRAIL_MAX_COUNT);
}

void orbitgrazerUpdate() {
  Collision scratch;
  if (!ticks) {
    orbitgrazerCenterX = 50;
    orbitgrazerCenterY = 50;
    orbitgrazerOrbitRadius = 25;
    orbitgrazerOrbitAngle = 0;
    orbitgrazerOrbitDir = 1;
    vectorSet(&orbitgrazerPlayerPos, 0, 0);
    INIT_UNALIVED_ARRAY_FAST(orbitgrazerAsteroids);
    orbitgrazerAsteroidIndex = 0;
    orbitgrazerInvincibleTicks = 0;
    orbitgrazerPlayerStretch = 1;
    INIT_UNALIVED_ARRAY_FAST(orbitgrazerPlayerTrail);
    orbitgrazerPlayerTrailIndex = 0;
  }

  if (input.isJustPressed) {
    orbitgrazerOrbitDir *= -1;
    play(SELECT);
    orbitgrazerPlayerStretch = 1.6;
    color = CYAN;
    particle(orbitgrazerPlayerPos.x, orbitgrazerPlayerPos.y, 8, 1.5,
             orbitgrazerOrbitAngle - (CGLP_PI / 2) * orbitgrazerOrbitDir, CGLP_PI / 4);
  }

  orbitgrazerPlayerStretch += (1 - orbitgrazerPlayerStretch) * 0.15;

  float orbitSpeed = 0.5 * sqrt(difficulty);
  orbitgrazerOrbitAngle += (orbitSpeed * orbitgrazerOrbitDir) / (orbitgrazerOrbitRadius + 0.1);

  orbitgrazerPlayerPos.x = orbitgrazerCenterX + cos(orbitgrazerOrbitAngle) * orbitgrazerOrbitRadius;
  orbitgrazerPlayerPos.y = orbitgrazerCenterY + sin(orbitgrazerOrbitAngle) * orbitgrazerOrbitRadius;

  if (ticks % 3 == 0) {
    orbitgrazerAddPlayerTrailPoint(orbitgrazerPlayerPos.x, orbitgrazerPlayerPos.y);
  }
  FOR_EACH(orbitgrazerPlayerTrail, pti) {
    ASSIGN_ARRAY_ITEM(orbitgrazerPlayerTrail, pti, OrbitgrazerTrailPoint, t);
    SKIP_IS_NOT_ALIVE(t);
    t->life--;
    if (t->life <= 0) {
      t->isAlive = false;
    }
  }

  float spawnRateF = floor(50 / sqrt(difficulty));
  int spawnRate = (int)spawnRateF;
  // Vircon32 port note: guard against a zero interval (modulo by zero
  // hard-traps here) - upstream JS never guards this either, but this
  // dialect can't get away with it.
  if (spawnRate < 1) {
    spawnRate = 1;
  }
  if (ticks % spawnRate == 0) {
    float spawnAngle = rnd(0, CGLP_PI * 2);
    float dist = 60;
    float ax = orbitgrazerCenterX + cos(spawnAngle) * dist;
    float ay = orbitgrazerCenterY + sin(spawnAngle) * dist;

    float playerAngle = atan2(orbitgrazerPlayerPos.y - orbitgrazerCenterY, orbitgrazerPlayerPos.x - orbitgrazerCenterX);
    float targetAngle = playerAngle + orbitgrazerOrbitDir * rnd(0.2, 0.6);
    float targetX = orbitgrazerCenterX + cos(targetAngle) * orbitgrazerOrbitRadius;
    float targetY = orbitgrazerCenterY + sin(targetAngle) * orbitgrazerOrbitRadius;

    float speed = 0.5 * sqrt(difficulty);
    float dx = targetX - ax;
    float dy = targetY - ay;
    float len = sqrt(dx * dx + dy * dy);
    float distToPlayerX = ax - orbitgrazerPlayerPos.x;
    float distToPlayerY = ay - orbitgrazerPlayerPos.y;
    float distToPlayer = sqrt(distToPlayerX * distToPlayerX + distToPlayerY * distToPlayerY);
    if (len > 0 && distToPlayer > 50) {
      ASSIGN_ARRAY_ITEM(orbitgrazerAsteroids, orbitgrazerAsteroidIndex, OrbitgrazerAsteroid, na);
      na->pos.x = ax;
      na->pos.y = ay;
      na->vel.x = (dx / len) * speed;
      na->vel.y = (dy / len) * speed;
      na->size = rnd(6, 9);
      na->rotation = rnd(0, CGLP_PI * 2);
      na->rotSpeed = rnd(-0.1, 0.1);
      na->breathPhase = rnd(0, CGLP_PI * 2);
      INIT_UNALIVED_ARRAY_FAST(na->trail);
      na->trailIndex = 0;
      na->isAlive = true;
      orbitgrazerAsteroidIndex = cgl_wrap(orbitgrazerAsteroidIndex + 1, 0, ORBITGRAZER_MAX_ASTEROID_COUNT);
    }
  }
  orbitgrazerOrbitRadius = clamp(orbitgrazerOrbitRadius + 0.05, 0, 40);

  color = LIGHT_BLACK;
  thickness = 1;
  arc(orbitgrazerCenterX, orbitgrazerCenterY, orbitgrazerOrbitRadius, 0, CGLP_PI * 2, &scratch);

  float centerPulse = 1 + sin(ticks * 0.1) * 0.1;
  color = YELLOW;
  box(orbitgrazerCenterX, orbitgrazerCenterY, 4 * centerPulse, 4 * centerPulse, &scratch);

  FOR_EACH(orbitgrazerPlayerTrail, pti2) {
    ASSIGN_ARRAY_ITEM(orbitgrazerPlayerTrail, pti2, OrbitgrazerTrailPoint, t2);
    SKIP_IS_NOT_ALIVE(t2);
    float alpha = t2->life / 24.0;
    if (alpha > 0.5) {
      color = LIGHT_CYAN;
    } else {
      color = LIGHT_BLACK;
    }
    box(t2->pos.x, t2->pos.y, 8 * alpha, 8 * alpha, &scratch);
  }

  orbitgrazerInvincibleTicks--;
  if (orbitgrazerInvincibleTicks > 0) {
    color = BLUE;
  } else {
    color = CYAN;
  }

  float stretchW = 6 / orbitgrazerPlayerStretch;
  float stretchH = 6 * orbitgrazerPlayerStretch;

  Collision playerCol;
  box(orbitgrazerPlayerPos.x, orbitgrazerPlayerPos.y, stretchW, stretchH, &playerCol);
  if (playerCol.isColliding.rect[YELLOW]) {
    gameOver();
  }

  FOR_EACH(orbitgrazerAsteroids, ai) {
    ASSIGN_ARRAY_ITEM(orbitgrazerAsteroids, ai, OrbitgrazerAsteroid, a);
    SKIP_IS_NOT_ALIVE(a);
    a->pos.x += a->vel.x;
    a->pos.y += a->vel.y;
    a->rotation += a->rotSpeed;
    a->breathPhase += 0.15;

    float breathScale = 1 + sin(a->breathPhase) * 0.08;

    if (ticks % 4 == 0) {
      orbitgrazerAddAsteroidTrailPoint(a, a->pos.x, a->pos.y);
    }
    FOR_EACH(a->trail, ti) {
      ASSIGN_ARRAY_ITEM(a->trail, ti, OrbitgrazerTrailPoint, tp);
      SKIP_IS_NOT_ALIVE(tp);
      tp->life--;
      if (tp->life <= 0) {
        tp->isAlive = false;
      }
    }

    float dx2 = orbitgrazerPlayerPos.x - a->pos.x;
    float dy2 = orbitgrazerPlayerPos.y - a->pos.y;
    float dist2 = sqrt(dx2 * dx2 + dy2 * dy2);

    float grazeThreshold = a->size / 2 + 15;

    bool grazed = false;
    if (orbitgrazerInvincibleTicks < 0 && dist2 < grazeThreshold) {
      addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      play(CLICK);
      grazed = true;
      color = PURPLE;
      if (ticks % 3 == 0) {
        float oppositeDir = atan2(dy2, dx2) + CGLP_PI;
        particle(a->pos.x, a->pos.y, 3, 1, oppositeDir, CGLP_PI / 4);
      }
    }

    FOR_EACH(a->trail, ti2) {
      ASSIGN_ARRAY_ITEM(a->trail, ti2, OrbitgrazerTrailPoint, tp2);
      SKIP_IS_NOT_ALIVE(tp2);
      float alpha2 = tp2->life / 30.0;
      color = LIGHT_PURPLE;
      box(tp2->pos.x, tp2->pos.y, a->size * 0.7 * alpha2, a->size * 0.7 * alpha2, &scratch);
    }

    if (grazed) {
      color = RED;
    } else {
      color = PURPLE;
    }
    float drawSize = a->size * breathScale;
    thickness = drawSize * 0.7;
    Collision barCol;
    bar(a->pos.x, a->pos.y, drawSize, a->rotation, &barCol);

    if (barCol.isColliding.rect[BLUE]) {
      color = PURPLE;
      particle(a->pos.x, a->pos.y, 15, 2, 0, CGLP_PI * 2);
      a->isAlive = false;
      continue;
    }
    if (barCol.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      color = RED;
      particle(a->pos.x, a->pos.y, 20, 3, 0, CGLP_PI * 2);
      orbitgrazerOrbitRadius = clamp(orbitgrazerOrbitRadius - 20, 0, 40);
      orbitgrazerInvincibleTicks = 60;
    }

    if (a->pos.x < -10 || a->pos.x > 110 || a->pos.y < -10 || a->pos.y > 110) {
      a->isAlive = false;
    }
  }
}

void addGameOrbitgrazer() {
  addGame(orbitgrazerTitle, orbitgrazerDescription, orbitgrazerCharacters,
          orbitgrazerCharactersCount, &orbitgrazerOptions, false, &orbitgrazerUpdate);
}
