# Check Point 2 — Projeto Motiva: Atualização Remota de Firmware (OTA)

Projeto desenvolvido na **FIAP**, na turma **2CCPO**, para a disciplina do professor **Marcelo Fernando Morgantini**.

A solução utiliza um **ESP32 simulado no Wokwi** para representar um nó de monitoramento da altura da vegetação. Após executar três sessões de leitura, o Firmware 1.0 consulta um repositório remoto e instala o Firmware 2.0 por OTA, sem troca manual do programa durante a execução.

---

## Integrantes

| Integrante | RM |
| --- | --- |
| Giovanna Fernandes Pereira | 565434 |
| João Pedro de Moura Albino | 565323 |
| Kauê Silva Matheus | 561675 |

**Turma:** 2CCPO

---

## Links do Projeto

- [Simulação no Wokwi](https://wokwi.com/projects/475891405739849729)
- [Repositório no GitHub](https://github.com/jalbino0/motiva-ota)
- [Manifesto de versão](https://raw.githubusercontent.com/jalbino0/motiva-ota/main/version.json)
- [Firmware 2.0 compilado](https://raw.githubusercontent.com/jalbino0/motiva-ota/main/firmware_v2.bin)

---

## Sobre o Projeto

Um dispositivo instalado em campo pode precisar de novas funcionalidades sem que uma pessoa se desloque até ele para conectar um computador. A atualização OTA permite obter e instalar o novo firmware pela rede.

Neste trabalho, as alturas são **simuladas por valores pseudoaleatórios inteiros entre 10 e 20 cm**. Não há sensor físico de vegetação.

O tempo foi acelerado para a demonstração: cada sessão começa 48 segundos após o início da anterior e contém cinco leituras, espaçadas por dois segundos.

---

## Arquitetura da Solução

```text
ESP32 no Wokwi — Firmware 1.0 / LED azul
    |
    | Executa três sessões completas
    v
Consulta HTTPS ao version.json no GitHub
    |
    | Compara a versão instalada com a disponível
    v
Download de firmware_v2.bin
    |
    | Grava a partição OTA e seleciona o próximo boot
    v
Reinício automático do ESP32
    |
    v
Firmware 2.0 — média, ordenação, mediana e histerese
    |-- NORMAL: LED verde
    |-- ALERTA: LED vermelho
```

O ESP32 utiliza a rede virtual **Wokwi-GUEST**. O manifesto e o binário estão disponíveis por URLs públicas de download direto do GitHub.

---

## Firmware 1.0

- Identifica a versão no monitor serial.
- Mantém o LED RGB azul.
- Gera e armazena cinco leituras em um vetor.
- Exibe cada leitura e calcula a média aritmética.
- Controla a temporização com `millis()`.
- Após três sessões completas, consulta o manifesto remoto.
- Compara versões numéricas no formato `maior.menor`.
- Quando existe uma versão mais nova, baixa o binário, realiza a OTA e reinicia.

A consulta OTA é realizada uma vez por execução do Firmware 1.0. Se ela falhar ou não houver versão mais nova, o programa continua as sessões de monitoramento.

## Temporização

| Instante relativo ao início da sessão | Ação |
| --- | --- |
| 0 s | Primeira leitura |
| 2 s | Segunda leitura |
| 4 s | Terceira leitura |
| 6 s | Quarta leitura |
| 8 s | Quinta leitura e processamento |
| 48 s | Início da próxima sessão |

O intervalo de 48 segundos é contado entre os inícios das sessões, e não após a última leitura. A consulta OTA começa após a quinta leitura da terceira sessão.

---

## Firmware 2.0

A nova versão mantém as leituras e a média e acrescenta:

- Cópia do vetor para preservar a ordem original.
- Ordenação crescente implementada no programa.
- Exibição dos valores originais e ordenados.
- Mediana obtida pelo terceiro elemento do vetor ordenado.
- Controle dos estados NORMAL e ALERTA por histerese.
- Indicação visual verde ou vermelha.

### Média e mediana

A média é a soma das cinco leituras dividida por cinco. A mediana é o valor central após a ordenação e sofre menos influência de valores extremos.

Exemplo observado após a atualização OTA:

```text
Ordem original: 17 14 10 18 11
Ordem crescente: 10 11 14 17 18
Media: 14.0 cm
Mediana: 14 cm
Estado: NORMAL
```

### Histerese

| Mediana da sessão | Comportamento | LED |
| --- | --- | --- |
| Maior ou igual a 16 cm | Entra em ALERTA | Vermelho |
| Maior que 14 e menor que 16 cm | Mantém o estado anterior | Mantém a cor |
| Menor ou igual a 14 cm | Entra ou retorna a NORMAL | Verde |

O estado inicial do Firmware 2.0 é NORMAL. Como as leituras são inteiras, a mediana na faixa intermediária é 15 cm. A histerese evita alternâncias desnecessárias quando a medida está próxima dos limites.

---

## Circuito

O circuito utiliza um ESP32 e um LED RGB de cátodo comum.

| Terminal do LED | Ligação |
| --- | --- |
| R — vermelho | GPIO 27 |
| G — verde | GPIO 26 |
| B — azul | GPIO 25 |
| COM | GND |

O arquivo `diagram.json` descreve o circuito usado na demonstração.

---

## Manifesto de Atualização

O arquivo `version.json` contém:

```json
{
  "version": "2.0",
  "url": "https://raw.githubusercontent.com/jalbino0/motiva-ota/main/firmware_v2.bin"
}
```

O ESP32 lê a versão e a URL desse arquivo. O endereço `raw.githubusercontent.com` fornece o conteúdo diretamente, sem a página de navegação do GitHub.

## Partições OTA

O arquivo `partitions.csv` reserva duas partições de aplicação:

- `app0`, em `0x10000`: firmware inicial.
- `app1`, em `0x150000`: destino da primeira atualização.
- `otadata`: informações usadas para selecionar a aplicação no próximo boot.

Na demonstração, o diagnóstico indicou `app0` em execução e `app1` selecionada após a gravação. Em seguida, o monitor serial identificou o Firmware 2.0.

---

## Estrutura do Repositório

```text
motiva-ota/
├── README.md
├── diagram.json
├── partitions.csv
├── firmware_v1.cpp
├── firmware_v2.cpp
├── firmware_v2.bin
└── version.json
```

Os arquivos `.cpp` contêm os códigos-fonte. O `.bin` é o programa compilado usado na OTA. No projeto online do Wokwi, o código inicial está em `sketch.ino`.

---

## Como Executar

1. Abra o [projeto Wokwi](https://wokwi.com/projects/475891405739849729).
2. Inicie a simulação e mantenha a aba aberta, com o monitor serial visível.
3. Confirme a identificação `FW 1.0` e o LED azul.
4. Aguarde três sessões completas, com 48 segundos entre os inícios.
5. Observe a consulta ao manifesto, a identificação da versão 2.0 e a gravação OTA.
6. Aguarde o reinício automático, sem trocar o código nem carregar manualmente o firmware.
7. Confirme a identificação `FW 2.0`, a ordenação, a média, a mediana e o estado do sistema.

O tempo de compilação e download depende dos serviços externos. A terceira sessão termina aproximadamente 104 segundos após o início da primeira, antes do tempo gasto na atualização.

### Recriar o projeto online

Use o conteúdo de `firmware_v1.cpp` como `sketch.ino`, adicione `diagram.json` e `partitions.csv` e instale **ArduinoJson 7.x** pelo Library Manager. O binário da versão 2.0 deve continuar disponível no endereço informado pelo manifesto.

### Ambiente de desenvolvimento

Os programas foram compilados localmente com **VS Code, PlatformIO, placa esp32dev e framework Arduino**. A demonstração de OTA com reinício no Firmware 2.0 foi confirmada no **Wokwi do navegador**. O reinício OTA na extensão do VS Code apresentou falha durante os testes e não é apresentado como validado.

---

## Bibliotecas Utilizadas

| Biblioteca | Finalidade |
| --- | --- |
| Arduino.h | Funções básicas, GPIO e temporização |
| WiFi.h | Conexão Wi-Fi |
| WiFiClientSecure.h | Cliente HTTPS |
| HTTPClient.h | Consulta ao manifesto |
| HTTPUpdate.h | Download e gravação da atualização |
| ArduinoJson 7.x | Leitura e validação dos campos JSON |
| esp_ota_ops.h | Diagnóstico das partições OTA |

Para a demonstração acadêmica, o cliente utiliza `setInsecure()`: a conexão é HTTPS, mas o certificado do servidor não é validado. Uma implantação real precisa validar a identidade do servidor e a autenticidade do firmware.

---

## Resultados e Testes

| Verificação | Evidência disponível |
| --- | --- |
| FW 1.0: cinco leituras, média e LED azul | Observado durante os testes |
| Intervalo de 48 segundos | Inícios registrados em 4.396, 52.396 e 100.396 ms |
| Consulta somente após três sessões | Log de três sessões antes da consulta OTA |
| Download e reinício no FW 2.0 | Confirmado no Wokwi do navegador |
| Ordenação, média e mediana | Conferidas com os valores exibidos após o reboot |
| ALERTA e LED vermelho | Observados no teste direto do Firmware 2.0 |
| Mediana 15 mantendo NORMAL | Observada no teste direto do Firmware 2.0 |
| Mediana 14 e estado NORMAL | Observados após a OTA |
| Mediana 15 mantendo ALERTA | Ainda requer registro de teste específico |
| Transição ALERTA para NORMAL | Ainda requer registro de sequência entre sessões |

Os testes pendentes não são considerados comprovados apenas por estarem implementados no código.

### Tratamento de erros

O firmware possui mensagens para ausência de Wi-Fi, falha de acesso ao manifesto, JSON inválido, versão sem atualização disponível, URL inesperada e erro de download ou gravação OTA.

Durante o desenvolvimento, foi observado o erro de partição OTA inexistente. A inclusão de `partitions.csv` permitiu completar a OTA no navegador. Os demais cenários de erro ainda precisam de execução controlada e registro para comprovação.

O aviso `No core dump partition found` também apareceu: a tabela usada não reserva uma partição para registrar falhas. Esse aviso não impediu a execução e a atualização demonstradas.

---

## Aprendizados

- Diferença entre código-fonte e firmware compilado.
- Uso de vetores, média, ordenação e mediana.
- Temporização de sessões com `millis()`.
- Histerese para preservar o estado na faixa intermediária.
- Consulta de versão por HTTPS e atualização OTA.
- Importância da tabela de partições e da verificação após o reinício.
- Organização de códigos, binário e manifesto em um repositório público.

---

## Finalidade Acadêmica

Projeto desenvolvido para fins acadêmicos na **FIAP**.
