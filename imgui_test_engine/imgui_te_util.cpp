#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui_te_util.h"
#include "imgui.h"
#include "imgui_internal.h"
#define STR_IMPLEMENTATION
#include "libs/Str/Str.h"

// Hash "hello/world" as if it was "helloworld"
// To hash a forward slash we need to use "hello\\/world"
//   IM_ASSERT(ImHashDecoratedPath("Hello/world")   == ImHash("Helloworld", 0));
//   IM_ASSERT(ImHashDecoratedPath("Hello\\/world") == ImHash("Hello/world", 0));
// Adapted from ImHash(). Not particularly fast!
ImGuiID ImHashDecoratedPath(const char* str, const char* str_end, ImGuiID seed)
{
    static ImU32 crc32_lut[256] = { 0 };
    if (!crc32_lut[1])
    {
        const ImU32 polynomial = 0xEDB88320;
        for (ImU32 i = 0; i < 256; i++)
        {
            ImU32 crc = i;
            for (ImU32 j = 0; j < 8; j++)
                crc = (crc >> 1) ^ (ImU32(-int(crc & 1)) & polynomial);
            crc32_lut[i] = crc;
        }
    }

    // Prefixing the string with / ignore the seed
    if (str != str_end && str[0] == '/')
        seed = 0;

    seed = ~seed;
    ImU32 crc = seed;

    // Zero-terminated string
    bool inhibit_one = false;
    const unsigned char* current = (const unsigned char*)str;
    while (true)
    {
        if (str_end != NULL && current == (const unsigned char*)str_end)
            break;

        const unsigned char c = *current++;
        if (c == 0)
            break;
        if (c == '\\' && !inhibit_one)
        {
            inhibit_one = true;
            continue;
        }

        // Forward slashes are ignored unless prefixed with a backward slash
        if (c == '/' && !inhibit_one)
        {
            inhibit_one = false;
            continue;
        }

        // Reset the hash when encountering ###
        if (c == '#' && current[0] == '#' && current[1] == '#')
            crc = seed;

        crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
        inhibit_one = false;
    }
    return ~crc;
}

// FIXME: Think they are 16 combinations we may as well store them in literals?
void GetImGuiKeyModsPrefixStr(ImGuiKeyModFlags mod_flags, char* out_buf, size_t out_buf_size)
{
    if (mod_flags == 0)
    {
        out_buf[0] = 0;
        return;
    }
    ImFormatString(out_buf, out_buf_size, "%s%s%s%s",
        (mod_flags & ImGuiKeyModFlags_Ctrl) ? "Ctrl+" : "",
        (mod_flags & ImGuiKeyModFlags_Alt) ? "Alt+" : "",
        (mod_flags & ImGuiKeyModFlags_Shift) ? "Shift+" : "",
        (mod_flags & ImGuiKeyModFlags_Super) ? "Super+" : "");
}

const char* GetImGuiKeyName(ImGuiKey key)
{
    // Create switch-case from enum with regexp: ImGuiKey_{.*}, --> case ImGuiKey_\1: return "\1";
    switch (key)
    {
    case ImGuiKey_Tab: return "Tab";
    case ImGuiKey_LeftArrow: return "LeftArrow";
    case ImGuiKey_RightArrow: return "RightArrow";
    case ImGuiKey_UpArrow: return "UpArrow";
    case ImGuiKey_DownArrow: return "DownArrow";
    case ImGuiKey_PageUp: return "PageUp";
    case ImGuiKey_PageDown: return "PageDown";
    case ImGuiKey_Home: return "Home";
    case ImGuiKey_End: return "End";
    case ImGuiKey_Insert: return "Insert";
    case ImGuiKey_Delete: return "Delete";
    case ImGuiKey_Backspace: return "Backspace";
    case ImGuiKey_Space: return "Space";
    case ImGuiKey_Enter: return "Enter";
    case ImGuiKey_Escape: return "Escape";
    case ImGuiKey_A: return "A";
    case ImGuiKey_C: return "C";
    case ImGuiKey_V: return "V";
    case ImGuiKey_X: return "X";
    case ImGuiKey_Y: return "Y";
    case ImGuiKey_Z: return "Z";
    }
    IM_ASSERT(0);
    return "Unknown";
}

