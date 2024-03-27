// IMPORTANT: must be included first or else the definitions will be exported
#define OPENGL_DEFINE_FUNCTIONS
#include "opengl.h"

#include "thermal.h"
#include "string2.h"
#include "memory.h"
#include "platform.h"
#include "utf.h"
#include "font.h"
#include "io.h"

#define UNICODE
#define _UNICODE

#undef OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "windowsx.h"
#include "shlobj_core.h"
#include "shlwapi.h"
#include "shellapi.h"
#include "objbase.h"
#include "dbghelp.h"


#ifdef OS_WINDOWS
#include "win32_definitions.h"

#endif



#define ALLOCATE_STATIC_ARRAY(arena, type, count) {ALLOCATE((arena), type, (count)), (count)}

INTERNAL char NarrowBuffer[2048];
INTERNAL MemoryBuffer allocate_memory_buffer(Allocator alloc, s64 size) {
    MemoryBuffer buffer = {};
    buffer.memory = (u8*)allocate(alloc, size);
    buffer.used   = 0;
    buffer.alloc  = size;

    return buffer;
}

INTERNAL void free_memory_buffer(Allocator alloc, MemoryBuffer *buffer) {
    deallocate(alloc, buffer->memory, buffer->alloc);

    INIT_STRUCT(buffer);
}


INTERNAL HANDLE ProcessHandle;
INTERNAL r64 FrequencyInSeconds;

PlatformConsole Console;

#ifndef PLATFORM_CONSOLE_BUFFER_SIZE
#define PLATFORM_CONSOLE_BUFFER_SIZE 4096
#endif


INTERNAL String get_stack_trace(Allocator alloc);

#ifdef ALLOCATION_OVERSEER
// NOTE: this is needed because else the allocation for the overseer would recurse indefinitely
void *basic_alloc(void *, s64 size, void *old, s64 old_size) {
    void *result = 0;

    if (old) {
        if (size) {
            result = rprealloc(old, size);
        } else {
            rpfree(old);
        }
    } else {
        result = rpcalloc(1, size);
    }

    return result;
}

Allocator BasicAllocator = {basic_alloc, 0};

u64 overseer_hash(void *ptr) {
    u64 v = (u64)ptr;
    v ^= v >> 33;
    v *= 0xff51afd7ed558ccdL;
    v ^= v >> 33;
    v *= 0xc4ceb9fe1a85ec53L;
    v ^= v >> 33;

    return v;
}

struct AllocationInfo {
    void *address;
    String trace;
};

// TODO: move this out of the platform layer
struct AllocationOverseer {
    HashTable<void*, AllocationInfo, u64, overseer_hash> allocations;
};

AllocationOverseer Overseer = {
    {BasicAllocator}, // NOTE: set hash table allocator
};


INTERNAL void report_leak() {
    for (u32 i = 0; i < Overseer.allocations.alloc; i += 1) {
        auto *entry = &Overseer.allocations.entries[i];

        if (entry->hash != 0) {
            print("Leak detected: %p\n\n", entry->value.address);
            print("Trace: \n%S\n", entry->value.trace);
        }
    }
}

#endif ALLOCATION_OVERSEER

#ifdef USE_RPMALLOC
#include "rpmalloc/rpmalloc.h"

void *rp_malloc_alloc_func(void *, s64 size, void *old, s64 old_size) {
    void *result = 0;

#ifdef ALLOCATION_OVERSEER

    if (old) {
        if (size) {
            result = rprealloc(old, size);
            AllocationInfo *found = find(&Overseer.allocations, old);
            if (!found) {
                die("Allocation never happened.");
            } else {
                String trace = found->trace;
                remove(&Overseer.allocations, old);

                AllocationInfo info;
                info.address = result;
                info.trace   = trace;

                insert(&Overseer.allocations, result, info);
            }
        } else {
            rpfree(old);

            AllocationInfo *found = find(&Overseer.allocations, old);
            if (!found) {

                for (u32 i = 0; i < Overseer.allocations.buckets; i += 1) {
                    if (Overseer.allocations.entries[i].key == old) {
                        DebugBreak();
                    }
                }

                die("Allocations never happened.");
            } else {
                destroy_string(&found->trace, BasicAllocator);
                remove(&Overseer.allocations, old);
            }
        }
    } else {
        result = rpcalloc(1, size);

        AllocationInfo info;
        info.address = result;
        info.trace   = get_stack_trace(BasicAllocator);

        insert(&Overseer.allocations, result, info);
    }

#else

    if (old) {
        if (size) {
            result = rprealloc(old, size);
        } else {
            rpfree(old);
        }
    } else {
        result = rpcalloc(1, size);
    }

#endif
    return result;
}


