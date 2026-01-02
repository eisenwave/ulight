#include <algorithm>
#include <string_view>

#include "ulight/impl/ascii_algorithm.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/highlighter.hpp"
#include "ulight/impl/numbers.hpp"
#include "ulight/impl/parse_utils.hpp"

#include "ulight/impl/lang/nasm.hpp"
#include "ulight/impl/lang/nasm_chars.hpp"

namespace ulight {
namespace nasm {

[[nodiscard]]
Escape_Result match_escape_sequence(std::u8string_view str)
{
    // https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html#section-3.4.2
    if (str.length() < 2 || str[0] != u8'\\') {
        return {};
    }
    switch (str[1]) {
    case u8'\'':
    case u8'"':
    case u8'`':
    case u8'\\':
    case u8'?':
    case u8'a':
    case u8'b':
    case u8't':
    case u8'n':
    case u8'v':
    case u8'f':
    case u8'r':
    case u8'e': {
        return { .length = 2uz };
    }
    case u8'0':
    case u8'1':
    case u8'2':
    case u8'3':
    case u8'4':
    case u8'5':
    case u8'6':
    case u8'7': {
        return match_common_escape<Common_Escape::octal_1_to_2>(str, 2);
    }
    case u8'x': {
        return match_common_escape<Common_Escape::hex_1_to_2>(str, 2);
    }
    case u8'u': {
        return match_common_escape<Common_Escape::hex_4>(str, 2);
    }
    case u8'U': {
        return match_common_escape<Common_Escape::hex_8>(str, 2);
    }
    default: {
        return { .length = 2uz, .erroneous = true };
    }
    }
}

std::size_t match_operator(std::u8string_view str)
{
    // https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html#section-3.5
    if (str.empty()) {
        return 0;
    }
    switch (str[0]) {
    case u8'?':
    case u8':':
    case u8'+':
    case u8'-':
    case u8'~': return 1;
    case u8'!': return str.starts_with(u8"!=") ? 2 : 1;
    case u8'|': return str.starts_with(u8"||") ? 2 : 1;
    case u8'&': return str.starts_with(u8"&&") ? 2 : 1;
    case u8'^': return str.starts_with(u8"^^") ? 2 : 1;
    case u8'>':
        return str.starts_with(u8">>>") ? 3 //
            : str.starts_with(u8">>")   ? 2
            : str.starts_with(u8">=")   ? 2
                                        : 1;
    case u8'<':
        return str.starts_with(u8"<<<") ? 3 //
            : str.starts_with(u8"<=>")  ? 3
            : str.starts_with(u8"<<")   ? 2
            : str.starts_with(u8"<=")   ? 2
                                        : 1;
    case u8'/': return str.starts_with(u8"//") ? 2 : 1;
    case u8'%': return str.starts_with(u8"%%") ? 2 : 1;
    default: return 0;
    }
    ULIGHT_ASSERT_UNREACHABLE(u8"Logic bug.");
}

std::size_t match_identifier(std::u8string_view str)
{
    constexpr auto head = [](char8_t c) { return is_nasm_identifier_start(c); };
    constexpr auto tail = [](char8_t c) { return is_nasm_identifier(c); };
    return ascii::length_if_head_tail(str, head, tail);
}

[[nodiscard]]
int base_of_suffix_char(char8_t c)
{
    switch (c) {
    case u8'b':
    case u8'B':
    case u8'y':
    case u8'Y': return 2;
    case u8'q':
    case u8'Q':
    case u8'o':
    case u8'O': return 8;
    case u8'd':
    case u8'D':
    case u8't':
    case u8'T': return 10;
    case u8'h':
    case u8'H':
    case u8'x':
    case u8'X': return 16;
    default: return 0;
    }
}

namespace {

constexpr std::u8string_view pseudo_instructions[] {
    u8"db",     u8"dd",   u8"do",   u8"dq", //
    u8"dt",     u8"dw",   u8"dy",   u8"dz", //
    u8"equ", //
    u8"incbin", //
    u8"resb",   u8"resd", u8"reso", u8"resq", //
    u8"rest",   u8"resw", u8"resy", u8"resz", //
    u8"times",
};

static_assert(std::ranges::is_sorted(pseudo_instructions));

constexpr std::u8string_view types[] {
    u8"byte",  u8"dword", u8"far",  u8"oword", u8"ptr",
    u8"qword", u8"tword", u8"word", u8"yword", u8"zword",
};

static_assert(std::ranges::is_sorted(types));

constexpr std::u8string_view operator_keywords[] {
    u8"seg",
    u8"wrt",
};

static_assert(std::ranges::is_sorted(operator_keywords));

// clang-format off
constexpr std::u8string_view registers[] {
    u8"ah",
    u8"al",
    u8"ax",
    u8"bh",
    u8"bl",
    u8"bp",
    u8"bpl",
    u8"bx",
    
    u8"ch",
    u8"cl",
    u8"cr0", u8"cr2", u8"cr3", u8"cr4", u8"cr8",
    u8"cs",
    u8"cw",
    u8"cx",
    
    u8"dh",
    u8"di",
    u8"dil",
    u8"dl",
    u8"dr0", u8"dr1", u8"dr2", u8"dr3", u8"dr6", u8"dr7",
    u8"ds",
    u8"dx",
    
    u8"eax",
    u8"ebp",
    u8"ebx",
    u8"ecx",
    u8"edi",
    u8"edx",
    u8"eflags",
    u8"eip",
    u8"es",
    u8"esi",
    u8"esp",
    
    u8"fs",
    
    u8"gs",

    u8"ip",
    
    u8"k0", u8"k1", u8"k2", u8"k3", u8"k4", u8"k5", u8"k6", u8"k7",
    
    u8"mm0", u8"mm1", u8"mm2", u8"mm3", u8"mm4", u8"mm5", u8"mm6", u8"mm7",
    
    u8"r10", u8"r10b", u8"r10d", u8"r10w",
    u8"r11", u8"r11b", u8"r11d", u8"r11w",
    u8"r12", u8"r12b", u8"r12d", u8"r12w",
    u8"r13", u8"r13b", u8"r13d", u8"r13w",
    u8"r14", u8"r14b", u8"r14d", u8"r14w",
    u8"r15", u8"r15b", u8"r15d", u8"r15w",
    u8"r16", u8"r16b", u8"r16d", u8"r16w",
    u8"r17", u8"r17b", u8"r17d", u8"r17w",
    u8"r18", u8"r18b", u8"r18d", u8"r18w",
    u8"r19", u8"r19b", u8"r19d", u8"r19w",
    u8"r20", u8"r20b", u8"r20d", u8"r20w",
    u8"r21", u8"r21b", u8"r21d", u8"r21w",
    u8"r22", u8"r22b", u8"r22d", u8"r22w",
    u8"r23", u8"r23b", u8"r23d", u8"r23w",
    u8"r24", u8"r24b", u8"r24d", u8"r24w",
    u8"r25", u8"r25b", u8"r25d", u8"r25w",
    u8"r26", u8"r26b", u8"r26d", u8"r26w",
    u8"r27", u8"r27b", u8"r27d", u8"r27w",
    u8"r28", u8"r28b", u8"r28d", u8"r28w",
    u8"r29", u8"r29b", u8"r29d", u8"r29w",
    u8"r30", u8"r30b", u8"r30d", u8"r30w",
    u8"r31", u8"r31b", u8"r31d", u8"r31w",
    u8"r8", u8"r8b", u8"r8d", u8"r8w",
    u8"r9", u8"r9b", u8"r9d", u8"r9w",
    
    u8"rax",
    u8"rbp",
    u8"rbx",
    u8"rcx",
    u8"rdi",
    u8"rdx",
    u8"rflags",
    u8"rip",
    u8"rsi",
    u8"rsp",

    u8"si",
    u8"sil",
    u8"sp",
    u8"spl",
    u8"ss",
    u8"st0", u8"st1", u8"st2", u8"st3", u8"st4", u8"st5", u8"st6", u8"st7",
    u8"sw",

    u8"xmm0", u8"xmm1", u8"xmm10", u8"xmm11", u8"xmm12", u8"xmm13", u8"xmm14", u8"xmm15",
    u8"xmm16", u8"xmm17", u8"xmm18", u8"xmm19", u8"xmm2", u8"xmm20", u8"xmm21", u8"xmm22",
    u8"xmm23", u8"xmm24", u8"xmm25", u8"xmm26", u8"xmm27", u8"xmm28", u8"xmm29", u8"xmm3",
    u8"xmm31", u8"xmm32", u8"xmm4", u8"xmm5", u8"xmm6", u8"xmm7", u8"xmm8", u8"xmm9",

    u8"ymm0", u8"ymm1", u8"ymm10", u8"ymm11", u8"ymm12", u8"ymm13", u8"ymm14", u8"ymm15",
    u8"ymm16", u8"ymm17", u8"ymm18", u8"ymm19", u8"ymm2", u8"ymm20", u8"ymm21", u8"ymm22",
    u8"ymm23", u8"ymm24", u8"ymm25", u8"ymm26", u8"ymm27", u8"ymm28", u8"ymm29", u8"ymm3",
    u8"ymm31", u8"ymm32", u8"ymm4", u8"ymm5", u8"ymm6", u8"ymm7", u8"ymm8", u8"ymm9",

    u8"zmm0", u8"zmm1", u8"zmm10", u8"zmm11", u8"zmm12", u8"zmm13", u8"zmm14", u8"zmm15",
    u8"zmm16", u8"zmm17", u8"zmm18", u8"zmm19", u8"zmm2", u8"zmm20", u8"zmm21", u8"zmm22",
    u8"zmm23", u8"zmm24", u8"zmm25", u8"zmm26", u8"zmm27", u8"zmm28", u8"zmm29", u8"zmm3",
    u8"zmm31", u8"zmm32", u8"zmm4", u8"zmm5", u8"zmm6", u8"zmm7", u8"zmm8", u8"zmm9",
};
// clang-format on

static_assert(std::ranges::is_sorted(registers));

constexpr std::u8string_view label_instructions[] {
    u8"call",

    u8"ja",   u8"jae",   u8"jb",     u8"jbe",    u8"jc",    u8"je",   u8"jg",  u8"jge",
    u8"jl",   u8"jle",   u8"jmp",    u8"jna",    u8"jna",   u8"jnae", u8"jnb", u8"jnbe",
    u8"jnc",  u8"jne",   u8"jng",    u8"jnge",   u8"jnl",   u8"jnle", u8"jno", u8"jnp",
    u8"jnz",  u8"jo",    u8"jp",     u8"jpe",    u8"jpo",   u8"js",   u8"jz",

    u8"loop", u8"loope", u8"loopne", u8"loopnz", u8"loopz",
};

static_assert(std::ranges::is_sorted(label_instructions));

[[nodiscard]]
constexpr bool binary_search_case_insensitive(
    std::span<const std::u8string_view> haystack,
    std::u8string_view needle
)
{
    constexpr auto case_insensitive_less = [](std::u8string_view x, std::u8string_view y) {
        return ascii::compare_to_lower(x, y) < 0;
    };
    return std::ranges::binary_search(haystack, needle, case_insensitive_less);
}

[[nodiscard]]
constexpr Base_Suffix determine_suffix(std::u8string_view str)
{
    if (str.empty()) {
        return {};
    }
    const int base = base_of_suffix_char(str.back());
    if (base <= 0) {
        return {};
    }
    return { .length = 1, .base = base };
}

} // namespace

[[nodiscard]]
bool is_pseudo_instruction(std::u8string_view name) noexcept
{
    return binary_search_case_insensitive(pseudo_instructions, name);
}

[[nodiscard]]
bool is_type(std::u8string_view name) noexcept
{
    return binary_search_case_insensitive(types, name);
}

[[nodiscard]]
bool is_operator_keyword(std::u8string_view name) noexcept
{
    return binary_search_case_insensitive(operator_keywords, name);
}

[[nodiscard]]
bool is_register(std::u8string_view name) noexcept
{
    return binary_search_case_insensitive(registers, name);
}

[[nodiscard]]
bool is_label_instruction(std::u8string_view name) noexcept
{
    return binary_search_case_insensitive(label_instructions, name);
}

namespace {

constexpr char8_t digit_separator = u8'_';

struct Highlighter : Highlighter_Base {
private:
    /// @brief A fallback highlight for identifiers when we cannot otherwise tell
    /// how an identifier should be highlighted.
    Highlight_Type id_highlight = Highlight_Type::name_instruction;

public:
    Highlighter(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        std::pmr::memory_resource* memory,
        const Highlight_Options& options
    )
        : Highlighter_Base { out, source, memory, options }
    {
    }

