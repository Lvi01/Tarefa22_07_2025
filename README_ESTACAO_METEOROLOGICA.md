# 🌤️ Estação Meteorológica - EmbarcaTech

## 📋 **Descrição do Projeto**

Este projeto implementa uma **estação meteorológica completa** utilizando o Raspberry Pi Pico W (plataforma BitDogLab) com sensores AHT20 e BMP280. O sistema oferece monitoramento em tempo real de **temperatura**, **umidade** e **pressão atmosférica**, com interface web responsiva e sistema de alertas configuráveis.

---

## ✨ **Funcionalidades Implementadas**

### 🔧 **Requisitos Funcionais**
- ✅ **Leitura de Sensores**: AHT20 (temperatura/umidade) e BMP280 (pressão/temperatura)
- ✅ **Captura Contínua**: Medições a cada 2 segundos
- ✅ **Sistema de Alertas**: Buzzer e LED RGB para parâmetros fora dos limites
- ✅ **Cálculo de Altitude**: Baseado na pressão atmosférica

### 🖥️ **Interface Hardware (BitDogLab)**
- ✅ **Display OLED SSD1306**: 3 páginas navegáveis com dados dos sensores
- ✅ **Matriz de LEDs**: Representação visual dos níveis dos sensores
- ✅ **LED RGB**: Indicação de status (verde=normal, vermelho=alerta)
- ✅ **Buzzer**: Alertas sonoros para parâmetros críticos
- ✅ **Botões**: Navegação entre páginas e controle de alertas
- ✅ **Interrupções**: Debounce implementado para todos os botões

### 🌐 **Interface Web**
- ✅ **Servidor HTTP**: Páginas servidas via Wi-Fi
- ✅ **Dados em Tempo Real**: Atualização automática a cada 2 segundos via AJAX
- ✅ **Gráficos Dinâmicos**: Histórico das últimas 20 leituras de temperatura
- ✅ **Configuração de Limites**: Formulário para definir min/max de cada sensor
- ✅ **Calibração**: Ajuste de offsets para cada sensor
- ✅ **Design Responsivo**: Adaptável para celular e desktop
- ✅ **JSON API**: Endpoints para dados e configuração

---

## 🏗️ **Arquitetura do Sistema**

```
Estacao_Meteorologica/
│
├── estacao_meteorologica.c    # Arquivo principal com lógica do sistema
├── bmp_aht_disp.c            # Arquivo de referência original
├── CMakeLists.txt            # Configuração para ambos os projetos
│
└── lib/                      # Bibliotecas organizadas por funcionalidade
    ├── sensores/             # 🆕 Biblioteca unificada de sensores
    │   ├── sensores.h        # Interface para todos os sensores
    │   └── sensores.c        # Implementação unificada
    │
    ├── aht20/               # Sensor de temperatura e umidade
    │   ├── aht20.h
    │   └── aht20.c
    │
    ├── bmp280/              # Sensor de pressão e temperatura  
    │   ├── bmp280.h
    │   └── bmp280.c
    │
    ├── ssd1306/             # Display OLED
    │   ├── ssd1306.h
    │   ├── ssd1306.c
    │   └── font.h
    │
    ├── web/                 # Servidor web meteorológico
    │   ├── web_meteorologia.h
    │   ├── web_meteorologia.c
    │   └── meteorologia.html  # Interface web responsiva
    │
    ├── botoes/              # Controle de botões e interrupções
    ├── buzzer/              # Sistema de alertas sonoros
    ├── rgb/                 # LED RGB para status visual
    └── matriz/              # Matriz de LEDs para visualização
```

---

## 🔌 **Configuração de Hardware**

### **Sensores I2C**
- **Barramento I2C0** (GPIO 0/1): AHT20 e BMP280
- **Barramento I2C1** (GPIO 14/15): Display SSD1306

### **Controles**
- **GPIO 5**: Botão para trocar páginas no display
- **GPIO 6**: Botão BOOTSEL (modo firmware)
- **GPIO 22**: Botão joystick para toggle de alertas

### **Saídas**
- **Buzzer**: Alertas sonoros
- **LED RGB**: Status visual do sistema
- **Matriz LEDs**: Visualização gráfica dos sensores

---

## 📊 **Páginas do Display OLED**

### **Página 0: Dados Principais**
```
ESTACAO METEOR.
T1:23.5C T2:23.8C
Umidade: 65.2%
Pressao: 1013hPa
```

### **Página 1: Altitude e Status**
```
ALTITUDE & STATUS
Alt: 150m
Status: Normal
WiFi: Conectado
```

