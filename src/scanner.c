#include "tree_sitter/parser.h"
#include "unicode.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum TokenType {
  BLOCK_COMMENT,
  CHARACTER,
  MALFORMED_CHARACTER,
  CHARACTER_END,
  BOOLEAN,
  BYTE,
  INTEGER,
  RATIONAL,
  REAL,
  SYMBOL,
  FORBIDDEN_READER_EXTENSION,
};

void *tree_sitter_lispex_external_scanner_create(void) { return NULL; }
void tree_sitter_lispex_external_scanner_destroy(void *payload) {
  (void)payload;
}
unsigned tree_sitter_lispex_external_scanner_serialize(void *payload,
                                                       char *buffer) {
  (void)payload;
  (void)buffer;
  return 0;
}
void tree_sitter_lispex_external_scanner_deserialize(void *payload,
                                                     const char *buffer,
                                                     unsigned length) {
  (void)payload;
  (void)buffer;
  (void)length;
}

static bool is_space(int32_t c) {
  // Rust char::is_whitespace (Unicode White_Space); U+FEFF is a constituent.
  return (c >= 0x09 && c <= 0x0D) || c == 0x20 || c == 0x85 ||
         c == 0xA0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200A) ||
         c == 0x2028 || c == 0x2029 || c == 0x202F || c == 0x205F ||
         c == 0x3000;
}

static bool is_delimiter(int32_t c) {
  return is_space(c) || c == '(' || c == ')' || c == '[' ||
         c == ']' || c == '{' || c == '}' || c == '"' || c == ';' ||
         c == '\'' || c == '`' || c == ',';
}

static bool is_hex(int32_t c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

static bool equals_literal(const char *text, size_t length, const char *literal) {
  size_t literal_length = strlen(literal);
  return length == literal_length && memcmp(text, literal, length) == 0;
}

static bool is_named_character(const char *name, size_t length) {
  static const char *const names[] = {
      "space",  "newline", "linefeed", "tab",    "return",
      "null",   "nul",     "delete",   "rubout", "escape",
      "esc",    "backspace", "alarm",  "page",
  };
  const size_t count = sizeof(names) / sizeof(names[0]);
  for (size_t index = 0; index < count; index++) {
    if (equals_literal(name, length, names[index])) {
      return true;
    }
  }
  return false;
}

static bool scan_block_comment_after_hash(TSLexer *lexer) {
  lexer->advance(lexer, false);
  unsigned depth = 1;
  while (depth > 0) {
    if (lexer->eof(lexer)) {
      return false;
    }
    if (lexer->lookahead == '#') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '|') {
        lexer->advance(lexer, false);
        depth++;
      }
      continue;
    }
    if (lexer->lookahead == '|') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '#') {
        lexer->advance(lexer, false);
        depth--;
      }
      continue;
    }
    lexer->advance(lexer, false);
  }
  lexer->result_symbol = BLOCK_COMMENT;
  return true;
}

static bool scan_character_after_hash(TSLexer *lexer) {
  lexer->advance(lexer, false);
  if (lexer->eof(lexer)) {
    return false;
  }

  int32_t first = lexer->lookahead;
  lexer->advance(lexer, false);
  if (!is_unicode_alphanumeric(first)) {
    lexer->result_symbol = CHARACTER;
    return true;
  }

  char name[64];
  size_t length = 0;
  bool ascii_only = first < 128;
  bool hex = first == 'x' || first == 'X';
  bool has_tail = false;
  name[length++] = ascii_only ? (char)first : '\0';
  while (!lexer->eof(lexer) && !is_delimiter(lexer->lookahead) &&
         lexer->lookahead != '|') {
    has_tail = true;
    ascii_only = ascii_only && lexer->lookahead < 128;
    hex = hex && is_hex(lexer->lookahead);
    // Only named forms need buffering. Consume even arbitrarily long malformed
    // names and hexadecimal spellings as one token, without truncating it.
    if (length < sizeof(name)) {
      name[length++] = lexer->lookahead < 128 ? (char)lexer->lookahead : '\0';
    }
    lexer->advance(lexer, false);
  }
  bool valid = !has_tail || hex ||
               (ascii_only && is_named_character(name, length));
  lexer->result_symbol = valid ? CHARACTER : MALFORMED_CHARACTER;
  return true;
}

