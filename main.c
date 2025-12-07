#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>
#ifdef __linux__
#include <bsd/stdlib.h>
#endif
#include <ncurses.h>

#define NUM_TETROMINOES 7
#define FIELD_WIDTH     12
#define FIELD_HEIGHT    18
#define FIELD_LENGTH    (FIELD_WIDTH * FIELD_HEIGHT)
#define NS_PER_S        1000000000
#define NS_PER_FRAME    16666667

struct tetromino {
    int sidelen;
    int x;
    int y;
    int rot;
    const char* sprite;
};

//=================
// ROTATION TABLES
//=================
enum rotationDegrees {
    ROT_DEG_0 = 0,
    ROT_DEG_90,
    ROT_DEG_180,
    ROT_DEG_270
};

// For 3x3 shapes:
const int threeRot[4][3][3] = {
    [ROT_DEG_0] = {
        {0, 1, 2},
        {3, 4, 5},
        {6, 7, 8}
    },
    [ROT_DEG_90] = {
        {6, 3, 0},
        {7, 4, 1},
        {8, 5, 2}
     },
    [ROT_DEG_180] = {
        {8, 7, 6},
        {5, 4, 3},
        {2, 1, 0}
    },
    // 270 degrees:
    [ROT_DEG_270] = {
        {2, 5, 8},
        {1, 4, 7},
        {0, 3, 6}
    }
};

// For 4x4 shapes:
const int fourRot[4][4][4] = {
    [ROT_DEG_0] = {
        { 0,  1,  2,  3},
        { 4,  5,  6,  7},
        { 8,  9, 10, 11},
        {12, 13, 14, 15}
    },
    [ROT_DEG_90] = {
        {12,  8,  4,  0},
        {13,  9,  5,  1},
        {14, 10,  6,  2},
        {15, 11,  7,  3}
    },
    [ROT_DEG_180] = {
        {15, 14, 13, 12},
        {11, 10,  9,  8},
        { 7,  6,  5,  4},
        { 3,  2,  1,  0}
    },
    [ROT_DEG_270] = {
        { 3,  7, 11, 15},
        { 2,  6, 10, 14},
        { 1,  5,  9, 13},
        { 0,  4,  8, 12}
    }
};

void drawField(char field[const FIELD_LENGTH]);
void drawHUD(const int score, const int numLinesCleared, const int level);
void clearLinesFromField(char field[const FIELD_LENGTH], int numLinesToClear, int lowestLineToClear);
void drawPiece(const struct tetromino* const t);
int getPieceIndexForRotation(const struct tetromino* const t, const int x, const int y);
bool pieceCanFit(const char field[const FIELD_LENGTH], const struct tetromino* const t);
void shuffleArray(int bag[const NUM_TETROMINOES]);
#ifdef __linux__
uint64_t getTimeDiff(const struct timespec* const start, const struct timespec* const stop);
#endif

