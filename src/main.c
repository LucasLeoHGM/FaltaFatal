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

#define MAX_LORE_LINES 20
#define MAX_FRASES 10
#define MAX_BOSSES 7

char* introLore[MAX_LORE_LINES];
int introLoreCount = 0;

typedef struct {
    char name[100];
    char desc[256];
    char* entry[MAX_FRASES];
    int entryCount;
    char* idle[MAX_FRASES];
    int idleCount;
    char* hit[MAX_FRASES];
    int hitCount;
    char* critical[MAX_FRASES];
    int criticalCount;
    char* death[MAX_FRASES];
    int deathCount;
} BossData;

BossData bosses[MAX_BOSSES];

typedef struct {
    char* entry[MAX_FRASES]; int entryCount;
    char* idle[MAX_FRASES];  int idleCount;
    char* hit[MAX_FRASES];   int hitCount;
    char* critical[MAX_FRASES]; int criticalCount;
    char* death[MAX_FRASES]; int deathCount;
} EasyEnemyData;

EasyEnemyData easySlime;
EasyEnemyData easyOgre;
EasyEnemyData easyBoss;

typedef struct {
    char currentText[256];
    int charIndex;
    int timer;
    bool active;
} SpeechBubble;

SpeechBubble bossBubble = { "", 0, 0, false };

void TrimLine(char *line) {
    int len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' ')) {
        line[len - 1] = '\0';
        len--;
    }
}

void CarregarLore(const char* filename) {
    introLoreCount = 0;
    for (int i = 0; i < MAX_BOSSES; i++) {
        strcpy(bosses[i].name, "");
        strcpy(bosses[i].desc, "");
        bosses[i].entryCount = 0;
        bosses[i].idleCount = 0;
        bosses[i].hitCount = 0;
        bosses[i].criticalCount = 0;
        bosses[i].deathCount = 0;
    }
    easySlime.entryCount = 0; easySlime.idleCount = 0; easySlime.hitCount = 0; easySlime.criticalCount = 0; easySlime.deathCount = 0;
    easyOgre.entryCount = 0;  easyOgre.idleCount = 0;  easyOgre.hitCount = 0;  easyOgre.criticalCount = 0;  easyOgre.deathCount = 0;
    easyBoss.entryCount = 0;  easyBoss.idleCount = 0;  easyBoss.hitCount = 0;  easyBoss.criticalCount = 0;  easyBoss.deathCount = 0;

    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("ERRO: Nao foi possivel abrir o arquivo %s\n", filename);
        return;
    }

    char line[256];
    char currentTag[50] = "";
    
    while (fgets(line, sizeof(line), file)) {
        TrimLine(line);
        if (line[0] == '\0' || line[0] == '#') continue;

        if (line[0] == '[' && line[strlen(line) - 1] == ']') {
            strcpy(currentTag, line);
            continue;
        }

        if (strcmp(currentTag, "[INTRO]") == 0) {
            if (introLoreCount < MAX_LORE_LINES) {
                introLore[introLoreCount++] = strdup(line);
            }
        }
        
        for (int i = 1; i <= MAX_BOSSES; i++) {
            char tagCheck[50];
            snprintf(tagCheck, sizeof(tagCheck), "[BOSS_NAME_%d]", i);
            if (strcmp(currentTag, tagCheck) == 0) { strncpy(bosses[i-1].name, line, 99); bosses[i-1].name[99] = '\0'; }

            snprintf(tagCheck, sizeof(tagCheck), "[BOSS_DESC_%d]", i);
            if (strcmp(currentTag, tagCheck) == 0) { strncpy(bosses[i-1].desc, line, 255); bosses[i-1].desc[255] = '\0'; }

            snprintf(tagCheck, sizeof(tagCheck), "[BOSS_%d_ENTRY]", i);
            if (strcmp(currentTag, tagCheck) == 0 && bosses[i-1].entryCount < MAX_FRASES) bosses[i-1].entry[bosses[i-1].entryCount++] = strdup(line);

            snprintf(tagCheck, sizeof(tagCheck), "[BOSS_%d_IDLE]", i);
            if (strcmp(currentTag, tagCheck) == 0 && bosses[i-1].idleCount < MAX_FRASES) bosses[i-1].idle[bosses[i-1].idleCount++] = strdup(line);

            snprintf(tagCheck, sizeof(tagCheck), "[BOSS_%d_HIT]", i);
            if (strcmp(currentTag, tagCheck) == 0 && bosses[i-1].hitCount < MAX_FRASES) bosses[i-1].hit[bosses[i-1].hitCount++] = strdup(line);

            snprintf(tagCheck, sizeof(tagCheck), "[BOSS_%d_CRITICAL]", i);
            if (strcmp(currentTag, tagCheck) == 0 && bosses[i-1].criticalCount < MAX_FRASES) bosses[i-1].critical[bosses[i-1].criticalCount++] = strdup(line);

            snprintf(tagCheck, sizeof(tagCheck), "[BOSS_%d_DEATH]", i);
            if (strcmp(currentTag, tagCheck) == 0 && bosses[i-1].deathCount < MAX_FRASES) bosses[i-1].death[bosses[i-1].deathCount++] = strdup(line);
        }

        if (strcmp(currentTag, "[EASY_SLIME_ENTRY]") == 0 && easySlime.entryCount < MAX_FRASES) easySlime.entry[easySlime.entryCount++] = strdup(line);
        if (strcmp(currentTag, "[EASY_SLIME_IDLE]") == 0 && easySlime.idleCount < MAX_FRASES) easySlime.idle[easySlime.idleCount++] = strdup(line);
        if (strcmp(currentTag, "[EASY_SLIME_HIT]") == 0 && easySlime.hitCount < MAX_FRASES) easySlime.hit[easySlime.hitCount++] = strdup(line);
        if (strcmp(currentTag, "[EASY_SLIME_CRITICAL]") == 0 && easySlime.criticalCount < MAX_FRASES) easySlime.critical[easySlime.criticalCount++] = strdup(line);
        if (strcmp(currentTag, "[EASY_SLIME_DEATH]") == 0 && easySlime.deathCount < MAX_FRASES) easySlime.death[easySlime.deathCount++] = strdup(line);

        if (strcmp(currentTag, "[EASY_OGRE_ENTRY]") == 0 && easyOgre.entryCount < MAX_FRASES) easyOgre.entry[easyOgre.entryCount++] = strdup(line);
        if (strcmp(currentTag, "[EASY_OGRE_IDLE]") == 0 && easyOgre.idleCount < MAX_FRASES) easyOgre.idle[easyOgre.idleCount++] = strdup(line);
        if (strcmp(currentTag, "[EASY_OGRE_HIT]") == 0 && easyOgre.hitCount < MAX_FRASES) easyOgre.hit[easyOgre.hitCount++] = strdup(line);
        if (strcmp(currentTag, "[EASY_OGRE_CRITICAL]") == 0 && easyOgre.criticalCount < MAX_FRASES) easyOgre.critical[easyOgre.criticalCount++] = strdup(line);
        if (strcmp(currentTag, "[EASY_OGRE_DEATH]") == 0 && easyOgre.deathCount < MAX_FRASES) easyOgre.death[easyOgre.deathCount++] = strdup(line);

        if (strcmp(currentTag, "[EASY_BOSS_ENTRY]") == 0 && easyBoss.entryCount < MAX_FRASES) easyBoss.entry[easyBoss.entryCount++] = strdup(line);
        if (strcmp(currentTag, "[EASY_BOSS_IDLE]") == 0 && easyBoss.idleCount < MAX_FRASES) easyBoss.idle[easyBoss.idleCount++] = strdup(line);
        if (strcmp(currentTag, "[EASY_BOSS_HIT]") == 0 && easyBoss.hitCount < MAX_FRASES) easyBoss.hit[easyBoss.hitCount++] = strdup(line);
        if (strcmp(currentTag, "[EASY_BOSS_CRITICAL]") == 0 && easyBoss.criticalCount < MAX_FRASES) easyBoss.critical[easyBoss.criticalCount++] = strdup(line);
        if (strcmp(currentTag, "[EASY_BOSS_DEATH]") == 0 && easyBoss.deathCount < MAX_FRASES) easyBoss.death[easyBoss.deathCount++] = strdup(line);
    }
    fclose(file);
}

