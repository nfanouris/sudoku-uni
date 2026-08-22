#include <stdlib.h>
#include <stdio.h>
#include <string.h> /* strcmp */
#include <unistd.h> /* getpid */

#include "mygrid.h"
#include "sudoku.h"



Grid_T sudoku_solve(Grid_T g, Choice_T c){
  Choice_T empty_choice = {0, 0, 0};
  Choice_T backtrac_c, next_c;
  Grid_T returned_g;

  if (c.n != 0){ /* update only when I truly have some data. */
    g = grid_update(g, c);
  }

  /* Looking for the next cell with the least options */
  c = grid_iterate(g, empty_choice);

  /* if there is not other cell (solved or dead-end), return as is */
  if (c.n == 0) return g;

  /* Repeat iteration by pretending the previous one was failed,
   * this way I check if there is more than one choice to fill */
  next_c = grid_iterate(g, c);
  if (next_c.n != 0){
    g = grid_clear_unique(g);
  }



  /* backtracking loop for the possible cell choices */
  backtrac_c = c;
  while (backtrac_c.n != 0){

    returned_g = sudoku_solve(g, backtrac_c);

    /* Check if the recursion returned solved puzzle */
    if (returned_g.empty_cells == 0){
    
      return returned_g; 

    } else {
      /* Failure, Ask for the next possible value form the 'waiter' */
      backtrac_c = grid_iterate(g, backtrac_c);   
    }
  }
  /* If i test every value and it doesnt works, 
   * the caller gave me false info, so I return as is*/
  return g;
}



Grid_T sudoku_read(void){
  Grid_T g;
  int ch, i, j;
  int v[9][9] = {0};

  /* Reading the 81 digits */ 
  i = -1;
  while (++i < 9){
    j = 0;
    while (j < 9) {
       
      ch = getchar();
    
      if(ch == EOF){
        /* If I meet EOF before I read 81 elements */
        return grid_init(g, v);
      }

      if ('0' <= ch && ch <= '9') { 

        /* from: ASCII 
         * to:   int 
         * then: put it in the input struct of the grid_init*/
        v[i][j] = ch - '0';

        j++;
      }
    }
  }

  return grid_init(g, v);
}





void sudoku_print (FILE *s, Grid_T g){
  Choice_T c;
  int i, j;
  
  for (i = 0; i < 9; i++){
    for (j = 0; j < 9; j++){

      c.i = i;
      c.j = j;

      c = grid_read_value(g, c);
    
      /* Print the no. at stream s */
      fprintf(s, "%d", c.n);
    

      /* Check if end of row */
      if (j < 8) {
        fprintf(s, " ");
      }
    }

    fprintf(s, "\n");
  }
}



int sudoku_is_correct(Grid_T g){
  int i, j, val, b_idx;
  Choice_T c;

  unsigned short val_mask;
  unsigned short row_seen[9] = {0};
  unsigned short col_seen[9] = {0};
  unsigned short box_seen[9] = {0};

  for (i = 0; i < 9; i++){
    for (j = 0; j < 9; j++){

      c.i = i;
      c.j = j;
      c= grid_read_value(g, c);
      val = c.n;

      if (val == 0){
        return 0; 
      }

      b_idx = (i / 3) * 3 + (j / 3);

      val_mask = (unsigned short)(1U << val);

    
      if ((row_seen[i] & val_mask) ||
          (col_seen[j] & val_mask) ||
          (box_seen[b_idx] & val_mask)){
        return 0; /* Double element found - error */
      }

      row_seen[i] |= val_mask;
      col_seen[j] |= val_mask;
      box_seen[b_idx] |= val_mask;
    }
  }return 1;
}


void sudoku_print_errors(Grid_T g){
  int i, j, val, b_idx;
  Choice_T c;
  unsigned short val_mask ;
  unsigned short row_seen[9] = {0};
  unsigned short col_seen[9] = {0};
  unsigned short box_seen[9] = {0};

  for (i = 0; i < 9; i++){
    for(j = 0; j < 9; j++){
      c.i = i;
      c.j = j;
      c = grid_read_value(g, c);
      val = c.n;

      if (val == 0){
        fprintf(stderr, "Error: Cell at row %d, col %d is empty.\n", i+1, j+1);
        continue;
      }

      b_idx = (i/3) * 3 + (j/3);
      val_mask = (1U << val);
      if (row_seen[i] & (val_mask))
        fprintf(stderr, "Error: Value %d is duplicated in row %d.\n", val, i+1);
      if (col_seen[j] & (val_mask))
        fprintf(stderr, "Error: Value %d is duplicated in col %d.\n", val, j+1);
      if (box_seen[b_idx] & (val_mask))
        fprintf(stderr, "Error: Value %d is duplicated in box %d.\n", val, b_idx+1);

      row_seen[i] |= (val_mask);
      col_seen[j] |= (val_mask);
      box_seen[b_idx] |= (val_mask);

    }
  }
}



