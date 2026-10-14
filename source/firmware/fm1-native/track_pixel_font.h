/* Native TRACK font: exact 5x7 glyph pixels from the WebSimulator. */
#ifndef TRACK_PIXEL_FONT_H
#define TRACK_PIXEL_FONT_H
#include <stdint.h>
static const uint8_t track_web_5x7[128][7]={
 [80]={30u,17u,17u,30u,16u,16u,16u},
 [76]={16u,16u,16u,16u,16u,16u,31u},
 [65]={14u,17u,17u,31u,17u,17u,17u},
 [89]={17u,17u,10u,4u,4u,4u,4u},
 [68]={30u,17u,17u,17u,17u,17u,30u},
 [82]={30u,17u,17u,30u,20u,18u,17u},
 [87]={17u,17u,17u,21u,21u,21u,10u},
 [66]={30u,17u,17u,30u,17u,17u,30u},
 [67]={15u,16u,16u,16u,16u,16u,15u},
 [83]={15u,16u,16u,14u,1u,1u,30u},
 [86]={17u,17u,17u,17u,17u,10u,4u},
 [69]={31u,16u,16u,30u,16u,16u,31u},
 [70]={31u,16u,16u,30u,16u,16u,16u},
 [73]={14u,4u,4u,4u,4u,4u,14u},
 [79]={14u,17u,17u,17u,17u,17u,14u},
 [75]={17u,18u,20u,24u,20u,18u,17u},
 [77]={17u,27u,21u,21u,17u,17u,17u},
 [78]={17u,25u,21u,19u,17u,17u,17u},
 [85]={17u,17u,17u,17u,17u,17u,14u},
 [84]={31u,4u,4u,4u,4u,4u,4u},
 [48]={14u,17u,19u,21u,25u,17u,14u},
 [49]={4u,12u,4u,4u,4u,4u,14u},
 [50]={14u,17u,1u,2u,4u,8u,31u},
 [51]={30u,1u,1u,14u,1u,1u,30u},
 [52]={2u,6u,10u,18u,31u,2u,2u},
 [53]={31u,16u,16u,30u,1u,1u,30u},
 [54]={14u,16u,16u,30u,17u,17u,14u},
 [55]={31u,1u,2u,4u,8u,8u,8u},
 [56]={14u,17u,17u,14u,17u,17u,14u},
 [57]={14u,17u,17u,15u,1u,1u,14u},
 [88]={17u,17u,10u,4u,10u,17u,17u},
 [71]={14u,17u,16u,23u,17u,17u,14u},
 [72]={17u,17u,17u,31u,17u,17u,17u},
 [90]={31u,1u,2u,4u,8u,16u,31u},
 [81]={14u,17u,17u,17u,21u,18u,13u},
 [74]={1u,1u,1u,1u,17u,17u,14u},
 [45]={0u,0u,0u,31u,0u,0u,0u},
 [95]={0u,0u,0u,0u,0u,0u,31u},
 [35]={10u,31u,10u,31u,10u,0u,0u},
 [58]={0u,4u,0u,0u,4u,0u,0u},
 [43]={0u,4u,4u,31u,4u,4u,0u},
 [47]={1u,2u,4u,8u,16u,0u,0u},
 [37]={0u,0u,0u,0u,0u,6u,6u},
 [46]={0u,0u,0u,0u,0u,6u,6u},
 [60]={2u,4u,8u,16u,8u,4u,2u},
 [62]={8u,4u,2u,1u,2u,4u,8u},
 [33]={4u,4u,4u,4u,4u,0u,4u},
 [63]={14u,17u,1u,2u,4u,0u,4u},
 [61]={0u,31u,0u,31u,0u,0u,0u},
 [40]={2u,4u,8u,8u,8u,4u,2u},
 [41]={8u,4u,2u,2u,2u,4u,8u},
 [91]={31u,16u,16u,16u,16u,16u,31u},
 [93]={31u,1u,1u,1u,1u,1u,31u},
};
static void track_pix_text(const char *str,uint16_t x,uint16_t y,uint16_t color){
 if(!str)return;
 for(uint16_t i=0;str[i]&&i<36u;i++){
  unsigned char c=(unsigned char)str[i];
  if(c>='a'&&c<='z')c=(unsigned char)(c-32u);
  if(c>=128u)continue;
  for(uint8_t r=0;r<7u;r++)for(uint8_t col=0;col<5u;col++)
   if(track_web_5x7[c][r]&(16u>>col))lcd_fill(x+(uint16_t)i*6u+col,y+r,1u,1u,color);
 }
}
static uint16_t track_pix_width(const char *s){uint16_t len=0;if(!s)return 0;
 while(s[len]&&len<36u)len++;return len?len*6u-1u:0u;
}
static void track_pix_center(const char *s,uint16_t cx,uint16_t y,uint16_t color){
 uint16_t w=track_pix_width(s);track_pix_text(s,cx>w/2u?cx-w/2u:0u,y,color);
}
static void track_pix_right(const char *s,uint16_t right,uint16_t y,uint16_t color){
 uint16_t w=track_pix_width(s);track_pix_text(s,right>=w?right-w:0,y,color);
}
static void track_pix_top(const char *title,uint8_t channel,uint16_t bpm){
 char ch[4]={(char)('1'+channel),'/','6',0};char bpm_text[9];
 uint8_t n=0u;if(bpm>=100u)bpm_text[n++]=(char)('0'+bpm/100u%10u);
 if(bpm>=10u)bpm_text[n++]=(char)('0'+bpm/10u%10u);
 bpm_text[n++]=(char)('0'+bpm%10u);bpm_text[n++]=' ';bpm_text[n++]='B';bpm_text[n++]='P';bpm_text[n++]='M';bpm_text[n]=0;
 track_pix_text(ch,8u,4u,TRACK_PINK);track_pix_right(bpm_text,228u,4u,TRACK_PINK);
 track_pix_center("BEETLE",120u,4u,TRACK_BRIGHT);
 if(title)track_pix_center(title,120u,21u,TRACK_BRIGHT);
}
#endif
