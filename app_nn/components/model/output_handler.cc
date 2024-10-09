/**
 * @file output_handler.cc
 * @author LSE
 * @brief 
 * @version 0.1
 * @date 2022-04-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "output_handler.h"
#include <algorithm>
#include <stdio.h>
#include <vector>


void HandleOutput(tflite::ErrorReporter* error_reporter, std::vector<float> saida) {
  double max_prob = *std::max_element(saida.begin(), saida.end());
  //std::cout << "Max Probability: " << max_prob << std::endl;
  size_t class_index = std::distance(saida.begin(), std::max_element(saida.begin(),saida.end()));
  //std::cout << "Index: " << ataque_class_index << std::endl;

  printf("max prob: %f, class %s\n",max_prob, name_classes[class_index]);

  printf("vector saida [%f , %f, %f ]\n", saida[0], saida[1],saida[2]);

  printf("vector saida sum : %f\n", saida[0]+saida[1]+saida[2]);
}

void HandleOutput_format(tflite::ErrorReporter* error_reporter, std::vector<float> saida, bool relay, float conf, uint8_t* class_index_detection) {
  double max_prob = *std::max_element(saida.begin(), saida.end());
  //std::cout << "Max Probability: " << max_prob << std::endl;
  size_t class_index = std::distance(saida.begin(), std::max_element(saida.begin(),saida.end()));
  *class_index_detection = (uint8_t) class_index;
  //std::cout << "Index: " << ataque_class_index << std::endl;

  printf("%f, %f, %f, %d, %f, %d\n", saida[0], saida[1],saida[2], relay, conf, class_index);

}