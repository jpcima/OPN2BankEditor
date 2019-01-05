/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2019 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef BANK_EDITOR_H
#define BANK_EDITOR_H

#include <QTimer>
#include <QMainWindow>
#include <QList>
#include <QListWidgetItem>
#include "bank.h"
#include "opl/generator.h"
#include "opl/generator_realtime.h"
#include "opl/measurer.h"
#include "audio/ao_rtaudio.h"
#include "midi/midi_rtmidi.h"

#include "FileFormats/ffmt_base.h"

#ifdef NO_NATIVE_OPEN_DIALOGS
#define FILE_OPEN_DIALOG_OPTIONS QFileDialog::DontUseNativeDialog
#else
#define FILE_OPEN_DIALOG_OPTIONS 0
#endif

namespace Ui
{
    class BankEditor;
}

class Importer;

/**
 * @brief Main application window
 */
class BankEditor : public QMainWindow
{
    Q_OBJECT

    friend class Importer;
private:

    Importer           *m_importer;
    //! Path for currently opened file
    QString             m_recentPath;
    //! Recently opened bank file
    QString             m_recentBankFilePath;
    //! Choosen UI language
    QString             m_language;
    //! Currently using chip
    Generator::OPN_Chips m_currentChip;
    //! Current chip family
    OPNFamily           m_currentChipFamily = OPNChip_OPN2;
    //! Audio latency (ms)
    double m_audioLatency;

public:
    //! Audio latency constants (ms)
    enum
    {
        audioMinimumLatency = 1,
        audioDefaultLatency = 20,
        audioMaximumLatency = 100,
    };

private:
    //! Currently loaded FM bank
    FmBank              m_bank;
    //! Backup of currently loaded FM bank
    FmBank              m_bankBackup;

    //! Backup for melodic note while percusive mode is enabled
    int                 m_recentMelodicNote;

    //! Currently selected instrument
    FmBank::Instrument *m_curInst;
    //! Currently selected instrument
    FmBank::Instrument *m_curInstBackup;

    //! Recent index of instrument
    int m_recentNum;
    //! Recent flag of percussion
    bool m_recentPerc;

    //! Ignore all controls change events
    bool m_lock;

    //! OPL chip emulator frontent
    IRealtimeControl *m_generator;

    //! Sound length measurer
    Measurer        *m_measurer;

    //! Recent bank file format which was been used
    BankFormats     m_recentFormat;

    //! Recent instrument file format which was been used
    InstFormats     m_recentInstFormat;

    /* ********** Audio output stuff ********** */
    AudioOutRt    *m_audioOut = nullptr;

    /* ********** MIDI input stuff ********** */
    #ifdef ENABLE_MIDI
    MidiInRt        *m_midiIn = nullptr;
    QAction         *m_midiInAction = nullptr;
    #endif

    /*!
     * \brief Initializes audio subsystem and FM generator
     */
    void initAudio();

    #ifdef ENABLE_MIDI
    /*!
     * \brief Updates the available choices of MIDI inputs
     */
    void updateMidiInMenu();
    #endif

public:
    explicit BankEditor(QWidget *parent = 0);
    ~BankEditor();

    /**
     * @brief Loads recent application settings
     */
    void loadSettings();

    /**
     * @brief Saves recent application settings to the registry
     */
    void saveSettings();

private:
    /*!
     * \brief Initializes file data after file loading
     * \param filePath Path to just opened file
     */
    void initFileData(QString &filePath);

    /*!
     * \brief Reinitializes some data after file saving
     * \param filePath Path to just saved file
     */
    void reInitFileDataAfterSave(QString &filePath);

public:
    /*!
     * \brief Open file
     * \param filePath absolute path to the file to open
     * \return true if file successfully opened, false if failed
     */
    bool openFile(QString filePath);

    /*!
     * \brief Save bank file
     * \param filePath absolute path where save a file
     * \param format Target format to save a file
     * \return true if file successfully saved, false if failed
     */
    bool saveBankFile(QString filePath, BankFormats format);
    /*!
     * \brief Save current instrument file
     * \param filePath absolute path where save a file
     * \param format Target format to save a file
     * \return true if file successfully saved, false if failed
     */
    bool saveInstrumentFile(QString filePath, InstFormats format);
    /*!
     * \brief Open Save-As dialog box
     * \return true if file successfuly saved, false on rejecting or on fail
     */
    bool saveFileAs(const QString &optionalFilePath = QString());
    /*!
     * \brief Open Save-As dialog box for single instrument
     * \return true if file successfuly saved, false on rejecting or on fail
     */
    bool saveInstFileAs();

    /*!
     * \brief Checks was file modified or not. If file modified, asks saving.
     * \return true if no modifications detected or file saved or rejected. false if operation has been cancel
     */
    bool askForSaving();

