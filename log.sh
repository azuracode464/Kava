#!/bin/bash

# Arquivo de saída
OUTPUT="/sdcard/log.sh"

# Limpa o arquivo de saída
> "$OUTPUT"

# Data atual
NOW=$(date "+%Y-%m-%d %H:%M:%S")

# Percorre todos os arquivos NÃO ocultos
find . -type f ! -path '/.*' | while read -r file; do

    # Verifica se é texto (ignora binários)
    if file "$file" | grep -q "text"; then

        {
            echo "========================================"
            echo "ARQUIVO : $(basename "$file")"
            echo "CAMINHO : $(realpath "$file")"
            echo "DATA    : $NOW"
            echo "========================================"
            echo
            cat "$file"
            echo
            echo
        } >> "$OUTPUT"

    fi
done