Allocator RpmallocAllocator = {rp_malloc_alloc_func, 0};
#else
INTERNAL void *cstd_alloc_func(void *, s64 size, void *old, s64 old_size) {
    void *result = 0;

#ifdef ALLOCATION_OVERSEER

    if (old) {
        if (size) {
            result = realloc(old, size);
            AllocationInfo *found = find(&Overseer.allocations, old);
            if (!found) {
                die("Allocation never happened.");
            } else {
                String trace = found->trace;
                remove(&Overseer.allocations, old);

                AllocationInfo info;
                info.address = result;
                info.trace   = trace;

                insert(&Overseer.allocations, result, info);
            }
        } else {
            free(old);

            AllocationInfo *found = find(&Overseer.allocations, old);
            if (!found) {

                for (u32 i = 0; i < Overseer.allocations.buckets; i += 1) {
                    if (Overseer.allocations.entries[i].key == old) {
                        DebugBreak();
                    }
                }

                die("Allocations never happened.");
            } else {
                destroy_string(&found->trace, BasicAllocator);
                remove(&Overseer.allocations, old);
            }
        }
    } else {
        result = calloc(1, size);

        AllocationInfo info;
        info.address = result;
        info.trace   = get_stack_trace(BasicAllocator);

        insert(&Overseer.allocations, result, info);
    }

#else

    if (old) {
        if (size) {
            result = realloc(old, size);
        } else {
            free(old);
        }
    } else {
        result = calloc(1, size);
    }

#endif // ALLOCATION_OVERSEER

    return result;
}

Allocator CStdAllocator = {cstd_alloc_func, 0};
#endif // USE_RPMALLOC

// TODO: Thread safety... make TLS?
Allocator DefaultAllocator;
MemoryArena TemporaryStorage;

Allocator default_allocator() {
    return DefaultAllocator;
}

Allocator temporary_allocator() {
    return make_arena_allocator(&TemporaryStorage);
}

s64 temporary_storage_mark() {
    return TemporaryStorage.used;
}

void temporary_storage_rewind(s64 mark) {
    TemporaryStorage.used = mark;
}

void reset_temporary_storage() {
    TemporaryStorage.used = 0;
}

int main_main() {
#ifdef ALLOCATION_OVERSEER
    // IMPORTANT: Leak reporting must be the first DEFER. Else all other deferred allocations will be falsely reported
    DEFER(report_leak());
#endif

    ProcessHandle = GetCurrentProcess();

    CoInitialize(0);

    // TODO: Remove on release build? But it is still needed for stack traces
    if (!SymInitialize(ProcessHandle, 0, TRUE)) {
        die("Could not initialize DebugSymbols.");
    }

#ifdef USE_RPMALLOC
    rpmalloc_initialize();
    DEFER(rpmalloc_finalize(););

    DefaultAllocator = RpmallocAllocator;
#else
    DefaultAllocator = CStdAllocator;
#endif // USE_RPMALLOC

    TemporaryStorage = allocate_arena(KILOBYTES(32));

    LARGE_INTEGER PerformanceFrequenzy;
    QueryPerformanceFrequency(&PerformanceFrequenzy);
    FrequencyInSeconds = 1.0 / PerformanceFrequenzy.QuadPart;

    PlatformFile standard_out_handle = {};
    standard_out_handle.handle = GetStdHandle(STD_OUTPUT_HANDLE);
    standard_out_handle.write_buffer = allocate_memory_buffer(DefaultAllocator, PLATFORM_CONSOLE_BUFFER_SIZE);
    standard_out_handle.open   = true;

    Console.out = &standard_out_handle;

    wchar_t *cmd_line = GetCommandLineW();

    int cmd_arg_count;
    wchar_t **cmd_args = CommandLineToArgvW(cmd_line, &cmd_arg_count);
    DEFER(LocalFree(cmd_args));

    Array<String> args = ALLOCATE_ARRAY(String, cmd_arg_count);
    DEFER(destroy_array(&args));

    for (int i = 0; i < cmd_arg_count; i += 1) {
        String16 arg = {(u16*)cmd_args[i], (s64)wcslen(cmd_args[i])};
        args[i] = to_utf8(default_allocator(), arg);
    }

    s32 status = application_main(args);

    for (int i = 0; i < cmd_arg_count; i += 1) {
        destroy_string(&args[i]);
    }

    flush_write_buffer(Console.out);

    destroy(&TemporaryStorage);
    free_memory_buffer(DefaultAllocator, &Console.out->write_buffer);

    CoUninitialize();
    SymCleanup(ProcessHandle);

    return status;
}

