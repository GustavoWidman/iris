#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
struct RGB { uint8_t r,g,b; };
inline uint8_t byte_clamp(double v) { return static_cast<uint8_t>(std::lround(std::max(0.0,std::min(255.0,v)))); }
inline RGB hsv_rgb(uint8_t h,uint8_t s,uint8_t level) {
 double H=h*6.0/254.0,S=s/254.0,V=level/254.0; int i=static_cast<int>(std::floor(H)); double f=H-i;
 double p=V*(1-S),q=V*(1-S*f),t=V*(1-S*(1-f)),r=0,g=0,b=0;
 switch(i%6) {case 0:r=V;g=t;b=p;break;case 1:r=q;g=V;b=p;break;case 2:r=p;g=V;b=t;break;case 3:r=p;g=q;b=V;break;case 4:r=t;g=p;b=V;break;case 5:r=V;g=p;b=q;break;}
 return {byte_clamp(r*255),byte_clamp(g*255),byte_clamp(b*255)};
}
// D65 sRGB conversion reference: W3C CSS Color 4 (see THIRD_PARTY.md).
inline RGB xy_rgb(uint16_t x16,uint16_t y16,uint8_t level) {
 const double x=x16/65535.0,y=y16/65535.0;
 if(y<0.0001 || level==0)return {0,0,0};
 double X=x/y,Y=1,Z=(1-x-y)/y;
 // D65 XYZ to linear sRGB, clip negative channels, then apply sRGB transfer.
 double r=std::max(0.0,3.2406*X-1.5372*Y-0.4986*Z),g=std::max(0.0,-0.9689*X+1.8758*Y+0.0415*Z),b=std::max(0.0,0.0557*X-0.2040*Y+1.0570*Z);
 auto gamma=[](double v){return v<=0.0031308?12.92*v:1.055*std::pow(v,1.0/2.4)-0.055;};
 r=gamma(r);g=gamma(g);b=gamma(b);double scale=std::max({r,g,b,1.0});double gain=255.0*level/254.0/scale;
 return {byte_clamp(r*gain),byte_clamp(g*gain),byte_clamp(b*gain)};
}
// Kelvin approximation derived from Tanner Helland's published algorithm.
// Copyright (c) 2018, Tanner Helland. BSD-2-Clause; see
// licenses/Tanner-Helland-BSD-2-Clause.txt and THIRD_PARTY.md.
// This is not a calibrated illuminant.
inline RGB kelvin_rgb(uint16_t mired,uint8_t level) {
 double k=1000000.0/std::max<uint16_t>(mired,1)/100.0;
 double r=k<=66?255:329.698727446*std::pow(k-60,-0.1332047592);
 double g=k<=66?99.4708025861*std::log(k)-161.1195681661:288.1221695283*std::pow(k-60,-0.0755148492);
 double b=k>=66?255:(k<=19?0:138.5177312231*std::log(k-10)-305.0447927307);
 double gain=level/254.0;return {byte_clamp(byte_clamp(r)*gain),byte_clamp(byte_clamp(g)*gain),byte_clamp(byte_clamp(b)*gain)};
}
