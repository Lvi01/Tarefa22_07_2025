#include "web_meteorologia.h"
#include "sensores.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Variáveis globais para dados dos sensores
DadosSensoresCompletos dados_globais = {0};
ConfiguracoesSensores config_global = {
    .temp_min = 15.0, .temp_max = 35.0,
    .umidade_min = 30.0, .umidade_max = 80.0,
    .pressao_min = 950.0, .pressao_max = 1050.0,
    .offset_temp_aht = 0.0, .offset_temp_bmp = 0.0,
    .offset_umidade = 0.0, .offset_pressao = 0.0
};
volatile bool alerta_global = false;

// Protótipo da página HTML (será incluída como string)
const char METEOROLOGIA_HTML[] = 
    "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Estacao Meteorologica</title>"
    "<style>body{font-family:sans-serif;background:#667eea;color:#333;padding:10px}"
    ".container{max-width:800px;margin:0 auto;background:white;border-radius:10px;padding:20px}"
    ".header{text-align:center;margin-bottom:20px;color:#667eea}"
    ".data{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px}"
    ".sensor{background:#f8f9fa;padding:15px;border-radius:8px;text-align:center}"
    ".value{font-size:24px;font-weight:bold;color:#333;margin:10px 0}"
    "</style></head><body>"
    "<div class='container'>"
    "<div class='header'><h1>Estacao Meteorologica</h1><p>EmbarcaTech - Tempo Real</p></div>"
    "<div class='data'>"
    "<div class='sensor'><h3>Temp AHT20</h3><div class='value' id='tempAht'>--.-°C</div></div>"
    "<div class='sensor'><h3>Temp BMP280</h3><div class='value' id='tempBmp'>--.-°C</div></div>"
    "<div class='sensor'><h3>Umidade</h3><div class='value' id='humidity'>--.-%</div></div>"
    "<div class='sensor'><h3>Pressao</h3><div class='value' id='pressure'>----hPa</div></div>"
    "<div class='sensor'><h3>Altitude</h3><div class='value' id='altitude'>----m</div></div>"
    "</div></div>"
    "<script>"
    "function updateData(){"
    "fetch('/data').then(r=>r.json()).then(d=>{"
    "document.getElementById('tempAht').textContent=d.temperatura_aht.toFixed(1)+'°C';"
    "document.getElementById('tempBmp').textContent=d.temperatura_bmp.toFixed(1)+'°C';"
    "document.getElementById('humidity').textContent=d.umidade.toFixed(1)+'%';"
    "document.getElementById('pressure').textContent=d.pressao.toFixed(0)+'hPa';"
    "document.getElementById('altitude').textContent=d.altitude.toFixed(0)+'m';"
    "}).catch(e=>console.error('Erro:',e));}"
    "setInterval(updateData,2000);updateData();"
    "</script></body></html>";

