/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "main_functions.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "constants.h"
#include "model.h"
#include "output_handler.h"
#include "policy.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "esp_system.h" //para print de memory no heap

// Globals, used for compatibility with Arduino-style sketches.
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input_0 = nullptr;
TfLiteTensor* input_1 = nullptr;
TfLiteTensor* input_2 = nullptr;
TfLiteTensor* output = nullptr;
int inference_count = 0;

Weighted_Policies policies_guardiao = Weighted_Policies(
        90., 2., 2., false, 4., 5., 70., 100.);

std::vector<std::vector<float>> pred_groups;

//constexpr int kTensorArenaSize = 159864;
constexpr int kTensorArenaSize = 158400;
uint8_t tensor_arena[kTensorArenaSize];

}  // namespace

// The name of this function is important for Arduino compatibility.
void setup() {
  tflite::InitializeTarget();

  // Set up logging. Google style is to avoid globals or statics because of
  // lifetime uncertainty, but since this has a trivial destructor it's okay.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }


  // This pulls in all the operation implementations we need.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::AllOpsResolver resolver;
  // tflite::MicroMutableOpResolver<5> micro_op_resolver;
  // micro_op_resolver.AddConv2D();
  // micro_op_resolver.AddMaxPool2D();
  // micro_op_resolver.AddReshape();
  // micro_op_resolver.AddFullyConnected();
  // micro_op_resolver.AddSoftmax();

  // Build an interpreter to run the model with.
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }


  // Obtain pointers to the model's input and output tensors.
  input_0 = interpreter->input(0);
  input_1 = interpreter->input(1);
  input_2 = interpreter->input(2);
  output = interpreter->output(0);

  // Keep track of how many inferences we have performed.
  inference_count = 0;
}


void detect(volatile float *entrada_ir, volatile float entrada_acc[50][3], volatile float* entrada_mic, uint8_t *predict_class, uint8_t *class_index_detection)
{
  //TF_LITE_REPORT_ERROR(error_reporter, "start detect\n");
  //TF_LITE_REPORT_ERROR(error_reporter, "RAM left %d B", esp_get_free_heap_size());

  //pre processo
  //TF_LITE_REPORT_ERROR(error_reporter, "dstack start\n");
    std::vector<std::vector<float>> processed_input;

  //dstack equivalent
    for(int i = 0; i < 50;i++)
    {
      //printf("IR - %f \n", entrada_ir[i]);
      //printf("Audio - %0.8f \n", entrada_mic[i]);
      std::vector<float> t{entrada_acc[i][0],entrada_acc[i][1],entrada_acc[i][2]};
      processed_input.push_back(t);
    }
    /* for (size_t i = 0; i < 1000; i++)
    {
      printf("Audio - %0.8f \n", entrada_mic[i]);
    } */
    

  //TF_LITE_REPORT_ERROR(error_reporter, "dstack done\n");
  //center

    auto center_result = center_norm(processed_input);

    /* printf("entrada 1  dim[0]: %d \n", input_1->dims->data[0]); // 1
    printf("entrada 1 dim[1]: %d \n", input_1->dims->data[1]);  // 50
    printf("entrada 1 dim[2]: %d \n", input_1->dims->data[2]);  // 3
 */
    float *input_data_ptr_index_1 = interpreter->typed_input_tensor<float>(1); // inverted idx
    for (int i = 0; i < input_1->dims->data[1]; ++i)                           // dim = 50
    {
      *(input_data_ptr_index_1) = entrada_ir[i];
      input_data_ptr_index_1++;
    }

    /* printf("entrada 0  dim[0]: %d \n", input_0->dims->data[0]); // 1

    printf("entrada 0 dim[1]: %d \n", input_0->dims->data[1]); // 50
    printf("entrada 0 dim[2]: %d \n", input_0->dims->data[2]); // 3 */

    float *input_data_ptr_index_0 = interpreter->typed_input_tensor<float>(0);
    for (int i = 0; i < input_0->dims->data[1]; ++i) // dim = 50
    {
      for (int j = 0; j < input_0->dims->data[2]; ++j) // dim =3
      {
        *(input_data_ptr_index_0) = center_result[i][j];
        input_data_ptr_index_0++;
      }
    }

    std::vector<std::vector<float>>().swap(center_result); // free temp var from memory
/*   printf("entrada 2  dim[0]: %d \n", input_2->dims->data[0] ); //1
  printf("entrada 2 dim[1]: %d \n", input_2->dims->data[1] ); // 16000
  printf("entrada 2 dim[2]: %d \n", input_2->dims->data[2] ); // 1 */


    float* input_data_ptr_index_2 = interpreter->typed_input_tensor<float>(2);
    for (int i = 0; i < input_2->dims->data[1]; ++i) // dim = 16000
    {
        *(input_data_ptr_index_2) = entrada_mic[i];
        input_data_ptr_index_2++;
    }

  //TF_LITE_REPORT_ERROR(error_reporter, "input read\n");
  // Run inference, and report any error
  //TF_LITE_REPORT_ERROR(error_reporter, "RAM left %d B", esp_get_free_heap_size());

  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed on g_ataque_0\n");
    return;
  }

  //TF_LITE_REPORT_ERROR(error_reporter, "invoke done\n");

  //printf("saida dim[1]: %d \n", output->dims->data[1] );
    float* output_data_ptr = interpreter->typed_output_tensor<float>(0);
    std::vector<float> saida;
    for (int i = 0; i < output->dims->data[1]; ++i) // dim = 3
    {
        saida.push_back(*(output_data_ptr));
        output_data_ptr++;
    }

    pred_groups.push_back(saida);
    if (pred_groups.size() > 3)
    {
      pred_groups.erase(pred_groups.begin());
    }
    
  *predict_class = std::distance(saida.begin(), std::max_element(saida.begin(),saida.end()));
  //printf("PREDIÇÂO é - %d \n", class_index);
  //TF_LITE_REPORT_ERROR(error_reporter, "output read\n");

  //TF_LITE_REPORT_ERROR(error_reporter, "RAM left %d B\n", esp_get_free_heap_size());

  std::vector<std::vector<float>> predictions(pred_groups.size());
  //printf("size %i \n", pred_groups.size());
  std::copy(&(pred_groups.front()), &(pred_groups.front()) + pred_groups.size(), &predictions[0]);
  float conf;
  bool r = policies_guardiao.prediction(predictions, &conf);
  
  HandleOutput_format(error_reporter, saida, r, conf, class_index_detection);

  //clean up
  //delete[] saida;

  //TF_LITE_REPORT_ERROR(error_reporter, "RAM left %d B\n", esp_get_free_heap_size());
  //TF_LITE_REPORT_ERROR(error_reporter, "end detect\n----------\n");
}

bool detect_policies()
{
  //printf("detect policies \n");
  std::vector<std::vector<float>> predictions(pred_groups.size());
  //printf("size %i \n", pred_groups.size());
  std::copy(&(pred_groups.front()), &(pred_groups.front()) + pred_groups.size(), &predictions[0]);
  float conf;
  bool r = policies_guardiao.prediction(predictions, &conf);
  //printf("return policies \n");
  return r;
}

// The name of this function is important for Arduino compatibility.
void loop() {

  // float* buffer_entrada = new float[16000]();
  // for (int i = 0 ; i< 16000; i++)
  // {
  //    // troca depois para apontar para o buffer do microfone
  //   buffer_entrada[i] =  g_ataque_0[i];
  // }
  // detect(buffer_entrada);
}