void TriggerSpeech(const char* text) {
    if (text == NULL || strlen(text) == 0) return;
    strncpy(bossBubble.currentText, text, sizeof(bossBubble.currentText) - 1);
    bossBubble.currentText[sizeof(bossBubble.currentText) - 1] = '\0';
    bossBubble.charIndex = 0;
    bossBubble.timer = 0;
    bossBubble.active = true;
}

void AplicarFalaInimigo(int sala, int modoDificil, const char* tipoGatilho) {
    int idx = sala - 1;
    if (idx < 0) idx = 0;

    if (modoDificil) {
        if (idx >= MAX_BOSSES) return;
        if (strcmp(tipoGatilho, "ENTRY") == 0 && bosses[idx].entryCount > 0) TriggerSpeech(bosses[idx].entry[rand() % bosses[idx].entryCount]);
        else if (strcmp(tipoGatilho, "IDLE") == 0 && bosses[idx].idleCount > 0) TriggerSpeech(bosses[idx].idle[rand() % bosses[idx].idleCount]);
        else if (strcmp(tipoGatilho, "HIT") == 0 && bosses[idx].hitCount > 0) TriggerSpeech(bosses[idx].hit[rand() % bosses[idx].hitCount]);
        else if (strcmp(tipoGatilho, "CRITICAL") == 0 && bosses[idx].criticalCount > 0) TriggerSpeech(bosses[idx].critical[rand() % bosses[idx].criticalCount]);
        else if (strcmp(tipoGatilho, "DEATH") == 0 && bosses[idx].deathCount > 0) TriggerSpeech(bosses[idx].death[rand() % bosses[idx].deathCount]);
    } else {
        EasyEnemyData* e = (sala == 1 || sala == 2) ? &easySlime : ((sala == 3 || sala == 4) ? &easyOgre : &easyBoss);
        const char* nomePadrao = (sala == 1 || sala == 2) ? "Slime" : ((sala == 3 || sala == 4) ? "Ogro" : "Rei Ogro");

        if (strcmp(tipoGatilho, "ENTRY") == 0) {
            if (e->entryCount > 0) TriggerSpeech(e->entry[rand() % e->entryCount]);
            else TriggerSpeech(TextFormat("%s apareceu!", nomePadrao));
        }
        else if (strcmp(tipoGatilho, "IDLE") == 0) {
            if (e->idleCount > 0) TriggerSpeech(e->idle[rand() % e->idleCount]);
            else TriggerSpeech(TextFormat("%s esta te observando...", nomePadrao));
        }
        else if (strcmp(tipoGatilho, "HIT") == 0) {
            if (e->hitCount > 0) TriggerSpeech(e->hit[rand() % e->hitCount]);
            else TriggerSpeech(TextFormat("%s grunhiu de dor!", nomePadrao));
        }
        else if (strcmp(tipoGatilho, "CRITICAL") == 0) {
            if (e->criticalCount > 0) TriggerSpeech(e->critical[rand() % e->criticalCount]);
            else TriggerSpeech(TextFormat("%s sofreu um golpe critico!", nomePadrao));
        }
        else if (strcmp(tipoGatilho, "DEATH") == 0) {
            if (e->deathCount > 0) TriggerSpeech(e->death[rand() % e->deathCount]);
            else TriggerSpeech(TextFormat("%s desintegrou!", nomePadrao));
        }
    }
}

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
    {{"Um caminho e seguro somente se NAO esta corrompido","    E NAO esta travado.","O caminho VERDE esta corrompido.","O caminho AZUL esta travado in deadlock.","O caminho VERMELHO nao esta corrompido nem travado.",NULL},0,"Bonus: Monstro inicia com -30 HP!","Penalidade: Perde 1 vida!"},
    {{"Se VERMELHO esta acessivel, entao VERDE esta acessivel.","Se VERDE esta acessivel, entao AZUL esta isolado da rede.","AZUL nao esta isolado.","Pelo menos um caminho esta acessivel no barramento.",NULL,NULL},2,"Bonus: +2 vidas extras!","Penalidade: Perde 2 vidas!"},
    {{"VERMELHO ou VERDE tem firewall ativo (ou ambos).","Se VERMELHO tem firewall, a porta de saida e bloqueada.","A porta de saida NAO esta bloqueada.","Se VERDE tem firewall, o processo entra in loop infinito.","O processo NAO esta in loop infinito.","Se nenhum tem firewall, VERDE e o gateway padrao."},1,"Bonus: Monstro inicia com -20 HP!","Penalidade: Perde 1 vida!"},
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
    FILE *f = fopen(modoDificil ? "historico_dificil.txt" : "historico_facil.txt", "a");
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

    CarregarLore("../assets/lore.txt");

    RenderTexture2D canvas = LoadRenderTexture(VIRT_W, VIRT_H);

    Texture2D menuBg      = LoadTexture("../assets/telademenu.png");
    Texture2D gameBg      = LoadTexture("../assets/gamebg.png");
    Texture2D circuitBg   = LoadTexture("../assets/circuit.png");
    Texture2D btnNovoJogo = LoadTexture("../assets/novojogobotao.png");
    Texture2D btnFacil    = LoadTexture("../assets/facilbotao.png");
    Texture2D btnDificil  = LoadTexture("../assets/dificilbotao.png");
    Texture2D btnStats    = LoadTexture("../assets/estatisticasbotao.png");
    Texture2D btnSair     = LoadTexture("../assets/sairbotao.png");

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
    Texture2D dropBg = LoadTexture("../assets/drop.png");
    
    Texture2D textboxTex = LoadTexture("../assets/textbox.png");

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
    
    int loreScene = 0;
    int loreCharIndex = 0;
    int loreTimer = 0;

    Texture2D *loreTextures[] = { &lore1, &lore2, &lore3, &lore4, &lore5 };
    
    int pathEscolhaP1 = -1, pathEscolhaP2 = -1;
    int pathResultTimer = 0;
    int pathTimeLeft = 0;
    int salaComPath = 1;

    int deathTimer = 0;

    char historicoFacil[50][100];
    char historicoDificil[50][100];
    int  totalFacil = 0, totalDificil = 0;

    const float BTN_W = 340, BTN_H = 90;
    const Rectangle btnJogarRec = { (VIRT_W - BTN_W)*0.5f, 190, BTN_W, BTN_H };
    const Rectangle btnStatsRec = { (VIRT_W - BTN_W)*0.5f, 320, BTN_W, BTN_H };
    const Rectangle btnSairRec  = { (VIRT_W - BTN_W)*0.5f, 450, BTN_W, BTN_H };

    const Rectangle btnFacilRec   = { (VIRT_W - BTN_W)*0.5f, 250, BTN_W, BTN_H };
    const Rectangle btnDificilRec = { (VIRT_W - BTN_W)*0.5f, 390, BTN_W, BTN_H };

    const int   BTN_NUM_W    = 110;
    const int   BTN_NUM_H    = 55;
    const int   BTN_NUM_GAP  = 20;
    const float BTN_NUM_STARTX = (VIRT_W - (5*BTN_NUM_W + 4*BTN_NUM_GAP)) * 0.5f;
    const float BTN_P1_Y    = 520;
    const float BTN_P2_Y    = 610;

    const Rectangle spr_p1 = { 40,  390, 180, 260 };
    const Rectangle spr_p2 = { 240, 390, 180, 260 };
    const Rectangle spr_enemy = { 860, 130, 380, 430 };

    const Rectangle chestSim = { (VIRT_W*0.5f) - 160, 430, 130, 55 };
    const Rectangle chestNao = { (VIRT_W*0.5f) +  30, 430, 130, 55 };

    const Rectangle goRec = { (VIRT_W - 260)*0.5f, 380, 260, 55 };
    const Rectangle goExi = { (VIRT_W - 260)*0.5f, 460, 260, 55 };

    const Rectangle pathRed   = { 180, 310, 270, 220 };
    const Rectangle pathGreen = { 500, 310, 270, 220 };
    const Rectangle pathBlue  = { 820, 310, 270, 220 };

    const int BTN6_W   = 65;
    const int BTN6_H   = 55;
    const int BTN6_GAP = 10;

    const float PANEL_W = 320;
    const float PANEL_H = 190;
    
    const float P1_PANEL_X   = 20;
    const float P1_PANEL_Y   = VIRT_H - PANEL_H - 20; 
    const float P1_BTN_X     = P1_PANEL_X + 12;      
    const float P1_BTN_TOP_Y = P1_PANEL_Y + 55;      
    
    const float P2_PANEL_X   = VIRT_W - PANEL_W - 20; 
    const float P2_PANEL_Y   = VIRT_H - PANEL_H - 20; 
    const float P2_BTN_X     = P2_PANEL_X + 12;
    const float P2_BTN_TOP_Y = P2_PANEL_Y + 55;

    const Rectangle textboxRec = { 351, VIRT_H - 152 - 20, 578, 152 };

    const Rectangle spr2_p1    = { 340,  260, 170, 330 };
    const Rectangle spr2_p2    = { 540,  260, 170, 330 };
    const Rectangle spr2_enemy = { (VIRT_W - 420) * 0.5f, 150, 420, 480 };

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

        if (state == GAMEPLAY && bossBubble.active) {
            bossBubble.timer++;
            if (bossBubble.timer % 2 == 0) {
                int totalLen = strlen(bossBubble.currentText);
                if (bossBubble.charIndex < totalLen) {
                    bossBubble.charIndex++;
                }
            }
        }

        // =============  LÓGICA  =============

        if (state == MENU)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mouse, btnJogarRec)) state = MODE_SELECT;
                if (CheckCollisionPointRec(mouse, btnStatsRec)) {
                    totalFacil   = LerHistorico("historico_facil.txt",   historicoFacil,   50);
                    totalDificil = LerHistorico("historico_dificil.txt", historicoDificil, 50);
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
                    deathTimer = 0;
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
            if (introLoreCount == 0) {
                state = GAMEPLAY;
                AplicarFalaInimigo(sala, modoDificil, "ENTRY");
            } else {
                if (loreTimer % 2 == 0) {
                    if (loreScene < introLoreCount) {
                        int tamanho = strlen(introLore[loreScene]);
                        if (loreCharIndex < tamanho) loreCharIndex++;
                    }
                }

                if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    int tamanho = strlen(introLore[loreScene]);
                    if (loreCharIndex < tamanho) {
                        loreCharIndex = tamanho;
                    }
                    else {
                        loreScene++;
                        if (loreScene >= introLoreCount) {
                            state = GAMEPLAY;
                            bossBubble.active = false;
                            AplicarFalaInimigo(sala, modoDificil, "ENTRY");
                        }
                        else {
                            loreCharIndex = 0;
                            loreTimer = 0;
                        }
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
            bool p1QuerSim = IsKeyDown(KEY_R);
            bool p2QuerSim = IsKeyDown(KEY_P);
            bool p1QuerNao = IsKeyDown(KEY_F);
            bool p2QuerNao = IsKeyDown(KEY_L);

            #define AVANCAR_SALA() do { \
                salaComPath = !salaComPath; \
                if (salaComPath) { \
                    pathEventIndex  = rand() % NUM_PATH_EVENTS; \
                    pathResultTimer = 0; pathResultMsg[0] = '\0'; \
                    pathEscolhaP1 = -1; pathEscolhaP2 = -1; \
                    pathTimeLeft = modoDificil ? 1500 : 4000; \
                    state = PATH_CHOICE; \
                } else { \
                    rodada = 0; minN = 1; maxN = 100; \
                    MNumber = (rand() % 100) + 1; \
                    printf("DEBUG: %d\n", MNumber); \
                    monsterHP = 50 + (sala * 25); \
                    if (monsterHP < 10) monsterHP = 10; \
                    qntOpcoes = 6; novaRodada = 1; \
                    idxEscolhaP1 = -1; idxEscolhaP2 = -1; \
                    mensagemMonstro[0] = '\0'; \
                    snprintf(mensagem, 200, "Escolham um numero"); \
                    state = GAMEPLAY; \
                    AplicarFalaInimigo(sala, modoDificil, "ENTRY"); \
                } \
            } while(0)

            if (p1QuerSim && p2QuerSim) {
                PlaySound(openSound);
                if ((!modoDificil && sala >= 6) || (modoDificil && sala >= 8)) state = WIN;
                else {
                    AVANCAR_SALA();
                }
            }
            else if (p1QuerNao && p2QuerNao) {
                PlaySound(closeSound);
                if ((!modoDificil && sala >= 6) || (modoDificil && sala >= 8)) state = WIN;
                else {
                    AVANCAR_SALA();
                }
            }
            #undef AVANCAR_SALA
        }
        else if (state == PATH_CHOICE)
        {
            // CORREÇÃO 1: Removido termo 'Michaelmas' corrompido e restaurado validador limpo
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
                AplicarFalaInimigo(sala, modoDificil, "ENTRY"); \
            } while(0)

            if (pathResultTimer > 0) {
                pathResultTimer--;
                if (pathResultTimer == 0) { ENTRAR_GAMEPLAY(); }
            } else {
                if (pathTimeLeft > 0) pathTimeLeft--;

                if (pathTimeLeft == 0) {
                    PathEvent *ev2 = &pathEvents[pathEventIndex];
                    snprintf(pathResultMsg, 200, "TEMPO ESGOTADO! %s", ev2->penalDesc);
                    if (strstr(ev2->penalDesc, "vida")) { int n=1; sscanf(ev2->penalDesc,"Penalidade: Perde %d vida",&n); penalVidas=n; penalMonsterHP=0; }
                    else { int hp=0; sscanf(ev2->penalDesc,"Penalidade: Monstro ganha +%d HP!",&hp); penalMonsterHP=hp; penalVidas=0; }
                    bonusMonsterHP=0; bonusVidas=0;
                    pathResultTimer = 180;
                    pathTimeLeft = -1; 
                }

                PathEvent *ev = &pathEvents[pathEventIndex];
                if (IsKeyPressed(KEY_Q)) pathEscolhaP1 = 0;
                if (IsKeyPressed(KEY_W)) pathEscolhaP1 = 1;
                if (IsKeyPressed(KEY_E)) pathEscolhaP1 = 2;

                if (IsKeyPressed(KEY_U)) pathEscolhaP2 = 0;
                if (IsKeyPressed(KEY_I)) pathEscolhaP2 = 1;
                if (IsKeyPressed(KEY_O)) pathEscolhaP2 = 2;
                
                if (pathEscolhaP1 != -1 && pathEscolhaP2 != -1 && pathEscolhaP1 == pathEscolhaP2 && pathTimeLeft > 0) {
                    int escolhido = pathEscolhaP1;
                    pathTimeLeft = -1; 
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
            if (monsterHP <= 0) {
                deathTimer--;
                if (deathTimer <= 0) {
                    sala++;
                    state = ((!modoDificil && sala >= 6) || (modoDificil && sala >= 8)) ? WIN : CHEST;
                }
            } 
            else 
            {
                if (novaRodada) {
                    rodada++;
                    for (int i = 0; i < 6; i++) {
                        opcoesP1[i] = (rand() % (maxN - minN + 1)) + minN;
                        opcoesP2[i] = (rand() % (maxN - minN + 1)) + minN;
                    }
                    idxEscolhaP1 = -1;
                    idxEscolhaP2 = -1;
                    novaRodada = 0;
                }

                int teclasP1[6] = { KEY_Q, KEY_W, KEY_E, KEY_A, KEY_S, KEY_D };
                for (int i = 0; i < 6; i++) {
                    if (IsKeyPressed(teclasP1[i])) idxEscolhaP1 = i;
                }

                static int confirmadoP1 = 0;
                if (idxEscolhaP1 != -1 && IsKeyPressed(KEY_R)) confirmadoP1 = 1;
                if (IsKeyPressed(KEY_F)) { confirmadoP1 = 0; idxEscolhaP1 = -1; }

                int teclasP2[6] = { KEY_U, KEY_I, KEY_O, KEY_H, KEY_J, KEY_K };
                for (int i = 0; i < 6; i++) {
                    if (IsKeyPressed(teclasP2[i])) idxEscolhaP2 = i;
                }

                static int confirmadoP2 = 0;
                if (idxEscolhaP2 != -1 && IsKeyPressed(KEY_P)) confirmadoP2 = 1;
                if (IsKeyPressed(KEY_L)) { confirmadoP2 = 0; idxEscolhaP2 = -1; }

                if (confirmadoP1 && confirmadoP2)
                {
                    int escolhaP1 = opcoesP1[idxEscolhaP1];
                    int escolhaP2 = opcoesP2[idxEscolhaP2];

                    int danoP1 = calcularDano(escolhaP1, MNumber);
                    int danoP2 = calcularDano(escolhaP2, MNumber);
                    int danoTotal = danoP1 + danoP2;

                    monsterHP -= danoTotal;
                    if (monsterHP < 0) monsterHP = 0;

                    mensagemMonstro[0] = '\0';
                    char dica[100] = "";

                    if (!modoDificil)
                    {
                        int media = (escolhaP1 + escolhaP2) / 2;
                        if (media < MNumber) {
                            minN = media + 1;
                            snprintf(dica,100,"Numero maior que %d",media);
                        }
                        else if (media > MNumber) {
                            maxN = media - 1;
                            snprintf(dica,100,"Numero menor que %d",media);
                        }
                    }

                    if (modoDificil)
                        snprintf(mensagem,200, "P1:%d dano | P2:%d dano | Total:%d", danoP1,danoP2,danoTotal);
                    else
                        snprintf(mensagem,200, "P1:%d | P2:%d | Total:%d | %s", danoP1,danoP2,danoTotal,dica);

                    if (monsterHP > 0) {
                        if (danoP1 == 40 || danoP2 == 40) {
                            AplicarFalaInimigo(sala, modoDificil, "CRITICAL");
                        } else if (danoTotal > 10) {
                            AplicarFalaInimigo(sala, modoDificil, "HIT");
                        } else if (danoTotal <= 10) {
                            AplicarFalaInimigo(sala, modoDificil, "IDLE");
                        }
                    } else {
                        AplicarFalaInimigo(sala, modoDificil, "DEATH");
                        deathTimer = 180; 
                    }

                    confirmadoP1 = 0;
                    confirmadoP2 = 0;
                    novaRodada = 1;
                }

                if (rodada % 5 == 0 && novaRodada == 0) {
                    vidas--;
                    rodada++;
                    snprintf(mensagemMonstro, 200, ">>> O monstro atacou! -1 vida! <<<");
                }

                if (vidas <= 0) {
                    if (!resultadoSalvo) {
                        SalvarHistorico("MORREU", sala, modoDificil);
                        resultadoSalvo = 1;
                    }
                    state = GAMEOVER;
                }
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
            const char *titulo = "ESCOLHA O MODO";
            DrawText(titulo, (VIRT_W - MeasureText(titulo, 46)) / 2, 155, 46, BLACK);

            DrawTexVirt(btnFacil,   btnFacilRec,   hF ? LIGHTGRAY : WHITE);
            DrawTexVirt(btnDificil, btnDificilRec, hD ? LIGHTGRAY : WHITE);
        }
        else if (state == LORE)
        {
            Texture2D *bg = loreTextures[loreScene];
            DrawTexturePro(*bg, (Rectangle){0, 0, (float)bg->width, (float)bg->height}, (Rectangle){0, 0, VIRT_W, VIRT_H}, (Vector2){0, 0}, 0, WHITE);
            DrawRectangle(0, VIRT_H - 170, VIRT_W, 170, (Color){0,0,0,180});

            char visibleText[512] = {0};
            if (loreScene < introLoreCount) {
                strncpy(visibleText, introLore[loreScene], loreCharIndex);
                visibleText[loreCharIndex] = '\0';
            }

            DrawText(visibleText, 63, VIRT_H - 127, 30, BLACK);
            DrawText(visibleText, 60, VIRT_H - 130, 30, WHITE);
            DrawText("ENTER ou clique para continuar", VIRT_W - 420, VIRT_H - 40, 22, LIGHTGRAY);
        }
        else if (state == STATS)
        {
            DrawTexVirt(menuBg, (Rectangle){0,0,VIRT_W,VIRT_H}, WHITE);
            DrawRectangle(0,0,VIRT_W,VIRT_H,(Color){0,0,0,130});         

            const char *titulo = "ESTATISTICAS";
            DrawText(titulo, (VIRT_W - MeasureText(titulo, 40)) / 2, 30, 40, WHITE);

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
            bool p1ApertandoSim = IsKeyDown(KEY_R);
            bool p2ApertandoSim = IsKeyDown(KEY_P);
            bool p1ApertandoNao = IsKeyDown(KEY_F);
            bool p2ApertandoNao = IsKeyDown(KEY_L);

            DrawTexturePro(dropBg, (Rectangle){0,0,(float)dropBg.width,(float)dropBg.height}, (Rectangle){0,0,VIRT_W,VIRT_H}, (Vector2){0,0}, 0, WHITE);

            const char *t1 = "Voce derrotou o monstro";
            const char *t2 = "e ele deixou cair seus dados";
            const char *t3 = "Deseja baixar os dados? (Requer sincronia)";

            DrawText(t1, ((VIRT_W - MeasureText(t1, 38))/2) + 3, 183, 38, BLACK);
            DrawText(t2, ((VIRT_W - MeasureText(t2, 32))/2) + 3, 243, 32, BLACK);
            DrawText(t3, ((VIRT_W - MeasureText(t3, 28))/2) + 3, 323, 28, BLACK);

            DrawText(t1, (VIRT_W - MeasureText(t1, 38))/2, 180, 38, WHITE);
            DrawText(t2, (VIRT_W - MeasureText(t2, 32))/2, 240, 32, WHITE);
            DrawText(t3, (VIRT_W - MeasureText(t3, 28))/2, 320, 28, WHITE);

            Color simColor = (p1ApertandoSim && p2ApertandoSim) ? GREEN : ((p1ApertandoSim || p2ApertandoSim) ? DARKGREEN : DARKGRAY);
            Color naoColor = (p1ApertandoNao && p2ApertandoNao) ? RED   : ((p1ApertandoNao || p2ApertandoNao) ? MAROON    : DARKGRAY);

            DrawRectangleRec(chestSim, simColor);
            DrawRectangleRec(chestNao, naoColor);

            DrawText("SIM (R + P)", (int)(chestSim.x + (chestSim.width  - MeasureText("SIM (R + P)",18))*0.5f), (int)(chestSim.y+18), 18, WHITE);
            DrawText("NAO (F + L)", (int)(chestNao.x + (chestNao.width  - MeasureText("NAO (F + L)",18))*0.5f), (int)(chestNao.y+18), 18, WHITE);
            
            const char* dicaSincro = "[ P1 e P2 devem segurar as respectivas teclas juntas para prosseguir ]";
            DrawText(dicaSincro, (VIRT_W - MeasureText(dicaSincro, 16)) / 2, VIRT_H - 120, 16, LIGHTGRAY);
        }
        else if (state == PATH_CHOICE)
        {
            PathEvent *ev = &pathEvents[pathEventIndex];
            bool canClick = (pathResultTimer == 0);
            bool hR = canClick && CheckCollisionPointRec(mouse, pathRed);
            bool hG = canClick && CheckCollisionPointRec(mouse, pathGreen);
            bool hB = canClick && CheckCollisionPointRec(mouse, pathBlue);

            DrawTexturePro(circuitBg, (Rectangle){0,0,(float)circuitBg.width,(float)circuitBg.height}, (Rectangle){0,0,VIRT_W,VIRT_H},(Vector2){0,0},0,WHITE);
            DrawRectangle(0,0,VIRT_W,VIRT_H,(Color){0,0,0,130});

            const char *titulo = ">> ENCRUZILHADA DO SISTEMA <<";
            DrawText(titulo, (VIRT_W - MeasureText(titulo,30))/2, 12, 30, (Color){0,220,80,255});
            DrawLine(60,56,VIRT_W-60,56,(Color){0,180,60,120});

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

            {
                // CORREÇÃO 2: Ajustado o espaço indevido ('modo Dificil' -> 'modoDificil')
                int totalFrames = modoDificil ? 900 : 1500;
                int framesLeft  = (pathTimeLeft > 0) ? pathTimeLeft : 0;
                int segundos    = (framesLeft + 59) / 60;
                float progTimer = (totalFrames > 0) ? (float)framesLeft / totalFrames : 0.0f;
                Color timerCol  = (segundos <= 5) ? RED : (segundos <= 10 ? ORANGE : (Color){0,220,80,255});

                char timeBuf[32];
                snprintf(timeBuf, 32, "%02d s", segundos);
                DrawRectangle(60, 286, VIRT_W-120, 10, (Color){40,40,40,200});
                DrawRectangle(60, 286, (int)((VIRT_W-120)*progTimer), 10, timerCol);
                DrawText(timeBuf, VIRT_W - MeasureText(timeBuf,18) - 65, 280, 18, timerCol);
            }

            if (!modoDificil) {
                const char *nomes[] = {"VERMELHO","VERDE","AZUL"};
                if (pathEscolhaP1 != -1) {
                    char buf[64]; snprintf(buf, 64, "P1: %s", nomes[pathEscolhaP1]);
                    DrawRectangle(30, 306, MeasureText(buf,18)+16, 26, (Color){0,0,0,180});
                    DrawText(buf, 38, 310, 18, (Color){100,210,255,255});
                } else {
                    DrawRectangle(30, 306, 130, 26, (Color){0,0,0,100});
                    DrawText("P1: ???", 38, 310, 18, (Color){120,120,120,200});
                }
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
                DrawText("[PROC: 0x52]", (int)(pathRed.x + (pathRed.width - MeasureText("[PROC: 0x52]",14))*0.5f), (int)(pathRed.y+12), 14, (Color){255,180,180,200});
                const char *lbl = "VERMELHO";
                DrawText(lbl, (int)(pathRed.x + (pathRed.width - MeasureText(lbl,22))*0.5f), (int)(pathRed.y + pathRed.height*0.5f - 10), 22, WHITE);
                if (!modoDificil && (pathEscolhaP1 == 0 || pathEscolhaP2 == 0))
                    DrawText("[SELECIONADO]", (int)(pathRed.x+(pathRed.width-MeasureText("[SELECIONADO]",15))*0.5f), (int)(pathRed.y+pathRed.height-28), 15, (Color){255,220,220,255});
            }
            // Painel VERDE
            {
                Color bg = hG ? (Color){80,255,100,255} : (Color){10,130,40,255};
                DrawRectangleRec(pathGreen, bg);
                DrawRectangleLinesEx(pathGreen, hG?4:2, hG?WHITE:(Color){80,200,100,255});
                DrawText("[PROC: 0x47]", (int)(pathGreen.x + (pathGreen.width - MeasureText("[PROC: 0x47]",14))*0.5f), (int)(pathGreen.y+12), 14, (Color){180,255,190,200});
                const char *lbl = "VERDE";
                DrawText(lbl, (int)(pathGreen.x + (pathGreen.width - MeasureText(lbl,22))*0.5f), (int)(pathGreen.y + pathGreen.height*0.5f - 10), 22, WHITE);
                if (!modoDificil && (pathEscolhaP1 == 1 || pathEscolhaP2 == 1))
                    DrawText("[SELECIONADO]", (int)(pathGreen.x+(pathGreen.width-MeasureText("[SELECIONADO]",15))*0.5f), (int)(pathGreen.y+pathGreen.height-28), 15, (Color){220,255,220,255});
            }
            // Painel AZUL
            {
                Color bg = hB ? (Color){80,160,255,255} : (Color){15,40,160,255};
                DrawRectangleRec(pathBlue, bg);
                DrawRectangleLinesEx(pathBlue, hB?4:2, hB?WHITE:(Color){80,120,255,255});
                DrawText("[PROC: 0x42]", (int)(pathBlue.x + (pathBlue.width - MeasureText("[PROC: 0x42]",14))*0.5f), (int)(pathBlue.y+12), 14, (Color){160,190,255,200});
                const char *lbl = "AZUL";
                DrawText(lbl, (int)(pathBlue.x + (pathBlue.width - MeasureText(lbl,22))*0.5f), (int)(pathBlue.y + pathBlue.height*0.5f - 10), 22, WHITE);
                if (!modoDificil && (pathEscolhaP1 == 2 || pathEscolhaP2 == 2))
                    DrawText("[SELECIONADO]", (int)(pathBlue.x+(pathBlue.width-MeasureText("[SELECIONADO]",15))*0.5f), (int)(pathBlue.y+pathBlue.height-28), 15, (Color){200,210,255,255});
            }

            if (pathResultTimer > 0) {
                DrawRectangle(60, 560, VIRT_W-120, 75, (Color){0,0,0,230});
                DrawRectangleLinesEx((Rectangle){60,560,VIRT_W-120,75}, 2, (Color){0,220,80,255});
                bool acertou = (strstr(pathResultMsg, "CERTO") != NULL);
                char fullMsg[256];
                snprintf(fullMsg,256,"%s %s", acertou?"[OK] ":"[ERR]", pathResultMsg);
                DrawText(fullMsg, (VIRT_W-MeasureText(fullMsg,20))/2, 575, 20, acertou?(Color){80,255,100,255}:(Color){255,80,80,255});
                float prog = (float)pathResultTimer/180.0f;
                DrawRectangle(62, 622, (int)((VIRT_W-124)*prog), 10, acertou?(Color){0,200,60,255}:(Color){200,50,50,255});
            } else {
                const char *hint = "[ Clique ou use teclas para prosseguir ]";
                DrawText(hint, (VIRT_W-MeasureText(hint,15))/2, 648, 15, (Color){0,100,40,180});
            }
        }
        else if (state == GAMEPLAY)
        {
            DrawTexVirt(gameBg, (Rectangle){0,0,VIRT_W,VIRT_H}, WHITE);

            Texture2D *enemyTex = GetEnemyTexture(sala, modoDificil,
                &slimeTex,&ogroTex,&bossTex,
                &mariTex,&romaTex,&luisTex,&micaTex,&ruanTex,&lucasTex,&lucas2Tex);
            
            DrawTexturePro(*enemyTex, (Rectangle){0,0,(float)enemyTex->width,(float)enemyTex->height}, spr2_enemy, (Vector2){0,0}, 0, (monsterHP <= 0) ? RED : WHITE);

            DrawRectangle(0, 0, VIRT_W, 95, (Color){0,0,0,210});
            DrawText(TextFormat("MISSAO %d", sala), 20, 10, 32, (Color){0,255,120,255});

            if (modoDificil) {
                const char* bName = (strlen(bosses[sala-1].name) > 0) ? bosses[sala-1].name : "Chefe Desconhecido";
                const char* bDesc = (strlen(bosses[sala-1].desc) > 0) ? bosses[sala-1].desc : "Seguranca corrompida detectada.";
                
                DrawText(bName, 20, 48, 22, WHITE);
                int descX = 20 + MeasureText(bName, 22) + 20;
                DrawText(bDesc, descX, 52, 16, (Color){190,190,190,255});
            } else {
                const char *ebn = (sala == 5 || sala == 6) ? "Boss Ogro" : ((sala % 2 == 0) ? "Ogro" : "Slime");
                DrawText(ebn, 20, 48, 24, WHITE);
                DrawText("Uma criatura bloqueia o caminho.", 200, 52, 16, LIGHTGRAY);
            }

            const char *modoTxt = modoDificil ? "MODO DIFICIL" : "MODO FACIL";
            DrawText(modoTxt, VIRT_W - MeasureText(modoTxt,18) - 14, 10, 18, modoDificil?RED:(Color){0,220,80,255});
            DrawText(TextFormat("VIDAS: %d", vidas), VIRT_W - MeasureText(TextFormat("VIDAS: %d",vidas),18) - 14, 36, 18, RED);
            DrawText(TextFormat("HP: %d", monsterHP), VIRT_W - MeasureText(TextFormat("HP: %d",monsterHP),18) - 14, 58, 18, GREEN);

            DrawRectangle(300, 100, 680, 118, (Color){0,0,0,170});
            DrawText(mensagem, 300 + (680 - MeasureText(mensagem,20))/2, 115, 20, SKYBLUE);
            if (mensagemMonstro[0] != '\0')
                DrawText(mensagemMonstro, 300 + (680 - MeasureText(mensagemMonstro,18))/2, 150, 18, RED);

            if (bossBubble.active) {
                DrawTexVirt(textboxTex, textboxRec, WHITE);
                
                char textBuffer[256] = {0};
                strncpy(textBuffer, bossBubble.currentText, bossBubble.charIndex);
                textBuffer[bossBubble.charIndex] = '\0';
                
                const char* currentEnemyName = modoDificil ? ((strlen(bosses[sala-1].name) > 0) ? bosses[sala-1].name : "Boss") : ((sala == 5 || sala == 6) ? "Rei Ogro" : ((sala % 2 == 0) ? "Ogro" : "Slime"));
                DrawText(currentEnemyName, textboxRec.x + 25, textboxRec.y + 18, 18, GOLD);
                DrawText(textBuffer, textboxRec.x + 25, textboxRec.y + 50, 18, WHITE);
            }

            // Interface Player 1
            DrawRectangleRec((Rectangle){P1_PANEL_X, P1_PANEL_Y, PANEL_W, PANEL_H}, (Color){0,0,0,180});
            DrawRectangleLinesEx((Rectangle){P1_PANEL_X, P1_PANEL_Y, PANEL_W, PANEL_H}, 2, (Color){100,180,255,120});
            DrawText("PLAYER 1", P1_PANEL_X + (PANEL_W - MeasureText("PLAYER 1",18))*0.5f, P1_PANEL_Y + 10, 18, (Color){100,180,255,255});
            DrawText("Teclas: QWER / ASDF", P1_PANEL_X + (PANEL_W - MeasureText("Teclas: 1 a 6",12))*0.5f, P1_PANEL_Y + 32, 12, (Color){160,160,160,200});

            for (int i = 0; i < 8; i++) {
                int col = (i <= 3) ? i : i - 4;
                int row = (i <= 3) ? 0 : 1;
                Rectangle btn = { P1_BTN_X + col * (BTN6_W + BTN6_GAP), P1_BTN_TOP_Y + row * (BTN6_H + BTN6_GAP), BTN6_W, BTN6_H };

                if (i == 0 || i == 1 || i == 2 || i == 4 || i == 5 || i == 6) {
                    int optionIndex = (i <= 2) ? i : i - 1;
                    bool selected = (idxEscolhaP1 == optionIndex);
                    DrawRectangleRec(btn, selected ? ORANGE : (Color){50,50,80,220});
                    DrawRectangleLinesEx(btn, selected ? 3 : 1, selected ? WHITE : (Color){100,100,140,200});
                    const char *keyLabel[] = { "Q","W","E", "A","S","D" };
                    DrawText(keyLabel[optionIndex], (int)(btn.x+5), (int)(btn.y+4), 11, WHITE);
                    const char *ns = TextFormat("%d", opcoesP1[optionIndex]);
                    DrawText(ns, (int)(btn.x + (BTN6_W - MeasureText(ns,20))*0.5f), (int)(btn.y + (BTN6_H - 20)*0.5f), 20, WHITE);
                }
                else if (i == 3) {
                    DrawRectangleRec(btn, DARKGREEN); DrawRectangleLinesEx(btn, 2, GREEN);
                    DrawText("CONF", (int)(btn.x + 6), (int)(btn.y + 18), 16, WHITE);
                    DrawText("R", (int)(btn.x + BTN6_W - 18), (int)(btn.y + 4), 14, YELLOW);
                } else {
                    DrawRectangleRec(btn, MAROON); DrawRectangleLinesEx(btn, 2, RED);
                    DrawText("CANC", (int)(btn.x + 6), (int)(btn.y + 18), 16, WHITE);
                    DrawText("F", (int)(btn.x + BTN6_W - 18), (int)(btn.y + 4), 14, YELLOW);
                }
            }
        
            // Interface Player 2
            DrawRectangleRec((Rectangle){P2_PANEL_X, P2_PANEL_Y, PANEL_W, PANEL_H}, (Color){0,0,0,180});
            DrawRectangleLinesEx((Rectangle){P2_PANEL_X, P2_PANEL_Y, PANEL_W, PANEL_H}, 2, (Color){255,120,120,120});
            DrawText("PLAYER 2", P2_PANEL_X + (PANEL_W - MeasureText("PLAYER 2",18))*0.5f, P2_PANEL_Y + 10, 18, (Color){255,120,120,255});
            DrawText("Teclas: UIOP / HJKL", P2_PANEL_X + (PANEL_W - MeasureText("Teclas: 1 a 6",12))*0.5f, P2_PANEL_Y + 32, 12, (Color){160,160,160,200});

            for (int i = 0; i < 8; i++) {
                int col = (i <= 3) ? i : i - 4;
                int row = (i <= 3) ? 0 : 1;
                Rectangle btn = { P2_BTN_X + col * (BTN6_W + BTN6_GAP), P2_BTN_TOP_Y + row * (BTN6_H + BTN6_GAP), BTN6_W, BTN6_H };

                if (i == 0 || i == 1 || i == 2 || i == 4 || i == 5 || i == 6) {
                    int optionIndex = (i <= 2) ? i : i - 1;
                    bool selected = (idxEscolhaP2 == optionIndex);
                    DrawRectangleRec(btn, selected ? MAGENTA : (Color){80,20,50,220});
                    DrawRectangleLinesEx(btn, selected ? 3 : 1, selected ? WHITE : (Color){140,60,100,200});
                    const char *keyLabel[] = { "U","I","O", "H","J","K" };
                    DrawText(keyLabel[optionIndex], (int)(btn.x+5), (int)(btn.y+4), 11, WHITE);
                    const char *ns = TextFormat("%d", opcoesP2[optionIndex]);
                    DrawText(ns, (int)(btn.x + (BTN6_W - MeasureText(ns,20))*0.5f), (int)(btn.y + (BTN6_H - 20)*0.5f), 20, WHITE);
                }
                else if (i == 3) {
                    DrawRectangleRec(btn, DARKGREEN); DrawRectangleLinesEx(btn, 2, GREEN);
                    DrawText("CONF", (int)(btn.x + 6), (int)(btn.y + 18), 16, WHITE);
                    DrawText("P", (int)(btn.x + BTN6_W - 18), (int)(btn.y + 4), 14, YELLOW);
                } else {
                    DrawRectangleRec(btn, MAROON); DrawRectangleLinesEx(btn, 2, RED);
                    DrawText("CANC", (int)(btn.x + 6), (int)(btn.y + 18), 16, WHITE);
                    DrawText("L", (int)(btn.x + BTN6_W - 18), (int)(btn.y + 4), 14, YELLOW);
                }
            }

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

    for(int i=0; i<introLoreCount; i++) free(introLore[i]);
    for(int b=0; b<MAX_BOSSES; b++) {
        for(int i=0; i<bosses[b].entryCount; i++) free(bosses[b].entry[i]);
        for(int i=0; i<bosses[b].idleCount; i++) free(bosses[b].idle[i]);
        for(int i=0; i<bosses[b].hitCount; i++) free(bosses[b].hit[i]);
        for(int i=0; i<bosses[b].criticalCount; i++) free(bosses[b].critical[i]);
        for(int i=0; i<bosses[b].deathCount; i++) free(bosses[b].death[i]);
    }
    for(int i=0; i<easySlime.entryCount; i++) free(easySlime.entry[i]);
    for(int i=0; i<easySlime.idleCount; i++) free(easySlime.idle[i]);
    for(int i=0; i<easySlime.hitCount; i++) free(easySlime.hit[i]);
    for(int i=0; i<easySlime.criticalCount; i++) free(easySlime.critical[i]);
    for(int i=0; i<easySlime.deathCount; i++) free(easySlime.death[i]);
    
    for(int i=0; i<easyOgre.entryCount; i++) free(easyOgre.entry[i]);
    for(int i=0; i<easyOgre.idleCount; i++) free(easyOgre.idle[i]);
    for(int i=0; i<easyOgre.hitCount; i++) free(easyOgre.hit[i]);
    for(int i=0; i<easyOgre.criticalCount; i++) free(easyOgre.critical[i]);
    for(int i=0; i<easyOgre.deathCount; i++) free(easyOgre.death[i]);

    for(int i=0; i<easyBoss.entryCount; i++) free(easyBoss.entry[i]);
    for(int i=0; i<easyBoss.idleCount; i++) free(easyBoss.idle[i]);
    for(int i=0; i<easyBoss.hitCount; i++) free(easyBoss.hit[i]);
    for(int i=0; i<easyBoss.criticalCount; i++) free(easyBoss.critical[i]);
    for(int i=0; i<easyBoss.deathCount; i++) free(easyBoss.death[i]);

    UnloadRenderTexture(canvas);
    UnloadTexture(menuBg); UnloadTexture(gameBg); UnloadTexture(circuitBg);
    UnloadTexture(btnNovoJogo); UnloadTexture(btnFacil); UnloadTexture(btnDificil);
    UnloadTexture(btnStats); UnloadTexture(btnSair);
    UnloadTexture(slimeTex); UnloadTexture(ogroTex); UnloadTexture(bossTex);
    UnloadTexture(mariTex); UnloadTexture(romaTex); UnloadTexture(luisTex);
    UnloadTexture(micaTex); UnloadTexture(ruanTex); UnloadTexture(lucasTex);
    UnloadTexture(lucas2Tex);
    UnloadTexture(lore1); UnloadTexture(lore2); UnloadTexture(lore3); UnloadTexture(lore4); UnloadTexture(lore5);
    UnloadTexture(dropBg);
    UnloadTexture(textboxTex); 
    UnloadMusicStream(menuMusic); UnloadMusicStream(gameMusic);
    UnloadSound(openSound); UnloadSound(closeSound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}