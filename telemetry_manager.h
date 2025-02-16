#ifndef telemetry_manager_H
#define telemetry_manager_H

#include <Arduino.h>
#include <ArduinoJson.h>

struct TelemetryData
{
    String gear;
    String tyre_f_l;
    String tyre_f_r;
    String tyre_r_l;
    String tyre_r_r;

    int16_t tyre_f_l_m;
    int16_t tyre_f_r_m;
    int16_t tyre_r_l_m;
    int16_t tyre_r_r_m;

    String tc;
    String bias;
    String fuel;
    String abs;
    bool isValid() const
    {
        return !fuel.isEmpty();
    }
};

extern TelemetryData telemetry;

void updateTelemetry(const StaticJsonDocument<300> &doc);

#endif