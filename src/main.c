#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ===========================================================
//  SISTEMA DE COORDENADAS VIRTUAIS
//  O jogo roda numa "tela virtual" de 900x650.
//  Tudo e desenhado nessa resolucao e depois escalado para
//  caber na janela real (inclusive fullscreen), mantendo
//  proporcao com letterbox preto nas bordas se necessario.
// ===========================================================
#define VIRT_W 900
#define VIRT_H 650

typedef enum {
    MENU,
    MODE_SELECT,
    GAMEPLAY,
    GAMEOVER,
    WIN,
    STATS,
    CHEST,
    PATH_CHOICE
} GameState;

// -----------------------------------------------------------
//  Retorna escala e offset para o letterbox
// -----------------------------------------------------------
static void CalcLetterbox(float *scale, float *offX, float *offY)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float sx = (float)sw / VIRT_W;
    float sy = (float)sh / VIRT_H;
    *scale = (sx < sy) ? sx : sy;
    *offX  = (sw - VIRT_W * (*scale)) * 0.5f;
    *offY  = (sh - VIRT_H * (*scale)) * 0.5f;
}

// Converte posicao do mouse real -> coordenada virtual
static Vector2 MouseVirtual(void)
{
    float scale, offX, offY;
    CalcLetterbox(&scale, &offX, &offY);
    Vector2 m = GetMousePosition();
    return (Vector2){
        (m.x - offX) / scale,
        (m.y - offY) / scale
    };
}

// ===========================================================
//  SISTEMA DE DANO
// ===========================================================
static int calcularDano(int dado, int alvo)
{
    int d = abs(dado - alvo);
    if (d == 0)       return 40;
    else if (d <= 2)  return 25;
    else if (d <= 10) return 15;
    else if (d <= 20) return 10;
    else if (d <= 30) return  5;
    else              return  0;
}

// ===========================================================
//  HISTORICO
// ===========================================================
static void SalvarHistorico(const char *resultado, int sala, int modoDificil)
{
    FILE *f = fopen(modoDificil ? "../historico_dificil.txt"
                                : "../historico_facil.txt", "a");
    if (!f) return;
    if (strcmp(resultado, "VENCEU") == 0)
        fprintf(f, "VENCEU\n");
    else
        fprintf(f, "%s - Sala %d\n", resultado, sala);
    fclose(f);
}

static int LerHistorico(const char *arquivo, char linhas[][100], int max)
{
    FILE *f = fopen(arquivo, "r");
    if (!f) return 0;
    int i = 0;
    while (fgets(linhas[i], 100, f) && i < max - 1) i++;
    fclose(f);
    return i;
}

// ===========================================================
//  ESTRUTURA DE ENIGMA DE CAMINHO
// ===========================================================
#define MAX_LINHAS_ENIGMA 6

typedef struct {
    const char *linhas[MAX_LINHAS_ENIGMA];
    int caminhoCerto;   // 0=vermelho  1=verde  2=azul
    const char *bonusDesc;
    const char *penalDesc;
} PathEvent;

