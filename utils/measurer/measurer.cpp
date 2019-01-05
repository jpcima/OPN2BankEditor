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

#include <FileFormats/format_wohlstand_opn2.h>
#include <opl/measurer.h>
#include <QApplication>
#include <atomic>
#include <memory>
#include <cstring>

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        fprintf(stderr, "%s <wopn-file-input> <wopn-file-output>\n", argv[0]);
        return 1;
    }

    QApplication app(argc, argv);

    WohlstandOPN2 format;
    QString wopnFileInput = argv[1];

    FmBank bank;
    FfmtErrCode errLoad = format.loadFile(wopnFileInput, bank);

    if(errLoad != FfmtErrCode::ERR_OK)
    {
        fprintf(stderr, "Could not load the WOPN file.\n");
        return 1;
    }

    Measurer measurer;
    FmBank bankBackup = bank;

    if(!measurer.doMeasurement(bank, bankBackup, true))
    {
        fprintf(stderr, "Measurement was interrupted.\n");
        return 1;
    }

    QString wopnFileOutput = argv[2];

    FfmtErrCode errSave = format.saveFile(wopnFileOutput, bank);
    if(errSave != FfmtErrCode::ERR_OK)
    {
        fprintf(stderr, "Could not save the WOPN file.\n");
        return 1;
    }

    return 0;
}
