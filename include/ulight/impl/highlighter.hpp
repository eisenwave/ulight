#ifndef ULIGHT_HIGHLIGHTER_HPP
#define ULIGHT_HIGHLIGHTER_HPP

#include <cstddef>
#include <string_view>

#include "ulight/impl/assert.hpp"
#include "ulight/impl/buffer.hpp"
#include "ulight/impl/highlight.hpp"
#include "ulight/impl/numbers.hpp"
#include "ulight/impl/parse_utils.hpp"

namespace ulight {

enum struct Coalescing : bool {
    /// @brief Coalesces highlight tokens as usual.
    normal,
    /// @brief Forces coalescing of tokens.
    /// In particular, this may be used to coalesce `@` and the subsequent
    /// identifier into a single at-rule.
    /// It may also be used to coalesce sequences of `.`, `:`, identifiers,
    /// etc. into a single highlight token which forms a "selector".
    forced
};

/// @brief A skeleton implementation for the language-specific highlighters.
struct [[nodiscard]] Highlighter_Base {
protected:
    Non_Owning_Buffer<Token>& out;
    std::u8string_view remainder;
    std::pmr::memory_resource* const memory;
    const Highlight_Options options;
    const std::size_t source_length;

    std::size_t index = 0;

public:
    Highlighter_Base(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        std::pmr::memory_resource* memory,
        const Highlight_Options& options
    )
        : out { out }
        , remainder { source }
        , memory { memory }
        , options { options }
        , source_length { source.length() }
    {
    }

    Highlighter_Base(
        Non_Owning_Buffer<Token>& out,
        std::u8string_view source,
        const Highlight_Options& options
    )
        : Highlighter_Base { out, source, nullptr, options }
    {
    }

protected:
    /// @brief Equivalent to `remainder.empty()`.
    [[nodiscard]]
    bool eof() const
    {
        return remainder.empty();
    }

    /// @brief Emits a single token into the output buffer,
    /// or coalesces it into the most recent token.
    /// @param begin The start index within the source file of the token.
    /// @param length The length of the token.
    /// @param type The type of token.
    /// @param coalescing The policy on coalescing tokens.
    void emit(
        std::size_t begin,
        std::size_t length,
        Highlight_Type type,
        Coalescing coalescing = Coalescing::normal
    )
    {
        ULIGHT_DEBUG_ASSERT(length != 0);
        ULIGHT_DEBUG_ASSERT(begin < source_length);
        ULIGHT_DEBUG_ASSERT(begin + length <= source_length);

        const bool coalesce = (coalescing == Coalescing::forced || options.coalescing) //
            && !out.empty() //
            && Highlight_Type(out.back().type) == type //
            && out.back().begin + out.back().length == begin;
        if (coalesce) {
            out.back().length += length;
        }
        else {
            out.emplace_back(begin, length, Underlying(type));
        }
    }

    /// @brief Advances the `index` within the source file without emitting any token.
    /// This is typically used for code spans with no highlight,
    /// like plain text within markup languages.
    /// @param length The length in code units to advance by.
    void advance(std::size_t length)
    {
        ULIGHT_DEBUG_ASSERT(length <= remainder.length());

        index += length;
        remainder.remove_prefix(length);
    }

    /// @brief Equivalent to `emit(index, length, type, coalescing)`
    /// followed by `advance(length)`.
    void emit_and_advance(
        std::size_t length,
        Highlight_Type type,
        Coalescing coalescing = Coalescing::normal
    )
    {
        emit(index, length, type, coalescing);
        advance(length);
    }

    /// @brief Consumes a span of code in a language of choice,
    /// and appends the resulting tokens to this highlighter.
    /// @param lang The nested language to highlight.
    /// @param length The length of the nested language span, in code units.
    /// @param nested_tokens The backing buffer of nested tokens.
    /// @returns The status resulting from nested highlighting.
    [[nodiscard]]
    Status consume_nested_language(Lang lang, std::size_t length, std::span<Token> nested_tokens)
    {
        ULIGHT_ASSERT(lang != Lang::none);
        if (length == 0) {
            return Status::ok;
        }
        Non_Owning_Buffer<Token> sub = sub_buffer(nested_tokens);
        const std::u8string_view nested_source = remainder.substr(0, length);

        const Status result = highlight(sub, nested_source, lang, memory, options);
        if (result != Status::ok) {
            return result;
        }
        sub.flush();
        advance(length);
        return Status::ok;
    }

