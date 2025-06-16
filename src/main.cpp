#include "lvgl.h"
#include "Wire.h"
#include "MFRC522_I2C.h"

bool showingHistory = false;

// --- Broches et RFID ---
#define RST_PIN 6
#define GACHE_PIN D10

MFRC522_I2C mfrc522(0x28, RST_PIN);

// --- UI LVGL ---
lv_obj_t * main_label;
lv_obj_t * history_button;

// --- État des badges ---
struct BadgeInfo {
    unsigned long lastEntryTime;
    bool isIn;
};

BadgeInfo badgeStates[3];

// --- Badges connus ---
const byte BADGE1_UID[] = {10, 242, 99, 154};
const int BADGE1_UID_SIZE = sizeof(BADGE1_UID);
const char* BADGE1_NAME = "Mr Nanette";

const byte BADGE2_UID[] = {26, 162, 156, 154};
const int BADGE2_UID_SIZE = sizeof(BADGE2_UID);
const char* BADGE2_NAME = "Mr Mevel";

const byte BADGE3_UID[] = {233, 68, 32, 122};
const int BADGE3_UID_SIZE = sizeof(BADGE3_UID);
const char* BADGE3_NAME = "Mr Bur";

// --- Historique des passages ---
struct BadgeEvent {
    const char* name;
    bool isEntry;
    unsigned long timestamp;
    unsigned long duration;
};

#define MAX_EVENTS 50
BadgeEvent eventLog[MAX_EVENTS];
int eventCount = 0;

void logEvent(const char* name, bool isEntry, unsigned long timestamp, unsigned long duration = 0) {
    if (eventCount < MAX_EVENTS) {
        eventLog[eventCount++] = {name, isEntry, timestamp, duration};
    }
}

// --- Anti-relecture ---
unsigned long lastReadTime = 0;
const unsigned long COOLDOWN_TIME_MS = 5000;

// --- Comparaison UID ---
bool compareUids(byte *uid1, byte *uid2, int size) {
    for (int i = 0; i < size; i++) {
        if (uid1[i] != uid2[i]) return false;
    }
    return true;
}

// --- Met à jour le texte LVGL ---
void updateLvglText(const String & text) {
    if (main_label) {
        lv_label_set_text(main_label, text.c_str());
        lv_obj_center(main_label);
    }
}

// --- Affiche l'historique LVGL ---
void on_history_button_click(lv_event_t* e) {
    if (showingHistory) {
        // Si l'historique est déjà affiché, on revient à l'accueil
        updateLvglText("Approchez un badge RFID");
        showingHistory = false;
        return;
    }

    // Sinon, on affiche l'historique
    String text = "Historique des presences:\n";
    for (int i = 0; i < eventCount; i++) {
        const BadgeEvent& ev = eventLog[i];
        if (!ev.isEntry) {
            long h = ev.duration / 3600000;
            long m = (ev.duration % 3600000) / 60000;
            long s = (ev.duration % 60000) / 1000;
            text += String(ev.name) + " est sorti - duree: " + h + "h " + m + "m " + s + "s\n";
        }
    }
    if (text == "Historique des presences:\n") {
        text += "Aucun badge n'est encore sorti.";
    }
    updateLvglText(text);
    showingHistory = true;
}

// --- Création UI LVGL ---
void testLvgl() {
    main_label = lv_label_create(lv_screen_active());
    lv_label_set_text(main_label, "Approchez un badge RFID");
    lv_obj_center(main_label);

    history_button = lv_btn_create(lv_screen_active());
    lv_obj_align(history_button, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_event_cb(history_button, on_history_button_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t* label = lv_label_create(history_button);
    lv_label_set_text(label, "Historique");
}

#ifdef ARDUINO
#include "lvglDrivers.h"

void mySetup() {
    pinMode(GACHE_PIN, OUTPUT);
    digitalWrite(GACHE_PIN, LOW); // Gâche verrouillée par défaut

    // Init badges
    for (int i = 0; i < 3; i++) {
        badgeStates[i].lastEntryTime = 0;
        badgeStates[i].isIn = false;
    }

    testLvgl(); // UI
    Wire.begin();
    mfrc522.PCD_Init();
    Serial.begin(115200);
    Serial.println("Système prêt");

    xTaskCreate(myTask, "Main Task", 4096, NULL, 1, NULL);
}

void loop() {} // Tâche indépendante

// --- Tâche principale RFID ---
void myTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t RFID_INTERVAL = 200;

    while (1) {
        if (millis() - lastReadTime < COOLDOWN_TIME_MS) {
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(RFID_INTERVAL));
            continue;
        }

        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
            lastReadTime = millis();
            unsigned long now = millis();
            String message = "";
            int badgeIndex = -1;

            digitalWrite(GACHE_PIN, HIGH); // Ouvre la gâche

            // Identification du badge
            if (compareUids(mfrc522.uid.uidByte, (byte*)BADGE1_UID, BADGE1_UID_SIZE)) badgeIndex = 0;
            else if (compareUids(mfrc522.uid.uidByte, (byte*)BADGE2_UID, BADGE2_UID_SIZE)) badgeIndex = 1;
            else if (compareUids(mfrc522.uid.uidByte, (byte*)BADGE3_UID, BADGE3_UID_SIZE)) badgeIndex = 2;

            if (badgeIndex != -1) {
                const char* name = (badgeIndex == 0) ? BADGE1_NAME :
                                   (badgeIndex == 1) ? BADGE2_NAME : BADGE3_NAME;

                if (badgeStates[badgeIndex].isIn) {
                    // Sortie
                    unsigned long delta = now - badgeStates[badgeIndex].lastEntryTime;
                    long h = delta / 3600000;
                    long m = (delta % 3600000) / 60000;
                    long s = (delta % 60000) / 1000;
                    message = String("Au revoir ") + name + "! Temps: " + h + "h " + m + "m " + s + "s";
                    badgeStates[badgeIndex].isIn = false;
                    badgeStates[badgeIndex].lastEntryTime = 0;
                    logEvent(name, false, now, delta);
                } else {
                    // Entrée
                    message = String("Bonjour ") + name + " !";
                    badgeStates[badgeIndex].isIn = true;
                    badgeStates[badgeIndex].lastEntryTime = now;
                    logEvent(name, true, now);
                }

                Serial.println(message);
                updateLvglText(message);
            } else {
                // Badge inconnu
                message = "Badge inconnu !";
                Serial.println(message);
                updateLvglText(message);
                digitalWrite(GACHE_PIN, LOW);
            }

            vTaskDelay(pdMS_TO_TICKS(3000));
            digitalWrite(GACHE_PIN, LOW);
            updateLvglText("Approchez un badge RFID");
            mfrc522.PICC_HaltA();
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(RFID_INTERVAL));
    }
}

#else
// --- Simulation PC LVGL ---
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