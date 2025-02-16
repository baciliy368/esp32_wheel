#include "telemetry_manager.h"
#define GEAR "12"
#define TYRE_F_L "5"
#define TYRE_F_R "6"
#define TYRE_R_L "7"
#define TYRE_R_R "8"
#define TYRE_F_L_M "1"
#define TYRE_F_R_M "2"
#define TYRE_R_L_M "3"
#define TYRE_R_R_M "4"
#define TC "9"
#define BIAS "10"
#define FUEL "13"
#define ABS "11"

TelemetryData telemetry;

void updateTelemetry(const StaticJsonDocument<300> &doc)
{
    if (doc.containsKey("d"))
    {
        if (doc['d'].containsKey("T"))
        {
            if (doc['d']['T'].containsKey(GEAR))
                telemetry.gear = doc['d']['T'][GEAR].as<String>();
            if (doc['d']['T'].containsKey(TYRE_F_L))
                telemetry.tyre_f_l = doc['d']['T'][TYRE_F_L].as<String>();
            if (doc['d']['T'].containsKey(TYRE_F_R))
                telemetry.tyre_f_r = doc['d']['T'][TYRE_F_R].as<String>();
            if (doc['d']['T'].containsKey(TYRE_R_L))
                telemetry.tyre_r_l = doc['d']['T'][TYRE_R_L].as<String>();
            if (doc['d']['T'].containsKey(TYRE_R_R))
                telemetry.tyre_r_r = doc['d']['T'][TYRE_R_R].as<String>();

            if (doc['d']['T'].containsKey(TYRE_F_L_M))
                telemetry.tyre_f_l_m = doc['d']['T'][TYRE_F_L_M].as<int16_t>();
            if (doc['d']['T'].containsKey(TYRE_F_R_M))
                telemetry.tyre_f_r_m = doc['d']['T'][TYRE_F_R_M].as<int16_t>();
            if (doc['d']['T'].containsKey(TYRE_R_L_M))
                telemetry.tyre_r_l_m = doc['d']['T'][TYRE_R_L_M].as<int16_t>();
            if (doc['d']['T'].containsKey(TYRE_R_R_M))
                telemetry.tyre_r_r_m = doc['d']['T'][TYRE_R_R_M].as<int16_t>();

            if (doc['d']['T'].containsKey(TC))
                telemetry.tc = doc['d']['T'][TC].as<String>();
            if (doc['d']['T'].containsKey(BIAS))
                telemetry.bias = doc['d']['T'][BIAS].as<String>();
            if (doc['d']['T'].containsKey(FUEL))
                telemetry.fuel = doc['d']['T'][FUEL].as<String>();
            if (doc['d']['T'].containsKey(ABS))
                telemetry.abs = doc['d']['T'][ABS].as<String>();
        }
    }
}