// =====================================================================
//  SIMULACAO DE TENSOES EM PONTE ESTAIADA (versao de console)
// ---------------------------------------------------------------------
//  - A ponte tem 4 pilares (Viga 01, 02, 03 e 04).
//  - Cada pilar tem 3 sensores: S1 (topo), S2 (meio) e S3 (base).
//  - Os sensores tem limite minimo 108 MPa e maximo 831 MPa.
//  - Dois modos de simulacao:
//      1) O usuario digita a tensao de cada sensor.
//      2) Um carro atravessa a ponte e o programa calcula as tensoes.
// =====================================================================

// ---------- Bibliotecas usadas ----------
#include <iostream>   // cout (escrever na tela) e cin (ler do teclado)
#include <string>     // tipo string (texto)
#include <sstream>    // stringstream: transforma um texto em numero e verifica se e valido
#include <iomanip>    // setw e setprecision: formatar numeros na tela (largura e casas decimais)
#include <random>     // gerador de numeros aleatorios (para o "ruido" dos sensores)
#include <thread>     // this_thread::sleep_for: pausar o programa
#include <chrono>     // chrono::milliseconds: unidade de tempo usada na pausa
#include <cmath>      // fabs (valor absoluto) e lround (arredondar)
#include <windows.h> 
using namespace std;  // permite escrever cout em vez de std::cout

