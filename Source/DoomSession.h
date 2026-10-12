// This file is part of PadOSDoom.
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <cstdio>
#include <memory>

namespace PadOSDoom
{

class DoomSession
{
public:
    DoomSession();
    ~DoomSession();
    DoomSession(const DoomSession&) = delete;
    DoomSession& operator=(const DoomSession&) = delete;

    static DoomSession& GetCurrent();

    void* Allocate(size_t size);
    void* AllocateZeroed(size_t count, size_t size);
    void* Reallocate(void* memory, size_t size);
    void Free(void* memory);
    char* DuplicateString(const char* value);
    FILE* OpenFile(const char* path, const char* mode);
    int CloseFile(FILE* stream);

private:
    struct Allocation;
    struct FileEntry;

    void RestoreEngineState();

    std::unique_ptr<std::byte[]> m_InitialData;
    Allocation* m_Allocations = nullptr;
    FileEntry* m_OpenFiles = nullptr;

    static DoomSession* s_Current;
};

}

extern "C"
{
void* PadOSDoom_Malloc(size_t size);
void* PadOSDoom_Calloc(size_t count, size_t size);
void* PadOSDoom_Realloc(void* memory, size_t size);
void PadOSDoom_Free(void* memory);
char* PadOSDoom_Strdup(const char* value);
FILE* PadOSDoom_Fopen(const char* path, const char* mode);
int PadOSDoom_Fclose(FILE* stream);
}
