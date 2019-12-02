/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2019 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "format_m2v_gyb.h"
#include "../common.h"

#include <QSet>
#include <QFileInfo>
#include <QByteArray>
#include <algorithm>

QString Basic_M2V_GYB::formatExtensionMask() const
{
    return "*.gyb";
}

// TODO: Implement a hash summ generator and checker
FfmtErrCode Basic_M2V_GYB::loadFileVersion1Or2(QFile &file, FmBank &bank, uint8_t version)
{
    uint8_t header[5];
    if(file.read(char_p(header), 5) != 5)
        return FfmtErrCode::ERR_BADFORMAT;

    const unsigned melo_count = header[3];
    const unsigned drum_count = header[4];

    if(melo_count > 128 || drum_count > 128)
        return FfmtErrCode::ERR_BADFORMAT;

    const unsigned total_count = melo_count + drum_count;
    uint8_t ins_map[2 * 128];

    if(file.read(char_p(ins_map), sizeof(ins_map)) != sizeof(ins_map))
        return FfmtErrCode::ERR_BADFORMAT;

    bank.reset(1, 1);

    if(version == 2)
    {
        uint8_t regLfo;
        if(file.read(char_p(&regLfo), 1) != 1)
            return FfmtErrCode::ERR_BADFORMAT;
        bank.setRegLFO(regLfo);
    }

    memset(&bank.Banks_Melodic[0], 0, sizeof(FmBank::MidiBank));
    memset(&bank.Banks_Percussion[0], 0, sizeof(FmBank::MidiBank));
    for(unsigned i = 0; i < 128; ++i)
    {
        bank.Ins_Melodic[i].is_blank = true;
        bank.Ins_Percussion[i].is_blank = true;
    }

    for(unsigned ins_index = 0; ins_index < total_count; ++ins_index)
    {
        bool isdrum = ins_index >= melo_count;

        uint8_t idata[32];
        uint8_t idatalen = (version == 2) ? 32 : 30;

        if(file.read(char_p(idata), idatalen) != idatalen)
            return FfmtErrCode::ERR_BADFORMAT;

        unsigned gm = ~0u;
        // search for GM assignment in map
        for(unsigned map_index = 0; gm == ~0u && map_index < 128; ++map_index)
        {
            if(ins_map[2 * map_index + isdrum] == ((!isdrum) ? ins_index : (ins_index - melo_count)))
                gm = map_index;
        }
        // if not assigned, use index but only if it's a free GM slot
        if(gm == ~0u)
        {
            unsigned candidate_gm = ((!isdrum) ? ins_index : (ins_index - melo_count));
            bool is_free = ins_map[2 * candidate_gm + isdrum] >= 128;
            if(is_free)
                gm = candidate_gm;
        }
        // if cannot be assigned, drop
        if(gm == ~0u)
            continue;  // not assigned

        FmBank::Instrument *ins = isdrum ?
            &bank.Ins_Percussion[gm] : &bank.Ins_Melodic[gm];

        ins->is_blank = false;

        for(unsigned op_index = 0; op_index < 4; ++op_index)
        {
            const unsigned opnum[4] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};
            unsigned op = opnum[op_index];

            ins->setRegDUMUL(op, idata[0 + op_index]);
            ins->setRegLevel(op, idata[4 + op_index]);
            ins->setRegRSAt(op, idata[8 + op_index]);
            ins->setRegAMD1(op, idata[12 + op_index]);
            ins->setRegD2(op, idata[16 + op_index]);
            ins->setRegSysRel(op, idata[20 + op_index]);
            ins->setRegSsgEg(op, idata[24 + op_index]);
        }
        ins->setRegFbAlg(idata[28]);

        if(version == 2)
            ins->setRegLfoSens(idata[29]);

        uint8_t transposeOrKey = idata[(version == 2) ? 30 : 29];
        if(!isdrum)
            ins->note_offset1 = static_cast<int16_t>(-static_cast<int8_t>(transposeOrKey));
        else
            ins->percNoteNum = transposeOrKey & 127;
    }

    for(unsigned ins_index = 0; ins_index < total_count; ++ins_index)
    {
        bool isdrum = ins_index >= melo_count;

        uint8_t text_size;
        if(file.read(char_p(&text_size), 1) != 1)
            return FfmtErrCode::ERR_BADFORMAT;

        unsigned gm = ~0u;
        // search for GM assignment in map
        for(unsigned map_index = 0; gm == ~0u && map_index < 128; ++map_index)
        {
            if(ins_map[2 * map_index + isdrum] == ((!isdrum) ? ins_index : (ins_index - melo_count)))
                gm = map_index;
        }
        // if not assigned, use index but only if it's a free GM slot
        if(gm == ~0u)
        {
            unsigned candidate_gm = ((!isdrum) ? ins_index : (ins_index - melo_count));
            bool is_free = ins_map[2 * candidate_gm + isdrum] >= 128;
            if(is_free)
                gm = candidate_gm;
        }
        // if cannot be assigned, drop
        if(gm == ~0u)
        {
            if(!file.seek(file.pos() + text_size))
                return FfmtErrCode::ERR_BADFORMAT;
            continue;  // not assigned
        }

        unsigned text_skip = 0;
        if(text_size > 32)
        {
            text_skip = text_size - 32;
            text_size = 32;
        }

        FmBank::Instrument *ins = isdrum ?
            &bank.Ins_Percussion[gm] : &bank.Ins_Melodic[gm];

        if(file.read(ins->name, text_size) != text_size)
            return FfmtErrCode::ERR_BADFORMAT;

        if(!file.seek(file.pos() + text_skip))
            return FfmtErrCode::ERR_BADFORMAT;
    }

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode Basic_M2V_GYB::loadFileVersion3(QFile &file, FmBank &bank)
{
    uint8_t header[16];
    if(file.read(char_p(header), 16) != 16)
        return FfmtErrCode::ERR_BADFORMAT;

    bank.setRegLFO(header[3]);

    const uint32_t offset_banks = toUint32LE(&header[8]);
    const uint32_t offset_maps = toUint32LE(&header[12]);

    //////////////////////////
    // (1) Instrument Count //
    //////////////////////////

    // excursion into the banks part, in order to get the instrument count
    uint16_t inst_count;
    if(!file.seek(offset_banks) || file.read(char_p(&inst_count), 2) != 2)
        return FfmtErrCode::ERR_BADFORMAT;
    inst_count = toUint16LE((uint8_t *)&inst_count);

    /////////////////////////
    // (2) Instrument Maps //
    /////////////////////////

    if(!file.seek(offset_maps))
        return FfmtErrCode::ERR_BADFORMAT;

    // collect mapping information of instruments, in relation with their GYB index
    struct InstMappingInfo {
        bool isdrum;
        std::list<FmBank::Instrument *> targets;
        InstMappingInfo() : isdrum(false) {}
    };

    std::vector<InstMappingInfo> mapInfo(inst_count);

    for(unsigned nth_map = 0; nth_map < 2; ++nth_map)
    {
        bool isdrum = nth_map == 1;

        for(unsigned gm = 0; gm < 128; ++gm)
        {
            uint16_t sub_entry_count = 0;
            if(file.read(char_p(&sub_entry_count), 2) != 2)
                return FfmtErrCode::ERR_BADFORMAT;
            sub_entry_count = toUint16LE((uint8_t *)&sub_entry_count);

            for(unsigned nth_ent = 0; nth_ent < sub_entry_count; ++ nth_ent)
            {
                uint8_t ent_data[4];
                if(file.read(char_p(ent_data), 4) != 4)
                    return FfmtErrCode::ERR_BADFORMAT;

                uint8_t msb = ent_data[0];
                uint8_t lsb = ent_data[1];
                uint16_t inst_index = toUint16LE(&ent_data[2]);

                // 0xFF for MSB & LSB means "all". I don't know what to make of
                // it, so make it zero instead.
                if (msb == 0xff)
                    msb = 0;
                if (lsb == 0xff)
                    lsb = 0;

                if (msb >= 128 || lsb >= 128)
                    return FfmtErrCode::ERR_BADFORMAT;

                FmBank::Instrument *midi_bank_insts = nullptr;
                bank.createBank(msb, lsb, isdrum, nullptr, &midi_bank_insts);

                // high bit indicates drum, but it's already determined above
                inst_index &= (1 << 15) - 1;

                if(inst_index >= inst_count)
                    return FfmtErrCode::ERR_BADFORMAT;

                InstMappingInfo &info = mapInfo[inst_index];
                info.isdrum = isdrum;
                info.targets.push_back(&midi_bank_insts[gm]);
            }
        }
    }

    //////////////////////////
    // (3) Instrument Banks //
    //////////////////////////

    // reposition and skip count
    if(!file.seek(offset_banks + 2))
        return FfmtErrCode::ERR_BADFORMAT;

    for(unsigned nth_inst = 0; nth_inst < inst_count; ++nth_inst)
    {
        FmBank::Instrument inst = FmBank::emptyInst();
        InstMappingInfo &info = mapInfo[nth_inst];

        uint32_t offset_inst = (uint32_t)file.pos();

        uint16_t inst_size;
        if(file.read(char_p(&inst_size), 2) != 2)
            return FfmtErrCode::ERR_BADFORMAT;
        inst_size = toUint16LE((uint8_t *)&inst_size);

        uint8_t idata[32];
        if(file.read(char_p(idata), 32) != 32)
            return FfmtErrCode::ERR_BADFORMAT;

        for(unsigned op_index = 0; op_index < 4; ++op_index)
        {
            const unsigned opnum[4] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};
            unsigned op = opnum[op_index];

            inst.setRegDUMUL(op, idata[0 + op_index]);
            inst.setRegLevel(op, idata[4 + op_index]);
            inst.setRegRSAt(op, idata[8 + op_index]);
            inst.setRegAMD1(op, idata[12 + op_index]);
            inst.setRegD2(op, idata[16 + op_index]);
            inst.setRegSysRel(op, idata[20 + op_index]);
            inst.setRegSsgEg(op, idata[24 + op_index]);
        }
        inst.setRegFbAlg(idata[28]);
        inst.setRegLfoSens(idata[29]);

        uint8_t transposeOrKey = idata[30];
        if(!info.isdrum)
            inst.note_offset1 = static_cast<int16_t>(-static_cast<int8_t>(transposeOrKey));
        else
            inst.percNoteNum = transposeOrKey & 127;

        uint8_t extraBits = idata[31];
        if(extraBits & 1)
        {
            int8_t chord_notes[255];
            uint8_t chord_note_count;

            if(file.read(char_p(&chord_note_count), 1) != 1 ||
               file.read(char_p(chord_notes), chord_note_count) != chord_note_count)
                return FfmtErrCode::ERR_BADFORMAT;

            /* ignored */
        }

        char name[256];
        uint8_t name_length;

        if(file.read(char_p(&name_length), 1) != 1 ||
           file.read(name, name_length) != name_length)
            return FfmtErrCode::ERR_BADFORMAT;

        name[name_length] = '\0';
        strncpy(inst.name, name, 32);

        // copy it into bank slots (can be multiple)
        std::list<FmBank::Instrument *> &target_list = mapInfo[nth_inst].targets;
        for(std::list<FmBank::Instrument *>::iterator it = target_list.begin(),
                end = target_list.end(); it != end; ++it)
        {
            FmBank::Instrument *target = *it;
            memcpy(target, &inst, sizeof(FmBank::Instrument));
        }

        // next
        if(!file.seek(offset_inst + inst_size))
            return FfmtErrCode::ERR_BADFORMAT;
    }

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode Basic_M2V_GYB::saveFileVersion1Or2(QFile &file, FmBank &bank, uint8_t version)
{
    FmBank::Instrument blank_ins = FmBank::emptyInst();
    blank_ins.is_blank = true;
    FmBank::Instrument blank_ins_list[128];
    std::fill(blank_ins_list, blank_ins_list + 128, blank_ins);

    // find GM 1:1 melodics and drums
    const FmBank::Instrument *melo_ins_list = nullptr;
    const FmBank::Instrument *drum_ins_list = nullptr;
    for(size_t i = 0, n = bank.Banks_Melodic.size(); !melo_ins_list && i < n; ++i)
    {
        const FmBank::MidiBank &b = bank.Banks_Melodic[i];
        if(b.msb == 0 && b.lsb == 0)
            melo_ins_list = &bank.Ins_Melodic[i * 128];
    }
    for(size_t i = 0, n = bank.Banks_Percussion.size(); !drum_ins_list && i < n; ++i)
    {
        const FmBank::MidiBank &b = bank.Banks_Percussion[i];
        if(b.msb == 0 && b.lsb == 0)
            drum_ins_list = &bank.Ins_Percussion[i * 128];
    }
    if(!melo_ins_list)
        melo_ins_list = blank_ins_list;
    if(!drum_ins_list)
        drum_ins_list = blank_ins_list;

    // collect the non-blank entries
    const FmBank::Instrument *melo_entry[128], *drum_entry[128];
    unsigned melo_entry_count = 0, drum_entry_count = 0;
    for(unsigned i = 0; i < 128; ++i)
    {
        if(!melo_ins_list[i].is_blank)
            melo_entry[melo_entry_count++] = &melo_ins_list[i];
        if(!drum_ins_list[i].is_blank)
            drum_entry[drum_entry_count++] = &drum_ins_list[i];
    }
    unsigned total_count = melo_entry_count + drum_entry_count;

    // write file header
    const uint8_t header[5] =
        { 0x1a, 0x0c, version,
          (uint8_t)melo_entry_count, (uint8_t)drum_entry_count };
    file.write(char_p(header), sizeof(header));

    // write GM map
    for(unsigned i = 0, jm = 0, jp = 0; i < 128; ++i)
    {
        uint8_t m = (uint8_t)(melo_ins_list[i].is_blank ? 0xff : jm++);
        file.write(char_p(&m), 1);
        uint8_t p = (uint8_t)(drum_ins_list[i].is_blank ? 0xff : jp++);
        file.write(char_p(&p), 1);
    }

    // write LFO register
    if(version == 2)
    {
        uint8_t regLfo = bank.getRegLFO();
        file.write(char_p(&regLfo), 1);
    }

    // write instruments
    for(unsigned i = 0; i < total_count; ++i)
    {
        uint8_t idata[32];
        uint8_t idatalen = (version == 2) ? 32 : 30;

        memset(&idata, 0, sizeof(idata));

        bool isdrum = i >= melo_entry_count;

        const FmBank::Instrument *ins = (!isdrum) ?
            melo_entry[i] : drum_entry[i - melo_entry_count];

        for(unsigned op_index = 0; op_index < 4; ++op_index)
        {
            const unsigned opnum[4] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};
            unsigned op = opnum[op_index];

            idata[0 + op_index] = ins->getRegDUMUL(op);
            idata[4 + op_index] = ins->getRegLevel(op);
            idata[8 + op_index] = ins->getRegRSAt(op);
            idata[12 + op_index] = ins->getRegAMD1(op);
            idata[16 + op_index] = ins->getRegD2(op);
            idata[20 + op_index] = ins->getRegSysRel(op);
            idata[24 + op_index] = ins->getRegSsgEg(op);
        }
        idata[28] = ins->getRegFbAlg();

        if(version == 2)
            idata[29] = ins->getRegLfoSens();

        uint8_t *transposeOrKey = &idata[(version == 2) ? 30 : 29];
        if(!isdrum)
            *transposeOrKey = static_cast<uint8_t>(static_cast<int8_t>(-ins->note_offset1));
        else
            *transposeOrKey = ins->percNoteNum;

        file.write(char_p(idata), idatalen);
    }

    // write names
    for(unsigned i = 0; i < total_count; ++i)
    {
        bool isdrum = i >= melo_entry_count;

        const char *name = (!isdrum) ?
            melo_entry[i]->name : drum_entry[i - melo_entry_count]->name;

        uint8_t size = (uint8_t)strlen(name);

        file.write(char_p(&size), 1);
        file.write(name, size);
    }

    // write fake checksum
    uint8_t cs[4] = {0, 0, 0, 0};
    file.write(char_p(cs), sizeof(cs));

    if(!file.flush())
        return FfmtErrCode::ERR_NOFILE;

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode Basic_M2V_GYB::saveFileVersion3(QFile &file, FmBank &bank)
{
    //TODO: implement me version 3
    
    return FfmtErrCode::ERR_NOT_IMLEMENTED;
}