int main() {
	
	SetConsoleCP(1252);
    SetConsoleOutputCP(1252);
    
    cout << "=========== SIMULADOR DE PONTE ESTAIADA ===========\n\n";
    
    // =================================================================
    // CONSTANTES: valores fixos do sistema. 
    // =================================================================
    const double LIM_MIN = 108.0;          // abaixo disso o sensor esta fora do limite
    const double LIM_MAX = 831.0;          // acima disso o sensor esta fora do limite
    const double PONTE_MIN = 60.0;         // menor comprimento de ponte aceito (metros)
    const double PONTE_MAX = 600.0;        // maior comprimento de ponte aceito (metros)
    const int NP = 4;                      // numero de pilares
    const int NS = 3;                      // numero de sensores em cada pilar
    const double TENSAO_BASE = 200.0;      // tensao que o peso da propria ponte causa (sem carro)
    const double CARGA_POR_TON = 15.0;     // quanto cada tonelada do carro soma na tensao
    // Fator de cada sensor: o da base (S3) recebe mais carga que o do topo (S1).
    // FATOR_SENSOR[0] e o S1, [1] e o S2, [2] e o S3 (vetores comecam no 0).
    const double FATOR_SENSOR[NS] = {1.00, 1.00, 1.00};
    const int PASSOS = 40;                 // em quantas etapas o carro atravessa a ponte
    const int LARGURA = 41;                // quantos caracteres tem o desenho da ponte

    // =================================================================
    // VARIAVEIS: valores que mudam durante o programa
    // =================================================================
    double comprimento = 0.0;              // comprimento total da ponte digitado pelo usuario
    // Matriz 4 x 3: tensao[pilar][sensor]. Ex.: tensao[1][2] = Viga 02, sensor S3.
    // O "= {}" zera todas as posicoes.
    double tensao[NP][NS] = {};
    string linha;                          // guarda a linha que o usuario digitar

    // Gerador de numeros aleatorios.
    // random_device{}() gera uma "semente" diferente a cada execucao,
    // assim o ruido nao se repete sempre igual.
    mt19937 gerador(random_device{}());
    // Distribuicao normal: sorteia numeros perto de 0.0, variando em torno de 3.0.
    // Serve para imitar a pequena oscilacao de um sensor real.
    normal_distribution<double> ruido(0.0, 3.0);

    // Daqui em diante todo numero decimal aparece com 1 casa depois da virgula.
    cout << fixed << setprecision(1);

    // =================================================================
    // LEITURA DO COMPRIMENTO DA PONTE
    // O while(true) repete ate o usuario digitar um valor valido;
    // o "break" sai do laco quando der certo.
    // =================================================================
    
    while (true) {
        cout << "Comprimento total da ponte em metros (" << PONTE_MIN << " a " << PONTE_MAX << "): ";

        // getline le a linha inteira digitada.
        // Se retornar falso (fim da entrada, ex.: Ctrl+Z), o programa encerra.
        if (!getline(cin, linha)) return 0;

        // stringstream trata o texto como se fosse uma entrada de teclado,
        // para podermos tentar extrair um numero dele.
        stringstream ss(linha);
        double v;      // onde o numero lido vai ficar
        char sobra;    // usado para detectar lixo depois do numero (ex.: "12abc")

        // Condicoes para aceitar:
        //  ss >> v          -> conseguiu ler um numero no comeco
        //  !(ss >> sobra)   -> NAO sobrou nenhuma letra depois do numero
        //  v >= ... <= ...  -> o numero esta dentro do intervalo permitido
        if (ss >> v && !(ss >> sobra) && v >= PONTE_MIN && v <= PONTE_MAX) {
            comprimento = v;   // valor aceito
            system("cls");
            break;             // sai do while
        }else{
        	
        	cout << "Valor invalido. Digite apenas numeros dentro do intervalo (" << PONTE_MIN << " a " << PONTE_MAX << ").\n";
		}

    }

    // =================================================================
    // MENU PRINCIPAL
    // Fica repetindo ate o usuario escolher 0 (sair).
    // =================================================================
    int opcao = -1;   // comeca com -1 so para entrar no laco
    while (opcao != 0) {
        // Vao S = distancia entre dois pilares vizinhos.
        // 4 pilares formam 3 vaos, por isso divide por (NP - 1) = 3.
        double vao = comprimento / (NP - 1);

        // Mostra o menu
        cout << "\n================== PONTE ESTAIADA ==================\n";
        cout << "Comprimento: " << comprimento << " m | Vao entre pilares (S): " << vao << " m\n";
        cout << "====================================================\n";
        cout << "1 - Inserir tensoes manualmente por sensor\n";
        cout << "2 - Simular carros passando na ponte\n";
        cout << "3 - Alterar comprimento da ponte\n";
        cout << "0 - Sair\n";
        cout << "====================================================\n";

        // Le a opcao com a mesma validacao do comprimento,
        // so que agora o numero e inteiro (int) e deve estar entre 0 e 3.
        while (true) {
            cout << "Opcao: ";
            if (!getline(cin, linha)) return 0;
            stringstream ss(linha); int v; char sobra;
            if (ss >> v && !(ss >> sobra) && v >= 0 && v <= 3) { opcao = v; break; }
            cout << "Opcao invalida.\n";
        }

        // Controle do relatorio final:
        // os modos 1 e 2 ligam essa variavel para o relatorio ser mostrado.
        bool mostrarRelatorio = false;
        string titulo;   // titulo do relatorio (muda conforme o modo)

        // =============================================================
        // MODO 1: INSERCAO MANUAL DAS TENSOES
        // =============================================================
        if (opcao == 1) {
        	system("cls");
            // Dois for aninhados: para cada pilar (p), percorre seus 3 sensores (s).
            // Total de 4 x 3 = 12 leituras.
            cout << "Digite as tensões dos sensores:\n";
            for (int p = 0; p < NP; p++) {
                for (int s = 0; s < NS; s++) {
                    while (true) {
                        // p + 1 e s + 1 porque o usuario conta a partir de 1, nao de 0
                        cout << "Viga 0" << p + 1 << " - Sensor S" << s + 1 << " (MPa): ";
                        if (!getline(cin, linha)) return 0;
                        stringstream ss(linha); double v; char sobra;
                        // So aceita o valor se estiver DENTRO do limite do sensor
                        // (entre LIM_MIN = 108 e LIM_MAX = 831, incluindo os extremos).
                        // Se estiver fora, mostra o motivo e pede de novo.
                        if (ss >> v && !(ss >> sobra) && v >= LIM_MIN && v <= LIM_MAX) { tensao[p][s] = v; break; }
                        cout << "#Valor invalido. Digite um numero entre " << LIM_MIN
                             << " MPa e " << LIM_MAX << " MPa.\n";
                    }
                }
            }
            mostrarRelatorio = true;
            titulo = "RELATORIO - TENSOES INSERIDAS";
        }

        // =============================================================
        // MODO 2: SIMULACAO DE UM CARRO PASSANDO PELA PONTE
        // =============================================================
        else if (opcao == 2) {
        	system("cls");
        	
            double massa = 0, velocidade = 0;

            // Le a massa do veiculo (500 kg = carro pequeno, 60000 kg = carreta pesada)
            while (true) {
                cout << "Massa em kg (500 a 60000): ";
                if (!getline(cin, linha)) return 0;
                stringstream ss(linha); double v; char sobra;
                if (ss >> v && !(ss >> sobra) && v >= 500 && v <= 60000) { massa = v; break; }
                cout << "  Valor invalido.\n";
            }
            system("cls");
            // Le a velocidade do veiculo
            while (true) {
                cout << "Velocidade em km/h (10 a 120): ";
                if (!getline(cin, linha)) return 0;
                stringstream ss(linha); double v; char sobra;
                if (ss >> v && !(ss >> sobra) && v >= 10 && v <= 120) { velocidade = v; break; }
                cout << "Valor invalido.\n";
            }
            system("cls");

            // dx = quantos metros o carro anda a cada passo da simulacao
            double dx = comprimento / PASSOS;
            // dt = quantos segundos o carro leva para andar dx.
            // velocidade / 3.6 converte km/h para m/s; tempo = distancia / velocidade.
            double dt = dx / (velocidade / 3.6);
            // Guarda a MAIOR tensao que cada sensor teve durante a travessia
            double maximo[NP][NS] = {};

            // Cabecalho da tabela.
            // string(LARGURA - 5, ' ') cria uma sequencia de espacos para alinhar as colunas.
            cout << "\n   tempo   pos(m)  ponte" << string(LARGURA - 5, ' ')
                 << "   V01         V02         V03         V04\n";

            // Cada volta deste for e um "quadro" da simulacao.
            // Vai de 0 ate PASSOS (inclusive), ou seja, do inicio ao fim da ponte.
            for (int i = 0; i <= PASSOS; i++) {
                double pos = i * dx;   // posicao atual do carro em metros

                // ----- Calculo da tensao de cada sensor -----
                for (int p = 0; p < NP; p++) {
                    // O pilar p fica na posicao p * vao (0, S, 2S, 3S).
                    // fabs da a distancia entre o carro e o pilar, sempre positiva.
                    double distancia = fabs(pos - p * vao);

                    // Influencia do carro sobre este pilar:
                    //  - carro em cima do pilar (distancia 0)   -> influencia 1.0 (100%)
                    //  - carro a meio vao                        -> influencia 0.5 (50%)
                    //  - carro a um vao inteiro ou mais de longe -> influencia 0.0 (nada)
                    // O "? :" e um if de uma linha: (condicao) ? valor_se_sim : valor_se_nao
                    double influencia = (distancia < vao) ? 1.0 - distancia / vao : 0.0;

                    for (int s = 0; s < NS; s++) {
                        // Formula da tensao:
                        //  (peso da ponte + toneladas do carro x carga por tonelada x influencia)
                        //  x fator do sensor (topo, meio ou base)
                        //  + um pequeno ruido aleatorio
                        double t = (TENSAO_BASE + (massa / 1000.0) * CARGA_POR_TON * influencia)
                                   * FATOR_SENSOR[s] + ruido(gerador);
                        tensao[p][s] = t;
                        // Se esta tensao for a maior ate agora, guarda como maximo
                        if (t > maximo[p][s]) maximo[p][s] = t;
                    }
                }

                // ----- Desenho da ponte em texto -----
                // Comeca com uma linha de LARGURA sinais de '='
                string pista(LARGURA, '=');
                // Coloca um 'I' na posicao de cada pilar.
                // A conta distribui os 4 pilares de forma igual: posicoes 0, 13, 27 e 40.
                for (int p = 0; p < NP; p++)
                    pista[(int)lround(p * (LARGURA - 1) / (double)(NP - 1))] = 'I';
                // Coloca o carro 'C' proporcional a posicao dele na ponte
                pista[(int)lround(pos / comprimento * (LARGURA - 1))] = 'C';

                // ----- Imprime uma linha da tabela -----
                // setw(7) reserva 7 espacos para o numero, deixando as colunas alinhadas
                cout << setw(7) << i * dt << "s " << setw(7) << pos << "  " << pista << " ";
                for (int p = 0; p < NP; p++) {
                    // Media dos 3 sensores do pilar
                    double media = (tensao[p][0] + tensao[p][1] + tensao[p][2]) / NS;
                    // Verifica se algum sensor deste pilar saiu do limite
                    bool fora = false;
                    for (int s = 0; s < NS; s++)
                        if (tensao[p][s] < LIM_MIN || tensao[p][s] > LIM_MAX) fora = true;
                    // Mostra a media e um "*" se houver sensor fora do limite
                    cout << setw(6) << media << " MPa" << (fora ? "*" : " ") << " ";
                }
                cout << "\n";

                // Pausa de 150 milissegundos para dar efeito de animacao
                this_thread::sleep_for(chrono::milliseconds(150));
            }
            cout << "(* = algum sensor do pilar fora do limite)\n";

            // Copia os maximos para a matriz tensao, porque o relatorio
            // la embaixo sempre le a matriz tensao. Assim ele mostra os picos.
            for (int p = 0; p < NP; p++)
                for (int s = 0; s < NS; s++)
                    tensao[p][s] = maximo[p][s];
            mostrarRelatorio = true;
            titulo = "RELATORIO - PICOS DURANTE A TRAVESSIA";
        }

        // =============================================================
        // OPCAO 3: ALTERAR O COMPRIMENTO DA PONTE
        // Mesma validacao do inicio do programa.
        // =============================================================
        else if (opcao == 3) {
    	system("cls");                                       // limpa a tela uma vez, antes de pedir
    	while (true) {
        	cout << "Novo comprimento (" << PONTE_MIN << " a " << PONTE_MAX << " m): ";
        	if (!getline(cin, linha)) return 0;
        	stringstream ss(linha); double v; char sobra;
        	if (ss >> v && !(ss >> sobra) && v >= PONTE_MIN && v <= PONTE_MAX) { comprimento = v; break; }
        	system("cls");                                   // limpa as tentativas anteriores
        	cout << "  Valor invalido: " << linha << "\n";   // mostra so o erro mais recente
    	}
    	system("cls");
		}

        // =============================================================
        // RELATORIO (usado pelos modos 1 e 2)
        // Fica fora dos ifs para nao repetir o mesmo codigo duas vezes.
        // =============================================================
        if (mostrarRelatorio) {
            cout << "\n--- " << titulo << " ---\n";
            cout << "Limites: minimo " << LIM_MIN << " MPa | maximo " << LIM_MAX << " MPa\n\n";
            cout << "Pilar       S1         S2         S3      Media       Status\n";

            int alertas = 0;   // conta quantos pilares tem algum sensor fora do limite

            for (int p = 0; p < NP; p++) {
                double media = 0;
                string status = "";   // vai acumulando os avisos deste pilar

                cout << "Viga 0" << p + 1;
                for (int s = 0; s < NS; s++) {
                    cout << setw(7) << tensao[p][s] << " MPa";   // mostra o valor do sensor com a unidade
                    media += tensao[p][s];             // soma para calcular a media
                    // to_string transforma o numero do sensor em texto ("1", "2", "3")
                    if (tensao[p][s] < LIM_MIN) status += " S" + to_string(s + 1) + "-BAIXO";
                    if (tensao[p][s] > LIM_MAX) status += " S" + to_string(s + 1) + "-ALTO";
                }
                media /= NS;   // mesmo que media = media / 3

                // Se nenhum aviso foi adicionado, o pilar esta OK
                if (status.empty()) status = " OK";
                else alertas++;   // senao, conta mais um pilar em alerta

                cout << setw(7) << media << " MPa" << "   " << status << "\n";
            }

            // Resumo final
            if (alertas == 0) cout << "\nPonte dentro dos limites.\n";
            else cout << "\nALERTA: " << alertas << " pilar(es) com sensor fora do limite!\n";
            
             // Pausa: espera o usuario apertar ENTER antes de voltar ao menu
            cout << "\nPressione QUALQUER TECLA para voltar ao menu...";
            getline(cin, linha);   // fica parado aqui ate o usuario apertar ENTER
            system("cls");         // limpa a tela para o menu aparecer sozinho
        }
    }   // fim do while do menu: volta a mostrar o menu, a nao ser que a opcao seja 0

    cout << "Encerrado.\n";
    return 0;   // fim do programa
}
