#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    int MNumber, opcoes[5], escolha, i, acertou = 0, rodada = 0, min = 1, max = 100, MAttack = 3, vidas = 2;

    srand(time(NULL));
    MNumber = (rand() % 100) + 1;

    while (!acertou) {
        printf("Mnumber: %d", MNumber); // !!!!!!!!!!!!!!!! TIRAR O PRINT DPS !!!!!!!!!!!!!!!!
        rodada++;

        printf("\n--- Rodada %d ---\n", rodada);

        if (MAttack == 0) {
            printf("\nO MONSTRO ATACOU!\n");
            MAttack = 3;
            vidas--;
        }

        if (vidas <= 0) {
                printf("\nVoce morreu!\n");
                return 0; 
            }

        if (MAttack > 0) MAttack--;

        printf("--- Vidas: %d ---\n", vidas);
        printf("Escolha um numero:\n");

        for (i = 0; i < 5; i++) {
            opcoes[i] = (rand() % (max - min + 1)) + min;
        }
        

        for (i = 0; i < 5; i++) {
            printf("%d - %d\n", i + 1, opcoes[i]);
        }

        printf("Digite a opcao (1 a 5): ");
        scanf("%d", &escolha);

        if (escolha < 1 || escolha > 5) {
            printf("Opcao invalida!\n");
            continue;
        }

        int numeroEscolhido = opcoes[escolha - 1];

        if (numeroEscolhido == MNumber) {
            printf("\nVoce venceu! O numero era %d\n", MNumber);
            acertou = 1;
        } else {
            if (numeroEscolhido < MNumber) {
                printf("\nErrou! O numero secreto eh MAIOR que %d\n", numeroEscolhido);
                min = numeroEscolhido + 1; 
            } else {
                printf("\nErrou! O numero secreto eh MENOR que %d\n", numeroEscolhido);
                max = numeroEscolhido - 1; 
            }
        }
    }

    return 0;
}
