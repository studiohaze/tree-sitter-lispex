(line_comment) @comment
(block_comment) @comment
(forbidden_reader_extension) @error

(boolean) @boolean
(integer) @number
(rational) @number
(real) @number.float
(character) @character
(string) @string
(byte) @number

[
  "'"
  "`"
  ","
  ",@"
  "."
] @operator

((symbol) @keyword
  (#match? @keyword "^(lambda|if|begin|set!|define|let|let\\*|letrec|quote|quasiquote|unquote|unquote-splicing|values|call-with-values|call/cc|dynamic-wind|cond|case|and|or|when|unless|do|guard|module|export|import)$"))

((list
  . [(line_comment) (block_comment)]* .
  (datum
    (symbol) @function.call)))

((dotted_list
  . [(line_comment) (block_comment)]* .
  (datum
    (symbol) @function.call)))

(symbol) @variable