// Função para inicializar WiFi
int inicializar_wifi_meteorologia(char *ip_str, char *WIFI_SSID, char *WIFI_PASS) {
    if (cyw43_arch_init()) {
        printf("WiFi => Erro na inicialização do driver\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();
    printf("WiFi => Conectando à rede %s...\n", WIFI_SSID);

    int result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000);
    if (result != 0) {
        printf("WiFi => Falha na conexão (%d)\n", result);
        return result;
    }

    // Obter e exibir o endereço IP
    uint32_t ip_addr = cyw43_state.netif[CYW43_ITF_STA].ip_addr.addr;
    snprintf(ip_str, 16, "%lu.%lu.%lu.%lu", 
             ip_addr & 0xFF, 
             (ip_addr >> 8) & 0xFF, 
             (ip_addr >> 16) & 0xFF, 
             ip_addr >> 24);

    printf("WiFi => Conectado com sucesso!\n IP: %s\n", ip_str);
    return 0;
}

// Função para atualizar dados que serão servidos via web
void atualizar_dados_web(DadosSensoresCompletos *dados, ConfiguracoesSensores *config, bool alerta) {
    dados_globais = *dados;
    config_global = *config;
    alerta_global = alerta;
}

// Função para processar requisições HTTP
static err_t http_recv_meteo(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    struct http_state_meteo *hs = (struct http_state_meteo *)arg;
    
    if (p == NULL) {
        tcp_close(tpcb);
        if (hs) free(hs);
        return ERR_OK;
    }

    if (err != ERR_OK) {
        pbuf_free(p);
        return err;
    }

    // Extrair dados da requisição
    char *request = (char *)p->payload;
    char method[8] = {0};
    char path[128] = {0};
    
    sscanf(request, "%7s %127s", method, path);
    
    printf("HTTP Request: %s %s\n", method, path);

    // Processar diferentes rotas
    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            // Servir página principal
            snprintf(hs->response, sizeof(hs->response),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Connection: close\r\n"
                "\r\n"
                "%s", METEOROLOGIA_HTML);
                
        } else if (strcmp(path, "/data") == 0) {
            // Servir dados JSON dos sensores
            char json_data[512];
            formatar_dados_json(&dados_globais, json_data, sizeof(json_data));
            
            snprintf(hs->response, sizeof(hs->response),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Connection: close\r\n"
                "\r\n"
                "%s", json_data);
                
        } else if (strcmp(path, "/status") == 0) {
            // Status do sistema
            snprintf(hs->response, sizeof(hs->response),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Connection: close\r\n"
                "\r\n"
                "{"
                "\"wifi\":\"conectado\","
                "\"uptime\":%lu,"
                "\"alerta\":%s,"
                "\"sensores_ativos\":%d"
                "}",
                to_ms_since_boot(get_absolute_time()),
                alerta_global ? "true" : "false",
                (dados_globais.valido_aht ? 1 : 0) + (dados_globais.valido_bmp ? 1 : 0));
                
        } else {
            // Página não encontrada
            snprintf(hs->response, sizeof(hs->response),
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html\r\n"
                "Connection: close\r\n"
                "\r\n"
                "<html><body><h1>404 - Página não encontrada</h1></body></html>");
        }
        
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/config") == 0) {
        // Processar configuração (implementação básica)
        // Aqui você processaria o JSON POST para atualizar as configurações
        
        snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n"
            "\r\n"
            "{\"status\":\"configuracao_salva\",\"success\":true}");
            
    } else {
        // Método não suportado
        snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html><body><h1>405 - Método não permitido</h1></body></html>");
    }

    hs->len = strlen(hs->response);
    hs->sent = 0;

    pbuf_free(p);

    // Enviar resposta
    tcp_write(tpcb, hs->response, hs->len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    return ERR_OK;
}

// Callback para dados enviados
static err_t http_sent_meteo(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    struct http_state_meteo *hs = (struct http_state_meteo *)arg;
    
    hs->sent += len;
    
    if (hs->sent >= hs->len) {
        tcp_close(tpcb);
        if (hs) free(hs);
    }
    
    return ERR_OK;
}

// Callback para novas conexões
static err_t connection_callback_meteo(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK || newpcb == NULL) {
        printf("Erro na conexão\n");
        return ERR_VAL;
    }

    printf("Nova conexão HTTP estabelecida\n");

    struct http_state_meteo *hs = malloc(sizeof(struct http_state_meteo));
    if (!hs) {
        printf("Erro: sem memória para estado HTTP\n");
        tcp_close(newpcb);
        return ERR_MEM;
    }

    memset(hs, 0, sizeof(struct http_state_meteo));
    tcp_arg(newpcb, hs);
    tcp_recv(newpcb, http_recv_meteo);
    tcp_sent(newpcb, http_sent_meteo);

    return ERR_OK;
}

// Iniciar servidor HTTP
static void start_http_server_meteo(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro: não foi possível criar PCB\n");
        return;
    }

    err_t err = tcp_bind(pcb, IP_ADDR_ANY, 80);
    if (err != ERR_OK) {
        printf("Erro no bind: %d\n", err);
        tcp_close(pcb);
        return;
    }

    pcb = tcp_listen(pcb);
    if (!pcb) {
        printf("Erro: não foi possível colocar em modo listen\n");
        return;
    }

    tcp_accept(pcb, connection_callback_meteo);
    printf("Servidor HTTP iniciado na porta 80\n");
}

// Função principal para inicializar o servidor web
void init_web_meteorologia(void) {
    printf("Iniciando servidor web da estação meteorológica...\n");
    start_http_server_meteo();
}
