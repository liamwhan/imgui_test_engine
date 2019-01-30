#pragma once

// Helpers: miscellaneous functions
ImGuiID ImHashDecoratedPath(const char* str, ImGuiID seed);
ImU64   ImGetTimeInMicroseconds();

void    ImPathFixSeparatorsForCurrentOS(char* buf);

void    ImParseSplitCommandLine(int* out_argc, char*** out_argv, const char* cmd_line);
void    ImParseDateFromCompilerIntoYMD(const char* in_data, char* out_buf, size_t out_buf_size);

const char* GetImGuiKeyName(ImGuiKey key);

// Helper: maintain/calculate moving average
template<typename TYPE>
struct ImMovingAverage
{
    ImVector<TYPE>  Samples;
    TYPE            Accum;
    int             Idx;
    int             FillAmount;

    ImMovingAverage()               { Accum = (TYPE)0; Idx = FillAmount = 0; }
    void    Init(int count)         { Samples.resize(count); memset(Samples.Data, 0, Samples.Size * sizeof(TYPE)); Accum = (TYPE)0; Idx = FillAmount = 0; }
    void    AddSample(TYPE v)       { Accum += v - Samples[Idx]; Samples[Idx] = v; if (++Idx == Samples.Size) Idx = 0; if (FillAmount < Samples.Size) FillAmount++;  }
    TYPE    GetAverage() const      { return Accum / (TYPE)FillAmount; }
    int     GetSampleCount() const  { return Samples.Size; }
    bool    IsFull() const          { return FillAmount == Samples.Size; }
};
