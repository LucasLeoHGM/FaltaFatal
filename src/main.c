#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum {
    MENU,
    MODE_SELECT,
    GAMEPLAY,
    GAMEOVER,
    WIN,
    STATS,
    CHEST
} GameState;

// ===== SISTEMA DE DANO =====
int calcularDano(int dado, int alvo)
{
    int d = abs(dado - alvo);

    if (d == 0)
        return 40;
    else if (d <= 2)
        return 25;
    else if (d <= 10)
        return 15;
    else if (d <= 20)
        return 10;
    else if (d <= 30)
        return 5;
    else
        return 0;
}

// ===== HISTORICO =====
void SalvarHistorico(const char *resultado, int sala, int modoDificil)
{
    FILE *f;

    if (modoDificil)
        f = fopen("../historico_dificil.txt", "a");
    else
        f = fopen("../historico_facil.txt", "a");

    if (f) {

        // Se venceu, não mostra sala
        if (strcmp(resultado, "VENCEU") == 0)
            fprintf(f, "VENCEU\n");

        else
            fprintf(f, "%s - Sala %d\n", resultado, sala);

        fclose(f);
    }
}

int LerHistorico(const char *arquivo, char linhas[][100], int max)
{
    FILE *f = fopen(arquivo, "r");

    if (!f)
        return 0;

    int i = 0;

    while (fgets(linhas[i], 100, f) && i < max - 1)
        i++;

    fclose(f);

    return i;
}

