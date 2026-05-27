#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define VIRT_W 1280
#define VIRT_H 720

typedef enum {
    MENU,
    MODE_SELECT,
    LORE,
    GAMEPLAY,
    GAMEOVER,
    WIN,
    FINAL_LORE,
    TRANSFORM_LORE,
    STATS,
    CHEST,
    PATH_CHOICE
} GameState;

#define MAX_LORE_LINES 20
#define MAX_FRASES 10
#define MAX_BOSSES 7

char* introLore[MAX_LORE_LINES];
int   introLoreCount = 0;

char* transformLore[MAX_LORE_LINES];
int   transformLoreCount = 0;

char* finalLore[MAX_LORE_LINES];
int   finalLoreCount = 0;

typedef struct {
    char name[100];
    char desc[256];
    char* entry[MAX_FRASES];    int entryCount;
    char* idle[MAX_FRASES];     int idleCount;
    char* hit[MAX_FRASES];      int hitCount;
    char* critical[MAX_FRASES]; int criticalCount;
    char* death[MAX_FRASES];    int deathCount;
} BossData;

BossData bosses[MAX_BOSSES];

typedef struct {
    char* entry[MAX_FRASES];    int entryCount;
    char* idle[MAX_FRASES];     int idleCount;
    char* hit[MAX_FRASES];      int hitCount;
    char* critical[MAX_FRASES]; int criticalCount;
    char* death[MAX_FRASES];    int deathCount;
} EasyEnemyData;

EasyEnemyData easySlime, easyOgre, easyBoss;

typedef struct {
    char currentText[256];
    int  charIndex;
    int  timer;
    bool active;
} SpeechBubble;

SpeechBubble bossBubble = { "", 0, 0, false };

// ─── Buffs / Debuffs do baú ───────────────────────────────────────────────
typedef struct {
    const char *desc;
    int  vidasDelta;      // positivo = ganho, negativo = perda
    int  hpDelta;         // aplicado no PRÓXIMO monstro (negativo = facilita)
    int  maxNDelta;       // restringe o range de palpites (positivo = estreita)
    int  minNDelta;
} ChestEffect;

static ChestEffect chestBuffs[] = {
    { "+1 Vida extra!",          +1,   0,  0 ,  0 },
    { "+1 Vida e range 5-95",  +1,   0,   -5,  +5 },
    { "Proximo monstro -20 HP!",  0, -20,   0,  0 },
    { "Proximo monstro -30 HP!",  0, -30,   0,  0 },
    { "Range restrito: 10-90!",   0,   0, -10, +10 },
};
#define NUM_BUFFS (sizeof(chestBuffs)/sizeof(chestBuffs[0]))

static ChestEffect chestDebuffs[] = {
    { "-1 Vida!",               -1,   0,    0,  0 },
    { "-2 Vidas!",              -2,   0,    0,  0 },
    { "Proximo monstro +25 HP!",  0, +25,    0,  0 },
    { "Proximo monstro +35 HP!",  0, +35,    0,  0 },
    { "Range ampliado: 1-125!",   0,   0, +25, 0 },
    { "Range ampliado: 1-150!",   0,   0, +50, 0 },
    { "Grande azar! Range: 200!",   0,   0, +100, 0 },
};
#define NUM_DEBUFFS (sizeof(chestDebuffs)/sizeof(chestDebuffs[0]))

// Resultado do sorteio do baú (aplicado ao entrar na próxima sala)
static int   chestVidaDelta   = 0;
static int   chestHpDelta     = 0;
static int   chestMaxNDelta   = 0;
static int   chestMinNDelta   = 0;
static char  chestResultMsg[128] = "";
static bool  chestIsBuff      = false;
static int recSoma(int *v, int n)    { return n==0 ? 0 : v[0] + recSoma(v+1, n-1); }
static int recMin(int *v, int n)     { return n==1 ? v[0] : (v[0]<recMin(v+1,n-1)?v[0]:recMin(v+1,n-1)); }
static int recMax(int *v, int n)     { return n==1 ? v[0] : (v[0]>recMax(v+1,n-1)?v[0]:recMax(v+1,n-1)); }
static long recSomaSq(int *v, int n) { return n==0 ? 0 : (long)v[0]*v[0] + recSomaSq(v+1, n-1); }

static const char* GerarHeuristica(float media, int melhor, int pior, float desvio, int salaMax) {
    if (melhor == salaMax)
        return "COMPLETO! Tente o outro modo!";

    if (desvio > 2.5f)
        return "Inconsistente: combinem quem vai alto e quem vai baixo.";

    if (media >= (float)salaMax * 0.5f && desvio <= 1.5f && melhor < salaMax)
        return "Bau: indo bem? Recusem! Debuff pode estragar tudo.";

    if (salaMax == 5) {
        if (melhor <= 1)
            return "Slime: P1 chuta ~25, P2 chuta ~75 no range.";
        if (melhor == 2)
            return "Use maior/menor para afunilar no 2o Slime.";
        if (melhor <= 4 && media < 3.0f)
            return "Ogro: dano 25+ significa que estao proximos!";
        if (melhor == 4)
            return "Quase no Boss! Cheguem com vidas sobrando.";
        if (media >= 4.0f)
            return "No Boss: um vai em 1/4, o outro em 3/4 do range.";
        if (media >= 2.5f && desvio <= 1.2f)
            return "Bau: consistentes? Recusem o risco!";
        return "Dividam o range: P1 baixo, P2 alto. Sempre.";
    }

    if (salaMax == 7) {
        if (melhor <= 1)
            return "Dano 40=acerto, 25=perto, 15=quase. Memorizem!";
        if (melhor <= 2)
            return "Encruzilhada: acertar da bonus de HP no boss.";
        if (melhor <= 4 && desvio > 1.5f)
            return "Memorizem quais numeros deram mais dano antes.";
        if (melhor <= 4)
            return "Bau: buff de -HP no boss vale mais que +vidas.";
        if (melhor <= 6 && media < 5.0f)
            return "Encruzilhada certa = boss mais fraco. Pensem!";
        if (media >= 5.0f)
            return "Boss final: P1 em 1/3, P2 em 2/3 do range.";
        if (media >= 3.0f && desvio <= 1.5f)
            return "Indo bem? Recusem o bau, nao arrisquem!";
        return "Dano 15+ = diferenca de ate 10. Ajustem dai.";
    }

    return "Jogue mais para receber analise personalizada!";
}

// ─────────────────────────────────────────────────────────────────────────

void TrimLine(char *line) {
    int len = strlen(line);
    while (len > 0 && (line[len-1]=='\n'||line[len-1]=='\r'||line[len-1]==' ')) {
        line[len-1] = '\0'; len--;
    }
}

void CarregarLore(const char *filename) {
    introLoreCount = 0;
    transformLoreCount = 0;
    finalLoreCount = 0;
    for (int i = 0; i < MAX_BOSSES; i++) {
        strcpy(bosses[i].name, ""); strcpy(bosses[i].desc, "");
        bosses[i].entryCount = bosses[i].idleCount = bosses[i].hitCount =
        bosses[i].criticalCount = bosses[i].deathCount = 0;
    }
    easySlime.entryCount=easySlime.idleCount=easySlime.hitCount=easySlime.criticalCount=easySlime.deathCount=0;
    easyOgre.entryCount=easyOgre.idleCount=easyOgre.hitCount=easyOgre.criticalCount=easyOgre.deathCount=0;
    easyBoss.entryCount=easyBoss.idleCount=easyBoss.hitCount=easyBoss.criticalCount=easyBoss.deathCount=0;

    FILE *file = fopen(filename, "r");
    if (!file) { printf("ERRO: Nao foi possivel abrir %s\n", filename); return; }

    char line[256], currentTag[50] = "";
    while (fgets(line, sizeof(line), file)) {
        TrimLine(line);
        if (line[0]=='\0' || line[0]=='#') continue;
        if (line[0]=='[' && line[strlen(line)-1]==']') { strcpy(currentTag, line); continue; }

        if (strcmp(currentTag,"[INTRO]")==0 && introLoreCount<MAX_LORE_LINES)
            introLore[introLoreCount++] = strdup(line);

        if (strcmp(currentTag,"[TRANSFORM_LORE]")==0 && transformLoreCount<MAX_LORE_LINES)
        transformLore[transformLoreCount++] = strdup(line);

        if (strcmp(currentTag,"[FINAL_WIN]")==0 && finalLoreCount<MAX_LORE_LINES)
            finalLore[finalLoreCount++] = strdup(line);

        for (int i = 1; i <= MAX_BOSSES; i++) {
            char t[50];
            snprintf(t,50,"[BOSS_NAME_%d]",i); if(strcmp(currentTag,t)==0){strncpy(bosses[i-1].name,line,99);bosses[i-1].name[99]='\0';}
            snprintf(t,50,"[BOSS_DESC_%d]",i); if(strcmp(currentTag,t)==0){strncpy(bosses[i-1].desc,line,255);bosses[i-1].desc[255]='\0';}
            snprintf(t,50,"[BOSS_%d_ENTRY]",i);    if(strcmp(currentTag,t)==0&&bosses[i-1].entryCount<MAX_FRASES)    bosses[i-1].entry[bosses[i-1].entryCount++]=strdup(line);
            snprintf(t,50,"[BOSS_%d_IDLE]",i);     if(strcmp(currentTag,t)==0&&bosses[i-1].idleCount<MAX_FRASES)     bosses[i-1].idle[bosses[i-1].idleCount++]=strdup(line);
            snprintf(t,50,"[BOSS_%d_HIT]",i);      if(strcmp(currentTag,t)==0&&bosses[i-1].hitCount<MAX_FRASES)      bosses[i-1].hit[bosses[i-1].hitCount++]=strdup(line);
            snprintf(t,50,"[BOSS_%d_CRITICAL]",i); if(strcmp(currentTag,t)==0&&bosses[i-1].criticalCount<MAX_FRASES) bosses[i-1].critical[bosses[i-1].criticalCount++]=strdup(line);
            snprintf(t,50,"[BOSS_%d_DEATH]",i);    if(strcmp(currentTag,t)==0&&bosses[i-1].deathCount<MAX_FRASES)    bosses[i-1].death[bosses[i-1].deathCount++]=strdup(line);
        }
        #define EASY_LOAD(TAG,DST,FIELD) if(strcmp(currentTag,TAG)==0&&DST.FIELD##Count<MAX_FRASES) DST.FIELD[DST.FIELD##Count++]=strdup(line);
        EASY_LOAD("[EASY_SLIME_ENTRY]",   easySlime, entry)   EASY_LOAD("[EASY_SLIME_IDLE]",    easySlime, idle)
        EASY_LOAD("[EASY_SLIME_HIT]",      easySlime, hit)     EASY_LOAD("[EASY_SLIME_CRITICAL]",easySlime, critical)
        EASY_LOAD("[EASY_SLIME_DEATH]",   easySlime, death)
        EASY_LOAD("[EASY_OGRE_ENTRY]",    easyOgre,  entry)   EASY_LOAD("[EASY_OGRE_IDLE]",     easyOgre,  idle)
        EASY_LOAD("[EASY_OGRE_HIT]",      easyOgre,  hit)     EASY_LOAD("[EASY_OGRE_CRITICAL]", easyOgre,  critical)
        EASY_LOAD("[EASY_OGRE_DEATH]",    easyOgre,  death)
        EASY_LOAD("[EASY_BOSS_ENTRY]",    easyBoss,  entry)   EASY_LOAD("[EASY_BOSS_IDLE]",     easyBoss,  idle)
        EASY_LOAD("[EASY_BOSS_HIT]",      easyBoss,  hit)     EASY_LOAD("[EASY_BOSS_CRITICAL]", easyBoss,  critical)
        EASY_LOAD("[EASY_BOSS_DEATH]",    easyBoss,  death)
        #undef EASY_LOAD
    }
    fclose(file);
}

