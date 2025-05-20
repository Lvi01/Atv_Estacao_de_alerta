# 🌊 Estação de Monitoramento de Cheias – BitDogLab + FreeRTOS

Este projeto implementa uma estação embarcada de monitoramento de cheias utilizando a placa BitDogLab com o microcontrolador RP2040. Por meio de sensores simulados com joystick e periféricos integrados como OLED, LED RGB, buzzer e matriz de LEDs 5x5, o sistema detecta condições críticas de inundação e exibe os dados em tempo real. O gerenciamento é feito com o sistema operacional de tempo real **FreeRTOS**, garantindo execução paralela, reativa e confiável.

---

## 🎯 Objetivo Geral

O objetivo do projeto é desenvolver um sistema embarcado capaz de simular uma estação de monitoramento ambiental voltada para detecção de cheias. Através das leituras de nível de água e volume de chuva simuladas com o joystick, o sistema processa essas informações e responde de forma visual e sonora ao atingir valores críticos. O projeto foi concebido para reforçar o aprendizado prático do uso do FreeRTOS, tarefas concorrentes, filas de comunicação e controle de periféricos da placa BitDogLab.

---

## ⚙️ Descrição Funcional

O sistema opera continuamente monitorando os dados de entrada simulados pelas portas ADC. Os eixos X e Y do joystick representam respectivamente o volume de chuva e o nível de água de um rio. Esses dados são convertidos para valores percentuais e enviados por meio de uma fila FreeRTOS para múltiplas tarefas especializadas. A partir desses dados, o sistema identifica se está em **modo normal** ou em **modo alerta**.

No modo normal, quando ambos os valores estão abaixo dos limites definidos (chuva < 80%, água < 70%), os alertas permanecem inativos. O LED RGB se mantém verde, a matriz de LEDs acende em verde e o buzzer permanece desligado. Já no modo alerta, qualquer leitura que ultrapasse os limites definidos ativa todos os sinais de emergência. O LED RGB acende em vermelho, o buzzer emite sinais sonoros intermitentes, a matriz de LEDs muda para vermelho, e o display OLED exibe mensagens de evacuação, juntamente com os valores percentuais em destaque.

Cada funcionalidade (leitura, exibição, sinalização) é gerenciada por uma tarefa independente e sincronizada por meio de uma fila de dados comum. Essa separação assegura modularidade e reatividade, permitindo que cada tarefa atue no tempo certo e sem bloqueios entre si.

---

## 🧠 FreeRTOS: Multitarefa e Comunicação

O projeto utiliza o FreeRTOS para organizar o sistema em cinco tarefas paralelas:

- `vJoystickTask`: lê os valores simulados via ADC (joystick)
- `vDisplayTask`: exibe os dados no display OLED
- `vLedRgbTask`: controla o LED RGB para sinalização visual
- `vBuzzerTask`: ativa o buzzer sonoro em caso de alerta
- `vMatrizLedTask`: altera a cor da matriz de LEDs conforme o estado

Essas tarefas se comunicam exclusivamente por meio de uma fila do tipo `QueueHandle_t`, onde os dados sensoriais são encapsulados em uma estrutura `dados_sensor_t`. Essa estrutura inclui os dois valores monitorados (chuva e nível de água) e um sinal booleano de alerta.

O uso do `xQueueSend()` e `xQueueReceive()` permite que as tarefas produtoras e consumidoras operem de forma desacoplada, evitando conflitos de concorrência. Com `vTaskDelay()`, cada tarefa aguarda seu tempo de execução adequado, reduzindo o consumo da CPU e melhorando a previsibilidade. Essa abordagem reflete o uso profissional de sistemas embarcados em tempo real.

---

## 🔌 Uso dos Periféricos da BitDogLab / RP2040

### 🎮 Joystick (ADC)

Os eixos analógicos do joystick foram conectados às portas ADC0 e ADC1 do RP2040. Eles simulam os sensores de nível de água e volume de chuva. As leituras são convertidas para porcentagem com base no valor máximo de 12 bits do ADC (4095) e são o ponto de partida da lógica do sistema. A leitura ocorre a cada 100ms na tarefa `vJoystickTask`.

### 🖥️ Display OLED (I2C)

O display OLED de 128x64 pixels foi utilizado para exibir os valores monitorados e o estado do sistema (normal ou alerta). A comunicação é feita via protocolo I2C, usando os pinos GPIO 14 (SDA) e 15 (SCL). A biblioteca `ssd1306` gerencia a renderização gráfica e textual. O conteúdo do display é atualizado com destaque visual em situações de emergência.

### 🔴🟢🔵 LED RGB (PWM)

O LED RGB da BitDogLab foi controlado via PWM pelos pinos GPIO 11, 12 e 13. O sistema acende o LED em **verde** quando em modo normal e em **vermelho** quando entra em modo alerta. A configuração do PWM é feita diretamente nas tarefas e ajustada dinamicamente a cada nova leitura da fila.

### 🔊 Buzzer (PWM)

O buzzer é acionado por PWM no pino GPIO 21. No modo alerta, ele emite sinais intermitentes de 200ms ligados e 300ms desligados, simulando um alerta sonoro real. No modo normal, permanece completamente desativado, evitando ruídos desnecessários.

### 🟩🟥 Matriz de LEDs (PIO)

A matriz de LEDs 5x5 utiliza um programa em PIO carregado no `pio0` para controle direto dos LEDs WS2812. Os LEDs mudam de cor com base no estado do sistema: **verde** no modo normal e **vermelho** no modo alerta. A atualização ocorre a cada 500ms, garantindo resposta visual em tempo real.

---

## 🧪 Simulação de Sensores

| Sensor Simulado | Pino GPIO | Descrição                      |
|-----------------|-----------|-------------------------------|
| Nível de Água    | GPIO 26   | Eixo Y do joystick (ADC0)     |
| Volume de Chuva  | GPIO 27   | Eixo X do joystick (ADC1)     |

---

## 👨‍💻 Autor

Desenvolvido por Levi Silva Freitas  
CEPEDI - Embarcatech TIC37  
