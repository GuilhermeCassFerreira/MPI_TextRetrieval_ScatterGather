#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORDS 100
#define MAX_WORD_LENGTH 256
#define MAX_FILES 100
#define MAX_FILENAME_LENGTH 256

// Função para contar ocorrências de uma palavra em um arquivo
int count_occurrences(const char* filename, const char* word) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", filename);
        return -1;
    }
    
    int count = 0;
    char temp[MAX_WORD_LENGTH];
    while (fscanf(file, "%255s", temp) == 1) { // Lê cada palavra do arquivo
        if (strcmp(temp, word) == 0) { // Compara com a palavra buscada
            count++;
        }
    }
    
    fclose(file);
    return count;
}

// Função para dividir a frase em palavras e salvar em um array de strings
void split_phrase(char* phrase, char words[MAX_WORDS][MAX_WORD_LENGTH], int* num_words) {
    char* token = strtok(phrase, " ");
    *num_words = 0;
    while (token != NULL) {
        if (*num_words < MAX_WORDS) {
            strncpy(words[*num_words], token, MAX_WORD_LENGTH - 1);
            words[*num_words][MAX_WORD_LENGTH - 1] = '\0'; // Garantir terminação correta da string
            (*num_words)++;
        }
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
    strncpy(search_phrase, argv[1], sizeof(search_phrase) - 1);
    search_phrase[sizeof(search_phrase) - 1] = '\0'; // Garantir terminação correta da string

    char words[MAX_WORDS][MAX_WORD_LENGTH];
    int num_words;

    // Processo mestre divide a frase em palavras e distribui
    if (rank == 0) {
        split_phrase(search_phrase, words, &num_words);
        printf("Nó raiz dividindo a frase em palavras...\n");

        // Envia a quantidade de palavras para todos os processos réplicas
        for (i = 1; i < size; i++) {
            MPI_Send(&num_words, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            printf("Nó raiz enviou quantidade de palavras (%d) para o processo %d\n", num_words, i);
        }

        // Envia as palavras para todos os processos réplicas
        for (i = 1; i < size; i++) {
            MPI_Send(words, num_words * MAX_WORD_LENGTH, MPI_CHAR, i, 0, MPI_COMM_WORLD);
            printf("Nó raiz enviou palavras para o processo %d\n", i);
        }
    } else {
        // Processos réplicas recebem a quantidade de palavras
        MPI_Recv(&num_words, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Processo %d recebeu quantidade de palavras (%d) do nó raiz\n", rank, num_words);

        // Processos réplicas recebem as palavras
        MPI_Recv(words, num_words * MAX_WORD_LENGTH, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Processo %d recebeu as palavras do nó raiz\n", rank);
    }

    char filenames[MAX_FILES][MAX_FILENAME_LENGTH];
    int num_files = 0;

    if (rank == 0) { // Processo mestre
        // Especifica os caminhos dos arquivos a serem buscados
        strncpy(filenames[num_files++], "/home/bridge/MPI_TextRetrieval_ScatterGather/file1.txt", MAX_FILENAME_LENGTH - 1);
        strncpy(filenames[num_files++], "/home/bridge/MPI_TextRetrieval_ScatterGather/file2.txt", MAX_FILENAME_LENGTH - 1);
        // Adicionar mais arquivos conforme necessário

        // Envia o número de arquivos para todos os processos réplicas
        for (i = 1; i < size; i++) {
            MPI_Send(&num_files, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            printf("Nó raiz enviou quantidade de arquivos (%d) para o processo %d\n", num_files, i);
        }

        // Envia os nomes dos arquivos para todos os processos réplicas
        for (i = 1; i < size; i++) {
            MPI_Send(filenames, num_files * MAX_FILENAME_LENGTH, MPI_CHAR, i, 0, MPI_COMM_WORLD);
            printf("Nó raiz enviou lista de arquivos para o processo %d\n", i);
        }
    } else {
        // Processos réplicas recebem o número de arquivos
        MPI_Recv(&num_files, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Processo %d recebeu quantidade de arquivos (%d) do nó raiz\n", rank, num_files);

        // Processos réplicas recebem os nomes dos arquivos
        MPI_Recv(filenames, num_files * MAX_FILENAME_LENGTH, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Processo %d recebeu lista de arquivos do nó raiz\n", rank);
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
            int count = count_occurrences(filenames[i], words[j]);
            if (count == -1) {
                // tratamento de erro ao contar ocorrências
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            occurrences[i * num_words + j] = count;
            printf("Processo %d contou %d ocorrências da palavra '%s' no arquivo '%s'\n", rank, occurrences[i * num_words + j], words[j], filenames[i]);
        }
    }

    // Envia os resultados de volta para o processo raiz
    if (rank != 0) {
        MPI_Send(occurrences, num_files * num_words, MPI_INT, 0, 0, MPI_COMM_WORLD);
        printf("Processo %d enviou seus resultados ao nó raiz\n", rank);
    } else {
        // Processo raiz recebe resultados dos processos réplicas
        for (i = 1; i < size; i++) {
            MPI_Recv(occurrences + i * files_per_process * num_words, files_per_process * num_words, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("Nó raiz recebeu resultados do processo %d\n", i);
        }

        // Se houver arquivos extras, recebemos do último processo
        if (extra_files > 0) {
            MPI_Recv(occurrences + size * files_per_process * num_words, extra_files * num_words, MPI_INT, size - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("Nó raiz recebeu resultados do processo %d\n", size - 1);
        }

        // Processo raiz imprime os resultados
        for (int j = 0; j < num_files; j++) {
           
        // Processo raiz imprime os resultados
        for (int j = 0; j < num_files; j++) {
            for (int k = 0; k < num_words; k++) {
                printf("Arquivo: %s, Palavra: %s, Ocorrências: %d\n", filenames[j], words[k], occurrences[j * num_words + k]);
            }
        }
    }

    free(occurrences); // Libera memória alocada

    MPI_Finalize(); // Finaliza o ambiente MPI
    return 0;
}
