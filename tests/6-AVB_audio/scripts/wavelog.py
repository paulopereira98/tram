import matplotlib.pyplot as plt
import wave
import numpy as np

wav_obj = wave.open('wave100hz.wav', 'rb')
sample_freq = wav_obj.getframerate()
print("sample freq: ", sample_freq)

n_samples = wav_obj.getnframes()
print("n samples: ", n_samples)


t_audio = n_samples/sample_freq
print("t audio: ", t_audio)

n_channels = wav_obj.getnchannels()
print("n channels: ", n_samples)


signal_wave = wav_obj.readframes(n_samples)

signal_array = np.frombuffer(signal_wave, dtype=np.int16)

l_channel = signal_array[0::2]
r_channel = signal_array[1::2]


times = np.linspace(0, n_samples/sample_freq, num=n_samples)


plt.figure(figsize=(15, 5))

plt.plot(times, r_channel)
plt.title('Left Channel')
plt.ylabel('Signal Value')
plt.xlabel('Time (s)')
plt.xlim(0, t_audio)
plt.show()


plt.figure(figsize=(15, 5))
plt.specgram(r_channel, Fs=sample_freq, vmin=-20, vmax=10)
plt.title('Left Channel')
plt.ylabel('Frequency (Hz)')
plt.xlabel('Time (s)')
plt.xlim(0, t_audio)
plt.colorbar()
plt.show()



from numpy.fft import fft, ifft

X = fft(r_channel)
N = n_samples
n = np.arange(N)
T = N/sample_freq
freq = n/T 

plt.figure(figsize = (12, 6))
plt.subplot(121)

plt.stem(freq, np.abs(X), 'b', \
         markerfmt=" ", basefmt="-b")
plt.xlabel('Freq (Hz)')
plt.ylabel('FFT Amplitude |X(freq)|')
plt.xlim(0, 10)

plt.subplot(122)
plt.plot(T, ifft(X), 'r')
plt.xlabel('Time (s)')
plt.ylabel('Amplitude')
plt.tight_layout()
plt.show()
