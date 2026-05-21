#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define VIRT_W 1280
#define VIRT_H 720

typedef enum {
    MENU,
    MODE_SELECT,
    LORE,
    GAMEPLAY,
    GAMEOVER,
    WIN,
    STATS,
    CHEST,
    PATH_CHOICE
} GameState;

static const char *bossNames[] = {
    "Mari Rata", "Roma, o Seguranca", "Luis, o Ninja",
    "Mica, a Advogata", "Ruan, o Galo Assassino",
    "Lucas, o Hacker", "Lucas, o Invasor"
};

static const char *bossDescriptions[] = {
    "As chuvas inundaram Recife e os ratos dominaram a rua.",
    "O seguranca do CESAR detectou movimentacao suspeita.",
    "Outro aluno tentando hackear o Lyceum apareceu.",
    "Ela ameaca processar qualquer invasor do sistema.",
    "Uma aberracao protege o servidor principal.",
    "O hacker responsavel pelo sistema finalmente apareceu.",
    "O sistema inteiro agora responde a Lucas, o PRIMEIRO invasor."
};

static const char *introLore[] = {
    "Voce usou todas as faltas da cadeira de sexta-feira.",
    "No ultimo dia, o professor marcou falta por engano.",
    "Depois de discutir no Slack sem sucesso...",
    "ele decidiu invadir o sistema do Lyceum.",
    "A chuva deixou o CESAR vazio.",
    "Essa era a chance perfeita.",
    NULL
};

#define MAX_LINHAS_ENIGMA 6

typedef struct {
    const char *linhas[MAX_LINHAS_ENIGMA];
    int caminhoCerto;
    const char *bonusDesc;
    const char *penalDesc;
} PathEvent;

static PathEvent pathEvents[] = {
    {{"VERDE so e seguro se VERMELHO esta monitorado","    OU se AZUL esta corrompido.","VERMELHO nao esta sendo monitorado.","AZUL nao apresenta corrupcao.","O kernel registrou AZUL como ultimo processo limpo.",NULL},2,"Bonus: Monstro inicia com -25 HP!","Penalidade: Perde 2 vidas!"},
    {{"Exatamente um caminho esta sincronizado com o clock.","Se VERMELHO esta sincronizado, entao AZUL esta travado.","AZUL nao esta travado.","Se VERDE esta sincronizado, entao VERMELHO esta isolado.","VERMELHO nao esta isolado.",NULL},2,"Bonus: +2 vidas extras!","Penalidade: Monstro ganha +20 HP!"},
    {{"Um caminho e seguro somente se NAO esta corrompido","    E NAO esta travado.","O caminho VERDE esta corrompido.","O caminho AZUL esta travado em deadlock.","O caminho VERMELHO nao esta corrompido nem travado.",NULL},0,"Bonus: Monstro inicia com -30 HP!","Penalidade: Perde 1 vida!"},
    {{"Se VERMELHO esta acessivel, entao VERDE esta acessivel.","Se VERDE esta acessivel, entao AZUL esta isolado da rede.","AZUL nao esta isolado.","Pelo menos um caminho esta acessivel no barramento.",NULL,NULL},2,"Bonus: +2 vidas extras!","Penalidade: Perde 2 vidas!"},
    {{"VERMELHO ou VERDE tem firewall ativo (ou ambos).","Se VERMELHO tem firewall, a porta de saida e bloqueada.","A porta de saida NAO esta bloqueada.","Se VERDE tem firewall, o processo entra em loop infinito.","O processo NAO esta em loop infinito.","Se nenhum tem firewall, VERDE e o gateway padrao."},1,"Bonus: Monstro inicia com -20 HP!","Penalidade: Perde 1 vida!"},
    {{"Tres processos disputam um unico bloco de memoria.","VERMELHO alocou o recurso primeiro (mutex adquirido).","O detentor do mutex nao pode ser corrompido.","VERDE e AZUL estao bloqueados aguardando o recurso.","Apenas o detentor do mutex pode ser atravessado.",NULL},0,"Bonus: +2 vidas extras!","Penalidade: Monstro ganha +15 HP!"},
    {{"Exatamente dois caminhos estao com checksum invalido.","O caminho VERMELHO tem checksum invalido.","O caminho VERDE tem checksum invalido.","Apenas o caminho com checksum valido e seguro.",NULL,NULL},2,"Bonus: Monstro inicia com -25 HP!","Penalidade: Perde 2 vidas!"},
    {{"Se AZUL esta online, VERMELHO sofre buffer overflow.","Se VERMELHO sofre overflow, ele trava imediatamente.","VERMELHO nao esta travado.","Se VERDE esta online, AZUL e desativado.","Pelo menos um caminho esta online.",NULL},1,"Bonus: +2 vidas extras!","Penalidade: Perde 1 vida!"},
    {{"VERMELHO ou VERDE esta seguro, mas nao os dois.","Se VERDE esta seguro, entao AZUL esta corrompido.","AZUL nao esta corrompido.","Apenas o caminho seguro pode ser transitado.",NULL,NULL},0,"Bonus: Monstro inicia com -30 HP!","Penalidade: Monstro ganha +20 HP!"},
    {{"Sistema usa round-robin: um caminho ativo por vez.","VERMELHO esgotou seu quantum de CPU e foi bloqueado.","VERDE e AZUL ainda nao receberam quantum.","O escalonador prioriza o processo de menor PID.","VERDE tem PID menor que AZUL.",NULL},1,"Bonus: Monstro inicia com -20 HP!","Penalidade: Perde 2 vidas!"},
};

#define NUM_PATH_EVENTS (sizeof(pathEvents) / sizeof(pathEvents[0]))

static void CalcLetterbox(float *scale, float *offX, float *offY)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float sx = (float)sw / VIRT_W, sy = (float)sh / VIRT_H;
    *scale = (sx < sy) ? sx : sy;
    *offX  = (sw - VIRT_W * (*scale)) * 0.5f;
    *offY  = (sh - VIRT_H * (*scale)) * 0.5f;
}

static Vector2 MouseVirtual(void)
{
    float scale, offX, offY;
    CalcLetterbox(&scale, &offX, &offY);
    Vector2 m = GetMousePosition();
    return (Vector2){ (m.x - offX) / scale, (m.y - offY) / scale };
}

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

