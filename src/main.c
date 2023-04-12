/**
 * @file      main.c
 * @brief     Main file for the game.
 * @author    8dcc
 *
 * @todo Add mouse support.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> /* tolower */
#include <time.h>
#include <ncurses.h>

#include "main.h" /* Structs and defines */

/**
 * @brief Parses a resolution string with format `WIDTHxHEIGHT` using atoi.
 * @param[out] w Pointer where to save the resolution's width
 * @param[out] h Pointer where to save the resolution's height
 * @param[inout] src String containing the resolution in `WIDTHxHEIGHT` format
 */
static inline void parse_resolution(uint16_t* w, uint16_t* h, char* src) {
    *w = 0;
    *h = 0;

    char* start = src;
    while (*src != 'x' && *src != '\0')
        src++;

    /* No x, invalid format */
    if (*src != 'x')
        return;

    /* Cut string at 'x', make it point to start of 2nd digit */
    *src++ = '\0';

    *w = atoi(start);
    *h = atoi(src);
}

/**
 * @brief Parses the program arguments changing the settings of the current
 * game.
 * @param[in] argc Number of arguments from main.
 * @param[in] argv String vector with the artuments.
 * @param[out] ctx Target ctx_t struct to write the changes.
 * @return False if the caller (main()) needs to exit. True otherwise.
 */
static inline bool parse_args(int argc, char** argv, ctx_t* ctx) {
    bool arg_error = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--resolution")) {
            if (i == argc - 1) {
                fprintf(stderr, "Not enough arguments for \"%s\"\n", argv[i]);
                arg_error = true;
                break;
            }

            i++;
            parse_resolution(&ctx->w, &ctx->h, argv[i]);
            if (ctx->w < MIN_W || ctx->h < MIN_H) {
                fprintf(stderr,
                        "Invalid resolution format for \"%s\".\n"
                        "Minimum resolution: %dx%d\n",
                        argv[i - 1], MIN_W, MIN_H);
                arg_error = true;
                break;
            }
        } else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--scale")) {
            if (i == argc - 1) {
                fprintf(stderr, "Not enough arguments for \"%s\"\n", argv[i]);
                arg_error = true;
                break;
            }

            i++;
            ctx->sc = atoi(argv[i]);
            if (ctx->sc < 1) {
                fprintf(stderr,
                        "Invalid scale format for \"%s\".\n"
                        "Minimum scale: 1\n",
                        argv[i - 1]);
                arg_error = true;
                break;
            }
        } else if (!strcmp(argv[i], "-k") || !strcmp(argv[i], "--keys")) {
            puts("Controls:\n"
                 "    <arrows> - Move in the grid\n"
                 "        hjkl - Move in the grid (vim-like)\n"
                 "     <space> - Toggle selected cell (and adjacent)\n"
                 "           r - Generate random grid\n"
                 "           q - Quit the game");
            return false;
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            arg_error = true;
            break;
        }
    }

    if (arg_error) {
        printf("Usage:\n"
               "    %s                   - Launch with default resolution and "
               "scale\n"
               "    %s --help            - Show this help\n"
               "    %s -h                - Same as --help\n"
               "    %s --keys            - Show the controls\n"
               "    %s -k                - Same as --keys\n"
               "    %s --resolution WxH  - Launch with specified resolution "
               "(width, height)\n"
               "    %s -r WxH            - Same as --resolution\n"
               "    %s --scale N         - Launch with specified scale\n"
               "    %s -s N              - Same as --scale\n",
               argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0],
               argv[0], argv[0]);
        return false;
    }

    return true;
}

/**
 * @brief Initialize the game grid.
 * @param[out] ctx Game context used to write the new grid.
 * @param[in] random Will fill it with ON cells in random positions if true, or
 * all OFF otherwise.
 */
static inline void init_grid(ctx_t* ctx, bool random) {
    if (random) {
        /** @todo Random grid */
    } else {
        for (int y = 0; y < ctx->h; y++)
            for (int x = 0; x < ctx->w; x++)
                ctx->grid[y * ctx->w + x] = OFF_CH;
    }
}

/**
 * @brief Clears a line in the screen.
 * @details Doesn't change the cursor position.
 * @param[in] y Line number starting from 0 to clear.
 */
static inline void clear_line(int y) {
    int oy, ox;
    getyx(stdscr, oy, ox);

    move(y, 0);
    clrtoeol();

    move(oy, ox);
}

/**
 * @brief Draws the grid border for the game.
 * @param[in] ctx Game context structure for the width and height.
 */
static inline void draw_border(const ctx_t* ctx) {
    const int real_w = ctx->w * ctx->sc;
    const int real_h = ctx->h * ctx->sc;

    /** @todo Add vertical spaces after columns here and in redraw_grid() */

    /* First line */
    mvaddch(0, 0, '+');
    for (int x = 0; x < real_w; x++)
        mvaddch(0, x + 1, '-');
    mvaddch(0, real_w + 1, '+');

    /* Mid lines */
    for (int y = 1; y <= real_h; y++) {
        mvaddch(y, 0, '|');
        mvaddch(y, real_w + 1, '|');
    }

    /* Last line */
    mvaddch(real_h + 1, 0, '+');
    for (int x = 0; x < real_w; x++)
        mvaddch(real_h + 1, x + 1, '-');
    mvaddch(real_h + 1, real_w + 1, '+');
}

/**
 * @brief Redraws the grid based on the ctx_t.grid array.
 * @details The cursor in the context struct is the position inside the
 * (unscaled) grid. This function will use it to move to the real post-scale
 * cursor position on the terminal.
 *
 * First of all, each for loop will initialize (clear) the term_* variables to
 * the real positions of the first row/col.
 *
 * Each for loop iteration, it will:
 *  - Increase the grid position (go to next tile)
 *  - Add the scale to the term_x and term_y variables, getting the real
 *    position of that next cell in the terminal.
 *
 * @param[in] ctx Game context structure for the grid.
 */