### **Página 2: Limites Configurados**
```
LIMITES CONFIG
T:15-35C
U:30-80%
P:950-1050hPa
```

---

## 🌐 **Interface Web - Endpoints**

### **Rotas Disponíveis**
- `GET /` - Página principal da estação meteorológica
- `GET /data` - Dados JSON em tempo real dos sensores
- `GET /status` - Status do sistema (WiFi, uptime, alertas)
- `POST /config` - Configuração de limites e calibração

### **Exemplo de Resposta JSON (/data)**
```json
{
  "temperatura_aht": 23.5,
  "temperatura_bmp": 23.8,
  "umidade": 65.2,
  "pressao": 1013.2,
  "altitude": 150.5,
  "valido_aht": true,
  "valido_bmp": true,
  "timestamp": 12345678
}
```

---

## 🚨 **Sistema de Alertas**

### **Condições de Alerta**
- **Temperatura**: < 15°C ou > 35°C
- **Umidade**: < 30% ou > 80%
- **Pressão**: < 950 hPa ou > 1050 hPa

### **Ações de Alerta**
1. **LED RGB** fica vermelho
2. **Buzzer** emite som de alerta
3. **Display** mostra "ALERTA!"
4. **Interface web** exibe banner de aviso
5. **Matriz LEDs** pisca indicando o sensor com problema

---

## 🔧 **Como Compilar e Executar**

### **1. Preparar Ambiente**
```bash
# Verificar se o Pico SDK está configurado
echo $PICO_SDK_PATH
```

### **2. Compilar o Projeto**
```bash
cd build
cmake ..
make Estacao_Meteorologica
```

### **3. Configurar Wi-Fi**
Edite as credenciais no arquivo `estacao_meteorologica.c`:
```c
char WIFI_SSID[] = "SeuWiFi";  
char WIFI_PASS[] = "SuaSenha"; 
```

### **4. Flash no Pico W**
1. Segure BOOTSEL e conecte o Pico
2. Copie `Estacao_Meteorologica.uf2` para o drive RPI-RP2
3. O sistema reiniciará automaticamente

---

## 📱 **Interface Web Responsiva**

### **Recursos da Interface**
- **Dashboard em Tempo Real**: Valores atuais de todos os sensores
- **Gráfico de Temperatura**: Histórico das últimas 20 leituras
- **Status do Sistema**: Wi-Fi, uptime, estado dos sensores
- **Configuração Avançada**: Limites personalizáveis e calibração
- **Design Moderno**: Gradientes, cards, animações CSS
- **Totalmente Responsivo**: Funciona em celular e desktop

### **Acesso**
1. Conecte-se à mesma rede Wi-Fi do Pico
2. Abra o navegador em `http://IP_DO_PICO`
3. A página carrega automaticamente os dados

---

## 🔬 **Características Técnicas**

### **Precisão dos Sensores**
- **AHT20**: ±0.3°C (temperatura), ±2% (umidade)
- **BMP280**: ±1°C (temperatura), ±1 hPa (pressão)
- **Altitude**: Calculada pela fórmula barométrica padrão

### **Performance**
- **Frequência de Leitura**: 2 segundos (configurável)
- **Atualização Display**: 500ms
- **Atualização Web**: 2 segundos via AJAX
- **Consumo**: Otimizado com sleep entre leituras

### **Conectividade**
- **Wi-Fi**: 802.11 b/g/n (2.4GHz)
- **Servidor HTTP**: Porta 80
- **Protocolo**: HTTP/1.1 com suporte a JSON

---

## 🎯 **Próximas Melhorias**

- [ ] **Logging de Dados**: Armazenamento histórico em memória flash
- [ ] **Previsão do Tempo**: Algoritmo baseado em variação de pressão
- [ ] **Alertas por Email**: Notificações automáticas
- [ ] **Interface Mobile**: App nativo para Android/iOS
- [ ] **Sensores Adicionais**: CO2, luz, radiação UV
- [ ] **Painel Solar**: Alimentação independente

---

## 👥 **Equipe de Desenvolvimento**

**Projeto EmbarcaTech 2025**
- Sistema embarcado para IoT
- Integração hardware/software
- Interface web moderna e responsiva

---

## 📄 **Licença**

Este projeto é parte do programa EmbarcaTech e está disponível para fins educacionais e de pesquisa.

---

**🌟 Estação Meteorológica EmbarcaTech - Monitoramento Inteligente do Clima! 🌟**