// FIXME-TESTS: Should eventually remove.
ImFont* FindFontByName(const char* name)
{
    ImGuiContext& g = *GImGui;
    for (ImFont* font : g.IO.Fonts->Fonts)
        if (strcmp(font->ConfigData->Name, name) == 0)
            return font;
    return NULL;
}

static const char* ImStrStr(const char* haystack, size_t hlen, const char* needle, int nlen)
{
    const char* end = haystack + hlen;
    const char* p = haystack;
    while ((p = (const char*)memchr(p, *needle, end - p)) != NULL)
    {
        if (end - p < nlen)
            return NULL;
        if (memcmp(p, needle, nlen) == 0)
            return p;
        p++;
    }
    return NULL;
}

void ImStrReplace(Str* s, const char* find, const char* repl)
{
    IM_ASSERT(find != NULL && *find);
    IM_ASSERT(repl != NULL);
    int find_len = (int)strlen(find);
    int repl_len = (int)strlen(repl);
    int repl_diff = repl_len - find_len;

    // Estimate required length of new buffer if string size increases.
    int need_capacity = s->capacity();
    int num_matches = INT_MAX;
    if (repl_diff > 0)
    {
        num_matches = 0;
        need_capacity = s->length() + 1;
        for (char* p = s->c_str(), *end = s->c_str() + s->length(); p != NULL && p < end;)
        {
            p = (char*)ImStrStr(p, end - p, find, find_len);
            if (p)
            {
                need_capacity += repl_diff;
                p += find_len;
                num_matches++;
            }
        }
    }

    if (num_matches == 0)
        return;

    const char* not_owned_data = s->owned() ? NULL : s->c_str();
    if (!s->owned() || need_capacity > s->capacity())
        s->reserve(need_capacity);
    if (not_owned_data != NULL)
        s->set(not_owned_data);

    // Replace data.
    for (char* p = s->c_str(), *end = s->c_str() + s->length(); p != NULL && p < end && num_matches--;)
    {
        p = (char*)ImStrStr(p, end - p, find, find_len);
        if (p)
        {
            memmove(p + repl_len, p + find_len, end - p - find_len + 1);
            memcpy(p, repl, repl_len);
            p += repl_len;
            end += repl_diff;
        }
    }
}

// Based on code from https://github.com/EddieBreeg/C_b64 by @EddieBreeg.
int ImBase64Encode(const unsigned char* src, char* dst, int length)
{
    static const char* b64Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
    int i, j, k, l, encoded_len = 0;

    while (length > 0)
    {
        switch (length)
        {
        case 1:
            i = src[0] >> 2;
            j = (src[0] & 3) << 4;
            k = 64;
            l = 64;
            break;
        case 2:
            i = src[0] >> 2;
            j = ((src[0] & 3) << 4) | (src[1] >> 4);
            k = (src[1] & 15) << 2;
            l = 64;
            break;
        default:
            i = src[0] >> 2;
            j = ((src[0] & 3) << 4) | (src[1] >> 4);
            k = ((src[1] & 0xf) << 2) | (src[2] >> 6 & 3);
            l = src[2] & 0x3f;
            break;
        }
        dst[0] = b64Table[i];
        dst[1] = b64Table[j];
        dst[2] = b64Table[k];
        dst[3] = b64Table[l];
        src += 3;
        dst += 4;
        length -= 3;
        encoded_len += 4;
    }
    return encoded_len;
}

//-----------------------------------------------------------------------------
// STR + InputText bindings
//-----------------------------------------------------------------------------

struct InputTextCallbackStr_UserData
{
    Str*                    StrObj;
    ImGuiInputTextCallback  ChainCallback;
    void*                   ChainCallbackUserData;
};

