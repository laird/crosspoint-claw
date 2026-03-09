#pragma once
#include <string>
#include <cstdint>

/// Pixel indent applied to bullets, numbered lists, and blockquotes.
constexpr int MD_BULLET_INDENT = 20;

/// Text style flags — values match EpdFontFamily::Style so callers can cast directly.
enum class MdStyle : uint8_t {
  REGULAR    = 0,
  BOLD       = 1,
  ITALIC     = 2,
  BOLD_ITALIC = 3,
};

/// A single rendered line produced by the Markdown parser.
struct MdLine {
  std::string text;
  MdStyle style = MdStyle::REGULAR;
  int indentPixels = 0;  ///< Left indent for bullets, numbered lists, blockquotes.
  bool isHRule = false;  ///< Draw a horizontal rule instead of text.
};

namespace MdParser {

/// Remove leading/trailing spaces and tabs.
std::string trimWhitespace(const std::string& s);

/// Return true if the trimmed line is a Markdown horizontal rule (3+ of -, *, or _).
bool isHorizontalRule(const std::string& trimmed);

/// Strip inline Markdown markers from a string, keeping visible text only.
/// - Images: ![alt](url)  → [image: alt]
/// - Links:  [text](url)  → text
/// - Bold/italic/code spans stripped, content kept.
std::string stripInlineMarkdown(const std::string& s);

/// Parse a single raw line into an MdLine. Updates inCodeFence state.
/// Pass stripInline=false during page index building to skip the string
/// allocation overhead of stripInlineMarkdown (page breaks stay conservative).
MdLine parseMdLine(const std::string& raw, bool& inCodeFence, bool stripInline = true);

}  // namespace MdParser
