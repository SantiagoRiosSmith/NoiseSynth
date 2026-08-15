#pragma once

#include <cstdint>
#include <string>
#include <vector>

class WavFile
{
public:
    bool load(const std::string& filePath);

    const std::vector<float>& getSamples() const;

    int getSampleRate() const;
    int getChannels() const;

private:
    std::vector<float> samples;

    int sampleRate = 0;
    int channels = 0;
};