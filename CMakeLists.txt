# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)
project(Grupo_Bomba C CXX ASM)
pico_sdk_init()

include_directories(${CMAKE_SOURCE_DIR}/lib)

# Executável do projeto original (controle de bomba)
add_executable(Grupo_Bomba  
        Grupo_Bomba.c
        lib/ssd1306/ssd1306.c
        lib/botoes/botoes.c
        lib/matriz/matriz.c
        lib/potenciometro/potenciometro.c
        lib/web/web.c
        lib/buzzer/buzzer.c
        lib/rgb/rgb.c
        )

# Executável do novo projeto (estação meteorológica)
add_executable(Estacao_Meteorologica  
        estacao_meteorologica.c
        lib/ssd1306/ssd1306.c
        lib/botoes/botoes.c
        lib/matriz/matriz.c
        lib/buzzer/buzzer.c
        lib/rgb/rgb.c
        lib/aht20/aht20.c
        lib/bmp280/bmp280.c
        lib/sensores/sensores.c
        lib/web/web_meteorologia.c
        )

pico_generate_pio_header(Grupo_Bomba ${CMAKE_CURRENT_LIST_DIR}/lib/matriz/matriz.pio)
pico_generate_pio_header(Estacao_Meteorologica ${CMAKE_CURRENT_LIST_DIR}/lib/matriz/matriz.pio)

# Configurações para projeto original
target_link_libraries(Grupo_Bomba 
        pico_stdlib 
        hardware_i2c
        hardware_adc
        hardware_pwm        
        pico_cyw43_arch_lwip_threadsafe_background
        )

# Configurações para estação meteorológica
target_link_libraries(Estacao_Meteorologica 
        pico_stdlib 
        hardware_i2c
        hardware_gpio
        hardware_pwm        
        pico_cyw43_arch_lwip_threadsafe_background
        )

target_include_directories(Grupo_Bomba PRIVATE ${CMAKE_CURRENT_LIST_DIR} 
        lib/ssd1306
        lib/botoes
        lib/matriz
        lib/potenciometro
        lib/web
        lib/buzzer
        lib/rgb
        )

target_include_directories(Estacao_Meteorologica PRIVATE ${CMAKE_CURRENT_LIST_DIR} 
        lib/ssd1306
        lib/botoes
        lib/matriz
        lib/buzzer
        lib/rgb
        lib/aht20
        lib/bmp280
        lib/sensores
        lib/web
        )

pico_enable_stdio_usb(Grupo_Bomba 1)
pico_enable_stdio_uart(Grupo_Bomba 0)
pico_add_extra_outputs(Grupo_Bomba)

pico_enable_stdio_usb(Estacao_Meteorologica 1)
pico_enable_stdio_uart(Estacao_Meteorologica 0)
pico_add_extra_outputs(Estacao_Meteorologica)