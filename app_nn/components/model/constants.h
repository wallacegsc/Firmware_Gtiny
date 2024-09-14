/**
 * @file constants.h
 * @author LSE
 * @brief 
 * @version 0.1
 * @date 2022-04-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef TENSORFLOW_LITE_MICRO_CONSTANTS_H_
#define TENSORFLOW_LITE_MICRO_CONSTANTS_H_

// This constant represents the range of x values our model was trained on,
// which is from 0 to (2 * Pi). We approximate Pi to avoid requiring additional
// libraries.
#define NUM_CLASSES_ANOMALIA 3
extern const int kInferencesPerCycle;
extern const char* name_classes[NUM_CLASSES_ANOMALIA];

#endif  // TENSORFLOW_LITE_MICRO_EXAMPLES_HELLO_WORLD_CONSTANTS_H_
