module.exports = grammar({
  name: "lispex",

  word: ($) => $.symbol,

  externals: ($) => [
    $.block_comment,
    $.character,
    $._malformed_character,
    $.character_end,
    $.boolean,
    $.byte,
    $.integer,
    $.rational,
    $.real,
    $.symbol,
    $.forbidden_reader_extension,
  ],

  extras: ($) => [
    /[\u0009-\u000D\u0020\u0085\u00A0\u1680\u2000-\u200A\u2028\u2029\u202F\u205F\u3000]/,
    $.line_comment,
    $.block_comment,
  ],

  rules: {
    source_file: ($) =>
      repeat(
        choice(
          $.datum,
          $.forbidden_reader_extension,
        ),
      ),

    datum: ($) =>
      choice(
        $.quote,
        $.quasiquote,
        $.unquote,
        $.unquote_splicing,
        $.list,
        $.dotted_list,
        $.vector,
        $.bytevector,
        $.boolean,
        $.character,
        // Never emitted: recovery marks the whole malformed token as rejected.
        seq(alias($._malformed_character, $.character), $.character_end),
        $.string,
        $.rational,
        $.real,
        $.integer,
        $.symbol,
      ),

    quote: ($) => seq("'", field("datum", $.datum)),
    quasiquote: ($) => seq("`", field("datum", $.datum)),
    unquote: ($) => seq(",", field("datum", $.datum)),
    unquote_splicing: ($) => seq(",@", field("datum", $.datum)),

    list: ($) => seq("(", repeat($.datum), ")"),
    dotted_list: ($) =>
      seq(
        "(",
        field("head", repeat1($.datum)),
        ".",
        field("tail", $.datum),
        ")",
      ),

    vector: ($) => seq("#(", repeat($.datum), ")"),
    bytevector: ($) => seq("#u8(", repeat($.byte), ")"),

    string: (_) =>
      token(
        prec(
          4,
          /"(?:[^"\\]|\\["\\ntr]|\\[xX][0-9A-Fa-f]+;)*"/,
        ),
      ),

    line_comment: (_) => token(prec(1, /;[^\n]*/)),
  },
});
