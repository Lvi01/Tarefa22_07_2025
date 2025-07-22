#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/bootrom.h"
#include <math.h>
#include <string.h>

    // Incluir bibliotecas dos sensores e periféricos
#include "aht20.h"
#include "bmp280.h"
#include "ssd1306.h"
#include "font.h"
#include "botoes.h"
#include "buzzer.h"
#include "rgb.h"
#include "matriz.h"
#include "web_meteorologia.h"
#include "sensores.h"

// Configurações I2C para sensores
#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1

// Configurações I2C para display
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define ENDERECO_DISPLAY 0x3C

// Configuração do botão BOOTSEL
#define BOTAO_BOOTSEL 6

// Constantes físicas
#define SEA_LEVEL_PRESSURE 101325.0

// Estrutura para armazenar dados dos sensores (usando a biblioteca)
DadosSensoresCompletos dados_sensores;
ConfiguracoesSensores config_sensores = {
    .temp_min = 15.0, .temp_max = 35.0,
    .umidade_min = 30.0, .umidade_max = 80.0,
    .pressao_min = 950.0, .pressao_max = 1050.0,
    .offset_temp_aht = 0.0, .offset_temp_bmp = 0.0,
    .offset_umidade = 0.0, .offset_pressao = 0.0
};

// Variáveis locais de controle
volatile bool alerta_ativo = false;
volatile int pagina_atual = 0;

// Protótipos de funções
void inicializar_sistema(void);
void ler_sensores(void);
void verificar_alertas(void);
void atualizar_display(void);
void atualizar_matriz_meteorologica(void);
void verificar_botoes(void);
double calculate_altitude(double pressure);

// Função para calcular altitude
double calculate_altitude(double pressure) {
    return 44330.0 * (1.0 - pow(pressure / SEA_LEVEL_PRESSURE, 0.1903));
}

// Função para verificar botões (polling simples)
void verificar_botoes(void) {
    static uint32_t ultimo_botao = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    // Debounce simples
    if (now - ultimo_botao < 200) return;
    
    // Verificar botão BOOTSEL
    if (!gpio_get(BOTAO_BOOTSEL)) {
        reset_usb_boot(0, 0);
    }
    
    // Verificar botão 5 (trocar página)
    if (!gpio_get(BOTAO_5)) {
        pagina_atual = (pagina_atual + 1) % 3;
        ultimo_botao = now;
    }
    
    // Verificar botão joystick (toggle alerta)
    if (!gpio_get(BOTAO_JOYSTICK)) {
        alerta_ativo = !alerta_ativo;
        ultimo_botao = now;
    }
}

void inicializar_sistema(void) {
    stdio_init_all();
    
    // Configurar botão BOOTSEL
    gpio_init(BOTAO_BOOTSEL);
    gpio_set_dir(BOTAO_BOOTSEL, GPIO_IN);
    gpio_pull_up(BOTAO_BOOTSEL);
    
    // Inicializar botões
    init_botoes();
    
    // Inicializar periféricos
    configurar_buzzer();
    configurar_leds();
    init_matriz();
    
    // Inicializar I2C para display
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);
    
    // Inicializar display
    static ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO_DISPLAY, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
    
    // Inicializar I2C para sensores
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    // Inicializar sensores usando a biblioteca unificada
    if (!inicializar_sensores(I2C_PORT)) {
        printf("Erro ao inicializar sensores!\n");
    }
    
    // Inicializar Wi-Fi e servidor web
    char ip_str[16];
    char WIFI_SSID[] = "SeuWiFi";  // Configurar conforme necessário
    char WIFI_PASS[] = "SuaSenha"; // Configurar conforme necessário
    
    if (inicializar_wifi_meteorologia(ip_str, WIFI_SSID, WIFI_PASS) == 0) {
        init_web_meteorologia();
        printf("Sistema inicializado com sucesso!\n");
        printf("IP: %s\n", ip_str);
    }
}

void ler_sensores(void) {
    if (ler_todos_sensores(I2C_PORT, &dados_sensores, &config_sensores)) {
        // Os dados já incluem offsets aplicados pela biblioteca
        // Verificar limites e alertas
        verificar_limites(&dados_sensores, &config_sensores);
    }
}

void verificar_alertas(void) {
    bool alerta_temp = (dados_sensores.temperatura_aht < config_sensores.temp_min || 
                       dados_sensores.temperatura_aht > config_sensores.temp_max);
    bool alerta_umidade = (dados_sensores.umidade < config_sensores.umidade_min || 
                          dados_sensores.umidade > config_sensores.umidade_max);
    bool alerta_pressao = (dados_sensores.pressao < config_sensores.pressao_min || 
                          dados_sensores.pressao > config_sensores.pressao_max);
    
    if (alerta_temp || alerta_umidade || alerta_pressao) {
        alerta_ativo = true;
        set_rgb(true, false, false); // LED vermelho
        tocar_buzzer_alerta();
    } else {
        if (alerta_ativo) {
            alerta_ativo = false;
            set_rgb(false, true, false); // LED verde
            parar_buzzer();
        }
    }
}

