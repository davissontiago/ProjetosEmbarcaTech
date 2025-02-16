#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

// Definição do número de LEDs e pino.
#define LED_COUNT 25
#define LED_PIN 7

// Definição dos botões
#define BUTTON_A 5

// Definição de pixel GRB
typedef struct {
    uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
} pixel_t;

typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

// Protótipos de funções
void npInit(uint pin);
void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b);
void npClear();
void npWrite();
int getIndex(int x, int y);
void updateLEDs(int matriz[5][5][3]);

/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInit(uint pin) {
    // Cria programa PIO.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    // Toma posse de uma máquina PIO.
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
    }

    // Inicia programa na máquina PIO obtida.
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    // Limpa buffer de pixels.
    npClear();
}

/**
 * Atribui uma cor RGB a um LED.
 */
void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

/**
 * Limpa o buffer de pixels.
 */
void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        npSetLED(i, 0, 0, 0);
    }
}

/**
 * Escreve os dados do buffer nos LEDs.
 */
void npWrite() {
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}

/**
 * Calcula o índice do LED na matriz com base nas coordenadas (x, y).
 */
int getIndex(int x, int y) {
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0) {
        return 24 - (y * 5 + x); // Linha par (esquerda para direita).
    } else {
        return 24 - (y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}

/**
 * Atualiza os LEDs com base em uma matriz de cores.
 */
void updateLEDs(int matriz[5][5][3]) {
    for (int linha = 0; linha < 5; linha++) {
        for (int coluna = 0; coluna < 5; coluna++) {
            int posicao = getIndex(linha, coluna);
            npSetLED(posicao, matriz[coluna][linha][0], matriz[coluna][linha][1], matriz[coluna][linha][2]);
        }
    }
    npWrite(); // Envia os dados para os LEDs.
}

int main() {
    // Inicializa entradas e saídas.
    stdio_init_all();

    // Configura o botão A como entrada com pull-up.
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    // Inicializa matriz de LEDs NeoPixel.
    npInit(LED_PIN);
    npClear();

    // Matrizes de cores para os LEDs.
    int matriz_pressionado[5][5][3] = {
        {{0, 0, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255}},
        {{0, 0, 255}, {0, 255, 0}, {0, 0, 255}, {0, 255, 0}, {0, 0, 255}},
        {{0, 0, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255}},
        {{0, 0, 255}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 255}},
        {{0, 0, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255}}
    };

    int matriz_solto[5][5][3] = {
        {{0, 0, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255}},
        {{0, 0, 255}, {0, 0, 0}, {0, 0, 255}, {0, 0, 0}, {0, 0, 255}},
        {{0, 0, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255}},
        {{0, 0, 255}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {0, 0, 255}},
        {{0, 0, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255}}
    };

    bool botao_pressionado = false;

    while (true) {
        if (gpio_get(BUTTON_A) == 0) { // Botão pressionado
            if (!botao_pressionado) { // Verifica se o estado mudou
                updateLEDs(matriz_pressionado);
                botao_pressionado = true; // Atualiza o estado do botão
            }
        } else { // Botão não pressionado
            if (botao_pressionado) { // Verifica se o estado mudou
                updateLEDs(matriz_solto);
                botao_pressionado = false; // Atualiza o estado do botão
            }
        }
        sleep_ms(10); // Evita leitura excessiva do botão.
    }

    return 0;
}