/**
 * @file model.h
 * @author LSE
 * @brief 
 * @version 0.1
 * @date 2022-04-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */

// This is a standard TensorFlow Lite FlatBuffer model file that has been
// converted into a C data array, so it can be easily compiled into a binary
// for devices that don't have a file system. It was created using the command:
// xxd -i model.tflite > model.cc

#ifndef TENSORFLOW_LITE_MICRO_ANOMALIA_MICRO_FEATURES_MODEL_H_
#define TENSORFLOW_LITE_MICRO_ANOMALIA_MICRO_FEATURES_MODEL_H_

extern const unsigned char g_model[];
extern const int g_model_len;

#endif  // TENSORFLOW_LITE_MICRO_EXAMPLES_ANOMALIA_MICRO_FEATURES_MODEL_H_