void atualizar_display(void) {
    static ssd1306_t ssd;
    char linha1[20], linha2[20], linha3[20], linha4[20];
    
    ssd1306_fill(&ssd, false);
    
    switch(pagina_atual) {
        case 0: // Página principal - dados dos sensores
            snprintf(linha1, sizeof(linha1), "ESTACAO METEOR.");
            snprintf(linha2, sizeof(linha2), "T1:%.1fC T2:%.1fC", 
                    dados_sensores.temperatura_aht, dados_sensores.temperatura_bmp);
            snprintf(linha3, sizeof(linha3), "Umidade: %.1f%%", dados_sensores.umidade);
            snprintf(linha4, sizeof(linha4), "Pressao:%.0fhPa", dados_sensores.pressao);
            break;
            
        case 1: // Página de altitude e status
            snprintf(linha1, sizeof(linha1), "ALTITUDE & STATUS");
            snprintf(linha2, sizeof(linha2), "Alt: %.0fm", dados_sensores.altitude);
            snprintf(linha3, sizeof(linha3), "Status: %s", alerta_ativo ? "ALERTA!" : "Normal");
            snprintf(linha4, sizeof(linha4), "WiFi: Conectado");
            break;
            
        case 2: // Página de limites
            snprintf(linha1, sizeof(linha1), "LIMITES CONFIG");
            snprintf(linha2, sizeof(linha2), "T:%.0f-%.0fC", config_sensores.temp_min, config_sensores.temp_max);
            snprintf(linha3, sizeof(linha3), "U:%.0f-%.0f%%", config_sensores.umidade_min, config_sensores.umidade_max);
            snprintf(linha4, sizeof(linha4), "P:%.0f-%.0fhPa", config_sensores.pressao_min, config_sensores.pressao_max);
            break;
    }
    
    ssd1306_draw_string(&ssd, linha1, 2, 2);
    ssd1306_draw_string(&ssd, linha2, 2, 16);
    ssd1306_draw_string(&ssd, linha3, 2, 30);
    ssd1306_draw_string(&ssd, linha4, 2, 44);
    
    // Indicador de página
    for(int i = 0; i < 3; i++) {
        ssd1306_pixel(&ssd, 118 + i*2, 58, i == pagina_atual);
    }
    
    ssd1306_send_data(&ssd);
}

void atualizar_matriz_meteorologica(void) {
    // Usar a matriz para mostrar níveis dos sensores
    // Temperatura: coluna 0-1, Umidade: coluna 2-3, Pressão: coluna 4-5
    
    uint16_t temp_percent = (uint16_t)((dados_sensores.temperatura_aht - config_sensores.temp_min) * 100 / 
                                      (config_sensores.temp_max - config_sensores.temp_min));
    uint16_t umid_percent = (uint16_t)(dados_sensores.umidade * 100 / 100);
    uint16_t press_percent = (uint16_t)((dados_sensores.pressao - config_sensores.pressao_min) * 100 / 
                                       (config_sensores.pressao_max - config_sensores.pressao_min));
    
    // Limitar valores entre 0 e 100
    temp_percent = temp_percent > 100 ? 100 : temp_percent;
    umid_percent = umid_percent > 100 ? 100 : umid_percent;
    press_percent = press_percent > 100 ? 100 : press_percent;
    
    matriz_atualizar_tanque(temp_percent, 100);
    sleep_ms(100);
    matriz_atualizar_tanque(umid_percent, 100);
    sleep_ms(100);
    matriz_atualizar_tanque(press_percent, 100);
}

int main(void) {
    // Inicializar sistema
    inicializar_sistema();
    
    printf("=== ESTAÇÃO METEOROLÓGICA INICIADA ===\n");
    
    uint32_t ultimo_update = 0;
    uint32_t ultimo_display = 0;
    
    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        
        // Verificar botões a cada loop
        verificar_botoes();
        
        // Ler sensores a cada 2 segundos
        if (now - ultimo_update >= 2000) {
            ler_sensores();
            verificar_alertas();
            atualizar_matriz_meteorologica();
            
            // Log dos dados
            printf("Temp AHT: %.1f°C | Temp BMP: %.1f°C | Umidade: %.1f%% | Pressão: %.1f hPa | Altitude: %.1fm\n",
                   dados_sensores.temperatura_aht, dados_sensores.temperatura_bmp, 
                   dados_sensores.umidade, dados_sensores.pressao, dados_sensores.altitude);
            
            ultimo_update = now;
        }
        
        // Atualizar display a cada 500ms
        if (now - ultimo_display >= 500) {
            atualizar_display();
            ultimo_display = now;
        }
        
        sleep_ms(50); // Delay para não sobrecarregar o sistema
    }
    
    return 0;
}
