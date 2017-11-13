/*
  ==============================================================================

   This file is part of the ADC 2017 DSP Workshop demo project.
   Copyright (c) 2017 - ROLI Ltd.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

#pragma once

#include "Oscillator.h"

//==============================================================================
class Voice  : public juce::MPESynthesiserVoice
{
public:
    Voice()
    {
        auto& masterGain = processorChain.get<masterGainIndex>();
        masterGain.setGainLinear (0.7f);

            // <- 4.3. get a reference to the filter with processorChain.get<>()
        auto& ladderFilter = processorChain.get<filterIndex>();
            // <- 4.4. set the cutoff frequency of the filter to 1 kHz
        ladderFilter.setCutoffFrequencyHz(1000.0f);
            // <- 4.5. set the resonance of the filter to 0.7
        ladderFilter.setResonance(0.7f);

        // <- 5.2. initialise the lfo with a sine wave
        lfo.initialise([] (float x) {
            return std::sin(x);
        }, 128 );
        // <- 5.3. change the lfo frequency to 3 Hz
        lfo.setFrequency(3.f);
    }

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        tempBlock = juce::dsp::AudioBlock<float> (heapBlock, spec.numChannels, spec.maximumBlockSize);
        processorChain.prepare (spec);

        // <- 5.4. set the sample rate of the lfo to spec.sampleRate / lfoUpdateRate
        juce::dsp::ProcessSpec mod_spec = spec;
        mod_spec.sampleRate = spec.sampleRate / lfoUpdateRate;
        lfo.prepare(mod_spec);
    }

    //==============================================================================
    void noteStarted() override
    {
        const auto velocity = getCurrentlyPlayingNote().noteOnVelocity.asUnsignedFloat();
        const auto freqHz = (float) getCurrentlyPlayingNote().getFrequencyInHertz();

        processorChain.get<osc1Index>().setFrequency (freqHz, true);
        processorChain.get<osc1Index>().setLevel (velocity);

            // <- 3.3. set the frequency of the second oscillator to 1.01 * freqHz
        //processorChain.get<osc2Index>().setFrequency(1.01f * freqHz, true);
        processorChain.get<osc2Index>().setFrequency(velocity * freqHz, true);
            // <- 3.4. set the level of the second oscillator
        processorChain.get<osc2Index>().setLevel(velocity);
    }

    //==============================================================================
    void notePitchbendChanged() override
    {
        const auto freqHz = (float) getCurrentlyPlayingNote().getFrequencyInHertz();
        processorChain.get<osc1Index>().setFrequency (freqHz);

            // <- 3.5. set the frequency of the second oscillator to 1.01 * freqHz
        processorChain.get<osc2Index>().setFrequency (freqHz);
    }

    //==============================================================================
    void noteStopped (bool /*allowTailOff*/) override
    {
        clearCurrentNote();
    }

    //==============================================================================
    void notePressureChanged() override {}
    void noteTimbreChanged() override   {}
    void noteKeyStateChanged() override {}

    //==============================================================================
    // add new samples ontop of each other
    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
            // <- 5.5. call processSample() on the lfo once per lfoUpdateRate input
            //         samples (use lfoUpdateCounter)
        for (int i = 0; i < numSamples; ++i)
        {
            if (--lfoUpdateCounter == 0)
            {
                lfoUpdateCounter = lfoUpdateRate;
                const auto lfoOut = lfo.processSample (0.0f);
                
                const auto cutoffFreqHz = jmap (lfoOut, -1.0f, 1.0f, 1e2f, 2e3f);
                processorChain.get<filterIndex>().setCutoffFrequencyHz (cutoffFreqHz);
            }
        }
            // <- 5.6. use the output of lfo.processSample() to change the cutoff
            //         frequency of the filter between 100 Hz and 2 kHz

        auto block = tempBlock.getSubBlock (0, (size_t) numSamples);
        block.clear();
        juce::dsp::ProcessContextReplacing<float> context (block);
        processorChain.process (context);

        juce::dsp::AudioBlock<float> (outputBuffer)
            .getSubBlock ((size_t) startSample, (size_t) numSamples)
            .add (tempBlock);
    }

private:
    //==============================================================================
    juce::HeapBlock<char> heapBlock;
    juce::dsp::AudioBlock<float> tempBlock;

    enum
    {
        osc1Index,
        // <- 3.2. add index for the second Oscillator
        osc2Index,
        // <- 4.2. add index for the Filter
        filterIndex,
        masterGainIndex
    };

    juce::dsp::ProcessorChain<
        Oscillator<float>,
            // <- 3.1. add a second Oscillator
        Oscillator<float>,
    
            // <- 4.1. add a juce::dsp::LadderFilter
        juce::dsp::LadderFilter<float>,
        juce::dsp::Gain<float>
    > processorChain;

    static constexpr size_t lfoUpdateRate = 100;
    size_t lfoUpdateCounter = lfoUpdateRate;
    // <- 5.1. declare a juce::dsp::Oscillator named lfo
    juce::dsp::Oscillator<float> lfo;
    
    
};
