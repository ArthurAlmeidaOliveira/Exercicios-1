#include <iostream>
#include <vector>
#include <limits>
#include <locale.h>
#include <string>
#include <cstdlib>
#include <windows.h>
using namespace std;

// Lê um nome: aceita letras, acentos e nomes compostos; recusa números e vazio
string lerNome(string mensagem) {
    string nome;
    while (true) { //loop infinito
        cout << mensagem;
        getline(cin, nome);

        // remove espaços do começo e do final
        size_t inicio = nome.find_first_not_of(' ');
        size_t fim = nome.find_last_not_of(' ');

        if (inicio == string::npos) {
            nome.clear();                                 // só tinha espaços
        } else {
            nome = nome.substr(inicio, fim - inicio + 1); // pega só a parte útil
        }

        // troca espaços repetidos no meio por um só
        string limpo;
        for (int i = 0; i < nome.length(); i++) {
            if (nome[i] == ' ' && i > 0 && nome[i - 1] == ' ') {
                continue;                                 // pula o espaço repetido
            }
            limpo += nome[i];
        }
        nome = limpo;

        if (nome.empty()) {
            system("cls");
            cout << "Nome inválido! Não pode ser vazio.\n";
            continue;
        }

        // verifica se existe algum número no nome
        bool temNumero = false;
        for (int i = 0; i < nome.length(); i++) {
            if (nome[i] >= '0' && nome[i] <= '9') {       // é um dígito de 0 a 9?
                temNumero = true;
                break;
            }
        }

        if (temNumero) {
            system("cls");
            cout << "Nome inválido! Não digite números.\n";
            continue;
        }

        system("cls");
        return nome;
    }
}

// Lê uma nota, impedindo letras
double lerNota(string mensagem) {
    double nota;
    while (true) { //loop infinito
        cout << mensagem;
        cin >> nota;

        if (cin.fail() || nota < 0 || nota > 10) {
    	cin.clear();
    	cin.ignore(numeric_limits<streamsize>::max(), '\n');
    	system("cls");
    	cout << "Entrada inválida! Digite uma nota entre 0 e 10.\n";
		} else {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            system("cls");
            return nota;
        }
    }
}

// Lê a opção do menu valida o intervalo
int lerOpcao(string mensagem, int min, int max) {
    string entrada;
    while (true) {
        cout << mensagem;
        getline(cin, entrada);
        
        //Retira os espacos no final da string
        size_t pos = entrada.find_last_not_of(' ');
        if (pos != string::npos) {
            entrada.erase(pos + 1);
        } else {
            entrada.clear();
        }
		
		//Verifica se for vazio
		if (entrada.empty()) {
            cout << "Opção inválido! Não pode ser vazio.\n\n";
            continue;
        }
        
        // Verifica se tem exatamente 1 caractere
        if (entrada.length() != 1) {
            cout << "Opção inválida! Digite apenas um número entre " << min << " e " << max << ".\n\n";
            continue;
        }
        
        int opcao = entrada[0] - '0'; // converte o caractere para o número inteiro correspondente

		//verifica se foi digitado 1-3
        if (opcao < min || opcao > max) {
            cout << "Opção inválida! Escolha um número entre " << min << " e " << max << ".\n\n";
            continue;
        }

        return opcao;
    }
}

// Pergunta se o usuário realmente quer sair
bool confirmarSaida() {
	system("cls");
    string letra;
    while (true) {
        cout << "\nTem certeza que deseja sair? S para sim / N para não (S/N): ";
        getline(cin, letra);

        if (letra.length() != 1) {
        	system("cls");
            cout << "Resposta inválida! Digite apenas uma letra: 's' ou 'n'.\n";
            continue;
        }

        if (letra == "s" || letra == "S") {
        	system("cls");
            return true;
        } else if (letra == "n" || letra == "N") {
        	system("cls");
            return false;
        } else {
        	system("cls");
            cout << "Resposta inválida! Digite 's' para sim ou 'n' para não.\n";
        }
    }
}

// Cadastra um único aluno (nome + duas notas + média) nos vetores
void cadastrarAluno(vector<string>& nomes, vector<double>& notas1, vector<double>& notas2, vector<double>& medias) {
    system("cls");
    string nome = lerNome("Digite o nome do aluno: ");
    double nota1 = lerNota("Digite a primeira nota do aluno: ");
    double nota2 = lerNota("Digite a segunda nota do aluno: ");
    double media = (nota1 + nota2) / 2;

    nomes.push_back(nome);
    notas1.push_back(nota1);
    notas2.push_back(nota2);
    medias.push_back(media);

    system("cls");
    cout << "Aluno cadastrado com sucesso!\n";
}

// Exibe o relatório com todos os alunos cadastrados
void exibirRelatorio(const vector<string>& nomes, const vector<double>& notas1, const vector<double>& notas2, const vector<double>& medias) {
    system("cls");
    if (nomes.empty()) {
        cout << "Nenhum aluno cadastrado ainda.\n";
        return;
    }

    cout << "\nRELATÓRIO DE ALUNOS\n";
    for (int i = 0; i < nomes.size(); i++) {
        cout << "Nome:" << nomes[i] << " AV 01:" << notas1[i] << " AV 02:" << notas2[i] << " Média:" << medias[i] << '\n';
    }
}

// Exibe o menu
void menu() {
    cout << "\n---------------------\n";
    cout << "FERRAMENTA DE NOTAS\n";
    cout << "# Digite um número de 1-3 #\n";
    cout << "1. Cadastrar aluno\n";
    cout << "2. Relatório\n";
    cout << "3. Encerrar programa\n";
    cout << "---------------------\n";
    cout << '\n';
}

int main() {
	SetConsoleCP(1252);
    SetConsoleOutputCP(1252);
    setlocale(LC_ALL, "Portuguese");

    vector<string> nomes;
    vector<double> notas1;
    vector<double> notas2;
    vector<double> medias;

    int opcao;
    bool sair = false;

	//loop enquanto opcao != 3
    do {
        menu();
        opcao = lerOpcao("Escolha uma opção: ", 1, 3); //chama a funcao ler opcao

        switch (opcao) { //switch com 3 opcoes
            case 1:
                cadastrarAluno(nomes, notas1, notas2, medias); //chama funcao cadastrarAluno
                break;
            case 2:
                exibirRelatorio(nomes, notas1, notas2, medias); //chama funcao exibirRelatorio
                break;
            case 3:
                sair = confirmarSaida(); //chama funcao confirmarSaida
                if (sair) { //condicao baseada no retorno da funcao confirmarSaida
                    cout << "\nEncerrando o programa\n";
                } else {
                    system("cls");
                }
                break;
        }

    } while (!sair);

    return 0;
}
