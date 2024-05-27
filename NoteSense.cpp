
#include <iostream>
#include <portaudio.h>

//// CONFIGURATION CONSTANTS | KONFIGURACIJSKE KONSTANTE:
constexpr uint16_t SAMPLE_RATE = 44100; // Hz
constexpr size_t BUFFER_SIZE = 512; // B
constexpr float MIN_FREQUENCY = 82.41f; // Minimum frequency (E2) | Najniza frekvencija (E2)
constexpr float MAX_FREQUENCY = 1318.51f; // Maximum frequency (E6) | Najvise frekvencija (E6)

//// YIN ALGORITHM FOR PITCH DETECTION | YIN ALGORITAM ZA PREPOZNAVANJE VISINE TONA:
float YinPitchDetection(const float* input_buffer, int buffer_size, int sample_rate) {
	float threshold = 0.2f; // Threshold for peak detection
	
	float* difference_buffer = new float[buffer_size / 2];

	// Calculate difference function
	for(int tau = 0; tau < buffer_size / 2; tau++) {
		float difference_sum = 0;
		for(int t = 0; t < buffer_size / 2; t++) {
			float difference = input_buffer[t] - input_buffer[t + tau];
			difference_sum += difference * difference;
		}
		difference_buffer[tau] = difference_sum;
	}

	// Calculate cumulative mean normalized difference function
	float* cumulative_mean_normalized_difference = new float[buffer_size / 2];
	cumulative_mean_normalized_difference[0] = 1;
	for(int tau = 1; tau < buffer_size / 2; tau++) {
		float cumulative_sum = 0;
		for(int j = 1; j <= tau; j++) {
			cumulative_sum += difference_buffer[j];
		}
		cumulative_mean_normalized_difference[tau] = difference_buffer[tau] / (cumulative_sum / tau);
	}

	// Find best pitch candidate
	float best_pitch = -1;
	float best_correlation = 1;
	for(int tau = 0; tau < buffer_size / 2; tau++) {
		if(cumulative_mean_normalized_difference[tau] < threshold) {
			float freq = sample_rate / tau;
			if(freq >= MIN_FREQUENCY && freq <= MAX_FREQUENCY) {
				if(cumulative_mean_normalized_difference[tau] < best_correlation) {
					best_correlation = cumulative_mean_normalized_difference[tau];
					best_pitch = freq;
				}
			}
		}
	}

	delete[] difference_buffer;
	delete[] cumulative_mean_normalized_difference;

	return best_pitch;
}

void processAudio(const float* inputBuffer, int bufferSize) {
	// Perform FFT on inputBuffer
	// Run pitch detection algorithm (e.g., YIN) on FFT output
	// Print detected note to console
	//std::cout << "Detected Note: C" << std::endl; // Example output

	static const float pitch_epsilon = 1.0f;
	static float last_detected_pitch = -1;

	float detectedPitch = YinPitchDetection(inputBuffer, BUFFER_SIZE, SAMPLE_RATE);
	if(detectedPitch != -1 && std::fabs(detectedPitch - last_detected_pitch) > pitch_epsilon) {
		std::cout << detectedPitch << std::endl;
		last_detected_pitch = detectedPitch;
	}
}

//// PORTAUDIO CALLBACK FUNCTION | FUNKCIJA POVRATNOG POZIVA PORTAUDIO-A:
int paCallback(const void* inputBuffer, void* outputBuffer,
			   unsigned long framesPerBuffer,
			   const PaStreamCallbackTimeInfo* timeInfo,
			   PaStreamCallbackFlags statusFlags,
			   void* userData) {

	const float* in = (const float*)inputBuffer;
	processAudio(in, framesPerBuffer);

	return paContinue;
}

int main() {
	// PortAudio:
	PaStream* stream;
	PaError err;

	// Initialize PortAudio | Inicijaliziraj PortAudio
	err = Pa_Initialize();
	if(err != paNoError) {
		std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
		return -1;
	}

	// Open PortAudio stream | Otvori strujanje PortAudio-a
	err = Pa_OpenDefaultStream(&stream, 1, 0, paFloat32, SAMPLE_RATE,
							   BUFFER_SIZE, paCallback, nullptr);
	if(err != paNoError) {
		std::cerr << "PortAudio stream opening failed: " << Pa_GetErrorText(err) << std::endl;
		Pa_Terminate();
		return -1;
	}

	// Start the stream | Pokreni strujanje
	err = Pa_StartStream(stream);
	if(err != paNoError) {
		std::cerr << "PortAudio stream starting failed: " << Pa_GetErrorText(err) << std::endl;
		Pa_CloseStream(stream);
		Pa_Terminate();
		return -1;
	}

	// Wait for input to finish
	std::cout << "Press Enter to quit..." << std::endl;
	std::cin.get();

	// Stop and close the stream | Zaustavi i zatvori strujanje
	err = Pa_StopStream(stream);
	if(err != paNoError) {
		std::cerr << "PortAudio stream stopping failed: " << Pa_GetErrorText(err) << std::endl;
	}
	err = Pa_CloseStream(stream);
	if(err != paNoError) {
		std::cerr << "PortAudio stream closing failed: " << Pa_GetErrorText(err) << std::endl;
	}

	// Terminate PortAudio | Prekini PortAudio
	Pa_Terminate();

	return 0;
}