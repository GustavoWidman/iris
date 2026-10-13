// Hardware-free mapping report. No vendor SDK, serial or camera access.
#include "../main/led_calibration.h"
#include <cstdio>
int main(){
 LedCalibration profile{};profile.profile=LedColorProfile::ExperimentalRelativeBalance;
 std::puts("name,input_r,input_g,input_b,output_r,output_g,output_b");
 for(int level: {1,8,32,120,128,255}){
  const auto l=static_cast<uint8_t>(level);
  struct Example {const char *name;RGB rgb;};
  for(const auto &sample: {Example{"red",{l,0,0}},Example{"orange_source",{l,byte_clamp(level/2.0),0}},
                          Example{"yellow_source",{l,l,0}},Example{"white_source",{l,l,l}},
                          Example{"magenta_source",{l,0,l}},Example{"cyan_source",{0,l,l}},
                          Example{"blue",{0,0,l}}}){
   const auto out=calibrated_rgb(sample.rgb,profile);
   std::printf("%s,%u,%u,%u,%u,%u,%u\n",sample.name,sample.rgb.r,sample.rgb.g,sample.rgb.b,out.r,out.g,out.b);
  }
 }
}
