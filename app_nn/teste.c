#include "stdio.h"
#include "stdint.h"
#include <stdlib.h>
#include "time.h"
#include <sys/stat.h>

#define ROOT_PATH_SIZE 21 + 1               // c_dd-mm-yyyy_hh_mm_ss
#define AUDIO_PATH_SIZE ROOT_PATH_SIZE + 6  // c_dd-mm-yyyy_hh_mm_ss/audio
#define ACC_PATH_SIZE   ROOT_PATH_SIZE + 14 // c_dd-mm-yyyy_hh_mm_ss/accelerometer
#define IR_PATH_SIZE    ROOT_PATH_SIZE + 9  // c_dd-mm-yyyy_hh_mm_ss/infrared

#define AUDIO_FILE_SIZE AUDIO_PATH_SIZE + 27 // c_dd-mm-yyyy_hh_mm_ss/audio/0001_Sample_1704081600.wav
#define ACC_FILE_SIZE   ACC_PATH_SIZE   + 8 // c_dd-mm-yyyy_hh_mm_ss/accelerometer/acc.csv
#define IR_FILE_SIZE    IR_PATH_SIZE    + 7 // c_dd-mm-yyyy_hh_mm_ss/infrared/ir.csv
#define ESP_OK 1
#define ESP_FAIL 0

mode_t permissions = S_IRUSR | S_IWUSR | S_IXUSR;

int path_root_generator(char* root_path, uint32_t timestamp){
    
    struct tm *data_hora = localtime((time_t*)&timestamp);
    sprintf(root_path, "c_%02d-%02d-%04d_%02d_%02d_%02d",
            data_hora->tm_mday,
            data_hora->tm_mon + 1,    // O mês começa em 0, então somamos 1
            data_hora->tm_year + 1900, // O ano começa em 1900, então somamos 1900
            data_hora->tm_hour,
            data_hora->tm_min,
            data_hora->tm_sec);
    return ESP_OK;
}

int create_directory(char* path, mode_t permissions){
    struct stat info;

    if(stat(path,&info) != 0 ){
        if (mkdir(path, permissions) == 0){
            printf("Directory creation: %s\n", path);
        } 
        else {
            printf("Directory creation fail\n");
            return ESP_FAIL;
        }
    }
    else {
        printf("Directory Already exists: %s\n",path);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

int create_directory_struct(char * path_root){
    char *path_audio_folder = (char*)malloc(AUDIO_PATH_SIZE * sizeof(char)),     
         *path_acc_folder   = (char*)malloc(ACC_PATH_SIZE   * sizeof(char)),
         *path_ir_folder    = (char*)malloc(IR_PATH_SIZE    * sizeof(char));

    snprintf(path_audio_folder,AUDIO_PATH_SIZE,"%s/audio",path_root);
    snprintf(path_acc_folder,ACC_PATH_SIZE,"%s/accelerometer",path_root);
    snprintf(path_ir_folder,IR_PATH_SIZE,"%s/infrared",path_root);

    create_directory(path_audio_folder,permissions);
    create_directory(path_acc_folder,permissions);
    create_directory(path_ir_folder,permissions);

    free(path_audio_folder);
    free(path_acc_folder);
    free(path_ir_folder);
    return ESP_OK;
}

void main(void){
    FILE *F;
    uint32_t timestamp = 1704081600;
    char path_root_folder[ROOT_PATH_SIZE],
         path_audio_file[AUDIO_FILE_SIZE],
         path_acc_file[ACC_FILE_SIZE],
         path_ir_file[IR_FILE_SIZE];
    
    path_root_generator(path_root_folder, timestamp);
    rmdir(path_root_folder);
    if(create_directory(path_root_folder,permissions) == ESP_OK){
        create_directory_struct(path_root_folder);
    }
    else printf("Root creation fail\n");

    //audio
    for (int i = 1; i <= 300; i++){
        sprintf(path_audio_file, "%s/audio/0%03d_Sample_%d.wav", path_root_folder,i, timestamp);
        F = fopen(path_audio_file, "w");
        if (F == NULL) printf("Arquivo 01 NULL");
        fclose(F);
    }

    //IR
    sprintf(path_ir_file, "%s/infrared/ir.csv", path_root_folder);
    F = fopen(path_ir_file, "w");
    fclose(F);

    char ir_info[16];
    for(int save = 0; save < 4; save++)
    {   
        sprintf(path_ir_file, "%s/infrared/ir.csv", path_root_folder);
        F = fopen(path_ir_file, "a");

        int num = 0;
        for(int i = 0; i < 1000; i+= 20){
            sprintf(ir_info, "%d%03d,%01d\n", timestamp,i,num%2);
            fwrite(ir_info,1,sizeof(ir_info), F);
            num++;
        }
        
        fclose(F);
        timestamp++;
    }
    
    float accelX, accelY, accelZ;
    uint16_t tt_ms_acc;
    char acc_info[60];

    accelX = 0.03515625;
    accelY = -0.017578125;
    accelZ = 1.01953125;
    tt_ms_acc = 177;

    

    accelX = 0.052734375;
    tt_ms_acc = 279;
    



    //{fn}_Sample_{timestamp}.wav
}