void TriggerSpeech(const char *text) {
    if (!text || strlen(text)==0) return;
    strncpy(bossBubble.currentText, text, sizeof(bossBubble.currentText)-1);
    bossBubble.currentText[sizeof(bossBubble.currentText)-1] = '\0';
    bossBubble.charIndex = 0; bossBubble.timer = 0; bossBubble.active = true;
}

void AplicarFalaInimigo(int sala, int modoDificil, const char *gatilho) {
    int idx = (sala-1 < 0) ? 0 : sala-1;
    if (modoDificil) {
        if (idx >= MAX_BOSSES) return;
        BossData *b = &bosses[idx];
        if (strcmp(gatilho,"ENTRY")==0    && b->entryCount>0)    TriggerSpeech(b->entry[rand()%b->entryCount]);
        else if(strcmp(gatilho,"IDLE")==0 && b->idleCount>0)     TriggerSpeech(b->idle[rand()%b->idleCount]);
        else if(strcmp(gatilho,"HIT")==0  && b->hitCount>0)      TriggerSpeech(b->hit[rand()%b->hitCount]);
        else if(strcmp(gatilho,"CRITICAL")==0&&b->criticalCount>0) TriggerSpeech(b->critical[rand()%b->criticalCount]);
        else if(strcmp(gatilho,"DEATH")==0 && b->deathCount>0)   TriggerSpeech(b->death[rand()%b->deathCount]);
    } else {
        EasyEnemyData *e = (sala==1||sala==2)?&easySlime:((sala==3||sala==4)?&easyOgre:&easyBoss);
        const char *nom = (sala==1||sala==2)?"Slime":((sala==3||sala==4)?"Ogro":"Boss Ogro");
        if (strcmp(gatilho,"ENTRY")==0)    { if(e->entryCount>0)    TriggerSpeech(e->entry[rand()%e->entryCount]);    else TriggerSpeech(TextFormat("%s apareceu!", nom)); }
        else if(strcmp(gatilho,"IDLE")==0) { if(e->idleCount>0)     TriggerSpeech(e->idle[rand()%e->idleCount]);      else TriggerSpeech(TextFormat("%s observa...", nom)); }
        else if(strcmp(gatilho,"HIT")==0)  { if(e->hitCount>0)      TriggerSpeech(e->hit[rand()%e->hitCount]);        else TriggerSpeech(TextFormat("%s grunhiu!", nom)); }
        else if(strcmp(gatilho,"CRITICAL")==0){if(e->criticalCount>0) TriggerSpeech(e->critical[rand()%e->criticalCount]); else TriggerSpeech(TextFormat("%s golpe critico!", nom));}
        else if(strcmp(gatilho,"DEATH")==0){ if(e->deathCount>0)    TriggerSpeech(e->death[rand()%e->deathCount]);    else TriggerSpeech(TextFormat("%s desintegrou!", nom));}
    }
}

#define MAX_LINHAS_ENIGMA 6
typedef struct { const char *linhas[MAX_LINHAS_ENIGMA]; int caminhoCerto; const char *bonusDesc; const char *penalDesc; } PathEvent;

static PathEvent pathEvents[] = {
    {{"VERDE so e seguro se VERMELHO esta monitorado","    OU se AZUL esta corrompido.","VERMELHO nao esta sendo monitorado.","AZUL nao apresenta corrupcao.","O kernel registrou AZUL como ultimo processo limpo.",NULL},2,"Bonus: Monstro inicia com -25 HP!","Penalidade: Perde 2 vidas!"},
    {{"Exatamente um caminho esta sincronizado com o clock.","Se VERMELHO esta sincronizado, entao AZUL esta travado.","AZUL nao esta travado.","Se VERDE esta sincronizado, entao VERMELHO esta isolado.","VERMELHO nao esta isolado.",NULL},2,"Bonus: +2 vidas extras!","Penalidade: Monstro ganha +20 HP!"},
    {{"Um caminho e seguro somente se NAO esta corrompido","    E NAO esta travado.","O caminho VERDE esta corrompido.","O caminho AZUL esta travado in deadlock.","O caminho VERMELHO nao esta corrompido nem travado.",NULL},0,"Bonus: Monstro inicia com -30 HP!","Penalidade: Perde 1 vida!"},
    {{"Se VERMELHO esta acessivel, entao VERDE esta acessivel.","Se VERDE esta acessivel, entao AZUL esta isolado da rede.","AZUL nao esta isolado.","Pelo menos um caminho esta acessivel no barramento.",NULL,NULL},2,"Bonus: +2 vidas extras!","Penalidade: Perde 2 vidas!"},
    {{"VERMELHO ou VERDE tem firewall ativo (ou ambos).","Se VERMELHO tem firewall, a porta de saida e bloqueada.","A porta de saida NAO esta bloqueada.","Se VERDE tem firewall, o processo entra in loop infinito.","O processo NAO esta in loop infinito.","Se nenhum tem firewall, VERDE e o gateway padrao."},1,"Bonus: Monstro inicia com -20 HP!","Penalidade: Perde 1 vida!"},
    {{"Tres processos disputam um unico bloco de memoria.","VERMELHO alocou o recurso primeiro (mutex adquirido).","O detentor do mutex nao pode ser corrompido.","VERDE e AZUL estao bloqueados aguardando o recurso.","Apenas o detentor do mutex pode ser atravessado.",NULL},0,"Bonus: +2 vidas extras!","Penalidade: Monstro ganha +15 HP!"},
    {{"Exatamente dois caminhos estao com checksum invalido.","O caminho VERMEDHO tem checksum invalido.","O caminho VERDE tem checksum invalido.","Apenas o caminho com checksum valido e seguro.",NULL,NULL},2,"Bonus: Monstro inicia com -25 HP!","Penalidade: Perde 2 vidas!"},
    {{"Se AZUL esta online, VERMELHO sofre buffer overflow.","Se VERMELHO sofre overflow, ele trava imediatamente.","VERMELHO nao esta travado.","Se VERDE esta online, AZUL e desativado.","Pelo menos um caminho esta online.",NULL},1,"Bonus: +2 vidas extras!","Penalidade: Perde 1 vida!"},
    {{"VERMELHO ou VERDE esta seguro, mas nao os dois.","Se VERDE esta seguro, entao AZUL esta corrompido.","AZUL nao esta corrompido.","Apenas o caminho seguro pode ser transitado.",NULL,NULL},0,"Bonus: Monstro inicia com -30 HP!","Penalidade: Monstro ganha +20 HP!"},
    {{"Sistema usa round-robin: um caminho ativo por vez.","VERMELHO esgotou seu quantum de CPU e foi bloqueado.","VERDE e AZUL ainda nao receberam quantum.","O escalonador prioriza o processo de menor PID.","VERDE tem PID menor que AZUL.",NULL},1,"Bonus: Monstro inicia com -20 HP!","Penalidade: Perde 2 vidas!"},
};
#define NUM_PATH_EVENTS (sizeof(pathEvents)/sizeof(pathEvents[0]))

static void CalcLetterbox(float *scale,float *offX,float *offY){
    int sw=GetScreenWidth(),sh=GetScreenHeight();
    float sx=(float)sw/VIRT_W,sy=(float)sh/VIRT_H;
    *scale=(sx<sy)?sx:sy;
    *offX=(sw-VIRT_W*(*scale))*0.5f; *offY=(sh-VIRT_H*(*scale))*0.5f;
}

static Vector2 MouseVirtual(void){
    float scale,offX,offY; CalcLetterbox(&scale,&offX,&offY);
    Vector2 m=GetMousePosition();
    return (Vector2){ (m.x-offX)/scale, (m.y-offY)/scale };
}

static int calcularDano(int dado,int alvo){
    int d=abs(dado-alvo);
    if(d==0) return 40; if(d<=2) return 25; if(d<=10) return 15;
    if(d<=20) return 10; if(d<=30) return 5; return 0;
}
static void SalvarHistorico(const char *res,int sala,int modoDificil){
    FILE *f=fopen(modoDificil?"historico_dificil.txt":"historico_facil.txt","a");
    if(!f) return;
    if(strcmp(res,"VENCEU")==0) fprintf(f,"VENCEU\n"); else fprintf(f,"%s - Sala %d\n",res,sala);
    fclose(f);
}
static int LerHistorico(const char *arq, char linhas[][100], int max) {
    FILE *f = fopen(arq, "r"); if (!f) return 0;
    int i = 0;
    while (fgets(linhas[i], 100, f) && i < max-1) {
        int l = strlen(linhas[i]);
        while (l>0 && (linhas[i][l-1]=='\n'||linhas[i][l-1]=='\r')) linhas[i][--l]='\0';
        i++;
    }
    fclose(f); return i;
}

static int ExtrairSalas(char linhas[][100], int total, int *salas, int salaMax) {
    int n = 0;
    for (int i = 0; i < total; i++) {
        if (strstr(linhas[i], "VENCEU")) { salas[n++] = salaMax; }
        else {
            int s = 0;
            if (sscanf(linhas[i], "MORREU - Sala %d", &s) == 1) salas[n++] = s;
        }
    }
    return n;
}
static void DrawTexVirt(Texture2D t,Rectangle dst,Color tint){
    DrawTexturePro(t,(Rectangle){0,0,(float)t.width,(float)t.height},dst,(Vector2){0,0},0.0f,tint);
}
static Texture2D* GetEnemyTexture(int sala,int modoDificil,
    Texture2D *slimeTex,Texture2D *ogroTex,Texture2D *bossTex,
    Texture2D *mariTex,Texture2D *romaTex,Texture2D *luisTex,
    Texture2D *micaTex,Texture2D *ruanTex,Texture2D *lucasTex,Texture2D *lucas2Tex){
    if(!modoDificil){
        if(sala == 1 || sala == 2) return slimeTex;
        if(sala == 3 || sala == 4) return ogroTex;
        return bossTex;
    }
    switch(sala){case 1:return mariTex;case 2:return romaTex;case 3:return luisTex;
    case 4:return micaTex;case 5:return ruanTex;case 6:return lucasTex;default:return lucas2Tex;}
}