int APIENTRY WinMain(HINSTANCE, HINSTANCE, PSTR, int) {
    return main_main();
}

int main() {
    return main_main();
}

INTERNAL wchar_t WidenBuffer[2048];
INTERNAL wchar_t *widen(String str) {
    int chars = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)str.data, str.size, WidenBuffer, 2048);
    WidenBuffer[chars] = L'\0';

    return WidenBuffer;
}

INTERNAL void show_message_box_and_crash(String message) {
    MessageBoxW(0, widen(message), L"Fatal Error", MB_OK | MB_ICONERROR);

    DebugBreak();
    ExitProcess(-1);
}

INTERNAL void convert_backslash_to_slash(wchar_t *buffer, s64 size) {
    for (s64 i = 0; i < size; i += 1) {
        if (buffer[i] == L'\\') buffer[i] = L'/';
    }
}
INTERNAL void convert_slash_to_backslash(wchar_t *buffer, s64 size) {
    for (s64 i = 0; i < size; i += 1) {
        if (buffer[i] == L'/') buffer[i] = L'\\';
    }
}


PlatformFile platform_create_file_handle(String filename, u32 mode) {
    DWORD win32_mode = 0;
    DWORD win32_open_mode = 0;

    switch (mode) {
    case PLATFORM_FILE_READ: {
        win32_mode = GENERIC_READ;
        win32_open_mode = OPEN_ALWAYS;
    } break;

    case PLATFORM_FILE_WRITE: {
        win32_mode = GENERIC_WRITE;
        win32_open_mode = CREATE_ALWAYS;
    } break;

    case PLATFORM_FILE_APPEND: {
        win32_mode = GENERIC_READ | GENERIC_WRITE;
        win32_open_mode = OPEN_ALWAYS;
    } break;

    default:
        die("Unknown file mode.");
    }

    PlatformFile result = {};

    String16 wide_filename = to_utf16(temporary_allocator(), filename, true);
    DEFER(deallocate(temporary_allocator(), wide_filename.data, wide_filename.size * sizeof(u16)));

    convert_slash_to_backslash((wchar_t*)wide_filename.data, wide_filename.size);

    HANDLE handle = CreateFileW((wchar_t*)wide_filename.data, win32_mode, 0, 0, win32_open_mode, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle == INVALID_HANDLE_VALUE) {
        return result;
    }

    if (mode == PLATFORM_FILE_APPEND) {
        SetFilePointer(handle, 0, 0, FILE_END);
    }

    result.handle = handle;
    result.open   = true;

    return result;
}

void platform_close_file_handle(PlatformFile *file) {
    if (file->handle != INVALID_HANDLE_VALUE && file->handle != 0) {
        CloseHandle(file->handle);
    }

    INIT_STRUCT(file);
}

s64 platform_file_size(PlatformFile *file) {
    LARGE_INTEGER size;
    GetFileSizeEx(file->handle, &size);

    return size.QuadPart;
}

String platform_read(PlatformFile *file, u64 offset, void *buffer, s64 size) {
    // TODO: split reads if they are bigger than 32bit
    assert(size <= INT_MAX);

    if (!file->open) return {};

    OVERLAPPED ov = {0};
    ov.Offset     = offset & 0xFFFFFFFF;
    ov.OffsetHigh = (offset >> 32) & 0xFFFFFFFF;

    ReadFile(file->handle, buffer, size, 0, &ov);

    DWORD bytes_read = 0;
    GetOverlappedResult(file->handle, &ov, &bytes_read, TRUE);

    return {(u8*)buffer, bytes_read};
}