    /// @brief Creates a sub-buffer from `out`.
    /// Upon flushing that buffer,
    /// the start positions of the tokens are offset by `self.index`.
    /// @param data The backing buffer of tokens used by the sub-buffer.
    [[nodiscard]]
    Non_Owning_Buffer<Token> sub_buffer(std::span<Token> data)
    {
        constexpr auto flush = +[](const void* this_pointer, Token* tokens, std::size_t amount) {
            const auto& self = *static_cast<const Highlighter_Base*>(this_pointer);
            for (std::size_t i = 0; i < amount; ++i) {
                tokens[i].begin += self.index;
            }
            self.out.append_range(std::span<const Token> { tokens, amount });
        };
        return { data.data(), data.size(), this, flush };
    }

private:
    template <void (Highlighter_Base::*highlight_digits_fn)(std::u8string_view, char8_t)>
    void highlight_number_impl(const Common_Number_Result& result, char8_t digit_separator)
    {
        if (result.erroneous) {
            emit_and_advance(result.length, Highlight_Type::error);
            return;
        }

        if (result.sign + result.prefix) {
            emit_and_advance(result.sign + result.prefix, Highlight_Type::number_decor);
        }
        if (result.integer) {
            (this->*highlight_digits_fn)(remainder.substr(0, result.integer), digit_separator);
        }
        if (result.radix_point) {
            emit_and_advance(result.radix_point, Highlight_Type::number_delim);
        }
        if (result.fractional) {
            (this->*highlight_digits_fn)(remainder.substr(0, result.fractional), digit_separator);
        }
        if (result.exponent_sep) {
            emit_and_advance(result.exponent_sep, Highlight_Type::number_delim);
        }
        if (result.exponent_digits) {
            (this->*highlight_digits_fn)(
                remainder.substr(0, result.exponent_digits), digit_separator
            );
        }
        if (result.suffix) {
            emit_and_advance(result.suffix, Highlight_Type::number_decor);
        }
    }

    void highlight_digits_ignore_separator(std::u8string_view digits, char8_t)
    {
        if (!digits.empty()) {
            emit_and_advance(digits.length(), Highlight_Type::number);
        }
    }

public:
    void highlight_number(const Common_Number_Result& result, char8_t digit_separator)
    {
        highlight_number_impl<&Highlighter_Base::highlight_digits>(result, digit_separator);
    }

    void highlight_number(const Common_Number_Result& result)
    {
        highlight_number_impl<&Highlighter_Base::highlight_digits_ignore_separator>(result, {});
    }

    void highlight_digits(std::u8string_view digits, char8_t separator)
    {
        ULIGHT_ASSERT(separator != char8_t {});
        ULIGHT_ASSERT(digits.length() <= remainder.length());

        std::size_t length = 0;
        const auto flush = [&] {
            if (length != 0) {
                emit_and_advance(length, Highlight_Type::number);
                digits.remove_prefix(length);
                length = 0;
            }
        };

        while (length < digits.length()) {
            if (digits[length] == separator) {
                flush();
                digits.remove_prefix(1);
                emit_and_advance(1, Highlight_Type::number_delim);
            }
            else {
                ++length;
            }
        }
        flush();
    }

    void highlight_enclosed(
        const Enclosed_Result& enclosed,
        std::size_t prefix_length,
        std::size_t suffix_length,
        Highlight_Type content_highlight,
        Highlight_Type delimiter_highlight
    )
    {
        ULIGHT_DEBUG_ASSERT(enclosed);
        ULIGHT_DEBUG_ASSERT(prefix_length <= enclosed.length);
        ULIGHT_DEBUG_ASSERT(
            !enclosed.is_terminated || prefix_length + suffix_length <= enclosed.length
        );

        emit(index, prefix_length, delimiter_highlight);
        if (enclosed.is_terminated) {
            const std::size_t content_length = enclosed.length - prefix_length - suffix_length;
            if (content_length) {
                emit(index + prefix_length, content_length, content_highlight);
            }
            emit(index + prefix_length + content_length, suffix_length, delimiter_highlight);
        }
        else {
            const std::size_t content_length = enclosed.length - prefix_length;
            if (content_length) {
                emit(index + prefix_length, content_length, content_highlight);
            }
        }
        advance(enclosed.length);
    }

    void highlight_enclosed_comment(
        const Enclosed_Result& enclosed,
        std::size_t prefix_length,
        std::size_t suffix_length
    )
    {
        highlight_enclosed(
            enclosed, prefix_length, suffix_length, Highlight_Type::comment,
            Highlight_Type::comment_delim
        );
    }
};

} // namespace ulight

#endif
