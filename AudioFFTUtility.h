#ifndef AUDIO_FFT_UTILITY_H_
#define AUDIO_FFT_UTILITY_H_

namespace AudioFFTUtility
{
	const float cfNoteCNeg1Freq = 8.17579891564f;
	const float cfNoteCFreq[11] = {
		cfNoteCNeg1Freq*    2, cfNoteCNeg1Freq*    4,cfNoteCNeg1Freq*    8,
		cfNoteCNeg1Freq*   16, cfNoteCNeg1Freq*   32,cfNoteCNeg1Freq*   64,
		cfNoteCNeg1Freq*  128, cfNoteCNeg1Freq*  256,cfNoteCNeg1Freq*  512,
		cfNoteCNeg1Freq* 1024, cfNoteCNeg1Freq* 2048};
	const float cfLog2 = log(2);
	//float log2f( float n )  
	//{  
 //   // log(n)/log(2) is log2.  
	//	return log( n ) / cfLog2;  
	//}
	inline float Mag2dB(float mag){return (20*log10(mag));}
	inline float Freq2Pitch(float freq){return 69+12*(log(freq/440.f)/cfLog2);}
	inline float Pitch2Freq(float pitch){return 440.f*powf(2,(pitch-69.f)/12.f);}
};


#ifndef MIN
#define MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif


#endif