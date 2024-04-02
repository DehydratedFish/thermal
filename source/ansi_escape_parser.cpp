#include "ansi_escape_parser.h"

#include "io.h"


struct ANSIColorTable {
    u32 colors[256];
};
INTERNAL ANSIColorTable const DefaultColors = {
    // Default colors.
    PACK_RGB(0x00, 0x00, 0x00),
    PACK_RGB(0x80, 0x00, 0x00),
    PACK_RGB(0x00, 0x80, 0x00),
    PACK_RGB(0x80, 0x80, 0x00),
    PACK_RGB(0x00, 0x00, 0x80),
    PACK_RGB(0x80, 0x00, 0x80),
    PACK_RGB(0x00, 0x80, 0x80),
    PACK_RGB(0xC0, 0xC0, 0xC0),

    // Default bright colors.
    PACK_RGB(0x80, 0x80, 0x80),
    PACK_RGB(0xFF, 0x00, 0x00),
    PACK_RGB(0x00, 0xFF, 0x00),
    PACK_RGB(0xFF, 0xFF, 0x00),
    PACK_RGB(0x00, 0x00, 0xFF),
    PACK_RGB(0xFF, 0x00, 0xFF),
    PACK_RGB(0x00, 0xFF, 0xFF),
    PACK_RGB(0xFF, 0xFF, 0xFF),

    // 8bit colors, 216 in total
    PACK_RGB(0x00, 0x00, 0x00), PACK_RGB(0x00, 0x00, 0x5F), PACK_RGB(0x00, 0x00, 0x87), PACK_RGB(0x00, 0x00, 0xAF), PACK_RGB(0x00, 0x00, 0xD7), PACK_RGB(0x00, 0x00, 0xFF),
    PACK_RGB(0x00, 0x5F, 0x00), PACK_RGB(0x00, 0x5F, 0x5F), PACK_RGB(0x00, 0x5F, 0x87), PACK_RGB(0x00, 0x5F, 0xAF), PACK_RGB(0x00, 0x5F, 0xD7), PACK_RGB(0x00, 0x5F, 0xFF),
    PACK_RGB(0x00, 0x87, 0x00), PACK_RGB(0x00, 0x87, 0x5F), PACK_RGB(0x00, 0x87, 0x87), PACK_RGB(0x00, 0x87, 0xAF), PACK_RGB(0x00, 0x87, 0xD7), PACK_RGB(0x00, 0x87, 0xFF),
    PACK_RGB(0x00, 0xAF, 0x00), PACK_RGB(0x00, 0xAF, 0x5F), PACK_RGB(0x00, 0xAF, 0x87), PACK_RGB(0x00, 0xAF, 0xAF), PACK_RGB(0x00, 0xAF, 0xD7), PACK_RGB(0x00, 0xAF, 0xFF),
    PACK_RGB(0x00, 0xD7, 0x00), PACK_RGB(0x00, 0xD7, 0x5F), PACK_RGB(0x00, 0xD7, 0x87), PACK_RGB(0x00, 0xD7, 0xAF), PACK_RGB(0x00, 0xD7, 0xD7), PACK_RGB(0x00, 0xD7, 0xFF),
    PACK_RGB(0x00, 0xFF, 0x00), PACK_RGB(0x00, 0xFF, 0x5F), PACK_RGB(0x00, 0xFF, 0x87), PACK_RGB(0x00, 0xFF, 0xAF), PACK_RGB(0x00, 0xFF, 0xD7), PACK_RGB(0x00, 0xFF, 0xFF),
    PACK_RGB(0x5F, 0x00, 0x00), PACK_RGB(0x5F, 0x00, 0x5F), PACK_RGB(0x5F, 0x00, 0x87), PACK_RGB(0x5F, 0x00, 0xAF), PACK_RGB(0x5F, 0x00, 0xD7), PACK_RGB(0x5F, 0x00, 0xFF),
    PACK_RGB(0x5F, 0x5F, 0x00), PACK_RGB(0x5F, 0x5F, 0x5F), PACK_RGB(0x5F, 0x5F, 0x87), PACK_RGB(0x5F, 0x5F, 0xAF), PACK_RGB(0x5F, 0x5F, 0xD7), PACK_RGB(0x5F, 0x5F, 0xFF),
    PACK_RGB(0x5F, 0x87, 0x00), PACK_RGB(0x5F, 0x87, 0x5F), PACK_RGB(0x5F, 0x87, 0x87), PACK_RGB(0x5F, 0x87, 0xAF), PACK_RGB(0x5F, 0x87, 0xD7), PACK_RGB(0x5F, 0x87, 0xFF),
    PACK_RGB(0x5F, 0xAF, 0x00), PACK_RGB(0x5F, 0xAF, 0x5F), PACK_RGB(0x5F, 0xAF, 0x87), PACK_RGB(0x5F, 0xAF, 0xAF), PACK_RGB(0x5F, 0xAF, 0xD7), PACK_RGB(0x5F, 0xAF, 0xFF),
    PACK_RGB(0x5F, 0xD7, 0x00), PACK_RGB(0x5F, 0xD7, 0x5F), PACK_RGB(0x5F, 0xD7, 0x87), PACK_RGB(0x5F, 0xD7, 0xAF), PACK_RGB(0x5F, 0xD7, 0xD7), PACK_RGB(0x5F, 0xD7, 0xFF),
    PACK_RGB(0x5F, 0xFF, 0x00), PACK_RGB(0x5F, 0xFF, 0x5F), PACK_RGB(0x5F, 0xFF, 0x87), PACK_RGB(0x5F, 0xFF, 0xAF), PACK_RGB(0x5F, 0xFF, 0xD7), PACK_RGB(0x5F, 0xFF, 0xFF),
    PACK_RGB(0x87, 0x00, 0x00), PACK_RGB(0x87, 0x00, 0x5F), PACK_RGB(0x87, 0x00, 0x87), PACK_RGB(0x87, 0x00, 0xAF), PACK_RGB(0x87, 0x00, 0xD7), PACK_RGB(0x87, 0x00, 0xFF),
    PACK_RGB(0x87, 0x5F, 0x00), PACK_RGB(0x87, 0x5F, 0x5F), PACK_RGB(0x87, 0x5F, 0x87), PACK_RGB(0x87, 0x5F, 0xAF), PACK_RGB(0x87, 0x5F, 0xD7), PACK_RGB(0x87, 0x5F, 0xFF),
    PACK_RGB(0x87, 0x87, 0x00), PACK_RGB(0x87, 0x87, 0x5F), PACK_RGB(0x87, 0x87, 0x87), PACK_RGB(0x87, 0x87, 0xAF), PACK_RGB(0x87, 0x87, 0xD7), PACK_RGB(0x87, 0x87, 0xFF),
    PACK_RGB(0x87, 0xAF, 0x00), PACK_RGB(0x87, 0xAF, 0x5F), PACK_RGB(0x87, 0xAF, 0x87), PACK_RGB(0x87, 0xAF, 0xAF), PACK_RGB(0x87, 0xAF, 0xD7), PACK_RGB(0x87, 0xAF, 0xFF),
    PACK_RGB(0x87, 0xD7, 0x00), PACK_RGB(0x87, 0xD7, 0x5F), PACK_RGB(0x87, 0xD7, 0x87), PACK_RGB(0x87, 0xD7, 0xAF), PACK_RGB(0x87, 0xD7, 0xD7), PACK_RGB(0x87, 0xD7, 0xFF),
    PACK_RGB(0x87, 0xFF, 0x00), PACK_RGB(0x87, 0xFF, 0x5F), PACK_RGB(0x87, 0xFF, 0x87), PACK_RGB(0x87, 0xFF, 0xAF), PACK_RGB(0x87, 0xFF, 0xD7), PACK_RGB(0x87, 0xFF, 0xFF),
    PACK_RGB(0xAF, 0x00, 0x00), PACK_RGB(0xAF, 0x00, 0x5F), PACK_RGB(0xAF, 0x00, 0x87), PACK_RGB(0xAF, 0x00, 0xAF), PACK_RGB(0xAF, 0x00, 0xD7), PACK_RGB(0xAF, 0x00, 0xFF),
    PACK_RGB(0xAF, 0x5F, 0x00), PACK_RGB(0xAF, 0x5F, 0x5F), PACK_RGB(0xAF, 0x5F, 0x87), PACK_RGB(0xAF, 0x5F, 0xAF), PACK_RGB(0xAF, 0x5F, 0xD7), PACK_RGB(0xAF, 0x5F, 0xFF),
    PACK_RGB(0xAF, 0x87, 0x00), PACK_RGB(0xAF, 0x87, 0x5F), PACK_RGB(0xAF, 0x87, 0x87), PACK_RGB(0xAF, 0x87, 0xAF), PACK_RGB(0xAF, 0x87, 0xD7), PACK_RGB(0xAF, 0x87, 0xFF),
    PACK_RGB(0xAF, 0xAF, 0x00), PACK_RGB(0xAF, 0xAF, 0x5F), PACK_RGB(0xAF, 0xAF, 0x87), PACK_RGB(0xAF, 0xAF, 0xAF), PACK_RGB(0xAF, 0xAF, 0xD7), PACK_RGB(0xAF, 0xAF, 0xFF),
    PACK_RGB(0xAF, 0xD7, 0x00), PACK_RGB(0xAF, 0xD7, 0x5F), PACK_RGB(0xAF, 0xD7, 0x87), PACK_RGB(0xAF, 0xD7, 0xAF), PACK_RGB(0xAF, 0xD7, 0xD7), PACK_RGB(0xAF, 0xD7, 0xFF),
    PACK_RGB(0xAF, 0xFF, 0x00), PACK_RGB(0xAF, 0xFF, 0x5F), PACK_RGB(0xAF, 0xFF, 0x87), PACK_RGB(0xAF, 0xFF, 0xAF), PACK_RGB(0xAF, 0xFF, 0xD7), PACK_RGB(0xAF, 0xFF, 0xFF),
    PACK_RGB(0xD7, 0x00, 0x00), PACK_RGB(0xD7, 0x00, 0x5F), PACK_RGB(0xD7, 0x00, 0x87), PACK_RGB(0xD7, 0x00, 0xAF), PACK_RGB(0xD7, 0x00, 0xD7), PACK_RGB(0xD7, 0x00, 0xFF),
    PACK_RGB(0xD7, 0x5F, 0x00), PACK_RGB(0xD7, 0x5F, 0x5F), PACK_RGB(0xD7, 0x5F, 0x87), PACK_RGB(0xD7, 0x5F, 0xAF), PACK_RGB(0xD7, 0x5F, 0xD7), PACK_RGB(0xD7, 0x5F, 0xFF),
    PACK_RGB(0xD7, 0x87, 0x00), PACK_RGB(0xD7, 0x87, 0x5F), PACK_RGB(0xD7, 0x87, 0x87), PACK_RGB(0xD7, 0x87, 0xAF), PACK_RGB(0xD7, 0x87, 0xD7), PACK_RGB(0xD7, 0x87, 0xFF),
    PACK_RGB(0xD7, 0xAF, 0x00), PACK_RGB(0xD7, 0xAF, 0x5F), PACK_RGB(0xD7, 0xAF, 0x87), PACK_RGB(0xD7, 0xAF, 0xAF), PACK_RGB(0xD7, 0xAF, 0xD7), PACK_RGB(0xD7, 0xAF, 0xFF),
    PACK_RGB(0xD7, 0xD7, 0x00), PACK_RGB(0xD7, 0xD7, 0x5F), PACK_RGB(0xD7, 0xD7, 0x87), PACK_RGB(0xD7, 0xD7, 0xAF), PACK_RGB(0xD7, 0xD7, 0xD7), PACK_RGB(0xD7, 0xD7, 0xFF),
    PACK_RGB(0xD7, 0xFF, 0x00), PACK_RGB(0xD7, 0xFF, 0x5F), PACK_RGB(0xD7, 0xFF, 0x87), PACK_RGB(0xD7, 0xFF, 0xAF), PACK_RGB(0xD7, 0xFF, 0xD7), PACK_RGB(0xD7, 0xFF, 0xFF),
    PACK_RGB(0xFF, 0x00, 0x00), PACK_RGB(0xFF, 0x00, 0x5F), PACK_RGB(0xFF, 0x00, 0x87), PACK_RGB(0xFF, 0x00, 0xAF), PACK_RGB(0xFF, 0x00, 0xD7), PACK_RGB(0xFF, 0x00, 0xFF),
    PACK_RGB(0xFF, 0x5F, 0x00), PACK_RGB(0xFF, 0x5F, 0x5F), PACK_RGB(0xFF, 0x5F, 0x87), PACK_RGB(0xFF, 0x5F, 0xAF), PACK_RGB(0xFF, 0x5F, 0xD7), PACK_RGB(0xFF, 0x5F, 0xFF),
    PACK_RGB(0xFF, 0x87, 0x00), PACK_RGB(0xFF, 0x87, 0x5F), PACK_RGB(0xFF, 0x87, 0x87), PACK_RGB(0xFF, 0x87, 0xAF), PACK_RGB(0xFF, 0x87, 0xD7), PACK_RGB(0xFF, 0x87, 0xFF),
    PACK_RGB(0xFF, 0xAF, 0x00), PACK_RGB(0xFF, 0xAF, 0x5F), PACK_RGB(0xFF, 0xAF, 0x87), PACK_RGB(0xFF, 0xAF, 0xAF), PACK_RGB(0xFF, 0xAF, 0xD7), PACK_RGB(0xFF, 0xAF, 0xFF),
    PACK_RGB(0xFF, 0xD7, 0x00), PACK_RGB(0xFF, 0xD7, 0x5F), PACK_RGB(0xFF, 0xD7, 0x87), PACK_RGB(0xFF, 0xD7, 0xAF), PACK_RGB(0xFF, 0xD7, 0xD7), PACK_RGB(0xFF, 0xD7, 0xFF),
    PACK_RGB(0xFF, 0xFF, 0x00), PACK_RGB(0xFF, 0xFF, 0x5F), PACK_RGB(0xFF, 0xFF, 0x87), PACK_RGB(0xFF, 0xFF, 0xAF), PACK_RGB(0xFF, 0xFF, 0xD7), PACK_RGB(0xFF, 0xFF, 0xFF),

    // 24 shades of grey
    PACK_RGB(0x08, 0x08, 0x08),
    PACK_RGB(0x12, 0x12, 0x12),
    PACK_RGB(0x1C, 0x1C, 0x1C),
    PACK_RGB(0x26, 0x26, 0x26),
    PACK_RGB(0x30, 0x30, 0x30),
    PACK_RGB(0x3A, 0x3A, 0x3A),
    PACK_RGB(0x44, 0x44, 0x44),
    PACK_RGB(0x4E, 0x4E, 0x4E),
    PACK_RGB(0x58, 0x58, 0x58),
    PACK_RGB(0x60, 0x60, 0x60),
    PACK_RGB(0x66, 0x66, 0x66),
    PACK_RGB(0x76, 0x76, 0x76),
    PACK_RGB(0x80, 0x80, 0x80),
    PACK_RGB(0x8A, 0x8A, 0x8A),
    PACK_RGB(0x94, 0x94, 0x94),
    PACK_RGB(0x9E, 0x9E, 0x9E),
    PACK_RGB(0xA8, 0xA8, 0xA8),
    PACK_RGB(0xB2, 0xB2, 0xB2),
    PACK_RGB(0xBC, 0xBC, 0xBC),
    PACK_RGB(0xC6, 0xC6, 0xC6),
    PACK_RGB(0xD0, 0xD0, 0xD0),
    PACK_RGB(0xDA, 0xDA, 0xDA),
    PACK_RGB(0xE4, 0xE4, 0xE4),
    PACK_RGB(0xEE, 0xEE, 0xEE),
};

