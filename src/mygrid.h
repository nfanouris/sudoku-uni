#ifndef MYGRID_H
#define MYGRID_H

/**
 * The purpose of the assignment was the development 
 * of a programm using recursion and stack memory.
 */


/**
 * @brief Representation of a single Sudoku cell.
 *
 * The available values are represented by the lower nine bits of
 * avail_mask:
 *
 *     bit 0 -> value 1
 *     bit 1 -> value 2
 *     ...
 *     bit 8 -> value 9
 *
 * value stores the currently assigned value (0 if unresolved), while
 * num_choices stores the number of set bits in avail_mask.
 *
 * The field types are chosen so that Cell_T occupies 4 bytes on the
 * target implementation.
 */
typedef struct cell_s {
    unsigned short avail_mask;
    unsigned char value;
    unsigned char num_choices;
} Cell_T;


/**
 * @brief Internal representation of a Sudoku grid.
 *
 * In addition to the state of the individual cells, the structure
 * maintains bitmasks for the values already present in each row,
 * column and 3x3 box.
 *
 * The constraint masks allow grid_update() to update candidate
 * information incrementally instead of recomputing constraints from
 * the complete grid.
 *
 * The structure also stores solver and generator state, 
 * keeping grid-related stae self-contained
 * witout relying on global variables.
 *
 * sizeof(Grid_T) is target-ABI dependent and should be verified with
 * sizeof(Grid_T) when making assumptions about its memory footprint.
 */
typedef struct grid_s {
    Cell_T cell[81];

    unsigned short rows[9];
    unsigned short cols[9];
    unsigned short boxes[9];

    unsigned char unique;
    unsigned char random_mode;
    unsigned char empty_cells;
} Grid_T;


/**
 * @brief Representation of a choice-location in Sudoku grid.
 */
typedef struct choice_s{
  unsigned char i;  /* Row index (0-8) */
  unsigned char j;  /* Column index (0-8) */
  unsigned char n;  /* Value (1-9), or 0 if empty value */
} Choice_T;




/**
 * @brief Enables or disables the internal random mode for generation.
 * @param g The current grid state.
 * @param mode 1 to enable random mode, 0 to disable.
 * @return Grid_T A new grid object with the updated mode.
 */
Grid_T grid_set_random_mode(Grid_T g, int mode);



/**
 * @brief Initializes a grid from a 2D integer array.
 * @param g An uninitialized grid object.
 * @param v A 9x9 array containing the initial values (0 for empty).
 * @return Grid_T The fully initialized grid with pre-calculated bitmasks.
 */
Grid_T grid_init(Grid_T g, int v[9][9]);		/* init g with values from array v */


/**
 * @brief Applies a choice to the grid and updates all relevant peer bitmasks.
 * @param g The current grid state.
 * @param c The choice to apply (coordinates and value).
 * @return Grid_T A new grid object reflecting the applied choice.
 */
Grid_T grid_update(Grid_T g, Choice_T c);	



/**
 * @brief Iterates over choices or finds the next best cell to solve.
 * If t.n == 0: 
 * Acts as a heuristic search, returning the cell with the minimum
 * available choices. If random_mode is 1, it breaks ties randomly.
 * If t.n != 0: 
 * Returns the next valid choice for cell (t.i, t.j) strictly greater than t.n.
 * *
 * @param g The current grid state.
 * @param t The choice state to base the iteration on.
 * @return Choice_T The next valid choice, or (0,0,0) if no choices are left / puzzle is full.
 */
Choice_T grid_iterate(Grid_T g, Choice_T t);


/**
 * @brief Checks if the grid was solved using strict deductive steps.
 * @param g The grid state to check.
 * @return int 1 if unique (no backtracking guesses), 0 otherwise.
 */
int grid_unique(Grid_T g);	


/**
 * @brief Finds a cell that has exactly one valid choice left.
 * @param g The current grid state.
 * @return Choice_T The unique choice, or (0,0,0) if none exists.
 */
Choice_T grid_exist_unique(Grid_T g);


/**
 * @brief Clears the uniqueness flag (used when the solver is forced to guess).
 * @param g The current grid state.
 * @return Grid_T A new grid object with the unique flag set to 0.
 */
Grid_T grid_clear_unique(Grid_T g);	


/**
 * @brief Reads the value of a specific cell.
 * @param g The current grid state.
 * @param c A choice object containing the coordinates (c.i, c.j) to read.
 * @return Choice_T The same object with c.n updated to the cell's value.
 */
Choice_T grid_read_value(Grid_T g, Choice_T c);
#ifdef DEBUG
void grid_cell_print(FILE *stream, Grid_T g, Choice_T c);
#endif

#endif
