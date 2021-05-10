#pragma once

#include <vector>
#include <algorithm>
#include "PhaseVocoder.h"

namespace stekyne
{

    template<class T>
    constexpr const T& clamp( const T& v, const T& lo, const T& hi )
    {
        if (v < lo)
            return lo;
        if (v > hi)
            return hi;

        return v;
    }
    
template<typename FloatType = float>
class PitchShifter : public PhaseVocoder<FloatType>
{
public:
	PitchShifter () :
		synthPhaseIncrements (this->windowSize, 0),
		previousFramePhases (this->windowSize, 0)
	{
		setPitchRatio (1.f);
	}

	void setPitchRatio (float ratio)
	{
		if (pitchRatio == ratio) 
			return;

		//const juce::SpinLock::ScopedLockType lock(paramLock);

		// Lower ratios require a larger amount of incoming samples
		// This will introduce more latency and large analysis and synthesis buffers
		pitchRatio = clamp (ratio,
                            PhaseVocoder<FloatType>::MinPitchRatio,
                            PhaseVocoder<FloatType>::MaxPitchRatio);
		
		this->synthesisHopSize =
            (int)(PhaseVocoder<FloatType>::windowSize /
                  (float)PhaseVocoder<FloatType>::MinOverlapAmount);
		this->analysisHopSize =
            (int)round (PhaseVocoder<FloatType>::synthesisHopSize / pitchRatio);
		this->resampleSize =
            (int)std::ceil (PhaseVocoder<FloatType>::windowSize *
                            PhaseVocoder<FloatType>::analysisHopSize /
                            (float)PhaseVocoder<FloatType>::synthesisHopSize);
		timeStretchRatio = PhaseVocoder<FloatType>::synthesisHopSize /
            (float)PhaseVocoder<FloatType>::analysisHopSize;

		// Rescaling due to OLA processing gain
		double accum = 0.0;

		for (int i = 0; i < PhaseVocoder<FloatType>::windowSize; ++i)
			accum += PhaseVocoder<FloatType>::windowBuffer[i] *
                (double)PhaseVocoder<FloatType>::windowBuffer[i];

		accum /= PhaseVocoder<FloatType>::synthesisHopSize;
		PhaseVocoder<FloatType>::rescalingFactor = (float)accum;
		this->synthesisHopSize = PhaseVocoder<FloatType>::analysisHopSize;

		// Reset phases
		memset(previousFramePhases.data(), 0,
               sizeof(FloatType) * PhaseVocoder<FloatType>::windowSize);
		memset(synthPhaseIncrements.data(), 0,
               sizeof(FloatType) * PhaseVocoder<FloatType>::windowSize);
	}

	void processImpl (FloatType* const buffer, const int bufferSize) override final
	{
		// Update phase increments for pitch shifting
		for (int i = 0, x = 0; i < bufferSize - 1; i += 2, ++x)
		{
			const auto real = buffer[i];
			const auto imag = buffer[i + 1];
			const auto mag = sqrtf (real * real + imag * imag);
			const auto phase = atan2 (imag, real);
			const auto omega =
                TWO_PI * PhaseVocoder<FloatType>::analysisHopSize * x /
                (float)PhaseVocoder<FloatType>::windowSize;

			const auto deltaPhase =
                omega +
                PhaseVocoder<FloatType>::principalArgument(phase -
                                                           previousFramePhases[x] -
                                                           omega);

			previousFramePhases[x] = phase;

			synthPhaseIncrements[x] =
                PhaseVocoder<FloatType>::principalArgument(synthPhaseIncrements[x] +
                                                           (deltaPhase *
                                                            timeStretchRatio));

			buffer[i] = mag * std::cos (synthPhaseIncrements[x]);
			buffer[i + 1] = mag * std::sin (synthPhaseIncrements[x]);
		}
	}

private:
	std::vector<FloatType> synthPhaseIncrements;
	std::vector<FloatType> previousFramePhases;
	
	float pitchRatio = 0.f;
	float timeStretchRatio = 1.f;
};

}
