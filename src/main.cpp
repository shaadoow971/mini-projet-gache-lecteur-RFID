#include "lvgl.h"
#include "Wire.h"
#include "MFRC522_I2C.h"

// Définitions des broches
#define RST_PIN 6 
#define GACHE_PIN D10 // Broche pour la gâche

MFRC522_I2C mfrc522(0x28, RST_PIN);

// Label LVGL
lv_obj_t * main_label;

// --- UID des badges connus ---
const byte BADGE1_UID[] = {10, 242, 99, 154}; 
const int BADGE1_UID_SIZE = sizeof(BADGE1_UID);

const byte BADGE2_UID[] = {26, 162, 156, 154};
const int BADGE2_UID_SIZE = sizeof(BADGE2_UID);

const byte BADGE3_UID[] = {233, 68, 32, 122};
const int BADGE3_UID_SIZE = sizeof(BADGE3_UID);

// Compare deux UID
bool compareUids(byte *uid1, byte *uid2, int size) {
    if (size != mfrc522.uid.size) return false;
    for (int i = 0; i < size; i++) {
        if (uid1[i] != uid2[i]) return false;
    }
    return true;
}

// Met à jour le texte
void updateLvglText(const char * text) {
    if (main_label) {
        lv_label_set_text(main_label, text);
        lv_obj_center(main_label);
    }
}

// UI LVGL
void testLvgl() {
    main_label = lv_label_create(lv_screen_active());
    lv_label_set_text(main_label, "Approchez un badge RFID");
    lv_obj_center(main_label);
}

#ifdef ARDUINO 

#include "lvglDrivers.h"

void mySetup() {
    pinMode(GACHE_PIN, OUTPUT);
    digitalWrite(GACHE_PIN, LOW); // Gâche ouverte par défaut

    testLvgl();

    Wire.begin();
    mfrc522.PCD_Init();
    Serial.begin(115200);
    Serial.println("Système prêt. Approchez un badge.");

    xTaskCreate(
        myTask,
        "Main Task",
        4096,
        NULL,
        1,
        NULL
    );
}

void loop() {
    // Géré par FreeRTOS
}

void myTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount(); 
    const TickType_t RFID_POLLING_INTERVAL_MS = 200;

    while (1) {
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
            Serial.print("Badge lu : ");
            for (byte i = 0; i < mfrc522.uid.size; i++) {
                Serial.print(mfrc522.uid.uidByte[i]);
                Serial.print(' ');
            }
            Serial.println(); 

            if (compareUids(mfrc522.uid.uidByte, (byte*)BADGE1_UID, BADGE1_UID_SIZE)) {
                Serial.println("Badge 1 détecté : GAGNER !");
                updateLvglText("GAGNER !");
                
                digitalWrite(GACHE_PIN, HIGH); // Ferme la gâche
                vTaskDelay(pdMS_TO_TICKS(3000));
                digitalWrite(GACHE_PIN, LOW);  // Rouvre la gâche
            } 
            else if (compareUids(mfrc522.uid.uidByte, (byte*)BADGE2_UID, BADGE2_UID_SIZE)) {
                Serial.println("Badge 2 détecté : PERDU !");
                updateLvglText("PERDU !");
            } 
            else if (compareUids(mfrc522.uid.uidByte, (byte*)BADGE3_UID, BADGE3_UID_SIZE)) {
                Serial.println("Badge 3 détecté : PERDU !");
                updateLvglText("PERDU T'ES NUL !");
            }

            vTaskDelay(pdMS_TO_TICKS(1000)); // Attente avant de réinitialiser le message
            updateLvglText("Approchez un badge RFID");

            mfrc522.PICC_HaltA();
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(RFID_POLLING_INTERVAL_MS));
    }
}

#else // Simulateur PC

#include "lvgl.h"
#include "app_hal.h"
#include <cstdio>

int main(void) {
    printf("LVGL Simulateur\n");
    fflush(stdout);

    lv_init();
    hal_setup();

    testLvgl();

    hal_loop();
    return 0;
}

#endif
