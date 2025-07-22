#ifndef SENSORES_H
#define SENSORES_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "aht20.h"
#include "bmp280.h"

// Estrutura para dados dos sensores
typedef struct {
    float temperatura_aht;
    float temperatura_bmp;
    float umidade;
    float pressao;
    float altitude;
    bool valido_aht;
    bool valido_bmp;
} DadosSensoresCompletos;

// Estrutura para configurações e limites
typedef struct {
    float temp_min, temp_max;
    float umidade_min, umidade_max;
    float pressao_min, pressao_max;
    float offset_temp_aht;
    float offset_temp_bmp;
    float offset_umidade;
    float offset_pressao;
} ConfiguracoesSensores;

// Inicialização dos sensores
bool inicializar_sensores(i2c_inst_t *i2c);

// Leitura unificada dos sensores
bool ler_todos_sensores(i2c_inst_t *i2c, DadosSensoresCompletos *dados, ConfiguracoesSensores *config);

// Verificação de alertas
bool verificar_limites(DadosSensoresCompletos *dados, ConfiguracoesSensores *config);

// Cálculo de altitude
double calcular_altitude(double pressao_pa);

// Formatação de dados para exibição
void formatar_dados_display(DadosSensoresCompletos *dados, char *linha1, char *linha2, char *linha3, char *linha4);

// Formatação de dados para web (JSON)
void formatar_dados_json(DadosSensoresCompletos *dados, char *json_buffer, size_t buffer_size);

#endif // SENSORES_H
