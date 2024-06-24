# Sistema de Recuperação de Texto com MPI

## Funcionamento

O programa é projetado para encontrar ocorrências de palavras-chave em arquivos de texto distribuídos entre processos paralelos. Detalhes adicionais de implementação, compilação e execução são fornecidos abaixo.

## Evolução do Código

1. **Implementação Sequencial**: Inicialmente, o código foi desenvolvido de forma sequencial em C. Isso envolveu a criação de funções para contar ocorrências de palavras em arquivos de texto e para dividir uma frase em palavras.

2. **Adaptação para MPI**: Posteriormente, o sistema foi modificado para utilizar MPI, permitindo a paralelização do processamento. A função principal foi adaptada para inicializar o MPI, distribuir as palavras-chave entre os processos MPI usando scatter, coletar os resultados usando gather e exibir os resultados finais.

Este projeto implementa um serviço de recuperação de texto utilizando o padrão scatter/gather com MPI (Message Passing Interface). Ele divide uma frase de busca em palavras-chave e distribui o processamento entre múltiplos processos MPI, cada um responsável por contar as ocorrências das palavras em arquivos de texto específicos.

## Funções Implementadas

- **count_occurrences**: Esta função recebe o nome de um arquivo e uma palavra. Ela abre o arquivo, conta quantas vezes a palavra ocorre no arquivo e retorna o número de ocorrências.

- **split_phrase**: Esta função recebe uma frase como entrada e a divide em palavras individuais. Ela armazena as palavras em um array bidimensional `words` e atualiza o valor de `num_words` para indicar quantas palavras foram encontradas na frase.

- **scatter**: Esta função é responsável por distribuir as palavras-chave entre os processos MPI. No processo com rank 0, ela envia cada palavra individualmente para os outros processos utilizando `MPI_Send`. Nos outros processos, as palavras são recebidas através de `MPI_Recv`.

- **gather**: Esta função é responsável por coletar os resultados de cada processo MPI após a contagem das ocorrências das palavras-chave nos arquivos de texto.

- **load_configuration**: Esta função lê um arquivo de configuração para carregar os nomes dos arquivos de texto e a frase de busca.

## Detalhes de Implementação MPI

- **MPI_Init**: Inicia o MPI, determinando o rank (identificação do processo) e o tamanho do grupo.

- **MPI_Bcast**: Utilizado para enviar a frase de busca do processo raiz para todos os outros processos MPI.

- **MPI_Send/MPI_Recv**: Utilizados na função scatter para enviar e receber palavras-chave entre os processos MPI.

- **MPI_Barrier**: Sincroniza todos os processos MPI para garantir que nenhum processo prossiga antes que todos tenham alcançado um ponto específico no código.

- **MPI_Finalize**: Finaliza o ambiente MPI ao término do programa.

## Compilação e Execução

Para compilar o código, utilize o seguinte comando:

```bash
mpicc -o text_retrieval text_retrieval.c
```
Para compilar o código, utilize o seguinte comando:

```bash
mpirun -np <number_of_processes> ./text_retrieval
```
Substitua `<number_of_processes>` pelo número de processos MPI que você deseja utilizar.

## Exemplo de Uso

Crie um arquivo `config.txt` no mesmo diretório do código com o seguinte conteúdo:

```text
# Arquivos de texto
./file1.txt
./file2.txt
```

Este arquivo deve listar os nomes dos arquivos de texto, um por linha. Linhas começando com # são consideradas comentários e serão ignoradas.

Ao executar o programa, ele carregará a configuração, distribuirá as palavras-chave entre os processos MPI, realizará a contagem de ocorrências e coletará os resultados para exibir no nó raiz.
Saída Esperada

O programa exibirá a frase de busca e os arquivos de texto processados. Em seguida, para cada arquivo, ele exibirá o número total de ocorrências das palavras-chave.


Frase de busca: programacao paralela e distribuida
Arquivo de texto: ./file1.txt
Arquivo de texto: ./file2.txt
Réplica 1 com a palavra 'programacao'
('./file1.txt', 2) ('programacao')
('./file2.txt', 1) ('programacao')
...
Resultado final
{'./file1.txt',  5}
{'./file2.txt',  2}

## Estudantes

- Bruno Vazquez Lafaiete
- Cauã Pablo Padilha
- Guilherme Cassiano Ferreira

## Docentes

- Odorico Machado Mendizabal
- Douglas Pereira Luiz

Este é um exemplo de como a saída pode ser exibida, mostrando o número total de ocorrências das palavras-chave em cada arquivo de texto.