s64 platform_write(PlatformFile *file, u64 offset, void const *buffer, s64 size) {
    // TODO: split writes if they are bigger than 32bit
    assert(size <= INT_MAX);

    if (!file->open) return 0;

    OVERLAPPED ov = {0};
    ov.Offset     = offset & 0xFFFFFFFF;
    ov.OffsetHigh = (offset >> 32) & 0xFFFFFFFF;

    WriteFile(file->handle, buffer, size, 0, &ov);

    DWORD bytes_written = 0;
    BOOL status = GetOverlappedResult(file->handle, &ov, &bytes_written, TRUE);

    return bytes_written;
}

s64 platform_write(PlatformFile *file, void const *buffer, s64 size) {
    return platform_write(file, ULLONG_MAX, buffer, size);
}

void platform_create_directory(String path) {
    String16 wide_path = to_utf16(temporary_allocator(), path, true);
    convert_slash_to_backslash((wchar_t*)wide_path.data, wide_path.size);

    CreateDirectoryW((wchar_t*)wide_path.data, 0);
}

void platform_delete_file_or_directory(String path) {
    RESET_TEMP_STORAGE_ON_EXIT();

    // NOTE: There need to be two \0 at the end of the string or SHFileOperation will continue reading the string
    // past the first zero. That happened before and I went insane because I could not understand why the program
    // suddenly deleted other files as well....
    //
    // NOTE: Can't use format here, the \0 would be ignored because of the length function
    String zeroed_path = allocate_string(path.size + 1, temporary_allocator());
    copy_memory(zeroed_path.data, path.data, path.size);
    zeroed_path[zeroed_path.size - 1] = '\0';

    String16 wide_path = to_utf16(temporary_allocator(), zeroed_path, true);
    convert_slash_to_backslash((wchar_t*)wide_path.data, wide_path.size);

    SHFILEOPSTRUCTW op = {
        0, FO_DELETE, (wchar_t*)wide_path.data, 0,
        FOF_NO_UI, 0, 0, 0
    };

    SHFileOperationW(&op);
}

String read_entire_file(String file, u32 *status, Allocator alloc) {
    RESET_TEMP_STORAGE_ON_EXIT();
    
    String16 wide_file = to_utf16(temporary_allocator(), file, true);

    HANDLE handle = CreateFileW((wchar_t*)wide_file.data, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle == INVALID_HANDLE_VALUE) {
        if (status) *status = READ_ENTIRE_FILE_NOT_FOUND;

        return {};
    }
    DEFER(CloseHandle(handle));

    LARGE_INTEGER size;
    GetFileSizeEx(handle, &size);

    String result = {};

    if (size.QuadPart) {
        result = allocate_string(size.QuadPart, alloc);

        s64 total = 0;
        while (total != result.size) {
            DWORD bytes_read = 0;

            if (!ReadFile(handle, result.data + total, size.QuadPart, &bytes_read, 0)) {
                if (status) *status = READ_ENTIRE_FILE_READ_ERROR;

                destroy_string(&result, alloc);
                return {};
            }

            total += bytes_read;
            size.QuadPart -= bytes_read;
        }
    }
    if (status) *status = READ_ENTIRE_FILE_OK;

    return result;
}

b32 write_builder_to_file(StringBuilder *builder, String file) {
    PlatformFile handle = platform_create_file_handle(file, PLATFORM_FILE_WRITE);
    if (!handle.open) return false;

    StringBuilderBlock *block = &builder->first;
    while (block) {
        platform_write(&handle, block->buffer, block->used);

        block = block->next;
    }

    platform_close_file_handle(&handle);

    return true;
}


b32 platform_is_relative_path(String path) {
    String16 wide_path = to_utf16(temporary_allocator(), path, true);
    convert_slash_to_backslash((wchar_t*)wide_path.data, wide_path.size);

    return PathIsRelativeW((wchar_t*)wide_path.data);
}

b32 platform_change_directory(String path) {
    String16 wide_path = to_utf16(temporary_allocator(), path, true);
    convert_slash_to_backslash((wchar_t*)wide_path.data, wide_path.size);

    return SetCurrentDirectory((wchar_t*)wide_path.data);
}

String platform_get_current_directory() {
    u32 length = GetCurrentDirectoryW(0, 0);
    assert(length);

    wchar_t *tmp = ALLOC(temporary_allocator(), wchar_t, length);
    length = GetCurrentDirectoryW(length, tmp);

    String16 wide_dir = {(u16*)tmp, length};

    return to_utf8(temporary_allocator(), wide_dir);
}

