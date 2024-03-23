#pragma once


struct PECInternal {
    HANDLE read_pipe;
    HANDLE write_pipe;

    PROCESS_INFORMATION info;
};

