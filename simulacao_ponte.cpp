// ================================================================
//  Simulacao de um veiculo atravessando uma ponte estaiada
//
//  - Tabuleiro continuo apoiado nas pontas, nos 8 estais e no pilone
//  - 3 sensores (S1, S2, S3) a cada x; 4 pontos de tensao (P1..P4)
//    no meio entre eles; estais a x/4 de cada ponto
//  - O comprimento da ponte e escolhido pelo usuario
//  - O peso do veiculo e limitado a capacidade da ponte, definida
//    pela tensao admissivel do material em qualquer ponto do tabuleiro
// ================================================================
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <sstream>
#include <cstdlib>
#include <windows.h>


// ---------------- Geometria (definida pelo usuario) ----------------
// A ponte tem comprimento total = 4x  (x = distancia entre sensores)
// S = x/2 = distancia entre pontos vizinhos = COMPRIMENTO / 8
double COMPRIMENTO;   // comprimento total (m)
double S;    // distancia entre pontos (m)

// Limites do comprimento (m). Nesta configuracao (1 pilone, 8 estais),
// a distancia entre estais vale COMPRIMENTO / 8. Em pontes estaiadas reais
// ela fica entre ~5 m e ~20 m, o que da uma ponte de 40 m a 160 m.
const double COMPRIMENTO_MIN = 40.0;
const double COMPRIMENTO_MAX = 160.0;

// ---------------- Secao, material e cargas ----------------
const double q          = 80.0;   // peso proprio distribuido (kN/m)
const double I          = 0.12;   // momento de inercia da secao (m^4)
const double c          = 0.6;    // distancia da linha neutra a fibra inferior (m)
const double SIGMA_ADM  = 30.0;   // tensao admissivel do material (MPa)

// ---------------- Simulacao ----------------
const double VELOCIDADE_KMH = 60.0;  // velocidade do veiculo (km/h)
const double DT             = 0.05;  // passo de tempo (s)
const int    IMPRIMIR_CADA  = 10;    // mostra na tela 1 a cada 10 passos (0,5 s)
const double PASSO_VERIF    = 0.5;   // malha para verificar a capacidade (m)

// Pontos de tensao (azuis): ficam em S, 3S, 5S e 7S
const int N_PONTOS = 4;
double PONTOS[N_PONTOS];

// ================================================================
//  Define a geometria a partir do comprimento escolhido
// ================================================================
void definirGeometria(double comprimento)
{
	system("cls");
    COMPRIMENTO = comprimento;
    S = COMPRIMENTO / 8.0;
    for (int k = 0; k < N_PONTOS; k++)
        PONTOS[k] = (2 * k + 1) * S;     // S, 3S, 5S, 7S
}

// ================================================================
//  Geometria dos apoios
// ================================================================
std::vector<double> montarApoios()
{
    const double posicoes[] = { 0.0,
                                0.5 * S, 1.5 * S, 2.5 * S, 3.5 * S,   // estais da esquerda
                                4.0 * S,                              // pilone
                                4.5 * S, 5.5 * S, 6.5 * S, 7.5 * S,   // estais da direita
                                8.0 * S };
    int n = sizeof(posicoes) / sizeof(posicoes[0]);
    return std::vector<double>(posicoes, posicoes + n);
}

int vaoDe(double X, const std::vector<double>& apoios)
{
    int n = static_cast<int>(apoios.size()) - 1;
    for (int j = 0; j < n; j++)
        if (X < apoios[j + 1]) return j;
    return n - 1;
}

// ================================================================
//  PASSO 1: momento no vao isolado (biapoiado)
// ================================================================
double momentoVaoIsolado(double x, double Lv, double P, bool temVeiculo, double a)
{
    double M = q * x * (Lv - x) / 2.0;              // peso proprio

    if (temVeiculo) {                                // veiculo
        if (x <= a) M += P * (Lv - a) * x / Lv;
        else        M += P * a * (Lv - x) / Lv;
    }
    return M;
}

