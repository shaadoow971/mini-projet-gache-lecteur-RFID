#include "lvgl.h"
#include "Wire.h"
#include "MFRC522_I2C.h"

// Définitions des broches
#define RST_PIN 6
#define GACHE_PIN D10 // Broche pour la gâche

MFRC522_I2C mfrc522(0x28, RST_PIN);

// Label LVGL
lv_obj_t * main_label;

// Structure pour stocker les informations de chaque badge
struct BadgeInfo {
    unsigned long lastEntryTime; // Dernier temps d'entrée pour le badge
    bool isIn;                   // Indique si le badge est actuellement "entré"
};

// Tableau pour stocker les informations de chaque badge
BadgeInfo badgeStates[3]; // Pour les 3 badges connus

// UIDs connus
const byte BADGE1_UID[] = {10, 242, 99, 154};
const int BADGE1_UID_SIZE = sizeof(BADGE1_UID);
const char* BADGE1_NAME = "Mr Nanette";

const byte BADGE2_UID[] = {26, 162, 156, 154};
const int BADGE2_UID_SIZE = sizeof(BADGE2_UID);
const char* BADGE2_NAME = "Mr Mevel";

const byte BADGE3_UID[] = {233, 68, 32, 122};
const int BADGE3_UID_SIZE = sizeof(BADGE3_UID);
const char* BADGE3_NAME = "Mr Bur";

// Variables pour le délai anti-relecture
unsigned long lastReadTime = 0;
const unsigned long COOLDOWN_TIME_MS = 5000; // Délai de 5 secondes (ajustable)

// Compare deux UID
bool compareUids(byte *uid1, byte *uid2, int size) {
    for (int i = 0; i < size; i++) {
        if (uid1[i] != uid2[i]) return false;
    }
    return true;
}

// Met à jour le texte
void updateLvglText(const String & text) {
    if (main_label) {
        lv_label_set_text(main_label, text.c_str());
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

    // Initialiser les états des badges
    for (int i = 0; i < 3; i++) {
        badgeStates[i].lastEntryTime = 0;
        badgeStates[i].isIn = false;
    }

    testLvgl();

    Wire.begin();
    mfrc522.PCD_Init();
    Serial.begin(115200);
    Serial.println("Systeme pret. Approchez un badge.");

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
    // Géré par la tâche
}

void myTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t RFID_POLLING_INTERVAL_MS = 200;

    while (1) {
        // Vérifier si le temps de recharge est écoulé
        if (millis() - lastReadTime < COOLDOWN_TIME_MS) {
            // Trop tôt pour lire un nouveau badge
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(RFID_POLLING_INTERVAL_MS));
            continue; // Passer au cycle suivant de la boucle
        }

        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
            lastReadTime = millis(); // Mettre à jour le temps de la dernière lecture

            Serial.print("Badge lu (Dec) : ");
            for (byte i = 0; i < mfrc522.uid.size; i++) {
                Serial.print(mfrc522.uid.uidByte[i]);
                Serial.print(' ');
            }
            Serial.println();

            unsigned long currentTime = millis();
            String message = "";
            int badgeIndex = -1; // Index du badge trouvé

            // Gâche fermée temporairement
            digitalWrite(GACHE_PIN, HIGH);

            if (compareUids(mfrc522.uid.uidByte, (byte*)BADGE1_UID, BADGE1_UID_SIZE)) {
                badgeIndex = 0;
            }
            else if (compareUids(mfrc522.uid.uidByte, (byte*)BADGE2_UID, BADGE2_UID_SIZE)) {
                badgeIndex = 1;
            }
            else if (compareUids(mfrc522.uid.uidByte, (byte*)BADGE3_UID, BADGE3_UID_SIZE)) {
                badgeIndex = 2;
            }

            if (badgeIndex != -1) {
                const char* currentName = "";
                if (badgeIndex == 0) currentName = BADGE1_NAME;
                else if (badgeIndex == 1) currentName = BADGE2_NAME;
                else if (badgeIndex == 2) currentName = BADGE3_NAME;

                if (badgeStates[badgeIndex].isIn) {
                    // Le badge est déjà à l'intérieur, c'est une sortie
                    unsigned long timeElapsed = currentTime - badgeStates[badgeIndex].lastEntryTime;
                    long hours = timeElapsed / (1000 * 60 * 60);
                    long minutes = (timeElapsed % (1000 * 60 * 60)) / (1000 * 60);
                    long seconds = ((timeElapsed % (1000 * 60 * 60)) % (1000 * 60)) / 1000;
                    message = String("Au revoir ") + currentName + "! Temps ecoule: " + hours + "h " + minutes + "m " + seconds + "s";
                    badgeStates[badgeIndex].isIn = false;
                    badgeStates[badgeIndex].lastEntryTime = 0; // Réinitialiser le temps d'entrée
                } else {
                    // Le badge n'est pas à l'intérieur, c'est une entrée
                    message = String("Bonjour ") + currentName + "! decompte du temps d'entrer ";
                    badgeStates[badgeIndex].isIn = true;
                    badgeStates[badgeIndex].lastEntryTime = currentTime;
                }
                Serial.println(message);
                updateLvglText(message);
            } else {
                message = "Badge inconnu !";
                Serial.println(message);
                updateLvglText(message);
                digitalWrite(GACHE_PIN, LOW);
            }

            // Délai pour afficher le message sur LVGL avant de revenir à l'écran par défaut
            vTaskDelay(pdMS_TO_TICKS(3000));
            digitalWrite(GACHE_PIN, LOW);
            updateLvglText("Approchez un badge RFID");

            mfrc522.PICC_HaltA();
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(RFID_POLLING_INTERVAL_MS));
    }
}

#else 

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