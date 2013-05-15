# sysAudioSpectrogram

This is a sample **windows** project which visualizes system [WASAPI loopback capture](http://blogs.msdn.com/b/matthew_van_eerde/archive/2008/12/16/sample-wasapi-loopback-capture-record-what-you-hear.aspx) as spectrogram by using kissFFT and FFTW.


## Todo
```
Indicate music scale.
Frequency-dB line chart.
Replace highgui with LayeredWindowD2D
```

## Buliding Prerequisites & Dependencies
* Microsoft Visual Studio 2012 Express
* OpenCV 
	** (In this project, I only use the highgui to create window and show some FFT results. Highgui will be replace with LayeredWindowD2D(Windows API) in the future.)
* KissFFT
* FFTW (Optional)

## Authors

**Harpseal Tsai**

+ [Google Plus](https://plus.google.com/u/1/104780260310145497080/)
+ [Twitter](https://twitter.com/HarpsealTsai)


## Attribution

Some source code originally from the windows SDK samples, some taken from [1]
So you'll probably need to install the Windows SDK before playing around with the source code, legally.

[1] http://blogs.msdn.com/b/matthew_van_eerde/archive/2008/12/16/sample-wasapi-loopback-capture-record-what-you-hear.aspx