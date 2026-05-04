#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef enum {
    MENU,
    GAMEPLAY,
    GAMEOVER,
    WIN,
    STATS,
    CHEST
} GameState;

// ===== HISTORICO =====
void SalvarHistorico(const char *resultado, int sala) {
    FILE *f = fopen("../historico.txt", "a");
    if (f) {
        fprintf(f, "%s - Sala %d\n", resultado, sala);
        fclose(f);
    }
}

int LerHistorico(char linhas[][100], int max) {
    FILE *f = fopen("../historico.txt", "r");
    if (!f) return 0;

    int i = 0;
    while (fgets(linhas[i], 100, f) && i < max - 1) i++;
    fclose(f);
    return i;
}

int main() {
    InitWindow(900, 600, "Dice Warrior");
    InitAudioDevice();
    SetTargetFPS(60);
    srand(time(NULL));

    Texture2D menuBg = LoadTexture("../assets/menu.jpg");
    Texture2D gameBg = LoadTexture("../assets/forest.jpg");

    Music menuMusic = LoadMusicStream("../assets/menu.wav");
    Music gameMusic = LoadMusicStream("../assets/music.mp3");

    Sound openSound = LoadSound("../assets/open.wav");
    Sound closeSound = LoadSound("../assets/close.wav");

    PlayMusicStream(menuMusic);

    int MNumber, rodada, min, max, novaRodada;
    int MAttack, vidas, qntOpcoes, sala;
    int opcoes[20];

    GameState state = MENU;
    char mensagem[200];

    char historico[50][100];
    int totalLinhas = 0;

    while (!WindowShouldClose()) {

        // ===== MUSICA =====
        if (state == MENU || state == STATS)
            UpdateMusicStream(menuMusic);
        else
            UpdateMusicStream(gameMusic);

        // ===== MENU =====
        if (state == MENU) {

            Rectangle btnJogar = {350,200,200,50};
            Rectangle btnStats = {350,270,200,50};
            Rectangle btnSair  = {350,340,200,50};

            Vector2 mouse = GetMousePosition();

            bool hJ = CheckCollisionPointRec(mouse, btnJogar);
            bool hS = CheckCollisionPointRec(mouse, btnStats);
            bool hX = CheckCollisionPointRec(mouse, btnSair);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (hJ) {
                    StopMusicStream(menuMusic);
                    PlayMusicStream(gameMusic);

                    MNumber = (rand()%100)+1;
                    printf("DEBUG: %d\n", MNumber);

                    rodada=0; min=1; max=100; novaRodada=1;
                    MAttack=5; vidas=5; qntOpcoes=5; sala=1;

                    snprintf(mensagem,200,"Escolha um numero");
                    state = GAMEPLAY;
                }
                if (hS) {
                    totalLinhas = LerHistorico(historico,50);
                    state = STATS;
                }
                if (hX) break;
            }

            BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawTexturePro(menuBg,(Rectangle){0,0,menuBg.width,menuBg.height},
                (Rectangle){0,0,900,600},(Vector2){0,0},0,WHITE);

            DrawText("DICE WARRIOR",260,100,40,BLACK);

            DrawRectangleRec(btnJogar,hJ?RED:DARKGRAY);
            DrawRectangleRec(btnStats,hS?RED:DARKGRAY);
            DrawRectangleRec(btnSair,hX?RED:DARKGRAY);

            DrawText("JOGAR",410,215,20,WHITE);
            DrawText("ESTATISTICAS",370,285,20,WHITE);
            DrawText("SAIR",430,355,20,WHITE);

            EndDrawing();
            continue;
        }

        // ===== STATS =====
        if (state == STATS) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) state = MENU;

            BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawTexturePro(menuBg,(Rectangle){0,0,menuBg.width,menuBg.height},
                (Rectangle){0,0,900,600},(Vector2){0,0},0,WHITE);

            DrawText("HISTORICO:",50,50,30,BLACK);

            for (int i=0;i<totalLinhas;i++)
                DrawText(historico[i],50,100+i*20,20,DARKGRAY);

            DrawText("Clique para voltar",300,550,20,BLACK);

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

                if (hSim) PlaySound(openSound);
                if (hNao) PlaySound(closeSound);

                rodada=0; min=1; max=100;
                MNumber = (rand()%100)+1;
                printf("DEBUG: %d\n", MNumber);

                if (sala<=2) MAttack=4;
                else if (sala<=4) MAttack=3;
                else MAttack=2;

                qntOpcoes=5;

                if (sala==6) state=WIN;
                else state=GAMEPLAY;

                novaRodada=1;
            }

            BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawText("VOCE DERROTOU O MONSTRO!",180,150,30,BLACK);
            DrawText("Abrir seu bau de tesouros?",200,200,25,DARKGRAY);

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
                for (int i=0;i<qntOpcoes;i++)
                    opcoes[i]=(rand()%(max-min+1))+min;
                novaRodada=0;
            }

            Vector2 mouse = GetMousePosition();

            for (int i=0;i<qntOpcoes;i++) {

                Rectangle btn = {100+i*120,400,100,50};
                bool hover = CheckCollisionPointRec(mouse,btn);

                if ((hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || IsKeyPressed(KEY_ONE+i)) {

                    int num = opcoes[i];

                    if (num==MNumber) {
                        sala++;
                        state = CHEST;
                    } else {
                        MAttack--;
                        if (num<MNumber) {
                            min=num+1;
                            snprintf(mensagem,200,"Maior que %d",num);
                        } else {
                            max=num-1;
                            snprintf(mensagem,200,"Menor que %d",num);
                        }
                    }

                    novaRodada=1;
                }
            }

            if (MAttack<=0) {
                vidas--;
                MAttack=3;
                qntOpcoes++;
            }

            if (vidas<=0) {
                SalvarHistorico("MORREU",sala);
                state=GAMEOVER;
            }
        }

        // ===== DRAW =====
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawTexturePro(gameBg,(Rectangle){0,0,gameBg.width,gameBg.height},
            (Rectangle){0,0,900,600},(Vector2){0,0},0,WHITE);

        if (state == GAMEPLAY) {
            DrawText(TextFormat("SALA: %d",sala),50,20,30,BLACK);
            DrawText(TextFormat("VIDAS: %d",vidas),50,60,30,RED);
            DrawText(mensagem,50,120,25,BLUE);

            Vector2 mouse = GetMousePosition();

            for (int i=0;i<qntOpcoes;i++) {
                Rectangle btn={100+i*120,400,100,50};
                bool h=CheckCollisionPointRec(mouse,btn);

                DrawRectangleRec(btn,h?ORANGE:LIGHTGRAY);
                DrawText(TextFormat("%d",opcoes[i]),btn.x+30,btn.y+15,20,BLACK);
            }
        }

        else if (state == GAMEOVER) {

            Rectangle r={350,300,200,50};
            Rectangle e={350,370,200,50};

            Vector2 mouse=GetMousePosition();

            bool hr=CheckCollisionPointRec(mouse,r);
            bool he=CheckCollisionPointRec(mouse,e);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (hr) state=MENU;
                if (he) break;
            }

            DrawText("VOCE MORREU!",280,200,40,RED);

            DrawRectangleRec(r,hr?ORANGE:DARKGRAY);
            DrawRectangleRec(e,he?ORANGE:DARKGRAY);

            DrawText("JOGAR NOVAMENTE",360,315,18,WHITE);
            DrawText("SAIR",430,385,20,WHITE);
        }

        else if (state == WIN) {
            SalvarHistorico("VENCEU",sala);

            DrawText("VOCE VENCEU!",300,250,40,GREEN);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                state = MENU;
        }

        EndDrawing();
    }

    UnloadTexture(menuBg);
    UnloadTexture(gameBg);
    UnloadMusicStream(menuMusic);
    UnloadMusicStream(gameMusic);
    UnloadSound(openSound);
    UnloadSound(closeSound);

    CloseAudioDevice();
    CloseWindow();
}