Array<String> platform_directory_listing(String path) {
    DArray<String> listing = {};

    String search = path;
    if (path.size == 0) {
        search = "*";
    } else if (!ends_with(path, "/") || !ends_with(path, "\\")) {
        search = t_format("%S/%s", path, "*");
    } else if (!ends_with(path, "/*") || !ends_with(path, "\\*")) {
        search = t_format("%S/%s", path, "\\*");
    }

    String16 wide_folder = to_utf16(temporary_allocator(), search, true);
    convert_slash_to_backslash((wchar_t*)wide_folder.data, wide_folder.size);

    WIN32_FIND_DATAW data;
    HANDLE handle = FindFirstFileW((wchar_t*)wide_folder.data, &data);
    if (handle == INVALID_HANDLE_VALUE) return listing;

    Allocator alloc = default_allocator();
    append(listing, to_utf8(alloc, {(u16*)data.cFileName, (s64)wcslen(data.cFileName)}));

    while (FindNextFileW(handle, &data)) {
        append(listing, to_utf8(alloc, {(u16*)data.cFileName, (s64)wcslen(data.cFileName)}));
    }

    FindClose(handle);

    return listing;
}

void platform_destroy_directory_listing(Array<String> *listing) {
    for (s64 i = 0; i < listing->size; i += 1) {
        destroy_string(&listing->memory[i]);
    }
    destroy_array(listing);
}

void *platform_allocate_raw_memory(s64 size) {
    void *result = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (result == 0) show_message_box_and_crash("Could not allocate memory.");

    return result;
}

void platform_free_raw_memory(void *memory) {
    VirtualFree(memory, 0, MEM_RELEASE);
}


u32 const STACK_TRACE_SIZE = 64;
u32 const SYMBOL_NAME_LENGTH = 1024;

INTERNAL String get_stack_trace(Allocator alloc) {
    u8 InfoStorage[sizeof(SYMBOL_INFO) + SYMBOL_NAME_LENGTH];

    void *stack[STACK_TRACE_SIZE];

    USHORT frames = CaptureStackBackTrace(0, STACK_TRACE_SIZE, stack, 0);
    SYMBOL_INFO *info = (SYMBOL_INFO *)InfoStorage;
    info->SizeOfStruct = sizeof(SYMBOL_INFO);
    info->MaxNameLen = SYMBOL_NAME_LENGTH - 1;

    DWORD displacement;
    IMAGEHLP_LINE64 line;

    StringBuilder builder = {};
    builder.allocator = alloc;
    DEFER(destroy(&builder));

    for (int i = 2; i < frames - 4; i += 1) {
        if (SymFromAddr(ProcessHandle, (DWORD64)stack[i], 0, info)) {
            SymGetLineFromAddr64(ProcessHandle, (DWORD64)stack[i], &displacement, &line);
            format(&builder, "%s(%d): %s\n", line.FileName, line.LineNumber, info->Name);
        } else {
            append(&builder, "0x000000000000: ???");
        }
    }

    return to_allocated_string(&builder, alloc);
}

INTERNAL void print_stack_trace() {
    u8 InfoStorage[sizeof(SYMBOL_INFO) + SYMBOL_NAME_LENGTH];

    void *stack[STACK_TRACE_SIZE];

    USHORT frames = CaptureStackBackTrace(0, STACK_TRACE_SIZE, stack, 0);
    SYMBOL_INFO *info = (SYMBOL_INFO *)InfoStorage;
    info->SizeOfStruct = sizeof(SYMBOL_INFO);
    info->MaxNameLen = SYMBOL_NAME_LENGTH - 1;

    DWORD displacement;
    IMAGEHLP_LINE64 line;

    for (int i = 2; i < frames - 4; i += 1) {
        if (SymFromAddr(ProcessHandle, (DWORD64)stack[i], 0, info)) {
            SymGetLineFromAddr64(ProcessHandle, (DWORD64)stack[i], &displacement, &line);
            print("%s(%d): %s\n", line.FileName, line.LineNumber, info->Name);
        } else {
            print("0x000000000000: ???");
        }
    }
}

void die(String msg) {
    print("Fatal Error: %S\n\n", msg);
    print_stack_trace();

    DebugBreak();
    ExitProcess(-1);
}

void fire_assert(char const *msg, char const *func, char const *file, int line) {
    print("Assertion failed: %s\n", msg);
    print("\t%s\n\t%s:%d\n\n", file, func, line);

    print_stack_trace();

    DebugBreak();
    ExitProcess(-1);
}


