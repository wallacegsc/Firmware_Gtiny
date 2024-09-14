/**
 * @file output_handler.h
 * @author LSE
 * @brief 
 * @version 0.1
 * @date 2022-04-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef TENSORFLOW_LITE_MICRO_OUTPUT_HANDLER_H_
#define TENSORFLOW_LITE_MICRO_OUTPUT_HANDLER_H_

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "constants.h"
#include <vector>

// Called by the main loop to produce some output based on the x and y values
void HandleOutput(tflite::ErrorReporter* error_reporter, std::vector<float> saida);
void HandleOutput_format(tflite::ErrorReporter* error_reporter, std::vector<float> saida, bool relay, float conf, uint8_t* class_index_detection);

#endif  // TENSORFLOW_LITE_MICRO_EXAMPLES_HELLO_WORLD_OUTPUT_HANDLER_H_
