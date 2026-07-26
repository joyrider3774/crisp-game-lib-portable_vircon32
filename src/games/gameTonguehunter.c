#include "../cglp.h"

int* tonguehunterTitle = "TONGUE HUNTER";
int* tonguehunterDescription = "[Hold] Extend tongue";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] tonguehunterCharacters = {{
    " llll ",
    "llllll",
    "llllll",
    "llllll",
    "llllll",
    " l  l ",
}};
int tonguehunterCharactersCount = 1;

Options tonguehunterOptions = {100, 100, 0, false};

struct TonguehunterFrog {
  float x;
  float y;
};
TonguehunterFrog tonguehunterFrog;

float tonguehunterTongueLength;
float tonguehunterTongueAngle;
int tonguehunterCombo;
float tonguehunterFrogSquash;

struct TonguehunterPrey {
  float x;
  float y;
  float vx;
  float squash;
  bool isAlive;
};
// Worked out worst-case concurrency around difficulty 3-4 (spawn interval
// clamps to 20 ticks while lifetime is still ~220 ticks) at roughly 11-12
// concurrent prey - sized well above that with headroom.
#define TONGUEHUNTER_MAX_PREY_COUNT 48
TonguehunterPrey[TONGUEHUNTER_MAX_PREY_COUNT] tonguehunterPreys;
int tonguehunterPreyIndex;

struct TonguehunterHazard {
  float x;
  float y;
  float vy;
  float rot;
  bool isAlive;
};
// Concurrency peaks around difficulty 1 at roughly 2.5 - generous headroom.
#define TONGUEHUNTER_MAX_HAZARD_COUNT 16
TonguehunterHazard[TONGUEHUNTER_MAX_HAZARD_COUNT] tonguehunterHazards;
int tonguehunterHazardIndex;

struct TonguehunterTrail {
  float x;
  float y;
  float life;
  bool isAlive;
};
// One trail spawned per tick while the tongue is extended (life 1.0,
// decays 0.15/tick -> ~7 tick lifetime) - up to ~7 concurrent, doubled here.
#define TONGUEHUNTER_MAX_TRAIL_COUNT 16
TonguehunterTrail[TONGUEHUNTER_MAX_TRAIL_COUNT] tonguehunterTrails;
int tonguehunterTrailIndex;

