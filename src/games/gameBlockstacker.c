#include "../cglp.h"

int* blockstackerTitle = "BLOCK STACKER";
int* blockstackerDescription = "[Tap]\n Release block";

int[1][CHARACTER_WIDTH][CHARACTER_HEIGHT + 1] blockstackerCharacters = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
int blockstackerCharactersCount = 0;

Options blockstackerOptions = {100, 100, 9, false};

struct BlockstackerBlock {
  Vector pos;
  Vector size;
  bool isPlaced;
  Vector velocity;
  bool isAlive;
};

float blockstackerPendulumAngle;
float blockstackerPendulumSpeed;
BlockstackerBlock blockstackerCurrentBlock;
#define BLOCKSTACKER_MAX_PLACED_COUNT 64
BlockstackerBlock[BLOCKSTACKER_MAX_PLACED_COUNT] blockstackerPlacedBlocks;
int blockstackerPlacedBlockIndex;
float blockstackerScrX;

void blockstackerMakeNewBlock(BlockstackerBlock* out, float difficultyValue) {
  blockstackerScrX = rnd(0, difficultyValue - 1) * RNDPM();
  vectorSet(&out->pos, 0, 20);
  rotate(&out->pos, sin(blockstackerPendulumAngle));
  vectorAdd(&out->pos, 50, 0);
  vectorSet(&out->size, rnd(10, 30), 10);
  out->isPlaced = false;
  vectorSet(&out->velocity, 0, 0);
  out->isAlive = true;
}

void blockstackerUpdate() {
  Collision scratch;
  if (!ticks) {
    blockstackerPendulumAngle = 0;
    blockstackerPendulumSpeed = 0.05;
    blockstackerMakeNewBlock(&blockstackerCurrentBlock, difficulty);
    INIT_UNALIVED_ARRAY_FAST(blockstackerPlacedBlocks);
    vectorSet(&blockstackerPlacedBlocks[0].pos, 50, 99);
    vectorSet(&blockstackerPlacedBlocks[0].size, 99, 10);
    blockstackerPlacedBlocks[0].isAlive = true;
    blockstackerPlacedBlockIndex = 1;
    blockstackerScrX = 0;
  }
  blockstackerPendulumAngle += blockstackerPendulumSpeed * difficulty;
  color = BLACK;
  Vector pendulumEnd;
  vectorSet(&pendulumEnd, 0, 20);
  rotate(&pendulumEnd, sin(blockstackerPendulumAngle));
  vectorAdd(&pendulumEnd, 50, 0);
  thickness = 3;
  line(50, 0, pendulumEnd.x, pendulumEnd.y, &scratch);
  if (!blockstackerCurrentBlock.isPlaced) {
    blockstackerCurrentBlock.pos = pendulumEnd;
    vectorAdd(&blockstackerCurrentBlock.pos, 0, 5);
    if (input.isJustPressed) {
      play(SELECT);
      blockstackerCurrentBlock.isPlaced = true;
      vectorSet(&blockstackerCurrentBlock.velocity, -cos(blockstackerPendulumAngle), 0);
    }
  } else {
    Vector move;
    move.x = blockstackerCurrentBlock.velocity.x * difficulty;
    move.y = blockstackerCurrentBlock.velocity.y * difficulty;
    vectorAdd(&blockstackerCurrentBlock.pos, move.x, move.y);
    blockstackerCurrentBlock.velocity.y += 0.1;
    if (blockstackerCurrentBlock.pos.y > 105) {
      play(EXPLOSION);
      gameOver();
    }
  }
  color = LIGHT_BLUE;
  box(blockstackerCurrentBlock.pos.x, blockstackerCurrentBlock.pos.y,
      blockstackerCurrentBlock.size.x, blockstackerCurrentBlock.size.y, &scratch);
  float minY = 99;
  color = BLUE;
  bool isStacked = false;
  FOR_EACH(blockstackerPlacedBlocks, i) {
    ASSIGN_ARRAY_ITEM(blockstackerPlacedBlocks, i, BlockstackerBlock, block);
    SKIP_IS_NOT_ALIVE(block);
    block->pos.x = cgl_wrap(block->pos.x + blockstackerScrX, 0, 100);
    Collision s1, s2, s3;
    box(block->pos.x - 100, block->pos.y, block->size.x, block->size.y, &s1);
    box(block->pos.x, block->pos.y, block->size.x, block->size.y, &s2);
    box(block->pos.x + 100, block->pos.y, block->size.x, block->size.y, &s3);
    bool c1 = s1.isColliding.rect[LIGHT_BLUE];
    bool c2 = s2.isColliding.rect[LIGHT_BLUE];
    bool c3 = s3.isColliding.rect[LIGHT_BLUE];
    if (!isStacked && (c1 || c2 || c3)) {
      blockstackerCurrentBlock.pos.y = block->pos.y - blockstackerCurrentBlock.size.y;
      play(HIT);
      ASSIGN_ARRAY_ITEM(blockstackerPlacedBlocks, blockstackerPlacedBlockIndex, BlockstackerBlock, nb);
      nb->pos = blockstackerCurrentBlock.pos;
      nb->size = blockstackerCurrentBlock.size;
      nb->isAlive = true;
      blockstackerPlacedBlockIndex =
          cgl_wrap(blockstackerPlacedBlockIndex + 1, 0, BLOCKSTACKER_MAX_PLACED_COUNT);
      blockstackerMakeNewBlock(&blockstackerCurrentBlock, difficulty);
      addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      isStacked = true;
    }
    if (block->pos.y < minY) {
      minY = block->pos.y;
    }
  }
  float vy = difficulty * 0.01;
  if (minY < 70) {
    vy += (70 - minY) * 0.1;
  }
  FOR_EACH(blockstackerPlacedBlocks, i) {
    ASSIGN_ARRAY_ITEM(blockstackerPlacedBlocks, i, BlockstackerBlock, block);
    SKIP_IS_NOT_ALIVE(block);
    block->pos.y += vy;
    if (block->pos.y > 106) {
      block->isAlive = false;
    }
  }
  COUNT_IS_ALIVE(blockstackerPlacedBlocks, aliveCount);
  if (aliveCount == 0) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameBlockstacker() {
  addGame(blockstackerTitle, blockstackerDescription, blockstackerCharacters,
          blockstackerCharactersCount, &blockstackerOptions, false,
          &blockstackerUpdate);
}
