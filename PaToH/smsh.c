/*
 *  Implementation of quicksort algorithm and some other functions
 *  to be used with elements of matrices in coordinate format.
 *  A version that also includes values of the matrix is also available,
 *  however since we don't use values with PaToH, it is ommited in 
 *  this file. Please contact me if you need it.
 * 
 *  This implementation is adapted from Dr. Kevin Browne's lecture 
 *  on quicksort algorithm.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void indexSwap(int *x, int *y);

void quicksort(int arrayI[], int arrayJ[], int length);

void quicksort_recursion(int arrayI[], int arrayJ[], int low, int high);

int partition(int arrayI[], int arrayJ[], int low, int high);


bool isSorted(int arrayI[], int arrayJ[], int arrayLength);
bool isAfter(int x_I, int x_J, int y_I, int y_J);
long Set_Priority = 2147483647;


void indexSwap(int *x, int *y)
{
  int temp = *x;
  *x = *y;
  *y = temp;
}

void quicksort(int arrayI[], int arrayJ[], int length)
{

  srand(time(NULL));
  
  quicksort_recursion(arrayI, arrayJ, 0, length - 1);
}


void quicksort_recursion(int arrayI[], int arrayJ[], int low, int high)
{

  if (low < high)
  {
    int pivot_index = partition(arrayI, arrayJ, low, high);

    quicksort_recursion(arrayI, arrayJ, low, pivot_index - 1);

    quicksort_recursion(arrayI, arrayJ, pivot_index + 1, high);
  }
}

int partition(int arrayI[], int arrayJ[], int low, int high)
{

  int pivot_index = low + (rand() % (high - low));

  if (pivot_index != high) {
    indexSwap(&arrayI[pivot_index], &arrayI[high]);
    indexSwap(&arrayJ[pivot_index], &arrayJ[high]);
  }

  long pivot_value = arrayI[high] + Set_Priority*arrayJ[high];

  int i = low; 
  
  for (int j = low; j < high; j++)
  {

    if ((arrayI[j] + arrayJ[j] * Set_Priority) <= pivot_value)
    {
      indexSwap(&arrayI[i], &arrayI[j]);
      indexSwap(&arrayJ[i], &arrayJ[j]);
      i++;
    }
  }
  
  indexSwap(&arrayI[i], &arrayI[high]);
  indexSwap(&arrayJ[i], &arrayJ[high]);
  
  return i;
}

bool isSorted(int arrayI[], int arrayJ[], int arrayLength)
{
  for (int i = 0; i < arrayLength - 1; i++){
    if(!isAfter(arrayI[i], arrayJ[i], arrayI[i + 1], arrayJ[i + 1])){
      return false;
    }
  }
  return true;
}

bool isAfter(int x_I, int x_J, int y_I, int y_J)
{
  if(x_J < y_J) {
    return true;
  } else if (x_J > y_J) {
    return false;
  } else if (x_I < y_I) {
    return true;
  } else {
    return false;
  }
}