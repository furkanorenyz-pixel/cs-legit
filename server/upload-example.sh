#!/bin/bash
# Пример загрузки файлов на сервер

SERVER_URL="http://localhost:3000"
GAME="cs2"
TYPE="external"
VERSION="v1.0.0"

# Загрузить external cheat
curl -X POST "${SERVER_URL}/api/upload" \
  -F "game=${GAME}" \
  -F "type=${TYPE}" \
  -F "version=${VERSION}" \
  -F "description=CS2 External ESP v1.0.0" \
  -F "files=@../build/externa/externa.exe"

# Загрузить internal cheat
curl -X POST "${SERVER_URL}/api/upload" \
  -F "game=${GAME}" \
  -F "type=internal" \
  -F "version=${VERSION}" \
  -F "description=CS2 Internal DLL v1.0.0" \
  -F "files=@../build/interna/internal.dll" \
  -F "files=@../build/injector/injector.exe"

