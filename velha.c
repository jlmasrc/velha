/*
 * velha: Tic-Tac-Toe player.
 *
 * The game can be player between two human players, a human player
 * and the computer or by two computer players. A human player can
 * request computer analysis before doing a move.
 *
 * Copyright (C) 2023  Joao Luis Meloni Assirati.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


/*
 * Compile it with 'gcc -o velha velha.c'
 * Run './velha -h' for details
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>


/*** Utilities ****************************************************************/

/* Sleep function, argument in miliseconds. */
void msleep(int ms) {
    struct timespec req = {
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000,
    };
    struct timespec rem;
    while(nanosleep(&req, &rem)) req = rem;
}

/* This works as a printf() function, but with delays after
   newlines. It makes easier to follow the output. */
__attribute__ ((format (printf, 1, 2)))
void dprn(char *format, ...) {
    char buffer[200], *p = buffer;
    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);
    while(*p) {
        putchar(*p);
        if(*p == '\n') msleep(50); /* 50 miliseconds delay. */
        p++;
    }
    fflush(stdout);
}

/* 1 if strings are equal, 0 otherwise. Comparison is case insensitive. */
int streq(char *a, char *b) {
    return !strcasecmp(a, b);
}

/**************************************************************** Utilities ***/


/*** Board status *************************************************************/

/* Checks the immediate status of the game.
   - If one of the marks (X or O) is the winner, then returns it.
   - If all squares are marked, returns 'D' (for draw)
   - Otherwise, returns zero (no immediate game definition) */
int status(char board[][3]) {
    char m;

    m = board[1][1];
    if(m != ' ' &&
       ((board[1][0] == m && board[1][2] == m) ||
        (board[0][1] == m && board[2][1] == m) ||
        (board[0][0] == m && board[2][2] == m) ||
        (board[0][2] == m && board[2][0] == m))) return m;

    m = board[0][0];
    if(m != ' ' &&
       ((board[0][1] == m && board[0][2] == m) ||
        (board[1][0] == m && board[2][0] == m))) return m;

    m = board[2][2];
    if(m != ' ' &&
       ((board[2][0] == m && board[2][1] == m) ||
        (board[0][2] == m && board[1][2] == m))) return m;

    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            if(board[i][j] == ' ') return 0;

    return 'D';
}

/* Returns a value for the status st in the context of player. */
int value(int st, int player) {
    if(st == player) return 1;
    if(st == 'D') return 0;
    return -1;
}

/************************************************************* Board status ***/


/*** Data structures **********************************************************/

/* First part of output data for think() */
typedef struct {
    /* Status of the game as returned by status(). */
    int status;
    /* Number of moves needed to reach the above status. */
    int depth;
    /* Total number of moves calculated in the analysis. */
    int move_count;
} thinkdata;

/* Record for 1 move */
typedef struct {
    /* Line and column of the move. */
    int lin, col;
    /* Resulting status of the move. */
    int status;
    /* Number of moves needed to reach the above status after this move. */
    int depth;
} move_t;

/* Second part of output data fot think(). This is a list of possible
   moves. */
typedef struct {
    move_t move[9];
    int length;
} movelist;

/* Adds a move to lst. */
void addmove(movelist *lst, int lin, int col, int sts, int dep) {
    lst->move[lst->length++] =
        /* C99 compound literal. */
        (move_t){.lin = lin, .col = col, .status = sts, .depth = dep,};
}

/********************************************************** Data structures ***/


/*** Recursive analyser *******************************************************/

/* Returns the opponent of player. */
int opponent(int player) {
    return player == 'X' ? 'O' : 'X';
}

/* Recursivelly descends the game tree to evaluate the best immediate
   move for player ('X' or 'O'). The argument lst is optional (may be
   NULL). */
void think(char board[][3], thinkdata *data, movelist *lst, int player) {
    int const opp = opponent(player);
    int best_val = -2; /* Dummy value that gets overwritten below. */

    data->move_count = 0;
    if(lst) lst->length = 0;

    for(int lin = 0; lin < 3; lin++) {
        for(int col = 0; col < 3; col++) {
            int sts, dep, val;

            /* Do not overwrite a mark. */
            if(board[lin][col] != ' ') continue;

            data->move_count++;

            /* Mark the board, evaluate the status and unmark it. */
            board[lin][col] = player;
            /* If the mark does not define the game status, descend. */
            if((sts = status(board))) {
                dep = 1;
            } else {
                thinkdata subdata;
                think(board, &subdata, NULL, opp);
                sts = subdata.status;
                data->move_count += subdata.move_count;
                dep = subdata.depth + 1;
            }
            board[lin][col] = ' ';

            if(lst) addmove(lst, lin, col, sts, dep);

            if((val = value(sts, player)) > best_val) {
                /* We get always here in the first loop, so data->depth and
                   data->status do get initialized here. */
                best_val = val;
                data->depth = dep;
                data->status = sts;
            } else if(val == best_val) {
                if(sts == player) {
                    /* If it is a winning move, we choose the shortest path. */
                    if(dep < data->depth) data->depth = dep;
                } else {
                    /* Otherwise, we choose the longest path. */
                    if(dep > data->depth) data->depth = dep;
                }
            }

        }
    }

}

