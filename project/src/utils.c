#include "utils.h"

// Checks if there is exactly one true value in the 2D array
bool isSingleElementTrue(bool arr[N_FLOORS][N_BUTTONS]) {
  int number_of_trues = 0;

  for (int f = 0; f < N_FLOORS; f++) {
    for (int b = 0; b < N_BUTTONS; b++) {
      if (arr[f][b]) { 
        number_of_trues++;
        if (number_of_trues > 1) return false;
      }
    }
  }
  if (number_of_trues == 0) return false;
  return true;
}

// Checks if all values in the 2D array are false (0)
bool areAllElementsFalse(bool arr[N_FLOORS][N_BUTTONS]) {
  for (int f = 0; f < N_FLOORS; f++) {
    for (int b = 0; b < N_BUTTONS; b++) {
      if (arr[f][b]) return false;
    }
  }
  return true;
}
