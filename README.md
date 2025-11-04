# 🚆 Vai de Trem Hub - Conexão Wi-Fi com ESP32

## 📋 Descrição
O projeto **Vai de Trem** tem como objetivo configurar o microcontrolador **ESP32** para conectar-se à rede Wi-Fi da escola, exibindo o endereço IP obtido e mantendo a reconexão automática em caso de falha.  
Esta é a **primeira etapa** do desenvolvimento do aplicativo **Vai de Trem**, que futuramente permitirá o gerenciamento de linhas de trem e controle de informações através de um sistema web.

---

## 🎯 Objetivo
- Criar e configurar o repositório do projeto no **GitHub**.  
- Programar o **ESP32** para se conectar automaticamente à rede Wi-Fi.  
- Exibir no **Monitor Serial** o status da conexão e o **endereço IP** obtido.  
- Implementar **reconexão automática** em caso de perda de sinal.

---

## ⚙️ Tecnologias Utilizadas
- **Placa:** ESP32 DevKit  
- **Linguagem:** C++ (Arduino IDE)  
- **Plataforma:** Arduino IDE ou VS Code (PlatformIO)  
- **Controle de Versão:** Git + GitHub  

---

## 🧠 Estrutura do Projeto
vai-de-trem/
┣ 📂 src/
┃ ┗ 📜 wifi_connect.ino
┣ 📜 README.md
┣ 📜 .gitignore
