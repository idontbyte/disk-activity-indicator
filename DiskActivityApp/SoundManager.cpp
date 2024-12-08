#include "SoundManager.h"
#include <mmsystem.h>
#include <math.h>
#include <thread>
#include <mutex>
#include <windows.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "DiskActivityMonitor.h"

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
    extern DiskActivityMonitor monitor; // Ensure access to the monitor
    double maxActivity = 10000000;      // Define a maximum activity level for scaling

    for (int i = 0; i < 10; ++i)
    {
        monitor.Update(); // Update the monitor to get the latest activity
        double diskActivity = monitor.GetCurrentActivity();

        int duration = 100 + (rand() % 200);
        GenerateToneAndPlay(400.0, diskActivity, maxActivity, duration); // Base frequency 400 Hz

        // Add a delay between tones to make them distinct
        Sleep(50);
    }
}


double GenerateSafeFrequency(double baseFreq, double diskActivity, double maxActivity)
{
    // Scale frequency with activity, but less aggressively
    double frequency = baseFreq + (diskActivity / maxActivity) * 10.0; 

    // Ensure a lower bound to avoid too-low pitch and potential division by zero
    if (frequency < 5.0) frequency = 5.0; // at least 100 Hz

    // Ensure upper bound to avoid extremely high pitch
    if (frequency > 1000.0) frequency = 1000.0;

    return frequency;
}

void GenerateToneAndPlay(double baseFrequency, double diskActivity, double maxActivity, int durationMs)
{
    if (durationMs <= 0) durationMs = 50;
    const int sampleRate = 44100;
    const int amplitude = 3000;
    const int numSamples = (sampleRate * durationMs) / 1000;

    if (numSamples <= 0) return;

    short* waveData = new short[numSamples];

    double frequency = GenerateSafeFrequency(baseFrequency, diskActivity, maxActivity);

    for (int i = 0; i < numSamples; ++i)
    {
        // Smaller variation range: ±5 Hz instead of ±10
        double variedFrequency = frequency + ((rand() % 10) - 5);
        if (variedFrequency < 100.0) variedFrequency = 100.0;
        if (variedFrequency > 2000.0) variedFrequency = 2000.0;

        double periodDouble = (double)sampleRate / variedFrequency;
        if (periodDouble < 1.0) periodDouble = 1.0;
        int period = (int)periodDouble;
        if (period <= 0) period = 1;

        double saw = ((double)(i % period) / period) * 2.0 - 1.0;
        double baseWave = amplitude * saw * 0.5;

        double subtleNoise = ((rand() % 200) - 100) * 0.05;
        double sampleValue = baseWave + subtleNoise;

        if ((rand() % 50) == 0) {
            sampleValue *= 0.7;
        }

        // Clamp values before bit-crushing
        if (sampleValue > 32767.0) sampleValue = 32767.0;
        if (sampleValue < -32768.0) sampleValue = -32768.0;

        int sampleInt = (int)sampleValue & 0xFFF0;
        waveData[i] = (short)sampleInt;
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

        waveOutReset(hWaveOut);

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