// IMPORTANT(race_condition): this is not thread safe
INTERNAL wchar_t PathBuffer[MAX_PATH];

String platform_get_executable_path() {
    s64 size = GetModuleFileNameW(0, PathBuffer, MAX_PATH);

    for (; size > 0; size -= 1) {
        if (PathBuffer[size - 1] == L'\\') break;
    }
    convert_backslash_to_slash(PathBuffer, size);

    return to_utf8(temporary_allocator(), {(u16*)PathBuffer, size});
}

b32 platform_file_exists(String file) {
    String16 wide_file = to_utf16(temporary_allocator(), file, true);
    convert_slash_to_backslash((wchar_t*)wide_file.data, wide_file.size);

    return PathFileExistsW((wchar_t*)wide_file.data);
}

/*
 * Main window creation process
 */


#include "win32_opengl.cpp"

wchar_t const *DefaultWindowClassName = L"DAO_WINDOW_CLASS";
HWND MainWindowHandle;
HDC  MainWindowDC;

LRESULT CALLBACK main_window_callback(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param);


bool WindowClassRegistered;
bool platform_setup_window() {
    if (!WindowClassRegistered) {
        WNDCLASSW wnd_class = {
            CS_OWNDC,
            main_window_callback,
            0, 0,
            GetModuleHandle(0),
            0,
            LoadCursor(0, IDC_ARROW),
            0, 0,
            DefaultWindowClassName
        };

        if (!RegisterClassW(&wnd_class)) show_message_box_and_crash("Could not create win32 window class.");
    }

    s32 const width  = 1280;
    s32 const height = 720;
    MainWindowHandle = CreateWindowW(DefaultWindowClassName, L"Thermal",
                                     WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     width, height,
                                     0, 0, GetModuleHandle(0), 0);

    if (!MainWindowHandle) show_message_box_and_crash("Could not create win32 window.");
    MainWindowDC = GetDC(MainWindowHandle);

    setup_gl_context(MainWindowHandle);
    if (load_opengl_functions(load_gl_function) == 0) {
        show_message_box_and_crash("Could not load opengl functions.");
    }

    return 0;
}

void platform_window_swap_buffers() {
    SwapBuffers(MainWindowDC);
}

String platform_file_selection_dialog(String path) {
    String16 wide_path = to_utf16(temporary_allocator(), path, true);
    convert_slash_to_backslash((wchar_t*)wide_path.data, wide_path.size);

    wchar_t path_storage[MAX_PATH];

    BROWSEINFOW dir_info = {};
    dir_info.hwndOwner = MainWindowHandle;
    dir_info.pszDisplayName = path_storage;
    dir_info.lpszTitle = L"Bitte Gothic II Verzeichnis choosen. (Fix translation)";
    dir_info.ulFlags   = BIF_NEWDIALOGSTYLE;

    PIDLIST_ABSOLUTE shell_id = SHBrowseForFolderW(&dir_info);
    SHGetPathFromIDListW(shell_id, path_storage);

    String16 result = {(u16*)path_storage, (s64)wcslen(path_storage)};
    return to_utf8(default_allocator(), result);
}

PlatformRingBuffer platform_create_ring_buffer(s32 size) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // Set size to the next page boundry, else the mapping can not be done to the memory after the first block
    size = (size + info.dwAllocationGranularity - 1) & ~(info.dwAllocationGranularity - 1);

    void *placeholder1 = VirtualAlloc2(0, 0, size * 2, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, 0, 0);
    assert(placeholder1);

    VirtualFree(placeholder1, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);

    void *placeholder2 = (u8*)placeholder1 + size;

    HANDLE section = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, size, 0);
    assert(section);

    void *view1 = MapViewOfFile3(section, 0, placeholder1, 0, size, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, 0, 0);
    assert(view1);

    void *view2 = MapViewOfFile3(section, 0, placeholder2, 0, size, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, 0, 0);
    assert(view2);

    PlatformRingBuffer ring = {};
    ring.memory = view1;
    ring.size   = 0;
    ring.alloc  = size;
    ring.pos    = 0;

    return ring;
}

s32 platform_write(PlatformRingBuffer *ring, void *buffer, s32 size) {
    if (size > ring->alloc) {
        buffer = (u8*)buffer + (size - ring->alloc);
        size = ring->alloc;
    }

    copy_memory((u8*)ring->memory + ring->pos, buffer, size);
    ring->pos  += size;
    ring->size += size;

    if (ring->pos  > ring->alloc) ring->pos  -= ring->alloc;
    if (ring->size > ring->alloc) ring->size  = ring->alloc;

    assert(ring->pos  <= ring->alloc);

    return size;
}



