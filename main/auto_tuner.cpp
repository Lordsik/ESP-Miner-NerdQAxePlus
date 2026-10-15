#include "auto_tuner.h"
#include "esp_log.h"
#include "nvs_config.h"

static const char *TAG = "auto_tuner";

namespace AutoTuner {
    static bool s_enabled = false;
    static int s_current_step = 0; // 0 do 20 (co 5%)
    static int s_block_counter = 0;
    const int TARGET_BLOCKS = 10;
    
    static TunerStepResult s_steps[21];
    static uint32_t s_best_extranonce = 0;
    static uint32_t absolute_max_diff = 0;

    void init() {
        // Inicjalizacja kroków (0% do 100% co 5%)
        for (int i = 0; i <= 20; i++) {
            float pct = i * 5.0f;
            s_steps[i].percentage = pct;
            s_steps[i].extranonce_val = (uint32_t)(4294967295.0f * (pct / 100.0f));
            s_steps[i].max_diff = 0;
            s_steps[i].completed = false;
        }
        s_current_step = 0;
        s_block_counter = 0;
    }

    void setEnabled(bool enable) {
        s_enabled = enable;
        ESP_LOGI(TAG, "Auto-Tuner %s", enable ? "ENABLED" : "DISABLED");
    }

    bool isEnabled() {
        return s_enabled;
    }

    void onNewBlockDetected() {
        if (!s_enabled) return;

        s_block_counter++;
        ESP_LOGI(TAG, "Auto-Tuner: Block observed (%d/%d) on step %d (%.1f%%)", 
                 s_block_counter, TARGET_BLOCKS, s_current_step, s_steps[s_current_step].percentage);

        if (s_block_counter >= TARGET_BLOCKS) {
            // Zakończ obecny krok, przejdź do następnego
            s_steps[s_current_step].completed = true;
            ESP_LOGI(TAG, "Step %.1f%% completed. Max diff: %lu", 
                     s_steps[s_current_step].percentage, s_steps[s_current_step].max_diff);

            s_block_counter = 0;
            s_current_step++;

            if (s_current_step > 20) {
                // Koniec całego testu! Znajdź zwycięzcę
                s_enabled = false;
                ESP_LOGI(TAG, "Auto-Tuner finished all steps! Best Extranonce: 0x%08X", s_best_extranonce);
                // Tutaj zapisz zwycięzcę do NVS jako domyślny start
                Config::setExtranonce2Pct((uint16_t)(s_best_extranonce * 100 / 4294967295));
            }
        }
    }

    void reportShareDiff(uint32_t nonce_diff) {
        if (!s_enabled) return;

        if (nonce_diff > s_steps[s_current_step].max_diff) {
            s_steps[s_current_step].max_diff = nonce_diff;
            
            // Sprawdź czy to absolutny rekord ogólny
            if (nonce_diff > absolute_max_diff) {
                absolute_max_diff = nonce_diff;
                s_best_extranonce = s_steps[s_current_step].extranonce_val;
            }
        }
    }

    void getResultsJson(JsonDocument &doc) {
        JsonArray arr = doc["tuner_steps"].to<JsonArray>();
        for (int i = 0; i <= 20; i++) {
            JsonObject obj = arr.add<JsonObject>();
            obj["pct"] = s_steps[i].percentage;
            obj["hex"] = s_steps[i].extranonce_val;
            obj["max_diff"] = s_steps[i].max_diff;
            obj["completed"] = s_steps[i].completed;
            obj["is_best"] = (s_steps[i].extranonce_val == s_best_extranonce);
        }
        doc["tuner_active"] = s_enabled;
        doc["current_step"] = s_current_step;
    }
}
