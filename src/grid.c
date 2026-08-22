#include <stdlib.h>
#include <assert.h>
#include "mygrid.h"


Grid_T grid_init(Grid_T g, int v[9][9]){
  Choice_T c;
  int i, j;
  

  /* Initialization of grid with '0' */
  for (i = 0; i < 9; i++){
    g.rows[i] = 0;
    g.cols[i] = 0;
    g.boxes[i] = 0;
  }

  for (i = 0; i < 81; i++){
    g.cell[i].value = 0;
    g.cell[i].avail_mask = 0x1FF; /* turn on the 9 bits */
    g.cell[i].num_choices = 9;
  }
  g.unique = 1;
  g.random_mode = 0;


  g.empty_cells = 81;
  /* Data insertion */
  for (i = 0; i < 9; i++){
    for (j = 0; j < 9; j++){

      
      assert(0 <= v[i][j] && v[i][j] <= 9);
      
      c.n = v[i][j];
      
      if (c.n != 0){
        c.i = i;
        c.j = j;
        g = grid_update(g, c);
      }
    }
  }

  return g;
}


Grid_T grid_set_random_mode(Grid_T g, int mode){
  g.random_mode = (unsigned char)mode;
  return g;
}


/* Find the index of the bit that gives me info 
 * of the value I can use, and translate it to integer */
static int find_first_bit(unsigned short mask) {
  int bit_index = 1;

  /* If mask == 0 , infinite loop */
  assert(mask != 0);

  while ((mask & 1) == 0){
    mask >>= 1;
    bit_index++;
  }
  return bit_index;
}


Grid_T grid_update(Grid_T g, Choice_T c){

  /* These 2 exist in order to avoid the 
   * div and mod instructions 
   * (waste memory for gaining speed - less CPU cycles) */
  const int div_3[9] = {0, 0, 0, 1, 1, 1, 2, 2, 2};
  const int mod_3[9] = {0, 1, 2, 0, 1, 2, 0, 1, 2};
  int idx, k;
  unsigned short mask_to_remove;
  int box_start_i, box_start_j, box_idx;
  int r_idx, c_idx, b_idx, b_i, b_j;



  if (c.n == 0){
    return g;
  }

  /* Coordinates in limits  */
  assert(c.i >= 0 && c.i < 9);
  assert(c.j >= 0 && c.j < 9);

  /* value in [1,9] */
  assert(c.n >= 1 && c.n <= 9);


  idx = c.i * 9 + c.j; 
  assert(0 <= idx && idx < 81);

  /* zero mask with 1 in the index of the c.n value */
  mask_to_remove = (unsigned short) (1U << (c.n - 1));  
  /* not nessesary casts but... just to be safe */

  
  box_start_i = div_3[c.i] * 3; 
  box_start_j = div_3[c.j] * 3
  ;
  /* Calculate 3x3 box index (0-8):*/
  box_idx = box_start_i + div_3[c.j];       /*(c.i / 3) * 3 + (c.j / 3);*/

  /* Update the cell */
  g.cell[idx].value = c.n;
  g.cell[idx].num_choices = 0;
  g.cell[idx].avail_mask = 0; 

  /* Inform grid that it has less empty cells */
  g.empty_cells--;

  /* Update the grid masks (OR with 0..010..0 at the index of the value)*/
  g.rows[c.i] |= mask_to_remove;
  g.cols[c.j] |= mask_to_remove;
  g.boxes[box_idx] |= mask_to_remove;

  /* 2. Update Row peers */


  /* Here I started with multiple loops of informing the peers,
   * later, I saw them following the same logic and thought:
   * "I could just make a function for that..."
   * but I needed to call the function 3 times /per grid_update 
   * call, passing by value the whole grid etc so, 
   * I decided to avoid it while at the same time 
   * exploiting the common traits */

  for (k = 0; k < 9; k++){
    /* Calculate the 3 different indicies */
    r_idx = c.i * 9 + k;    /* realated row */
    c_idx = k * 9 + c.j;    /* related column */

    
    b_i = box_start_i + div_3[k];    /* Row of related in the same subgrid */
    b_j = box_start_j + mod_3[k];    /* Col of related in the same subgrid */
    b_idx = b_i * 9 + b_j;          /* idx of the related in the same subgrid */




    if (k != c.j && (g.cell[r_idx].avail_mask & mask_to_remove)){
      g.cell[r_idx].avail_mask &= ~mask_to_remove;
      g.cell[r_idx].num_choices--;
    }

    if (k != c.i && (g.cell[c_idx].avail_mask & mask_to_remove)){
      g.cell[c_idx].avail_mask &= ~mask_to_remove;
      g.cell[c_idx].num_choices--;
    }

    if (!(b_i == c.i && b_j == c.j) /*exclude the targeted cell*/ && (g.cell[b_idx].avail_mask & mask_to_remove)){
      g.cell[b_idx].avail_mask &= ~mask_to_remove;
      g.cell[b_idx].num_choices--;
    }
  } 
  return g;
}