static PathEvent pathEvents[] = {

    // ENIGMA 1 — bicondicional + negacao
    {
        {
            "VERDE so e seguro se VERMELHO esta monitorado",
            "    OU se AZUL esta corrompido.",
            "VERMELHO nao esta sendo monitorado.",
            "AZUL nao apresenta corrupcao.",
            "O kernel registrou AZUL como ultimo processo limpo.",
            NULL
        },
        2,
        "Bonus: Monstro inicia com -25 HP!",
        "Penalidade: Perde 2 vidas!"
    },

    // ENIGMA 2 — modus tollens encadeado
    {
        {
            "Exatamente um caminho esta sincronizado com o clock.",
            "Se VERMELHO esta sincronizado, entao AZUL esta travado.",
            "AZUL nao esta travado.",
            "Se VERDE esta sincronizado, entao VERMELHO esta isolado.",
            "VERMELHO nao esta isolado.",
            NULL
        },
        2,
        "Bonus: +2 vidas extras!",
        "Penalidade: Monstro ganha +20 HP!"
    },

    // ENIGMA 3 — conjuncao de condicoes
    {
        {
            "Um caminho e seguro somente se NAO esta corrompido",
            "    E NAO esta travado.",
            "O caminho VERDE esta corrompido.",
            "O caminho AZUL esta travado em deadlock.",
            "O caminho VERMELHO nao esta corrompido nem travado.",
            NULL
        },
        0,
        "Bonus: Monstro inicia com -30 HP!",
        "Penalidade: Perde 1 vida!"
    },

    // ENIGMA 4 — cadeia condicional + modus tollens
    {
        {
            "Se VERMELHO esta acessivel, entao VERDE esta acessivel.",
            "Se VERDE esta acessivel, entao AZUL esta isolado da rede.",
            "AZUL nao esta isolado.",
            "Pelo menos um caminho esta acessivel no barramento.",
            NULL, NULL
        },
        2,
        "Bonus: +2 vidas extras!",
        "Penalidade: Perde 2 vidas!"
    },

    // ENIGMA 5 — duplo modus tollens + regra default
    {
        {
            "VERMELHO ou VERDE tem firewall ativo (ou ambos).",
            "Se VERMELHO tem firewall, a porta de saida e bloqueada.",
            "A porta de saida NAO esta bloqueada.",
            "Se VERDE tem firewall, o processo entra em loop infinito.",
            "O processo NAO esta em loop infinito.",
            "Se nenhum tem firewall, VERDE e o gateway padrao."
        },
        1,
        "Bonus: Monstro inicia com -20 HP!",
        "Penalidade: Perde 1 vida!"
    },

    // ENIGMA 6 — mutex / exclusao mutua
    {
        {
            "Tres processos disputam um unico bloco de memoria.",
            "VERMELHO alocou o recurso primeiro (mutex adquirido).",
            "O detentor do mutex nao pode ser corrompido.",
            "VERDE e AZUL estao bloqueados aguardando o recurso.",
            "Apenas o detentor do mutex pode ser atravessado.",
            NULL
        },
        0,
        "Bonus: +2 vidas extras!",
        "Penalidade: Monstro ganha +15 HP!"
    },

    // ENIGMA 7 — checksum
    {
        {
            "Exatamente dois caminhos estao com checksum invalido.",
            "O caminho VERMELHO tem checksum invalido.",
            "O caminho VERDE tem checksum invalido.",
            "Apenas o caminho com checksum valido e seguro.",
            NULL, NULL
        },
        2,
        "Bonus: Monstro inicia com -25 HP!",
        "Penalidade: Perde 2 vidas!"
    },

    // ENIGMA 8 — buffer overflow encadeado
    {
        {
            "Se AZUL esta online, VERMELHO sofre buffer overflow.",
            "Se VERMELHO sofre overflow, ele trava imediatamente.",
            "VERMELHO nao esta travado.",
            "Se VERDE esta online, AZUL e desativado.",
            "Pelo menos um caminho esta online.",
            NULL
        },
        1,
        "Bonus: +2 vidas extras!",
        "Penalidade: Perde 1 vida!"
    },

    // ENIGMA 9 — XOR logico + modus tollens
    {
        {
            "VERMELHO ou VERDE esta seguro, mas nao os dois.",
            "Se VERDE esta seguro, entao AZUL esta corrompido.",
            "AZUL nao esta corrompido.",
            "Apenas o caminho seguro pode ser transitado.",
            NULL, NULL
        },
        0,
        "Bonus: Monstro inicia com -30 HP!",
        "Penalidade: Monstro ganha +20 HP!"
    },

    // ENIGMA 10 — round-robin + PID
    {
        {
            "Sistema usa round-robin: um caminho ativo por vez.",
            "VERMELHO esgotou seu quantum de CPU e foi bloqueado.",
            "VERDE e AZUL ainda nao receberam quantum.",
            "O escalonador prioriza o processo de menor PID.",
            "VERDE tem PID menor que AZUL.",
            NULL
        },
        1,
        "Bonus: Monstro inicia com -20 HP!",
        "Penalidade: Perde 2 vidas!"
    },
};

#define NUM_PATH_EVENTS (sizeof(pathEvents) / sizeof(pathEvents[0]))

// ===========================================================
//  HELPER: desenha textura ocupando um Rectangle virtual
// ===========================================================
static void DrawTexVirt(Texture2D t, Rectangle dst, Color tint)
{
    DrawTexturePro(t,
        (Rectangle){0, 0, (float)t.width, (float)t.height},
        dst,
        (Vector2){0, 0}, 0.0f, tint);
}