    /* ************** Helpful functions ************** */
    /**
     * @brief Loads current instrument into GUI controlls and sends it to generator
     */
    void flushInstrument();
    /**
     * @brief Synchronize instrument name in the list widget with actual instrument name in the data store
     */
    void syncInstrumentName();
    /**
     * @brief Synchronize instrument blank state in the list widget with actual instrument name in the data store
     */
    void syncInstrumentBlankness();
    /**
     * @brief Sets current instrument to editand test
     * @param num Number of instrument (from 0 to 127)
     * @param isPerc Use percusive set
     */
    void setCurrentInstrument(int num, bool isPerc);

    /**
     * @brief Set parameters of currently selected instrument into the GUI controlls
     */
    void loadInstrument();

    /**
     * @brief Send current instrument to OPL chip emulator for to test
     */
    void sendPatch();

    /**
     * @brief Disable/Enable melodic specific GUI controlls which are useless while editing of percussion instrument
     * @param dmode if true, most of melodic specific controlls (such as piano, note selector and chords) are will be disabled
     */
    void setDrumMode(bool dmode);

    bool isDrumsMode();

    void reloadBanks();

    /**
     * @brief Creates the list of available languages
     */
    void createLanguageChoices();

private:
    /**
     * Path of the file which is currently edited
     */
    QString m_currentFilePath;
    /**
     * Format of the file which is currently edited
     */
    BankFormats m_currentFileFormat;

public slots:
    /**
     * @brief Toggle melodic mode and fill instruments list with melodic instruments names
     */
    void setMelodic();

    /**
     * @brief Toggle percussion mode and fill instruments list with percussion insruments names
     */
    void setDrums();

    /**
     * @brief Reload names lf instruments in the list
     */
    void reloadInstrumentNames();

private slots:
    /* ***************** Common slots ***************** */
    /**
     * @brief When instrument list entry is selected
     * @param current Selected item from the instruments list, if NULL, turn into "unselected" mode and lock GUI
     * @param previous Unused, just pass NULL
     */
    void on_instruments_currentItemChanged(QListWidgetItem *current, QListWidgetItem *);

    /**
     * @brief Toggle the chip emulator
     */
    void toggleEmulator();

    /**
     * @brief Clear all buffers and begin a new bank
     */
    void on_actionNew_triggered();
    /**
     * @brief Open existing bank file
     */
    void on_actionOpen_triggered();
    /**
     * @brief Save current bank state into the file
     */
    void on_actionSave_triggered();
    /**
     * @brief Save current bank state into the file
     */
    void on_actionSaveAs_triggered();
    /**
     * @brief Save current instrument into the file
     */
    void on_actionSaveInstrument_triggered();
    /**
     * @brief Exit from the program
     */
    void on_actionExit_triggered();
    /**
     * @brief Copy current instrument into the clipboard
     */
    void on_actionCopy_triggered();
    /**
     * @brief Paste contents of clipboard into the current instrument
     */
    void on_actionPaste_triggered();
    /**
     * @brief Reset current instrument state to initial
     */
    void on_actionReset_current_instrument_triggered();

    /**
     * @brief Re-calculate all sounding delays
     */
    void on_actionReMeasure_triggered();

    /**
     * @brief Re-calculate sounding delays of the current instrument
     */
    void on_actionReMeasureOne_triggered();

    /**
     * @brief Run benchmark of all supported emulators basing on current instrument
     */
    void on_actionChipsBenchmark_triggered();

#if defined(ENABLE_PLOTS)
    /**
     * @brief Run the delay analysis of the current instrument
     */
    void on_actionDelayAnalysis_triggered();
#endif

    /**
     * @brief Run the register editor
     */
    void on_actionEditRegisters_triggered();

    /**
     * @brief Show all supported formats list
     */
    void on_actionFormatsSup_triggered();

    /**
     * @brief Show about dialog
     */
    void on_actionAbout_triggered();

    /**
     * @brief Rename current bank
     */
    void on_bankRename_clicked();

    /**
     * @brief Reload instruments list for specific bank ID
     * @param index ID of bank
     */
    void on_bank_no_currentIndexChanged(int index);

    /**
     * @brief Read value from MSB field and write into bank meta-data entry
     */
    void on_bank_msb_editingFinished();

    /**
     * @brief Read value from LSB field and write into bank meta-data entry
     */
    void on_bank_lsb_editingFinished();

    /**
     * @brief Add new instrument into end
     */
    void on_actionAddInst_triggered();
    /**
     * @brief Clear currently selected instrument entry
     */
    void on_actionClearInstrument_triggered();
    /**
     * @brief Delete currently selected instrument entry
     */
    void on_actionDelInst_triggered();

