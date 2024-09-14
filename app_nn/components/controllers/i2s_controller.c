#include "i2s_controller.h"
#include "esp_dsp.h"

//#define FILTER_DSP 1

#ifdef FILTER_DSP
    #define FC (1000.0)
    #define FS (16000.0)
    #define FREQ (FC/FS)
    #define QFACTOR (1.0)
    float coeffs_hpf[5];
    float w_hpf[5] = {0,0};
#endif    

struct i2s_controller_data_t *i2s_controller_data = &i2s_data_controller;
struct i2s_controller_t i2s_controller;

esp_err_t i2s_init(void)
{
    
    #ifdef FILTER_DSP
        dsps_biquad_gen_hpf_f32(coeffs_hpf, FREQ, QFACTOR);
    #endif

    return ESP_OK;
}

i2s_port_t getI2SPort(void)
{
    return i2s_controller_data->m_i2sPort;
}

float *getCapturedAudioBuffer(void)
{
    //return i2s_controller_data->m_currentAudioBuffer;
    return i2s_controller_data->m_capturedAudioBuffer;
}

esp_err_t i2s_add_sample(float sample)
{
    // add the sample to the current audio buffers
    i2s_controller_data->m_currentAudioBuffer[i2s_controller_data->m_audioBufferPos] = sample;
    i2s_controller_data->m_audioBufferPos++;

    // have we filled the buffer with data?
    if (i2s_controller_data->m_audioBufferPos == i2s_controller_data->m_bufferSizeInSamples)
    {
        float *temp;
        temp = i2s_controller_data->m_capturedAudioBuffer;
        i2s_controller_data->m_capturedAudioBuffer = i2s_controller_data->m_currentAudioBuffer;
        i2s_controller_data->m_currentAudioBuffer = temp;

        
        // swap to the other buffer
        //std::swap(m_currentAudioBuffer, m_capturedAudioBuffer);
        //memcpy(i2s_controller_data->m_capturedAudioBuffer,i2s_controller_data->m_currentAudioBuffer,i2s_controller_data->m_bufferSizeInSamples);

        /* for (size_t i = 0; i < i2s_controller_data->m_bufferSizeInSamples; i++)
        {
            i2s_controller_data->m_capturedAudioBuffer = i2s_controller_data->m_currentAudioBuffer;
        }
         */
        // reset the buffer position
        i2s_controller_data->m_audioBufferPos = 0;
        // tell the writer task to save the data
        xTaskNotify(i2s_controller_data->m_writerTaskHandle, 1, eIncrement);
    }

    return ESP_OK;
}

esp_err_t processI2SData(uint8_t *i2sData, size_t bytesRead)
{
    int32_t *samples = (int32_t *)i2sData;
    
    #ifdef FILTER_DSP

        float samples_norm[256];
        for (int i = 0; i < bytesRead / 4; i++)
        {
            samples_norm[i] = (float) samples[i]/(2147483647.0);
        }

        dsps_biquad_f32(samples_norm, samples_norm, (bytesRead/4), coeffs_hpf, w_hpf);      

        for (int i = 0; i < bytesRead / 4; i++)
        {
            i2s_add_sample(samples_norm[i]);
        }

    #else

        for (int i = 0; i < bytesRead / 4; i++)
        {
            i2s_add_sample((float) samples[i]/(2147483647.0));
        }

    #endif

    return ESP_OK;
}

struct i2s_controller_t *i2s_controller_instance()
{
    if(i2s_controller.i2s_init == 0)
    {
        i2s_controller = (struct i2s_controller_t)
        {
            .i2s_init = &i2s_init,
            .getI2SPort = &getI2SPort,
            .processI2SData = &processI2SData,
            .i2s_add_sample = &i2s_add_sample,
            .getCapturedAudioBuffer = getCapturedAudioBuffer,
        };
    }
    return &i2s_controller;
}
