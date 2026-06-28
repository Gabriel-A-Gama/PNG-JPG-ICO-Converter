# PNG/JPG to ICO Converter

Converte imagens PNG ou JPG em arquivos .ico de alta qualidade.

---

##  Por que esse projeto?

Ferramentas online de conversão de ICO costumam gerar ícones borrados, com bordas quebradas e sem todos os tamanhos necessários. Esse conversor usa redimensionamento de alta qualidade (filtro Lanczos/SRGB via stb), garantindo que círculos continuem círculos, bordas suaves continuem suaves, e transparência seja preservada em todos os tamanhos.

---

##  Tamanhos gerados

O `.ico` de saída inclui automaticamente:

| Tamanho | Formato interno |
|---------|----------------|
| 256×256 | PNG (padrão moderno, Windows Vista+) |
| 128×128 | BMP 32-bit com alpha |
| 96×96   | BMP 32-bit com alpha |
| 64×64   | BMP 32-bit com alpha |
| 48×48   | BMP 32-bit com alpha |
| 32×32   | BMP 32-bit com alpha |
| 24×24   | BMP 32-bit com alpha |
| 16×16   | BMP 32-bit com alpha |

O Windows seleciona automaticamente o tamanho mais adequado para cada contexto (barra de tarefas, explorador, atalhos, etc.).

---

## Como usar

### Arrastar e soltar (modo mais fácil)

Arraste qualquer arquivo `.png` ou `.jpg` diretamente sobre o `img_to_ico.exe`.

O arquivo `.ico` será gerado **na mesma pasta da imagem original**, com o mesmo nome.

```
minha_logo.png  →  minha_logo.ico
```

### Linha de comando

```cmd
img_to_ico.exe "C:\caminho\para\imagem.png"
```

---

## Como compilar (Visual Studio 2022)

### 1. Clone o repositório

```bash
git clone https://github.com/Gabriel-A-Gama/PNG-JPG-ICO-Converter.git
cd img-to-ico
```



### 2. Abra no Visual Studio 2022

- Abra o arquivo `.sln` ou crie um novo projeto **Console App (C++)**
- Adicione `img_to_ico.cpp` ao projeto


### 4. Configure o projeto

Em `Project → Properties`:

| Seção | Configuração | Valor |
|-------|-------------|-------|
| General | C++ Language Standard | **ISO C++17 (`/std:c++17`)** |
| C/C++ → Preprocessor | Preprocessor Definitions | adicione `_CRT_SECURE_NO_WARNINGS` |
| Linker → System | SubSystem | **Console** |

### 5. Build

```
Ctrl + Shift + B
```

O executável ficará em `x64\Release\img_to_ico.exe`.

---

## Estrutura do projeto

```
img-to-ico/
├── img_to_ico.cpp       # Código-fonte principal
├── stb_image.h          # ← você baixa (não incluso)
├── stb_image_resize2.h  # ← você baixa (não incluso)
├── stb_image_write.h    # ← você baixa (não incluso)
└── README.md
```

---

##  Como funciona

O conversor escreve o formato `.ico` manualmente, sem bibliotecas de terceiros além do stb:

- **Leitura**: `stb_image` carrega PNG, JPG, BMP e TGA em RGBA
- **Redimensionamento**: `stb_image_resize2` com filtro SRGB de alta qualidade — sem pixelização, sem aliasing
- **256px → PNG**: embutido como PNG dentro do ICO (padrão do Windows Vista+)
- **Demais tamanhos → BMP 32-bit**: com canal alpha e AND mask, compatível com todas as versões do Windows
- **Escrita**: o arquivo `.ico` é montado diretamente em memória e gravado via `std::ofstream`

---

##  Formatos de entrada suportados

- `.png`
- `.jpg` / `.jpeg`
- `.bmp`
- `.tga`

---

## Créditos e dependências

Este projeto utiliza as seguintes bibliotecas open-source:

**[stb](https://github.com/nothings/stb)** por Sean Barrett

- `stb_image.h` — carregamento de imagens
- `stb_image_resize2.h` — redimensionamento de alta qualidade
- `stb_image_write.h` — escrita de PNG em memória

Licença: MIT / Domínio Público (escolha do usuário). Veja [https://github.com/nothings/stb](https://github.com/nothings/stb) para detalhes.

---

##  Licença

MIT License. Veja o arquivo `LICENSE` para mais detalhes.