static char *scan_ascii_token(TSLexer *lexer, bool *ascii_only,
                              size_t *length_out) {
  size_t capacity = 64;
  size_t length = 0;
  char *buffer = malloc(capacity);
  if (buffer == NULL) {
    return NULL;
  }
  *ascii_only = true;
  while (!lexer->eof(lexer) && !is_delimiter(lexer->lookahead)) {
    if (lexer->lookahead >= 128) {
      *ascii_only = false;
    }
    if (length + 1 >= capacity) {
      if (capacity > SIZE_MAX / 2) {
        free(buffer);
        return NULL;
      }
      capacity *= 2;
      char *grown = realloc(buffer, capacity);
      if (grown == NULL) {
        free(buffer);
        return NULL;
      }
      buffer = grown;
    }
    buffer[length++] =
        lexer->lookahead < 128 ? (char)lexer->lookahead : '\0';
    lexer->advance(lexer, false);
  }
  buffer[length] = '\0';
  *length_out = length;
  return buffer;
}

static bool unsigned_integer_shape(const char *text, size_t length) {
  if (length == 0) {
    return false;
  }
  if (text[0] == '0') {
    return length == 1;
  }
  if (text[0] < '1' || text[0] > '9') {
    return false;
  }
  for (size_t index = 1; index < length; index++) {
    if (text[index] < '0' || text[index] > '9') {
      return false;
    }
  }
  return true;
}

static bool integer_shape(const char *text, size_t length) {
  return length > 0 && text[0] == '-'
             ? unsigned_integer_shape(text + 1, length - 1)
             : unsigned_integer_shape(text, length);
}

static bool rational_shape(const char *text, size_t length) {
  const char *slash = memchr(text, '/', length);
  if (slash == NULL) {
    return false;
  }
  size_t numerator_length = (size_t)(slash - text);
  size_t denominator_length = length - numerator_length - 1;
  const char *denominator = slash + 1;
  if (!integer_shape(text, numerator_length) || denominator_length == 0 ||
      denominator[0] < '1' || denominator[0] > '9') {
    return false;
  }
  return unsigned_integer_shape(denominator, denominator_length);
}

static bool consume_digits(const char *text, size_t length, size_t *cursor) {
  size_t start = *cursor;
  while (*cursor < length && text[*cursor] >= '0' && text[*cursor] <= '9') {
    (*cursor)++;
  }
  return *cursor != start;
}

static bool real_shape(const char *text, size_t length) {
  size_t cursor = 0;
  if (cursor < length && text[cursor] == '-') {
    cursor++;
  }
  size_t whole = cursor;
  if (!consume_digits(text, length, &cursor)) {
    return false;
  }
  if (!unsigned_integer_shape(text + whole, cursor - whole)) {
    return false;
  }

  bool fraction = false;
  bool exponent = false;
  if (cursor < length && text[cursor] == '.') {
    fraction = true;
    cursor++;
    if (!consume_digits(text, length, &cursor)) {
      return false;
    }
  }
  if (cursor < length && (text[cursor] == 'e' || text[cursor] == 'E')) {
    exponent = true;
    cursor++;
    if (cursor < length && (text[cursor] == '+' || text[cursor] == '-')) {
      cursor++;
    }
    if (!consume_digits(text, length, &cursor)) {
      return false;
    }
  }
  return cursor == length && (fraction || exponent);
}

