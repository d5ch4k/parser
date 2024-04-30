/*=============================================================================
Copyright (c) 2001-2014 Joel de Guzman

Distributed under the Boost Software License, Version 1.0. (See accompanying
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  A Calculator example demonstrating the grammar and semantic actions
//  using lambda functions. The parser prints code and stack based calculations
//  are done in the semantic actions.
//
//  [ JDG May 10, 2002 ]        spirit 1
//  [ JDG March 4, 2007 ]       spirit 2
//  [ JDG February 21, 2011 ]   spirit 2.5
//  [ JDG June 6, 2014 ]        spirit x3 (from qi calc2, but using lambda functions)
//  [ MPo Oct 27, 2022 ]        boost::parser (from spirit x3)
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/parser/parser.hpp>

#include <iostream>
#include <algorithm>
#include <string>
#include <cstring>
#include <deque>
#include <sstream>

namespace bp = boost::parser;

namespace client {
///////////////////////////////////////////////////////////////////////////////
//  Semantic actions
///////////////////////////////////////////////////////////////////////////////
    namespace {

        std::deque<double> my_stack;

        auto do_int = [](auto & ctx) {
            std::cout << "push " << bp::_attr(ctx) << std::endl;
            my_stack.push_front(_attr(ctx));
        };
        auto const do_add = [] (auto & ctx) {
            std::cout << "add\n";
            // stack effect notation ( a, b -- c ) where c = a + b
            assert(my_stack.size() > 1);
            auto&& second = my_stack[0];
            auto&& first = my_stack[1];
            auto result = first + second;
            my_stack[1] = result;
            my_stack.pop_front();
        };
        auto const do_subt = [] (auto & ctx) {
            std::cout << "subtract\n";
            // stack effect notation ( a, b -- c ) where c = a - b
            assert(my_stack.size() > 1);
            auto&& second = my_stack[0];
            auto&& first = my_stack[1];
            auto result = first - second;
            my_stack[1] = result;
            my_stack.pop_front();
        };
        auto const do_mult = [] (auto & ctx) {
            std::cout << "mult\n";
            // stack effect notation ( a, b -- c ) where c = a * b
            assert(my_stack.size() > 1);
            auto second = my_stack[0];
            auto&& first = my_stack[1];
            auto&& result = first * second;
            my_stack[1] = result;
            my_stack.pop_front();

        };
        auto const do_div = [] (auto & ctx) {
            std::cout << "divide\n";
            // stack effect notation ( a, b -- c ) where c = a / b
            assert(my_stack.size() > 1);
            auto&& second = my_stack[0];
            auto&& first = my_stack[1];
            auto result = first / second;
            my_stack[1] = result;
            my_stack.pop_front();
        };
        auto const do_neg = [] (auto & ctx) {
            std::cout << "negate\n";
            // stack effect notation ( a -- -a )
            assert(my_stack.size() > 0);
            my_stack[0] *= -1;
        };
    }
///////////////////////////////////////////////////////////////////////////////
//  The calculator grammar
///////////////////////////////////////////////////////////////////////////////
    namespace calculator_grammar {
        bp::rule<class expression> const expression("expression");
        bp::rule<class term> const term("term");
        bp::rule<class factor> const factor("factor");


        auto const expression_def = term >> *(('+' >> term[do_add]) |
                                              ('-' >> term[do_subt]));

        auto const term_def = factor >> *(('*' >> factor[do_mult]) |
                                          ('/' >> factor[do_div]));

        auto const factor_def = bp::uint_[do_int] | '(' >> expression >> ')' |
                                ('-' >> factor[do_neg]) | ('+' >> factor);

        BOOST_PARSER_DEFINE_RULES(expression, term, factor);

        auto calculator = expression;
    }

    using calculator_grammar::calculator;

}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Expression parser...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Type an expression...or [q or Q] to quit\n\n";

    typedef std::string::const_iterator iterator_type;

    std::string str("(3+4)*5-2*3");
    std::cout << "As an example, the following input: \n" << str << "\nwill result in\n";
    auto& calc = client::calculator;    // Our grammar

    auto r = bp::parse(str, calc, bp::ws);

    if (r)
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        assert(client::my_stack.size() == 1);
        std::cout << str << " ==> " << client::my_stack[0] << std::endl;
        client::my_stack.pop_front();
        std::cout << "-------------------------\n";
    }
    else
    {
        iterator_type iter = str.begin();
        iterator_type end = str.end();
        std::cout << "-------------------------\n";
        std::cout << "Parsing failed\n";
        bp::prefix_parse(iter, end, calc, bp::ws);
        std::string rest(iter, end);
        std::cout << "stopped at: \"" << rest << "\"\n";
        std::cout << "-------------------------\n";
    }
    std::cout << "Type an expression...or [q or Q] to quit\n\n";

    while (std::getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        auto& calc = client::calculator;    // Our grammar

        auto r = bp::parse(str, calc, bp::ws);

        if (r)
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            assert(client::my_stack.size() == 1);
            std::cout << str << " ==> " << client::my_stack[0] << std::endl;
            client::my_stack.pop_front();
            std::cout << "-------------------------\n";
        }
        else
        {
            iterator_type iter = str.begin();
            iterator_type end = str.end();
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            bp::prefix_parse(iter, end, calc, bp::ws);
            std::string rest(iter, end);
            std::cout << "stopped at: \"" << rest << "\"\n";
            client::my_stack.clear();
        std::cout << "-------------------------\n";
        }
        std::cout << "Type an expression...or [q or Q] to quit\n\n";
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}
