#pragma once

// Approximate LiPo state-of-charge as a function voltage in steps of 5%
static const float LIPO_SOC_VOLT[] = {4.20, 4.15, 4.11, 4.08, 4.02,  // 100, 95, 90, 85, 80
                                      3.98, 3.95, 3.91, 3.87, 3.85,  //  75, 70, 65, 60, 55
                                      3.84, 3.82, 3.80, 3.79, 3.77,  //  50, 45, 40, 35, 30
                                      3.75, 3.73, 3.71, 3.69, 3.61,  //  25, 20, 15, 10,  5
                                      3.27, 0.00};                   //   0, end

// Approximate SLA/AGM state-of-charge (at 25 C) as a function of voltage in steps of 25%
static const float SLA_SOC_VOLT[] = {12.70, 12.45, 12.24,  // 100, 75, 50
                                     12.06, 11.90, 0.00};  //  25,  0, end


/* Convert a LiPo voltage to an estimated state-of-charge */
inline float lipo_estimate_soc(float vbat) {
    if( vbat != vbat ) {
      return NAN;
    }
    vbat = fminf(vbat, LIPO_SOC_VOLT[0]);
    vbat = fmaxf(vbat, LIPO_SOC_VOLT[20]);
    
    float soc = 0.0;
    for(int i=0; i<20; i++) {
        if( (vbat <= LIPO_SOC_VOLT[i]) && (vbat > LIPO_SOC_VOLT[i+1]) ) {
            soc = (100-i*5) + (vbat - LIPO_SOC_VOLT[i]) * 5 / (LIPO_SOC_VOLT[i] - LIPO_SOC_VOLT[i+1]);
            break;
        }
    }
    return soc;
}

/* Convert a SLA/AGM voltage to an estimated state-of-charge */
inline float sla_estimate_soc(float vbat) {
    if( vbat != vbat ) {
      return NAN;
    }
    vbat = fminf(vbat, SLA_SOC_VOLT[0]);
    vbat = fmaxf(vbat, SLA_SOC_VOLT[4]);
    
    float soc = 0.0;
    for(int i=0; i<5; i++) {
        if( (vbat <= SLA_SOC_VOLT[i]) && (vbat > SLA_SOC_VOLT[i+1]) ) {
            soc = (100-i*25) + (vbat - SLA_SOC_VOLT[i]) * 25 / (SLA_SOC_VOLT[i] - SLA_SOC_VOLT[i+1]);
            break;
        }
    }
    return soc;
}

/* Estimate the full charge voltage for a SLA/AGM battery at temperature
   `tbat` in C.  If `tbat` is NaN, 25 C is used.
 */
inline float sla_estimate_full_volts(float tbat) {
    if( tbat != tbat ) {
        tbat = 25.0;
    }
    
    return 13.80 - 0.024 * (tbat - 25);
}

/* Estimate the overcharge voltage for a SLA/AGM battery at temperature
   `tbat` in C.  If `tbat` is NaN, 25 C is used.
 */
inline float sla_estimate_over_volts(float tbat) {
    if( tbat != tbat ) {
      tbat = 25.0;
    }

    return 14.80 - 0.024 * (tbat - 25);
}