/*
 * GYB file reader v1/v2/v3
 */

bool M2V_GYB_READ::detect(const QString &filePath, char *magic)
{
    //By name extension
    if(filePath.endsWith(".gyb", Qt::CaseInsensitive))
        return true;

    //By magic
    if(magic[0] == 26 && magic[1] == 12)
        return true;

    return false;
}

FfmtErrCode M2V_GYB_READ::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    uint8_t header[3];
    if(file.peek(char_p(header), 3) != 3)
        return FfmtErrCode::ERR_BADFORMAT;

    if(!(header[0] == 26 && header[1] == 12))
        return FfmtErrCode::ERR_BADFORMAT;

    uint8_t version = header[2];
    switch(version)
    {
    case 1:
    case 2:
        return loadFileVersion1Or2(file, bank, version);
    case 3:
        return loadFileVersion3(file, bank);
    default:
        return FfmtErrCode::ERR_BADFORMAT;
    }
}

int M2V_GYB_READ::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_OPEN|(int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString M2V_GYB_READ::formatName() const
{
    return "GYB bank";
}

BankFormats M2V_GYB_READ::formatId() const
{
    return BankFormats::FORMAT_M2V_GYB_READ;
}

/*
 * GYB file writer v1
 */

FfmtErrCode M2V_GYB_WRITEv1::saveFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    uint8_t version = 1;
    return saveFileVersion1Or2(file, bank, version);
}