static void SortearEfeitoChest(void) {
    bool isBuff = (rand() % 2 == 0);
    chestIsBuff = isBuff;
    if (isBuff) {
        int idx = rand() % NUM_BUFFS;
        ChestEffect *e = &chestBuffs[idx];
        chestVidaDelta  = e->vidasDelta;
        chestHpDelta    = e->hpDelta;
        chestMaxNDelta  = e->maxNDelta;
        chestMinNDelta  = e->minNDelta;
        snprintf(chestResultMsg, sizeof(chestResultMsg), "BUFF: %s", e->desc);
    } else {
        int idx = rand() % NUM_DEBUFFS;
        ChestEffect *e = &chestDebuffs[idx];
        chestVidaDelta  = e->vidasDelta;
        chestHpDelta    = e->hpDelta;
        chestMaxNDelta  = e->maxNDelta;
        chestMinNDelta  = e->minNDelta;
        snprintf(chestResultMsg, sizeof(chestResultMsg), "DEBUFF: %s", e->desc);
    }
}

int main(void) {
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
    Texture2D lore6 = LoadTexture("../assets/lore6.png");
    Texture2D lore7 = LoadTexture("../assets/lore7.png");
    Texture2D lore8 = LoadTexture("../assets/lore8.png");

    Texture2D transformLore1 = LoadTexture("../assets/transformlore1.png");
    Texture2D transformLore2 = LoadTexture("../assets/transformlore2.png");
    Texture2D transformLore3 = LoadTexture("../assets/transformlore3.png");
    Texture2D transformLore4 = LoadTexture("../assets/transformlore4.png");
    Texture2D transformLore5 = LoadTexture("../assets/transformlore5.png");
    Texture2D transformLore6 = LoadTexture("../assets/transformlore6.png");
    Texture2D transformLore7 = LoadTexture("../assets/transformlore7.png");
    Texture2D transformLore8 = LoadTexture("../assets/transformlore8.png");

    Texture2D loreFinal1 = LoadTexture("../assets/finaldif1.png");
    Texture2D loreFinal2 = LoadTexture("../assets/finaldif2.png");
    Texture2D loreFinal3 = LoadTexture("../assets/finaldif3.png");
    Texture2D loreFinal4 = LoadTexture("../assets/finaldif4.png");
    Texture2D loreFinal5 = LoadTexture("../assets/finaldif5.png");
    Texture2D loreFinal6 = LoadTexture("../assets/finaldif6.png");
    Texture2D loreFinal7 = LoadTexture("../assets/finaldif7.png");
    Texture2D loreFinal8 = LoadTexture("../assets/finaldif8.png");

    Texture2D dropBg    = LoadTexture("../assets/drop.png");
    Texture2D textboxTex= LoadTexture("../assets/textbox.png");

    Music menuMusic = LoadMusicStream("../assets/menu.wav");
    Music gameMusic = LoadMusicStream("../assets/soundtrack.wav");
    Sound buffSound  = LoadSound("../assets/downloadbuff.wav");
    Sound debuffSound = LoadSound("../assets/downloaddebuff.wav");

    PlayMusicStream(menuMusic);

    Texture2D *loreTextures[]          = { &lore1, &lore2, &lore3, &lore4, &lore5, &lore6, &lore7, &lore8};
    Texture2D *transformLoreTextures[] = {
        &transformLore1, &transformLore2, &transformLore3, &transformLore4,
        &transformLore5, &transformLore6, &transformLore7, &transformLore8
    };
    Texture2D *loreFinalTextures[] = { &loreFinal1,&loreFinal2,&loreFinal3,&loreFinal4,
                                       &loreFinal5,&loreFinal6,&loreFinal7,&loreFinal8 };
    #define NUM_TRANSFORM_TEXTURES 8
    #define NUM_INTRO_TEXTURES 8
    #define NUM_FINAL_TEXTURES 8

    GameState state = MENU;

    int MNumber=0,rodada=0,minN=1,maxN=100,novaRodada=0;
    int monsterHP=50, monsterMaxHP=50, vidas=5,qntOpcoes=6,sala=1;
    int resultadoSalvo=0,modoDificil=0;
    int opcoesP1[10],opcoesP2[10];
    int idxEscolhaP1=-1,idxEscolhaP2=-1;

    char mensagem[200]        = "Escolham um numero";
    char mensagemMonstro[200] = "";

    int ultimoDanoP1 = -1; 
    int ultimoDanoP2 = -1;

    int pathEventIndex=0,bonusMonsterHP=0,bonusVidas=0,penalVidas=0,penalMonsterHP=0;
    char pathResultMsg[200] = "";

    int loreScene=0,loreCharIndex=0,loreTimer=0;
    int transformLoreScene=0, transformLoreCharIndex=0, transformLoreTimer=0;
    int finalLoreScene=0,finalLoreCharIndex=0,finalLoreTimer=0;

    int pathEscolhaP1=-1,pathEscolhaP2=-1;
    int pathResultTimer=0,pathTimeLeft=0;
    int salaComPath=1;
    int deathTimer=-1;

    int chestShowTimer = 0;
    bool chestAguardandoEfeito = false;

    char historicoFacil[50][100];
    char historicoDificil[50][100];
    int  totalFacil=0,totalDificil=0;

    int   salasFacil[50],   salasDificil[50];
    int   nSalasFacil=0,    nSalasDificil=0;
    float mediaFacil=0,      mediaDificil=0;
    int   melhorFacil=0,    melhorDificil=0;
    int   piorFacil=0,      piorDificil=0;
    float desvioFacil=0,    desvioDificil=0;

    // ── Layout constants ───────────────────────────────────────────────────
    const float BTN_W=310,BTN_H=80;
    const Rectangle btnJogarRec={(VIRT_W-BTN_W)*0.0f,165,BTN_W,BTN_H};
    const Rectangle btnStatsRec={(VIRT_W-BTN_W)*0.0f,250,BTN_W,BTN_H};
    const Rectangle btnSairRec ={(VIRT_W-BTN_W)*0.0f,335,BTN_W,BTN_H};
    const Rectangle btnFacilRec  ={(VIRT_W-BTN_W)*0.5f,250,BTN_W,BTN_H};
    const Rectangle btnDificilRec={(VIRT_W-BTN_W)*0.5f,390,BTN_W,BTN_H};

    const Rectangle chestSim={(VIRT_W*0.5f)-160,430,130,55};
    const Rectangle chestNao={(VIRT_W*0.5f)+30, 430,130,55};
    const Rectangle goRec={(VIRT_W-260)*0.5f,380,260,55};
    const Rectangle goExi={(VIRT_W-260)*0.5f,460,260,55};
    const Rectangle pathRed  ={180,310,270,220};
    const Rectangle pathGreen={500,310,270,220};
    const Rectangle pathBlue ={820,310,270,220};
    const Rectangle textboxRec={351,VIRT_H-152-20,578,152};

    const int BTN6_W=65,BTN6_H=55,BTN6_GAP=10;
    const float PANEL_W=320,PANEL_H=190;
    const float P1_PANEL_X=20,         P1_PANEL_Y=VIRT_H-PANEL_H-20;
    const float P1_BTN_X=P1_PANEL_X+12, P1_BTN_TOP_Y=P1_PANEL_Y+55;
    const float P2_PANEL_X=VIRT_W-PANEL_W-20, P2_PANEL_Y=VIRT_H-PANEL_H-20;
    const float P2_BTN_X=P2_PANEL_X+12,       P2_BTN_TOP_Y=P2_PANEL_Y+55;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F11)) {
            if (IsWindowFullscreen()){ToggleFullscreen();SetWindowSize(VIRT_W,VIRT_H);}
            else{int mon=GetCurrentMonitor();SetWindowSize(GetMonitorWidth(mon),GetMonitorHeight(mon));ToggleFullscreen();}
        }

        if (state==MENU||state==STATS||state==MODE_SELECT) UpdateMusicStream(menuMusic);
        else UpdateMusicStream(gameMusic);

        Vector2 mouse = MouseVirtual();

        if (state==GAMEPLAY && bossBubble.active) {
            bossBubble.timer++;
            if (bossBubble.timer%2==0) {
                int len=strlen(bossBubble.currentText);
                if (bossBubble.charIndex<len) bossBubble.charIndex++;
            }
        }

        // ── LÓGICA ──────────────────────────────────────────────────────────

        if (state==MENU) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mouse,btnJogarRec)) state=MODE_SELECT;
                if (CheckCollisionPointRec(mouse,btnStatsRec)){
                    totalFacil   = LerHistorico("historico_facil.txt",   historicoFacil,   50);
                    totalDificil = LerHistorico("historico_dificil.txt", historicoDificil, 50);

                    nSalasFacil = ExtrairSalas(historicoFacil, totalFacil, salasFacil, 5);
                    if (nSalasFacil > 0) {
                        mediaFacil  = (float)recSoma(salasFacil, nSalasFacil) / nSalasFacil;
                        melhorFacil = recMax(salasFacil, nSalasFacil);
                        piorFacil   = recMin(salasFacil, nSalasFacil);
                        float variancia = (float)recSomaSq(salasFacil, nSalasFacil)/nSalasFacil - mediaFacil*mediaFacil;
                        desvioFacil = (variancia > 0) ? sqrtf(variancia) : 0;
                    }

                    nSalasDificil = ExtrairSalas(historicoDificil, totalDificil, salasDificil, 7);
                    if (nSalasDificil > 0) {
                        mediaDificil  = (float)recSoma(salasDificil, nSalasDificil) / nSalasDificil;
                        melhorDificil = recMax(salasDificil, nSalasDificil);
                        piorDificil   = recMin(salasDificil, nSalasDificil);
                        float variancia = (float)recSomaSq(salasDificil, nSalasDificil)/nSalasDificil - mediaDificil*mediaDificil;
                        desvioDificil = (variancia > 0) ? sqrtf(variancia) : 0;
                    }

                    state=STATS;
                }
                if (CheckCollisionPointRec(mouse,btnSairRec)) break;
            }
        }
        else if (state==MODE_SELECT) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                bool hF=CheckCollisionPointRec(mouse,btnFacilRec);
                bool hD=CheckCollisionPointRec(mouse,btnDificilRec);
                if (hF||hD) {
                    modoDificil=hD?1:0;
                    StopMusicStream(menuMusic); PlayMusicStream(gameMusic);
                    MNumber=(rand()%100)+1; printf("[DEBUG SALA 1] MNumber sorteado: %d\n", MNumber);
                    rodada=0;minN=1;maxN=100;novaRodada=1;
                    monsterHP=50; monsterMaxHP=50; vidas=50;qntOpcoes=6;sala=1; 
                    ultimoDanoP1=-1; ultimoDanoP2=-1;
                    resultadoSalvo=0;bonusMonsterHP=0;bonusVidas=0;penalVidas=0;penalMonsterHP=0;
                    idxEscolhaP1=-1;idxEscolhaP2=-1;salaComPath=1;pathTimeLeft=0;deathTimer=-1;
                    chestVidaDelta=0;chestHpDelta=0;chestMaxNDelta=0;chestMinNDelta=0;
                    chestResultMsg[0]='\0';chestShowTimer=0;chestAguardandoEfeito=false;
                    mensagemMonstro[0]='\0';
                    snprintf(mensagem,200,"Escolham um numero");
                    loreScene=0;loreCharIndex=0;loreTimer=0;
                    state=LORE;
                }
            }
        }
        else if (state==LORE) {
            loreTimer++;
            if (introLoreCount==0) {
                state=GAMEPLAY; AplicarFalaInimigo(sala,modoDificil,"ENTRY");
            } else {
                if (loreTimer%2==0) {
                    int tam=strlen(introLore[loreScene]);
                    if (loreCharIndex<tam) loreCharIndex++;
                }
                if (IsKeyPressed(KEY_ENTER)||IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    int tam=strlen(introLore[loreScene]);
                    if (loreCharIndex<tam) loreCharIndex=tam;
                    else {
                        loreScene++;
                        if (loreScene>=introLoreCount){
                            state=GAMEPLAY; bossBubble.active=false;
                            AplicarFalaInimigo(sala,modoDificil,"ENTRY");
                        } else { loreCharIndex=0;loreTimer=0; }
                    }
                }
            }
        }
        else if (state == TRANSFORM_LORE) {
            transformLoreTimer++;
            if (transformLoreCount == 0) {
                sala++; 
                minN = 1; maxN = 100; rodada = 0;
                monsterHP = 50 + (sala * 25); 
                monsterMaxHP = monsterHP;
                MNumber = (rand() % (maxN - minN + 1)) + minN;
                printf("[DEBUG SALA 7 - LUCAS 2] MNumber sorteado: %d\n", MNumber);
                qntOpcoes = 6; novaRodada = 1;
                idxEscolhaP1 = -1; idxEscolhaP2 = -1;
                ultimoDanoP1 = -1; ultimoDanoP2 = -1;
                mensagemMonstro[0] = '\0';
                snprintf(mensagem, 200, "Escolham um numero");
                state = GAMEPLAY;
                AplicarFalaInimigo(sala, modoDificil, "ENTRY");
            } else {
                if (transformLoreTimer % 2 == 0) {
                    int tam = strlen(transformLore[transformLoreScene]);
                    if (transformLoreCharIndex < tam) transformLoreCharIndex++;
                }
                if (transformLoreTimer > 10 && (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))) {
                    int tam = strlen(transformLore[transformLoreScene]);
                    if (transformLoreCharIndex < tam) {
                        transformLoreCharIndex = tam;
                    } else {
                        transformLoreScene++;
                        if (transformLoreScene >= transformLoreCount) {
                            sala++; 
                            minN = 1; maxN = 100; rodada = 0;
                            monsterHP = 50 + (sala * 25); 
                            monsterMaxHP = monsterHP;
                            MNumber = (rand() % (maxN - minN + 1)) + minN;
                            printf("[DEBUG SALA 7 - LUCAS 2] MNumber sorteado: %d\n", MNumber);
                            qntOpcoes = 6; novaRodada = 1;
                            idxEscolhaP1 = -1; idxEscolhaP2 = -1;
                            ultimoDanoP1 = -1; ultimoDanoP2 = -1;
                            mensagemMonstro[0] = '\0';
                            snprintf(mensagem, 200, "Escolham um numero");
                            state = GAMEPLAY; 
                            AplicarFalaInimigo(sala, modoDificil, "ENTRY");
                        } else {
                            transformLoreCharIndex = 0; transformLoreTimer = 0;
                        }
                    }
                }
            }
        }
        else if (state==FINAL_LORE) {
            finalLoreTimer++;
            if (finalLoreCount==0) {
                state=WIN;
            } else {
                if (finalLoreTimer%2==0) {
                    int tam=strlen(finalLore[finalLoreScene]);
                    if (finalLoreCharIndex<tam) finalLoreCharIndex++;
                }
                if (IsKeyPressed(KEY_ENTER)||IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    int tam=strlen(finalLore[finalLoreScene]);
                    if (finalLoreCharIndex<tam) finalLoreCharIndex=tam;
                    else {
                        finalLoreScene++;
                        if (finalLoreScene>=finalLoreCount){
                            state=WIN;
                        } else { finalLoreCharIndex=0;finalLoreTimer=0; }
                    }
                }
            }
        }
        else if (state==STATS) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) state=MENU;
        }
        else if (state==CHEST) {
            if (chestAguardandoEfeito) {
                chestShowTimer--;
                if (chestShowTimer<=0) {
                    chestAguardandoEfeito=false;

                    #define AVANCAR_SALA() do { \
                        vidas += chestVidaDelta; \
                        if (vidas < 1) vidas = 1; \
                        int novoMaxN = 100 + chestMaxNDelta; \
                        int novoMinN = 1 + chestMinNDelta; \
                        if (novoMaxN > 200) novoMaxN = 200; \
                        if (novoMinN < 1)   novoMinN = 1; \
                        if (novoMinN >= novoMaxN) { novoMinN = 1; novoMaxN = 100; } \
                        minN = novoMinN; \
                        maxN = novoMaxN; \
                        salaComPath = !salaComPath; \
                        if (salaComPath) {\
                            pathEventIndex  = rand() % NUM_PATH_EVENTS; \
                            const char* caminhosNomes[] = {"VERMELHO", "VERDE", "AZUL"}; \
                            printf("[DEBUG ENCRUZILHADA] Caminho Certo: %d (%s)\n", pathEvents[pathEventIndex].caminhoCerto, caminhosNomes[pathEvents[pathEventIndex].caminhoCerto]); \
                            pathResultTimer = 0; pathResultMsg[0] = '\0'; \
                            pathEscolhaP1 = -1; pathEscolhaP2 = -1; \
                            pathTimeLeft = modoDificil ? 1500 : 4000; \
                            state = PATH_CHOICE; \
                        } else { \
                            rodada = 0; \
                            MNumber = (rand() % (maxN - minN + 1)) + minN; \
                            printf("[DEBUG SALA %d] MNumber sorteado: %d\n", sala, MNumber); \
                            monsterHP = 50 + (sala * 25) + chestHpDelta; \
                            if (monsterHP < 10) monsterHP = 10; \
                            monsterMaxHP = monsterHP; \
                            qntOpcoes = 6; novaRodada = 1; \
                            idxEscolhaP1 = -1; idxEscolhaP2 = -1; \
                            ultimoDanoP1 = -1; ultimoDanoP2 = -1; \
                            mensagemMonstro[0] = '\0'; \
                            snprintf(mensagem, 200, "Escolham um numero"); \
                            chestVidaDelta = 0; chestHpDelta = 0; chestMaxNDelta = 0; chestMinNDelta = 0; \
                            state = GAMEPLAY; \
                            AplicarFalaInimigo(sala, modoDificil, "ENTRY"); \
                        } \
                    } while(0)

                    AVANCAR_SALA();
                    #undef AVANCAR_SALA
                }
            } else {
                bool p1Sim=IsKeyDown(KEY_R), p2Sim=IsKeyDown(KEY_P);
                bool p1Nao=IsKeyDown(KEY_F), p2Nao=IsKeyDown(KEY_L);
                bool baixou = (p1Sim && p2Sim);
                bool recusou= (p1Nao && p2Nao);

                if (baixou || recusou) {
                    if (baixou) {
                        SortearEfeitoChest();
                        if (chestIsBuff) PlaySound (buffSound);
                        else             PlaySound(debuffSound);
                    } else {                        
                        chestVidaDelta=0;chestHpDelta=0;chestMaxNDelta=0;chestMinNDelta=0;
                        snprintf(chestResultMsg,sizeof(chestResultMsg),"Dados ignorados. Sem efeito.");
                        chestIsBuff=true;
                    }

                    if (!modoDificil && sala >= 6) {
                        if (!resultadoSalvo){SalvarHistorico("VENCEU",sala,modoDificil);resultadoSalvo=1;}
                        state=WIN;
                    } else {
                        chestShowTimer=180;
                        chestAguardandoEfeito=true;
                    }
                }
            }
        }
        else if (state==PATH_CHOICE) {
            #define ENTRAR_GAMEPLAY() do { \
                vidas += bonusVidas; vidas -= penalVidas; \
                if(vidas < 1) vidas = 1; \
                rodada = 0; \
                MNumber = (rand() % (maxN - minN + 1)) + minN; \
                printf("[DEBUG SALA %d] MNumber sorteado: %d\n", sala, MNumber); \
                monsterHP = 50 + (sala * 25) - bonusMonsterHP + penalMonsterHP + chestHpDelta; \
                if(monsterHP < 10) monsterHP = 10; \
                bonusMonsterHP = 0; bonusVidas = 0; penalVidas = 0; penalMonsterHP = 0; \
                chestVidaDelta = 0; chestHpDelta = 0; chestMaxNDelta = 0; chestMinNDelta = 0; \
                qntOpcoes = 6; novaRodada = 1; \
                idxEscolhaP1 = -1; idxEscolhaP2 = -1; \
                ultimoDanoP1 = -1; ultimoDanoP2 = -1; \
                pathEscolhaP1 = -1; pathEscolhaP2 = -1; \
                mensagemMonstro[0] = '\0'; \
                snprintf(mensagem, 200, "Escolham um numero"); \
                state = GAMEPLAY; \
                AplicarFalaInimigo(sala, modoDificil, "ENTRY"); \
            } while(0)

            if (pathResultTimer>0){pathResultTimer--;if(pathResultTimer==0){ENTRAR_GAMEPLAY();}}
            else {
                if (pathTimeLeft>0) pathTimeLeft--;
                if (pathTimeLeft==0) {
                    PathEvent *ev2=&pathEvents[pathEventIndex];
                    snprintf(pathResultMsg,200,"TEMPO ESGOTADO! %s",ev2->penalDesc);
                    if(strstr(ev2->penalDesc,"vida")){int n=1;sscanf(ev2->penalDesc,"Penalidade: Perde %d vida",&n);penalVidas=n;penalMonsterHP=0;}
                    else{int hp=0;sscanf(ev2->penalDesc,"Penalidade: Monstro ganha +%d HP!",&hp);penalMonsterHP=hp;penalVidas=0;}
                    bonusMonsterHP=0;bonusVidas=0;pathResultTimer=180;pathTimeLeft=-1;
                }
                PathEvent *ev=&pathEvents[pathEventIndex];
                if(IsKeyPressed(KEY_Q)) pathEscolhaP1=0;
                if(IsKeyPressed(KEY_W)) pathEscolhaP1=1;
                if(IsKeyPressed(KEY_E)) pathEscolhaP1=2;
                if(IsKeyPressed(KEY_U)) pathEscolhaP2=0;
                if(IsKeyPressed(KEY_I)) pathEscolhaP2=1;
                if(IsKeyPressed(KEY_O)) pathEscolhaP2=2;
                if(pathEscolhaP1!=-1&&pathEscolhaP2!=-1&&pathEscolhaP1==pathEscolhaP2&&pathTimeLeft>0){
                    int esc=pathEscolhaP1; pathTimeLeft=-1;
                    if(esc==ev->caminhoCerto){
                        snprintf(pathResultMsg,200,"CAMINHO CERTO! %s",ev->bonusDesc);
                        if(strstr(ev->bonusDesc,"vida")){int n=2;sscanf(ev->bonusDesc,"Bonus: +%d vida",&n);bonusVidas=n;bonusMonsterHP=0;}
                        else{int hp=0;sscanf(ev->bonusDesc,"Bonus: Monstro inicia com -%d HP!",&hp);bonusMonsterHP=hp;bonusVidas=0;}
                        penalVidas=0;penalMonsterHP=0;
                    } else {
                        snprintf(pathResultMsg,200,"CAMINHO ERRADO! %s",ev->penalDesc);
                        if(strstr(ev->penalDesc,"vida")){int n=1;sscanf(ev->penalDesc,"Penalidade: Perde %d vida",&n);penalVidas=n;penalMonsterHP=0;}
                        else{int hp=0;sscanf(ev->penalDesc,"Penalidade: Monstro ganha +%d HP!",&hp);penalMonsterHP=hp;penalVidas=0;}
                        bonusMonsterHP=0;bonusVidas=0;
                    }
                    pathResultTimer=180;
                }
            }
            #undef ENTRAR_GAMEPLAY
        }
        else if (state==GAMEPLAY) {
            if (monsterHP <= 0) {
                if (deathTimer > 0) deathTimer--;
                if (deathTimer == 0) {
                    if (!modoDificil && sala == 5) {
                        sala++;
                        if (!resultadoSalvo) { SalvarHistorico("VENCEU", sala, modoDificil); resultadoSalvo = 1; }
                        state = WIN;
                    }  
                    else if (modoDificil && sala == 6) {
                        transformLoreScene = 0; transformLoreCharIndex = 0; transformLoreTimer = 0;
                        bossBubble.active = false;
                        state = TRANSFORM_LORE;
                    }  
                    else if (modoDificil && sala == 7) {
                        if (!resultadoSalvo) { SalvarHistorico("VENCEU", sala, modoDificil); resultadoSalvo = 1; }
                        finalLoreScene = 0; finalLoreCharIndex = 0; finalLoreTimer = 0;
                        bossBubble.active = false;
                        state = FINAL_LORE; 
                    }  
                    else {
                        sala++;
                        minN = 1; maxN = 100;
                        state = CHEST;
                    }
                }
            } else {
                if (novaRodada){
                    rodada++;
                    for(int i=0;i<6;i++){opcoesP1[i]=(rand()%(maxN-minN+1))+minN;opcoesP2[i]=(rand()%(maxN-minN+1))+minN;}
                    idxEscolhaP1=-1;idxEscolhaP2=-1;novaRodada=0;
                }
                int teclasP1[6]={KEY_Q,KEY_W,KEY_E,KEY_A,KEY_S,KEY_D};
                for(int i=0;i<6;i++) if(IsKeyPressed(teclasP1[i])) idxEscolhaP1=i;
                static int confirmadoP1=0;
                if(idxEscolhaP1!=-1&&IsKeyPressed(KEY_R)) confirmadoP1=1;
                if(IsKeyPressed(KEY_F)){confirmadoP1=0;idxEscolhaP1=-1;}

                int teclasP2[6]={KEY_U,KEY_I,KEY_O,KEY_H,KEY_J,KEY_K};
                for(int i=0;i<6;i++) if(IsKeyPressed(teclasP2[i])) idxEscolhaP2=i;
                static int confirmadoP2=0;
                if(idxEscolhaP2!=-1&&IsKeyPressed(KEY_P)) confirmadoP2=1;
                if(IsKeyPressed(KEY_L)){confirmadoP2=0;idxEscolhaP2=-1;}

                if(confirmadoP1&&confirmadoP2){
                    int eP1=opcoesP1[idxEscolhaP1],eP2=opcoesP2[idxEscolhaP2];
                    
                    ultimoDanoP1=calcularDano(eP1,MNumber);
                    ultimoDanoP2=calcularDano(eP2,MNumber);
                    int dT=ultimoDanoP1+ultimoDanoP2;
                    
                    monsterHP-=dT; if(monsterHP<0)monsterHP=0;
                    
                    mensagemMonstro[0]='\0'; 
                    
                    char dica[100]="";
                    if(!modoDificil){
                        int med=(eP1+eP2)/2;
                        if(med<MNumber){minN=med+1;snprintf(dica,100,"Numero maior que %d",med);}
                        else if(med>MNumber){maxN=med-1;snprintf(dica,100,"Numero menor que %d",med);}
                    }
                    
                    if(modoDificil) 
                        snprintf(mensagem,200,"Ataque Combinado! Total de dano: %d",dT);
                    else 
                        snprintf(mensagem,200,"Total: %d | %s",dT,dica);
                        
                    if(monsterHP>0){
                        if(ultimoDanoP1==40||ultimoDanoP2==40)AplicarFalaInimigo(sala,modoDificil,"CRITICAL");
                        else if(dT>10)AplicarFalaInimigo(sala,modoDificil,"HIT");
                        else AplicarFalaInimigo(sala,modoDificil,"IDLE");
                    }
                    else{AplicarFalaInimigo(sala,modoDificil,"DEATH");deathTimer=180;}
                    
                    confirmadoP1=0;confirmadoP2=0;novaRodada=1;
                }
                
                if(rodada%5==0&&novaRodada==0){
                    vidas--;
                    rodada++;
                    snprintf(mensagemMonstro,200,">>> O monstro atacou! -1 vida! <<<");
                }
                if(vidas<=0){if(!resultadoSalvo){SalvarHistorico("MORREU",sala,modoDificil);resultadoSalvo=1;}state=GAMEOVER;}
            }
        }
        else if (state==GAMEOVER) {
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                if(CheckCollisionPointRec(mouse,goRec)){
                    StopMusicStream(gameMusic);
                    PlayMusicStream(menuMusic);
                    state=MENU;
                }
                if(CheckCollisionPointRec(mouse,goExi)) break;
            }
        }
        else if (state==WIN) {
            if(!resultadoSalvo){SalvarHistorico("VENCEU",sala,modoDificil);resultadoSalvo=1;}
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                StopMusicStream(gameMusic);
                PlayMusicStream(menuMusic);
                state=MENU;
            }
        }

        // ── DRAW ───────────────────────────────────────────────────────────
        BeginTextureMode(canvas);
        ClearBackground(BLACK);

        if (state==MENU) {
            bool hJ=CheckCollisionPointRec(mouse,btnJogarRec);
            bool hS=CheckCollisionPointRec(mouse,btnStatsRec);
            bool hX=CheckCollisionPointRec(mouse,btnSairRec);
            DrawTexVirt(menuBg,(Rectangle){0,0,VIRT_W,VIRT_H},WHITE);
            DrawTexVirt(btnNovoJogo,btnJogarRec,hJ?LIGHTGRAY:WHITE);
            DrawTexVirt(btnStats,btnStatsRec,hS?LIGHTGRAY:WHITE);
            DrawTexVirt(btnSair,btnSairRec,hX?LIGHTGRAY:WHITE);
        }
        else if (state==MODE_SELECT) {
            bool hF=CheckCollisionPointRec(mouse,btnFacilRec);
            bool hD=CheckCollisionPointRec(mouse,btnDificilRec);
            DrawTexVirt(menuBg,(Rectangle){0,0,VIRT_W,VIRT_H},WHITE);
            const char *titulo="ESCOLHA O MODO";
            int tx = (VIRT_W - MeasureText(titulo,46)) / 2;
            DrawRectangle(tx - 20, 163, MeasureText(titulo,46) + 40, 58, (Color){0,0,0,170}); 
            DrawText(titulo, tx+2, 172, 46, (Color){0,0,0,200});  
            DrawText(titulo, tx, 170, 46, WHITE);
            DrawTexVirt(btnFacil,btnFacilRec,hF?LIGHTGRAY:WHITE);
            DrawTexVirt(btnDificil,btnDificilRec,hD?LIGHTGRAY:WHITE);
        }
        else if (state==LORE) {
            int texIdx = loreScene;
            if (texIdx >= NUM_INTRO_TEXTURES) texIdx = NUM_INTRO_TEXTURES - 1;
            Texture2D *bg = loreTextures[texIdx];
            DrawTexturePro(*bg,(Rectangle){0,0,(float)bg->width,(float)bg->height},(Rectangle){0,0,VIRT_W,VIRT_H},(Vector2){0,0},0,WHITE);
            DrawRectangle(0,VIRT_H-170,VIRT_W,170,(Color){0,0,0,180});
            char vis[512]={0};
            if(loreScene<introLoreCount){strncpy(vis,introLore[loreScene],loreCharIndex);vis[loreCharIndex]='\0';}
            DrawText(vis,63,VIRT_H-127,30,BLACK);
            DrawText(vis,60,VIRT_H-130,30,WHITE);
            DrawText("ENTER ou clique para continuar",VIRT_W-420,VIRT_H-40,22,LIGHTGRAY);
        }
        else if (state==TRANSFORM_LORE) {
            int texIdx = transformLoreScene;
            if (texIdx >= NUM_TRANSFORM_TEXTURES) texIdx = NUM_TRANSFORM_TEXTURES-1;
            Texture2D *bg = transformLoreTextures[texIdx];
            DrawTexturePro(*bg,(Rectangle){0,0,(float)bg->width,(float)bg->height},(Rectangle){0,0,VIRT_W,VIRT_H},(Vector2){0,0},0,WHITE);
            DrawRectangle(0,VIRT_H-170,VIRT_W,170,(Color){0,0,0,180});
            char vis[512]={0};
            if(transformLoreScene<transformLoreCount){
                strncpy(vis,transformLore[transformLoreScene],transformLoreCharIndex);
                vis[transformLoreCharIndex]='\0';
            }
            DrawText(vis,63,VIRT_H-127,30,BLACK);
            DrawText(vis,60,VIRT_H-130,30,WHITE);
            DrawText("ENTER ou clique para continuar",VIRT_W-420,VIRT_H-40,22,LIGHTGRAY);
        }
        else if (state==FINAL_LORE) {
            int texIdx = finalLoreScene;
            if (texIdx >= NUM_FINAL_TEXTURES) texIdx = NUM_FINAL_TEXTURES - 1;
            Texture2D *bg = loreFinalTextures[texIdx];
            DrawTexturePro(*bg,(Rectangle){0,0,(float)bg->width,(float)bg->height},(Rectangle){0,0,VIRT_W,VIRT_H},(Vector2){0,0},0,WHITE);
            DrawRectangle(0,VIRT_H-170,VIRT_W,170,(Color){0,0,0,180});
            char vis[512]={0};
            if(finalLoreScene<finalLoreCount){strncpy(vis,finalLore[finalLoreScene],finalLoreCharIndex);vis[finalLoreCharIndex]='\0';}
            DrawText(vis,63,VIRT_H-127,30,BLACK);
            DrawText(vis,60,VIRT_H-130,30,WHITE);
            DrawText("ENTER ou clique para continuar",VIRT_W-420,VIRT_H-40,22,LIGHTGRAY);
        }
        else if (state==STATS) {
            DrawTexVirt(menuBg,(Rectangle){0,0,VIRT_W,VIRT_H},WHITE);
            DrawRectangle(0,0,VIRT_W,VIRT_H,(Color){0,0,0,160});

            const char *titulo="RELATORIO ANALITICO";
            DrawText(titulo,(VIRT_W-MeasureText(titulo,36))/2,18,36,WHITE);
            DrawLine(60,62,VIRT_W-60,62,GRAY);

            float cx = VIRT_W * 0.5f;
            DrawLine((int)cx, 70, (int)cx, 660, (Color){80,80,80,200});

            DrawText("FACIL",   (int)(cx*0.5f - MeasureText("FACIL",  26)*0.5f), 72, 26, DARKGREEN);
            DrawText("DIFICIL", (int)(cx+cx*0.5f - MeasureText("DIFICIL",26)*0.5f), 72, 26, MAROON);
            DrawLine(60,104,VIRT_W-60,104,(Color){60,60,60,200});

            int lx = 80, rx = (int)cx+30, ly = 115, ry = 115, gap = 28;
            if (nSalasFacil > 0) {
                DrawText(TextFormat("Partidas:  %d",   nSalasFacil),             lx, ly,      22, LIGHTGRAY); ly+=gap;
                DrawText(TextFormat("Media:     %.2f", mediaFacil),              lx, ly,      22, WHITE);      ly+=gap;
                DrawText(TextFormat("Melhor:    Sala %d", melhorFacil),          lx, ly,      22, GREEN);      ly+=gap;
                DrawText(TextFormat("Pior:      Sala %d", piorFacil),            lx, ly,      22, RED);        ly+=gap;
                DrawText(TextFormat("Desvio:    %.2f", desvioFacil),             lx, ly,      22, YELLOW);     ly+=gap+6;
                DrawLine(lx, ly, (int)cx-30, ly, (Color){60,60,60,180}); ly+=10;
                const char *hF = GerarHeuristica(mediaFacil,melhorFacil,piorFacil,desvioFacil,5);
                DrawText("Analise:", lx, ly, 18, DARKGREEN); ly+=24;
                DrawText(hF, lx, ly, 16, (Color){180,255,180,255}); ly+=28;
                DrawLine(lx, ly, (int)cx-30, ly, (Color){60,60,60,180}); ly+=12;
                DrawText("Historico:", lx, ly, 18, LIGHTGRAY); ly+=24;
                for (int i=0; i<totalFacil && ly<660; i++) {
                    Color c = strstr(historicoFacil[i],"VENCEU") ? GREEN : RED;
                    DrawText(historicoFacil[i], lx, ly, 16, c); ly+=20;
                }
            } else {
                DrawText("Sem partidas registradas.", lx, ly, 18, GRAY);
            }

            if (nSalasDificil > 0) {
                DrawText(TextFormat("Partidas:  %d",   nSalasDificil),           rx, ry,      22, LIGHTGRAY); ry+=gap;
                DrawText(TextFormat("Media:     %.2f", mediaDificil),            rx, ry,      22, WHITE);      ry+=gap;
                DrawText(TextFormat("Melhor:    Sala %d", melhorDificil),        rx, ry,      22, GREEN);      ry+=gap;
                DrawText(TextFormat("Pior:      Sala %d", piorDificil),          rx, ry,      22, RED);        ry+=gap;
                DrawText(TextFormat("Desvio:    %.2f", desvioDificil),           rx, ry,      22, YELLOW);     ry+=gap+6;
                DrawLine(rx, ry, VIRT_W-60, ry, (Color){60,60,60,180}); ry+=10;
                const char *hD = GerarHeuristica(mediaDificil,melhorDificil,piorDificil,desvioDificil,7);
                DrawText("Analise:", rx, ry, 18, MAROON); ry+=24;
                DrawText(hD, rx, ry, 16, (Color){255,180,180,255}); ry+=28;
                DrawLine(rx, ry, VIRT_W-60, ry, (Color){60,60,60,180}); ry+=12;
                DrawText("Historico:", rx, ry, 18, LIGHTGRAY); ry+=24;
                for (int i=0; i<totalDificil && ry<660; i++) {
                    Color c = strstr(historicoDificil[i],"VENCEU") ? GREEN : RED;
                    DrawText(historicoDificil[i], rx, ry, 16, c); ry+=20;
                }
            } else {
                DrawText("Sem partidas registradas.", rx, ry, 18, GRAY);
            }

            const char *v="Clique para voltar";
            DrawText(v,(VIRT_W-MeasureText(v,20))/2,VIRT_H-36,20,GRAY);
        }
        else if (state==CHEST) {
            bool p1Sim=IsKeyDown(KEY_R),p2Sim=IsKeyDown(KEY_P);
            bool p1Nao=IsKeyDown(KEY_F),p2Nao=IsKeyDown(KEY_L);

            DrawTexturePro(dropBg,(Rectangle){0,0,(float)dropBg.width,(float)dropBg.height},(Rectangle){0,0,VIRT_W,VIRT_H},(Vector2){0,0},0,WHITE);

            if (chestAguardandoEfeito) {
                Color cor = chestIsBuff ? (Color){80,255,100,255} : (Color){255,80,80,255};
                const char *prefixo = chestIsBuff ? "DADOS BAIXADOS!" : "DADOS CORROMPIDOS!";
                DrawRectangle((VIRT_W-600)/2, VIRT_H/2-80, 600, 160, (Color){0,0,0,220});
                DrawRectangleLinesEx((Rectangle){(float)(VIRT_W-600)/2,(float)(VIRT_H/2-80),600,160},3,cor);
                DrawText(prefixo,(VIRT_W-MeasureText(prefixo,32))/2,VIRT_H/2-60,32,cor);
                DrawText(chestResultMsg,(VIRT_W-MeasureText(chestResultMsg,26))/2,VIRT_H/2-10,26,WHITE);
                float prog=(float)chestShowTimer/180.0f;
                DrawRectangle((VIRT_W-400)/2,VIRT_H/2+58,400,10,(Color){40,40,40,200});
                DrawRectangle((VIRT_W-400)/2,VIRT_H/2+58,(int)(400*prog),10,cor);
            } else {
                const char *t1="Voce derrotou o monstro";
                const char *t2="e ele deixou cair seus dados";
                const char *t3="Deseja baixar os dados? (Requer sincronia)";
                DrawText(t1,((VIRT_W-MeasureText(t1,38))/2)+3,183,38,BLACK);
                DrawText(t2,((VIRT_W-MeasureText(t2,32))/2)+3,243,32,BLACK);
                DrawText(t3,((VIRT_W-MeasureText(t3,28))/2)+3,323,28,BLACK);
                DrawText(t1,(VIRT_W-MeasureText(t1,38))/2,180,38,WHITE);
                DrawText(t2,(VIRT_W-MeasureText(t2,32))/2,240,32,WHITE);
                DrawText(t3,(VIRT_W-MeasureText(t3,28))/2,320,28,WHITE);

                Color simColor=(p1Sim&&p2Sim)?GREEN:((p1Sim||p2Sim)?DARKGREEN:DARKGRAY);
                Color naoColor=(p1Nao&&p2Nao)?RED:((p1Nao||p2Nao)?MAROON:DARKGRAY);
                DrawRectangleRec(chestSim,simColor);
                DrawRectangleRec(chestNao,naoColor);
                DrawText("SIM (R + P)",(int)(chestSim.x+(chestSim.width-MeasureText("SIM (R + P)",18))*0.5f),(int)(chestSim.y+18),18,WHITE);
                DrawText("NAO (F + L)",(int)(chestNao.x+(chestNao.width-MeasureText("NAO (F + L)",18))*0.5f),(int)(chestNao.y+18),18,WHITE);
                const char *d="[ P1 e P2 devem segurar as respectivas teclas juntas para prosseguir ]";
                DrawText(d,(VIRT_W-MeasureText(d,16))/2,VIRT_H-120,16,LIGHTGRAY);
            }
        }
        else if (state==PATH_CHOICE) {
            PathEvent *ev=&pathEvents[pathEventIndex];
            bool canClick=(pathResultTimer==0);
            bool hR=canClick&&CheckCollisionPointRec(mouse,pathRed);
            bool hG=canClick&&CheckCollisionPointRec(mouse,pathGreen);
            bool hB=canClick&&CheckCollisionPointRec(mouse,pathBlue);

            DrawTexturePro(circuitBg,(Rectangle){0,0,(float)circuitBg.width,(float)circuitBg.height},(Rectangle){0,0,VIRT_W,VIRT_H},(Vector2){0,0},0,WHITE);
            DrawRectangle(0,0,VIRT_W,VIRT_H,(Color){0,0,0,130});

            const char *titulo=">> ENCRUZILHADA DO SISTEMA <<";
            DrawText(titulo,(VIRT_W-MeasureText(titulo,30))/2,12,30,(Color){0,220,80,255});
            DrawLine(60,56,VIRT_W-60,56,(Color){0,180,60,120});

            float panelX=60,panelW=VIRT_W-120;
            DrawRectangle((int)panelX,64,(int)panelW,210,(Color){0,0,0,190});
            DrawRectangleLinesEx((Rectangle){panelX,64,panelW,210},1,(Color){0,180,60,160});
            DrawText("[KERNEL LOG] Analisando rotas de acesso...",(int)panelX+14,72,17,(Color){0,160,50,255});
            DrawLine((int)panelX+1,94,(int)(panelX+panelW-1),94,(Color){0,100,30,200});
            int ly=102;
            for(int li=0;li<MAX_LINHAS_ENIGMA;li++){
                if(!ev->linhas[li]) break;
                Color lc=(li==0)?(Color){240,220,60,255}:(Color){200,220,200,255};
                DrawText(ev->linhas[li],(int)panelX+14,ly,19,lc); ly+=26;
            }
            DrawLine(60,278,VIRT_W-60,278,(Color){0,180,60,120});
            const char *inst="[ Analise os logs e escolha o caminho seguro ]";
            DrawText(inst,(VIRT_W-MeasureText(inst,16))/2,284,16,(Color){0,140,50,200});

            {
                int totalFrames = modoDificil ? 1500 : 4000;
                int framesLeft  = (pathTimeLeft>0)?pathTimeLeft:0;
                int segundos    = (framesLeft+59)/60;
                float prog      = (totalFrames>0)?(float)framesLeft/totalFrames:0.0f;
                Color tc        = (segundos<=5)?RED:(segundos<=10?ORANGE:(Color){0,220,80,255});
                char tb[32]; snprintf(tb,32,"%02d s",segundos);
                DrawRectangle(60,286,VIRT_W-120,10,(Color){40,40,40,200});
                DrawRectangle(60,286,(int)((VIRT_W-120)*prog),10,tc);
                DrawText(tb,VIRT_W-MeasureText(tb,18)-65,280,18,tc);
            }

            if(!modoDificil){
                const char *nomes[]={"VERMELHO","VERDE","AZUL"};
                if(pathEscolhaP1!=-1){char b[64];snprintf(b,64,"P1: %s",nomes[pathEscolhaP1]);DrawRectangle(30,306,MeasureText(b,18)+16,26,(Color){0,0,0,180});DrawText(b,38,310,18,(Color){100,210,255,255});}
                else{DrawRectangle(30,306,130,26,(Color){0,0,0,100});DrawText("P1: ???",38,310,18,(Color){120,120,120,200});}
                if(pathEscolhaP2!=-1){char b[64];snprintf(b,64,"P2: %s",nomes[pathEscolhaP2]);int bw=MeasureText(b,18)+16;DrawRectangle(VIRT_W-30-bw,306,bw,26,(Color){0,0,0,180});DrawText(b,VIRT_W-30-bw+8,310,18,(Color){255,150,150,255});}
                else{int bw=MeasureText("P2: ???",18)+16;DrawRectangle(VIRT_W-30-bw,306,bw,26,(Color){0,0,0,100});DrawText("P2: ???",VIRT_W-30-bw+8,310,18,(Color){120,120,120,200});}
            }

            {Color bg=hR?(Color){255,110,110,255}:(Color){160,20,20,255};DrawRectangleRec(pathRed,bg);DrawRectangleLinesEx(pathRed,hR?4:2,hR?WHITE:(Color){255,120,120,255});DrawText("[PROC: 0x52]",(int)(pathRed.x+(pathRed.width-MeasureText("[PROC: 0x52]",14))*0.5f),(int)(pathRed.y+12),14,(Color){255,180,180,200});const char*lbl="VERMELHO";DrawText(lbl,(int)(pathRed.x+(pathRed.width-MeasureText(lbl,22))*0.5f),(int)(pathRed.y+pathRed.height*0.5f-10),22,WHITE);if(!modoDificil&&(pathEscolhaP1==0||pathEscolhaP2==0))DrawText("[SELECIONADO]",(int)(pathRed.x+(pathRed.width-MeasureText("[SELECIONADO]",15))*0.5f),(int)(pathRed.y+pathRed.height-28),15,(Color){255,220,220,255});}
            {Color bg=hG?(Color){80,255,100,255}:(Color){10,130,40,255};DrawRectangleRec(pathGreen,bg);DrawRectangleLinesEx(pathGreen,hG?4:2,hG?WHITE:(Color){80,200,100,255});DrawText("[PROC: 0x47]",(int)(pathGreen.x+(pathGreen.width-MeasureText("[PROC: 0x47]",14))*0.5f),(int)(pathGreen.y+12),14,(Color){180,255,190,200});const char*lbl="VERDE";DrawText(lbl,(int)(pathGreen.x+(pathGreen.width-MeasureText(lbl,22))*0.5f),(int)(pathGreen.y+pathGreen.height*0.5f-10),22,WHITE);if(!modoDificil&&(pathEscolhaP1==1||pathEscolhaP2==1))DrawText("[SELECIONADO]",(int)(pathGreen.x+(pathGreen.width-MeasureText("[SELECIONADO]",15))*0.5f),(int)(pathGreen.y+pathGreen.height-28),15,(Color){220,255,220,255});}
            {Color bg=hB?(Color){80,160,255,255}:(Color){15,40,160,255};DrawRectangleRec(pathBlue,bg);DrawRectangleLinesEx(pathBlue,hB?4:2,hB?WHITE:(Color){80,120,255,255});DrawText("[PROC: 0x42]",(int)(pathBlue.x+(pathBlue.width-MeasureText("[PROC: 0x42]",14))*0.5f),(int)(pathBlue.y+12),14,(Color){160,190,255,200});const char*lbl="AZUL";DrawText(lbl,(int)(pathBlue.x+(pathBlue.width-MeasureText(lbl,22))*0.5f),(int)(pathBlue.y+pathBlue.height*0.5f-10),22,WHITE);if(!modoDificil&&(pathEscolhaP1==2||pathEscolhaP2==2))DrawText("[SELECIONADO]",(int)(pathBlue.x+(pathBlue.width-MeasureText("[SELECIONADO]",15))*0.5f),(int)(pathBlue.y+pathBlue.height-28),15,(Color){200,210,255,255});}

            if(pathResultTimer>0){
                DrawRectangle(60,560,VIRT_W-120,75,(Color){0,0,0,230});
                DrawRectangleLinesEx((Rectangle){60,560,VIRT_W-120,75},2,(Color){0,220,80,255});
                bool ac=(strstr(pathResultMsg,"CERTO")!=NULL);
                char fm[256]; snprintf(fm,256,"%s %s",ac?"[OK] ":"[ERR]",pathResultMsg);
                DrawText(fm,(VIRT_W-MeasureText(fm,20))/2,575,20,ac?(Color){80,255,100,255}:(Color){255,80,80,255});
                float pg=(float)pathResultTimer/180.0f;
                DrawRectangle(62,622,(int)((VIRT_W-124)*pg),10,ac?(Color){0,200,60,255}:(Color){200,50,50,255});
            } else {
                const char *h="[ Clique ou use teclas para prosseguir ]";
                DrawText(h,(VIRT_W-MeasureText(h,15))/2,648,15,(Color){0,100,40,180});
            }
        }
        else if (state==GAMEPLAY) {
            DrawTexVirt(gameBg,(Rectangle){0,0,VIRT_W,VIRT_H},WHITE);

            Texture2D *enemyTex=GetEnemyTexture(sala,modoDificil,&slimeTex,&ogroTex,&bossTex,&mariTex,&romaTex,&luisTex,&micaTex,&ruanTex,&lucasTex,&lucas2Tex);
            float sprH = 260.0f;
            float sprW = (enemyTex->height > 0) ? ((float)enemyTex->width / enemyTex->height) * sprH : sprH;
            Rectangle spr2_enemy = { (VIRT_W - sprW) * 0.5f, (VIRT_H - sprH) * 0.5f + 25, sprW, sprH };

            DrawTexturePro(*enemyTex,(Rectangle){0,0,(float)enemyTex->width,(float)enemyTex->height},spr2_enemy,(Vector2){0,0},0,(monsterHP<=0)?RED:WHITE);

            DrawRectangle(0,0,VIRT_W,95,(Color){0,0,0,210});
            DrawText(TextFormat("MISSAO %d",sala),20,10,32,(Color){0,255,120,255});
            if(modoDificil){
                const char*bn=(strlen(bosses[sala-1].name)>0)?bosses[sala-1].name:"Chefe Desconhecido";
                const char*bd=(strlen(bosses[sala-1].desc)>0)?bosses[sala-1].desc:"Seguranca corrompida detectada.";
                DrawText(bn,20,48,22,WHITE);
                DrawText(bd,20+MeasureText(bn,22)+20,52,16,(Color){190,190,190,255});
            } else {
                const char*ebn=(sala==5)?"Boss Ogro":((sala==3||sala==4)?"Ogro":"Slime");
                DrawText(ebn,20,48,24,WHITE);
                DrawText("Uma criatura bloqueia o caminho.",200,52,16,LIGHTGRAY);
            }
            const char*modoTxt=modoDificil?"MODO DIFICIL":"MODO FACIL";
            DrawText(modoTxt,VIRT_W-MeasureText(modoTxt,18)-14,10,18,modoDificil?RED:(Color){0,220,80,255});
            DrawText(TextFormat("VIDAS: %d",vidas),VIRT_W-MeasureText(TextFormat("VIDAS: %d",vidas),18)-14,36,18,RED);
            DrawText(TextFormat("HP: %d",monsterHP),VIRT_W-MeasureText(TextFormat("HP: %d",monsterHP),18)-14,58,18,GREEN);
            {
                float hpPct = (monsterMaxHP > 0) ? (float)monsterHP / monsterMaxHP : 0.0f;
                if (hpPct < 0.0f) hpPct = 0.0f;
                if (hpPct > 1.0f) hpPct = 1.0f;
                Color hpColor = (hpPct > 0.6f) ? GREEN : (hpPct > 0.3f) ? ORANGE : RED;
                int barW = 200, barH = 14;
                int barX = VIRT_W - barW - 14;
                int barY = 78;
                DrawRectangle(barX, barY, barW, barH, (Color){40,40,40,220});  
                DrawRectangle(barX, barY, (int)(barW * hpPct), barH, hpColor);          
                DrawRectangleLinesEx((Rectangle){(float)barX, (float)barY, (float)barW, (float)barH}, 1, DARKGRAY); 
            }
            DrawRectangle(300,100,680,118,(Color){0,0,0,170});
            DrawText(mensagem,300+(680-MeasureText(mensagem,20))/2,115,20,SKYBLUE);
            
            if(mensagemMonstro[0]!='\0') DrawText(mensagemMonstro,300+(680-MeasureText(mensagemMonstro,18))/2,150,18,RED);

            if(bossBubble.active){
                DrawTexVirt(textboxTex,textboxRec,WHITE);
                char tb[256]={0}; strncpy(tb,bossBubble.currentText,bossBubble.charIndex); tb[bossBubble.charIndex]='\0';
                const char*en=modoDificil?((strlen(bosses[sala-1].name)>0)?bosses[sala-1].name:"Boss"):((sala==5)?"Boss Ogro":((sala==3||sala==4)?"Ogro":"Slime"));
                DrawText(en,textboxRec.x+25,textboxRec.y+18,18,GOLD);
                DrawText(tb,textboxRec.x+25,textboxRec.y+50,18,WHITE);
            }

            // Painel P1
            DrawRectangleRec((Rectangle){P1_PANEL_X,P1_PANEL_Y,PANEL_W,PANEL_H},(Color){0,0,0,180});
            DrawRectangleLinesEx((Rectangle){P1_PANEL_X,P1_PANEL_Y,PANEL_W,PANEL_H},2,(Color){100,180,255,120});
            DrawText("PLAYER 1",P1_PANEL_X+(PANEL_W-MeasureText("PLAYER 1",18))*0.5f,P1_PANEL_Y+8,18,(Color){100,180,255,255});
            if (ultimoDanoP1 >= 0) {
                const char* p1DanoTxt = TextFormat("Ultimo Dano: %d", ultimoDanoP1);
                DrawText(p1DanoTxt, P1_PANEL_X+(PANEL_W-MeasureText(p1DanoTxt,12))*0.5f, P1_PANEL_Y+26, 12, ORANGE);
            } else {
                DrawText("Teclas: QWER / ASDF",P1_PANEL_X+(PANEL_W-MeasureText("Teclas: QWER / ASDF",12))*0.5f,P1_PANEL_Y+26,12,(Color){160,160,160,200});
            }
            for(int i=0;i<8;i++){
                int col=(i<=3)?i:i-4,row=(i<=3)?0:1;
                Rectangle btn={P1_BTN_X+col*(BTN6_W+BTN6_GAP),P1_BTN_TOP_Y+row*(BTN6_H+BTN6_GAP),(float)BTN6_W,(float)BTN6_H};
                if(i==0||i==1||i==2||i==4||i==5||i==6){
                    int oi=(i<=2)?i:i-1;bool sel=(idxEscolhaP1==oi);
                    DrawRectangleRec(btn,sel?ORANGE:(Color){50,50,80,220});DrawRectangleLinesEx(btn,sel?3:1,sel?WHITE:(Color){100,100,140,200});
                    const char*kl[]={"Q","W","E","A","S","D"};DrawText(kl[oi],(int)(btn.x+5),(int)(btn.y+4),11,WHITE);
                    const char*ns=TextFormat("%d",opcoesP1[oi]);DrawText(ns,(int)(btn.x+(BTN6_W-MeasureText(ns,20))*0.5f),(int)(btn.y+(BTN6_H-20)*0.5f),20,WHITE);
                }else if(i==3){DrawRectangleRec(btn,DARKGREEN);DrawRectangleLinesEx(btn,2,GREEN);DrawText("CONF",(int)(btn.x+6),(int)(btn.y+18),16,WHITE);DrawText("R",(int)(btn.x+BTN6_W-18),(int)(btn.y+4),14,YELLOW);}
                else{DrawRectangleRec(btn,MAROON);DrawRectangleLinesEx(btn,2,RED);DrawText("CANC",(int)(btn.x+6),(int)(btn.y+18),16,WHITE);DrawText("F",(int)(btn.x+BTN6_W-18),(int)(btn.y+4),14,YELLOW);}
            }

            // Painel P2
            DrawRectangleRec((Rectangle){P2_PANEL_X,P2_PANEL_Y,PANEL_W,PANEL_H},(Color){0,0,0,180});
            DrawRectangleLinesEx((Rectangle){P2_PANEL_X,P2_PANEL_Y,PANEL_W,PANEL_H},2,(Color){255,120,120,120});
            DrawText("PLAYER 2",P2_PANEL_X+(PANEL_W-MeasureText("PLAYER 2",18))*0.5f,P2_PANEL_Y+8,18,(Color){255,120,120,255});
            if (ultimoDanoP2 >= 0) {
                const char* p2DanoTxt = TextFormat("Ultimo Dano: %d", ultimoDanoP2);
                DrawText(p2DanoTxt, P2_PANEL_X+(PANEL_W-MeasureText(p2DanoTxt,12))*0.5f, P2_PANEL_Y+26, 12, MAGENTA);
            } else {
                DrawText("Teclas: UIOP / HJKL",P2_PANEL_X+(PANEL_W-MeasureText("Teclas: UIOP / HJKL",12))*0.5f,P2_PANEL_Y+26,12,(Color){160,160,160,200});
            }
            for(int i=0;i<8;i++){
                int col=(i<=3)?i:i-4,row=(i<=3)?0:1;
                Rectangle btn={P2_BTN_X+col*(BTN6_W+BTN6_GAP),P2_BTN_TOP_Y+row*(BTN6_H+BTN6_GAP),(float)BTN6_W,(float)BTN6_H};
                if(i==0||i==1||i==2||i==4||i==5||i==6){
                    int oi=(i<=2)?i:i-1;bool sel=(idxEscolhaP2==oi);
                    DrawRectangleRec(btn,sel?MAGENTA:(Color){80,20,50,220});DrawRectangleLinesEx(btn,sel?3:1,sel?WHITE:(Color){140,60,100,200});
                    const char*kl[]={"U","I","O","H","J","K"};DrawText(kl[oi],(int)(btn.x+5),(int)(btn.y+4),11,WHITE);
                    const char*ns=TextFormat("%d",opcoesP2[oi]);DrawText(ns,(int)(btn.x+(BTN6_W-MeasureText(ns,20))*0.5f),(int)(btn.y+(BTN6_H-20)*0.5f),20,WHITE);
                }else if(i==3){DrawRectangleRec(btn,DARKGREEN);DrawRectangleLinesEx(btn,2,GREEN);DrawText("CONF",(int)(btn.x+6),(int)(btn.y+18),16,WHITE);DrawText("P",(int)(btn.x+BTN6_W-18),(int)(btn.y+4),14,YELLOW);}
                else{DrawRectangleRec(btn,MAROON);DrawRectangleLinesEx(btn,2,RED);DrawText("CANC",(int)(btn.x+6),(int)(btn.y+18),16,WHITE);DrawText("L",(int)(btn.x+BTN6_W-18),(int)(btn.y+4),14,YELLOW);}
            }

            if(rodada>=10){const char*al="ALERTA: SISTEMA DETECTANDO INVASAO";int aw=MeasureText(al,16)+24;DrawRectangle((VIRT_W-aw)/2,VIRT_H-46,aw,34,(Color){120,0,0,230});DrawText(al,(VIRT_W-MeasureText(al,16))/2,VIRT_H-38,16,RED);}
        }
        else if (state==GAMEOVER) {
            DrawTexVirt(gameBg,(Rectangle){0,0,VIRT_W,VIRT_H},WHITE);
            DrawRectangle(0,0,VIRT_W,VIRT_H,(Color){0,0,0,160});
            bool hr=CheckCollisionPointRec(mouse,goRec),he=CheckCollisionPointRec(mouse,goExi);
            const char*t1="O LYCEUM TE DERROTOU",*t2="/?#@$ bloqueou sua invasao.",*t3="Voce foi reprovado por falta.";
            DrawText(t1,(VIRT_W-MeasureText(t1,44))/2,190,44,RED);
            DrawText(t2,(VIRT_W-MeasureText(t2,26))/2,260,26,WHITE);
            DrawText(t3,(VIRT_W-MeasureText(t3,26))/2,298,26,WHITE);
            DrawRectangleRec(goRec,hr?ORANGE:DARKGRAY);DrawRectangleRec(goExi,he?ORANGE:DARKGRAY);
            const char*lr="MENU",*le="SAIR";
            DrawText(lr,(int)(goRec.x+(goRec.width-MeasureText(lr,20))*0.5f),(int)(goRec.y+18),20,WHITE);
            DrawText(le,(int)(goExi.x+(goExi.width-MeasureText(le,20))*0.5f),(int)(goExi.y+18),20,WHITE);
        }
        else if (state==WIN) {
            DrawTexVirt(gameBg,(Rectangle){0,0,VIRT_W,VIRT_H},WHITE);
            DrawRectangle(0,0,VIRT_W,VIRT_H,(Color){0,0,0,150});
            const char*t1="ACESSO AO LYCEUM CONCEDIDO",*t2="A falta foi removida.",*t3="STATUS: APROVADO";
            const char*t4="Lucas observava tudo in silencio...",*t5="Clique para voltar ao menu";
            DrawText(t1,(VIRT_W-MeasureText(t1,44))/2,190,44,GREEN);
            DrawText(t2,(VIRT_W-MeasureText(t2,30))/2,264,30,WHITE);
            DrawText(t3,(VIRT_W-MeasureText(t3,38))/2,316,38,YELLOW);
            DrawText(t4,(VIRT_W-MeasureText(t4,24))/2,424,24,LIGHTGRAY);
            DrawText(t5,(VIRT_W-MeasureText(t5,22))/2,650,22,GRAY);
        }

        EndTextureMode();

        float scale,offX,offY; CalcLetterbox(&scale,&offX,&offY);
        BeginDrawing(); ClearBackground(BLACK);
        DrawTexturePro(canvas.texture,(Rectangle){0,0,(float)VIRT_W,-(float)VIRT_H},(Rectangle){offX,offY,VIRT_W*scale,VIRT_H*scale},(Vector2){0,0},0,WHITE);
        EndDrawing();
    }

    // Cleanup lore
    for(int i=0; i<introLoreCount; i++) free(introLore[i]);
    for(int i=0; i<finalLoreCount; i++) free(finalLore[i]);
    for(int b=0; b<MAX_BOSSES; b++){
        for(int i=0; i<bosses[b].entryCount; i++) free(bosses[b].entry[i]);
        for(int i=0; i<bosses[b].idleCount; i++) free(bosses[b].idle[i]);
        for(int i=0; i<bosses[b].hitCount; i++) free(bosses[b].hit[i]);
        for(int i=0; i<bosses[b].criticalCount; i++) free(bosses[b].critical[i]);
        for(int i=0; i<bosses[b].deathCount; i++) free(bosses[b].death[i]);
    }
    #define FREE_EASY(E) do{for(int i=0;i<E.entryCount;i++)free(E.entry[i]);for(int i=0;i<E.idleCount;i++)free(E.idle[i]);for(int i=0;i<E.hitCount;i++)free(E.hit[i]);for(int i=0;i<E.criticalCount;i++)free(E.critical[i]);for(int i=0;i<E.deathCount;i++)free(E.death[i]);}while(0)
    FREE_EASY(easySlime); FREE_EASY(easyOgre); FREE_EASY(easyBoss);
    #undef FREE_EASY

    UnloadRenderTexture(canvas);
    UnloadTexture(menuBg);UnloadTexture(gameBg);UnloadTexture(circuitBg);
    UnloadTexture(btnNovoJogo);UnloadTexture(btnFacil);UnloadTexture(btnDificil);
    UnloadTexture(btnStats);UnloadTexture(btnSair);
    UnloadTexture(slimeTex);UnloadTexture(ogroTex);UnloadTexture(bossTex);
    UnloadTexture(mariTex);UnloadTexture(romaTex);UnloadTexture(luisTex);
    UnloadTexture(micaTex);UnloadTexture(ruanTex);UnloadTexture(lucasTex);UnloadTexture(lucas2Tex);
    UnloadTexture(lore1);UnloadTexture(lore2);UnloadTexture(lore3);UnloadTexture(lore4);
    UnloadTexture(lore5);UnloadTexture(lore6);UnloadTexture(lore7);UnloadTexture(lore8);
    for(int i=0; i<transformLoreCount; i++) free(transformLore[i]);
    UnloadTexture(transformLore1); UnloadTexture(transformLore2); UnloadTexture(transformLore3); UnloadTexture(transformLore4);
    UnloadTexture(transformLore5); UnloadTexture(transformLore6); UnloadTexture(transformLore7); UnloadTexture(transformLore8);
    UnloadTexture(loreFinal1);UnloadTexture(loreFinal2);UnloadTexture(loreFinal3);UnloadTexture(loreFinal4);
    UnloadTexture(loreFinal5);UnloadTexture(loreFinal6);UnloadTexture(loreFinal7);UnloadTexture(loreFinal8);
    UnloadTexture(dropBg);UnloadTexture(textboxTex);
    UnloadMusicStream(menuMusic);UnloadMusicStream(gameMusic);
    UnloadSound(buffSound); UnloadSound(debuffSound); 
    CloseAudioDevice();
    CloseWindow();
    return 0;
}