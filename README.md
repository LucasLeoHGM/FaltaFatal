<h1 align="center">📊 Diagramas</h1>

<p align="center">
  <img src="assets/menuScreenshot.jpeg" width="750">
</p>

---

## 📌 Descrição
O projeto consiste em um jogo de adivinhação de números desenvolvido em linguagem C. O objetivo do jogador é descobrir um número aleatório gerado pelo sistema dentro de um intervalo definido, recebendo dicas a cada tentativa.

Além do tradicional, este jogo incorpora funcionalidades de análise de desempenho, registrando cada sessão em arquivo e permitindo o cálculo de estatísticas como média de tentativas, melhor e pior desempenho e desvio padrão. Com base nesses dados, o sistema fornece sugestões de estratégia, incentivando o jogador a melhorar sua forma de jogar.

Como diferencial, o jogo incorpora uma jogabilidade inspirada em elementos de RPG. A cada rodada, o jogador escolhe um dos valores disponíveis tentando acertar o número correto. Ao acertar, o jogador causa dano em um inimigo e avança de fase, criando uma progressão semelhante a combates em jogos de RPG. Tudo isso somado a identidade visual cyberpunk/matrix/hacker por conta da sua lore, sobre dois estudantes de cybersegurança que querem hackear o sistema da escola para passar de ano.

Além do aspecto lúdico, o projeto possui um forte caráter educacional, explorando conceitos fundamentais da programação em C, como geração de números aleatórios, manipulação de arquivos, validação de entrada e uso de recursão para cálculos estatísticos. Além de uma navegação pelas fases dos chefes através de uma série de charadas em proposições lógicas Dessa forma, o jogo funciona tanto como entretenimento quanto como ferramenta de aprendizado prático.


## 🎯 Objetivo
O objetivo do jogador é derrotar inimigos ao longo de diferentes fases, acertando o número secreto gerado pelo sistema. A cada rodada, o jogador escolhe entre valores apresentados e recebe dicas que indicam se o número correto está perto ou longe baseado no dano dos inimigos, utilizando essas informações para tomar decisões mais estratégicas.

Ao acertar o número, o jogador causa dano ao inimigo e avança no jogo. Durante essa progressão, suas tentativas são registradas, permitindo acompanhar seu desempenho por meio de estatísticas e melhorar sua estratégia ao longo das partidas.

---

## 🧑‍💻 Equipe