static bool byte_shape(const char *text, size_t length) {
  // The reader parses an integer token, so negative zero is also byte zero.
  if (equals_literal(text, length, "-0")) return true;
  if (!unsigned_integer_shape(text, length)) {
    return false;
  }
  unsigned value = 0;
  for (size_t index = 0; index < length; index++) {
    value = value * 10 + (unsigned)(text[index] - '0');
    if (value > 255) return false;
  }
  return value <= 255;
}

static bool starts_number_like(const char *text, size_t length) {
  return (length > 0 && text[0] >= '0' && text[0] <= '9') ||
         (length > 1 && text[0] == '-' && text[1] >= '0' && text[1] <= '9');
}

static bool scan_hash_token_after_hash(TSLexer *lexer,
                                       const bool *valid_symbols) {
  if (lexer->lookahead == ';' &&
      valid_symbols[FORBIDDEN_READER_EXTENSION]) {
    lexer->advance(lexer, false);
    lexer->result_symbol = FORBIDDEN_READER_EXTENSION;
    return true;
  }
  if (!valid_symbols[BOOLEAN] &&
      !valid_symbols[FORBIDDEN_READER_EXTENSION]) {
    return false;
  }

  bool ascii_only = true;
  size_t length = 0;
  char *tail = scan_ascii_token(lexer, &ascii_only, &length);
  if (tail == NULL) {
    return false;
  }
  bool matched = false;
  if (valid_symbols[BOOLEAN] && ascii_only &&
      (equals_literal(tail, length, "t") || equals_literal(tail, length, "true") ||
       equals_literal(tail, length, "f") || equals_literal(tail, length, "false"))) {
    lexer->result_symbol = BOOLEAN;
    matched = true;
  } else if (valid_symbols[FORBIDDEN_READER_EXTENSION] && ascii_only &&
             equals_literal(tail, length, "lang")) {
    lexer->result_symbol = FORBIDDEN_READER_EXTENSION;
    matched = true;
  }
  free(tail);
  return matched;
}

static bool scan_atom(TSLexer *lexer, const bool *valid_symbols) {
  bool ascii_only = true;
  size_t length = 0;
  char *text = scan_ascii_token(lexer, &ascii_only, &length);
  if (text == NULL || length == 0) {
    free(text);
    return false;
  }

  bool matched = false;
  if (valid_symbols[BYTE] && ascii_only && byte_shape(text, length)) {
    lexer->result_symbol = BYTE;
    matched = true;
  } else if (valid_symbols[RATIONAL] && ascii_only && rational_shape(text, length)) {
    lexer->result_symbol = RATIONAL;
    matched = true;
  } else if (valid_symbols[REAL] && ascii_only && real_shape(text, length)) {
    lexer->result_symbol = REAL;
    matched = true;
  } else if (valid_symbols[INTEGER] && ascii_only && integer_shape(text, length)) {
    lexer->result_symbol = INTEGER;
    matched = true;
  } else if (valid_symbols[SYMBOL] && text[0] != '#' &&
             !starts_number_like(text, length) &&
             !(ascii_only && equals_literal(text, length, "."))) {
    lexer->result_symbol = SYMBOL;
    matched = true;
  }
  free(text);
  return matched;
}

bool tree_sitter_lispex_external_scanner_scan(void *payload, TSLexer *lexer,
                                              const bool *valid_symbols) {
  (void)payload;
  while (is_space(lexer->lookahead)) {
    lexer->advance(lexer, true);
  }
  if (lexer->lookahead == '#') {
    lexer->advance(lexer, false);
    if (lexer->lookahead == '|' && valid_symbols[BLOCK_COMMENT]) {
      return scan_block_comment_after_hash(lexer);
    }
    if (lexer->lookahead == '\\' &&
        (valid_symbols[CHARACTER] || valid_symbols[MALFORMED_CHARACTER])) {
      return scan_character_after_hash(lexer);
    }
    return scan_hash_token_after_hash(lexer, valid_symbols);
  }
  return scan_atom(lexer, valid_symbols);
}