    /**
     * @brief Add new bank entry (add 128 instruments into end)
     */
    void on_actionAddBank_triggered();
    /**
     * @brief Add new bank into end and copy all instruments from current bank into it
     */
    void on_actionCloneBank_triggered();
    /**
     * @brief Clear all instruments in current bank
     */
    void on_actionClearBank_triggered();
    /**
     * @brief Delete current bank (delete 128 instruments) except of last bank
     */
    void on_actionDeleteBank_triggered();
    /**
     * @brief Turn on the AdLib BNK mode where all instruments shown as united list without dividing by banks
     * @param checked AdLib BNK mode is turned on
     */
    void on_actionAdLibBnkMode_triggered(bool checked);
    /**
     * @brief Opens the latency setting dialog
     */
    void on_actionLatency_triggered();
    /**
     * @brief Changes the current language
     */
    void onActionLanguageTriggered();


    /* ***************** Instrument Parameters editing ***************** */

    void on_insName_textChanged(const QString &arg1);
    void on_insName_editingFinished();

    void on_feedback1_valueChanged(int arg1);
    void on_perc_noteNum_valueChanged(int arg1);

    void on_algorithm_currentIndexChanged(int index);
    void on_amsens_currentIndexChanged(int index);
    void on_fmsens_currentIndexChanged(int index);

    void on_noteOffset1_valueChanged(int arg1);

    void on_chipType_currentIndexChanged(int index);
    void on_lfoEnable_clicked(bool checked);
    void on_lfoFrequency_currentIndexChanged(int index);

    void on_op1_attack_valueChanged(int arg1);
    void on_op1_decay1_valueChanged(int arg1);
    void on_op1_decay2_valueChanged(int arg1);
    void on_op1_sustain_valueChanged(int arg1);
    void on_op1_release_valueChanged(int arg1);
    void on_op1_am_toggled(bool checked);
    void on_op1_level_valueChanged(int arg1);
    void on_op1_freqmult_valueChanged(int arg1);
    void on_op1_detune_currentIndexChanged(int index);
    void on_op1_ratescale_valueChanged(int arg1);
    void on_op1_ssgeg_currentIndexChanged(int index);

    void on_op2_attack_valueChanged(int arg1);
    void on_op2_decay1_valueChanged(int arg1);
    void on_op2_decay2_valueChanged(int arg1);
    void on_op2_sustain_valueChanged(int arg1);
    void on_op2_release_valueChanged(int arg1);
    void on_op2_am_toggled(bool checked);
    void on_op2_level_valueChanged(int arg1);
    void on_op2_freqmult_valueChanged(int arg1);
    void on_op2_detune_currentIndexChanged(int index);
    void on_op2_ratescale_valueChanged(int arg1);
    void on_op2_ssgeg_currentIndexChanged(int index);

    void on_op3_attack_valueChanged(int arg1);
    void on_op3_decay1_valueChanged(int arg1);
    void on_op3_decay2_valueChanged(int arg1);
    void on_op3_sustain_valueChanged(int arg1);
    void on_op3_release_valueChanged(int arg1);
    void on_op3_am_toggled(bool checked);
    void on_op3_level_valueChanged(int arg1);
    void on_op3_freqmult_valueChanged(int arg1);
    void on_op3_detune_currentIndexChanged(int index);
    void on_op3_ratescale_valueChanged(int arg1);
    void on_op3_ssgeg_currentIndexChanged(int index);

    void on_op4_attack_valueChanged(int arg1);
    void on_op4_decay1_valueChanged(int arg1);
    void on_op4_decay2_valueChanged(int arg1);
    void on_op4_sustain_valueChanged(int arg1);
    void on_op4_release_valueChanged(int arg1);
    void on_op4_am_toggled(bool checked);
    void on_op4_level_valueChanged(int arg1);
    void on_op4_freqmult_valueChanged(int arg1);
    void on_op4_detune_currentIndexChanged(int index);
    void on_op4_ratescale_valueChanged(int arg1);
    void on_op4_ssgeg_currentIndexChanged(int index);

    void on_pitchBendSlider_valueChanged(int value);
    void on_pitchBendSlider_sliderReleased();

    void on_holdButton_toggled(bool checked);

    /**
     * @brief Update after a change of any control value
     */
    void afterChangeControlValue();

    /**
     * @brief Adjusts the size of the window after it has been shown
     */
    void onBankEditorShown();

    #ifdef ENABLE_MIDI
    void on_midiIn_triggered(QAction *);
    void onMidiPortTriggered();
    #endif

private:
    /**
     * @brief Updates the text to display after a language change
     */
    void onLanguageChanged();

protected:
    virtual void pianoKeyPress(QKeyEvent *event);
    virtual void pianoKeyRelease(QKeyEvent *event);

private:
    /**
     * @brief Set of key codes currently active on piano
     */
    QSet<int> pianoKeyCodes;

protected:
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    void showEvent(QShowEvent *event);
    void changeEvent(QEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);

private:
    Ui::BankEditor *ui;
};

#endif // BANK_EDITOR_H