// ===========================================================
//  MAIN
// ===========================================================
int main(void)
{
    InitWindow(VIRT_W, VIRT_H, "Falta Fatal");
    InitAudioDevice();
    SetTargetFPS(60);
    srand((unsigned)time(NULL));

    // Canvas virtual — tudo e renderizado aqui em 900x650
    RenderTexture2D canvas = LoadRenderTexture(VIRT_W, VIRT_H);

    // --- Assets ---
    Texture2D menuBg      = LoadTexture("../assets/telademenu.png");
    Texture2D gameBg      = LoadTexture("../assets/forest.jpg");
    Texture2D circuitBg   = LoadTexture("../assets/circuit.jpg");
    Texture2D btnNovoJogo = LoadTexture("../assets/novojogobotao.png");
    Texture2D btnFacil    = LoadTexture("../assets/facilbotao.png");
    Texture2D btnDificil  = LoadTexture("../assets/dificilbotao.png");
    Texture2D btnStats    = LoadTexture("../assets/estatisticasbotao.png");
    Texture2D btnSair     = LoadTexture("../assets/saairbotao.png");

    Music menuMusic = LoadMusicStream("../assets/menu.wav");
    Music gameMusic = LoadMusicStream("../assets/soundtrack.wav");
    Sound openSound  = LoadSound("../assets/open.wav");
    Sound closeSound = LoadSound("../assets/close.wav");

    PlayMusicStream(menuMusic);

    // --- Estado ---
    GameState state = MENU;

    int MNumber = 0, rodada = 0, minN = 1, maxN = 100, novaRodada = 0;
    int monsterHP = 50, vidas = 5, qntOpcoes = 5, sala = 1;
    int resultadoSalvo = 0, modoDificil = 0;

    int opcoesP1[10], opcoesP2[10];

    // Indice do botao selecionado (-1 = nenhum ainda)
    int idxEscolhaP1 = -1;
    int idxEscolhaP2 = -1;
    int escolhaPathP1 = -1;
    int escolhaPathP2 = -1;

    // Linha 1: resultado do turno / dica de range
    char mensagem[200]       = "Escolham um numero";
    // Linha 2: aviso de ataque do monstro (so aparece quando relevante)
    char mensagemMonstro[200] = "";

    // --- Path choice ---
    int pathEventIndex   = 0;
    int bonusMonsterHP   = 0;
    int bonusVidas       = 0;
    int penalVidas       = 0;
    int penalMonsterHP   = 0;
    char pathResultMsg[200] = "";
    int pathEscolhaP1 = -1;
    int pathEscolhaP2 = -1;
    int  pathResultTimer    = 0;

    // --- Historico ---
    char historicoFacil[50][100];
    char historicoDificil[50][100];
    int  totalFacil   = 0;
    int  totalDificil = 0;

    while (!WindowShouldClose())
    {
        // -------------------------------------------------------
        //  F11 = toggle fullscreen com letterbox automatico
        // -------------------------------------------------------
        if (IsKeyPressed(KEY_F11))
        {
            if (IsWindowFullscreen())
            {
                ToggleFullscreen();
                SetWindowSize(VIRT_W, VIRT_H);
            }
            else
            {
                int mon = GetCurrentMonitor();
                SetWindowSize(GetMonitorWidth(mon), GetMonitorHeight(mon));
                ToggleFullscreen();
            }
        }

        // -------------------------------------------------------
        //  Musica
        // -------------------------------------------------------
        if (state == MENU || state == STATS || state == MODE_SELECT)
            UpdateMusicStream(menuMusic);
        else
            UpdateMusicStream(gameMusic);

        // Mouse em coordenadas virtuais (correto em qualquer tamanho de janela)
        Vector2 mouse = MouseVirtual();

        // =======================================================
        //  LOGICA (sem Draw)
        // =======================================================

        if (state == MENU)
        {
            Rectangle btnJogarRec = {300,140,300,110};
            Rectangle btnStatsRec = {300,280,300,110};
            Rectangle btnSairRec  = {300,420,300,110};

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(mouse, btnJogarRec)) state = MODE_SELECT;

                if (CheckCollisionPointRec(mouse, btnStatsRec))
                {
                    totalFacil   = LerHistorico("../historico_facil.txt",   historicoFacil,   50);
                    totalDificil = LerHistorico("../historico_dificil.txt",  historicoDificil, 50);
                    state = STATS;
                }

                if (CheckCollisionPointRec(mouse, btnSairRec)) break;
            }
        }

        else if (state == MODE_SELECT)
        {
            Rectangle btnFacilRec   = {300,190,270,100};
            Rectangle btnDificilRec = {300,350,270,100};

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                bool hF = CheckCollisionPointRec(mouse, btnFacilRec);
                bool hD = CheckCollisionPointRec(mouse, btnDificilRec);

                if (hF || hD)
                {
                    modoDificil = hD ? 1 : 0;
                    StopMusicStream(menuMusic);
                    PlayMusicStream(gameMusic);

                    MNumber    = (rand() % 100) + 1;
                    printf("DEBUG: %d\n", MNumber);

                    rodada     = 0;
                    minN       = 1;
                    maxN       = 100;
                    novaRodada = 1;
                    monsterHP  = 50;
                    vidas      = 5;
                    qntOpcoes  = 5;
                    sala       = 1;
                    resultadoSalvo     = 0;
                    bonusMonsterHP     = 0;
                    bonusVidas         = 0;
                    penalVidas         = 0;
                    penalMonsterHP     = 0;
                    idxEscolhaP1       = -1;
                    idxEscolhaP2       = -1;
                    mensagemMonstro[0] = '\0';

                    snprintf(mensagem, 200, "Escolham um numero");
                    state = GAMEPLAY;
                }
            }
        }

        else if (state == STATS)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                state = MENU;
        }

        else if (state == CHEST)
        {
            Rectangle btnSim = {300,300,120,50};
            Rectangle btnNao = {480,300,120,50};

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                bool hSim = CheckCollisionPointRec(mouse, btnSim);
                bool hNao = CheckCollisionPointRec(mouse, btnNao);

                if (hSim || hNao)
                {
                    if (hSim) PlaySound(openSound);
                    if (hNao) PlaySound(closeSound);

                    if (sala >= 6)
                        state = WIN;
                    else
                    {
                        pathEventIndex   = rand() % NUM_PATH_EVENTS;
                        printf("DEBUG PATH EVENT: %d | Caminho correto: %d\n",
                            pathEventIndex,
                            pathEvents[pathEventIndex].caminhoCerto);
                        pathResultTimer  = 0;
                        pathResultMsg[0] = '\0';
                        escolhaPathP1 = -1;
                        escolhaPathP2 = -1;
                        state = PATH_CHOICE;
                    }
                }
            }
        }

        else if (state == PATH_CHOICE)
        {
            Rectangle pathRed   = { 50, 310, 220, 200};
            Rectangle pathGreen = {340, 290, 220, 220};
            Rectangle pathBlue  = {630, 310, 220, 200};

            if (pathResultTimer > 0)
            {
                pathResultTimer--;

                if (pathResultTimer == 0)
                {
                    vidas += bonusVidas;
                    vidas -= penalVidas;

                    if (vidas < 1)
                        vidas = 1;

                    rodada = 0;
                    minN = 1;
                    maxN = 100;

                    MNumber = (rand() % 100) + 1;
                    printf("DEBUG: %d\n", MNumber);

                    monsterHP = 50 + (sala * 25);
                    monsterHP -= bonusMonsterHP;
                    monsterHP += penalMonsterHP;

                    if (monsterHP < 10)
                        monsterHP = 10;

                    bonusMonsterHP = 0;
                    bonusVidas = 0;
                    penalVidas = 0;
                    penalMonsterHP = 0;

                    qntOpcoes = 5;
                    novaRodada = 1;

                    idxEscolhaP1 = -1;
                    idxEscolhaP2 = -1;

                    pathEscolhaP1 = -1;
                    pathEscolhaP2 = -1;

                    mensagemMonstro[0] = '\0';

                    snprintf(mensagem, 200, "Escolham um numero");

                    state = GAMEPLAY;
                }
            }
            else
            {
                PathEvent *ev = &pathEvents[pathEventIndex];

                // =========================================
                // PLAYER 1 -> 1 2 3
                // =========================================

                if (IsKeyPressed(KEY_ONE))
                    pathEscolhaP1 = 0;

                if (IsKeyPressed(KEY_TWO))
                    pathEscolhaP1 = 1;

                if (IsKeyPressed(KEY_THREE))
                    pathEscolhaP1 = 2;

                // =========================================
                // PLAYER 2 -> Q W E
                // =========================================

                if (IsKeyPressed(KEY_Q))
                    pathEscolhaP2 = 0;

                if (IsKeyPressed(KEY_W))
                    pathEscolhaP2 = 1;

                if (IsKeyPressed(KEY_E))
                    pathEscolhaP2 = 2;

                // =========================================
                // Só confirma se os DOIS escolherem igual
                // =========================================

                if (pathEscolhaP1 != -1 &&
                    pathEscolhaP2 != -1)
                {
                    if (pathEscolhaP1 == pathEscolhaP2)
                    {
                        int escolhido = pathEscolhaP1;

                        if (escolhido == ev->caminhoCerto)
                        {
                            snprintf(pathResultMsg, 200,
                                "CAMINHO CERTO! %s",
                                ev->bonusDesc);

                            if (strstr(ev->bonusDesc, "vida"))
                            {
                                int n = 2;

                                sscanf(ev->bonusDesc,
                                    "Bonus: +%d vida",
                                    &n);

                                bonusVidas = n;
                                bonusMonsterHP = 0;
                            }
                            else
                            {
                                int hp = 0;

                                sscanf(ev->bonusDesc,
                                    "Bonus: Monstro inicia com -%d HP!",
                                    &hp);

                                bonusMonsterHP = hp;
                                bonusVidas = 0;
                            }

                            penalVidas = 0;
                            penalMonsterHP = 0;
                        }
                        else
                        {
                            snprintf(pathResultMsg, 200,
                                "CAMINHO ERRADO! %s",
                                ev->penalDesc);

                            if (strstr(ev->penalDesc, "vida"))
                            {
                                int n = 1;

                                sscanf(ev->penalDesc,
                                    "Penalidade: Perde %d vida",
                                    &n);

                                penalVidas = n;
                                penalMonsterHP = 0;
                            }
                            else
                            {
                                int hp = 0;

                                sscanf(ev->penalDesc,
                                    "Penalidade: Monstro ganha +%d HP!",
                                    &hp);

                                penalMonsterHP = hp;
                                penalVidas = 0;
                            }

                            bonusMonsterHP = 0;
                            bonusVidas = 0;
                        }

                        pathResultTimer = 180;
                    }
                }
            }
        }

        else if (state == GAMEPLAY)
        {
            if (novaRodada)
            {
                rodada++;
                for (int i = 0; i < qntOpcoes; i++)
                {
                    opcoesP1[i] = (rand() % (maxN - minN + 1)) + minN;
                    opcoesP2[i] = (rand() % (maxN - minN + 1)) + minN;
                }
                idxEscolhaP1 = -1;
                idxEscolhaP2 = -1;
                novaRodada   = 0;
            }

            // ---- Entrada P1: teclas 1-5 ou clique esquerdo ----
            for (int i = 0; i < qntOpcoes; i++)
            {
                Rectangle btn = {80.0f + i*120.0f, 370.0f, 100.0f, 50.0f};
                bool hover    = CheckCollisionPointRec(mouse, btn);

                int teclasP1[5] =
                {
                    KEY_ONE,
                    KEY_TWO,
                    KEY_THREE,
                    KEY_FOUR,
                    KEY_FIVE
                };

                if (IsKeyPressed(teclasP1[i]) ||
                    (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)))
                {
                    idxEscolhaP1 = i;
                }
            }

            // ---- Entrada P2: teclas Q-T ou clique direito ----
            int teclasP2[5] = {
                KEY_Q, 
                KEY_W, 
                KEY_E, 
                KEY_R, 
                KEY_T };

            for (int i = 0; i < qntOpcoes; i++)
            {
                Rectangle btn = {80.0f + i*120.0f, 490.0f, 100.0f, 50.0f};
                bool hover    = CheckCollisionPointRec(mouse, btn);

                if (IsKeyPressed(teclasP2[i]) ||
                    (hover && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)))
                {
                    idxEscolhaP2 = i;
                }
            }

            // ---- Resolve turno quando ambos escolheram ----
            if (idxEscolhaP1 != -1 && idxEscolhaP2 != -1)
            {
                int escolhaP1 = opcoesP1[idxEscolhaP1];
                int escolhaP2 = opcoesP2[idxEscolhaP2];

                int danoP1    = calcularDano(escolhaP1, MNumber);
                int danoP2    = calcularDano(escolhaP2, MNumber);
                int danoTotal = danoP1 + danoP2;

                monsterHP -= danoTotal;
                if (monsterHP < 0) monsterHP = 0;

                mensagemMonstro[0] = '\0'; // limpa aviso anterior

                char dica[100] = "";

                if (!modoDificil)
                {
                    int media = (escolhaP1 + escolhaP2) / 2;

                    if (media < MNumber)
                    {
                        minN = media + 1;
                        snprintf(dica, 100, "Numero maior que %d", media);
                    }
                    else if (media > MNumber)
                    {
                        maxN = media - 1;
                        snprintf(dica, 100, "Numero menor que %d", media);
                    }
                }

                if (modoDificil)
                    snprintf(mensagem, 200,
                        "P1:%d dano | P2:%d dano | Total:%d",
                        danoP1, danoP2, danoTotal);
                else
                    snprintf(mensagem, 200,
                        "P1:%d | P2:%d | Total:%d | %s",
                        danoP1, danoP2, danoTotal, dica);

                if (monsterHP <= 0)
                {
                    sala++;
                    state = (sala >= 6) ? WIN : CHEST;
                }

                novaRodada = 1;
            }

            // ---- Ataque do monstro a cada 5 rodadas ----
            // So dispara enquanto jogadores ainda nao resolveram (novaRodada==0)
            if (rodada % 5 == 0 && novaRodada == 0)
            {
                vidas--;
                rodada++;
                // Aviso na linha separada — nao sobrescreve dano/dica
                snprintf(mensagemMonstro, 200,
                    ">>> O monstro atacou! -1 vida! <<<");
            }

            if (vidas <= 0)
            {
                if (!resultadoSalvo)
                {
                    SalvarHistorico("MORREU", sala, modoDificil);
                    resultadoSalvo = 1;
                }
                state = GAMEOVER;
            }
        }

        else if (state == GAMEOVER)
        {
            Rectangle r = {350,300,200,50};
            Rectangle e = {350,370,200,50};

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(mouse, r)) state = MENU;
                if (CheckCollisionPointRec(mouse, e)) break;
            }
        }

        else if (state == WIN)
        {
            if (!resultadoSalvo)
            {
                SalvarHistorico("VENCEU", sala, modoDificil);
                resultadoSalvo = 1;
            }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                state = MENU;
        }

        // =======================================================
        //  DRAW — tudo no canvas virtual 900x650
        // =======================================================
        BeginTextureMode(canvas);
        ClearBackground(BLACK);

        // ---- MENU ----
        if (state == MENU)
        {
            Rectangle btnJogarRec = {300,140,300,110};
            Rectangle btnStatsRec = {300,280,300,110};
            Rectangle btnSairRec  = {300,420,300,110};

            bool hJ = CheckCollisionPointRec(mouse, btnJogarRec);
            bool hS = CheckCollisionPointRec(mouse, btnStatsRec);
            bool hX = CheckCollisionPointRec(mouse, btnSairRec);

            DrawTexVirt(menuBg,     (Rectangle){0,0,VIRT_W,VIRT_H}, WHITE);
            DrawTexVirt(btnNovoJogo, btnJogarRec, hJ ? LIGHTGRAY : WHITE);
            DrawTexVirt(btnStats,    btnStatsRec, hS ? LIGHTGRAY : WHITE);
            DrawTexVirt(btnSair,     btnSairRec,  hX ? LIGHTGRAY : WHITE);
        }

        // ---- MODE SELECT ----
        else if (state == MODE_SELECT)
        {
            Rectangle btnFacilRec   = {300,190,270,100};
            Rectangle btnDificilRec = {300,350,270,100};

            bool hF = CheckCollisionPointRec(mouse, btnFacilRec);
            bool hD = CheckCollisionPointRec(mouse, btnDificilRec);

            DrawTexVirt(menuBg,     (Rectangle){0,0,VIRT_W,VIRT_H}, WHITE);
            DrawText("ESCOLHA O MODO", 230, 120, 40, BLACK);
            DrawTexVirt(btnFacil,   btnFacilRec,   hF ? LIGHTGRAY : WHITE);
            DrawTexVirt(btnDificil, btnDificilRec,  hD ? LIGHTGRAY : WHITE);
        }

        // ---- STATS ----
        else if (state == STATS)
        {
            DrawTexVirt(menuBg, (Rectangle){0,0,VIRT_W,VIRT_H}, WHITE);
            DrawText("ESTATISTICAS", 300, 30, 40, BLACK);
            DrawLine(450, 80, 450, 600, BLACK);
            DrawText("FACIL",   170, 90, 30, DARKGREEN);
            DrawText("DIFICIL", 590, 90, 30, MAROON);

            for (int i = 0; i < totalFacil;  i++)
                DrawText(historicoFacil[i],   50, 140+(i*22), 20, DARKGREEN);
            for (int i = 0; i < totalDificil; i++)
                DrawText(historicoDificil[i], 500, 140+(i*22), 20, MAROON);

            DrawText("Clique para voltar", 300, 610, 20, BLACK);
        }

        // ---- CHEST ----
        else if (state == CHEST)
        {
            Rectangle btnSim = {300,300,120,50};
            Rectangle btnNao = {480,300,120,50};

            bool hSim = CheckCollisionPointRec(mouse, btnSim);
            bool hNao = CheckCollisionPointRec(mouse, btnNao);

            ClearBackground(RAYWHITE);
            DrawText("VOCE DERROTOU O MONSTRO!", 180, 150, 30, BLACK);
            DrawText("Abrir seu bau de tesouros?", 200, 200, 25, DARKGRAY);

            DrawRectangleRec(btnSim, hSim ? GREEN   : DARKGRAY);
            DrawRectangleRec(btnNao, hNao ? RED     : DARKGRAY);
            DrawText("SIM", (int)(btnSim.x+30), (int)(btnSim.y+15), 20, WHITE);
            DrawText("NAO", (int)(btnNao.x+30), (int)(btnNao.y+15), 20, WHITE);
        }

        // ---- PATH CHOICE ----
        else if (state == PATH_CHOICE)
        {
            PathEvent *ev = &pathEvents[pathEventIndex];

            Rectangle pathRed   = { 50, 310, 220, 200};
            Rectangle pathGreen = {340, 290, 220, 220};
            Rectangle pathBlue  = {630, 310, 220, 200};

            bool canClick = (pathResultTimer == 0);
            bool hR = canClick && CheckCollisionPointRec(mouse, pathRed);
            bool hG = canClick && CheckCollisionPointRec(mouse, pathGreen);
            bool hB = canClick && CheckCollisionPointRec(mouse, pathBlue);

            // Fundo estilo terminal
            DrawTexturePro(
                circuitBg,
                (Rectangle){
                    0,
                    0,
                    (float)circuitBg.width,
                    (float)circuitBg.height
                },
                (Rectangle){0,0,VIRT_W,VIRT_H},
                (Vector2){0,0},
                0.0f,
                WHITE
            );

            // Escurece o fundo para melhorar leitura
            DrawRectangle(
                0,
                0,
                VIRT_W,
                VIRT_H,
                (Color){0,0,0,120}
            );

            // Titulo
            const char *titulo = ">> ENCRUZILHADA DO SISTEMA <<";
            DrawText(titulo, VIRT_W/2 - MeasureText(titulo,28)/2, 12, 28,
                (Color){0,220,80,255});
            DrawLine(60, 50, 840, 50, (Color){0,180,60,120});

            // Painel do enigma
            DrawRectangle(55, 58, 790, 200, (Color){0,0,0,180});
            DrawRectangleLinesEx((Rectangle){55,58,790,200}, 1, (Color){0,180,60,160});
            DrawText("[KERNEL LOG] Analisando rotas de acesso...",
                70, 65, 16, (Color){0,160,50,255});
            DrawLine(56, 86, 844, 86, (Color){0,100,30,200});

            {
                int ly = 94;
                for (int li = 0; li < MAX_LINHAS_ENIGMA; li++)
                {
                    if (ev->linhas[li] == NULL) break;
                    Color lc = (li == 0)
                        ? (Color){240,220,60,255}
                        : (Color){200,220,200,255};
                    DrawText(ev->linhas[li], 70, ly, 18, lc);
                    ly += 24;
                }
            }

            DrawLine(60, 262, 840, 262, (Color){0,180,60,120});

            const char *instrucao = "[ Analise os logs e escolha o caminho seguro ]";
            DrawText(instrucao, VIRT_W/2 - MeasureText(instrucao,16)/2,
                268, 16, (Color){0,140,50,200});

            // Caminhos
            // VERMELHO
            {
                Color bg = hR ? (Color){255,100,100,255} : (Color){160,20,20,255};
                DrawRectangleRec(pathRed, bg);
                DrawRectangleLinesEx(pathRed, hR ? 4 : 2,
                    hR ? WHITE : (Color){255,120,120,255});
                DrawText("[PROC: 0x52]",
                    (int)(pathRed.x + pathRed.width/2 - MeasureText("[PROC: 0x52]",14)/2),
                    (int)(pathRed.y + 10), 14, (Color){255,180,180,200});
                DrawText("VERMELHO",
                    (int)(pathRed.x + pathRed.width/2 - MeasureText("VERMELHO",20)/2),
                    (int)(pathRed.y + pathRed.height/2 - 10), 20, WHITE);
                if (hR)
                    DrawText("< SELECIONAR",
                        (int)(pathRed.x+10), (int)(pathRed.y-22), 15,
                        (Color){255,160,160,255});
            }

            // VERDE
            {
                Color bg = hG ? (Color){80,255,100,255} : (Color){10,130,40,255};
                DrawRectangleRec(pathGreen, bg);
                DrawRectangleLinesEx(pathGreen, hG ? 4 : 2,
                    hG ? WHITE : (Color){80,200,100,255});
                DrawText("[PROC: 0x47]",
                    (int)(pathGreen.x + pathGreen.width/2 - MeasureText("[PROC: 0x47]",14)/2),
                    (int)(pathGreen.y + 10), 14, (Color){180,255,190,200});
                DrawText("VERDE",
                    (int)(pathGreen.x + pathGreen.width/2 - MeasureText("VERDE",20)/2),
                    (int)(pathGreen.y + pathGreen.height/2 - 10), 20, WHITE);
                if (hG)
                    DrawText("  SELECIONAR",
                        (int)(pathGreen.x+20), (int)(pathGreen.y-22), 15,
                        (Color){160,255,170,255});
            }

            // AZUL
            {
                Color bg = hB ? (Color){80,160,255,255} : (Color){15,40,160,255};
                DrawRectangleRec(pathBlue, bg);
                DrawRectangleLinesEx(pathBlue, hB ? 4 : 2,
                    hB ? WHITE : (Color){80,120,255,255});
                DrawText("[PROC: 0x42]",
                    (int)(pathBlue.x + pathBlue.width/2 - MeasureText("[PROC: 0x42]",14)/2),
                    (int)(pathBlue.y + 10), 14, (Color){160,190,255,200});
                DrawText("AZUL",
                    (int)(pathBlue.x + pathBlue.width/2 - MeasureText("AZUL",20)/2),
                    (int)(pathBlue.y + pathBlue.height/2 - 10), 20, WHITE);
                if (hB)
                    DrawText("  SELECIONAR >",
                        (int)(pathBlue.x+10), (int)(pathBlue.y-22), 15,
                        (Color){140,170,255,255});
            }

            // Resultado / barra de progresso
            if (pathResultTimer > 0)
            {
                DrawRectangle(80, 540, 740, 72, (Color){0,0,0,230});
                DrawRectangleLinesEx((Rectangle){80,540,740,72}, 2,
                    (Color){0,220,80,255});

                bool acertou = (strstr(pathResultMsg, "CERTO") != NULL);

                char fullMsg[256];
                snprintf(fullMsg, 256, "%s %s",
                    acertou ? "[OK] " : "[ERR]", pathResultMsg);

                DrawText(fullMsg,
                    VIRT_W/2 - MeasureText(fullMsg,19)/2,
                    550, 19,
                    acertou ? (Color){80,255,100,255} : (Color){255,80,80,255});

                float prog = (float)pathResultTimer / 180.0f;
                DrawRectangle(82, 600, (int)(736*prog), 10,
                    acertou ? (Color){0,200,60,255} : (Color){200,50,50,255});
                DrawRectangleLinesEx((Rectangle){82,600,736,10}, 1,
                    (Color){60,60,60,255});
            }
            else
            {
                const char *hint = "[ Clique em um caminho para prosseguir ]";
                DrawText(hint, VIRT_W/2 - MeasureText(hint,15)/2,
                    622, 15, (Color){0,100,40,180});
            }
        }

        // ---- GAMEPLAY ----
        else if (state == GAMEPLAY)
        {
            DrawTexVirt(gameBg, (Rectangle){0,0,VIRT_W,VIRT_H}, WHITE);

            // HUD topo
            DrawText(TextFormat("SALA: %d", sala), 50, 18, 28, BLACK);
            DrawText(modoDificil ? "MODO: DIFICIL" : "MODO: FACIL",
                640, 18, 23, modoDificil ? RED : DARKGREEN);
            DrawText(TextFormat("VIDAS: %d", vidas), 50, 52, 28, RED);
            DrawText(TextFormat("MONSTRO HP: %d", monsterHP), 50, 86, 28, DARKGREEN);

            // Linha 1: dano do turno / dica de range
            DrawText(mensagem, 50, 125, 21, BLUE);

            // Linha 2: aviso de ataque do monstro (cor vermelha, separada)
            if (mensagemMonstro[0] != '\0')
                DrawText(mensagemMonstro, 50, 152, 20, RED);

            // ---- PLAYER 1 ----
            DrawText("PLAYER 1", 50, 310, 24, BLUE);
            DrawText("Teclas 1-5 | clique esquerdo", 215, 315, 17, DARKBLUE);

            for (int i = 0; i < qntOpcoes; i++)
            {
                Rectangle btn = {80.0f + i*120.0f, 345.0f, 100.0f, 50.0f};
                bool hover    = CheckCollisionPointRec(mouse, btn);
                bool selected = (idxEscolhaP1 == i);

                // Cor: selecionado > hover > normal
                Color bg;
                if      (selected) bg = (Color){255,140,0,255};   // laranja vivo
                else if (hover)    bg = (Color){255,200,100,255};  // laranja claro
                else               bg = LIGHTGRAY;

                DrawRectangleRec(btn, bg);

                // Borda grossa quando selecionado
                if (selected)
                    DrawRectangleLinesEx(btn, 4, (Color){180,80,0,255});

                // Numero centralizado
                const char *numStr = TextFormat("%d", opcoesP1[i]);
                DrawText(numStr,
                    (int)(btn.x + 50 - MeasureText(numStr,20)/2),
                    (int)(btn.y + 15), 20, BLACK);
            }

            // Aviso enquanto P1 escolheu mas P2 ainda nao
            if (idxEscolhaP1 != -1 && idxEscolhaP2 == -1)
                DrawText("P1 pronto!  Aguardando P2...",
                    50, 405, 17, (Color){200,100,0,255});

            // ---- PLAYER 2 ----
            DrawText("PLAYER 2", 50, 435, 24, RED);
            DrawText("Teclas Q-T | clique direito", 215, 440, 17, MAROON);

            for (int i = 0; i < qntOpcoes; i++)
            {
                Rectangle btn = {80.0f + i*120.0f, 468.0f, 100.0f, 50.0f};
                bool hover    = CheckCollisionPointRec(mouse, btn);
                bool selected = (idxEscolhaP2 == i);

                Color bg;
                if      (selected) bg = (Color){210,50,110,255};   // rosa forte
                else if (hover)    bg = (Color){255,150,190,255};   // rosa claro
                else               bg = LIGHTGRAY;

                DrawRectangleRec(btn, bg);

                if (selected)
                    DrawRectangleLinesEx(btn, 4, (Color){140,0,70,255});

                const char *numStr = TextFormat("%d", opcoesP2[i]);
                DrawText(numStr,
                    (int)(btn.x + 50 - MeasureText(numStr,20)/2),
                    (int)(btn.y + 15), 20, BLACK);
            }

            // Aviso enquanto P2 escolheu mas P1 ainda nao
            if (idxEscolhaP2 != -1 && idxEscolhaP1 == -1)
                DrawText("P2 pronto!  Aguardando P1...",
                    50, 528, 17, (Color){160,0,80,255});

            // Dica F11
            DrawText("[F11] Fullscreen",
                VIRT_W - MeasureText("[F11] Fullscreen",15) - 8,
                VIRT_H - 22, 15, (Color){80,80,80,180});
        }

        // ---- GAME OVER ----
        else if (state == GAMEOVER)
        {
            DrawTexVirt(gameBg, (Rectangle){0,0,VIRT_W,VIRT_H}, WHITE);

            Rectangle r = {350,300,200,50};
            Rectangle e = {350,370,200,50};

            bool hr = CheckCollisionPointRec(mouse, r);
            bool he = CheckCollisionPointRec(mouse, e);

            DrawText("O LYCEUM TE DERROTOU", 180, 180, 42, RED);
            DrawText(
                "Lucas bloqueou sua invasao.",
                240,
                240,
                26,
                WHITE
            );

            DrawText(
                "Voce foi reprovado por falta.",
                230,
                280,
                26,
                WHITE
            );

            DrawRectangleRec(r, hr ? ORANGE : DARKGRAY);
            DrawRectangleRec(e, he ? ORANGE : DARKGRAY);
            DrawText("JOGAR NOVAMENTE", 360, 315, 18, WHITE);
            DrawText("SAIR",            430, 385, 20, WHITE);
        }

        // ---- WIN ----
        else if (state == WIN)
        {
            DrawTexVirt(gameBg, (Rectangle){0,0,VIRT_W,VIRT_H}, WHITE);
            DrawRectangle(0,0,VIRT_W,VIRT_H,(Color){0,0,0,150});
            DrawText(
                "ACESSO AO LYCEUM CONCEDIDO",
                150,
                170,
                42,
                GREEN
            );

            DrawText(
                "A falta foi removida.",
                270,
                260,
                30,
                WHITE
            );

            DrawText(
                "STATUS: APROVADO",
                270,
                320,
                36,
                YELLOW
            );

            DrawText(
                "Lucas observava tudo em silencio...",
                180,
                420,
                24,
                LIGHTGRAY
            );

            DrawText(
                "Clique para voltar ao menu",
                250,
                560,
                22,
                GRAY
            );
        }

        EndTextureMode();

        // =======================================================
        //  Escala o canvas para a janela real com letterbox
        // =======================================================
        float scale, offX, offY;
        CalcLetterbox(&scale, &offX, &offY);

        BeginDrawing();
        ClearBackground(BLACK);  // barras do letterbox

        // Render texture tem Y invertido no OpenGL: height negativa corrige
        DrawTexturePro(
            canvas.texture,
            (Rectangle){0.0f, 0.0f, (float)VIRT_W, -(float)VIRT_H},
            (Rectangle){offX, offY, VIRT_W * scale, VIRT_H * scale},
            (Vector2){0.0f, 0.0f},
            0.0f,
            WHITE
        );

        EndDrawing();
    }

    // --- Cleanup ---
    UnloadRenderTexture(canvas);
    UnloadTexture(menuBg);
    UnloadTexture(gameBg);
    UnloadTexture(circuitBg);
    UnloadTexture(btnNovoJogo);
    UnloadTexture(btnFacil);
    UnloadTexture(btnDificil);
    UnloadTexture(btnStats);
    UnloadTexture(btnSair);
    UnloadMusicStream(menuMusic);
    UnloadMusicStream(gameMusic);
    UnloadSound(openSound);
    UnloadSound(closeSound);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}