INTERNAL void NullRenderFunc(GameState*) {};
INTERNAL RenderFunc *CurrentRenderFunc = NullRenderFunc;

void platform_set_render_function(RenderFunc *func) {
    CurrentRenderFunc = func;
}


struct Win32ThreadData {
    HANDLE handle;
    DWORD  id;
};

// NOTE: This function is needed as an intermediate between the os and the user function.
INTERNAL DWORD WINAPI win32_thread_proxy(void *data) {
    PlatformThread *thread = (PlatformThread*)data;
    
    thread->result = thread->func(thread->user_data);

    return thread->result;
}

PlatformThread *platform_create_thread(PlatformThreadFunc *func, void *user_data) {
    PlatformThread *thread = ALLOC(default_allocator(), PlatformThread, 1);
    thread->func   = func;
    thread->user_data = user_data;

    Win32ThreadData *data = ALLOC(default_allocator(), Win32ThreadData, 1);

    // TODO: Does the handle need the value THREAD_TERMINATE set here?
    data->handle = CreateThread(0, 0, win32_thread_proxy, thread, 0, &data->id);
    if (data->handle == 0) {
        DEALLOC(default_allocator(), data, 1);
        DEALLOC(default_allocator(), thread, 1);

        return 0;
    }

    thread->platform_data = data;

    return thread;
}

void platform_destroy_thread(PlatformThread *thread) {
    Win32ThreadData *data = (Win32ThreadData*)thread->platform_data;

    TerminateThread(data->handle, 0);
    CloseHandle(data->handle);

    DEALLOC(default_allocator(), data, 1);
    DEALLOC(default_allocator(), thread, 1);
}

void platform_join_thread(PlatformThread *thread) {
    Win32ThreadData *data = (Win32ThreadData*)thread->platform_data;

    WaitForSingleObject(data->handle, INFINITE);
}

PlatformExecutionContext platform_execute(String command) {
    String16 wide_command = to_utf16(temporary_allocator(), command, true);

    HANDLE read_pipe  = 0;
    HANDLE write_pipe = 0;

    SECURITY_ATTRIBUTES attributes = {};
    attributes.nLength              = sizeof(attributes);
    attributes.bInheritHandle       = TRUE;
    attributes.lpSecurityDescriptor = 0;

    assert(CreatePipe(&read_pipe, &write_pipe, &attributes, 0));
    assert(SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0));

    PROCESS_INFORMATION process = {};
    STARTUPINFO info = {};
    info.cb         = sizeof(info);
    info.hStdError  = write_pipe;
    info.hStdOutput = write_pipe;
    info.dwFlags   |= STARTF_USESTDHANDLES;

    PlatformExecutionContext context = {};
    BOOL status = CreateProcessW(0, (wchar_t*)wide_command.data, 0, 0, TRUE, 0, 0, 0, &info, &process);
    if (!status) {
        CloseHandle(read_pipe);
        CloseHandle(write_pipe);

        return context;
    }

    context.read_pipe      = read_pipe;
    context.thread_handle  = process.hThread;
    context.process_handle = process.hProcess;
    context.started_successfully = true;

    CloseHandle(write_pipe);

    return context;
}

b32 platform_finished_execution(PlatformExecutionContext *pec) {
    if (WaitForSingleObject(pec->process_handle, 0) != WAIT_TIMEOUT) {
        CloseHandle(pec->process_handle);
        CloseHandle(pec->thread_handle);
        CloseHandle(pec->read_pipe);

        return true;
    }

    return false;
}

u32 platform_input_available(PlatformExecutionContext *pec) {
    DWORD available = 0;
    if (!PeekNamedPipe(pec->read_pipe, 0, 0, 0, &available, 0)) {
        DWORD error = GetLastError();
        // NOTE: Mentioned in the "Pipes" section of the ReadFile documentation.
        if (error == ERROR_BROKEN_PIPE) {
            CloseHandle(pec->read_pipe);
            CloseHandle(pec->thread_handle);
            CloseHandle(pec->process_handle);

            pec->started_successfully = false;
        } else {
            print("platform_input_pending: Error code %d\n", GetLastError());
        }
    }

    return available;
}

