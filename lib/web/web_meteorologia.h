#ifndef WEB_METEOROLOGIA_H
#define WEB_METEOROLOGIA_H

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/err.h"
#include "sensores.h"

// Estrutura para manter o estado da conexão HTTP
struct http_state_meteo {
    char response[8192];  // Buffer maior para a página web
    size_t len;
    size_t sent;
};

// Variáveis globais para compartilhar dados com o servidor web
extern DadosSensoresCompletos dados_globais;
extern ConfiguracoesSensores config_global;
extern volatile bool alerta_global;

// Funções principais
int inicializar_wifi_meteorologia(char *ip_str, char *WIFI_SSID, char *WIFI_PASS);
void init_web_meteorologia(void);
void atualizar_dados_web(DadosSensoresCompletos *dados, ConfiguracoesSensores *config, bool alerta);

// Funções internas do servidor
static void start_http_server_meteo(void);
static err_t http_sent_meteo(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t http_recv_meteo(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t connection_callback_meteo(void *arg, struct tcp_pcb *newpcb, err_t err);

#endif // WEB_METEOROLOGIA_H
