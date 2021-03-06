/*
 * libADLMIDI is a free MIDI to WAV conversion library with OPL3 emulation
 *
 * Original ADLMIDI code: Copyright (c) 2010-2014 Joel Yliluoma <bisqwit@iki.fi>
 * ADLMIDI Library API:   Copyright (c) 2015-2018 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * Library is based on the ADLMIDI, a MIDI player for Linux and Windows with OPL3 emulation:
 * http://iki.fi/bisqwit/source/adlmidi.html
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

#include "adlmidi_private.hpp"

std::string ADLMIDI_ErrorString;

// Generator callback on audio rate ticks

void adl_audioTickHandler(void *instance, uint32_t chipId, uint32_t rate)
{
    reinterpret_cast<MIDIplay *>(instance)->AudioTick(chipId, rate);
}

int adlRefreshNumCards(ADL_MIDIPlayer *device)
{
    unsigned n_fourop[2] = {0, 0}, n_total[2] = {0, 0};
    MIDIplay *play = reinterpret_cast<MIDIplay *>(device->adl_midiPlayer);

    //Automatically calculate how much 4-operator channels is necessary
    if(play->opl.AdlBank == ~0u)
    {
        //For custom bank
        OPL3::BankMap::iterator it = play->opl.dynamic_banks.begin();
        OPL3::BankMap::iterator end = play->opl.dynamic_banks.end();
        for(; it != end; ++it)
        {
            uint16_t bank = it->first;
            unsigned div = (bank & OPL3::PercussionTag) ? 1 : 0;
            for(unsigned i = 0; i < 128; ++i)
            {
                adlinsdata2 &ins = it->second.ins[i];
                if(ins.flags & adlinsdata::Flag_NoSound)
                    continue;
                if((ins.adl[0] != ins.adl[1]) && ((ins.flags & adlinsdata::Flag_Pseudo4op) == 0))
                    ++n_fourop[div];
                ++n_total[div];
            }
        }
    }
    else
    {
        //For embedded bank
        for(unsigned a = 0; a < 256; ++a)
        {
            unsigned insno = banks[play->m_setup.AdlBank][a];
            if(insno == 198)
                continue;
            ++n_total[a / 128];
            adlinsdata2 ins(adlins[insno]);
            if(ins.flags & adlinsdata::Flag_Real4op)
                ++n_fourop[a / 128];
        }
    }

    unsigned numFourOps = 0;

    // All 2ops (no 4ops)
    if((n_fourop[0] == 0) && (n_fourop[1] == 0))
        numFourOps = 0;
    // All 2op melodics and Some (or All) 4op drums
    else if((n_fourop[0] == 0) && (n_fourop[1] > 0))
        numFourOps = 2;
    // Many 4op melodics
    else if((n_fourop[0] >= (n_total[0] * 7) / 8))
        numFourOps = 6;
    // Few 4op melodics
    else if(n_fourop[0] > 0)
        numFourOps = 4;

/* //Old formula
    unsigned NumFourOps = ((n_fourop[0] == 0) && (n_fourop[1] == 0)) ? 0
        : (n_fourop[0] >= (n_total[0] * 7) / 8) ? play->m_setup.NumCards * 6
        : (play->m_setup.NumCards == 1 ? 1 : play->m_setup.NumCards * 4);
*/

    play->opl.NumFourOps = play->m_setup.NumFourOps = (numFourOps * play->m_setup.NumCards);

    return 0;
}