INTERNAL ANSIColorTable ColorPalette = DefaultColors;


// TODO: Harden the parser to reject incomplete or invalid/unknown paths.
EscapeSequence parse_escape_sequence(String str) {
    EscapeSequence seq = {};

    s64 index = 0;
    if (index < str.size && str[index] == 0x1B) {
        index += 1;

        if (index < str.size && str[index] == '[') {
            index += 1;

            String arg = {str.data + index, 0};
            while (index < str.size) {
                if (str[index] >= '0' && str[index] <= '9') {
                    arg.size += 1;
                } else if (str[index] == ';') {
                    s64 num = 0;
                    if (arg.size) {
                        num = convert_string_to_s64(arg.data, arg.size);
                    }

                    if (seq.arg_count == EscapeSequenceMaxArgs) {
                        LOG(LOG_ERROR, "Escape sequence has too many arguments.");
                        return {};
                    }

                    seq.args[seq.arg_count] = num;
                    seq.arg_count += 1;

                    arg = {str.data + index + 1, 0};
                } else {
                    s64 num = 0;
                    if (arg.size) {
                        num = convert_string_to_s64(arg.data, arg.size);
                    }

                    if (seq.arg_count == EscapeSequenceMaxArgs) {
                        LOG(LOG_ERROR, "Escape sequence has too many arguments.");
                        return {};
                    }

                    seq.args[seq.arg_count] = num;
                    seq.arg_count += 1;

                    break;
                }

                index += 1;
            }

            if (str[index] < 0x40 || str[index] > 0x7E) {
                String error = {str.data, index};
                LOG(LOG_ERROR, "Unsupported ANSI escape sequence %S.", error);

                return {};
            }

            seq.kind = str[index];

            index += 1;
            seq.valid = true;
        }
    }

    seq.length_in_bytes = index;

    return seq;
}