// ================================================================
//  PASSO 2: momentos nos apoios (equacao dos tres momentos + Thomas)
// ================================================================
std::vector<double> momentosNosApoios(double X_veiculo, double P,
                                      const std::vector<double>& apoios)
{
    int nApoios = static_cast<int>(apoios.size());
    int n = nApoios - 2;

    std::vector<double> sub(n), diag(n), sup(n), rhs(n);

    int    vv = vaoDe(X_veiculo, apoios);
    double av = X_veiculo - apoios[vv];

    for (int k = 0; k < n; k++) {
        int i = k + 1;
        double Le = apoios[i]     - apoios[i - 1];
        double Ld = apoios[i + 1] - apoios[i];

        sub[k]  = Le;
        diag[k] = 2.0 * (Le + Ld);
        sup[k]  = Ld;

        rhs[k] = -(q * Le * Le * Le / 4.0 + q * Ld * Ld * Ld / 4.0);

        if (vv == i - 1)
            rhs[k] -= P * av * (Le * Le - av * av) / Le;
        if (vv == i) {
            double b = Ld - av;
            rhs[k] -= P * b * (Ld * Ld - b * b) / Ld;
        }
    }

    std::vector<double> cp(n), dp(n), Mint(n);
    cp[0] = sup[0] / diag[0];
    dp[0] = rhs[0] / diag[0];
    for (int k = 1; k < n; k++) {
        double m = diag[k] - sub[k] * cp[k - 1];
        cp[k] = sup[k] / m;
        dp[k] = (rhs[k] - sub[k] * dp[k - 1]) / m;
    }
    Mint[n - 1] = dp[n - 1];
    for (int k = n - 2; k >= 0; k--)
        Mint[k] = dp[k] - cp[k] * Mint[k + 1];

    std::vector<double> M(nApoios, 0.0);
    for (int k = 0; k < n; k++) M[k + 1] = Mint[k];
    return M;
}

// ================================================================
//  PASSO 3: momento em qualquer ponto X
// ================================================================
double momentoNaPonte(double X, double X_veiculo, double P,
                      const std::vector<double>& apoios,
                      const std::vector<double>& Mapoio)
{
    int    j  = vaoDe(X, apoios);
    double Lv = apoios[j + 1] - apoios[j];
    double x  = X - apoios[j];

    bool   temVeiculo = (vaoDe(X_veiculo, apoios) == j);
    double a = X_veiculo - apoios[j];

    double M_isolado = momentoVaoIsolado(x, Lv, P, temVeiculo, a);
    double M_apoios  = Mapoio[j] * (1.0 - x / Lv) + Mapoio[j + 1] * (x / Lv);
    return M_isolado + M_apoios;
}

// ================================================================
//  PASSO 4: tensao na fibra inferior (positivo = tracao)
// ================================================================
double tensao(double M)
{
    return M * c / I / 1000.0;   // kPa -> MPa
}

// ================================================================
//  CAPACIDADE DA PONTE
//  A tensao e linear no peso do veiculo:
//      sigma(P) = sigma_peso + P * sigma_1
//  sigma_peso = tensao so com o peso proprio
//  sigma_1    = tensao causada por 1 kN de veiculo
//  Para cada posicao do veiculo e cada ponto do tabuleiro, o maior P
//  que mantem |sigma| <= SIGMA_ADM e:
//      tracao:     P = ( SIGMA_ADM - sigma_peso) / sigma_1   (sigma_1 > 0)
//      compressao: P = (-SIGMA_ADM - sigma_peso) / sigma_1   (sigma_1 < 0)
//  A capacidade e o MENOR desses valores (situacao mais critica).
// ================================================================
struct Capacidade {
    double Pmax;       // peso maximo do veiculo (kN)
    double Xponto;     // ponto do tabuleiro que governa (m)
    double Xveiculo;   // posicao do veiculo na situacao critica (m)
};

