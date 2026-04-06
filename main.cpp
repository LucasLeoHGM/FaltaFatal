#include <stdio.h>

int main() {
    float vendas, comissao;

    printf("Entre com a venda em reais (-1 para finalizar): ");
    scanf("%f", &vendas);

    while (vendas != -1) {
        comissao = 150 + (vendas * 0.10);

        printf("Comissao: R$%.2f\n", comissao);

        printf("Entre com a venda em reais (-1 para finalizar): ");
        scanf("%f", &vendas);
    }

    return 0;
}