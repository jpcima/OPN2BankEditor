#include "tinysynth.h"
#include "opl/chips/np2_opna.h"
#include "opl/chips/nuked_opn2.h"
#include "opl/chips/mame_opna.h"
#include "FileFormats/format_wohlstand_opn2.h"

typedef NukedOPN2 RefOPN2;
//typedef MameOPNA RefOPN2;
typedef NP2OPNA<> TestOPN2;

enum {
    EG_CONT = 8,
    EG_ATT = 4,
    EG_ALT = 2,
    EG_HOLD = 1,
};

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
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "please give opni file\n");
        return 1;
    }

    WohlstandOPN2 loader;
    FmBank::Instrument ins;
    if (loader.loadFileInst(argv[1], ins) != FfmtErrCode::ERR_OK) {
        fprintf(stderr, "cannot load opni\n");
        return 1;
    }

#warning XXX
//ins.OP[0].attack = 5;
fprintf(stderr, "AR1 %d\n", ins.OP[0].attack);
fprintf(stderr, "AR2 %d\n", ins.OP[1].attack);
fprintf(stderr, "AR3 %d\n", ins.OP[2].attack);
fprintf(stderr, "AR4 %d\n", ins.OP[3].attack);

#warning XXX
    if (argc == 3) {
        int newssg = strtoul(argv[2], nullptr, 0) & 7;
        newssg |= EG_CONT;
        if (newssg & EG_ALT) fprintf(stderr, "SSG: ALT\n");
        if (newssg & EG_ATT) fprintf(stderr, "SSG: ATT\n");
        if (newssg & EG_HOLD) fprintf(stderr, "SSG: HOLD\n");

        //int newssg = 0;
        //newssg |= EG_CONT;
        //newssg |= EG_ALT;
        //newssg |= EG_ATT;
        // newssg |= EG_HOLD;
        for (int i = 0; i < 4; ++i) {
            ins.OP[i].ssg_eg = newssg;
        }
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

    // multi.generate(bufs, frames);
    multi.noteOn();
    for (unsigned i = 0; i < 30; ++i) {
        multi.generate(bufs, frames);
        if (0) for (int f = 0; f < frames; ++f) printf("%d %d\n", bufs[0][2 * f], bufs[1][2 * f]);
    }
    multi.noteOff();
    multi.generate(bufs, frames);
    if (0) for (int f = 0; f < frames; ++f) printf("%d %d\n", bufs[0][2 * f], bufs[1][2 * f]);

    return 0;
}
