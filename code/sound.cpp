float32 GetStep(uint32 samples_per_tick, float frequency)
{
    float32 step = 0;
    if (frequency > 0)
    {
        uint32 samples_per_second = samples_per_tick * 1000;
        float32 period = samples_per_second / frequency;
        step = frequency / samples_per_second;
    }
    return step;
}

void SinWave(float32* stream, uint64 length, uint32 samples_per_tick, float frequency, float attack, void* memory)
{
    SIN_STATE* state = (SIN_STATE*) memory;
    float32 step = GetStep(samples_per_tick, frequency);
    for (uint32 i=0; i<length; i++)
    {
        *stream += ((float32)sin(state->x * Tau32) * 0.5f);
        stream++;
        state->x += step;
        while (state->x > 1.0f)
        {
            state->x -= 1.0f;
        }
    }
}


void Triangle(float32* stream, uint64 length, uint32 samples_per_tick, float frequency, float attack, void* memory)
{
    TRIANGLE_STATE* state = (TRIANGLE_STATE*) memory;
    float32 period = GetStep(samples_per_tick, frequency);
    for (uint32 i=0; i<length; i++)
    {
        float32 result;
        {
            float32 x = state->x;
            float32 phase = x * 2.0f;
            if (x >= 0.5f)
            {
                phase -= 1.0f;
            }
            result = 0;
            float32 step = (float32)floor(phase * 15);
            float32 step_phase = (phase * 15) - step;
            // step_phase = (float32)(pow(step_phase * 2.0f - 1.0f, 3) + 1.0f) / 2.0f;
            // step_phase = (float32)sin(step_phase * Tau32) * 0.1f;
            step_phase = (float32)asin(pow(step_phase * 2.0f - 1.0f, 1)) / Tau32;
            step += step_phase;
            if (step > 15) {
                step = 30 - step;
            }
            step = max(step, 0.0f);
            // result = (float32)(pow(floor(phase * 15.0f) / 15.0f, 0.75f) * 2.0f) - 1.0f;
            // result = (float32)(pow(step / 15.0f, .9) * 2.0f) - 1.0f;
            // result = (float32)((step / 15.0f) * 2.0f) - 1.0f;
            result = ((float32)pow(phase, 0.8) * 2.0f) - 1.0f;
            // result = (float32)((floor(phase * 15) / 15.0f) * 2.0f) - 1.0f;
            if (x < 0.5f)
            {
                result = (result *-1);
                // result = result - ((float32)sin(step_phase * Tau32 / 2.0f) * 0.1f);
            }
            else
            {

            }
        }
        *stream += ((float32)(result) * 0.15f);
        stream++;
        state->x += period;
        while (state->x > 1.0)
        {
            state->x -= 1.0;
        }
    }
}

void Instrument(float32* stream, uint64 length, uint32 samples_per_tick, float frequency, float attack, void* memory)
{
    INSTRUMENT_STATE* state = (INSTRUMENT_STATE*) memory;
    Triangle(stream, length, samples_per_tick, frequency * 2.0f, attack * 0.1f, &(state->triangle));
}

void Play(float64 time, AUDIO_CURSOR* cursor, float frequency, float attack, INSTRUMENT* instrument)
{
    instrument->Play = Instrument;
    if (cursor->written >= cursor->end)
    {
        return;
    }
    cursor->position = cursor->position.Plus(time);
    if (cursor->position.LessThan(cursor->start))
    {
        return;
    }
    float64 diff = cursor->position.Minus(cursor->start).Time();
    float64 time_to_play = min(diff, time);
    uint64 length = min((uint64)(time_to_play * cursor->samples_per_beat), cursor->end - cursor->written);
    instrument->Play(&(cursor->stream)[cursor->written], length, cursor->samples_per_tick, frequency, attack, instrument->memory);
    cursor->written += length;
}

float32 AbsoluteNote(int note)
{
    return 440.0f * (float32)pow(2, (note - 69) / 12.0f);
}

