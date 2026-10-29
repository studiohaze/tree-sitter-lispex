(source_file) @local.scope

((list
  . [(line_comment) (block_comment)]* .
  (datum
    (symbol) @_define)
  . [(line_comment) (block_comment)]* .
  (datum
    (symbol) @local.definition))
  (#eq? @_define "define"))

((list
  . [(line_comment) (block_comment)]* .
  (datum (symbol) @_define)
  . [(line_comment) (block_comment)]* .
  (datum
    [(list . [(line_comment) (block_comment)]* . (datum (symbol) @local.definition))
     (dotted_list . [(line_comment) (block_comment)]* . (datum (symbol) @local.definition))]))
  (#eq? @_define "define"))

((list
  . [(line_comment) (block_comment)]* .
  (datum
    (symbol) @_lambda)
  . [(line_comment) (block_comment)]* .
  (datum
    (list
      (datum
        (symbol) @local.definition))))
  (#eq? @_lambda "lambda"))

(symbol) @local.reference