int M2V_GYB_WRITEv1::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_SAVE;
}

QString M2V_GYB_WRITEv1::formatName() const
{
    return "GYB bank version 1";
}

BankFormats M2V_GYB_WRITEv1::formatId() const
{
    return BankFormats::FORMAT_M2V_GYB_WRITEv1;
}

/*
 * GYB file writer v2
 */

FfmtErrCode M2V_GYB_WRITEv2::saveFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    uint8_t version = 2;
    return saveFileVersion1Or2(file, bank, version);
}

int M2V_GYB_WRITEv2::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_SAVE;
}

QString M2V_GYB_WRITEv2::formatName() const
{
    return "GYB bank version 2";
}

BankFormats M2V_GYB_WRITEv2::formatId() const
{
    return BankFormats::FORMAT_M2V_GYB_WRITEv2;
}

/*
 * GYB file writer v3
 */

FfmtErrCode M2V_GYB_WRITEv3::saveFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    return saveFileVersion3(file, bank);
}

int M2V_GYB_WRITEv3::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_SAVE;
}

QString M2V_GYB_WRITEv3::formatName() const
{
    return "GYB bank version 3";
}

BankFormats M2V_GYB_WRITEv3::formatId() const
{
    return BankFormats::FORMAT_M2V_GYB_WRITEv3;
}
