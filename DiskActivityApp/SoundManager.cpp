#include "SoundManager.h"
#include <mmsystem.h>
#include <math.h>
#include <thread>
#include <mutex>
#include <windows.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#pragma comment(lib, "winmm.lib")

// Data structure to keep track of playing sounds
struct SoundData
{
    HWAVEOUT hWaveOut;
    WAVEHDR waveHeader;
    short* waveData;
};

// Vector of active sounds, protected by a mutex
static std::vector<SoundData*> g_activeSounds;
static std::mutex g_soundMutex;

void PlayBeepSoundAsync()
{
    std::thread([]() {
        PlayBeepSound();
        }).detach();
}

void PlayBeepSound()
{
    for (int i = 0; i < 10; ++i)
    {
        int frequency = 200 + (rand() % 800);
        int duration = 100 + (rand() % 200);

        wchar_t buf[100];
        swprintf_s(buf, L"Playing tone: Frequency=%d, Duration=%d\n", frequency, duration);
        OutputDebugString(buf);

        GenerateToneAndPlay(700.0, 150); // 150ms burst at ~700 Hz

        // Add a delay between tones to make them distinct
        Sleep(50);
    }
}

void GenerateToneAndPlay(double baseFrequency, int durationMs)
{
    const int sampleRate = 44100; // Samples per second
    const int amplitude = 3000;   // Volume level
    const int numSamples = (sampleRate * durationMs) / 1000;

    short* waveData = new short[numSamples];

    // Generate waveform with randomized bursts, noise, and silence
    for (int i = 0; i < numSamples; ++i)
    {
        // Add random gaps (silence)
        if (rand() % 20 < 2) // ~10% chance of silence
        {
            waveData[i] = 0;
            continue;
        }

        // Vary frequency slightly for a dynamic pitch
        double variedFrequency = baseFrequency + ((rand() % 100) - 50); // +/-50 Hz variation

        // Generate a noisy wave
        double baseWave = amplitude * sin((2.0 * M_PI * variedFrequency * i) / sampleRate);
        double noise = (rand() % (amplitude * 2)) - amplitude; // Random noise
        waveData[i] = static_cast<short>((baseWave * 0.4) + (noise * 0.6)); // Blend sine wave and noise
    }

    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 1;
    waveFormat.nSamplesPerSec = sampleRate;
    waveFormat.nAvgBytesPerSec = sampleRate * sizeof(short);
    waveFormat.nBlockAlign = sizeof(short);
    waveFormat.wBitsPerSample = 16;

    HWAVEOUT hWaveOut;
    MMRESULT res = waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);

    if (res == MMSYSERR_NOERROR)
    {
        WAVEHDR waveHeader = {};
        waveHeader.lpData = (LPSTR)waveData;
        waveHeader.dwBufferLength = numSamples * sizeof(short);
        waveHeader.dwFlags = 0;

        waveOutPrepareHeader(hWaveOut, &waveHeader, sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, &waveHeader, sizeof(WAVEHDR));

        // Wait for playback to finish
        waveOutReset(hWaveOut);

        // Clean up
        waveOutUnprepareHeader(hWaveOut, &waveHeader, sizeof(WAVEHDR));
        waveOutClose(hWaveOut);
        delete[] waveData;
    }
    else
    {
        delete[] waveData;
    }
}


void CALLBACK waveOutCallback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    if (uMsg == WOM_DONE)
    {
        OutputDebugString(L"waveOutCallback WOM_DONE received.\n");

        std::lock_guard<std::mutex> lock(g_soundMutex);
        for (auto it = g_activeSounds.begin(); it != g_activeSounds.end(); ++it)
        {
            SoundData* sd = *it;
            if (sd->hWaveOut == hwo)
            {
                OutputDebugString(L"Cleaning up sound resources.\n");

                waveOutUnprepareHeader(hwo, &sd->waveHeader, sizeof(WAVEHDR));
                waveOutClose(hwo);
                delete[] sd->waveData;
                delete sd;
                g_activeSounds.erase(it);
                break;
            }
        }
    }
}


