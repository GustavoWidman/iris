#pragma once
#include "color_math.h"

// Destination R/G/B output selects an input channel. Identity until physical measurements.
// Gains only reduce channel maxima; gamma adjusts encoded channel values (1 = unchanged).
struct LedCalibration {
 uint8_t source[3] = {0,1,2};
 double gain[3] = {1.0,1.0,1.0};
 double gamma[3] = {1.0,1.0,1.0};
};
inline RGB calibrated_rgb(RGB input,const LedCalibration &c) {
 const uint8_t src[3]={input.r,input.g,input.b};
 uint8_t out[3]={};
 for(int i=0;i<3;i++){
  if(c.source[i]>2 || !std::isfinite(c.gain[i]) || !std::isfinite(c.gamma[i]) ||
     c.gain[i]<0.0 || c.gain[i]>1.0 || c.gamma[i]<0.2 || c.gamma[i]>5.0)
   return input; // Invalid calibration: never send an unexpectedly altered color.
  if(c.gain[i]==1.0 && c.gamma[i]==1.0)out[i]=src[c.source[i]];
  else out[i]=byte_clamp(255.0*c.gain[i]*std::pow(src[c.source[i]]/255.0,c.gamma[i]));
 }
 return {out[0],out[1],out[2]};
}