s32 platform_read(PlatformExecutionContext *pec, void *buffer, s32 size) {
    DWORD bytes_read = 0;
    ReadFile(pec->read_pipe, buffer, size, &bytes_read, 0);

    return bytes_read;
}



INTERNAL V2  CurrentMousePosition;
INTERNAL s32 CurrentMouseScroll;

INTERNAL r32 WindowWidth;
INTERNAL r32 WindowHeight;

INTERNAL b32 AltHeld;
INTERNAL b32 CtrlHeld;
INTERNAL b32 ShiftHeld;

INTERNAL KeyPress KeyBuffer[KeyBufferSize];
INTERNAL s32 KeyBufferUsed;
void platform_update(ApplicationState *state) {
    state->window_size_changed = false;

    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_QUIT) {
            state->running = false;
        }
    }

    state->user_input.last_mouse = state->user_input.mouse;
    state->user_input.mouse.cursor = CurrentMousePosition;
    state->user_input.mouse.lmb    = GetAsyncKeyState(VK_LBUTTON);
    state->user_input.mouse.scroll = CurrentMouseScroll;

    CurrentMouseScroll = 0;

    state->user_input.alt_held   = AltHeld;
    state->user_input.ctrl_held  = CtrlHeld;
    state->user_input.shift_held = ShiftHeld;

    copy_memory(state->user_input.key_buffer, KeyBuffer, KeyBufferUsed * sizeof(KeyPress));
    state->user_input.key_buffer_used = KeyBufferUsed;
    KeyBufferUsed = 0;

    if (state->window_size.width != WindowWidth || state->window_size.height != WindowHeight) {
        state->window_size.width  = WindowWidth;
        state->window_size.height = WindowHeight;
        state->window_aspect_ratio = WindowWidth / WindowHeight;

        state->window_size_changed = true;
    }
}


LRESULT CALLBACK main_window_callback(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
    break;

    case WM_MOUSEMOVE: {
        CurrentMousePosition.x = (r32)GET_X_LPARAM(l_param);
        CurrentMousePosition.y = (r32)GET_Y_LPARAM(l_param);
    } break;

    case WM_MOUSEWHEEL: {
        CurrentMouseScroll += GET_WHEEL_DELTA_WPARAM(w_param) / WHEEL_DELTA;
    } break;

    case WM_SIZE: {
        WindowWidth  = LOWORD(l_param);
        WindowHeight = HIWORD(l_param);
    } break;

    case WM_KEYDOWN: {
        if (w_param == VK_MENU) {
            AltHeld = true;
        } else if (w_param == VK_CONTROL) {
            CtrlHeld = true;
        } else if (w_param == VK_SHIFT) {
            ShiftHeld = true;
        }

        if (KeyBufferUsed == KeyBufferSize) break;

        KeyPress key = {};
        if      (w_param == VK_RETURN) key.key = KEY_RETURN;
        else if (w_param == VK_BACK)   key.key = KEY_BACKSPACE;
        else if (w_param == VK_DELETE) key.key = KEY_DELETE;
        else if (w_param == VK_LEFT)   key.key = KEY_LEFT_ARROW;
        else if (w_param == VK_RIGHT)  key.key = KEY_RIGHT_ARROW;
        else if (w_param == VK_HOME)   key.key = KEY_HOME;
        else if (w_param == VK_END)    key.key = KEY_END;

        if (key.key) {
            KeyBuffer[KeyBufferUsed] = key;
            KeyBufferUsed += 1;
        }
    } break;

    case WM_KEYUP: {
        if (w_param == VK_MENU) {
            AltHeld = false;
        } else if (w_param == VK_CONTROL) {
            CtrlHeld = false;
        } else if (w_param == VK_SHIFT) {
            ShiftHeld = false;
        }
    } break;

    case WM_CHAR: {
        if (KeyBufferUsed == KeyBufferSize) break;
        assert(!IS_HIGH_SURROGATE(w_param));

        u16 c = w_param;
        if (c < 0x20 || c == 0x7F) break;

        UTF8CharResult result = to_utf8(w_param);
        assert(result.status == 0);

        KeyPress key = {};
        key.code_point = result.cp;

        KeyBuffer[KeyBufferUsed] = key;
        KeyBufferUsed += 1;
    } break;
    }

    return DefWindowProc(handle, msg, w_param, l_param);
}

