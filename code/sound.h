struct AUDIO_TIME
{
    int beats;
    float64 time;
    AUDIO_TIME Plus(AUDIO_TIME other)
    {
        AUDIO_TIME result = *this;
        result.beats += other.beats;
        result.time += other.time;
        while (result.time >= 1.0)
        {
            result.beats++;
            result.time -= 1.0;
        }
        return result;
    }
    AUDIO_TIME Plus(float64 add)
    {
        AUDIO_TIME result = *this;
        result.time += add;
        while (result.time >= 1.0)
        {
            result.beats++;
            result.time -= 1.0;
        }
        return result;
    }
    AUDIO_TIME Minus(AUDIO_TIME other)
    {
        AUDIO_TIME result = *this;
        result.beats -= other.beats;
        result.time -= other.time;
        while (result.time < 0) {
            result.beats--;
            result.time += 1.0;
        }
        return result;
    }
    AUDIO_TIME Minus(float64 minus)
    {
        AUDIO_TIME result = *this;
        result.time -= minus;
        while (result.time < 0) {
            result.beats--;
            result.time += 1.0;
        }
        return result;
    }
    bool32 LessThan(AUDIO_TIME other)
    {
        if (beats < other.beats)
        {
            return true;
        }
        if (beats == other.beats && time < other.time) {
            return true;
        }
        return false;
    }
    float64 Time()
    {
        return beats + time;
    }
};

struct AUDIO_CURSOR
{
    AUDIO_TIME start;
    AUDIO_TIME position;
    uint64 written;
    uint64 end;
    float32 samples_per_beat;
    uint32 samples_per_tick;
    float32* stream;

};

struct AUDIO_CLOCK
{
    AUDIO_TIME time;
    float32 bpm;
    int meter;
};

struct SIN_STATE
{
    float32 x;
};

// struct TRIANGLE_STATE
// {
//     float32 x;
// };

struct TriangleChannel
{
    float32 x;
    int num_harmonics;
};

// struct INSTRUMENT_STATE
// {
//     TRIANGLE_STATE triangle;
// };

// struct INSTRUMENT
// {
//     void* memory;
//     void (*Play)(
//         float32* stream, uint64 length,
//         uint32 samples_per_tick, float frequency, float attack,
//         void* memory
//     );
// };