    bool operator()()
    {
        while (!eof()) {
            consume_anything();
        }
        return true;
    }

private:
    void consume_anything()
    {

        switch (const char8_t c = remainder[0]) {
        case u8' ':
        case u8'\t': {
            advance(1);
            break;
        }
        case u8'\r':
        case u8'\n': {
            id_highlight = Highlight_Type::name_instruction;
            advance(1);
            break;
        }

        case u8'0':
        case u8'1':
        case u8'2':
        case u8'3':
        case u8'4':
        case u8'5':
        case u8'6':
        case u8'7':
        case u8'8':
        case u8'9': {
            const bool success = expect_number();
            ULIGHT_ASSERT(success);
            break;
        }
        case u8'"':
        case u8'\'':
        case u8'`': {
            consume_string(c);
            break;
        }
        case u8'(':
        case u8')': {
            emit_and_advance(1, Highlight_Type::symbol_parens);
            break;
        }
        case u8'[':
        case u8']': {
            emit_and_advance(1, Highlight_Type::symbol_square);
            break;
        }
        case u8'{':
        case u8'}': {
            emit_and_advance(1, Highlight_Type::symbol_brace);
            break;
        }
        case u8',': {
            emit_and_advance(1, Highlight_Type::symbol_punc);
            break;
        }
        case u8';': {
            consume_comment();
            break;
        }
        case u8'%': {
            consume_macro();
            break;
        }

        default: {
            if (const std::size_t op_length = match_operator(remainder)) {
                emit_and_advance(op_length, Highlight_Type::symbol_op);
                break;
            }
            if (expect_number()) {
                break;
            }
            if (is_nasm_identifier_start(c)) {
                consume_identifier();
                break;
            }
            emit_and_advance(1, Highlight_Type::error, Coalescing::forced);
            break;
        }
        }
    }