int Minor(int note)
{
    switch (note)
    {
        case 0:
            return 0;
        case 1:
            return 2;
        case 2:
            return 3;
        case 3:
            return 5;
        case 4:
            return 7;
        case 5:
            return 8;
        case 6:
            return 10;
        default:
            return 0;
    }
}

float32 AMinor(int note, int accidental)
{
    int octave = note / 7;
    int mod = note % 7;
    if (mod < 0)
    {
        mod += 7;
    }
    int pitch = Minor(mod);
    pitch += octave * 12;
    pitch += accidental;
    pitch += 45;
    return AbsoluteNote(pitch);
}

float32 AMinor(int note)
{
    return AMinor(note, 0);
}

void GetSound(GAME_AUDIO* audio, GAME_STATE* state, uint32 ticks)
{
    AUDIO_CLOCK *clock = &(state->clock);
    AUDIO_CURSOR cursor = {};

    float32 samples_per_minute = (float32)(audio->samples_per_tick * 1000 * 60);
    float32 samples_per_beat = samples_per_minute / clock->bpm;
    float64 elapsed = (ticks * audio->samples_per_tick) / samples_per_beat;
    clock->time = clock->time.Plus(elapsed);
    audio->written -= elapsed;
    audio->written = max(audio->written, 0);

    cursor.start = clock->time.Plus(audio->written);
    cursor.end = audio->size;
    cursor.samples_per_beat = samples_per_beat;
    cursor.samples_per_tick = audio->samples_per_tick;
    cursor.stream = (float32*)audio->stream;

#if 0
    while(cursor.written < cursor.end)
    {
        // Play(1.0f, &cursor, 0, 0.5f, &(state->instrument));
        // Play(1.0f, &cursor, 117.188f, 0.5f, &(state->instrument));
        // Play(1.0f, &cursor, AMinaaor(0, 1), 0.5f, &(state->instrument));
        Play(1.0, &cursor, AMinor(4), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(1), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(2), 0.5f, &(state->instrument));
        Play(1.0, &cursor, AMinor(3), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(2), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(1), 0.5f, &(state->instrument));

        Play(1.0, &cursor, AMinor(0), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(0), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(2), 0.5f, &(state->instrument));
        Play(1.0, &cursor, AMinor(4), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(3), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(2), 0.5f, &(state->instrument));

        Play(1.0, &cursor, AMinor(1), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(1), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(2), 0.5f, &(state->instrument));
        Play(1.0, &cursor, AMinor(3), 0.5f, &(state->instrument));
        Play(1.0, &cursor, AMinor(4), 0.5f, &(state->instrument));

        Play(1.0, &cursor, AMinor(2), 0.5f, &(state->instrument));
        Play(1.0, &cursor, AMinor(0), 0.5f, &(state->instrument));
        Play(1.0, &cursor, AMinor(0), 0.5f, &(state->instrument));
        Play(1.0, &cursor,     0,     0.5f, &(state->instrument));

        Play(0.5, &cursor,     0,     0.5f, &(state->instrument));
        Play(1.0, &cursor, AMinor(3), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(5), 0.5f, &(state->instrument));
        Play(1.0, &cursor, AMinor(7), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(6), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(5), 0.5f, &(state->instrument));

        Play(1.5, &cursor, AMinor(4), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(2), 0.5f, &(state->instrument));
        Play(1.0, &cursor, AMinor(4), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(3), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(2), 0.5f, &(state->instrument));

        Play(1.0, &cursor, AMinor(1), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(1), 0.5f, &(state->instrument));
        Play(0.5, &cursor, AMinor(2), 0.5f, &(state->instrument));
        Play(1.0, &cursor, AMinor(3), 0.5f, &(state->instrument));
        Play(1.0, &cursor, AMinor(4), 0.5f, &(state->instrument));

        Play(1.0, &cursor, AMinor(2), 0.5f, &(state->instrument));
        Play(1.0, &cursor, AMinor(0), 0.5f, &(state->instrument));
        Play(1.0, &cursor, AMinor(0), 0.5f, &(state->instrument));
        Play(1.0, &cursor,     0,     0.5f, &(state->instrument));
    }
#endif
    audio->written += audio->size / samples_per_beat;
}
