# 🔐 Настройка SSH для Автодеплоя

## Проблема
GitHub Actions не может подключиться к серверу через SSH для автоматического деплоя.

## Решение: SSH Keys

### 1️⃣ Генерация SSH ключей (на локальной машине)

```bash
# Создать новую пару ключей для деплоя
ssh-keygen -t ed25519 -C "github-actions@deploy" -f ~/.ssh/github_deploy_key

# Или RSA (если ed25519 не поддерживается)
ssh-keygen -t rsa -b 4096 -C "github-actions@deploy" -f ~/.ssh/github_deploy_key
```

Это создаст два файла:
- `github_deploy_key` - приватный ключ (секретный)
- `github_deploy_key.pub` - публичный ключ

### 2️⃣ Копирование публичного ключа на сервер

```bash
# Способ 1: ssh-copy-id
ssh-copy-id -i ~/.ssh/github_deploy_key.pub root@138.124.0.8

# Способ 2: вручную
cat ~/.ssh/github_deploy_key.pub | ssh root@138.124.0.8 "mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys"

# Способ 3: через веб-консоль
# 1. Открыть файл на локальной машине
cat ~/.ssh/github_deploy_key.pub

# 2. Скопировать содержимое
# 3. На сервере:
mkdir -p ~/.ssh
nano ~/.ssh/authorized_keys
# Вставить скопированный ключ в новую строку
# Ctrl+O, Enter, Ctrl+X

# Установить правильные права
chmod 700 ~/.ssh
chmod 600 ~/.ssh/authorized_keys
```

### 3️⃣ Проверка подключения

```bash
# Тест с приватным ключом
ssh -i ~/.ssh/github_deploy_key root@138.124.0.8

# Если работает - успех! ✅
```

### 4️⃣ Добавление секретов в GitHub

1. Открыть репозиторий на GitHub
2. Settings → Secrets and variables → Actions
3. Добавить секреты:

**SSH_HOST:**
```
138.124.0.8
```

**SSH_USERNAME:**
```
root
```

**SSH_PRIVATE_KEY:**
```bash
# Скопировать содержимое приватного ключа
cat ~/.ssh/github_deploy_key

# Вставить ВСЁ, включая:
# -----BEGIN OPENSSH PRIVATE KEY-----
# ...
# -----END OPENSSH PRIVATE KEY-----
```

**SERVER_URL** (если ещё нет):
```
http://single-project.duckdns.org
```

---

## Альтернатива: Пароль (менее безопасно)

Если SSH keys не работают, можно использовать пароль:

### GitHub Secrets:
- `SSH_HOST`: `138.124.0.8`
- `SSH_USERNAME`: `root`
- `SSH_PASSWORD`: `ваш_пароль`

### Workflow изменения:
```yaml
- name: 📡 Deploy via SSH
  uses: appleboy/ssh-action@v1.0.0
  with:
    host: ${{ secrets.SSH_HOST }}
    username: ${{ secrets.SSH_USERNAME }}
    password: ${{ secrets.SSH_PASSWORD }}  # вместо key
    port: 22
    script: |
      cd ~/cs-legit
      git pull
      cd launcher-server/backend
      npm install --production
      systemctl restart launcher
```

---

## Troubleshooting

### Permission denied (publickey)
```bash
# На сервере проверить:
cat ~/.ssh/authorized_keys
# Должен содержать ваш публичный ключ

# Проверить права:
ls -la ~/.ssh/
# Должно быть:
# drwx------ ~/.ssh
# -rw------- authorized_keys

# Исправить права если нужно:
chmod 700 ~/.ssh
chmod 600 ~/.ssh/authorized_keys
```

### SSH connection closed
```bash
# Проверить fail2ban
fail2ban-client status sshd
# Если IP забанен:
fail2ban-client unban <IP>

# Или добавить в whitelist:
nano /etc/fail2ban/jail.local

[sshd]
ignoreip = 127.0.0.1/8 ::1 <GitHub_Actions_IP>

systemctl restart fail2ban
```

### Port 22 filtered
```bash
# Проверить firewall
ufw status
# Должен быть:
# 22/tcp ALLOW Anywhere

# Если закрыт:
ufw allow 22/tcp
ufw reload
```

---

## Проверка работы автодеплоя

1. Внести любое изменение в `launcher-server/`:
```bash
cd launcher-server/backend/src
nano index.js  # например, изменить console.log
git add -A
git commit -m "test: trigger autodeploy"
git push
```

2. Проверить GitHub Actions:
- Открыть https://github.com/gavrikov2044-bot/cs-legit/actions
- Найти workflow "🚀 Deploy Backend"
- Проверить, что все шаги прошли успешно ✅

3. Проверить сервер:
```bash
ssh root@138.124.0.8
journalctl -u launcher -n 50
# Должны увидеть перезапуск и новые логи
```

---

## Безопасность SSH

### Disable Password Auth (после настройки ключей)
```bash
# На сервере
nano /etc/ssh/sshd_config

# Найти и изменить:
PasswordAuthentication no
PubkeyAuthentication yes
PermitRootLogin prohibit-password

# Перезапустить SSH
systemctl restart sshd
```

⚠️ **Внимание:** Делать это только после того, как SSH keys точно работают!

### Whitelist для GitHub Actions
```bash
# fail2ban whitelist
nano /etc/fail2ban/jail.local

[sshd]
enabled = true
ignoreip = 127.0.0.1/8 ::1 140.82.112.0/20 143.55.64.0/20
# IP диапазоны GitHub Actions
```

---

**Итог:** После настройки SSH keys автодеплой будет работать автоматически при каждом push в `launcher-server/`!

