#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    int MNumber, escolha, i, rodada = 0, min = 1, max = 100, MAttack, vidas = 5, qntOpcoes = 3, sala = 1;

    srand(time(NULL));
    MNumber = (rand() % 100) + 1;

    while (1) {
        printf("\n=== SALA %d ===\n", sala);
        printf("Mnumber: %d", MNumber); // !!!!!!!!!!!!!!! TIRAR O PRINT DPS !!!!!!!!!!!!!!!
        rodada++;

        int atkSala;
        if (sala <= 2) atkSala = 3;
        else if (sala <=4) atkSala = 2;
        else atkSala = 1;
        
        printf("\n--- Rodada %d ---\n", rodada);

        if (MAttack == 0) {
            printf("\n!!!O MONSTRO ATACOU!!!\n");
            MAttack = atkSala;
            vidas--;
            qntOpcoes++;
        }

        if (vidas <= 0) {
                printf("\nVoce morreu!\n");
                return 0; 
            }

        printf("--- Vidas: %d ---\n", vidas);
        printf("Escolha um numero:\n");

        int opcoes[qntOpcoes];       

        for (i = 0; i < qntOpcoes; i++) {
            opcoes[i] = (rand() % (max - min + 1)) + min;
        }
        

        for (i = 0; i < qntOpcoes; i++) {
            printf("%d - %d\n", i + 1, opcoes[i]);
        }

        printf("Digite a opcao (1 a %d): ", qntOpcoes);
        scanf("%d", &escolha);

        if (escolha < 1 || escolha > qntOpcoes) {
            printf("Opcao invalida!\n");
            continue;
        }

        int numeroEscolhido = opcoes[escolha - 1];

        if (numeroEscolhido == MNumber) {
            printf("\nVoce venceu! O numero era %d\n", MNumber);
            sala++;
            rodada = 0;
            min = 1;
            max = 100;
            MNumber = (rand() % 100) + 1;
            qntOpcoes = 3;
            if (sala <= 2) MAttack = 3;
            else if (sala <= 4) MAttack = 2;
            else MAttack = 1;
            if (sala == 6){
                printf("\nParabens!!! Voce zerou o jogo!!!");
                return 0;
            }
        } else {
            MAttack--;
            if (numeroEscolhido < MNumber) {
                printf("\n\n\n\n\n\n\n\n\n\n\n\nErrou!!! O numero secreto eh MAIOR que %d\n", numeroEscolhido);
                min = numeroEscolhido + 1;
            } else {
                printf("\n\n\n\n\n\n\n\n\n\n\n\nErrou!!! O numero secreto eh MENOR que %d\n", numeroEscolhido);
                max = numeroEscolhido - 1; 
            }
        }
    }
}