Capacidade calcularCapacidade(const std::vector<double>& apoios)
{
    Capacidade cap;
    cap.Pmax = 1e30;
    cap.Xponto = 0.0;
    cap.Xveiculo = 0.0;

    for (double Xv = 0.0; Xv <= COMPRIMENTO + 1e-9; Xv += PASSO_VERIF) {
        std::vector<double> M0 = momentosNosApoios(Xv, 0.0, apoios);  // so peso proprio
        std::vector<double> M1 = momentosNosApoios(Xv, 1.0, apoios);  // + 1 kN de veiculo

        for (double X = 0.0; X <= COMPRIMENTO + 1e-9; X += PASSO_VERIF) {
            double s0 = tensao(momentoNaPonte(X, Xv, 0.0, apoios, M0));
            double s1 = tensao(momentoNaPonte(X, Xv, 1.0, apoios, M1)) - s0;

            if (std::fabs(s0) > SIGMA_ADM) {    // nem o peso proprio e suportado
                cap.Pmax = 0.0;
                cap.Xponto = X;
                cap.Xveiculo = Xv;
                return cap;
            }

            double limite = 1e30;
            if (s1 > 1e-12)  limite = ( SIGMA_ADM - s0) / s1;
            if (s1 < -1e-12) limite = (-SIGMA_ADM - s0) / s1;

            if (limite < cap.Pmax) {
                cap.Pmax = limite;
                cap.Xponto = X;
                cap.Xveiculo = Xv;
            }
        }
    }
    return cap;
}

// ================================================================
//  DESENHO ESQUEMATICO DA PONTE (em texto, no console)
//  A ponte e desenhada numa "tela" de caracteres: cada coluna
//  representa COMPRIMENTO/80 metros.
// ================================================================
const int TELA_LARG = 81;   // colunas do desenho
const int TELA_ALT  = 16;   // linhas do desenho
const int LINHA_TAB = 12;   // linha onde fica o tabuleiro

int coluna(double X)
{
    return static_cast<int>(X / COMPRIMENTO * (TELA_LARG - 1) + 0.5);
}

void escrever(std::vector<std::string>& tela, int lin, int col, const std::string& texto)
{
    for (size_t i = 0; i < texto.size(); i++) {
        int cc = col + static_cast<int>(i);
        if (lin >= 0 && lin < TELA_ALT && cc >= 0 && cc < TELA_LARG)
            tela[lin][cc] = texto[i];
    }
}

// Desenha uma reta de (l0, c0) ate (l1, c1) com o caractere ch
void desenharReta(std::vector<std::string>& tela, int l0, int c0, int l1, int c1, char ch)
{
    int passos = std::abs(l1 - l0);
    for (int t = 0; t <= passos; t++) {
        double f = (passos == 0) ? 0.0 : static_cast<double>(t) / passos;
        int l = static_cast<int>(l0 + (l1 - l0) * f + 0.5);
        int c = static_cast<int>(c0 + (c1 - c0) * f + 0.5);
        escrever(tela, l, c, std::string(1, ch));
    }
}