static void redraw_grid(ctx_t* ctx) {
    const int border_sz = 1;

    draw_border(ctx);

    /* See function details for explanation about the for loops */
    for (int y = 0; y < ctx->h; y++) {
        for (int x = 0; x < ctx->w; x++) {
            const char c = ctx->grid[y * ctx->w + x];

            /* Draw the actual scaled tile in the real positions */
            const int term_y = y * ctx->sc + border_sz;
            const int term_x = x * ctx->sc + border_sz;
            for (int ty = term_y; ty < term_y + ctx->sc; ty++)
                for (int tx = term_x; tx < term_x + ctx->sc; tx++)
                    mvaddch(ty, tx, c);
        }
    }

    /* Update the cursor to the real position:
     *   - Get scaled position of tile.
     *   - Go to center of the tile.
     *   - Add border size to get real position.
     *   - Subtract 1 to get the zero-starting index. */
    const int real_y =
      (ctx->cursor.y * ctx->sc) + (ctx->sc - ctx->sc / 2) + border_sz - 1;
    const int real_x =
      (ctx->cursor.x * ctx->sc) + (ctx->sc - ctx->sc / 2) + border_sz - 1;
    move(real_y, real_x);
}

/**
 * @brief Toggle the selected grid cell, and the adjacent ones.
 * @details Adjacent meaning up, down, left and right.
 * @param[inout] ctx Context struct used for the cursor and the grid.
 */
static inline void toggle_adjacent(ctx_t* ctx) {
    const int y = ctx->cursor.y;
    const int x = ctx->cursor.x;

    /* Key actual position */
    if (ctx->grid[y * ctx->w + x] == ON_CH)
        ctx->grid[y * ctx->w + x] = OFF_CH;
    else
        ctx->grid[y * ctx->w + x] = ON_CH;

    /* Up */
    if (y > 0) {
        if (ctx->grid[(y - 1) * ctx->w + x] == ON_CH)
            ctx->grid[(y - 1) * ctx->w + x] = OFF_CH;
        else
            ctx->grid[(y - 1) * ctx->w + x] = ON_CH;
    }

    /* Down */
    if (y < ctx->h - 1) {
        if (ctx->grid[(y + 1) * ctx->w + x] == ON_CH)
            ctx->grid[(y + 1) * ctx->w + x] = OFF_CH;
        else
            ctx->grid[(y + 1) * ctx->w + x] = ON_CH;
    }

    /* Left */
    if (x > 0) {
        if (ctx->grid[y * ctx->w + (x - 1)] == ON_CH)
            ctx->grid[y * ctx->w + (x - 1)] = OFF_CH;
        else
            ctx->grid[y * ctx->w + (x - 1)] = ON_CH;
    }

    /* Right */
    if (x < ctx->w - 1) {
        if (ctx->grid[y * ctx->w + (x + 1)] == ON_CH)
            ctx->grid[y * ctx->w + (x + 1)] = OFF_CH;
        else
            ctx->grid[y * ctx->w + (x + 1)] = ON_CH;
    }
}

/**
 * @brief Entry point of the program
 * @param argc Number of arguments
 * @param argv Vector of string arguments
 * @return Exit code
 */
int main(int argc, char** argv) {
    /* Main context struct */
    ctx_t ctx = (ctx_t){
        .w      = DEFAULT_W,
        .h      = DEFAULT_H,
        .sc     = DEFAULT_S,
        .cursor = (point_t){ 0, 0 },
        .grid   = NULL,
    };

    /* Parse arguments before ncurses */
    if (!parse_args(argc, argv, &ctx))
        return 1;

    /* Real cursor position after parsing args */
    ctx.cursor = (point_t){
        ctx.h - ctx.h / 2 - 1,
        ctx.w - ctx.w / 2 - 1,
    };

    initscr();            /* Init ncurses */
    raw();                /* Scan input without pressing enter */
    noecho();             /* Don't print when typing */
    keypad(stdscr, true); /* Enable keypad (arrow keys) */

    /* Init random seed */
    srand(time(NULL));

    /* Allocate and initialize grid */
    ctx.grid = calloc(ctx.w * ctx.h, sizeof(uint8_t));
    init_grid(&ctx, false);

    /* Char the user is pressing */
    int c = 0;
    do {
        /* First, redraw the grid */
        redraw_grid(&ctx);

        /* Refresh screen */
        refresh();

        /* Wait for user input */
        c = tolower(getch());

        /* Clear the output line */
        clear_line(ctx.h + 3);

        /* Parse input. 'q' quits and there is vim-like navigation */
        switch (c) {
            case 'k':
            case KEY_UP:
                if (ctx.cursor.y > 0)
                    ctx.cursor.y--;
                break;
            case 'j':
            case KEY_DOWN:
                if (ctx.cursor.y < ctx.h - 1)
                    ctx.cursor.y++;
                break;
            case 'h':
            case KEY_LEFT:
                if (ctx.cursor.x > 0)
                    ctx.cursor.x--;
                break;
            case 'l':
            case KEY_RIGHT:
                if (ctx.cursor.x < ctx.w - 1)
                    ctx.cursor.x++;
                break;
            case ' ':
                toggle_adjacent(&ctx);
                break;
            case 'r':
                /** @todo Random ON/OFF grid */
                break;
            case KEY_CTRLC:
                c = 'q';
                break;
            case 'q':
            default:
                break;
        }
    } while (c != 'q');

    endwin();

    return 0;
}
