
#include <iostream>
#include <string>
#include <iomanip>
#include <portaudio.h>

//// CONFIGURATION CONSTANTS | KONFIGURACIJSKE KONSTANTE:
constexpr uint16_t SAMPLE_RATE = 48000; // Sampling frequency (Hz) | Frekvencija uzorkovanja (Hz)
constexpr size_t BUFFER_SIZE = 4096; // Buffer size (B) | Velicina spremnika (B)
constexpr float MIN_FREQUENCY = 80.0f; // 82.41f; // Minimum frequency (E2) | Najniza frekvencija (E2)
constexpr float MAX_FREQUENCY = 1320.0f; // 1318.51f; // Maximum frequency (E6) | Najvise frekvencija (E6)
constexpr float THRESHOLD = 0.1f; // Threshold | Prag

//// ALGORITHM FOR PITCH DETECTION | ALGORITAM ZA PREPOZNAVANJE VISINE TONA:
// 
float detectPitch(const float* input_buffer, int buffer_size, int sample_rate, float min_freq, float max_freq, float threshold) {
	int tau_min = sample_rate / max_freq;
	int tau_max = sample_rate / min_freq;

	// Calculate DF | Izracunaj DF
	float* DF_buffer = new float[buffer_size / 2];

	for (int tau = 0; tau < buffer_size / 2; tau++) {
		float difference_sum = 0;
		for (int t = 0; t < buffer_size / 2; t++) {
			float difference = input_buffer[t] - input_buffer[t + tau];
			difference_sum += difference * difference;
		}
		DF_buffer[tau] = difference_sum;
	}

	// Calculate CMNDF | Izracunaj CMNDF
	float* CMNDF_buffer = new float[buffer_size / 2];
	CMNDF_buffer[0] = 1.0f;

	float cumulative_sum = DF_buffer[0];
	for (int tau = 1; tau < buffer_size / 2; tau++) {
		cumulative_sum += DF_buffer[tau];
		CMNDF_buffer[tau] = DF_buffer[tau] * tau / cumulative_sum;
	}

	// Find best pitch candidate | Pronadi najboljeg kandidata visine tona
	float best_pitch = -1.0f;
	float best_correlation = threshold;
	for (int tau = tau_min; tau < tau_max; tau++) {
		if (CMNDF_buffer[tau] < best_correlation) {
			best_correlation = CMNDF_buffer[tau];
			best_pitch = static_cast<float>(sample_rate) / tau;
		}
	}

	delete[] DF_buffer;
	delete[] CMNDF_buffer;

	return best_pitch;
}

std::string getNoteFromPitch(float pitch) {

	static const std::string note_names[] = {
		"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
	};
	int note_MIDI = static_cast<int>(round(12 * log2(pitch / 440.0) + 69));
	int octave = (note_MIDI / 12) - 1;

	return note_names[note_MIDI % 12] + std::to_string(octave);
}

//// PORTAUDIO CALLBACK FUNCTION | FUNKCIJA POVRATNOG POZIVA PORTAUDIO-A:
int paCallback(const void* input_buffer, void* output_buffer,
			   unsigned long frames_per_buffer,
			   const PaStreamCallbackTimeInfo* time_info,
			   PaStreamCallbackFlags status_flags,
			   void* user_data) {

	float detected_pitch = detectPitch((const float*) input_buffer, BUFFER_SIZE, SAMPLE_RATE, MIN_FREQUENCY, MAX_FREQUENCY, THRESHOLD);
	if (detected_pitch != -1) {
		std::cout << "Note: " << std::setw(5) << std::left << getNoteFromPitch(detected_pitch) << "Pitch: " << std::fixed << std::setprecision(2) << detected_pitch << " Hz     " << std::endl;
	}
	return paContinue;
}

int main() {
	// PortAudio:
	PaStream* stream;
	PaError err;

	// Initialize PortAudio | Inicijaliziraj PortAudio
	err = Pa_Initialize();
	if (err != paNoError) {
		std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
		return -1;
	}

	// Open PortAudio stream | Otvori strujanje PortAudio-a
	err = Pa_OpenDefaultStream(&stream, 1, 0, paFloat32, SAMPLE_RATE,
							   BUFFER_SIZE, paCallback, nullptr);
	if (err != paNoError) {
		std::cerr << "PortAudio stream opening failed: " << Pa_GetErrorText(err) << std::endl;
		Pa_Terminate();
		return -1;
	}

	// Start the stream | Pokreni strujanje
	err = Pa_StartStream(stream);
	if (err != paNoError) {
		std::cerr << "PortAudio stream starting failed: " << Pa_GetErrorText(err) << std::endl;
		Pa_CloseStream(stream);
		Pa_Terminate();
		return -1;
	}

	// Wait for input to finish | Pricekaj 
	std::cout << "Press Enter to quit..." << std::endl;
	std::cin.get();

	// Stop and close the stream | Zaustavi i zatvori strujanje
	err = Pa_StopStream(stream);
	if (err != paNoError) {
		std::cerr << "PortAudio stream stopping failed: " << Pa_GetErrorText(err) << std::endl;
	}
	err = Pa_CloseStream(stream);
	if (err != paNoError) {
		std::cerr << "PortAudio stream closing failed: " << Pa_GetErrorText(err) << std::endl;
	}

	// Terminate PortAudio | Prekini PortAudio
	Pa_Terminate();

	return 0;
}