#include "../cglp.h"

int* quantumleaperTitle = "QUANTUM LEAPER";
int* quantumleaperDescription = "[Tap]\nChange lane";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] quantumleaperCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int quantumleaperCharactersCount = 0;

Options quantumleaperOptions = {100, 100, 1, false};

#define QUANTUMLEAPER_NUM_UNIVERSES 5
#define QUANTUMLEAPER_WAVE_AMPLITUDE 10
#define QUANTUMLEAPER_WAVE_FREQUENCY 0.1

struct QuantumleaperPlayer {
  Vector pos;
  float waveOffset;
  int currentUniverse;
  float nextAddingScoreOffset;
};
QuantumleaperPlayer quantumleaperPlayer;

struct QuantumleaperAntiParticle {
  Vector pos;
  int universeIdx;
  bool isAlive;
};
#define QUANTUMLEAPER_MAX_ANTIPARTICLE_COUNT 16
QuantumleaperAntiParticle[QUANTUMLEAPER_MAX_ANTIPARTICLE_COUNT] quantumleaperAntiParticles;
int quantumleaperAntiParticleIndex;

float[QUANTUMLEAPER_NUM_UNIVERSES] quantumleaperUniverseLanesY;

int quantumleaperGoldLaneIndex;
float quantumleaperChangingGoldLaneTicks;
int quantumleaperMultiplier;
float quantumleaperScrollingSpeed;

void quantumleaperInitializeGame() {
  vectorSet(&quantumleaperPlayer.pos, 20, 50);
  quantumleaperPlayer.waveOffset = 0;
  quantumleaperPlayer.currentUniverse = 2;
  quantumleaperPlayer.nextAddingScoreOffset = 0;
  INIT_UNALIVED_ARRAY_FAST(quantumleaperAntiParticles);
  quantumleaperAntiParticleIndex = 0;
  TIMES(QUANTUMLEAPER_NUM_UNIVERSES, i) {
    quantumleaperUniverseLanesY[i] = 20 + i * 70.0 / (QUANTUMLEAPER_NUM_UNIVERSES - 1);
  }
  quantumleaperGoldLaneIndex = 2;
  quantumleaperChangingGoldLaneTicks = 300;
  quantumleaperScrollingSpeed = 1;
  quantumleaperMultiplier = 1;
}

void quantumleaperUpdateScrollingSpeed() { quantumleaperScrollingSpeed = difficulty; }

void quantumleaperUpdatePlayerPosition() {
  quantumleaperPlayer.waveOffset += QUANTUMLEAPER_WAVE_FREQUENCY;
  if (quantumleaperPlayer.waveOffset >= quantumleaperPlayer.nextAddingScoreOffset) {
    if (quantumleaperPlayer.currentUniverse == quantumleaperGoldLaneIndex) {
      play(COIN);
      addScore(quantumleaperMultiplier, quantumleaperPlayer.pos.x, quantumleaperPlayer.pos.y);
      quantumleaperMultiplier = (int)clamp(quantumleaperMultiplier + 1, 1, 99);
    } else {
      play(HIT);
      quantumleaperMultiplier = (int)clamp(quantumleaperMultiplier - 1, 1, 99);
    }
    quantumleaperPlayer.nextAddingScoreOffset += CGLP_PI;
  }
  quantumleaperPlayer.pos.y = quantumleaperUniverseLanesY[quantumleaperPlayer.currentUniverse] +
                              sin(quantumleaperPlayer.waveOffset) * QUANTUMLEAPER_WAVE_AMPLITUDE;
}

void quantumleaperHandlePlayerInput() {
  if (input.isJustPressed) {
    float dir;
    if (sin(quantumleaperPlayer.waveOffset) > 0) {
      dir = 1;
    } else {
      dir = -1;
    }
    quantumleaperPlayer.currentUniverse =
        (int)cgl_wrap(quantumleaperPlayer.currentUniverse + dir, 0, QUANTUMLEAPER_NUM_UNIVERSES);
  }
}

