# ESP32 RainMaker Switch with FreeRTOS LEDs 🚀

Projeto de controle de **relé/switch via ESP RainMaker**, utilizando **FreeRTOS** para gerenciamento de LEDs de status.

Ideal para projetos IoT profissionais com provisionamento Wi-Fi, feedback visual e integração com o app **ESP RainMaker**.

---

## 📌 Funcionalidades

- ✅ Provisionamento Wi-Fi via **BLE ou SoftAP**
- ✅ Integração com **ESP RainMaker**
- ✅ Controle de switch (relé ou carga)
- ✅ **LED de status Wi-Fi**
  - Pisca quando offline
  - Apagado quando online
- ✅ **LED de mensagem**
  - Pisca ao receber comando do RainMaker
- ✅ Botão físico com múltiplas funções:
  - Toque curto → alterna o switch
  - 3s pressionado → reset Wi-Fi
  - 10s pressionado → factory reset
- ✅ OTA, Scenes, Schedule e Timezone habilitados

---

## 🧰 Hardware utilizado

- ESP32 / ESP32-C3 / ESP32-C6
- 2 LEDs + resistores
- 1 botão push
- Relé ou carga no GPIO definido

---

## 🔌 Mapeamento de pinos

| Função        | GPIO |
|--------------|------|
| LED Wi-Fi    | 2    |
| LED Mensagem | 4    |
| Switch/Relé  | 16   |
| Botão        | 0    |

> ⚠️ Para ESP32-C3/C6 os pinos são ajustados automaticamente no código.

---

## ⚙️ Bibliotecas necessárias

- ESP RainMaker
- WiFi
- WiFiProv
- AppInsights

Instale tudo pelo **Board Manager da Espressif**.

---

## 🧠 Arquitetura FreeRTOS

- `ledWifiTask`  
  Gerencia o LED de status da conexão Wi-Fi.

- `ledMsgTask`  
  Indica visualmente o recebimento de comandos remotos.

As tasks rodam de forma independente do `loop()`, garantindo fluidez e estabilidade.

---

## 📲 Funcionamento no App RainMaker

- Cada comando recebido aciona:
  - Atualização do estado do switch
  - Feedback visual via LED
- Provisionamento fácil via QR Code

---