static int InputTextCallbackStr(ImGuiInputTextCallbackData* data)
{
    InputTextCallbackStr_UserData* user_data = (InputTextCallbackStr_UserData*)data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        // Resize string callback
        // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
        Str* str = user_data->StrObj;
        IM_ASSERT(data->Buf == str->c_str());
        str->reserve(data->BufTextLen + 1);
        data->Buf = (char*)str->c_str();
    }
    else if (user_data->ChainCallback)
    {
        // Forward to user callback, if any
        data->UserData = user_data->ChainCallbackUserData;
        return user_data->ChainCallback(data);
    }
    return 0;
}

bool ImGui::InputText(const char* label, Str* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallbackStr_UserData cb_user_data;
    cb_user_data.StrObj = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputText(label, (char*)str->c_str(), (size_t)str->capacity() + 1, flags, InputTextCallbackStr, &cb_user_data);
}

bool ImGui::InputTextWithHint(const char* label, const char* hint, Str* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallbackStr_UserData cb_user_data;
    cb_user_data.StrObj = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputTextWithHint(label, hint, (char*)str->c_str(), (size_t)str->capacity() + 1, flags, InputTextCallbackStr, &cb_user_data);
}

bool ImGui::InputTextMultiline(const char* label, Str* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallbackStr_UserData cb_user_data;
    cb_user_data.StrObj = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputTextMultiline(label, (char*)str->c_str(), (size_t)str->capacity() + 1, size, flags, InputTextCallbackStr, &cb_user_data);
}

// anchor parameter indicates which split would retain it's constant size.
// anchor = 0 - both splits resize when parent container size changes. Both value_1 and value_2 should be persistent.
// anchor = -1 - top/left split would have a constant size. bottom/right split would resize when parent container size changes. value_1 should be persistent, value_2 will always be recalculated from value_1.
// anchor = +1 - bottom/right split would have a constant size. top/left split would resize when parent container size changes. value_2 should be persistent, value_1 will always be recalculated from value_2.
bool ImGui::Splitter(const char* id, float* value_1, float* value_2, int axis, int anchor, float min_size_0, float min_size_1)
{
    // FIXME-DOGFOODING: This needs further refining.
    // FIXME-SCROLL: When resizing either we'd like to keep scroll focus on something (e.g. last clicked item for list, bottom for log)
    // See https://github.com/ocornut/imgui/issues/319
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (min_size_0 < 0)
        min_size_0 = ImGui::GetFrameHeight();
    if (min_size_1)
        min_size_1 = ImGui::GetFrameHeight();

    IM_ASSERT(axis == ImGuiAxis_X || axis == ImGuiAxis_Y);

    float& v_1 = *value_1;
    float& v_2 = *value_2;
    ImRect splitter_bb;
    const float avail = axis == ImGuiAxis_X ? ImGui::GetContentRegionAvail().x - style.ItemSpacing.x : ImGui::GetContentRegionAvail().y - style.ItemSpacing.y;
    if (anchor < 0)
    {
        v_2 = ImMax(avail - v_1, min_size_1);   // First split is constant size.
    }
    else if (anchor > 0)
    {
        v_1 = ImMax(avail - v_2, min_size_0);   // Second split is constant size.
    }
    else
    {
        float r = v_1 / (v_1 + v_2);            // Both splits maintain same relative size to parent.
        v_1 = IM_ROUND(avail * r) - 1;
        v_2 = IM_ROUND(avail * (1.0f - r)) - 1;
    }
    if (axis == ImGuiAxis_X)
    {
        float x = window->DC.CursorPos.x + v_1 + IM_ROUND(style.ItemSpacing.x * 0.5f);
        splitter_bb = ImRect(x - 1, window->WorkRect.Min.y, x + 1, window->WorkRect.Max.y);
    }
    else if (axis == ImGuiAxis_Y)
    {
        float y = window->DC.CursorPos.y + v_1 + IM_ROUND(style.ItemSpacing.y * 0.5f);
        splitter_bb = ImRect(window->WorkRect.Min.x, y - 1, window->WorkRect.Max.x, y + 1);
    }
    return ImGui::SplitterBehavior(splitter_bb, ImGui::GetID(id), (ImGuiAxis)axis, &v_1, &v_2, min_size_0, min_size_1, 3.0f);
}

