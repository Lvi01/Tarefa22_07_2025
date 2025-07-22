#include "sensores.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#define SEA_LEVEL_PRESSURE 101325.0 // Pressão ao nível do mar em Pa

// Variáveis estáticas para calibração do BMP280
static struct bmp280_calib_param params_bmp;
static bool bmp_calibrado = false;

bool inicializar_sensores(i2c_inst_t *i2c) {
    bool sucesso = true;
    
    printf("Inicializando sensores...\n");
    
    // Inicializar AHT20
    aht20_reset(i2c);
    sleep_ms(100);
    
    if (aht20_init(i2c)) {
        printf("AHT20: Inicializado com sucesso\n");
    } else {
        printf("AHT20: Erro na inicialização\n");
        sucesso = false;
    }
    
    // Inicializar BMP280
    bmp280_init(i2c);
    bmp280_get_calib_params(i2c, &params_bmp);
    bmp_calibrado = true;
    printf("BMP280: Inicializado com sucesso\n");
    
    return sucesso;
}

bool ler_todos_sensores(i2c_inst_t *i2c, DadosSensoresCompletos *dados, ConfiguracoesSensores *config) {
    bool sucesso_geral = true;
    
    // Leitura do AHT20
    AHT20_Data data_aht;
    dados->valido_aht = aht20_read(i2c, &data_aht);
    
    if (dados->valido_aht) {
        dados->temperatura_aht = data_aht.temperature + config->offset_temp_aht;
        dados->umidade = data_aht.humidity + config->offset_umidade;
        
        // Limitar umidade entre 0 e 100%
        if (dados->umidade < 0) dados->umidade = 0;
        if (dados->umidade > 100) dados->umidade = 100;
    } else {
        dados->temperatura_aht = 0.0;
        dados->umidade = 0.0;
        sucesso_geral = false;
    }
    
    // Leitura do BMP280
    if (bmp_calibrado) {
        int32_t raw_temp, raw_pressure;
        bmp280_read_raw(i2c, &raw_temp, &raw_pressure);
        dados->valido_bmp = true; // Assumir sucesso por enquanto
        
        if (dados->valido_bmp) {
            int32_t temperature = bmp280_convert_temp(raw_temp, &params_bmp);
            int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temp, &params_bmp);
            
            dados->temperatura_bmp = (temperature / 100.0) + config->offset_temp_bmp;
            dados->pressao = (pressure / 100.0) + config->offset_pressao; // Em hPa
            dados->altitude = calcular_altitude(pressure);
        } else {
            dados->temperatura_bmp = 0.0;
            dados->pressao = 0.0;
            dados->altitude = 0.0;
            sucesso_geral = false;
        }
    } else {
        dados->valido_bmp = false;
        dados->temperatura_bmp = 0.0;
        dados->pressao = 0.0;
        dados->altitude = 0.0;
        sucesso_geral = false;
    }
    
    return sucesso_geral;
}

bool verificar_limites(DadosSensoresCompletos *dados, ConfiguracoesSensores *config) {
    bool alerta = false;
    
    // Verificar temperatura (usando AHT20 como referência)
    if (dados->valido_aht) {
        if (dados->temperatura_aht < config->temp_min || dados->temperatura_aht > config->temp_max) {
            printf("ALERTA: Temperatura fora dos limites: %.1f°C (%.1f - %.1f)\n", 
                   dados->temperatura_aht, config->temp_min, config->temp_max);
            alerta = true;
        }
    }
    
    // Verificar umidade
    if (dados->valido_aht) {
        if (dados->umidade < config->umidade_min || dados->umidade > config->umidade_max) {
            printf("ALERTA: Umidade fora dos limites: %.1f%% (%.1f - %.1f)\n", 
                   dados->umidade, config->umidade_min, config->umidade_max);
            alerta = true;
        }
    }
    
    // Verificar pressão
    if (dados->valido_bmp) {
        if (dados->pressao < config->pressao_min || dados->pressao > config->pressao_max) {
            printf("ALERTA: Pressão fora dos limites: %.1f hPa (%.1f - %.1f)\n", 
                   dados->pressao, config->pressao_min, config->pressao_max);
            alerta = true;
        }
    }
    
    return alerta;
}

double calcular_altitude(double pressao_pa) {
    return 44330.0 * (1.0 - pow(pressao_pa / SEA_LEVEL_PRESSURE, 0.1903));
}

void formatar_dados_display(DadosSensoresCompletos *dados, char *linha1, char *linha2, char *linha3, char *linha4) {
    snprintf(linha1, 20, "ESTACAO METEOR.");
    
    if (dados->valido_aht && dados->valido_bmp) {
        snprintf(linha2, 20, "T1:%.1fC T2:%.1fC", dados->temperatura_aht, dados->temperatura_bmp);
    } else if (dados->valido_aht) {
        snprintf(linha2, 20, "Temp: %.1fC", dados->temperatura_aht);
    } else if (dados->valido_bmp) {
        snprintf(linha2, 20, "Temp: %.1fC", dados->temperatura_bmp);
    } else {
        snprintf(linha2, 20, "Temp: --.-C");
    }
    
    if (dados->valido_aht) {
        snprintf(linha3, 20, "Umidade: %.1f%%", dados->umidade);
    } else {
        snprintf(linha3, 20, "Umidade: --.-%");
    }
    
    if (dados->valido_bmp) {
        snprintf(linha4, 20, "Press:%.0fhPa", dados->pressao);
    } else {
        snprintf(linha4, 20, "Press: ----hPa");
    }
}

void formatar_dados_json(DadosSensoresCompletos *dados, char *json_buffer, size_t buffer_size) {
    snprintf(json_buffer, buffer_size,
        "{"
        "\"temperatura_aht\":%.1f,"
        "\"temperatura_bmp\":%.1f,"
        "\"umidade\":%.1f,"
        "\"pressao\":%.1f,"
        "\"altitude\":%.1f,"
        "\"valido_aht\":%s,"
        "\"valido_bmp\":%s,"
        "\"timestamp\":%lu"
        "}",
        dados->temperatura_aht,
        dados->temperatura_bmp,
        dados->umidade,
        dados->pressao,
        dados->altitude,
        dados->valido_aht ? "true" : "false",
        dados->valido_bmp ? "true" : "false",
        to_ms_since_boot(get_absolute_time())
    );
}