/******************************************************* Recursive analyser ***/


/*** Computer personality *****************************************************/

/* Filter the best moves from list into best given it is player's turn. */
void bestmoves(thinkdata *data, movelist *list, movelist *best, int player) {
    move_t *list_end = list->move + list->length;
    int dep;

    best->length = 0;

    if(data->status == player) {
        /* There are winning moves. Choose the fastest ones. */
        dep = 100;
        for(move_t *p = list->move; p != list_end; p++)
            if(p->status == data->status && p->depth < dep) dep = p->depth;
    } else {
        /* There are no winning moves. Choose the slowest ones. */
        dep = -100;
        for(move_t *p = list->move; p != list_end; p++)
            if(p->status == data->status && p->depth > dep) dep = p->depth;
    }

    for(move_t *p = list->move; p != list_end; p++)
        if(p->status == data->status && p->depth == dep)
            addmove(best, p->lin, p->col, p->status, p->depth);

}

/* Choose a move from list randomly. */
void choose_random(movelist *list, int *lin, int *col) {
    int c = rand() % list->length;
    *lin = list->move[c].lin;
    *col = list->move[c].col;
}

/***************************************************** Computer personality ***/


/*** Input/output functions ***************************************************/

/* Displays the board. */
void display(char board[][3]) {
    char line[] = "ABC";
    dprn("\n");
    dprn("   1   2   3 \n");
    for(int i = 0; i < 3; i++) {
        dprn("%c ", line[i]);
        for(int j = 0; j < 2; j++) dprn(" %c |", board[i][j]);
        dprn(" %c \n", board[i][2]);
        if(i != 2) dprn("  -----------\n");
    }
    dprn("\n\n");
}

/* Read a token from terminal (a word with no spaces) */
char *input(void) {
    char buffer[32];
    static char token[32];
    fgets(buffer, sizeof(buffer), stdin);
    if(sscanf(buffer, "%31s", token) != 1) token[0] = '\0';
    return token;
}

char *read_user_command(int player) {
    dprn("Move for player %c> ", player);
    return input();
}

/* Ask a question and reads a 1 letter answer (printf syntax). The
   allowed answers are in the string opt. */
__attribute__ ((format (printf, 2, 3)))
char *ask_options(char *opt, char *format, ...) {
    va_list ap;
    char *s;

    for(;;) {
        va_start(ap, format);
        vprintf(format, ap);
        va_end(ap);
        s = input();
        if(s[0] != '\0' && strchr(opt, s[0]) && s[1] == '\0') break;
        dprn("Invalid option.\n");
    }

    return s;
}

/*************************************************** Input/output functions ***/


/*** Encode/decode moves ******************************************************/

/* Given a move in the form A1, A2, A3, B1, B2, B3, C1, C2 or C3,
   decode it in line and column (0, 1 or 2). Returns 0 if decoding was
   impossible. */
int decode_move(char *s, int *lin, int *col) {
    if(strlen(s) != 2 || !strchr("ABCabc", s[0]) || !strchr("123", s[1]))
        return 0;
    *lin = toupper(s[0]) - 'A';
    *col = s[1] - '1';
    return 1;
}

/* Encode the integer pair lin, col into a string in the form accepted
   by decode_move(). */
char *encode_move(int lin, int col) {
    static char buffer[3];
    sprintf(buffer, "%c%d", 'A' + lin, col + 1);
    return buffer;
}

/****************************************************** Encode/decode moves ***/


/*** Game analysis ************************************************************/

/* Auxiliary to print_analysis() */
void print_moves(movelist *list, int sts) {
    move_t *list_end = list->move + list->length;
    int firstline = 1;
    for(move_t *p = list->move; p != list_end; p++) {
        if(p->status != sts) continue;
        if(!firstline) dprn("%s", ", ");
        /* dprn("%s (%d move%s)", encode_move(p->lin, p->col),
           p->depth, plural(p->depth)); */
        dprn("%s(%d)", encode_move(p->lin, p->col), p->depth);
        firstline = 0;
    }
    if(firstline) dprn("None");
    dprn("\n");
}

/* Print an analysis of the current confuguration of the board given
   it is player's turn. */
void print_analysis(thinkdata *data, movelist *list, int player) {
    dprn("\n");
    dprn("Analysis for player %c:\n", player);
    dprn("Number of moves until game end are between parentheses.\n");
    dprn("  Winning moves: ");
    print_moves(list, player);
    dprn("  Drawing moves: ");
    print_moves(list, 'D');
    dprn("  Loosing moves: ");
    print_moves(list, opponent(player));
    dprn("  Total analysed moves: %d\n", data->move_count);
    dprn("\n");
}