static void SalvarHistorico(const char *resultado, int sala, int modoDificil)
{
    FILE *f = fopen(modoDificil ? "../historico_dificil.txt" : "../historico_facil.txt", "a");
    if (!f) return;
    if (strcmp(resultado, "VENCEU") == 0) fprintf(f, "VENCEU\n");
    else fprintf(f, "%s - Sala %d\n", resultado, sala);
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

static void DrawTexVirt(Texture2D t, Rectangle dst, Color tint)
{
    DrawTexturePro(t,
        (Rectangle){0, 0, (float)t.width, (float)t.height},
        dst, (Vector2){0, 0}, 0.0f, tint);
}

static Texture2D* GetEnemyTexture(int sala, int modoDificil,
    Texture2D *slimeTex, Texture2D *ogroTex, Texture2D *bossTex,
    Texture2D *mariTex, Texture2D *romaTex, Texture2D *luisTex,
    Texture2D *micaTex, Texture2D *ruanTex, Texture2D *lucasTex, Texture2D *lucas2Tex)
{
    if (!modoDificil) {
        if (sala == 1 || sala == 2) return slimeTex;
        if (sala == 3 || sala == 4) return ogroTex;
        return bossTex;
    }
    switch (sala) {
        case 1: return mariTex;
        case 2: return romaTex;
        case 3: return luisTex;
        case 4: return micaTex;
        case 5: return ruanTex;
        case 6: return lucasTex;
        default:return lucas2Tex;
    }
}

// =========================================================
//  HELPERS DE LAYOUT  (centralização para 1280x720)
// =========================================================
// Retorna um retangulo centralizado horizontalmente em VIRT_W
static Rectangle CenteredRect(float y, float w, float h)
{
    return (Rectangle){ (VIRT_W - w) * 0.5f, y, w, h };
}

int main(void)
{
    InitWindow(VIRT_W, VIRT_H, "Falta Fatal");
    InitAudioDevice();
    SetTargetFPS(60);
    srand((unsigned)time(NULL));

    RenderTexture2D canvas = LoadRenderTexture(VIRT_W, VIRT_H);

    Texture2D menuBg      = LoadTexture("../assets/telademenu.png");
    Texture2D gameBg      = LoadTexture("../assets/forest.jpg");
    Texture2D circuitBg   = LoadTexture("../assets/circuit.jpg");
    Texture2D btnNovoJogo = LoadTexture("../assets/novojogobotao.png");
    Texture2D btnFacil    = LoadTexture("../assets/facilbotao.png");
    Texture2D btnDificil  = LoadTexture("../assets/dificilbotao.png");
    Texture2D btnStats    = LoadTexture("../assets/estatisticasbotao.png");
    Texture2D btnSair     = LoadTexture("../assets/saairbotao.png");
    Texture2D player1Tex  = LoadTexture("../assets/player1.png");
    Texture2D player2Tex  = LoadTexture("../assets/player2.png");

    Texture2D slimeTex  = LoadTexture("../assets/slime.png");
    Texture2D ogroTex   = LoadTexture("../assets/ogro.png");
    Texture2D bossTex   = LoadTexture("../assets/boss.png");
    Texture2D mariTex   = LoadTexture("../assets/mari.png");
    Texture2D romaTex   = LoadTexture("../assets/roma.png");
    Texture2D luisTex   = LoadTexture("../assets/luis.png");
    Texture2D micaTex   = LoadTexture("../assets/mica.png");
    Texture2D ruanTex   = LoadTexture("../assets/ruan.png");
    Texture2D lucasTex  = LoadTexture("../assets/lucas.png");
    Texture2D lucas2Tex = LoadTexture("../assets/lucas2.png");

    Texture2D lore1 = LoadTexture("../assets/lore1.png");
    Texture2D lore2 = LoadTexture("../assets/lore2.png");
    Texture2D lore3 = LoadTexture("../assets/lore3.png");
    Texture2D lore4 = LoadTexture("../assets/lore4.png");
    Texture2D lore5 = LoadTexture("../assets/lore5.png");

    Music menuMusic = LoadMusicStream("../assets/menu.wav");
    Music gameMusic = LoadMusicStream("../assets/soundtrack.wav");
    Sound openSound  = LoadSound("../assets/open.wav");
    Sound closeSound = LoadSound("../assets/close.wav");

    PlayMusicStream(menuMusic);

    GameState state = MENU;

    int MNumber = 0, rodada = 0, minN = 1, maxN = 100, novaRodada = 0;
    int monsterHP = 50, vidas = 5, qntOpcoes = 6, sala = 1;
    int resultadoSalvo = 0, modoDificil = 0;
    int opcoesP1[10], opcoesP2[10];
    int idxEscolhaP1 = -1, idxEscolhaP2 = -1;
    int escolhaPathP1 = -1, escolhaPathP2 = -1;

    char mensagem[200]        = "Escolham um numero";
    char mensagemMonstro[200] = "";

    int pathEventIndex  = 0;
    int bonusMonsterHP  = 0, bonusVidas = 0, penalVidas = 0, penalMonsterHP = 0;
    char pathResultMsg[200] = "";
    // =======================
    // LORE
    // =======================
    int loreScene = 0;
    int loreCharIndex = 0;
    int loreTimer = 0;

    const char *loreTexts[] = {
        "Voce usou todas as faltas da cadeira de sexta-feira.",
        "No ultimo dia, o professor marcou falta por engano.",
        "Depois de discutir no Slack sem sucesso...",
        "Voce decidiu invadir o sistema do Lyceum.",
        "A chuva deixou o CESAR vazio...\nEssa era a chance perfeita."
    };

    Texture2D *loreTextures[] = {
        &lore1,
        &lore2,
        &lore3,
        &lore4,
        &lore5
    };
    int pathEscolhaP1 = -1, pathEscolhaP2 = -1;
    int pathResultTimer = 0;
    // Timer da tela de caminho (frames a 60fps): facil=25s=1500 | dificil=15s=900
    int pathTimeLeft = 0;
    // Alterna se a sala atual exibe PATH_CHOICE (sim/nao/sim/nao...)
    int salaComPath = 1;

    char historicoFacil[50][100];
    char historicoDificil[50][100];
    int  totalFacil = 0, totalDificil = 0;

    // =========================================================
    //  LAYOUT CONSTANTS  (ajustados para 1280x720)
    // =========================================================

    // MENU — 3 botões centralizados, 300x90 cada
    const float BTN_W = 340, BTN_H = 90;
    const Rectangle btnJogarRec = { (VIRT_W - BTN_W)*0.5f, 190, BTN_W, BTN_H };
    const Rectangle btnStatsRec = { (VIRT_W - BTN_W)*0.5f, 320, BTN_W, BTN_H };
    const Rectangle btnSairRec  = { (VIRT_W - BTN_W)*0.5f, 450, BTN_W, BTN_H };

    // MODE SELECT
    const Rectangle btnFacilRec   = { (VIRT_W - BTN_W)*0.5f, 250, BTN_W, BTN_H };
    const Rectangle btnDificilRec = { (VIRT_W - BTN_W)*0.5f, 390, BTN_W, BTN_H };

    // GAMEPLAY — botões dos jogadores (5 botões, 110px wide, gap 20)
    // Total: 5*110 + 4*20 = 630. Start X = (1280-630)/2 = 325
    const int   BTN_NUM_W    = 110;
    const int   BTN_NUM_H    = 55;
    const int   BTN_NUM_GAP  = 20;
    const float BTN_NUM_STARTX = (VIRT_W - (5*BTN_NUM_W + 4*BTN_NUM_GAP)) * 0.5f;
    const float BTN_P1_Y    = 520;
    const float BTN_P2_Y    = 610;

    // SPRITES — jogadores ficam à esquerda, inimigo à direita, bem proporcionais
    // Player 1: canto esq-baixo
    const Rectangle spr_p1 = { 40,  390, 180, 260 };
    // Player 2: ao lado de P1
    const Rectangle spr_p2 = { 240, 390, 180, 260 };
    // Inimigo: lado direito, maior
    const Rectangle spr_enemy = { 860, 130, 380, 430 };

    // CHEST
    const Rectangle chestSim = { (VIRT_W*0.5f) - 160, 340, 130, 55 };
    const Rectangle chestNao = { (VIRT_W*0.5f) +  30, 340, 130, 55 };

    // GAME OVER
    const Rectangle goRec = { (VIRT_W - 260)*0.5f, 380, 260, 55 };
    const Rectangle goExi = { (VIRT_W - 260)*0.5f, 460, 260, 55 };

    // PATH CHOICE — 3 painéis igualmente espaçados
    const Rectangle pathRed   = { 180, 310, 270, 220 };
    const Rectangle pathGreen = { 500, 310, 270, 220 };
    const Rectangle pathBlue  = { 820, 310, 270, 220 };

    // =========================================================
    //  GAMEPLAY — layout 3 colunas  (1280 x 720)
    //
    //  Col esq  [  0.. 310]: botões P1 + sprite P1
    //  Col mid  [310.. 970]: sprites (P1, inimigo, P2) + mensagens
    //  Col dir  [970..1280]: botões P2 + sprite P2
    //
    //  Botões: grid 3x2 por jogador
    //    BTN6_W=80  BTN6_H=52  BTN6_GAP=8
    //    Bloco total: 3*80+2*8 = 256 wide, 2*52+8 = 112 tall
    // =========================================================
    const int BTN6_W   = 80;
    const int BTN6_H   = 52;
    const int BTN6_GAP = 8;
    // Coluna esquerda: bloco de botões centralizado em x=[10..300]
    const float P1_BTN_X     = 27;          // margem esq da grade P1
    const float P1_BTN_TOP_Y = 340;         // linha superior da grade P1
    // Coluna direita: bloco de botões alinhado à direita em x=[970..1270]
    const float P2_BTN_X     = VIRT_W - 27 - (3*BTN6_W + 2*BTN6_GAP);
    const float P2_BTN_TOP_Y = 340;         // mesma altura que P1
    // Sprites centrais
    // P1 e P2 ocupam faixa vertical 95-720, centralizado no terço do meio
    // Inimigo: centro-dir do palco
    const Rectangle spr2_p1    = { 330,  230, 160, 310 };
    const Rectangle spr2_p2    = { 530,  230, 160, 310 };
    const Rectangle spr2_enemy = { 790,  130, 380, 440 };

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_F11)) {
            if (IsWindowFullscreen()) { ToggleFullscreen(); SetWindowSize(VIRT_W, VIRT_H); }
            else { int mon = GetCurrentMonitor(); SetWindowSize(GetMonitorWidth(mon), GetMonitorHeight(mon)); ToggleFullscreen(); }
        }

        if (state == MENU || state == STATS || state == MODE_SELECT)
            UpdateMusicStream(menuMusic);
        else
            UpdateMusicStream(gameMusic);

        Vector2 mouse = MouseVirtual();

        // =============  LÓGICA  =============

        if (state == MENU)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mouse, btnJogarRec)) state = MODE_SELECT;
                if (CheckCollisionPointRec(mouse, btnStatsRec)) {
                    totalFacil   = LerHistorico("../historico_facil.txt",   historicoFacil,   50);
                    totalDificil = LerHistorico("../historico_dificil.txt", historicoDificil, 50);
                    state = STATS;
                }
                if (CheckCollisionPointRec(mouse, btnSairRec)) break;
            }
        }
        else if (state == MODE_SELECT)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                bool hF = CheckCollisionPointRec(mouse, btnFacilRec);
                bool hD = CheckCollisionPointRec(mouse, btnDificilRec);
                if (hF || hD) {
                    modoDificil = hD ? 1 : 0;
                    StopMusicStream(menuMusic);
                    PlayMusicStream(gameMusic);
                    MNumber = (rand() % 100) + 1;
                    printf("DEBUG: %d\n", MNumber);
                    rodada = 0; minN = 1; maxN = 100; novaRodada = 1;
                    monsterHP = 50; vidas = 5; qntOpcoes = 6; sala = 1;
                    resultadoSalvo = 0; bonusMonsterHP = 0; bonusVidas = 0;
                    penalVidas = 0; penalMonsterHP = 0;
                    idxEscolhaP1 = -1; idxEscolhaP2 = -1;
                    salaComPath = 1; pathTimeLeft = 0;
                    mensagemMonstro[0] = '\0';
                    snprintf(mensagem, 200, "Escolham um numero");
                    loreScene = 0;
                    loreCharIndex = 0;
                    loreTimer = 0;
                    state = LORE;
                }
            }
        }
        else if (state == LORE)
        {
            loreTimer++;

            // efeito digitando
            if (loreTimer % 2 == 0) {
                int tamanho = strlen(loreTexts[loreScene]);
                if (loreCharIndex < tamanho)
                    loreCharIndex++;
            }

            // avançar cena
            if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

                int tamanho = strlen(loreTexts[loreScene]);

                // termina texto instantaneamente
                if (loreCharIndex < tamanho) {
                    loreCharIndex = tamanho;
                }
                else {
                    loreScene++;

                    if (loreScene >= 5) {
                        state = GAMEPLAY;
                    }
                    else {
                        loreCharIndex = 0;
                        loreTimer = 0;
                    }
                }
            }
        }
        
        else if (state == STATS)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) state = MENU;
        }
        else if (state == CHEST)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                bool hSim = CheckCollisionPointRec(mouse, chestSim);
                bool hNao = CheckCollisionPointRec(mouse, chestNao);
                if (hSim || hNao) {
                    if (hSim) PlaySound(openSound);
                    if (hNao) PlaySound(closeSound);
                    if ((!modoDificil && sala >= 6) || (modoDificil && sala >= 8)) state = WIN;
                    else {
                        salaComPath = !salaComPath;
                        if (salaComPath) {
                            pathEventIndex  = rand() % NUM_PATH_EVENTS;
                            printf("DEBUG PATH EVENT: %d | Caminho correto: %d\n", pathEventIndex, pathEvents[pathEventIndex].caminhoCerto);
                            pathResultTimer = 0; pathResultMsg[0] = '\0';
                            pathEscolhaP1 = -1; pathEscolhaP2 = -1;
                            pathTimeLeft = modoDificil ? 900 : 1500;
                            state = PATH_CHOICE;
                        } else {
                            rodada = 0; minN = 1; maxN = 100;
                            MNumber = (rand() % 100) + 1;
                            printf("DEBUG: %d\n", MNumber);
                            monsterHP = 50 + (sala * 25);
                            if (monsterHP < 10) monsterHP = 10;
                            qntOpcoes = 6; novaRodada = 1;
                            idxEscolhaP1 = -1; idxEscolhaP2 = -1;
                            mensagemMonstro[0] = '\0';
                            snprintf(mensagem, 200, "Escolham um numero");
                            state = GAMEPLAY;
                        }
                    }
                }
            }
        }
        else if (state == PATH_CHOICE)
        {
            // Helper: avança para o próximo combate após resolução
            #define ENTRAR_GAMEPLAY() do { \
                vidas += bonusVidas; vidas -= penalVidas; \
                if (vidas < 1) vidas = 1; \
                rodada = 0; minN = 1; maxN = 100; \
                MNumber = (rand() % 100) + 1; \
                printf("DEBUG: %d\n", MNumber); \
                monsterHP = 50 + (sala * 25) - bonusMonsterHP + penalMonsterHP; \
                if (monsterHP < 10) monsterHP = 10; \
                bonusMonsterHP = 0; bonusVidas = 0; penalVidas = 0; penalMonsterHP = 0; \
                qntOpcoes = 6; novaRodada = 1; \
                idxEscolhaP1 = -1; idxEscolhaP2 = -1; \
                pathEscolhaP1 = -1; pathEscolhaP2 = -1; \
                mensagemMonstro[0] = '\0'; \
                snprintf(mensagem, 200, "Escolham um numero"); \
                state = GAMEPLAY; \
            } while(0)

            if (pathResultTimer > 0) {
                pathResultTimer--;
                if (pathResultTimer == 0) { ENTRAR_GAMEPLAY(); }
            } else {
                // Decrementa timer principal
                if (pathTimeLeft > 0) pathTimeLeft--;

                // Timeout: penalidade automática
                if (pathTimeLeft == 0) {
                    PathEvent *ev2 = &pathEvents[pathEventIndex];
                    snprintf(pathResultMsg, 200, "TEMPO ESGOTADO! %s", ev2->penalDesc);
                    if (strstr(ev2->penalDesc, "vida")) { int n=1; sscanf(ev2->penalDesc,"Penalidade: Perde %d vida",&n); penalVidas=n; penalMonsterHP=0; }
                    else { int hp=0; sscanf(ev2->penalDesc,"Penalidade: Monstro ganha +%d HP!",&hp); penalMonsterHP=hp; penalVidas=0; }
                    bonusMonsterHP=0; bonusVidas=0;
                    pathResultTimer = 180;
                    pathTimeLeft = -1; // marca como expirado
                }

                PathEvent *ev = &pathEvents[pathEventIndex];
                if (IsKeyPressed(KEY_ONE))   pathEscolhaP1 = 0;
                if (IsKeyPressed(KEY_TWO))   pathEscolhaP1 = 1;
                if (IsKeyPressed(KEY_THREE)) pathEscolhaP1 = 2;
                if (IsKeyPressed(KEY_Q))     pathEscolhaP2 = 0;
                if (IsKeyPressed(KEY_W))     pathEscolhaP2 = 1;
                if (IsKeyPressed(KEY_E))     pathEscolhaP2 = 2;
                if (pathEscolhaP1 != -1 && pathEscolhaP2 != -1 && pathEscolhaP1 == pathEscolhaP2 && pathTimeLeft > 0) {
                    int escolhido = pathEscolhaP1;
                    pathTimeLeft = -1; // para o timer
                    if (escolhido == ev->caminhoCerto) {
                        snprintf(pathResultMsg, 200, "CAMINHO CERTO! %s", ev->bonusDesc);
                        if (strstr(ev->bonusDesc, "vida")) { int n=2; sscanf(ev->bonusDesc,"Bonus: +%d vida",&n); bonusVidas=n; bonusMonsterHP=0; }
                        else { int hp=0; sscanf(ev->bonusDesc,"Bonus: Monstro inicia com -%d HP!",&hp); bonusMonsterHP=hp; bonusVidas=0; }
                        penalVidas=0; penalMonsterHP=0;
                    } else {
                        snprintf(pathResultMsg, 200, "CAMINHO ERRADO! %s", ev->penalDesc);
                        if (strstr(ev->penalDesc, "vida")) { int n=1; sscanf(ev->penalDesc,"Penalidade: Perde %d vida",&n); penalVidas=n; penalMonsterHP=0; }
                        else { int hp=0; sscanf(ev->penalDesc,"Penalidade: Monstro ganha +%d HP!",&hp); penalMonsterHP=hp; penalVidas=0; }
                        bonusMonsterHP=0; bonusVidas=0;
                    }
                    pathResultTimer = 180;
                }
            }
            #undef ENTRAR_GAMEPLAY
        }
        else if (state == GAMEPLAY)
        {
            if (novaRodada) {
                rodada++;
                for (int i = 0; i < qntOpcoes; i++) {
                    opcoesP1[i] = (rand() % (maxN - minN + 1)) + minN;
                    opcoesP2[i] = (rand() % (maxN - minN + 1)) + minN;
                }
                idxEscolhaP1 = -1; idxEscolhaP2 = -1; novaRodada = 0;
            }
            // P1: 1 2 3 4 5 6   P2: Q W E R T Y
            // Botoes P1: col esq, 2 linhas de 3
            //   btn 0,1,2 = linha de cima  |  btn 3,4,5 = linha de baixo
            int teclasP1[6] = { KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX };
            for (int i = 0; i < qntOpcoes; i++) {
                int col = i % 3, row = i / 3;
                Rectangle btn = {
                    P1_BTN_X + col * (BTN6_W + BTN6_GAP),
                    P1_BTN_TOP_Y + row * (BTN6_H + BTN6_GAP),
                    BTN6_W, BTN6_H
                };
                if (IsKeyPressed(teclasP1[i]) || (CheckCollisionPointRec(mouse, btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)))
                    idxEscolhaP1 = i;
            }
            int teclasP2[6] = { KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y };
            for (int i = 0; i < qntOpcoes; i++) {
                int col = i % 3, row = i / 3;
                Rectangle btn = {
                    P2_BTN_X + col * (BTN6_W + BTN6_GAP),
                    P2_BTN_TOP_Y + row * (BTN6_H + BTN6_GAP),
                    BTN6_W, BTN6_H
                };
                if (IsKeyPressed(teclasP2[i]) || (CheckCollisionPointRec(mouse, btn) && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)))
                    idxEscolhaP2 = i;
            }
            if (idxEscolhaP1 != -1 && idxEscolhaP2 != -1) {
                int escolhaP1 = opcoesP1[idxEscolhaP1], escolhaP2 = opcoesP2[idxEscolhaP2];
                int danoP1 = calcularDano(escolhaP1, MNumber), danoP2 = calcularDano(escolhaP2, MNumber);
                int danoTotal = danoP1 + danoP2;
                monsterHP -= danoTotal;
                if (monsterHP < 0) monsterHP = 0;
                mensagemMonstro[0] = '\0';
                char dica[100] = "";
                if (!modoDificil) {
                    int media = (escolhaP1 + escolhaP2) / 2;
                    if (media < MNumber) { minN = media+1; snprintf(dica,100,"Numero maior que %d",media); }
                    else if (media > MNumber) { maxN = media-1; snprintf(dica,100,"Numero menor que %d",media); }
                }
                if (modoDificil) snprintf(mensagem,200,"P1:%d dano | P2:%d dano | Total:%d",danoP1,danoP2,danoTotal);
                else snprintf(mensagem,200,"P1:%d | P2:%d | Total:%d | %s",danoP1,danoP2,danoTotal,dica);
                if (monsterHP <= 0) { sala++; state = ((!modoDificil && sala>=6)||(modoDificil && sala>=8)) ? WIN : CHEST; }
                novaRodada = 1;
            }
            if (rodada % 5 == 0 && novaRodada == 0) {
                vidas--; rodada++;
                snprintf(mensagemMonstro, 200, ">>> O monstro atacou! -1 vida! <<<");
            }
            if (vidas <= 0) {
                if (!resultadoSalvo) { SalvarHistorico("MORREU", sala, modoDificil); resultadoSalvo = 1; }
                state = GAMEOVER;
            }
        }
        else if (state == GAMEOVER)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mouse, goRec)) state = MENU;
                if (CheckCollisionPointRec(mouse, goExi)) break;
            }
        }
        else if (state == WIN)
        {
            if (!resultadoSalvo) { SalvarHistorico("VENCEU", sala, modoDificil); resultadoSalvo = 1; }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) state = MENU;
        }

        // =============  DRAW  =============
        BeginTextureMode(canvas);
        ClearBackground(BLACK);

        if (state == MENU)
        {
            bool hJ = CheckCollisionPointRec(mouse, btnJogarRec);
            bool hS = CheckCollisionPointRec(mouse, btnStatsRec);
            bool hX = CheckCollisionPointRec(mouse, btnSairRec);

            DrawTexVirt(menuBg,     (Rectangle){0,0,VIRT_W,VIRT_H}, WHITE);
            DrawTexVirt(btnNovoJogo, btnJogarRec, hJ ? LIGHTGRAY : WHITE);
            DrawTexVirt(btnStats,    btnStatsRec, hS ? LIGHTGRAY : WHITE);
            DrawTexVirt(btnSair,     btnSairRec,  hX ? LIGHTGRAY : WHITE);
        }
        else if (state == MODE_SELECT)
        {
            bool hF = CheckCollisionPointRec(mouse, btnFacilRec);
            bool hD = CheckCollisionPointRec(mouse, btnDificilRec);

            DrawTexVirt(menuBg, (Rectangle){0,0,VIRT_W,VIRT_H}, WHITE);

            // Título centralizado
            const char *titulo = "ESCOLHA O MODO";
            DrawText(titulo, (VIRT_W - MeasureText(titulo, 46)) / 2, 155, 46, BLACK);

            DrawTexVirt(btnFacil,   btnFacilRec,   hF ? LIGHTGRAY : WHITE);
            DrawTexVirt(btnDificil, btnDificilRec, hD ? LIGHTGRAY : WHITE);
        }
        else if (state == LORE)
        {
            Texture2D *bg = loreTextures[loreScene];

            // fundo da cena
            DrawTexturePro(
                *bg,
                (Rectangle){0, 0, (float)bg->width, (float)bg->height},
                (Rectangle){0, 0, VIRT_W, VIRT_H},
                (Vector2){0, 0},
                0,
                WHITE
            );

            // sombra inferior
            DrawRectangle(
                0,
                VIRT_H - 170,
                VIRT_W,
                170,
                (Color){0,0,0,180}
            );

            // texto aparecendo aos poucos
            char visibleText[512] = {0};

            strncpy(
                visibleText,
                loreTexts[loreScene],
                loreCharIndex
            );

            visibleText[loreCharIndex] = '\0';

            // sombra do texto
            DrawText(
                visibleText,
                63,
                VIRT_H - 127,
                30,
                BLACK
            );

            // texto principal branco
            DrawText(
                visibleText,
                60,
                VIRT_H - 130,
                30,
                WHITE
            );

            DrawText(
                "ENTER ou clique para continuar",
                VIRT_W - 420,
                VIRT_H - 40,
                22,
                LIGHTGRAY
            );
        }
        else if (state == STATS)
        {
            DrawTexVirt(menuBg, (Rectangle){0,0,VIRT_W,VIRT_H}, WHITE);
            DrawRectangle(0,0,VIRT_W,VIRT_H,(Color){0,0,0,130});

            // Lore (canto superior esq)
            int loreY = 30;
            for (int i = 0; introLore[i] != NULL; i++) {
                DrawText(introLore[i], 40, loreY, 20, WHITE);
                loreY += 28;
            }

            // Cabeçalho
            const char *titulo = "ESTATISTICAS";
            DrawText(titulo, (VIRT_W - MeasureText(titulo, 40)) / 2, 30, 40, WHITE);

            // Divisor vertical no centro
            float cx = VIRT_W * 0.5f;
            DrawLine((int)cx, 90, (int)cx, 680, WHITE);

            DrawText("FACIL",   (int)(cx - 200 - MeasureText("FACIL",30)),  100, 30, DARKGREEN);
            DrawText("DIFICIL", (int)(cx + 60), 100, 30, MAROON);

            for (int i = 0; i < totalFacil;   i++) DrawText(historicoFacil[i],   (int)(cx-380), 155+(i*24), 20, DARKGREEN);
            for (int i = 0; i < totalDificil; i++) DrawText(historicoDificil[i], (int)(cx+ 40), 155+(i*24), 20, MAROON);

            const char *voltar = "Clique para voltar";
            DrawText(voltar, (VIRT_W - MeasureText(voltar,22))/2, VIRT_H-45, 22, LIGHTGRAY);
        }
        else if (state == CHEST)
        {
            bool hSim = CheckCollisionPointRec(mouse, chestSim);
            bool hNao = CheckCollisionPointRec(mouse, chestNao);

            ClearBackground(RAYWHITE);

            const char *t1 = "VOCE DERROTOU O MONSTRO!";
            const char *t2 = "Abrir seu bau de tesouros?";
            DrawText(t1, (VIRT_W - MeasureText(t1, 36))/2, 210, 36, BLACK);
            DrawText(t2, (VIRT_W - MeasureText(t2, 26))/2, 270, 26, DARKGRAY);

            DrawRectangleRec(chestSim, hSim ? GREEN   : DARKGRAY);
            DrawRectangleRec(chestNao, hNao ? RED     : DARKGRAY);

            DrawText("SIM", (int)(chestSim.x + (chestSim.width  - MeasureText("SIM",22))*0.5f), (int)(chestSim.y+17), 22, WHITE);
            DrawText("NAO", (int)(chestNao.x + (chestNao.width  - MeasureText("NAO",22))*0.5f), (int)(chestNao.y+17), 22, WHITE);
        }
        else if (state == PATH_CHOICE)
        {
            PathEvent *ev = &pathEvents[pathEventIndex];
            bool canClick = (pathResultTimer == 0);
            bool hR = canClick && CheckCollisionPointRec(mouse, pathRed);
            bool hG = canClick && CheckCollisionPointRec(mouse, pathGreen);
            bool hB = canClick && CheckCollisionPointRec(mouse, pathBlue);

            // Fundo
            DrawTexturePro(circuitBg,
                (Rectangle){0,0,(float)circuitBg.width,(float)circuitBg.height},
                (Rectangle){0,0,VIRT_W,VIRT_H},(Vector2){0,0},0,WHITE);
            DrawRectangle(0,0,VIRT_W,VIRT_H,(Color){0,0,0,130});

            // Título centralizado
            const char *titulo = ">> ENCRUZILHADA DO SISTEMA <<";
            DrawText(titulo, (VIRT_W - MeasureText(titulo,30))/2, 12, 30, (Color){0,220,80,255});
            DrawLine(60,56,VIRT_W-60,56,(Color){0,180,60,120});

            // Painel de enigma (quase a largura toda, altura fixa)
            float panelX = 60, panelW = VIRT_W - 120;
            DrawRectangle((int)panelX, 64, (int)panelW, 210, (Color){0,0,0,190});
            DrawRectangleLinesEx((Rectangle){panelX,64,panelW,210}, 1, (Color){0,180,60,160});
            DrawText("[KERNEL LOG] Analisando rotas de acesso...", (int)panelX+14, 72, 17, (Color){0,160,50,255});
            DrawLine((int)panelX+1, 94, (int)(panelX+panelW-1), 94, (Color){0,100,30,200});

            int ly = 102;
            for (int li = 0; li < MAX_LINHAS_ENIGMA; li++) {
                if (ev->linhas[li] == NULL) break;
                Color lc = (li == 0) ? (Color){240,220,60,255} : (Color){200,220,200,255};
                DrawText(ev->linhas[li], (int)panelX+14, ly, 19, lc);
                ly += 26;
            }
            DrawLine(60,278,VIRT_W-60,278,(Color){0,180,60,120});

            const char *instrucao = "[ Analise os logs e escolha o caminho seguro ]";
            DrawText(instrucao, (VIRT_W - MeasureText(instrucao,16))/2, 284, 16, (Color){0,140,50,200});

            // --- Timer visual ---
            {
                int totalFrames = modoDificil ? 900 : 1500;
                int framesLeft  = (pathTimeLeft > 0) ? pathTimeLeft : 0;
                int segundos    = (framesLeft + 59) / 60;
                float progTimer = (totalFrames > 0) ? (float)framesLeft / totalFrames : 0.0f;
                Color timerCol  = (segundos <= 5) ? RED : (segundos <= 10 ? ORANGE : (Color){0,220,80,255});

                char timeBuf[32];
                snprintf(timeBuf, 32, "%02d s", segundos);
                // Barra de tempo
                DrawRectangle(60, 286, VIRT_W-120, 10, (Color){40,40,40,200});
                DrawRectangle(60, 286, (int)((VIRT_W-120)*progTimer), 10, timerCol);
                // Texto do tempo (canto dir, alinhado com barra)
                DrawText(timeBuf, VIRT_W - MeasureText(timeBuf,18) - 65, 280, 18, timerCol);
            }

            // --- Escolhas dos jogadores (modo fácil apenas) ---
            if (!modoDificil) {
                const char *nomes[] = {"VERMELHO","VERDE","AZUL"};
                // P1 à esquerda, acima dos painéis
                if (pathEscolhaP1 != -1) {
                    char buf[64]; snprintf(buf, 64, "P1: %s", nomes[pathEscolhaP1]);
                    DrawRectangle(30, 306, MeasureText(buf,18)+16, 26, (Color){0,0,0,180});
                    DrawText(buf, 38, 310, 18, (Color){100,210,255,255});
                } else {
                    DrawRectangle(30, 306, 130, 26, (Color){0,0,0,100});
                    DrawText("P1: ???", 38, 310, 18, (Color){120,120,120,200});
                }
                // P2 à direita, acima dos painéis
                if (pathEscolhaP2 != -1) {
                    char buf[64]; snprintf(buf, 64, "P2: %s", nomes[pathEscolhaP2]);
                    int bw = MeasureText(buf,18)+16;
                    DrawRectangle(VIRT_W-30-bw, 306, bw, 26, (Color){0,0,0,180});
                    DrawText(buf, VIRT_W-30-bw+8, 310, 18, (Color){255,150,150,255});
                } else {
                    int bw = MeasureText("P2: ???",18)+16;
                    DrawRectangle(VIRT_W-30-bw, 306, bw, 26, (Color){0,0,0,100});
                    DrawText("P2: ???", VIRT_W-30-bw+8, 310, 18, (Color){120,120,120,200});
                }
            }

            // Painel VERMELHO
            {
                Color bg = hR ? (Color){255,110,110,255} : (Color){160,20,20,255};
                DrawRectangleRec(pathRed, bg);
                DrawRectangleLinesEx(pathRed, hR?4:2, hR?WHITE:(Color){255,120,120,255});
                DrawText("[PROC: 0x52]",
                    (int)(pathRed.x + (pathRed.width - MeasureText("[PROC: 0x52]",14))*0.5f),
                    (int)(pathRed.y+12), 14, (Color){255,180,180,200});
                const char *lbl = "VERMELHO";
                DrawText(lbl,
                    (int)(pathRed.x + (pathRed.width - MeasureText(lbl,22))*0.5f),
                    (int)(pathRed.y + pathRed.height*0.5f - 10), 22, WHITE);
                if (pathEscolhaP1 == 0 || pathEscolhaP2 == 0)
                    DrawText("[SELECIONADO]",
                        (int)(pathRed.x+(pathRed.width-MeasureText("[SELECIONADO]",15))*0.5f),
                        (int)(pathRed.y+pathRed.height-28), 15, (Color){255,220,220,255});
            }
            // Painel VERDE
            {
                Color bg = hG ? (Color){80,255,100,255} : (Color){10,130,40,255};
                DrawRectangleRec(pathGreen, bg);
                DrawRectangleLinesEx(pathGreen, hG?4:2, hG?WHITE:(Color){80,200,100,255});
                DrawText("[PROC: 0x47]",
                    (int)(pathGreen.x + (pathGreen.width - MeasureText("[PROC: 0x47]",14))*0.5f),
                    (int)(pathGreen.y+12), 14, (Color){180,255,190,200});
                const char *lbl = "VERDE";
                DrawText(lbl,
                    (int)(pathGreen.x + (pathGreen.width - MeasureText(lbl,22))*0.5f),
                    (int)(pathGreen.y + pathGreen.height*0.5f - 10), 22, WHITE);
                if (pathEscolhaP1 == 1 || pathEscolhaP2 == 1)
                    DrawText("[SELECIONADO]",
                        (int)(pathGreen.x+(pathGreen.width-MeasureText("[SELECIONADO]",15))*0.5f),
                        (int)(pathGreen.y+pathGreen.height-28), 15, (Color){220,255,220,255});
            }
            // Painel AZUL
            {
                Color bg = hB ? (Color){80,160,255,255} : (Color){15,40,160,255};
                DrawRectangleRec(pathBlue, bg);
                DrawRectangleLinesEx(pathBlue, hB?4:2, hB?WHITE:(Color){80,120,255,255});
                DrawText("[PROC: 0x42]",
                    (int)(pathBlue.x + (pathBlue.width - MeasureText("[PROC: 0x42]",14))*0.5f),
                    (int)(pathBlue.y+12), 14, (Color){160,190,255,200});
                const char *lbl = "AZUL";
                DrawText(lbl,
                    (int)(pathBlue.x + (pathBlue.width - MeasureText(lbl,22))*0.5f),
                    (int)(pathBlue.y + pathBlue.height*0.5f - 10), 22, WHITE);
                if (pathEscolhaP1 == 2 || pathEscolhaP2 == 2)
                    DrawText("[SELECIONADO]",
                        (int)(pathBlue.x+(pathBlue.width-MeasureText("[SELECIONADO]",15))*0.5f),
                        (int)(pathBlue.y+pathBlue.height-28), 15, (Color){200,210,255,255});
            }

            // Barra de resultado
            if (pathResultTimer > 0) {
                DrawRectangle(60, 560, VIRT_W-120, 75, (Color){0,0,0,230});
                DrawRectangleLinesEx((Rectangle){60,560,VIRT_W-120,75}, 2, (Color){0,220,80,255});
                bool acertou = (strstr(pathResultMsg, "CERTO") != NULL);
                char fullMsg[256];
                snprintf(fullMsg,256,"%s %s", acertou?"[OK] ":"[ERR]", pathResultMsg);
                DrawText(fullMsg, (VIRT_W-MeasureText(fullMsg,20))/2, 575, 20,
                    acertou?(Color){80,255,100,255}:(Color){255,80,80,255});
                float prog = (float)pathResultTimer/180.0f;
                DrawRectangle(62, 622, (int)((VIRT_W-124)*prog), 10,
                    acertou?(Color){0,200,60,255}:(Color){200,50,50,255});
            } else {
                const char *hint = "[ Clique ou use teclas para prosseguir ]";
                DrawText(hint, (VIRT_W-MeasureText(hint,15))/2, 648, 15, (Color){0,100,40,180});
            }
        }
        else if (state == GAMEPLAY)
        {
            // ======================================================
            // ORDEM DE DRAW: fundo -> sprites -> HUD (sem overlay geral)
            // Nada tapa os sprites!
            // ======================================================

            // 1) Fundo
            DrawTexVirt(gameBg, (Rectangle){0,0,VIRT_W,VIRT_H}, WHITE);

            // 2) Sprites — desenhados ANTES de qualquer painel HUD
            Texture2D *enemyTex = GetEnemyTexture(sala, modoDificil,
                &slimeTex,&ogroTex,&bossTex,
                &mariTex,&romaTex,&luisTex,&micaTex,&ruanTex,&lucasTex,&lucas2Tex);

            DrawTexturePro(player1Tex,
                (Rectangle){0,0,(float)player1Tex.width,(float)player1Tex.height},
                spr2_p1, (Vector2){0,0}, 0, WHITE);
            DrawTexturePro(player2Tex,
                (Rectangle){0,0,(float)player2Tex.width,(float)player2Tex.height},
                spr2_p2, (Vector2){0,0}, 0, WHITE);
            DrawTexturePro(*enemyTex,
                (Rectangle){0,0,(float)enemyTex->width,(float)enemyTex->height},
                spr2_enemy, (Vector2){0,0}, 0, WHITE);

            // 3) HUD superior (barra preta no topo — cobre só os primeiros 95px)
            DrawRectangle(0, 0, VIRT_W, 95, (Color){0,0,0,210});

            // Missão
            DrawText(TextFormat("MISSAO %d", sala), 20, 10, 32, (Color){0,255,120,255});

            // Nome do boss / descrição
            if (modoDificil) {
                DrawText(bossNames[sala-1], 20, 48, 22, WHITE);
                int descX = 20 + MeasureText(bossNames[sala-1], 22) + 20;
                DrawText(bossDescriptions[sala-1], descX, 52, 16, (Color){190,190,190,255});
            } else {
                const char *ebn;
                if ((!modoDificil&&sala>=6)||(modoDificil&&sala>=8)) ebn="Boss Ogro";
                else if (sala%2==0) ebn="Ogro";
                else ebn="Slime";
                DrawText(ebn, 20, 48, 24, WHITE);
                DrawText("Uma criatura bloqueia o caminho.", 200, 52, 16, LIGHTGRAY);
            }

            // Modo (canto dir do HUD superior)
            const char *modoTxt = modoDificil ? "MODO DIFICIL" : "MODO FACIL";
            DrawText(modoTxt, VIRT_W - MeasureText(modoTxt,18) - 14, 10, 18, modoDificil?RED:(Color){0,220,80,255});

            // Vidas + HP (canto dir, segunda linha do HUD)
            DrawText(TextFormat("VIDAS: %d", vidas),
                VIRT_W - MeasureText(TextFormat("VIDAS: %d",vidas),18) - 14, 36, 18, RED);
            DrawText(TextFormat("HP: %d", monsterHP),
                VIRT_W - MeasureText(TextFormat("HP: %d",monsterHP),18) - 14, 58, 18, GREEN);

            // 4) Mensagem de resultado (faixa estreita, entre HUD e sprites)
            DrawRectangle(300, 100, 680, 118, (Color){0,0,0,170});
            DrawText(mensagem,
                300 + (680 - MeasureText(mensagem,20))/2, 115, 20, SKYBLUE);
            if (mensagemMonstro[0] != '\0')
                DrawText(mensagemMonstro,
                    300 + (680 - MeasureText(mensagemMonstro,18))/2, 150, 18, RED);

            // 5) Colunas laterais (painéis semitransparentes para botões)
            //    Esquerda: x=0..310  |  Direita: x=970..1280
            DrawRectangle(0,   95, 310, VIRT_H-95, (Color){0,0,0,140});
            DrawRectangle(970, 95, 310, VIRT_H-95, (Color){0,0,0,140});

            // Linha divisória suave
            DrawLine(310, 95, 310, VIRT_H, (Color){80,80,80,180});
            DrawLine(970, 95, 970, VIRT_H, (Color){80,80,80,180});

            // ── PLAYER 1 (coluna esquerda) ────────────────────────
            DrawText("PLAYER 1",
                (310 - MeasureText("PLAYER 1",20))/2, 100, 20, (Color){100,180,255,255});
            DrawText("1 2 3 / 4 5 6",
                (310 - MeasureText("1 2 3 / 4 5 6",15))/2, 124, 15, (Color){160,160,160,200});

            // Sprite P1 (pequeno, dentro da coluna esq)
            // Já desenhado acima em spr2_p1 — apenas label abaixo
            DrawText("[ P1 ]",
                (int)(spr2_p1.x + (spr2_p1.width - MeasureText("[ P1 ]",15))*0.5f),
                (int)(spr2_p1.y + spr2_p1.height + 4), 15, (Color){100,180,255,200});

            // Botões P1: grid 3x2 na coluna esquerda
            // Grade: Y topo=340, Y baixo=400+gap
            for (int i = 0; i < qntOpcoes; i++) {
                int col = i % 3, row = i / 3;
                Rectangle btn = {
                    P1_BTN_X + col * (BTN6_W + BTN6_GAP),
                    P1_BTN_TOP_Y + row * (BTN6_H + BTN6_GAP),
                    BTN6_W, BTN6_H
                };
                bool hover    = CheckCollisionPointRec(mouse, btn);
                bool selected = (idxEscolhaP1 == i);
                Color bg = selected?(Color){255,140,0,255}:(hover?(Color){255,200,50,255}:(Color){50,50,80,220});
                Color border = selected?WHITE:(hover?(Color){255,220,100,255}:(Color){100,100,140,200});
                DrawRectangleRec(btn, bg);
                DrawRectangleLinesEx(btn, selected?3:1, border);
                // Tecla label (canto sup esq do botão)
                const char *keyLabel[] = {"1","2","3","4","5","6"};
                DrawText(keyLabel[i], (int)(btn.x+4), (int)(btn.y+3), 11, selected?BLACK:(Color){180,180,180,180});
                // Número
                const char *ns = TextFormat("%d", opcoesP1[i]);
                DrawText(ns,
                    (int)(btn.x + (BTN6_W - MeasureText(ns,20))*0.5f),
                    (int)(btn.y + (BTN6_H - 20)*0.5f), 20,
                    selected?BLACK:WHITE);
            }

            // ── PLAYER 2 (coluna direita) ─────────────────────────
            DrawText("PLAYER 2",
                970 + (310 - MeasureText("PLAYER 2",20))/2, 100, 20, (Color){255,120,120,255});
            DrawText("Q W E / R T Y",
                970 + (310 - MeasureText("Q W E / R T Y",15))/2, 124, 15, (Color){160,160,160,200});

            DrawText("[ P2 ]",
                (int)(spr2_p2.x + (spr2_p2.width - MeasureText("[ P2 ]",15))*0.5f),
                (int)(spr2_p2.y + spr2_p2.height + 4), 15, (Color){255,120,120,200});

            // Botões P2: mesma grade, coluna direita
            for (int i = 0; i < qntOpcoes; i++) {
                int col = i % 3, row = i / 3;
                Rectangle btn = {
                    P2_BTN_X + col * (BTN6_W + BTN6_GAP),
                    P2_BTN_TOP_Y + row * (BTN6_H + BTN6_GAP),
                    BTN6_W, BTN6_H
                };
                bool hover    = CheckCollisionPointRec(mouse, btn);
                bool selected = (idxEscolhaP2 == i);
                Color bg = selected?(Color){220,0,180,255}:(hover?(Color){255,100,200,255}:(Color){80,20,50,220});
                Color border = selected?WHITE:(hover?(Color){255,150,220,255}:(Color){140,60,100,200});
                DrawRectangleRec(btn, bg);
                DrawRectangleLinesEx(btn, selected?3:1, border);
                const char *keyLabel[] = {"Q","W","E","R","T","Y"};
                DrawText(keyLabel[i], (int)(btn.x+4), (int)(btn.y+3), 11, selected?BLACK:(Color){180,180,180,180});
                const char *ns = TextFormat("%d", opcoesP2[i]);
                DrawText(ns,
                    (int)(btn.x + (BTN6_W - MeasureText(ns,20))*0.5f),
                    (int)(btn.y + (BTN6_H - 20)*0.5f), 20,
                    selected?BLACK:WHITE);
            }

            // 6) Alerta de invasão (rodapé, só no centro para não cobrir botões)
            if (rodada >= 10) {
                const char *alerta = "ALERTA: SISTEMA DETECTANDO INVASAO";
                int aw = MeasureText(alerta,16) + 24;
                DrawRectangle((VIRT_W-aw)/2, VIRT_H-46, aw, 34, (Color){120,0,0,230});
                DrawText(alerta, (VIRT_W - MeasureText(alerta,16))/2, VIRT_H-38, 16, RED);
            }
        }
        else if (state == GAMEOVER)
        {
            DrawTexVirt(gameBg, (Rectangle){0,0,VIRT_W,VIRT_H}, WHITE);
            DrawRectangle(0,0,VIRT_W,VIRT_H,(Color){0,0,0,160});

            bool hr = CheckCollisionPointRec(mouse, goRec);
            bool he = CheckCollisionPointRec(mouse, goExi);

            const char *t1 = "O LYCEUM TE DERROTOU";
            const char *t2 = "Lucas bloqueou sua invasao.";
            const char *t3 = "Voce foi reprovado por falta.";
            DrawText(t1, (VIRT_W - MeasureText(t1,44))/2, 190, 44, RED);
            DrawText(t2, (VIRT_W - MeasureText(t2,26))/2, 260, 26, WHITE);
            DrawText(t3, (VIRT_W - MeasureText(t3,26))/2, 298, 26, WHITE);

            DrawRectangleRec(goRec, hr ? ORANGE : DARKGRAY);
            DrawRectangleRec(goExi, he ? ORANGE : DARKGRAY);

            const char *lr = "JOGAR NOVAMENTE";
            const char *le = "SAIR";
            DrawText(lr, (int)(goRec.x+(goRec.width-MeasureText(lr,20))*0.5f), (int)(goRec.y+18), 20, WHITE);
            DrawText(le, (int)(goExi.x+(goExi.width-MeasureText(le,20))*0.5f), (int)(goExi.y+18), 20, WHITE);
        }
        else if (state == WIN)
        {
            DrawTexVirt(gameBg, (Rectangle){0,0,VIRT_W,VIRT_H}, WHITE);
            DrawRectangle(0,0,VIRT_W,VIRT_H,(Color){0,0,0,150});

            const char *t1 = "ACESSO AO LYCEUM CONCEDIDO";
            const char *t2 = "A falta foi removida.";
            const char *t3 = "STATUS: APROVADO";
            const char *t4 = "Lucas observava tudo em silencio...";
            const char *t5 = "Clique para voltar ao menu";
            DrawText(t1, (VIRT_W - MeasureText(t1,44))/2, 190, 44, GREEN);
            DrawText(t2, (VIRT_W - MeasureText(t2,30))/2, 264, 30, WHITE);
            DrawText(t3, (VIRT_W - MeasureText(t3,38))/2, 316, 38, YELLOW);
            DrawText(t4, (VIRT_W - MeasureText(t4,24))/2, 424, 24, LIGHTGRAY);
            DrawText(t5, (VIRT_W - MeasureText(t5,22))/2, 650, 22, GRAY);
        }

        EndTextureMode();

        // Escala letterbox
        float scale, offX, offY;
        CalcLetterbox(&scale, &offX, &offY);
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(
            canvas.texture,
            (Rectangle){0.0f, 0.0f, (float)VIRT_W, -(float)VIRT_H},
            (Rectangle){offX, offY, VIRT_W*scale, VIRT_H*scale},
            (Vector2){0.0f,0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(canvas);
    UnloadTexture(menuBg); UnloadTexture(gameBg); UnloadTexture(circuitBg);
    UnloadTexture(btnNovoJogo); UnloadTexture(btnFacil); UnloadTexture(btnDificil);
    UnloadTexture(btnStats); UnloadTexture(btnSair);
    UnloadTexture(player1Tex); UnloadTexture(player2Tex);
    UnloadTexture(slimeTex); UnloadTexture(ogroTex); UnloadTexture(bossTex);
    UnloadTexture(mariTex); UnloadTexture(romaTex); UnloadTexture(luisTex);
    UnloadTexture(micaTex); UnloadTexture(ruanTex); UnloadTexture(lucasTex);
    UnloadTexture(lucas2Tex);
    UnloadTexture(lore1); UnloadTexture(lore2); UnloadTexture(lore3); UnloadTexture(lore4); UnloadTexture(lore5);
    UnloadMusicStream(menuMusic); UnloadMusicStream(gameMusic);
    UnloadSound(openSound); UnloadSound(closeSound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}