int main()
{
    InitWindow(900, 650, "Dice Warrior");

    InitAudioDevice();

    SetTargetFPS(60);

    srand(time(NULL));

    Texture2D menuBg = LoadTexture("../assets/telademenu.png");
    Texture2D gameBg = LoadTexture("../assets/forest.jpg");
    Texture2D btnNovoJogo = LoadTexture("../assets/novojogobotao.png");
    Texture2D btnFacil = LoadTexture("../assets/facilbotao.png");
    Texture2D btnDificil = LoadTexture("../assets/dificilbotao.png");
    Texture2D btnStats = LoadTexture("../assets/estatisticasbotao.png");
    Texture2D btnSair = LoadTexture("../assets/saairbotao.png");

    Music menuMusic = LoadMusicStream("../assets/menu.wav");
    Music gameMusic = LoadMusicStream("../assets/soundtrack.wav");

    Sound openSound = LoadSound("../assets/open.wav");
    Sound closeSound = LoadSound("../assets/close.wav");

    PlayMusicStream(menuMusic);

    int MNumber;
    int rodada;
    int min;
    int max;
    int novaRodada;

    int monsterHP;
    int vidas;
    int qntOpcoes;
    int sala;

    int resultadoSalvo = 0;
    int modoDificil = 0;

    int opcoesP1[10];
    int opcoesP2[10];

    int escolhaP1 = -1;
    int escolhaP2 = -1;

    GameState state = MENU;

    char mensagem[200];

    char historicoFacil[50][100];
    char historicoDificil[50][100];

    int totalFacil = 0;
    int totalDificil = 0;

    while (!WindowShouldClose()) {

        // ===== MUSICA =====
        if (state == MENU || state == STATS || state == MODE_SELECT)
            UpdateMusicStream(menuMusic);
        else
            UpdateMusicStream(gameMusic);

        // ===== MENU =====
        if (state == MENU) {

            Rectangle btnJogarRec = {300,140,300,110};
            Rectangle btnStatsRec = {300,280,300,110};
            Rectangle btnSairRec  = {300,420,300,110};

            Vector2 mouse = GetMousePosition();

            bool hJ = CheckCollisionPointRec(mouse, btnJogarRec);
            bool hS = CheckCollisionPointRec(mouse, btnStatsRec);
            bool hX = CheckCollisionPointRec(mouse, btnSairRec);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

                if (hJ)
                    state = MODE_SELECT;

                if (hS) {

                    totalFacil = LerHistorico(
                        "../historico_facil.txt",
                        historicoFacil,
                        50
                    );

                    totalDificil = LerHistorico(
                        "../historico_dificil.txt",
                        historicoDificil,
                        50
                    );

                    state = STATS;
                }

                if (hX)
                    break;
            }

            BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawTexturePro(
                menuBg,
                (Rectangle){0,0,menuBg.width,menuBg.height},
                (Rectangle){0,0,900,650},
                (Vector2){0,0},
                0,
                WHITE
            );

            DrawTexturePro(
                btnNovoJogo,
                (Rectangle){0,0,btnNovoJogo.width,btnNovoJogo.height},
                btnJogarRec,
                (Vector2){0,0},
                0,
                hJ ? LIGHTGRAY : WHITE
            );

            DrawTexturePro(
                btnStats,
                (Rectangle){0,0,btnStats.width,btnStats.height},
                btnStatsRec,
                (Vector2){0,0},
                0,
                hS ? LIGHTGRAY : WHITE
            );

            DrawTexturePro(
                btnSair,
                (Rectangle){0,0,btnSair.width,btnSair.height},
                btnSairRec,
                (Vector2){0,0},
                0,
                hX ? LIGHTGRAY : WHITE
            );

            EndDrawing();

            continue;
        }

        // ===== SELECT MODE =====
        if (state == MODE_SELECT) {

            Rectangle btnFacilRec   = {300,190,270,100};
            Rectangle btnDificilRec = {300,350,270,100};

            Vector2 mouse = GetMousePosition();

            bool hF = CheckCollisionPointRec(mouse, btnFacilRec);
            bool hD = CheckCollisionPointRec(mouse, btnDificilRec);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

                if (hF || hD) {

                    modoDificil = hD;

                    StopMusicStream(menuMusic);
                    PlayMusicStream(gameMusic);

                    MNumber = (rand()%100)+1;

                    printf("DEBUG: %d\n", MNumber);

                    rodada = 0;
                    min = 1;
                    max = 100;
                    novaRodada = 1;

                    monsterHP = 50;

                    vidas = 5;
                    qntOpcoes = 5;
                    sala = 1;
                    resultadoSalvo = 0;

                    snprintf(mensagem,200,"Escolham um numero");

                    state = GAMEPLAY;
                }
            }

            BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawTexturePro(
                menuBg,
                (Rectangle){0,0,menuBg.width,menuBg.height},
                (Rectangle){0,0,900,650},
                (Vector2){0,0},
                0,
                WHITE
            );

            DrawText("ESCOLHA O MODO",230,120,40,BLACK);

            DrawTexturePro(
                btnFacil,
                (Rectangle){0,0,btnFacil.width,btnFacil.height},
                btnFacilRec,
                (Vector2){0,0},
                0,
                hF ? LIGHTGRAY : WHITE
            );

            DrawTexturePro(
                btnDificil,
                (Rectangle){0,0,btnDificil.width,btnDificil.height},
                btnDificilRec,
                (Vector2){0,0},
                0,
                hD ? LIGHTGRAY : WHITE
            );

            EndDrawing();

            continue;
        }

        // ===== STATS =====
        if (state == STATS) {

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                state = MENU;

            BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawTexturePro(
                menuBg,
                (Rectangle){0,0,menuBg.width,menuBg.height},
                (Rectangle){0,0,900,650},
                (Vector2){0,0},
                0,
                WHITE
            );

            DrawText("ESTATISTICAS",300,30,40,BLACK);

            DrawLine(450,80,450,600,BLACK);

            DrawText("FACIL",170,90,30,DARKGREEN);
            DrawText("DIFICIL",590,90,30,MAROON);

            for (int i = 0; i < totalFacil; i++) {

                DrawText(
                    historicoFacil[i],
                    50,
                    140 + (i * 22),
                    20,
                    DARKGREEN
                );
            }

            for (int i = 0; i < totalDificil; i++) {

                DrawText(
                    historicoDificil[i],
                    500,
                    140 + (i * 22),
                    20,
                    MAROON
                );
            }

            DrawText(
                "Clique para voltar",
                300,
                610,
                20,
                BLACK
            );

            EndDrawing();

            continue;
        }

        // ===== CHEST =====
        if (state == CHEST) {

            Rectangle btnSim = {300,300,120,50};
            Rectangle btnNao = {480,300,120,50};

            Vector2 mouse = GetMousePosition();

            bool hSim = CheckCollisionPointRec(mouse, btnSim);
            bool hNao = CheckCollisionPointRec(mouse, btnNao);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

                if (hSim || hNao) {

                    if (hSim)
                        PlaySound(openSound);

                    if (hNao)
                        PlaySound(closeSound);

                    rodada = 0;
                    min = 1;
                    max = 100;

                    MNumber = (rand()%100)+1;

                    printf("DEBUG: %d\n", MNumber);

                    monsterHP = 50 + (sala * 10);

                    qntOpcoes = 5;

                    if (sala == 6)
                        state = WIN;
                    else
                        state = GAMEPLAY;

                    novaRodada = 1;
                }
            }

            BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawText("VOCE DERROTOU O MONSTRO!",180,150,30,BLACK);

            DrawText(
                "Abrir seu bau de tesouros?",
                200,
                200,
                25,
                DARKGRAY
            );

            DrawRectangleRec(btnSim,hSim?GREEN:DARKGRAY);
            DrawRectangleRec(btnNao,hNao?RED:DARKGRAY);

            DrawText("SIM",btnSim.x+30,btnSim.y+15,20,WHITE);
            DrawText("NAO",btnNao.x+30,btnNao.y+15,20,WHITE);

            EndDrawing();

            continue;
        }

        // ===== GAMEPLAY =====
        if (state == GAMEPLAY) {

            if (novaRodada) {

                rodada++;

                for (int i = 0; i < qntOpcoes; i++) {

                    opcoesP1[i] = (rand() % (max - min + 1)) + min;
                    opcoesP2[i] = (rand() % (max - min + 1)) + min;
                }

                escolhaP1 = -1;
                escolhaP2 = -1;

                novaRodada = 0;
            }

            Vector2 mouse = GetMousePosition();

            // ===== PLAYER 1 =====
            for (int i = 0; i < qntOpcoes; i++) {

                Rectangle btnP1 = {80 + i * 120, 350, 100, 50};

                bool hover = CheckCollisionPointRec(mouse, btnP1);

                if ((hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    || IsKeyPressed(KEY_ONE + i)) {

                    escolhaP1 = opcoesP1[i];
                }
            }

            // ===== PLAYER 2 =====
            for (int i = 0; i < qntOpcoes; i++) {

                Rectangle btnP2 = {80 + i * 120, 470, 100, 50};

                bool hover = CheckCollisionPointRec(mouse, btnP2);

                if ((hover && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
                    || IsKeyPressed(KEY_Q + i)) {

                    escolhaP2 = opcoesP2[i];
                }
            }

            // ===== RESOLVE TURNO =====
            if (escolhaP1 != -1 && escolhaP2 != -1) {

                int danoP1 = calcularDano(escolhaP1, MNumber);
                int danoP2 = calcularDano(escolhaP2, MNumber);

                int danoTotal = danoP1 + danoP2;

                monsterHP -= danoTotal;

                if (monsterHP < 0)
                    monsterHP = 0;

                char dica[100] = "";

                if (!modoDificil) {

                    int media = (escolhaP1 + escolhaP2) / 2;

                    if (media < MNumber) {

                        min = media + 1;

                        snprintf(
                            dica,
                            100,
                            "Numero maior que %d",
                            media
                        );
                    }
                    else if (media > MNumber) {

                        max = media - 1;

                        snprintf(
                            dica,
                            100,
                            "Numero menor que %d",
                            media
                        );
                    }
                }

                if (modoDificil) {

                    snprintf(
                        mensagem,
                        200,
                        "P1:%d dano | P2:%d dano | Total:%d",
                        danoP1,
                        danoP2,
                        danoTotal
                    );
                }
                else {

                    snprintf(
                        mensagem,
                        200,
                        "P1:%d | P2:%d | Total:%d | %s",
                        danoP1,
                        danoP2,
                        danoTotal,
                        dica
                    );
                }

                if (monsterHP <= 0) {

                    sala++;

                    if (sala >= 6)
                        state = WIN;
                    else
                        state = CHEST;
                }

                novaRodada = 1;
            }

            if (rodada % 5 == 0 && novaRodada == 0) {

                vidas--;

                rodada++;

                snprintf(
                    mensagem,
                    200,
                    "O monstro atacou! Voce perdeu 1 vida!"
                );
            }

            if (vidas <= 0) {

                if (!resultadoSalvo) {

                    SalvarHistorico("MORREU", sala, modoDificil);

                    resultadoSalvo = 1;
                }

                state = GAMEOVER;
            }
        }

        // ===== DRAW =====
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawTexturePro(
            gameBg,
            (Rectangle){0,0,gameBg.width,gameBg.height},
            (Rectangle){0,0,900,650},
            (Vector2){0,0},
            0,
            WHITE
        );

        // ===== GAMEPLAY DRAW =====
        if (state == GAMEPLAY) {

            DrawText(
                TextFormat("SALA: %d", sala),
                50,
                20,
                30,
                BLACK
            );

            DrawText(
                modoDificil ? "MODO: DIFICIL" : "MODO: FACIL",
                650,
                20,
                25,
                modoDificil ? RED : DARKGREEN
            );

            DrawText(
                TextFormat("VIDAS: %d", vidas),
                50,
                60,
                30,
                RED
            );

            DrawText(
                TextFormat("MONSTRO HP: %d", monsterHP),
                50,
                100,
                30,
                DARKGREEN
            );

            DrawText(mensagem,50,150,25,BLUE);

            Vector2 mouse = GetMousePosition();

            DrawText("PLAYER 1", 50, 310, 25, BLUE);
            DrawText("Clique esquerdo ou teclas 1-5", 220, 315, 20, DARKBLUE);

            for (int i = 0; i < qntOpcoes; i++) {

                Rectangle btn = {80 + i * 120, 350, 100, 50};

                bool h = CheckCollisionPointRec(mouse, btn);

                DrawRectangleRec(btn, h ? ORANGE : LIGHTGRAY);

                DrawText(
                    TextFormat("%d", opcoesP1[i]),
                    btn.x + 30,
                    btn.y + 15,
                    20,
                    BLACK
                );
            }

            DrawText("PLAYER 2", 50, 430, 25, RED);
            DrawText("Clique direito ou teclas Q-T", 220, 435, 20, MAROON);

            for (int i = 0; i < qntOpcoes; i++) {

                Rectangle btn = {80 + i * 120, 470, 100, 50};

                bool h = CheckCollisionPointRec(mouse, btn);

                DrawRectangleRec(btn, h ? PINK : LIGHTGRAY);

                DrawText(
                    TextFormat("%d", opcoesP2[i]),
                    btn.x + 30,
                    btn.y + 15,
                    20,
                    BLACK
                );
            }
        }

        // ===== GAME OVER =====
        else if (state == GAMEOVER) {

            Rectangle r = {350,300,200,50};
            Rectangle e = {350,370,200,50};

            Vector2 mouse = GetMousePosition();

            bool hr = CheckCollisionPointRec(mouse,r);
            bool he = CheckCollisionPointRec(mouse,e);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

                if (hr)
                    state = MENU;

                if (he)
                    break;
            }

            DrawText("VOCE MORREU!",280,200,40,RED);

            DrawRectangleRec(r,hr?ORANGE:DARKGRAY);
            DrawRectangleRec(e,he?ORANGE:DARKGRAY);

            DrawText("JOGAR NOVAMENTE",360,315,18,WHITE);
            DrawText("SAIR",430,385,20,WHITE);
        }

        // ===== WIN =====
        else if (state == WIN) {

            if (!resultadoSalvo) {

                SalvarHistorico("VENCEU", sala, modoDificil);

                resultadoSalvo = 1;
            }

            DrawText("VOCE VENCEU!",300,250,40,GREEN);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                state = MENU;
        }

        EndDrawing();
    }

    UnloadTexture(menuBg);
    UnloadTexture(gameBg);
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