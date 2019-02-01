#pragma once
#include "bank.h"
#include "opl/chips/opn_chip_base.h"
#include <stdint.h>

struct TinySynth
{
    //! Context of the chip emulator
    OPNChipBase *m_chip;
    //! Count of playing notes
    unsigned m_notesNum;
    //! MIDI note to play
    int m_notenum;
    //! Centy detune
    int8_t  m_fineTune;
    //! Half-tone offset
    int16_t m_noteOffsets[2];

    //! Absolute channel
    uint32_t    m_c;
    //! Port of OPN2 chip
    uint8_t     m_port;
    //! Relative channel
    uint8_t     m_cc;

    void resetChip();
    void setInstrument(const FmBank::Instrument *in_p);
    void noteOn();
    void noteOff();
    void generate(int16_t *output, size_t frames);
};