    void consume_comment()
    {
        // https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html#section-3.1
        ULIGHT_ASSERT(remainder.starts_with(u8';'));
        emit_and_advance(1, Highlight_Type::comment_delim);
        const Line_Result line = match_crlf_line(remainder);
        if (line.content_length != 0) {
            emit_and_advance(line.content_length, Highlight_Type::comment);
        }
        advance(line.terminator_length);
        id_highlight = Highlight_Type::name_instruction;
    }

    void consume_macro()
    {
        // https://www.nasm.us/xdoc/2.16.03/html/nasmdoc4.html
        ULIGHT_ASSERT(remainder.starts_with(u8'%'));
        const Line_Result line = match_crlf_line(remainder.substr(1));
        emit_and_advance(line.content_length + 1, Highlight_Type::name_macro);
        advance(line.terminator_length);
        id_highlight = Highlight_Type::name_instruction;
    }

    void consume_identifier()
    {
        const std::size_t length = match_identifier(remainder);
        ULIGHT_ASSUME(length > 0);
        const std::u8string_view identifier = remainder.substr(0, length);
        if (ascii::equals_ignore_case(identifier, u8"seg")
            || ascii::equals_ignore_case(identifier, u8"wrt")) {
            emit_and_advance(length, Highlight_Type::keyword_op);
            return;
        }
        if (length < remainder.length() && remainder[length] == u8':') {
            emit_and_advance(length + 1, Highlight_Type::name_label_decl);
            id_highlight = Highlight_Type::name_instruction;
            return;
        }
        if (remainder.starts_with(u8'.')) {
            emit_and_advance(length, Highlight_Type::name_label_decl);
            id_highlight = Highlight_Type::name_instruction;
            return;
        }
        if (is_type(identifier)) {
            emit_and_advance(length, Highlight_Type::keyword_type);
            id_highlight = Highlight_Type::name_var;
            return;
        }
        if (is_operator_keyword(identifier)) {
            emit_and_advance(length, Highlight_Type::keyword_op);
            id_highlight = Highlight_Type::name_var;
            return;
        }
        if (is_register(identifier)) {
            emit_and_advance(length, Highlight_Type::name_var);
            id_highlight = Highlight_Type::name_var;
            return;
        }
        if (is_label_instruction(identifier)) {
            emit_and_advance(length, Highlight_Type::name_instruction);
            id_highlight = Highlight_Type::name_label;
            return;
        }
        if (identifier.starts_with(u8'$')) {
            emit_and_advance(length, Highlight_Type::name);
            id_highlight = Highlight_Type::name_var;
            return;
        }
        emit_and_advance(length, id_highlight);
        if (id_highlight == Highlight_Type::name_instruction) {
            id_highlight = Highlight_Type::name_var;
        }
    }

