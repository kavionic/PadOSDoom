// This file is part of PadOSDoom.
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "DoomSession.h"
#include "DoomState.h"

namespace PadOSDoom
{

struct alignas(std::max_align_t) DoomSession::Allocation
{
    Allocation* Previous;
    Allocation* Next;
};

struct DoomSession::FileEntry
{
    FILE* Stream;
    FileEntry* Next;
};

DoomSession* DoomSession::s_Current = nullptr;

DoomSession::DoomSession()
{
    size_t snapshotSize = 0;
    for (size_t i = 0; i < DOOM_STATE_REGION_COUNT; ++i) {
        if (!DOOM_STATE_REGIONS[i].ZeroInitialized) {
            snapshotSize += DOOM_STATE_REGIONS[i].Size;
        }
    }
    m_InitialData = std::make_unique_for_overwrite<std::byte[]>(snapshotSize);
    size_t offset = 0;
    for (size_t i = 0; i < DOOM_STATE_REGION_COUNT; ++i)
    {
        const DoomStateRegion& region = DOOM_STATE_REGIONS[i];
        if (!region.ZeroInitialized)
        {
            std::memcpy(m_InitialData.get() + offset, region.Address, region.Size);
            offset += region.Size;
        }
    }
    s_Current = this;
}

DoomSession::~DoomSession()
{
    // The application's views must be detached and synchronized before this runs.
    while (m_OpenFiles != nullptr) {
        CloseFile(m_OpenFiles->Stream);
    }
    while (m_Allocations != nullptr) {
        Free(m_Allocations + 1);
    }
    RestoreEngineState();
    s_Current = nullptr;
}

DoomSession& DoomSession::GetCurrent()
{
    return *s_Current;
}

void* DoomSession::Allocate(size_t size)
{
    if (size > std::numeric_limits<size_t>::max() - sizeof(Allocation))
    {
        errno = ENOMEM;
        return nullptr;
    }
    Allocation* allocation = static_cast<Allocation*>(std::malloc(sizeof(Allocation) + size));
    if (allocation == nullptr) {
        return nullptr;
    }
    allocation->Previous = nullptr;
    allocation->Next = m_Allocations;
    if (m_Allocations != nullptr) {
        m_Allocations->Previous = allocation;
    }
    m_Allocations = allocation;
    return allocation + 1;
}

void* DoomSession::AllocateZeroed(size_t count, size_t size)
{
    if (count != 0 && size > std::numeric_limits<size_t>::max() / count)
    {
        errno = ENOMEM;
        return nullptr;
    }
    const size_t totalSize = count * size;
    void* memory = Allocate(totalSize);
    if (memory != nullptr) {
        std::memset(memory, 0, totalSize);
    }
    return memory;
}

void* DoomSession::Reallocate(void* memory, size_t size)
{
    if (memory == nullptr) {
        return Allocate(size);
    }
    if (size == 0)
    {
        Free(memory);
        return nullptr;
    }
    if (size > std::numeric_limits<size_t>::max() - sizeof(Allocation))
    {
        errno = ENOMEM;
        return nullptr;
    }
    Allocation* allocation = static_cast<Allocation*>(memory) - 1;
    Allocation* previous = allocation->Previous;
    Allocation* next = allocation->Next;
    Allocation* resized = static_cast<Allocation*>(std::realloc(allocation, sizeof(Allocation) + size));
    if (resized != nullptr)
    {
        if (previous != nullptr) {
            previous->Next = resized;
        } else {
            m_Allocations = resized;
        }
        if (next != nullptr) {
            next->Previous = resized;
        }
    }
    return (resized != nullptr) ? resized + 1 : nullptr;
}

void DoomSession::Free(void* memory)
{
    if (memory != nullptr)
    {
        Allocation* allocation = static_cast<Allocation*>(memory) - 1;
        if (allocation->Previous != nullptr) {
            allocation->Previous->Next = allocation->Next;
        } else {
            m_Allocations = allocation->Next;
        }
        if (allocation->Next != nullptr) {
            allocation->Next->Previous = allocation->Previous;
        }
        std::free(allocation);
    }
}

char* DoomSession::DuplicateString(const char* value)
{
    const size_t size = std::strlen(value) + 1;
    char* result = static_cast<char*>(Allocate(size));
    if (result != nullptr) {
        std::memcpy(result, value, size);
    }
    return result;
}

FILE* DoomSession::OpenFile(const char* path, const char* mode)
{
    FILE* stream = std::fopen(path, mode);
    if (stream == nullptr) {
        return nullptr;
    }
    FileEntry* entry = static_cast<FileEntry*>(std::malloc(sizeof(FileEntry)));
    if (entry == nullptr)
    {
        std::fclose(stream);
        errno = ENOMEM;
        return nullptr;
    }
    entry->Stream = stream;
    entry->Next = m_OpenFiles;
    m_OpenFiles = entry;
    return stream;
}

int DoomSession::CloseFile(FILE* stream)
{
    FileEntry** entry = &m_OpenFiles;
    while (*entry != nullptr && (*entry)->Stream != stream) {
        entry = &(*entry)->Next;
    }
    if (*entry != nullptr)
    {
        FileEntry* removed = *entry;
        *entry = removed->Next;
        std::free(removed);
    }
    return std::fclose(stream);
}

void DoomSession::RestoreEngineState()
{
    size_t offset = 0;
    for (size_t i = 0; i < DOOM_STATE_REGION_COUNT; ++i)
    {
        const DoomStateRegion& region = DOOM_STATE_REGIONS[i];
        if (region.ZeroInitialized)
        {
            std::memset(region.Address, 0, region.Size);
        }
        else
        {
            std::memcpy(region.Address, m_InitialData.get() + offset, region.Size);
            offset += region.Size;
        }
    }
}

}

void* PadOSDoom_Malloc(size_t size)
{
    return PadOSDoom::DoomSession::GetCurrent().Allocate(size);
}

void* PadOSDoom_Calloc(size_t count, size_t size)
{
    return PadOSDoom::DoomSession::GetCurrent().AllocateZeroed(count, size);
}

void* PadOSDoom_Realloc(void* memory, size_t size)
{
    return PadOSDoom::DoomSession::GetCurrent().Reallocate(memory, size);
}

void PadOSDoom_Free(void* memory)
{
    PadOSDoom::DoomSession::GetCurrent().Free(memory);
}

char* PadOSDoom_Strdup(const char* value)
{
    return PadOSDoom::DoomSession::GetCurrent().DuplicateString(value);
}

FILE* PadOSDoom_Fopen(const char* path, const char* mode)
{
    return PadOSDoom::DoomSession::GetCurrent().OpenFile(path, mode);
}

int PadOSDoom_Fclose(FILE* stream)
{
    return PadOSDoom::DoomSession::GetCurrent().CloseFile(stream);
}