u32 ansi_4bit_color(u32 color_name) {
    if (color_name < ESCAPE_CSI_FOREGROUND_BLACK) {
        LOG(LOG_ERROR, "ansi_4bit_color: Unknown color name %u.\n", color_name);
    } else if (color_name < ESCAPE_CSI_FOREGROUND) {
        return ColorPalette.colors[color_name - ESCAPE_CSI_FOREGROUND_BLACK];
    } else if (color_name < ESCAPE_CSI_BACKGROUND) {
        return ColorPalette.colors[color_name - ESCAPE_CSI_BACKGROUND_BLACK];
    } else if (color_name < ESCAPE_CSI_BACKGROUND_BRIGHT_BLACK) {
        return ColorPalette.colors[color_name - ESCAPE_CSI_FOREGROUND_BRIGHT_BLACK + 8];
    } else if (color_name < ESCAPE_CSI_NAMED_COLORS_MAX) {
        return ColorPalette.colors[color_name - ESCAPE_CSI_BACKGROUND_BRIGHT_BLACK + 8];
    } else {
        LOG(LOG_ERROR, "ansi_4bit_color: Unknown color name %u.\n", color_name);
    }

    return PACK_RGB(0, 0, 0);
}

u32 ansi_8bit_color(u8 index) {
    return ColorPalette.colors[index];
}