    bool expect_number()
    {
        // https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html#section-3.4.1
        return expect_suffixed_number() || expect_common_number();
    }

    bool expect_common_number()
    {
        // https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html#section-3.4.1
        static constexpr Number_Prefix prefixes[] {
            { u8"0b", 2 },  { u8"0B", 2 },  { u8"0y", 2 },  { u8"0Y", 2 }, //
            { u8"0o", 8 },  { u8"0O", 8 },  { u8"0q", 8 },  { u8"0Q", 8 }, //
            { u8"0d", 10 }, { u8"0D", 10 }, { u8"0t", 10 }, { u8"0T", 10 }, //
            { u8"0x", 16 }, { u8"0X", 16 }, { u8"0h", 16 }, { u8"0H", 16 }, { u8"$", 16 }, //
        };
        static constexpr Exponent_Separator exponent_separators[] {
            { u8"e", 10 }, { u8"e+", 10 }, { u8"e-", 10 }, //
            { u8"E", 10 }, { u8"E+", 10 }, { u8"E-", 10 }, //
            { u8"p", 16 }, { u8"p+", 16 }, { u8"p-", 16 }, //
            { u8"P", 16 }, { u8"P+", 16 }, { u8"P", 16 }, //
        };
        static constexpr Common_Number_Options options //
            { .prefixes = prefixes,
              .exponent_separators = exponent_separators,
              .digit_separator = digit_separator };
        const Common_Number_Result result = match_common_number(remainder, options);
        if (!result) {
            return false;
        }
        highlight_number(result, digit_separator);
        return true;
    }