/* Think and print analysis */
void computer_analysis(char board[][3], int player) {
    thinkdata data;
    movelist list;

    think(board, &data, &list, player);
    print_analysis(&data, &list, player);
}

/************************************************************ Game analysis ***/


/*** Game play ****************************************************************/

void computer_plays(char board[][3], int player) {
    int lin, col;
    thinkdata data;
    movelist list, best;

    dprn("Computer playing as %c.\n", player);

    think(board, &data, &list, player);
    bestmoves(&data, &list, &best, player);
    choose_random(&best, &lin, &col);

    dprn("Move for player %c> %s\n", player, encode_move(lin, col));

    board[lin][col] = player;
}

/* Reads a move, checks its validity and executes it. */
int human_plays(char *cmd, char board[][3], int player) {
    int lin, col;

    if(!decode_move(cmd, &lin, &col)) {
        dprn("Syntax error, try again.\n");
        return 0;
    }

    if(board[lin][col] != ' ') {
        dprn("Invalid move, try again.\n");
        return 0;
    };

    board[lin][col] = player;

    return 1;
}

/* Detects end of game. */
int end_of_game(char board[][3]) {
    int sts = status(board);

    switch(sts) {
    case 0: return 0;
    case 'D':
        dprn("Draw.\n");
        break;
    default:
        dprn("%c wins.\n", sts);
    }

    return 1;
}

/* Plays a full game. */
void game(int computer_plays_X, int computer_plays_O) {
    int player = 'X';

    char board[][3] = {
        {' ', ' ', ' '},
        {' ', ' ', ' '},
        {' ', ' ', ' '},
    };

    display(board);

    for(;;) {

        if((player == 'X' && computer_plays_X) ||
           (player == 'O' && computer_plays_O)) {

            /* Delay before computer move. This makes the output
               easier to follow. */
            msleep(1000);
            computer_plays(board, player);

        } else {

            char *cmd = read_user_command(player);

            if(streq(cmd, "g")) return;
            if(streq(cmd, "q")) exit(0);

            if(streq(cmd, "a")) {
                computer_analysis(board, player);
                continue;
            }

            if(streq(cmd, "c")) {
                computer_plays(board, player);
            } else {
                if(!human_plays(cmd, board, player)) continue;
            }

        }

        display(board);

        if(end_of_game(board)) break;

        player = opponent(player);
    }

}

/**************************************************************** Game play ***/


/*** Main *********************************************************************/

void show_usage(void) {
    dprn("Usage: velha [-n <number of pĺayers>]\n");
    dprn("Number of players:\n");
    dprn("  0: Computer against itself.\n");
    dprn("  1: You against the computer (default).\n");
    dprn("  2: Two human players (analysis mode).\n");
    exit(0);
}

void show_commands(void) {
    dprn("Move commands are A1, A2, A3, B1, B2, B3, C1, C2 or C3\n");
    dprn("The player can also enter the commands:\n");
    dprn("  A for game analysis,\n");
    dprn("  C for computer generated move.\n");
    dprn("  G to give up the game.\n");
    dprn("  Q to quit the program.\n");
    dprn("\n");
}

void show_contenders(int computer_plays_X, int computer_plays_O) {
    if(computer_plays_X) {
        if(computer_plays_O) {
            dprn("Computer plays both X and O\n");
        } else {
            dprn("Computer plays as X\n");
            dprn("You play as O\n");
        }
    } else {
        if(computer_plays_O) {
            dprn("You play as X\n");
            dprn("Computer plays as O\n");
        } else {
            dprn("You play both X and O -- analysis mode\n");
        }
    }
    dprn("\n");
}

int main(int argc, char **argv) {

    int computer_plays_X, computer_plays_O;
    int opt, nplayers = 1;

    while((opt = getopt(argc, argv, "n:h")) != -1) {
        switch(opt) {
        case 'n':
            nplayers = atoi(optarg);
            break;
        case 'h':
        default:
            show_usage();
        }
    }

    switch(nplayers) {
    case 0:
        computer_plays_X = 1;
        computer_plays_O = 1;
        break;
    case 1:
        computer_plays_X = 0;
        computer_plays_O = 1;
        break;
    case 2:
        computer_plays_X = 0;
        computer_plays_O = 0;
        break;
    }

    srand(time(0));

    dprn("\n");
    dprn("                --- Tic-Tac-Toe ---\n");
    dprn("\n");

    if(!computer_plays_X || !computer_plays_O) show_commands();

    for(;;) {
        show_contenders(computer_plays_X, computer_plays_O);

        game(computer_plays_X, computer_plays_O);

        if(streq(ask_options("YyNn", "Play again? (Y/N) "), "n")) break;

        dprn("\nNew game.\n\n");

        if(computer_plays_X != computer_plays_O) {
            computer_plays_X = !computer_plays_X;
            computer_plays_O = !computer_plays_O;
        }
    }

    dprn("Good bye.\n");

    return 0;
}

/********************************************************************* Main ***/