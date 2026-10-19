/* SPDX-License-Identifier: GPL-3.0-only
 * Stereo equal-power pan, integer Q8 LUT, computed once per render buffer.
 * Center is 256/256; fully left is 362/0, fully right is 0/362.
 */
#ifndef BEETLE_STEREO_PAN_H
#define BEETLE_STEREO_PAN_H
#include <stdint.h>
static const uint16_t beetle_pan_lower_q8[101]={256u,255u,253u,252u,251u,250u,248u,247u,246u,244u,243u,242u,240u,239u,237u,236u,235u,233u,232u,230u,229u,228u,226u,225u,223u,222u,220u,219u,217u,216u,214u,213u,211u,210u,208u,206u,205u,203u,202u,200u,198u,197u,195u,193u,192u,190u,188u,186u,185u,183u,181u,179u,177u,176u,174u,172u,170u,168u,166u,164u,162u,160u,158u,156u,154u,151u,149u,147u,145u,143u,140u,138u,135u,133u,131u,128u,125u,123u,120u,117u,114u,112u,109u,106u,102u,99u,96u,92u,89u,85u,81u,77u,72u,68u,63u,57u,51u,44u,36u,26u,0u};
static const uint16_t beetle_pan_higher_q8[101]={256u,257u,259u,260u,261u,262u,264u,265u,266u,267u,268u,270u,271u,272u,273u,275u,276u,277u,278u,279u,280u,282u,283u,284u,285u,286u,287u,288u,290u,291u,292u,293u,294u,295u,296u,297u,299u,300u,301u,302u,303u,304u,305u,306u,307u,308u,309u,310u,311u,312u,314u,315u,316u,317u,318u,319u,320u,321u,322u,323u,324u,325u,326u,327u,328u,329u,330u,331u,332u,333u,334u,335u,336u,337u,338u,339u,340u,341u,342u,343u,343u,344u,345u,346u,347u,348u,349u,350u,351u,352u,353u,354u,355u,356u,357u,357u,358u,359u,360u,361u,362u};
static inline void beetle_stereo_pan_gain(int32_t pan,uint16_t *l,uint16_t *r){
 if(pan<-100)pan=-100;if(pan>100)pan=100;
 uint32_t x=(uint32_t)(pan<0?-pan:pan);
 uint16_t lower=beetle_pan_lower_q8[x];
 uint16_t higher=beetle_pan_higher_q8[x];
 *l=pan<0?higher:lower;
 *r=pan>0?higher:lower;
}
#endif
