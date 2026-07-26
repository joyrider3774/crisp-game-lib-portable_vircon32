#include "../cglp.h"

int* stageseparationTitle = "STAGE SEPARATION";
int* stageseparationDescription = "[Tap]\n Staging";

// This game never calls char()/text() - no custom character patterns
// needed, following the same "single blank placeholder" convention as
// other character-less ports (see gameAttackchain.c).
int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] stageseparationCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int stageseparationCharactersCount = 0;

Options stageseparationOptions = {100, 150, 5, true};

struct StageseparationRocket {
  Vector pos;
  Vector vel;
  float currentStage;
  float stageHeight;
  float stageBurnTime;
};
StageseparationRocket stageseparationRocket;

struct StageseparationStage {
  Vector pos;
  Vector vel;
  float height;
  bool isAlive;
};
#define STAGESEPARATION_MAX_SEPARATED_STAGE_COUNT 32
StageseparationStage[STAGESEPARATION_MAX_SEPARATED_STAGE_COUNT] stageseparationSeparatedStages;
int stageseparationSeparatedStageIndex;

struct StageseparationDebris {
  Vector pos;
  float size;
  Vector vel;
  bool isAlive;
};
#define STAGESEPARATION_MAX_DEBRIS_COUNT 32
StageseparationDebris[STAGESEPARATION_MAX_DEBRIS_COUNT] stageseparationDebris;
int stageseparationDebrisIndex;

#define STAGESEPARATION_STAR_COUNT 200
Vector[STAGESEPARATION_STAR_COUNT] stageseparationStars;

float stageseparationNextDebrisSpawn;
float stageseparationCameraY;
float stageseparationZoom;

// JS also kept a "background" object ({skyColor, starDensity}) initialized
// alongside the rest of the game state, but it's never read anywhere else
// in update() - dead state upstream too, so it's simply not ported here.

void stageseparationWorldToScreen(Vector* pos, Vector* result) {
  result->x = (pos->x - 50) / stageseparationZoom + 50;
  result->y = (pos->y - stageseparationCameraY) / stageseparationZoom;
}