void desenharPonte(const std::vector<double>& apoios)
{
    std::vector<std::string> tela(TELA_ALT, std::string(TELA_LARG, ' '));
    int colPilone = coluna(4.0 * S);
    int nApoios = static_cast<int>(apoios.size());

    // Estais: do topo do pilone ate cada ancoragem no tabuleiro.
    // Os estais mais afastados saem mais do alto do pilone.
    for (int i = 1; i < nApoios - 1; i++) {
        int col = coluna(apoios[i]);
        if (col == colPilone) continue;                 // e o pilone, nao um estai

        double dist = std::fabs(static_cast<double>(col - colPilone)) / colPilone;
        int linTopo = static_cast<int>(6.0 * (1.0 - dist) + 0.5);
        bool esquerda = (col < colPilone);

        desenharReta(tela, linTopo, esquerda ? colPilone - 1 : colPilone + 1,
                     LINHA_TAB - 1, col, esquerda ? '/' : '\\');
    }

    // Pilone
    for (int l = 0; l < TELA_ALT - 1; l++) escrever(tela, l, colPilone, "|");

    // Tabuleiro e apoios das pontas
    escrever(tela, LINHA_TAB, 0, std::string(TELA_LARG, '='));
    escrever(tela, LINHA_TAB, colPilone, "#");
    escrever(tela, LINHA_TAB + 1, 0, "^");
    escrever(tela, LINHA_TAB + 1, TELA_LARG - 1, "^");

    // Pontos de tensao (o) e sensores (*), com os nomes embaixo
    for (int k = 0; k < N_PONTOS; k++) {
        int col = coluna(PONTOS[k]);
        escrever(tela, LINHA_TAB, col, "o");
        escrever(tela, LINHA_TAB + 1, col - 1, std::string("P") + char('1' + k));
    }
    for (int k = 0; k < 3; k++) {
        int col = coluna((2 * k + 2) * S);
        if (col != colPilone) escrever(tela, LINHA_TAB, col, "*");
        int colTexto = (col == colPilone) ? col + 1 : col - 1;  // nao escreve em cima do pilone
        escrever(tela, LINHA_TAB + 1, colTexto, std::string("S") + char('1' + k));
    }

    // Regua com o comprimento
    std::ostringstream fim;
    fim << std::fixed << std::setprecision(1) << COMPRIMENTO << " m";
    escrever(tela, TELA_ALT - 1, 0, "0 m");
    escrever(tela, TELA_ALT - 1, TELA_LARG - static_cast<int>(fim.str().size()), fim.str());

    // Mostra a tela
    std::cout << "\n";
    for (int l = 0; l < TELA_ALT; l++) {
        std::string linha = tela[l];
        linha.erase(linha.find_last_not_of(' ') + 1);   // tira espacos do fim
        std::cout << "  " << linha << "\n";
    }
    std::cout << "\n  |o = ponto de tensao   * = sensor   / \\ = estais   | = pilone   ^ = apoio|\n";
}

// ================================================================
//  Leitura de um numero com validacao
// ================================================================
double lerNumero(double minimo, double maximo)
{
    double valor;
    while (!(std::cin >> valor) || valor < minimo || valor > maximo) {
        if (std::cin.eof()) {                    // entrada encerrada: sai do programa
            std::cout << "\nEntrada encerrada.\n";
            std::exit(1);
        }
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Valor invalido. Digite um numero entre "
                  << minimo << " e " << maximo << ": ";
    }
    return valor;
}

// ================================================================
//  Leitura do peso do veiculo (limitado a capacidade)
// ================================================================
double lerPesoVeiculo(double Pmax)
{
    std::cout << "Peso do veiculo em kN (maximo: " << Pmax << " kN): ";
    double P = lerNumero(0.0, 1e12);

    if (P > Pmax) {
        std::cout << "\nATENCAO: " << P << " kN ultrapassa a capacidade da ponte ("
                  << Pmax << " kN).\n"
                  << "O peso foi limitado a " << Pmax << " kN.\n";
        P = Pmax;
    }
    return P;
}