void tonguehunterUpdate() {
  Collision scratch;
  if (!ticks) {
    tonguehunterFrog.x = 50;
    tonguehunterFrog.y = 90;
    tonguehunterTongueLength = 0;
    tonguehunterTongueAngle = -CGLP_PI / 2;
    INIT_UNALIVED_ARRAY_FAST(tonguehunterPreys);
    tonguehunterPreyIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(tonguehunterHazards);
    tonguehunterHazardIndex = 0;
    tonguehunterCombo = 1;
    INIT_UNALIVED_ARRAY_FAST(tonguehunterTrails);
    tonguehunterTrailIndex = 0;
    tonguehunterFrogSquash = 1;
  }

  float spawnRate = 60 / difficulty;
  if (spawnRate < 20) {
    spawnRate = 20;
  }
  if (ticks % (int)floor(spawnRate) == 0) {
    bool side = rnd(0, 1) < 0.5;
    ASSIGN_ARRAY_ITEM(tonguehunterPreys, tonguehunterPreyIndex, TonguehunterPrey, p);
    if (side) {
      p->x = -5;
      p->vx = rnd(0.3, 0.6);
    } else {
      p->x = 105;
      p->vx = rnd(-0.6, -0.3);
    }
    p->y = rnd(20, 60);
    p->squash = 1;
    p->isAlive = true;
    tonguehunterPreyIndex = cgl_wrap(tonguehunterPreyIndex + 1, 0, TONGUEHUNTER_MAX_PREY_COUNT);
  }

  int hazardMod = (int)floor(90 / sqrt(difficulty));
  if (hazardMod < 1) {
    hazardMod = 1;
  }
  if (ticks % hazardMod == 0) {
    ASSIGN_ARRAY_ITEM(tonguehunterHazards, tonguehunterHazardIndex, TonguehunterHazard, h);
    h->x = rnd(10, 90);
    h->y = -5;
    h->vy = 0.5 * rnd(1, difficulty);
    h->rot = 0;
    h->isAlive = true;
    tonguehunterHazardIndex = cgl_wrap(tonguehunterHazardIndex + 1, 0, TONGUEHUNTER_MAX_HAZARD_COUNT);
  }

  if (input.isJustPressed) {
    tonguehunterFrogSquash = 1.3;
  }
  tonguehunterFrogSquash += (1 - tonguehunterFrogSquash) * 0.15;

  float tongueEndX = tonguehunterFrog.x + cos(tonguehunterTongueAngle) * tonguehunterTongueLength;
  float tongueEndY = tonguehunterFrog.y + sin(tonguehunterTongueAngle) * tonguehunterTongueLength;

  TonguehunterPrey* nearestPrey = NULL;
  float nearestDist = 999;
  FOR_EACH(tonguehunterPreys, npi) {
    ASSIGN_ARRAY_ITEM(tonguehunterPreys, npi, TonguehunterPrey, p);
    SKIP_IS_NOT_ALIVE(p);
    float dx = p->x - tongueEndX;
    float dy = p->y - tongueEndY;
    float d = sqrt(dx * dx + dy * dy);
    if (d < nearestDist) {
      nearestDist = d;
      nearestPrey = p;
    }
  }

  if (input.isPressed && nearestPrey != NULL) {
    float ta = atan2(nearestPrey->y - tonguehunterFrog.y, nearestPrey->x - tonguehunterFrog.x);
    if (fabs(ta - tonguehunterTongueAngle) < 0.1) {
      tonguehunterTongueAngle = ta;
      tonguehunterTongueLength += 8;
    } else if (ta < tonguehunterTongueAngle) {
      tonguehunterTongueAngle -= 0.1;
    } else {
      tonguehunterTongueAngle += 0.1;
    }
  }
  tonguehunterTongueLength = clamp(tonguehunterTongueLength - 5, 0, 80);

  if (tonguehunterTongueLength > 10) {
    ASSIGN_ARRAY_ITEM(tonguehunterTrails, tonguehunterTrailIndex, TonguehunterTrail, nt);
    nt->x = tongueEndX;
    nt->y = tongueEndY;
    nt->life = 1;
    nt->isAlive = true;
    tonguehunterTrailIndex = cgl_wrap(tonguehunterTrailIndex + 1, 0, TONGUEHUNTER_MAX_TRAIL_COUNT);
  }

  FOR_EACH(tonguehunterTrails, ti) {
    ASSIGN_ARRAY_ITEM(tonguehunterTrails, ti, TonguehunterTrail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->life -= 0.15;
    if (t->life <= 0) {
      t->isAlive = false;
      continue;
    }
  }

  FOR_EACH(tonguehunterPreys, pui) {
    ASSIGN_ARRAY_ITEM(tonguehunterPreys, pui, TonguehunterPrey, p);
    SKIP_IS_NOT_ALIVE(p);
    p->x += p->vx * (1 + difficulty * 0.05);
    p->squash += (1 - p->squash) * 0.1;
    if (p->x <= -10 || p->x >= 110) {
      p->isAlive = false;
      continue;
    }
  }

  FOR_EACH(tonguehunterHazards, hui) {
    ASSIGN_ARRAY_ITEM(tonguehunterHazards, hui, TonguehunterHazard, h);
    SKIP_IS_NOT_ALIVE(h);
    h->y += h->vy;
    h->rot += h->vy * 0.1;
    if (h->y >= 110) {
      h->isAlive = false;
      continue;
    }
  }

  // 1. Draw trails (decorative, no collision)
  color = LIGHT_YELLOW;
  FOR_EACH(tonguehunterTrails, ti2) {
    ASSIGN_ARRAY_ITEM(tonguehunterTrails, ti2, TonguehunterTrail, t);
    SKIP_IS_NOT_ALIVE(t);
    box(t->x, t->y, 4 * t->life, 4 * t->life, &scratch);
  }

  // 2. Draw prey bodies FIRST (collision target for tongue)
  color = BLACK;
  FOR_EACH(tonguehunterPreys, pdi) {
    ASSIGN_ARRAY_ITEM(tonguehunterPreys, pdi, TonguehunterPrey, p);
    SKIP_IS_NOT_ALIVE(p);
    float w = 5 * p->squash;
    float h = 5 / p->squash;
    box(p->x, p->y, w, h, &scratch);
  }

  // 3. Draw prey eyes (decorative, after body)
  FOR_EACH(tonguehunterPreys, pei) {
    ASSIGN_ARRAY_ITEM(tonguehunterPreys, pei, TonguehunterPrey, p);
    SKIP_IS_NOT_ALIVE(p);
    float eyeOffsetX;
    if (p->vx > 0) {
      eyeOffsetX = 1.5;
    } else {
      eyeOffsetX = -1.5;
    }
    color = WHITE;
    box(p->x + eyeOffsetX, p->y - 1, 2, 2, &scratch);
    color = BLUE;
    float pupilOfs;
    if (p->vx > 0) {
      pupilOfs = 0.5;
    } else {
      pupilOfs = -0.5;
    }
    box(p->x + eyeOffsetX + pupilOfs, p->y - 1, 1, 1, &scratch);
  }

  // 4. Draw frog
  color = GREEN;
  // Vircon32 port note: upstream draws this with a scaleX/scaleY squash
  // option this engine's character() has no equivalent for - drawn at
  // normal scale instead (cosmetic only, frog has no collision here).
  character("a", tonguehunterFrog.x, tonguehunterFrog.y, &scratch);

  // 5. Draw frog eyes looking at prey
  if (nearestPrey != NULL) {
    float eyeDx;
    if (nearestPrey->x > tonguehunterFrog.x) {
      eyeDx = 1;
    } else {
      eyeDx = -1;
    }
    float eyeDy;
    if (nearestPrey->y < tonguehunterFrog.y) {
      eyeDy = -0.5;
    } else {
      eyeDy = 0.5;
    }
    color = WHITE;
    box(tonguehunterFrog.x - 2, tonguehunterFrog.y - 2, 2, 2, &scratch);
    box(tonguehunterFrog.x + 2, tonguehunterFrog.y - 2, 2, 2, &scratch);
    color = BLUE;
    box(tonguehunterFrog.x - 2 + eyeDx * 0.5, tonguehunterFrog.y - 2 + eyeDy, 1, 1, &scratch);
    box(tonguehunterFrog.x + 2 + eyeDx * 0.5, tonguehunterFrog.y - 2 + eyeDy, 1, 1, &scratch);
  }

  // 6. Draw tongue and check collision with prey (black)
  if (tonguehunterTongueLength > 5) {
    color = LIGHT_RED;
    thickness = 2;
    line(tonguehunterFrog.x, tonguehunterFrog.y, tongueEndX, tongueEndY, &scratch);

    float tipPulse = 1 + sin(ticks * 0.3) * 0.2;
    color = CYAN;
    Collision tipCol;
    box(tongueEndX, tongueEndY, 6 * tipPulse, 6 * tipPulse, &tipCol);

    if (tipCol.isColliding.rect[BLACK]) {
      play(COIN);
      addScore(ceil(tonguehunterTongueLength) * tonguehunterCombo, tongueEndX, tongueEndY);
      tonguehunterCombo++;

      color = YELLOW;
      particle(tongueEndX, tongueEndY, 15, 2, 0, CGLP_PI * 2);

      TonguehunterPrey* toRemove = NULL;
      float minD = 999;
      FOR_EACH(tonguehunterPreys, ri) {
        ASSIGN_ARRAY_ITEM(tonguehunterPreys, ri, TonguehunterPrey, p);
        SKIP_IS_NOT_ALIVE(p);
        float dx = p->x - tongueEndX;
        float dy = p->y - tongueEndY;
        float d = sqrt(dx * dx + dy * dy);
        if (d < minD) {
          minD = d;
          toRemove = p;
        }
      }
      if (toRemove != NULL) {
        toRemove->isAlive = false;
      }
    }
  } else {
    tonguehunterCombo = 1;
  }

  bool isExtended = tonguehunterTongueLength > 5;

  // 7. Draw hazards and check collision with tongue
  FOR_EACH(tonguehunterHazards, hdi) {
    ASSIGN_ARRAY_ITEM(tonguehunterHazards, hdi, TonguehunterHazard, h);
    SKIP_IS_NOT_ALIVE(h);
    float breathe = 1 + sin(ticks * 0.15 + h->x) * 0.1;
    float size = 8 * breathe;

    color = RED;
    thickness = size * 0.7;
    bar(h->x, h->y, size, h->rot, &scratch);

    color = WHITE;
    box(h->x - 2, h->y, 2, 2, &scratch);
    box(h->x + 2, h->y, 2, 2, &scratch);
    color = PURPLE;
    box(h->x - 2, h->y + 0.5, 1, 1, &scratch);
    box(h->x + 2, h->y + 0.5, 1, 1, &scratch);

    color = RED;
    Collision hCol;
    box(h->x, h->y, size, size, &hCol);
    if (isExtended && hCol.isColliding.rect[LIGHT_RED]) {
      play(EXPLOSION);
      color = RED;
      particle(h->x, h->y, 30, 3, 0, CGLP_PI * 2);
      gameOver();
    }
  }

  color = BLACK;
  int[16] tonguehunterComboText;
  strcpy(tonguehunterComboText, "x");
  strcat(tonguehunterComboText, intToChar(tonguehunterCombo));
  text(tonguehunterComboText, 3, 9, &scratch);
}

void addGameTonguehunter() {
  addGame(tonguehunterTitle, tonguehunterDescription, tonguehunterCharacters,
          tonguehunterCharactersCount, &tonguehunterOptions, false,
          &tonguehunterUpdate);
}
