/**
 *   Copyright (C) 2010, 2011 Object Modeling Designs
 *   Copyright (c) 2010 Joel de Guzman
 */

#if !defined(OMD_COMMON_SCALAR_DEF_HPP)
#define OMD_COMMON_SCALAR_DEF_HPP

#include "scalar.hpp"

#include <boost/cstdint.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/regex/pending/unicode_iterator.hpp>

namespace omd { namespace parser
{
    namespace detail
    {
        struct push_utf8
        {
            template <typename S, typename C>
            struct result { typedef void type; };

            void operator()(std::string& utf8, uchar code_point) const
            {
                typedef std::back_insert_iterator<std::string> insert_iter;
                insert_iter out_iter(utf8);
                boost::utf8_output_iterator<insert_iter> utf8_iter(out_iter);
                *utf8_iter++ = code_point;
            }
        };

        struct push_esc
        {
            template <typename S, typename C>
            struct result { typedef void type; };

            void operator()(std::string& utf8, uchar c) const
            {
                switch (c)
                {
                    case 'b': utf8 += '\b';     break;
                    case 't': utf8 += '\t';     break;
                    case 'n': utf8 += '\n';     break;
                    case 'f': utf8 += '\f';     break;
                    case 'r': utf8 += '\r';     break;
                    case '"': utf8 += '"';      break;
                    case '/': utf8 += '/';      break;
                    case '\\': utf8 += '\\';    break;
                }
            }
        };

        //~ struct fold_plain_string
        //~ {
            //~ template <typename String, typename Range>
            //~ struct result { typedef void type; };

            //~ template <typename String, typename Range>
            //~ void operator()(String& utf8, Range const& rng) const
            //~ {
                //~ BOOST_FOREACH(typename Range::value_type c, rng)
                //~ {
                //~ }
            //~ }
        //~ };
    }

    template <typename Iterator>
    unicode_string<Iterator>::unicode_string()
      : unicode_string::base_type(unicode_start),
        indent(0)
    {
        qi::char_type char_;
        qi::_val_type _val;
        qi::_r1_type _r1;
        qi::_1_type _1;
        qi::_2_type _2;
        qi::lit_type lit;
        qi::blank_type blank;
        qi::eol_type eol;
        qi::repeat_type repeat;
        qi::inf_type inf;

        using boost::spirit::qi::uint_parser;
        using boost::phoenix::function;
        using boost::phoenix::ref;

        uint_parser<uchar, 16, 4, 4> hex4;
        uint_parser<uchar, 16, 8, 8> hex8;
        function<detail::push_utf8> push_utf8;
        function<detail::push_esc> push_esc;

        char_esc =
            '\\'
            > (   ('u' > hex4)                    [push_utf8(_r1, _1)]
              |   ('U' > hex8)                    [push_utf8(_r1, _1)]
              |   char_("btnfr/\\\"'")            [push_esc(_r1, _1)]
              )
            ;

        double_quoted =
              '"'
            > *(char_esc(_val) | (~char_('"'))    [_val += _1])
            > '"'
            ;

        single_quoted =
              '\''
            > *(
                  lit("''")                       [_val += '\'']
              |   (~char_('\''))                  [_val += _1]
              )
            > '\''
            ;

        auto space = blank | (eol >> repeat(ref(indent)+1, inf)[blank]);
        auto safe_first = ~char_(unsafe_first);
        auto safe_plain = ~char_(unsafe_plain);

        unquoted =
                safe_first[_val = _1]
            >>  *(
                        (+space >> safe_plain)    [_val += ' ', _val += _2]
                    |   safe_plain                [_val += _1]
                )
            ;

        unicode_start =
              double_quoted
            | single_quoted
            | unquoted
            ;

        BOOST_SPIRIT_DEBUG_NODES(
            (char_esc)
            (single_quoted)
            (double_quoted)
            (unquoted)
            (unicode_start)
        );
    }

    template <typename Iterator>
    scalar<Iterator>::scalar()
      : scalar::base_type(scalar_value)
    {
        qi::lit_type lit;
        qi::char_type char_;
        qi::hex_type hex;
        qi::oct_type oct;
        qi::no_case_type no_case;
        qi::int_type int_;
        qi::attr_type attr;

        qi::real_parser<double, qi::strict_real_policies<double> > double_value;

        scalar_value =
              double_value
            | integer_value
            | no_case[bool_value]
            | no_case[null_value]
            | string_value
            ;

        integer_value =
              no_case["0x"] > hex
            | '0' >> oct
            | int_
            ;

        bool_value.add
            ("true", true)
            ("false", false)
            ("on", true)
            ("off", false)
            ("yes", true)
            ("no", false)
            ;

        null_value =
              (lit("null") | '~')
            >> attr(ast::null_t())
            ;

        BOOST_SPIRIT_DEBUG_NODES(
            (scalar_value)
            (integer_value)
            (null_value)
        );
    }
}}

#endif