// ================================================================
//  Programa principal
// ================================================================
int main()
{
	SetConsoleCP(1252);
    SetConsoleOutputCP(1252);
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "=================================== SIMULAÇÃO DE PONTE ESTAIADA ===================================\n\n";

    // ---------- 1) Tamanho da ponte e capacidade ----------
    std::vector<double> apoios;
    Capacidade cap;

    while (true) {
        std::cout << "Comprimento da ponte em metros ("
                  << COMPRIMENTO_MIN << " a " << COMPRIMENTO_MAX << "): ";
        double comprimento = lerNumero(COMPRIMENTO_MIN, COMPRIMENTO_MAX);

        definirGeometria(comprimento);
        apoios = montarApoios();
        cap = calcularCapacidade(apoios);

        if (cap.Pmax > 0.0) break;

        std::cout << "\nCom " << COMPRIMENTO << " m a ponte nao suporta nem o proprio peso "
                  << "(tensao acima de " << SIGMA_ADM << " MPa).\n"
                  << "Escolha um comprimento menor.\n\n";
    }

    desenharPonte(apoios);

    std::cout << "\nPonte de " << COMPRIMENTO << " m | distancia entre sensores x = "
              << 2.0 * S << " m | tensao admissivel = " << SIGMA_ADM << " MPa\n";
    std::cout << "Pontos de tensao em:";
    for (int k = 0; k < N_PONTOS; k++)
        std::cout << " " << PONTOS[k] << " m" << (k < N_PONTOS - 1 ? "," : "\n");
    std::cout << "Capacidade da ponte: " << cap.Pmax << " kN ("
              << cap.Pmax / 9.81 << " t)\n";
    std::cout << "Situacao critica: veiculo em X = " << cap.Xveiculo
              << " m, tensao maxima em X = " << cap.Xponto << " m\n";
	std::cout << "===================================================================================================\n";

    // ---------- 2) Peso do veiculo ----------
    double P = lerPesoVeiculo(cap.Pmax);

    // ---------- 3) Simulacao no tempo ----------
    double v = VELOCIDADE_KMH / 3.6;   // m/s

    std::vector<double> sMax(N_PONTOS, -1e30), sMin(N_PONTOS, 1e30);
    std::vector<double> xMax(N_PONTOS, 0.0),   xMin(N_PONTOS, 0.0);
    double maiorTensaoPonte = 0.0;   // maior |tensao| em todo o tabuleiro

    std::cout << "\nVeiculo de " << P << " kN a " << VELOCIDADE_KMH << " km/h\n\n";
    std::cout << "   t(s)   X(m) |   P1      P2      P3      P4   (MPa)\n";
    std::cout << "---------------+--------------------------------------\n";

    for (int passo = 0; ; passo++) {
        double t  = passo * DT;
        double Xv = v * t;
        if (Xv > COMPRIMENTO) break;

        std::vector<double> Mapoio = momentosNosApoios(Xv, P, apoios);
        bool imprimir = (passo % IMPRIMIR_CADA == 0);

        if (imprimir) std::cout << std::setw(7) << t << std::setw(7) << Xv << " |";

        // Tensao nos 4 pontos azuis
        for (int k = 0; k < N_PONTOS; k++) {
            double s = tensao(momentoNaPonte(PONTOS[k], Xv, P, apoios, Mapoio));
            if (imprimir) std::cout << std::setw(8) << s;

            if (s > sMax[k]) { sMax[k] = s; xMax[k] = Xv; }
            if (s < sMin[k]) { sMin[k] = s; xMin[k] = Xv; }
        }

        // Verificacao de seguranca: tabuleiro inteiro
        for (double X = 0.0; X <= COMPRIMENTO + 1e-9; X += PASSO_VERIF) {
            double s = std::fabs(tensao(momentoNaPonte(X, Xv, P, apoios, Mapoio)));
            maiorTensaoPonte = std::max(maiorTensaoPonte, s);
        }

        if (imprimir) std::cout << "\n";
    }

    // ---------- 4) Resumo ----------
    std::cout << "\nResumo por ponto:\n";
    for (int k = 0; k < N_PONTOS; k++) {
        std::cout << "  P" << k + 1 << " (X = " << std::setw(7) << PONTOS[k] << " m): "
                  << "max = " << std::setw(6) << sMax[k] << " MPa (veiculo em "
                  << std::setw(7) << xMax[k] << " m) | "
                  << "min = " << std::setw(6) << sMin[k] << " MPa (veiculo em "
                  << std::setw(7) << xMin[k] << " m)\n";
    }
    std::cout << "\nMaior tensao em toda a ponte durante a travessia: "
              << maiorTensaoPonte << " MPa (admissivel: " << SIGMA_ADM << " MPa)\n";
    return 0;
}
