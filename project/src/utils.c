#include "utils.h"

// Checks if there is exactly one true value in the 2D array
bool isSingleElementTrue(int n_a, int n_b, bool arr[n_a][n_b]) {
    bool *ptr = &arr[0][0]; // Pointer to the first element
    int total_elements = n_a * n_b;
    
    int number_of_trues = 0;
    for (int i = 0; i < total_elements; i++) {
        if (ptr[i]) {
            number_of_trues++;
            if (number_of_trues > 1) return false;
        }
    }
    if (number_of_trues == 0) return false;
    return true;
}

// Checks if all values in the 2D array are false (0)
bool areAllElementsFalse(int n_a, int n_b, bool arr[n_a][n_b]) {
    bool *ptr = &arr[0][0]; // Pointer to the first element
    int total_elements = n_a * n_b;

    for (int i = 0; i < total_elements; i++) {
        if (ptr[i]) {
            return false;
        }
    }
    return true;
}

