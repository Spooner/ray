require 'yard'

class MatcherHandler < YARD::Handlers::Ruby::Base
  handles method_call(:describe_matcher)

  def process
    src = statement.parameters.children.first.source[1..-1]
    MethodObject.new(P("Ray::Matchers"), src) do |o|
      register(o)

      args = statement.last.first
      if args
        o.parameters = [args.source]
      end
    end
  end
end

class CustomCParser < YARD::Parser::CParser
  include YARD

  private
  def find_method_body(object, func_name, content = @content)
    case content
    when %r"((?>/\*.*?\*/\s*))(?:(?:static|SWIGINTERN)\s+)?(?:intern\s+)?
            VALUE\s+#{func_name}\s*(\([^)]*\))([^;]|$)"xm
      comment, params = $1, $2
      body_text = $&

      remove_private_comments(comment) if comment

      # see if we can find the whole body

      re = Regexp.escape(body_text.strip) + '[^(]*\{.*?^\}'
      if /#{re}/m =~ content
        body_text = $&
      end

      # The comment block may have been overridden with a 'Document-method'
      # block. This happens in the interpreter when multiple methods are
      # vectored through to the same C method but those methods are logically
      # distinct (for example Kernel.hash and Kernel.object_id share the same
      # implementation

      # override_comment = find_override_comment(object)
      # comment = override_comment if override_comment

      object.docstring = parse_comments(object, comment) if comment
      object.source = body_text
    when %r{((?>/\*.*?\*/\s*))^\s*\#\s*define\s+#{func_name}\s+(\w+)}m
      comment = $1
      find_method_body(object, $2, content)
    else
      # No body, but might still have an override comment
      # comment = find_override_comment(object)
      comment = nil
      object.docstring = parse_comments(object, comment) if comment
    end
  end
end

YARD::Parser::SourceParser.register_parser_type(:c, CustomCParser,
                                                ['c', 'cc', 'cxx', 'cpp'])
