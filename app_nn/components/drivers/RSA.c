#include "mbedtls/rsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/sha512.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"  // Para heap management
#include <string.h>
#include <stdio.h>

#define DTAG "RSA_APP"

void RSA_test(void) {
    int ret;
    mbedtls_rsa_context rsa;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "rsa_encrypt_decrypt";
    unsigned char text[] = "Hello, World!";
    unsigned char *buf = NULL;  // Mover para heap
    unsigned char *decrypted = NULL;  // Mover para heap
    size_t olen = 0;
    // unsigned char hash[32];
    // unsigned char private_key_pem[1600]; // Buffer grande para armazenar a chave PEM
    int64_t start_time, end_time;

    // Alocar buf e decrypted na heap
    buf = (unsigned char *)malloc(512 * sizeof(unsigned char));
    if (buf == NULL) {
        ESP_LOGE(DTAG, "Failed to allocate memory for buf");
        goto exit;
    }

    decrypted = (unsigned char *)malloc(512 * sizeof(unsigned char));
    if (decrypted == NULL) {
        ESP_LOGE(DTAG, "Failed to allocate memory for decrypted");
        free(buf);  // Liberar memória previamente alocada
        goto exit;
    }

    mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V15, 0);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers))) != 0) {
        ESP_LOGE(DTAG, "mbedtls_ctr_drbg_seed returned -0x%x", -ret);
        goto exit;
    }

    start_time = esp_timer_get_time();
    if ((ret = mbedtls_rsa_gen_key(&rsa, mbedtls_ctr_drbg_random, &ctr_drbg, 512, 65537)) != 0) {
        ESP_LOGE(DTAG, "mbedtls_rsa_gen_key returned -0x%x", -ret);
        goto exit;
    }
    end_time = esp_timer_get_time();
    ESP_LOGI(DTAG, "RSA key generation took %lld microseconds", end_time - start_time);

    // Imprimir chave pública
    printf("\n");
    // ESP_LOGI(DTAG, "Public key (N):");
    for (size_t i = 0; i < rsa.len; i++) {
        printf("%02X", rsa.N.p[i]);
    }
    printf("\n");
    // ESP_LOGI(DTAG, "Public exponent (E):");
    for (size_t i = 0; i < rsa.len; i++) {
        printf("%02X", rsa.E.p[i]);
    }
    printf("\n");
    // Imprimir chave privada
    // ESP_LOGI(DTAG, "Private key (D):");
    for (size_t i = 0; i < rsa.D.n; i++) {
        printf("%02X", rsa.D.p[i]);
    }
    printf("\n");
    // Imprimir mensagem original
    printf("Mensagem original: %s\n", text);

    // Encryption (using public key)
    start_time = esp_timer_get_time();
    if ((ret = mbedtls_rsa_pkcs1_encrypt(&rsa, mbedtls_ctr_drbg_random, &ctr_drbg, MBEDTLS_RSA_PUBLIC, sizeof(text) - 1, text, buf)) != 0) {
        ESP_LOGE(DTAG, "mbedtls_rsa_pkcs1_encrypt returned -0x%x", -ret);
        goto exit;
    }
    end_time = esp_timer_get_time();
    ESP_LOGI(DTAG, "Encryption took %lld microseconds", end_time - start_time);

    // Imprimir mensagem criptografada
    ESP_LOGI(DTAG, "Encrypted message:");
    for (size_t i = 0; i < rsa.len; i++) {
        printf("%02X", buf[i]);
    }
    printf("\n");

    // Decryption (using private key)
    start_time = esp_timer_get_time();
    if ((ret = mbedtls_rsa_pkcs1_decrypt(&rsa, mbedtls_ctr_drbg_random, &ctr_drbg, MBEDTLS_RSA_PRIVATE, &olen, buf, decrypted, 512)) != 0) {
        // ESP_LOGE(DTAG, "mbedtls_rsa_pkcs1_decrypt returned -0x%x", -ret);
        goto exit;
    }
    decrypted[olen] = '\0';  // Ensure null termination
    end_time = esp_timer_get_time();
    // ESP_LOGI(DTAG, "Decryption took %lld microseconds", end_time - start_time);

    // Imprimir mensagem descriptografada
    printf("Mensagem desencriptada: %s\n", decrypted);

exit:
    mbedtls_rsa_free(&rsa);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    if (buf != NULL) {
        free(buf);  // Liberar memória da heap
    }
    if (decrypted != NULL) {
        free(decrypted);  // Liberar memória da heap
    }
}