### 🧠 Product Owner // Desenvolvedor Backend (Core)
- **[Lucas Henrique](https://github.com/LucasLeoHGM)**  
Responsável pela definição de requisitos, priorização do backlog e validação das entregas.

---

### 📋 Scrum Master
- **[Luis Fim](https://github.com/luisfim)**  
Responsável pela organização do projeto, gerenciamento do quadro e remoção de impedimentos.

---

### 🎨 Designer (UX/UI) & Sound Designer
- **[Mariana Xavier](https://github.com/marixb)**  
Responsável pelo protótipo, fluxo do usuário, experiência visual do sistema e criação/definição de elementos sonoros (feedbacks, acertos, erros, etc.).

---

### 💻 Desenvolvedor Backend
- **[Victor Barros Roma](https://github.com/RomaNFS21)**  
Responsável pela lógica principal do jogo, incluindo geração de números e interação com o usuário.

---

### 📁 Desenvolvedor de Dados
- **[Ruan Carlos](https://github.com/Ruan-M-Oliveira)**  
Responsável pelo armazenamento e leitura de dados, além da manipulação de arquivos.

---

### 📊 Desenvolvedor de Dados e Estatísticas
- **[Micaella Cabral](https://github.com/micaellabcabral)**  
Responsável pelos cálculos estatísticos e análise de desempenho do jogador.

---

## 🧪 Tecnologias
💻 Linguagem principal
C - Utilizada para implementação da lógica do jogo, incluindo geração de números aleatórios, controle de fluxo, manipulação de arquivos e cálculos estatísticos.  
🖥️ Interface Visual
Raylib - Criar a interface visual 2d do jogo

---

## 🎨 Design e prototipação
Figma - Utilizado para criação do protótipo da interface do jogo e definição da experiência do usuário.

---

## 🗂️ Versionamento
Git e GitHub - Utilizados para controle de versão, organização do projeto e colaboração em equipe.

---

## 🎥 Documentação e apresentação
OBS Studio - Para gravação do screencast do projeto.

---

## 🧠 Metodologia
Desenvolvimento baseado em Scrum/Kanban (backlog; histórias de usuário ; 3Cs)

---

<h2>▶️ Como executar o projeto</h2>

<h3>1. Clonar o repositório</h3>

<pre><code>git clone https://github.com/LucasLeoHGM/Projeto-C.git
cd Projeto-C</code></pre>

<hr>

<h3>2. Verificar ambiente</h3>

<p>Este projeto utiliza <strong>Raylib</strong> com o <strong>GCC do MSYS2</strong>.</p>

<p>Abra o terminal <strong>MSYS2 MinGW64</strong> e verifique:</p>

<pre><code>gcc --version</code></pre>

<p>Se não funcionar, instale:</p>

<pre><code>pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-raylib</code></pre>

<hr>

<h3>3. Compilar o projeto</h3>

<pre><code>gcc main.c -o game.exe -lraylib -lopengl32 -lgdi32 -lwinmm</code></pre>

<hr>

<h3>4. Executar o projeto</h3>

<pre><code>./game.exe</code></pre>

<hr>

<h3>💡 Usando o VS Code (recomendado)</h3>

<p>Se o projeto estiver configurado com <code>tasks.json</code>, basta:</p>

<pre><code>Ctrl + Shift + B</code></pre>

<hr>

<h3>⚠️ Observações importantes</h3>

<ul>
    <li>Use sempre o terminal <strong>MSYS2 MinGW64</strong></li>
    <li>Certifique-se de que o Raylib está instalado</li>
    <li>Execute o <code>.exe</code> dentro da pasta do projeto</li>
</ul>

## 🎥 Screencast
- **[Youtube](https://youtu.be/DD_T3tZea4c): Link do vídeo no Youtube das fases iniciais de desenvolvimento.**  
- **[Youtube](https://youtu.be/gWsgtCcy6-g): Link do vídeo no Youtube da primeira versão de desenvolvimento.**  
- **[Youtube](https://youtu.be/b5CPDJiyfhY): Link do vídeo no Youtube da segunda versão de desenvolvimento.**  
- **[Youtube](https://youtu.be/36_c8sGkBbA): Link do vídeo no Youtube da terceira versão de desenvolvimento.**  


---

## 🔨 Ferramentas tecnológicas
Para o desenvolvimento do projeto utilizamos
- **[Notion](https://www.notion.so/Projeto-em-C-33014fb261ef8017acccc4b1c7c786c5): Utilizado para gestão do projeto**  
- **[Figma](https://www.figma.com/design/pLpnOWmcZg4JNdBwoAstdp/Jogo-Adivinhação?node-id=0-1&t=VEnLR5CqD9WFcHSY-0): Utilizado para prototipação**
  
---
## 📋 Tasks do Projeto

<p align="center">
  <img src="assets/tasks.jpg" width="700">
</p>

---

## ⚙️ Funcionalidades(MVP)

<p align="center">
  <img src="assets/funcionalidades.jpg" width="700">
</p>

---

## 📚 Histórias de Usuário

<p align="center">
  <img src="assets/historiasdeusuario.jpg" width="700">
</p>

---

## 📊 Diagramas

<p align="center">
  <img src="assets/diagrama.jpeg" width="700">
</p>

## 👥 Relato de Programação em Pares

Durante o desenvolvimento do projeto, a equipe experimentou a prática de programação em pares em diferentes etapas da construção do jogo. A atividade foi utilizada como forma de melhorar a revisão do código, dividir o raciocínio sobre as funcionalidades e facilitar a integração entre as partes desenvolvidas por cada integrante.

O projeto foi desenvolvido em linguagem C, com uso da biblioteca Raylib para a interface visual. Como o jogo possui diferentes áreas de funcionamento — lógica principal, combate, estatísticas, manipulação de arquivos, interface e documentação — a programação em pares foi aplicada principalmente nos pontos em que havia necessidade de integração entre funcionalidades.

### Organização dos pares e funcionalidades trabalhadas

**Lucas Henrique e Victor Barros Roma**
Atuaram na lógica principal do jogo, incluindo a estrutura da partida, funcionamento das tentativas, validação dos valores escolhidos pelo jogador e comportamento geral do sistema de adivinhação. Também participaram da integração da lógica com a progressão de fases e o combate contra inimigos.

Funcionalidades relacionadas:

* funcionamento principal da partida;
* escolha de valores pelo jogador;
* feedback das tentativas;
* progressão do jogo;
* integração da lógica de acerto com o dano causado aos inimigos.

**Ruan Carlos e Micaella Cabral**
Trabalharam nas funcionalidades ligadas ao registro de dados e análise de desempenho do jogador. O foco foi a manipulação de arquivos, leitura do histórico de partidas e cálculo das estatísticas utilizadas pelo sistema.

Funcionalidades relacionadas:

* registro de tentativas em arquivo;
* leitura do histórico de partidas;
* cálculo da média de tentativas;
* identificação da melhor e pior partida;
* cálculo do desvio padrão;
* sugestões de estratégia com base no desempenho do jogador.

**Luis Fim e Mariana Xavier**
Atuaram na organização da experiência do usuário, na validação do fluxo do jogo, na interface e na documentação das entregas. O trabalho em par foi importante para revisar se as funcionalidades implementadas estavam compreensíveis para o jogador e se a apresentação do projeto estava adequada às exigências da disciplina.

Funcionalidades e atividades relacionadas:

* revisão do fluxo do usuário;
* apoio à organização das telas e feedbacks visuais/sonoros;
* validação da experiência de jogo;
* organização do README;
* atualização das histórias de usuário;
* acompanhamento do board, backlog, sprints e issues no GitHub;
* preparação das evidências da entrega, como prints e screencasts.

### Atualização para a Entrega 4

Na Entrega 4, a prática foi incrementada com foco na revisão das novas funcionalidades, nos testes de sistema e na organização das evidências do projeto. A equipe revisou as histórias de usuário implementadas, registrou bugs e melhorias no GitHub Issues e atualizou o quadro do projeto para refletir o andamento das sprints.

Também foram analisados bugs identificados durante o uso do jogo, como:

* a música do menu que não voltava a tocar após o fim de uma partida;
* inconsistências na barra de HP dos monstros por não considerar corretamente buffs e debuffs.

Esses problemas foram registrados como issues no GitHub, permitindo que a equipe acompanhasse as correções e mantivesse um histórico mais claro do desenvolvimento.

### Avaliação da experiência

A programação em pares ajudou a equipe a revisar decisões de implementação, encontrar erros com mais facilidade e melhorar a comunicação entre os integrantes. A prática também contribuiu para que mais de uma pessoa compreendesse partes importantes do código, reduzindo a dependência de apenas um integrante para determinadas funcionalidades.

Além disso, o trabalho em pares facilitou a integração entre áreas diferentes do projeto, como lógica do jogo, estatísticas, interface e documentação. Com isso, a equipe conseguiu alinhar melhor as funcionalidades implementadas com as histórias de usuário planejadas no backlog.

