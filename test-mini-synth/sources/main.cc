#include "tinysynth.h"
#include "opl/chips/np2_opna.h"
#include "opl/chips/nuked_opn2.h"
#include "opl/chips/mame_opna.h"
#include "FileFormats/format_wohlstand_opn2.h"

//typedef NukedOPN2 RefOPN2;
typedef MameOPNA RefOPN2;
typedef NP2OPNA<> TestOPN2;

struct MultiSynth
{
    std::vector<TinySynth *> m_multi;

    void setNoteNum(unsigned note)
        { for (TinySynth *s : m_multi) s->m_notenum = note; }

    void resetChip()
        { for (TinySynth *s : m_multi) s->resetChip(); }
    void setInstrument(const FmBank::Instrument *in_p)
        { for (TinySynth *s : m_multi) s->setInstrument(in_p); }
    void noteOn()
        { for (TinySynth *s : m_multi) s->noteOn(); }
    void noteOff()
        { for (TinySynth *s : m_multi) s->noteOff(); }
    void generate(int16_t *output[], size_t frames)
        { for (size_t i = 0, n = m_multi.size(); i < n; ++i) m_multi[i]->generate(output[i], frames); }
};

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "please give opni file\n");
        return 1;
    }

    WohlstandOPN2 loader;
    FmBank::Instrument ins;
    if (loader.loadFileInst(argv[1], ins) != FfmtErrCode::ERR_OK) {
        fprintf(stderr, "cannot load opni\n");
        return 1;
    }

    RefOPN2 refchip(OPNChip_OPN2);
    TestOPN2 testchip(OPNChip_OPN2);

    TinySynth refsyn = {};
    refsyn.m_chip = &refchip;
    TinySynth testsyn = {};
    testsyn.m_chip = &testchip;

    MultiSynth multi;
    multi.m_multi = {&refsyn, &testsyn};
    multi.resetChip();
    multi.setInstrument(&ins);
    multi.setNoteNum(69);

    constexpr unsigned frames = 1024;
    int16_t refbuf[2 * frames];
    int16_t testbuf[2 * frames];
    int16_t *bufs[] {refbuf, testbuf};

    multi.generate(bufs, frames);
    multi.noteOn();
    multi.generate(bufs, frames);
    multi.noteOff();
    multi.generate(bufs, frames);

    return 0;
}