    bool expect_suffixed_number()
    {
        // https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html#section-3.4.1
        const Suffix_Number_Result suffixed
            = match_suffix_number(remainder, Constant<&determine_suffix> {}, digit_separator);
        if (!suffixed) {
            return false;
        }
        if (suffixed.base == 16) {
            // In the case of hex numbers, there are actually some insane ambiguities.
            // For example, "eax" could technically be parsed as hexadecimal digits
            // "ea" followed by the suffix "x".
            // The only thing we can really do in this case is detect special cases that shouldn't
            // be parsed as numbers.
            const std::u8string_view number = remainder.substr(0, suffixed.digits + 1);
            if (is_pseudo_instruction(number)) {
                emit_and_advance(number.length(), Highlight_Type::name_instruction_pseudo);
                return true;
            }
            if (is_register(number)) {
                emit_and_advance(number.length(), Highlight_Type::name_var);
                return true;
            }
        }
        if (suffixed.erroneous) {
            emit_and_advance(suffixed.digits + suffixed.suffix, Highlight_Type::error);
            return true;
        }
        highlight_digits(remainder.substr(0, suffixed.digits), digit_separator);
        emit_and_advance(suffixed.suffix, Highlight_Type::number_decor);
        return true;
    }

    void consume_string(char8_t quote_char)
    {
        // https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html#section-3.4.2
        ULIGHT_ASSERT(remainder.starts_with(quote_char));
        emit_and_advance(1, Highlight_Type::string_delim);

        if (quote_char == u8'\'' || quote_char == u8'"') {
            consume_verbatim_string_content(quote_char);
        }
        else {
            consume_backquote_string_content();
        }
    }

    void consume_verbatim_string_content(char8_t quote_char)
    {
        // https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html#section-3.4.2
        const auto is_string_end
            = [quote_char](char8_t c) { return c == u8'\n' || c == u8'\r' || c == quote_char; };
        if (const std::size_t length = ascii::length_if_not(remainder, is_string_end)) {
            emit_and_advance(length, Highlight_Type::string);
        }
        if (remainder.starts_with(quote_char)) {
            emit_and_advance(1, Highlight_Type::string_delim);
        }
    }

    void consume_backquote_string_content()
    {
        // https://www.nasm.us/xdoc/2.16.03/html/nasmdoc3.html#section-3.4.2

        std::size_t length = 0;
        const auto flush = [&] {
            if (length != 0) {
                emit_and_advance(length, Highlight_Type::string);
                length = 0;
            }
        };

        while (length < remainder.length()) {
            switch (remainder[length]) {
            case u8'\n':
            case u8'\r':
            case u8'\v': {
                flush();
                return;
            }
            case u8'`': {
                flush();
                emit_and_advance(1, Highlight_Type::string_delim);
                return;
            }
            case u8'\\': {
                flush();
                if (!expect_escape()) {
                    emit_and_advance(1, Highlight_Type::error);
                }
                break;
            }
            default: {
                ++length;
                break;
            }
            }
        }
    }

    bool expect_escape()
    {
        if (const Escape_Result escape = match_escape_sequence(remainder)) {
            const auto highlight
                = escape.erroneous ? Highlight_Type::error : Highlight_Type::string_escape;
            emit_and_advance(escape.length, highlight);
            return true;
        }
        return false;
    }
};

} // namespace

} // namespace nasm

bool highlight_nasm(
    Non_Owning_Buffer<Token>& out,
    std::u8string_view source,
    std::pmr::memory_resource* memory,
    const Highlight_Options& options
)
{
    return nasm::Highlighter { out, source, memory, options }();
}

} // namespace ulight
