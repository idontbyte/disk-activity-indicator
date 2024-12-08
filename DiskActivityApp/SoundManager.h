#pragma once
#include <windows.h>
#include <vector>

// Asynchronous sound manager for playing tones without blocking the UI

// Play a series of beeps asynchronously
void PlayBeepSoundAsync();

// Called from PlayBeepSoundAsync worker to generate random beeps
void PlayBeepSound();

// Generate and play a single tone asynchronously
void GenerateToneAndPlay(double frequency, int durationMs);

// waveOut callback for asynchronous playback
void CALLBACK waveOutCallback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
