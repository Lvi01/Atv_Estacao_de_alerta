// -----------------------------------------------------------------------------
// Autor: Levi Silva Freitas
// Data: 19/05/2024
// Projeto: Estacao de Monitoramento de Cheias com FreeRTOS e BitDogLab
// Descricao: Sistema com leitura simulada de nivel de agua e volume de chuva,
// com alertas visuais, sonoros e exibicao em display OLED.
// -----------------------------------------------------------------------------

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "final.pio.h"
#include "lib/font.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

// -------------------- Definicoes --------------------
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define I2C_ENDERECO 0x3C

#define ADC_NIVEL_AGUA 26   // eixo Y - GPIO 26
#define ADC_VOLUME_CHUVA 27 // eixo X - GPIO 27

#define LED_R 13
#define LED_G 11
#define LED_B 12
#define BUZZER 21
#define LED_MATRIX_PIN 7

#define LIMIAR_AGUA 70.0f
#define LIMIAR_CHUVA 80.0f

#define NUM_LEDS 25

// -------------------- Structs --------------------
// Estrutura para armazenar os dados lidos dos sensores simulados
typedef struct {
    float nivel_agua;    // em %
    float volume_chuva;  // em %
    bool alerta;         // true se algum valor passou o limiar
} dados_sensor_t;

// -------------------- Filas --------------------
// Fila global para troca de dados entre as tarefas
QueueHandle_t xQueueSensores;

// -------------------- Prototipos --------------------
void vJoystickTask(void *params);
void vDisplayTask(void *params);
void vLedRgbTask(void *params);
void vBuzzerTask(void *params);
void vMatrizLedTask(void *params);

// -------------------- Main --------------------
int main() {
    stdio_init_all();

    // Criação da fila para comunicação entre tarefas
    // Capacidade: 5 elementos do tipo dados_sensor_t
    xQueueSensores = xQueueCreate(5, sizeof(dados_sensor_t));

    // Criação das tarefas do FreeRTOS
    // Cada tarefa recebe um ponteiro para função, nome, tamanho da stack, parâmetros, prioridade e handle
    xTaskCreate(vJoystickTask, "Joystick", 256, NULL, 1, NULL);
    xTaskCreate(vDisplayTask, "Display", 512, NULL, 1, NULL);
    xTaskCreate(vLedRgbTask, "LED RGB", 256, NULL, 1, NULL);
    xTaskCreate(vBuzzerTask, "Buzzer", 256, NULL, 1, NULL);
    xTaskCreate(vMatrizLedTask, "Matriz", 256, NULL, 1, NULL); // opcional

    // Inicia o escalonador do FreeRTOS
    vTaskStartScheduler();
    while (true) {}
    return 0;
}

// -------------------- Tarefa: Leitura Joystick --------------------
// Esta tarefa simula a leitura dos sensores de nível de água e chuva usando ADC.
// Os valores lidos são convertidos em porcentagem e enviados para a fila.
// Se algum valor ultrapassar o limiar, o campo 'alerta' é ativado.
void vJoystickTask(void *params) {
    adc_init();
    adc_gpio_init(ADC_NIVEL_AGUA);
    adc_gpio_init(ADC_VOLUME_CHUVA);

    while (true) {
        adc_select_input(0); // ADC0 - Y
        uint16_t raw_y = adc_read();
        float nivel = (raw_y / 4095.0f) * 100.0f;

        adc_select_input(1); // ADC1 - X
        uint16_t raw_x = adc_read();
        float chuva = (raw_x / 4095.0f) * 100.0f;

        // Verifica se algum valor ultrapassou o limiar
        bool alerta = (nivel >= LIMIAR_AGUA || chuva >= LIMIAR_CHUVA);

        // Preenche a struct com os dados lidos
        dados_sensor_t dados = {
            .nivel_agua = nivel,
            .volume_chuva = chuva,
            .alerta = alerta
        };

        // Envia os dados para a fila (não bloqueante)
        xQueueSend(xQueueSensores, &dados, 0);
        vTaskDelay(pdMS_TO_TICKS(100)); // Aguarda 100ms
    }
}

