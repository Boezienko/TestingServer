#ifndef TESTDATA_H
#define TESTDATA_H

#include "testCase.h"

#define INITIAL_CAPACITY 100

typedef struct TestData {
    int size; // number testcases currently in our list
    int capacity; // number of testcases able to be stored
    TestCase* testCases; // list of testcases
    
    // pointer functions for testData to use
    void (*initialize)(struct TestData*);
    void (*addTestCase)(struct TestData*, const char*, const char*);
    void (*free)(struct TestData*);
    TestCase* (*getTestCase)(struct TestData*, int index);
} TestData;

// declaring functions
void initializeTestData(TestData* data);
void addTestCase(TestData* data, const char* input, const char* expectedOutput);
void freeTestData(TestData* data);
TestCase* getTestCase(TestData* data, int index);

// defining functions

// initializing function
void initializeTestData(TestData* data){
  data->size = 0;
  data->capacity = INITIAL_CAPACITY;
  data->testCases = malloc(data->capacity * sizeof(TestCase));
  
  // assigning function pointers
  data->initialize = initializeTestData;
  data->addTestCase = addTestCase;
  data->free = freeTestData;
  data->getTestCase = getTestCase;  
}

// to add a testcase to the list
void addTestCase(TestData* data, const char* input, const char* expectedOutput){
  // make sure we have enough allocated space
  if(data->size >= data->capacity){
    // double the capacity and reallocate for that much space
    data->capacity *= 2;
    data->testCases = realloc(data->testCases, data->capacity * sizeof(TestCase));
  }
  
  // adding new testcase to our array that can hold it
  data->testCases[data->size].input = strdup(input);
  data->testCases[data->size].expectedOutput = strdup(expectedOutput);
  
  data->size++;
}

void freeTestData(TestData* data){
  for(int i = 0; i < data->size; i++){
    free(data->testCases[i].input);
    free(data->testCases[i].expectedOutput);
  }
  free(data->testCases);
}

TestCase* getTestCase(TestData* data, int index){
  // incase its empty or out of bounds
  if(index < 0 || index >= data->size){
    return NULL;
  }
  return &data->testCases[index];
}

#endif
