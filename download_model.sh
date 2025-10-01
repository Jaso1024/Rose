#!/bin/bash
set -euo pipefail

mkdir -p models

echo "Downloading Whisper small.en model..."
curl -fL --retry 3 --retry-delay 2 \
  "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin" \
  -o "models/ggml-small.en.bin"

if [ "${WHISPER_SMALL_EN_SHA256:-}" != "" ]; then
  echo "${WHISPER_SMALL_EN_SHA256}  models/ggml-small.en.bin" | shasum -a 256 -c -
fi

echo "Model downloaded successfully"