// -------------------- Tarefa: Display OLED --------------------
// Esta tarefa recebe dados da fila e exibe no display OLED.
// Mostra o nível de água, volume de chuva e o estado de alerta.
void vDisplayTask(void *params) {
    ssd1306_t display;
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(&display, WIDTH, HEIGHT, false, I2C_ENDERECO, I2C_PORT);
    ssd1306_config(&display);

    dados_sensor_t dados;

    char buffer[5]; // Buffer para armazenar a string
    int contador = 0;
    bool cor = true;

    while (true) {
        // Aguarda novos dados na fila (bloqueante)
        if (xQueueReceive(xQueueSensores, &dados, portMAX_DELAY) == pdTRUE) {
            ssd1306_fill(&display, false);

            ssd1306_fill(&display, !cor);                          // Limpa o display
            ssd1306_rect(&display, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
            ssd1306_line(&display, 3, 25, 123, 25, cor);           // Desenha uma linha
            ssd1306_line(&display, 3, 37, 123, 37, cor);           // Desenha uma linha
            ssd1306_line(&display, 63, 41, 63, 60, cor);           // Linha vertical entre "Nivel" e "Chuva"

            // Exibe mensagem de alerta ou modo normal
            if (dados.alerta) {
                ssd1306_draw_string(&display, "Enchente Lida", 12, 6); // Desenha uma string
                ssd1306_draw_string(&display, "Evacuar agora", 12, 16);  // Desenha uma string
                ssd1306_draw_string(&display, "  EMERGENCIA", 10, 28);   // Desenha uma string
            } else {
                ssd1306_draw_string(&display, "CEPEDI   TIC37", 8, 6); // Desenha uma string
                ssd1306_draw_string(&display, "EMBARCATECH", 20, 16);  // Desenha uma string
                ssd1306_draw_string(&display, "   FreeRTOS", 10, 28);   // Desenha uma string
            }

            ssd1306_draw_string(&display, "Nivel", 10, 41);        // Desenha uma string
            ssd1306_draw_string(&display, "Chuva", 78, 41);        // Desenha uma string
            sprintf(buffer, "%.1f%%", dados.nivel_agua);            // Converte em string a leitura do ADC
            ssd1306_draw_string(&display, buffer, 10, 52);          // Desenha uma string
            sprintf(buffer, "%.1f%%", dados.volume_chuva);          // Converte em string a leitura do ADC
            ssd1306_draw_string(&display, buffer, 80, 52);          // Desenha uma string

            ssd1306_send_data(&display);                            // Atualiza o display
            sleep_ms(500);                                          // Aguarda 500ms

        }
    }
}

// -------------------- Tarefa: LED RGB --------------------
// Esta tarefa controla o LED RGB conforme o estado de alerta.
// Recebe dados da fila e acende o LED vermelho (alerta) ou verde (normal).
void vLedRgbTask(void *params) {
    gpio_set_function(LED_R, GPIO_FUNC_PWM);
    gpio_set_function(LED_G, GPIO_FUNC_PWM);
    gpio_set_function(LED_B, GPIO_FUNC_PWM);

    uint slice_r = pwm_gpio_to_slice_num(LED_R);
    uint slice_g = pwm_gpio_to_slice_num(LED_G);
    uint slice_b = pwm_gpio_to_slice_num(LED_B);

    pwm_set_wrap(slice_r, 255);
    pwm_set_wrap(slice_g, 255);
    pwm_set_wrap(slice_b, 255);

    pwm_set_enabled(slice_r, true);
    pwm_set_enabled(slice_g, true);
    pwm_set_enabled(slice_b, true);

    dados_sensor_t dados;

    while (true) {
        // Aguarda novos dados na fila
        if (xQueueReceive(xQueueSensores, &dados, portMAX_DELAY) == pdTRUE) {
            if (dados.alerta) {
                // Vermelho em alerta
                pwm_set_gpio_level(LED_R, 255);
                pwm_set_gpio_level(LED_G, 0);
                pwm_set_gpio_level(LED_B, 0);
            } else {
                // Verde no modo normal
                pwm_set_gpio_level(LED_R, 0);
                pwm_set_gpio_level(LED_G, 255);
                pwm_set_gpio_level(LED_B, 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200)); // Atualiza a cada 200ms
    }
}

// -------------------- Tarefa: Buzzer --------------------
// Esta tarefa controla o buzzer para emitir som em caso de alerta.
// Recebe dados da fila e ativa o buzzer apenas quando necessário.
void vBuzzerTask(void *params) {
    gpio_set_function(BUZZER, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(BUZZER);
    pwm_set_wrap(slice, 12500); // 10 kHz base
    pwm_set_clkdiv(slice, 125.0f);
    pwm_set_enabled(slice, true);

    dados_sensor_t dados;

    while (true) {
        // Aguarda novos dados na fila
        if (xQueueReceive(xQueueSensores, &dados, portMAX_DELAY) == pdTRUE) {
            if (dados.alerta) {
                // Ativa o buzzer (50% duty cycle)
                pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER), 6250);
                vTaskDelay(pdMS_TO_TICKS(200));
                // Desativa o buzzer
                pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER), 0);
                vTaskDelay(pdMS_TO_TICKS(300));
            } else {
                // Mantém o buzzer desligado
                pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER), 0);
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }
    }
}

// -------------------- Tarefa: Matriz de LEDs --------------------
// Esta tarefa controla uma matriz de LEDs via PIO.
// Acende todos os LEDs de vermelho em alerta ou verde no modo normal.
PIO pio = pio0;
uint sm;

void vMatrizLedTask(void *params) {
    // Inicializa PIO e carrega programa da matriz de LEDs
    uint offset = pio_add_program(pio, &final_program);
    sm = pio_claim_unused_sm(pio, true);
    final_program_init(pio, sm, offset, LED_MATRIX_PIN);

    dados_sensor_t dados;

    while (true) {
        // Aguarda novos dados na fila
        if (xQueueReceive(xQueueSensores, &dados, portMAX_DELAY) == pdTRUE) {
            // Define cor: vermelho para alerta, verde para normal
            uint32_t cor = dados.alerta ? 0x00FF0000 : 0xFF000000;
            for (int i = 0; i < NUM_LEDS; i++) {
                pio_sm_put_blocking(pio, sm, cor);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // Atualiza a cada 500ms
    }
}
