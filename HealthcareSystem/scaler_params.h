#ifndef SCALER_PARAMS_H
#define SCALER_PARAMS_H

// StandardScaler coefficients generated during Phase 3 preprocessing.
// Ordering: accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z, distance_cm, pir_motion, ir_obstacle

const float SCALER_MEAN[9] = {
    0.53156046f, 
    0.07913944f, 
    0.13962194f, 
    2.74787544f, 
    -1.03008472f, 
    4.02067366f, 
    76.03929607f, 
    0.3812f, 
    0.220565f
};

const float SCALER_STD[9] = {
    0.55499457f, 
    0.5293823f, 
    0.4453456f, 
    25.8349474f, 
    24.48607633f, 
    20.96686489f, 
    57.15828022f, 
    0.48568154f, 
    0.41462764f
};

#endif // SCALER_PARAMS_H
