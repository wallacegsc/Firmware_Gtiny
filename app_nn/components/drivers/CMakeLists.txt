idf_component_register(SRCS "RSA.c" "sensorUltrassonico.c" "serialUart.c" "mcp.c" "infrareds.c" "acc_lis3dh.c" "acc.c" "eeprom.c" "mic.c" 
                    INCLUDE_DIRS "." 
                    PRIV_REQUIRES fatfs
                    REQUIRES controllers sensors model storage mbedtls app_update)

           