void stageseparationUpdate() {
  Collision scratch;
  if (!ticks) {
    vectorSet(&stageseparationRocket.pos, 50, 90);
    vectorSet(&stageseparationRocket.vel, 0, -0.5);
    stageseparationRocket.currentStage = 4;
    stageseparationRocket.stageHeight = 10;
    stageseparationRocket.stageBurnTime = 100;
    INIT_UNALIVED_ARRAY_FAST(stageseparationDebris);
    stageseparationDebrisIndex = 0;
    INIT_UNALIVED_ARRAY_FAST(stageseparationSeparatedStages);
    stageseparationSeparatedStageIndex = 0;
    TIMES(STAGESEPARATION_STAR_COUNT, i) {
      stageseparationStars[i].x = rnd(-200, 300);
      stageseparationStars[i].y = rnd(-1000, 0);
    }
    stageseparationNextDebrisSpawn = 99;
    stageseparationCameraY = 0;
    stageseparationZoom = 1;
  }
  float altitude = -stageseparationRocket.pos.y;
  addScore(-stageseparationRocket.vel.y * 0.1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  float targetCameraY = stageseparationRocket.pos.y - 130 * stageseparationZoom;
  stageseparationCameraY += clamp((targetCameraY - stageseparationCameraY) * 0.1, -99, 0);
  stageseparationZoom = clamp(1 + altitude / 9999, 1, 3);
  color = BLACK;
  TIMES(STAGESEPARATION_STAR_COUNT, si) {
    Vector* star = &stageseparationStars[si];
    float screenX = (star->x - 50) / stageseparationZoom + 50;
    float screenY = fmod(star->y - stageseparationCameraY, 1000) / stageseparationZoom;
    if (screenX >= 0 && screenX <= 100 && screenY >= 0 && screenY <= 150) {
      box(screenX, screenY, 1, 1, &scratch);
    }
  }

  stageseparationRocket.vel.y -= 0.1 * (stageseparationRocket.stageBurnTime / 60);
  stageseparationRocket.vel.y += 0.05;
  stageseparationRocket.pos.y += stageseparationRocket.vel.y * difficulty;
  if (stageseparationRocket.stageBurnTime > 0) {
    stageseparationRocket.stageBurnTime -= difficulty;
  }
  if (input.isJustPressed && stageseparationRocket.currentStage > 2) {
    play(CLICK);
    ASSIGN_ARRAY_ITEM(stageseparationSeparatedStages, stageseparationSeparatedStageIndex,
                       StageseparationStage, ns);
    vectorSet(&ns->pos, stageseparationRocket.pos.x,
              stageseparationRocket.pos.y +
                  stageseparationRocket.stageHeight * (stageseparationRocket.currentStage - 1));
    vectorSet(&ns->vel, rnd(0, 0.5) * RNDPM(), stageseparationRocket.vel.y + 0.5);
    ns->height = stageseparationRocket.stageHeight;
    ns->isAlive = true;
    stageseparationSeparatedStageIndex =
        cgl_wrap(stageseparationSeparatedStageIndex + 1, 0, STAGESEPARATION_MAX_SEPARATED_STAGE_COUNT);
    stageseparationRocket.currentStage--;
    stageseparationRocket.vel.y -= 0.5;
    stageseparationRocket.stageBurnTime = 60;
  }
  stageseparationRocket.currentStage = clamp(stageseparationRocket.currentStage + 0.02 * difficulty, 0, 4);
  if (stageseparationRocket.currentStage > 2) {
    color = RED;
  } else {
    color = LIGHT_RED;
  }
  Vector rocketScreenPos;
  stageseparationWorldToScreen(&stageseparationRocket.pos, &rocketScreenPos);
  rect(rocketScreenPos.x - 5 / stageseparationZoom, rocketScreenPos.y, 10 / stageseparationZoom,
       stageseparationRocket.stageHeight * stageseparationRocket.currentStage / stageseparationZoom,
       &scratch);
  if (stageseparationRocket.stageBurnTime > 0) {
    particle(rocketScreenPos.x,
             rocketScreenPos.y +
                 stageseparationRocket.stageHeight * stageseparationRocket.currentStage / stageseparationZoom,
             1, stageseparationRocket.stageBurnTime / 20 / stageseparationZoom, CGLP_PI_2, 0.3);
  }
  if (rocketScreenPos.y > 150) {
    play(EXPLOSION);
    gameOver();
  }

  color = PURPLE;
  FOR_EACH(stageseparationSeparatedStages, si2) {
    ASSIGN_ARRAY_ITEM(stageseparationSeparatedStages, si2, StageseparationStage, s);
    SKIP_IS_NOT_ALIVE(s);
    s->vel.y += 0.1;
    vectorAdd(&s->pos, s->vel.x, s->vel.y);
    Vector stageScreenPos;
    stageseparationWorldToScreen(&s->pos, &stageScreenPos);
    rect(stageScreenPos.x - 5 / stageseparationZoom, stageScreenPos.y, 10 / stageseparationZoom,
         s->height / stageseparationZoom, &scratch);
    if (s->pos.y > stageseparationCameraY + 200 * stageseparationZoom) {
      s->isAlive = false;
      continue;
    }
  }

  stageseparationNextDebrisSpawn += stageseparationRocket.vel.y * difficulty;
  if (stageseparationNextDebrisSpawn < 0) {
    int side = rndi(0, 2);
    float spawnX;
    if (side) {
      spawnX = 50 + 70 * stageseparationZoom;
    } else {
      spawnX = 50 - 70 * stageseparationZoom;
    }
    ASSIGN_ARRAY_ITEM(stageseparationDebris, stageseparationDebrisIndex, StageseparationDebris, nd);
    vectorSet(&nd->pos, spawnX, stageseparationCameraY - rnd(0, 200 * stageseparationZoom));
    nd->size = rnd(5, 15);
    float velX;
    if (side) {
      velX = -1 * rnd(0.5, 0.8);
    } else {
      velX = 1 * rnd(0.5, 0.8);
    }
    vectorSet(&nd->vel, velX, rnd(-15, 0));
    vectorMul(&nd->vel, stageseparationZoom / 2);
    nd->isAlive = true;
    stageseparationDebrisIndex = cgl_wrap(stageseparationDebrisIndex + 1, 0, STAGESEPARATION_MAX_DEBRIS_COUNT);
    stageseparationNextDebrisSpawn = rnd(120, 160) / sqrt(stageseparationZoom);
  }
  color = LIGHT_BLACK;
  FOR_EACH(stageseparationDebris, di) {
    ASSIGN_ARRAY_ITEM(stageseparationDebris, di, StageseparationDebris, d);
    SKIP_IS_NOT_ALIVE(d);
    vectorAdd(&d->pos, d->vel.x * difficulty, d->vel.y * difficulty);
    Vector debrisScreenPos;
    stageseparationWorldToScreen(&d->pos, &debrisScreenPos);
    Collision dc;
    arc(debrisScreenPos.x, debrisScreenPos.y, d->size / stageseparationZoom, 0, CGLP_PI * 2, &dc);
    if (dc.isColliding.rect[RED] || dc.isColliding.rect[LIGHT_RED]) {
      play(EXPLOSION);
      gameOver();
      // JS explicitly "return false" here (collide but don't remove the
      // debris) - end() has already stopped the game at this point either
      // way, so just skip the removal check below rather than kill it.
      continue;
    }
    if (d->pos.x < 50 - 70 * stageseparationZoom || d->pos.x > 50 + 70 * stageseparationZoom) {
      d->isAlive = false;
      continue;
    }
  }
}

void addGameStageseparation() {
  addGame(stageseparationTitle, stageseparationDescription, stageseparationCharacters,
          stageseparationCharactersCount, &stageseparationOptions, false, &stageseparationUpdate);
}
