#include "tinysynth.h"

static const unsigned g_outputRate = 53267;

struct OPN_Operator
{
    //! Raw register data
    uint8_t     data[7];
};

struct OPN_PatchSetup
{
    //! Operators prepared for sending to OPL chip emulator
    OPN_Operator    OPS[4];
    uint8_t         fbalg;
    uint8_t         lfosens;
    //! Fine tuning
    int8_t          finetune;
    //! Single note (for percussion instruments)
    uint8_t         tone;
};

void TinySynth::resetChip()
{
    m_chip->setRate(g_outputRate, 7670454);

    m_chip->writeReg(0, 0x22, 0x00);   //LFO off
    m_chip->writeReg(0, 0x27, 0x0 );   //Channel 3 mode normal

    //Shut up all channels
    m_chip->writeReg(0, 0x28, 0x00 );   //Note Off 0 channel
    m_chip->writeReg(0, 0x28, 0x01 );   //Note Off 1 channel
    m_chip->writeReg(0, 0x28, 0x02 );   //Note Off 2 channel
    m_chip->writeReg(0, 0x28, 0x04 );   //Note Off 3 channel
    m_chip->writeReg(0, 0x28, 0x05 );   //Note Off 4 channel
    m_chip->writeReg(0, 0x28, 0x06 );   //Note Off 5 channel

    //Disable DAC
    m_chip->writeReg(0, 0x2B, 0x0 );   //DAC off
}

void TinySynth::setInstrument(const FmBank::Instrument *in_p)
{
    const FmBank::Instrument &in = *in_p;
    OPN_PatchSetup patch;

    m_notenum = in.percNoteNum >= 128 ? (in.percNoteNum - 128) : in.percNoteNum;
    if(m_notenum == 0)
        m_notenum = 25;
    m_notesNum = 1;
    m_fineTune = 0;
    m_noteOffsets[0] = in.note_offset1;
    //m_noteOffsets[1] = in.note_offset2;

    for(int op = 0; op < 4; op++)
    {
        patch.OPS[op].data[0] = in.getRegDUMUL(op);
        patch.OPS[op].data[1] = in.getRegLevel(op);
        patch.OPS[op].data[2] = in.getRegRSAt(op);
        patch.OPS[op].data[3] = in.getRegAMD1(op);
        patch.OPS[op].data[4] = in.getRegD2(op);
        patch.OPS[op].data[5] = in.getRegSysRel(op);
        patch.OPS[op].data[6] = in.getRegSsgEg(op);
    }
    patch.fbalg    = in.getRegFbAlg();
    patch.lfosens  = 0;//Disable LFO sensitivity for clear measure
    patch.finetune = static_cast<int8_t>(in.note_offset1);
    patch.tone     = 0;

    m_c = 0;
    m_port = (m_c <= 2) ? 0 : 1;
    m_cc   = m_c % 3;

    for(uint8_t op = 0; op < 4; op++)
    {
        m_chip->writeReg(m_port, 0x30 + (op * 4) + m_cc, patch.OPS[op].data[0]);
        m_chip->writeReg(m_port, 0x40 + (op * 4) + m_cc, patch.OPS[op].data[1]);
        m_chip->writeReg(m_port, 0x50 + (op * 4) + m_cc, patch.OPS[op].data[2]);
        m_chip->writeReg(m_port, 0x60 + (op * 4) + m_cc, patch.OPS[op].data[3]);
        m_chip->writeReg(m_port, 0x70 + (op * 4) + m_cc, patch.OPS[op].data[4]);
        m_chip->writeReg(m_port, 0x80 + (op * 4) + m_cc, patch.OPS[op].data[5]);
        m_chip->writeReg(m_port, 0x90 + (op * 4) + m_cc, patch.OPS[op].data[6]);
    }
    m_chip->writeReg(m_port, 0xB0 + m_cc, patch.fbalg);
    m_chip->writeReg(m_port, 0xB4 + m_cc, 0xC0);
}

void TinySynth::noteOn()
{
    double hertz = 321.88557 * std::exp(0.057762265 * (m_notenum + m_noteOffsets[0]));
    uint16_t x2 = 0x0000;
    if(hertz < 0 || hertz > 262143)
    {
        std::fprintf(stderr, "MEASURER WARNING: Why does note %d + note-offset %d produce hertz %g?          \n",
                     m_notenum, m_noteOffsets[0], hertz);
        hertz = 262143;
    }

    while(hertz >= 2047.5)
    {
        hertz /= 2.0;    // Calculate octave
        x2 += 0x800;
    }
    x2 += static_cast<uint32_t>(hertz + 0.5);

    // Keyon the note
    m_chip->writeReg(m_port, 0xA4 + m_cc, (x2>>8) & 0xFF);//Set frequency and octave
    m_chip->writeReg(m_port, 0xA0 + m_cc,  x2 & 0xFF);

    m_chip->writeReg(0, 0x28, 0xF0 + uint8_t((m_c <= 2) ? m_c : m_c + 1));
}

void TinySynth::noteOff()
{
    // Keyoff the note
    uint8_t cc = static_cast<uint8_t>(m_c % 6);
    m_chip->writeReg(0, 0x28, (m_c <= 2) ? cc : cc + 1);
}

void TinySynth::generate(int16_t *output, size_t frames)
{
    m_chip->generate(output, frames);
}
