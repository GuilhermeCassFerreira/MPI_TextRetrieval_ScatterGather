#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORDS 100
#define MAX_WORD_LENGTH 256
#define MAX_FILES 100
#define MAX_FILENAME_LENGTH 256
#define MAX_PHRASE_LENGTH 1024

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

int main(int argc, char** argv) {
  int num_words, num_files = 0;
  char search_phrase[MAX_PHRASE_LENGTH];
  char words[MAX_WORDS][MAX_WORD_LENGTH];
  char filenames[MAX_FILES][MAX_FILENAME_LENGTH];
  
  // verifica se há texto
  if (argc < 2) {
    fprintf(stderr, "Uso: %s <texto_de_busca>\n", argv[0]);
    return 1;
  }
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

  // para word[x], file[a] = count_occurrences(filenames[y], words[x])
  // para word[x], file[b] = count_occurrences(filenames[y], words[x])
  // para word[y], file[a] = count_occurrences(filenames[y], words[x])
  // para word[y], file[b] = count_occurrences(filenames[y], words[x])

  for (int i = 0; i < num_files; i++) {
    for (int j = 0; j < num_words; j++) {
      occurrences[i * num_words + j] = count_occurrences(filenames[i], words[j]);
      printf("('%s', %d) ('%s')\n", filenames[i], occurrences[i * num_words + j], words[j]);
    }
  }

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

  printf("Resultado final\n");

  for (int k = 0; k < num_files; k++) {
    int ocurrences_per_file = 0;
    
    for (int l = 0; l < num_words; l++) {
      ocurrences_per_file += occurrences[k * num_words + l];
    }

    printf("{'%s',  %d}\n", filenames[k], ocurrences_per_file);
  }

  free(occurrences);

  return 0;
}