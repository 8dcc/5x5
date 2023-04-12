#ifndef _MAIN_H
#define _MAIN_H 1

#include <stdint.h>

/**
 * @def CHEAT
 * @brief Compile with cheater features.
 * @details If the program is compiled with this macro defined, it will add the
 * "Generate cheat grid" feature, which generates a very easy game for testing.
 */
#define CHEAT

#define DEFAULT_W 5 /**< @brief Default width */
#define DEFAULT_H 5 /**< @brief Default height */
#define DEFAULT_S 3 /**< @brief Default scale */
#define MIN_W     5 /**< @brief Minimum width */
#define MIN_H     5 /**< @brief Minimum height */

/**
 * @def KEY_CTRLC
 * @brief Needed for getch()
 */
#define KEY_CTRLC 3

/**
 * @enum game_chars
 * @brief Characters for the tiles
 */
enum game_chars {
    ON_CH  = '#', /**< @brief On tile */
    OFF_CH = '.', /**< @brief Off tile */
};

/**
 * @struct point_t
 * @brief Point in the **context grid**.
 * @details Not a point on the real terminal, but on the grid. This is useful
 * for moving to the real terminal position after rendering the scalled grid.
 */
typedef struct {
    uint16_t y; /**< @brief Y coordinate */
    uint16_t x; /**< @brief X coordinate */
} point_t;

/**
 * @struct ctx_t
 * @brief Context used for the game.
 */
typedef struct {
    uint16_t w;     /**< @brief Grid width */
    uint16_t h;     /**< @brief Grid height */
    uint16_t sc;    /**< @brief Grid scale for rendering */
    point_t cursor; /**< @brief User position inside the grid */

    /**
     * @brief Pointer to the game grid.
     * @details It will save 1 char per cell, scale will be used for rendering
     * only.
     * */
    uint8_t* grid;
} ctx_t;

#endif /* MAIN_H_ */
