#pragma once
#include <stdint.h>
#include "ArduinoJson.h"

struct TunerStepResult {
    float percentage;        // np. 0.0, 5.0, 10.0 ... 100.0
    uint32_t extranonce_val; // wartość szesnastkowa punktu startowego
    uint32_t max_diff;       // najwyższy diff zanotowany w tym kroku
    bool completed;          // czy krok został zakończony
};

namespace AutoTuner {
    void init();
    void setEnabled(bool enable);
    bool isEnabled();
    
    // Wywoływane przy każdym nowym zadaniu/bloku z puli
    void onNewBlockDetected();
    
    // Rejestrowanie napotkanego udziału (sprawdza czy to nowy rekord dla bieżącego kroku)
    void reportShareDiff(uint32_t nonce_diff);
    
    // Pobieranie wyników do formatu JSON (dla panelu WWW)
    void getResultsJson(JsonDocument &doc);
}