Grid_T sudoku_generate(int nelts, int unique){
  Grid_T g;
  Grid_T test_g;
  const Choice_T empty_choice = {0};
  Choice_T c = {0, 0, 0};
  int v[9][9] = {0};
  int indices[81];
  int i, j, k, idx, temp, r_idx;
  int current_elmnts = 81;
  int row1[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};


  /* Seed: */
  /* Suffling of row_1 */
  for (i = 8; i > 0; i--){
    r_idx = rand() % (i + 1);
    temp = row1[i];
    row1[i] = row1[r_idx];
    row1[r_idx] = temp;
  }
  /* Pass it to the sudoku (pre processed form) */
  for (j = 0; j < 9; j++){
    v[0][j] = row1[j];
  }

  /* The pivot */
  /* Solve, in order to find a valid full grid */ 


  g = grid_init(g, v);

  /* In order to find more random output*/
  g = grid_set_random_mode(g, 1);
  g = sudoku_solve(g, c);

  g = grid_set_random_mode(g, 0);

  /* Export the solved grid to the array v[9][9] */
  for (i = 0; i < 9; i++){
    for (j = 0; j < 9; j++){
      c.i = i; 
      c.j = j;
      c = grid_read_value(g, c);
      v[i][j] = c.n;
    }
  }


  /* Now I have the filled grid, and start the random digging */

  /* Init an array of [0, 1, 2, 3 ... , 79, 80] 
   * the values of it, give me the index i will try to dig
   * but I will mix the assortment
   * */
  for (k = 0; k < 81; k++){
    indices[k] = k;
  }

  /* shuffle */
  for (i = 80; i > 0; i--){ /* the i++ way is more expensive */
    r_idx = rand() % (i + 1);
    temp = indices[i];
    indices[i] = indices[r_idx];
    indices[r_idx] = temp;
  }

  /* start digging, following the instractions from the indices array */
  for (k = 0; k < 81 && current_elmnts > nelts; k++){
    idx = indices[k];
    i = idx / 9;
    j = idx % 9;

    temp = v[i][j]; /* Backup */
    v[i][j] = 0;    /*  dig   */

    if (unique == 1) {
      test_g = grid_init(test_g, v);
      test_g = sudoku_solve(test_g, empty_choice);

      /* If solver zeroed the unique flag, undo the digging */
      if (grid_unique(test_g) == 0){
        v[i][j] = temp;
        continue;
      }
    } 
    current_elmnts--;
  }

  /* Export */
  g = grid_init(g, v);

  return g;

}
 
int main(int argc, char *argv[]) {
  Grid_T g;  
  const Choice_T empty_choice = {0};
  int nelts;
  int unique = 0; 


  /* randomness throw process ID, breaks the 
   * limit of "more than once in a sec" */
  srand(getpid()); 


  /* Case 1, just 'sudoku' */
  if (argc == 1){
    g = sudoku_read();
    sudoku_print(stderr, g); /* Print input at stderr */
  
    g = sudoku_solve(g, empty_choice);

    if (g.empty_cells != 0 || !sudoku_is_correct(g)){
      fprintf(stderr, "Puzzle has no solution.\n");
    } else if (grid_unique(g)){
      fprintf(stderr, "Puzzle has a unique choice solution.\n");
    } else {
      fprintf(stderr, "Puzzle has multiple choices solution.\n");
    }
    sudoku_print(stdout, g); /* Print solution in stdout */
  }

  /* Case 2: 'sudoku -c' */
  else if (argc == 2 && strcmp(argv[1], "-c") == 0) {
    g = sudoku_read();
    sudoku_print(stderr, g);

    if (sudoku_is_correct(g)){
      fprintf(stderr, "Puzzle is correct.\n");
    } else {
      fprintf(stderr, "Puzzle is incorrect. Errors found:\n");
      sudoku_print_errors(g);
    }
  }

  /* Case 3: 'sudoku -g nelts -u' */
  else if (argc >= 3 && strcmp(argv[1], "-g") == 0) {
    nelts = atoi(argv[2]);
    if (argc == 4 && strcmp(argv[3], "-u") == 0){
      unique = 1;
    }
    g = sudoku_generate(nelts, unique);
    sudoku_print(stdout, g);
  }

  else {
    fprintf(stderr, "Usage: sudoku [-c] | [-g nelts -u]\n");
    return 1;
  }
  return 0;
}