int main(void)
{
    // ----------------
    // Piece "sprites"
    // ----------------
    // Based on the Super Rotation System:
    // https://tetris.fandom.com/wiki/SRS
    static const char* const tetrominoes[NUM_TETROMINOES] = {
        "    IIII        ",
        "ZZ  ZZ   ",
        " SSSS    ",
        "OOOO",
        " T TTT   ",
        "  LLLL   ",
        "J  JJJ   "
    };
    static const int tetrominoSideLengths[NUM_TETROMINOES] = {4, 3, 3, 2, 3, 3, 3};

    // -------------------------
    // Initialize field map
    // -------------------------
    char field[FIELD_LENGTH];
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        const int fieldRow = y * FIELD_WIDTH;
        for (int x = 0; x < FIELD_WIDTH; x++) {
            const int i = fieldRow + x;
            if (x == 0 || x == FIELD_WIDTH - 1 || y == FIELD_HEIGHT - 1) {
                field[i] = '#';
            } else {
                field[i] = ' ';
            }
        }
    }

    // -------------------------
    // Initialize ncurses screen
    // -------------------------
    initscr();
    // Make user-typed characters immediately available
    cbreak();
    // Don't echo typed characters to the terminal
    noecho();
    // Enable reading of arrow keys
    keypad(stdscr, true);
    // Make getch non-blocking
    nodelay(stdscr, true);
    // Make cursor invisible
    curs_set(0);

    // Initialize the array with a random tetromino sequence
    int pieceBag[NUM_TETROMINOES] = {0, 1, 2, 3, 4, 5, 6};
    shuffleArray(pieceBag);

    // --------------------
    // Game state variables
    // --------------------
    int currentBagIndex = 0;
    int currentPieceNum = pieceBag[currentBagIndex];
    struct tetromino t = {
        //tetrominoLengths[currentPieceNum],
        tetrominoSideLengths[currentPieceNum],
        4,
        1,
        0,
        tetrominoes[currentPieceNum]
    };

    bool shouldForceDownward = false;

    unsigned int totalNumLinesCleared = 0;
    unsigned int score = 0;
    unsigned int level = 0;
    unsigned int tenLineCounter =0;

    // Timing
    int numTicks = 0;
    int maxTicksPerLine = 48;
    
    // Ensure game begins with the screen drawn
    drawField(field);
    drawHUD(score, totalNumLinesCleared, level);

    bool gameOver = false;
    while (!gameOver) {
#ifdef __APPLE__
        const uint64_t startTimeNs = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
#else
        struct timespec startTime;
        clock_gettime(CLOCK_MONOTONIC, &startTime);
#endif
        shouldForceDownward = (numTicks >= maxTicksPerLine);

        // Process input
        const int keyInput = getch();
        int newRotation = t.rot;
        switch (keyInput) {
        case 'h':
        case 'H':
        case KEY_LEFT:
            t.x--;
            if (!pieceCanFit(field, &t)) {
                t.x++;
            }
            break;
        case 'l':
        case 'L':
        case KEY_RIGHT:
            t.x++;
            if (!pieceCanFit(field, &t)) {
                t.x--;
            }
            break;
        case 'j':
        case 'J':
        case KEY_DOWN:
            shouldForceDownward = true;
            break;
        case 'a':
        case 'A':
            // Rotate 90 degrees counterclockwise
            newRotation = (newRotation == 0) ? 3 : newRotation - 1;
            break;
        case 's':
        case 'S':
            // Rotate 90 degrees clockwise
            newRotation = (newRotation == 3) ? 0 : newRotation + 1;
            break;
        default:
            break;
        }

        if (newRotation != t.rot) {
            const int currentRotation = t.rot;
            t.rot = newRotation;
            if (!pieceCanFit(field, &t)) {
                t.rot = currentRotation;
            }
        }

        bool shouldFixInPlace = false;
        if (shouldForceDownward) {
            t.y++;
            if (!pieceCanFit(field, &t)) {
                t.y--;
                shouldFixInPlace = true;
            }
            numTicks = 0;
            shouldForceDownward = false;
        }

        int numLinesToClear = 0;
        int lowestLineToClear = 0;

        if (!shouldFixInPlace) {
            drawField(field);
            drawPiece(&t);
        } else if (t.y <= 1) {
            gameOver = true;
        } else {
            // Add piece to field map
            for (int y = 0; y < t.sidelen; y++) {
                const int fieldYOffset = (t.y + y) * FIELD_WIDTH;
                for (int x = 0; x < t.sidelen; x++) {
                    const int pieceIndex = getPieceIndexForRotation(&t, x, y);
                    const char charSprite = t.sprite[pieceIndex];
                    if (charSprite == ' ') {
                        continue;
                    }
                    const int fieldIndex = fieldYOffset + (t.x + x);
                    field[fieldIndex] = charSprite;
                }
            }

            // Check if any lines should be cleared
            for (int y = 0; y < t.sidelen; y++) {
                const int screenRow = t.y + y;
                // Stop if going outside the boundaries
                if (screenRow >= FIELD_HEIGHT - 1) {
                    break;
                }

                // Begin with the assumption that the line is full of blocks
                bool lineIsFull = true;
                const int fieldRow = screenRow * FIELD_WIDTH;

                // Check whether there are any empty spaces in the line
                for (int x = 1; x < FIELD_WIDTH - 1; x++) {
                    const int fieldIndex = fieldRow + x;
                    if (field[fieldIndex] == ' ') {
                        lineIsFull = false;
                        break;
                    }
                }

                if (!lineIsFull)
                    continue;

                // Rewrite all the characters with '='
                for (int x = 1; x < FIELD_WIDTH - 1; x++) {
                    const int fieldIndex = fieldRow + x;
                    field[fieldIndex] = '=';
                }

                // Save the location of this line so it can be cleared later
                lowestLineToClear = screenRow;
                numLinesToClear++;
            }

            // Update field
            drawField(field);

            // Update game state
            currentBagIndex++;
            if (currentBagIndex >= NUM_TETROMINOES) {
                currentBagIndex = 0;
                shuffleArray(pieceBag);
            }
            currentPieceNum = pieceBag[currentBagIndex];
            //t.len = tetrominoLengths[currentPieceNum];
            t.sidelen = tetrominoSideLengths[currentPieceNum];
            t.x = 4;
            t.y = 1;
            t.rot = 0;
            t.sprite = tetrominoes[currentPieceNum];
        }

        if (numLinesToClear > 0) {
            // Must draw the screen once again
            // to show the lines disappearing.

            // First, wait for a short duration
            // so the player can see the effect.
            struct timespec sleepTime = {0, 600000000};
            nanosleep(&sleepTime, &sleepTime);

            // Keep track of player progress
            totalNumLinesCleared += numLinesToClear;

            // Scoring system similar to original Nintendo system
            const int scoringLevel = level + 1;
            switch (numLinesToClear) {
            case 1:
                score += 40 * scoringLevel;
                break;
            case 2:
                score += 100 * scoringLevel;
                break;
            case 3:
                score += 300 * scoringLevel;
                break;
            case 4:
                score += 1200 * scoringLevel;
                break;
            }

            // Check if level should advance
            tenLineCounter += numLinesToClear;
            if (tenLineCounter >= 10) {
                level++;
                tenLineCounter -= 10;
                // Adjust timing
                if (level < 8 && maxTicksPerLine > 5) {
                    maxTicksPerLine -= 5;
                } else if (maxTicksPerLine > 1) {
                    maxTicksPerLine--;
                }
            }

            clearLinesFromField(field, numLinesToClear, lowestLineToClear);
            drawField(field);
            drawHUD(score, totalNumLinesCleared, level);
        }
 
        numTicks++;
        // Wait if necessary to maintain roughly 60 loops per second
#ifdef __APPLE__
        const uint64_t nsElapsed = clock_gettime_nsec_np(CLOCK_UPTIME_RAW) - startTimeNs;
#else
        struct timespec stopTime;
        clock_gettime(CLOCK_MONOTONIC, &stopTime);
        const uint64_t nsElapsed = getTimeDiff(&startTime, &stopTime);
#endif
        if (nsElapsed < NS_PER_FRAME) {
            struct timespec sleepTime = {0, NS_PER_FRAME - nsElapsed};
            nanosleep(&sleepTime, &sleepTime);
        }
    }

    endwin();
    printf("Final score: %d\n", score);
    return EXIT_SUCCESS;
}