Choice_T grid_iterate(Grid_T g, Choice_T t){
  int idx, best_idx = -1;
  int min_choices = 10;
  
  Choice_T result = {0, 0 ,0};
  int bit_index;

  /* used for random mode */
  int ties_count = 0;


  if (t.n == 0) {
  /* When it takes no value, gets activated in "sesarch mode" 
   * looking for the best possible cell options 
   * - cells with least options */  

    for (idx = 0; idx < 81; idx++){

      /* already filled value */
      if (g.cell[idx].value != 0) continue;
 
      /* if cell with no possible input */
      if (g.cell[idx].num_choices == 0) return result; /* dead-end */       
    
      /* * NOTE: I deliberately DO NOT use grid_exist_unique(g) here.
       * Calling it would return a cell with 1 choice, potentially ignoring 
       * another empty cell with 0 choices (a dead-end). 
       * A single-pass loop guarantees we catch dead-ends BEFORE making any early exits.
       */

      /* Early exit I found an "ideal" cell */
      if (g.cell[idx].num_choices == 1){
        result.i = idx / 9;
        result.j = idx % 9;
        result.n = find_first_bit(g.cell[idx].avail_mask);
        return result;
      }

      if (g.cell[idx].num_choices < min_choices){

        min_choices = g.cell[idx].num_choices;
        best_idx = idx;
        ties_count = 1;

      } 
      else if (g.cell[idx].num_choices == min_choices) { 
        if (g.random_mode == 1){
          ties_count++;
        

          if(rand() % ties_count == 0){  /* random factor */
            best_idx = idx;
            /* Here, code dives deep, I recognise its bad practice
            * since its for a single command , I hope its okay*/
          }
        }
      }
    }

    if (best_idx != -1) {
      result.i = best_idx / 9;
      result.j = best_idx % 9;
      result.n = find_first_bit(g.cell[best_idx].avail_mask) ;
      return result;
    }
    return result; /* (0, 0, 0) solved or full */

  } else {
    /* t.n != 0 (2nd mode, next backtracking step)*/
    bit_index = t.n + 1;
    idx = t.i * 9 + t.j;

    while (bit_index <= 9){
      /* Checking if the bit is '1' in the already 
       * computed cell's avail_mask */
      if(g.cell[idx].avail_mask & (1U << (bit_index - 1))){
        t.n = bit_index;
        return t;
      }
      bit_index++;
    }

    return result;
  }  
}


int grid_unique(Grid_T g){
  return g.unique;
}

Grid_T grid_clear_unique(Grid_T g){
  g.unique = 0;
  return g;
}

Choice_T grid_read_value(Grid_T g, Choice_T c) {
  int idx ;
  idx = c.i * 9 + c.j;
  c.n = g.cell[idx].value;
  return c;
}

Choice_T grid_exist_unique(Grid_T g){
  Choice_T c = {0, 0, 0};
  int idx;

  for (idx = 0; idx < 81; idx++){
    if (g.cell[idx].value == 0 && g.cell[idx].num_choices == 1){
      c.i = idx / 9;
      c.j = idx % 9;
      c.n = find_first_bit(g.cell[idx].avail_mask);
      return c;
    }
  }
  return c;
}