ImGuiID TableGetHeaderID(ImGuiTable* table, const char* column, int instance_no)
{
    IM_ASSERT(table != NULL);
    int column_n = -1;
    for (int n = 0; n < table->Columns.size() && column_n < 0; n++)
        if (strcmp(ImGui::TableGetColumnName(table, n), column) == 0)
            column_n = n;
    IM_ASSERT(column_n != -1);
    int column_id = instance_no * table->ColumnsCount + column_n;
    return ImHashData(column, strlen(column), ImHashData(&column_id, sizeof(column_id), table->ID + instance_no));
}

ImGuiID TableGetHeaderID(ImGuiTable* table, int column_n, int instance_no)
{
    IM_ASSERT(column_n >= 0 && column_n < table->ColumnsCount);
    int column_id = instance_no * table->ColumnsCount + column_n;
    const char* column_name = ImGui::TableGetColumnName(table, column_n);
    return ImHashData(column_name, strlen(column_name), ImHashData(&column_id, sizeof(column_id), table->ID + instance_no));
}

void TableDiscardInstanceAndSettings(ImGuiID table_id)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.CurrentTable == NULL);
    if (ImGuiTableSettings* settings = ImGui::TableSettingsFindByID(table_id))
        settings->ID = 0;

    if (ImGuiTable* table = ImGui::TableFindByID(table_id))
        ImGui::TableRemove(table);
    // FIXME-TABLE: We should be able to use TableResetSettings() instead of TableRemove()! Maybe less of a clean slate but would be good to check that it does the job
    //ImGui::TableResetSettings(table);
}

//-----------------------------------------------------------------------------
// Simple CSV parser
//-----------------------------------------------------------------------------

void ImGuiCSVParser::Clear()
{
    Rows = Columns = 0;
    if (_Data != NULL)
        IM_FREE(_Data);
    _Data = NULL;
    _Index.clear();
}

bool ImGuiCSVParser::Load(const char* filename)
{
    size_t len = 0;
    _Data = (char*)ImFileLoadToMemory(filename, "rb", &len, 1);
    if (_Data == NULL)
        return false;

    int columns = 1;
    if (Columns > 0)
    {
        columns = Columns;                                          // User-provided expected column count.
    }
    else
    {
        for (const char* c = _Data; *c != '\n' && *c != '\0'; c++)  // Count columns. Quoted columns with commas are not supported.
            if (*c == ',')
                columns++;
    }

    // Count rows. Extra new lines anywhere in the file are ignored.
    int max_rows = 0;
    for (const char* c = _Data, *end = c + len; c < end; c++)
        if ((*c == '\n' && c[1] != '\r' && c[1] != '\n') || *c == '\0')
            max_rows++;

    if (columns == 0 || max_rows == 0)
        return false;

    // Create index
    _Index.resize(columns * max_rows);

    int col = 0;
    char* col_data = _Data;
    for (char* c = _Data; *c != '\0'; c++)
    {
        const bool is_comma = (*c == ',');
        const bool is_eol = (*c == '\n' || *c == '\r');
        const bool is_eof = (*c == '\0');
        if (is_comma || is_eol || is_eof)
        {
            _Index[Rows * columns + col] = col_data;
            col_data = c + 1;
            if (is_comma)
            {
                col++;
            }
            else
            {
                if (col + 1 == columns)
                    Rows++;
                else
                    fprintf(stderr, "%s: Unexpected number of columns on line %d, ignoring.\n", filename, Rows + 1); // FIXME
                col = 0;
            }
            *c = 0;
            if (is_eol)
                while (c[1] == '\r' || c[1] == '\n')
                    c++;
        }
    }

    Columns = columns;
    return true;
}

//-----------------------------------------------------------------------------