void drawField(char field[const FIELD_LENGTH])
{
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        const int fieldRow = y * FIELD_WIDTH;
        for (int x = 0; x < FIELD_WIDTH; x++) {
            const int fieldIndex = fieldRow + x;
            const char charSprite = field[fieldIndex];
            mvaddch(y, x, charSprite);
        }
    }
    refresh();
}

void drawHUD(const int score, const int numLinesCleared, const int level)
{
    static const int x = FIELD_WIDTH + 2;
    mvprintw(1, x, "SCORE:");
    mvprintw(2, x, "%d", score);
    mvprintw(4, x, "LINES:");
    mvprintw(5, x, "%d", numLinesCleared);
    mvprintw(7, x, "LEVEL:");
    mvprintw(8, x, "%d", level);
    refresh();
}

void clearLinesFromField(char field[const FIELD_LENGTH], int numLinesToClear, int lowestLineToClear)
{
    while (numLinesToClear > 0) {
        // Get number of lines to move down
        int numFullContiguousLines = 1;
        const int charAboveIndex = ((lowestLineToClear - 1) * FIELD_WIDTH) + 1;
        for (int i = charAboveIndex; field[i] == '='; i -= FIELD_WIDTH) {
            numFullContiguousLines++;
        }
        
        // This offset designates how far away in the field array
        // the old elements that must be moved down/ahead are
        const int oldYOffset = numFullContiguousLines * FIELD_WIDTH;

        // Move everything in the field array down
        for (int y = lowestLineToClear; y >= 0; y--) {
            const int fieldYOffset = y * FIELD_WIDTH;
            for (int x = 1; x < FIELD_WIDTH - 1; x++) {
                const int newFieldIndex = fieldYOffset + x;
                if (y <= numFullContiguousLines) {
                    field[newFieldIndex] = ' ';
                } else {
                    const int oldFieldIndex = newFieldIndex - oldYOffset;
                    field[newFieldIndex] = field[oldFieldIndex];
                }
            }
        }

        numLinesToClear -= numFullContiguousLines;
        if (numLinesToClear > 0) {
            // Find the next line that needs to be cleared
            int fieldIndex;
            do {
                lowestLineToClear--;
                fieldIndex = (lowestLineToClear * FIELD_WIDTH) + 1;
            } while (field[fieldIndex] != '=');
        }
    }
}

