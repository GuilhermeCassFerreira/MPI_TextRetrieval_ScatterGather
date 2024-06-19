#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char** argv) {
    int rank, size, i;

    if (argc < 2) { // Verifica se foi passada a palavra de busca como argumento
        if (rank == 0) {
            fprintf(stderr, "Uso: %s <palavra_de_busca>\n", argv[0]);
        }
        return 1; // Encerra o programa se não houver palavra de busca
    }

    MPI_Init(&argc, &argv); // Inicializa o ambiente MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Obtém o rank do processo
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Obtém o número total de processos

    const int MAX_FILES = 100;
    const int MAX_FILENAME_LENGTH = 256;

    char search_word[256];
    strncpy(search_word, argv[1], 256); // Copia a palavra de busca
    search_word[255] = '\0'; // Garantir terminação correta da string

    char filenames[MAX_FILES][MAX_FILENAME_LENGTH];
    int num_files = 0;

    if (rank == 0) { // Processo mestre
        // Especifica os caminhos dos arquivos a serem buscados
        strcpy(filenames[num_files++], "caminho/para/o/file1.txt");
        strcpy(filenames[num_files++], "caminho/para/o/file2.txt");
        // Adicionar mais arquivos conforme necessário

        // Envia a palavra de busca para todos os processos
        MPI_Bcast(search_word, 256, MPI_CHAR, 0, MPI_COMM_WORLD);
        
        // Envia o número de arquivos para todos os processos
        MPI_Bcast(&num_files, 1, MPI_INT, 0, MPI_COMM_WORLD);
        // Envia os nomes dos arquivos para todos os processos
        MPI_Bcast(filenames, num_files * MAX_FILENAME_LENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);
    } else {
        // Processos réplicas recebem a palavra de busca, número de arquivos e nomes dos arquivos
        MPI_Bcast(search_word, 256, MPI_CHAR, 0, MPI_COMM_WORLD);
        MPI_Bcast(&num_files, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(filenames, num_files * MAX_FILENAME_LENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);
    }

    // Cada processo conta ocorrências em sua parte dos arquivos
    int files_per_process = num_files / size; // Quantos arquivos cada processo cuidará
    int extra_files = num_files % size; // Quantidade extra de arquivos para alguns processos
    int start = rank * files_per_process + (rank < extra_files ? rank : extra_files); // Início da fatia de arquivos para este processo
    int end = start + files_per_process + (rank < extra_files ? 1 : 0); // Fim da fatia de arquivos

    int* occurrences = calloc(num_files, sizeof(int)); // Vetor para armazenar o número de ocorrências por arquivo
    
    // Conta as ocorrências nos arquivos atribuídos a este processo
    for (i = start; i < end; i++) {
        occurrences[i] = count_occurrences(filenames[i], search_word);
    }

    // Recolhe os resultados no nó raiz
    int* all_occurrences = NULL;
    if (rank == 0) {
        all_occurrences = calloc(num_files, sizeof(int)); // Vetor para armazenar todas as ocorrências
    }
    
    // Reduz os dados (soma as ocorrências de todos os processos) no processo raiz
    MPI_Reduce(occurrences, all_occurrences, num_files, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        // Imprime os resultados no processo raiz
        for (int j = 0; j < num_files; j++) {
            printf("Arquivo: %s, Ocorrências: %d\n", filenames[j], all_occurrences[j]);
        }
        free(all_occurrences); // Libera memória alocada
    }

    free(occurrences); // Libera memória alocada
    MPI_Finalize(); // Finaliza o ambiente MPI
    return 0;
}

//mpicc -o search_mpi search_mpi.c
//mpirun -np 4 ./search_mpi example
