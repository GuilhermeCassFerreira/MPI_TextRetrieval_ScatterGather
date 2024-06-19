#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORDS 100 // Número máximo de palavras na frase de busca
#define MAX_WORD_LENGTH 256 // Comprimento máximo de cada palavra
#define MAX_FILES 100
#define MAX_FILENAME_LENGTH 256

// Função para contar ocorrências de uma palavra em um arquivo
int count_occurrences(const char* filename, const char* word) {
    FILE *file = fopen(filename, "r");
    if (!file) return -1; // Retorna -1 se não conseguir abrir o arquivo
    
    int count = 0;
    char temp[512];
    while (fscanf(file, "%s", temp) != EOF) { // Lê cada palavra do arquivo
        if (strcmp(temp, word) == 0) { // Compara com a palavra buscada
            count++; // Incrementa o contador se encontrar a palavra
        }
    }
    
    fclose(file); // Fecha o arquivo
    return count; // Retorna o número de ocorrências
}

// Função para dividir a frase em palavras
void split_phrase(char* phrase, char words[MAX_WORDS][MAX_WORD_LENGTH], int* num_words) {
    char* token = strtok(phrase, " ");
    *num_words = 0;
    while (token != NULL) {
        strncpy(words[*num_words], token, MAX_WORD_LENGTH);
        words[*num_words][MAX_WORD_LENGTH - 1] = '\0'; // Garantir terminação correta da string
        (*num_words)++;
        token = strtok(NULL, " ");
    }
}

int main(int argc, char** argv) {
    int rank, size, i;

    MPI_Init(&argc, &argv); // Inicializa o ambiente MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Obtém o rank do processo
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Obtém o número total de processos

    if (argc < 2) { // Verifica se foi passada a frase de busca como argumento
        if (rank == 0) {
            fprintf(stderr, "Uso: %s <frase_de_busca>\n", argv[0]);
        }
        MPI_Finalize();
        return 1; // Encerra o programa se não houver frase de busca
    }

    char search_phrase[1024];
    strncpy(search_phrase, argv[1], 1024); // Copia a frase de busca
    search_phrase[1023] = '\0'; // Garantir terminação correta da string

    char words[MAX_WORDS][MAX_WORD_LENGTH];
    int num_words;

    // Processo mestre divide a frase em palavras e distribui
    if (rank == 0) {
        split_phrase(search_phrase, words, &num_words);

        // Envia a quantidade de palavras e a lista de palavras para todos os processos
        MPI_Bcast(&num_words, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(words, num_words * MAX_WORD_LENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);
    } else {
        // Processos réplicas recebem a quantidade de palavras e a lista de palavras
        MPI_Bcast(&num_words, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(words, num_words * MAX_WORD_LENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);
    }

    char filenames[MAX_FILES][MAX_FILENAME_LENGTH];
    int num_files = 0;

    if (rank == 0) { // Processo mestre
        // Especifica os caminhos dos arquivos a serem buscados
        strcpy(filenames[num_files++], "/home/bridge/MPI_TextRetrieval_ScatterGather/file1.txt");
        strcpy(filenames[num_files++], "/home/bridge/MPI_TextRetrieval_ScatterGather/file2.txt");
        // Adicionar mais arquivos conforme necessário

        // Envia o número de arquivos para todos os processos
        MPI_Bcast(&num_files, 1, MPI_INT, 0, MPI_COMM_WORLD);
        // Envia os nomes dos arquivos para todos os processos
        MPI_Bcast(filenames, num_files * MAX_FILENAME_LENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);
    } else {
        // Processos réplicas recebem o número de arquivos e nomes dos arquivos
        MPI_Bcast(&num_files, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(filenames, num_files * MAX_FILENAME_LENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);
    }

    // Cada processo conta ocorrências em sua parte dos arquivos
    int files_per_process = num_files / size; // Quantos arquivos cada processo cuidará
    int extra_files = num_files % size; // Quantidade extra de arquivos para alguns processos
    int start = rank * files_per_process + (rank < extra_files ? rank : extra_files); // Início da fatia de arquivos para este processo
    int end = start + files_per_process + (rank < extra_files ? 1 : 0); // Fim da fatia de arquivos

    int* occurrences = calloc(num_files * num_words, sizeof(int)); // Vetor para armazenar o número de ocorrências por arquivo e por palavra
    
    // Conta as ocorrências nos arquivos atribuídos a este processo
    for (i = start; i < end; i++) {
        for (int j = 0; j < num_words; j++) {
            occurrences[i * num_words + j] = count_occurrences(filenames[i], words[j]);
        }
    }

    // Recolhe os resultados no nó raiz
    int* all_occurrences = NULL;
    if (rank == 0) {
        all_occurrences = calloc(num_files * num_words, sizeof(int)); // Vetor para armazenar todas as ocorrências
    }
    
    // Reduz os dados (soma as ocorrências de todos os processos) no processo raiz
    MPI_Reduce(occurrences, all_occurrences, num_files * num_words, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        // Imprime os resultados no processo raiz
        for (int j = 0; j < num_files; j++) {
            for (int k = 0; k < num_words; k++) {
                printf("Arquivo: %s, Palavra: %s, Ocorrências: %d\n", filenames[j], words[k], all_occurrences[j * num_words + k]);
            }
        }
        free(all_occurrences); // Libera memória alocada
    }

    free(occurrences); // Libera memória alocada
    MPI_Finalize(); // Finaliza o ambiente MPI
    return 0;
}

// Compilar: mpicc -o search_mpi search_mpi.c
// Executar: mpirun -np 4 ./search_mpi "frase de busca"