void drawPiece(const struct tetromino* const t)
{
    for (int y = 0; y < t->sidelen; y++) {
        const int drawY = t->y + y;
        for (int x = 0; x < t->sidelen; x++) {
            const int pieceIndex = getPieceIndexForRotation(t, x, y);
            const char charSprite = t->sprite[pieceIndex];
            if (charSprite == ' ') {
                continue;
            }
            const int drawX = t->x + x;
            mvaddch(drawY, drawX, charSprite);
        }
    }
    refresh();
}

int getPieceIndexForRotation(const struct tetromino* const t, const int x, const int y)
{
    int index = 0;

    /*
    // Old method using arithmetic:
    // The "O" tetromino's rotation is irrelevant
    if (t->sidelen < 3)
        return index;
    switch (t->rot)
    {
    case 0:
        index = (y * t->sidelen) + x;
        break;
    case 1:
        index = (t->len - t->sidelen) + y - (x * t->sidelen);
        break;
    case 2:
        index = (t->len - 1) - (y * t->sidelen) - x;
        break;
    case 3:
        index = (t->sidelen - 1) - y + (x * t->sidelen);
        break;
    }
    */

    // New method using tables:
    switch (t->sidelen) {
    case 3:
        index = threeRot[t->rot][y][x];
        break;
    case 4:
        index = fourRot[t->rot][y][x];
        break;
    default:
        break;
    }

    return index;
}

bool pieceCanFit(const char field[const FIELD_LENGTH], const struct tetromino* const t)
{
    for (int y = 0; y < t->sidelen; y++) {
        const int screenRow = t->y + y;
        const int fieldRow = screenRow * FIELD_WIDTH;
        for (int x = 0; x < t->sidelen; x++) {
            const int pieceIndex = getPieceIndexForRotation(t, x, y);
            if (t->sprite[pieceIndex] == ' ') {
                continue;
            }
            const int screenCol = t->x + x;
            if (screenCol < 1 ||
                screenCol >= FIELD_WIDTH ||
                screenRow >= FIELD_HEIGHT ||
                field[fieldRow + screenCol] != ' ') {
                return false;
            }
        }
    }
    return true;
}

void shuffleArray(int bag[const NUM_TETROMINOES])
{
    // Fisher-Yates shuffle
    for (int i = NUM_TETROMINOES - 1; i >= 1; i--) {
        const int j = arc4random_uniform(i + 1);
        const int temp = bag[i];
        bag[i] = bag[j];
        bag[j] = temp;
    }
}

#ifdef __linux__
uint64_t getTimeDiff(const struct timespec* const start, const struct timespec* const stop)
{
    assert((stop->tv_sec > start->tv_sec) || ((stop->tv_sec == start->tv_sec) && (stop->tv_nsec >= start->tv_nsec)));
    int64_t sec = (int64_t)(stop->tv_sec - start->tv_sec);
    int64_t nsec = (int64_t)(stop->tv_nsec - start->tv_nsec);
    if (sec < 0) {
        assert(nsec >= NS_PER_S);
        sec++;
        nsec -= NS_PER_S;
    } else if (nsec < 0) {
        assert(sec >= 1);
        sec--;
        nsec += NS_PER_S;
    }
    nsec += sec * NS_PER_S;
    assert(nsec >= 0);
    return (uint64_t)nsec;
}
#endif

