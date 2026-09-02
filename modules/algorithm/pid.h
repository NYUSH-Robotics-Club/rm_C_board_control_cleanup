/**
 * Simple PID controller module
 */
#ifndef PID_H
#define PID_H

#include <stdint.h>

typedef struct {
  float Kp;
  float Ki;
  float Kd;

  float actual;
  float target;
  float error[3];
  float integral;

  float output;
  float output_max;
  float integral_max;
  
  // Added fields for improved PID calculation
  float last_measure;
  float last_output;
  float last_dout;
  
  float pout;
  float iout;
  float dout;
  float iterm;
  
  float dt;
  uint32_t last_time_us;
  
  // Low-pass filter RC constants
  float output_lpf_rc;
  float derivative_lpf_rc;
} PID_Controller;

void PID_Init(PID_Controller *pid, float kp, float ki, float kd, float output_max, float integral_max);
float PID_Calculate(PID_Controller *pid, float target, float actual);
void PID_Reset(PID_Controller *pid);
float PID_RPM_Calculate(PID_Controller *pid, float target_rpm, float actual_rpm);
#endif // PID_H

