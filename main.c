#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORDS 100
#define MAX_WORD_LENGTH 256
#define MAX_FILES 100
#define MAX_FILENAME_LENGTH 256
#define MAX_PHRASE_LENGTH 1024

typedef struct response_per_file {
  char filename[MAX_FILENAME_LENGTH];
  int count;
} response_per_file_t;

typedef struct response {
  response_per_file_t responses_per_file[MAX_FILES];
} response_t;

int count_occurrences(const char* filename, const char* word) {
  FILE *file = fopen(filename, "r");
  int count = 0;
  char temp[MAX_WORD_LENGTH];
  
  if (!file) {
    fprintf(stderr, "Erro ao abrir o arquivo %s\n", filename);
    return -1;
  }

  while (fscanf(file, "%255s", temp) == 1) {
    if (strcmp(temp, word) == 0) {
      count++;
    }
  }
  
  fclose(file);
  return count;
}

void split_phrase(char* phrase, char words[MAX_WORDS][MAX_WORD_LENGTH], int* num_words) {
  char* token = strtok(phrase, " ");
  *num_words = 0;

  while (token != NULL) {
    if (*num_words < MAX_WORDS) {
      strncpy(words[*num_words], token, MAX_WORD_LENGTH - 1);
      words[*num_words][MAX_WORD_LENGTH - 1] = '\0';
      (*num_words)++;
    }
    token = strtok(NULL, " ");
  }
}

void scatter(int rank, char words[MAX_WORDS][MAX_WORD_LENGTH], int num_words) {
  if (rank == 0) {
    for (int word = 0; word < num_words; word++) {
      MPI_Send(words[word], MAX_WORD_LENGTH, MPI_CHAR, word, 0, MPI_COMM_WORLD);
      printf("Enviando palavra '%s' para a réplica %d\n", words[word], word + 1);
    }
  } else {
    MPI_Recv(words[rank - 1], MAX_WORD_LENGTH, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("Réplica %d recebeu palavra '%s'\n", rank, words[rank - 1]);
  }

  MPI_Barrier(MPI_COMM_WORLD);
}

void gather(response_t* response, response_t* responses, int rank, int size) {
  if (rank == 0) {
    for (int i = 1; i < size; i++) {
      MPI_Recv(&responses[i], sizeof(response_t), MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      printf("Nó raiz recebeu resposta da réplica %d\n", i);
    }
  } else {
    MPI_Send(response, sizeof(response_t), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    printf("Enviando resposta para o nó raiz\n");
  }

  MPI_Barrier(MPI_COMM_WORLD);
}

int main(int argc, char** argv) {
  int rank, size, num_words, num_files = 0;
  char search_phrase[MAX_PHRASE_LENGTH];
  char words[MAX_WORDS][MAX_WORD_LENGTH];
  char filenames[MAX_FILES][MAX_FILENAME_LENGTH];
  
  // verifica se há texto
  if (argc < 2) {
    fprintf(stderr, "Uso: %s <texto_de_busca>\n", argv[0]);
    return 1;
  }

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // argv[1] = "programacao paralela e distribuida"

  // copia o input de texto para a variável search_phrase
  strncpy(search_phrase, argv[1], MAX_PHRASE_LENGTH - 1);
  search_phrase[MAX_PHRASE_LENGTH - 1] = '\0';

  // search_phrase = "programacao paralela e distribuida"

  // divide a frase em palavras e armazena no array words
  split_phrase(search_phrase, words, &num_words);

  // words[0] = "programacao"
  // words[1] = "paralela"
  // words[2] = "e"
  // words[3] = "distribuida"
  // words = {"programacao", "paralela", "e", "distribuida"}

  // copiando o nome dos arquivos para o array filenames
  strncpy(filenames[num_files++], "./file1.txt", MAX_FILENAME_LENGTH - 1);
  strncpy(filenames[num_files++], "./file2.txt", MAX_FILENAME_LENGTH - 1);

  // num_files = 2
  // num_words = 4
  // 4 * 4 Bytes = 16 Bytes
  int* occurrences = calloc(num_files * num_words, sizeof(int));

  // *words[0] enviar para rank 1
  // *words[1] enviar para rank 2
  // *words[2] enviar para rank 3
  // *words[3] enviar para rank 4

  scatter(rank, words, num_words);

  // para word[x], file[a] = count_occurrences(filenames[y], words[x])
  // para word[x], file[b] = count_occurrences(filenames[y], words[x])
  // para word[y], file[a] = count_occurrences(filenames[y], words[x])
  // para word[y], file[b] = count_occurrences(filenames[y], words[x])

  response_t response;

  printf("Réplica %d com a palavra '%s'\n", rank, words[rank]);
  for (int i = 0; i < num_files; i++) {
    response_per_file_t response_per_file;
occurrences[i * num_words + rank] = count_occurrences(filenames[i], words[rank]);
    printf("('%s', %d) ('%s')\n", filenames[i], occurrences[i * num_words + rank], words[rank]);

    strcpy(response_per_file.filename, filenames[i]);
    response_per_file.count = occurrences[i * num_words + rank];
    response.responses_per_file[i] = response_per_file;
  }

  response_t* responses = NULL;

  if (rank == 0) {
    responses = malloc(num_words * num_files * sizeof(response_t));
  }

  gather(&response, responses, rank, size);
  
  // O arquivo ./file1.txt contém 5 ocorrências da palavra 'exemplo'
  // O arquivo ./file1.txt contém 1 ocorrências da palavra 'texto'
  // O arquivo ./file2.txt contém 1 ocorrências da palavra 'exemplo'
  // O arquivo ./file2.txt contém 0 ocorrências da palavra 'texto'

  // "./file1.txt", "Outro exemplo de arquivo de texto exemplo exemplo.\n
  // Aqui também temos algumas palavras para busca.exemplo\n\n
  // exemplo exemplo\n
  // exemplo", 6

  // "./file2.txt", "Este é um exemplo de arquivo de texto.\n
  // Ele contém palavras aleatórias para teste.\n", 1

  if (rank == 0) {
    printf("Resultado final\n");

    for (int k = 0; k < num_files; k++) {
      int occurrences_per_file = 0;
      
      for (int l = 0; l < num_words; l++) {
        occurrences_per_file += occurrences[k * num_words + l];
      }

      printf("{'%s',  %d}\n", filenames[k], occurrences_per_file);
    }
    free(responses);
  }

  MPI_Finalize();

  free(occurrences);

  return 0;
}