void quantumleaperUpdateAndDrawUniverseLanes() {
  Collision scratch;
  quantumleaperChangingGoldLaneTicks--;
  if (quantumleaperChangingGoldLaneTicks < 0) {
    quantumleaperGoldLaneIndex = rndi(0, QUANTUMLEAPER_NUM_UNIVERSES);
    quantumleaperChangingGoldLaneTicks += 300;
  }
  TIMES(QUANTUMLEAPER_NUM_UNIVERSES, i) {
    if (i == quantumleaperGoldLaneIndex) {
      color = YELLOW;
    } else {
      color = LIGHT_BLUE;
    }
    thickness = 3;
    line(0, quantumleaperUniverseLanesY[i], 100, quantumleaperUniverseLanesY[i], &scratch);
  }
}

void quantumleaperUpdateAndDrawAntiParticles() {
  Collision scratch;
  color = PURPLE;
  FOR_EACH(quantumleaperAntiParticles, i) {
    ASSIGN_ARRAY_ITEM(quantumleaperAntiParticles, i, QuantumleaperAntiParticle, ap);
    SKIP_IS_NOT_ALIVE(ap);
    ap->pos.x -= quantumleaperScrollingSpeed;
    box(ap->pos.x, ap->pos.y, 3, 3, &scratch);
    if (scratch.isColliding.rect[CYAN] || ap->pos.x < 0) {
      ap->isAlive = false;
      continue;
    }
  }
}

void quantumleaperDrawPlayer() {
  Collision scratch;
  color = CYAN;
  thickness = 3;
  arc(quantumleaperPlayer.pos.x, quantumleaperPlayer.pos.y, 2, 0, CGLP_PI * 2, &scratch);
}

void quantumleaperCheckCollisions() {
  Collision scratch;
  box(quantumleaperPlayer.pos.x, quantumleaperPlayer.pos.y, 4, 4, &scratch);
  if (scratch.isColliding.rect[PURPLE]) {
    play(EXPLOSION);
    gameOver();
  }
}

void quantumleaperSpawnAntiParticles() {
  if (rnd(0, 1) < 0.02 * difficulty) {
    play(CLICK);
    int universeIdx = rndi(0, QUANTUMLEAPER_NUM_UNIVERSES);
    ASSIGN_ARRAY_ITEM(quantumleaperAntiParticles, quantumleaperAntiParticleIndex,
                       QuantumleaperAntiParticle, nap);
    vectorSet(&nap->pos, 103, quantumleaperUniverseLanesY[universeIdx]);
    nap->universeIdx = universeIdx;
    nap->isAlive = true;
    quantumleaperAntiParticleIndex =
        cgl_wrap(quantumleaperAntiParticleIndex + 1, 0, QUANTUMLEAPER_MAX_ANTIPARTICLE_COUNT);
  }
}

void quantumleaperUpdate() {
  Collision scratch;
  if (!ticks) {
    quantumleaperInitializeGame();
  }
  quantumleaperUpdateScrollingSpeed();
  quantumleaperUpdatePlayerPosition();
  quantumleaperHandlePlayerInput();
  quantumleaperUpdateAndDrawUniverseLanes();
  quantumleaperUpdateAndDrawAntiParticles();
  quantumleaperDrawPlayer();
  quantumleaperCheckCollisions();
  quantumleaperSpawnAntiParticles();
  color = BLACK;
  int[16] multText;
  strcpy(multText, "x");
  strcat(multText, intToChar(quantumleaperMultiplier));
  text(multText, 2, 9, &scratch);
}

void addGameQuantumleaper() {
  addGame(quantumleaperTitle, quantumleaperDescription, quantumleaperCharacters,
          quantumleaperCharactersCount, &quantumleaperOptions, false,
          &quantumleaperUpdate);
}
