/*=============================================================================
Copyright (c) 2001-2014 Joel de Guzman

Distributed under the Boost Software License, Version 1.0. (See accompanying
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  A Calculator example demonstrating the grammar and semantic actions
//  using lambda functions. The parser prints code suitable for a stack
//  based virtual machine, the semantic actions create the AST and
//  the evaluation is performed by a recursive visitor. The 'recursive' variant
//  decoupling is done with the help of std::optional<>
//
//  [ JDG May 10, 2002 ]        spirit 1
//  [ JDG March 4, 2007 ]       spirit 2
//  [ JDG February 21, 2011 ]   spirit 2.5
//  [ JDG June 6, 2014 ]        spirit x3 (from qi calc2, but using lambda functions)
//  [ MPo Oct 27, 2022 ]        boost::parser (from spirit x3)
//  [ MPo Apr 20, 2024 ]        AST, visitor
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/parser/parser.hpp>

#include <iostream>
#include <algorithm>
#include <string>
#include <deque>
#include <limits>

namespace bp = boost::parser;

template<typename TVisitor, typename TVariant>
decltype(auto) visit_node_recursively(TVisitor && visitor, TVariant && variant)
{
    return std::visit(
        std::forward<TVisitor>(visitor), std::forward<TVariant>(variant));
}

namespace impl {
    enum class ub_operator { uminus, add, subtract, multiply, divide };

    struct node;
    using onode = std::optional<node>;
    using vnode = std::variant<unsigned int, int, float, double, onode>;
    using node_array = std::vector<vnode>;
    struct node
    {
        ub_operator op;
        node_array nodes;

        node(const ub_operator op, vnode const & arg1) : op(op), nodes{arg1} {}
        node(const ub_operator op, vnode const & arg1, vnode const & arg2) :
            op(op), nodes{arg1, arg2}
        {}
    };
}

using impl::ub_operator;
using impl::vnode;
using impl::onode;

struct node_visitor
{
    double operator()(unsigned int x) const { return (double)x; }
    double operator()(int x) const { return (double)x; }
    double operator()(float x) const { return (double)x; }
    double operator()(double x) const { return x; }

    double operator()(onode const & pn)
    {
        auto & node = *pn;
        switch (node.op) {
        case ub_operator::uminus:
            assert(node.nodes.size() == 1);
            return -visit_node_recursively(*this, node.nodes[0]);
        case ub_operator::add:
            assert(node.nodes.size() == 2);
            return visit_node_recursively(*this, node.nodes[0]) +
                   visit_node_recursively(*this, node.nodes[1]);
        case ub_operator::subtract:
            assert(node.nodes.size() == 2);
            return visit_node_recursively(*this, node.nodes[0]) -
                   visit_node_recursively(*this, node.nodes[1]);
        case ub_operator::multiply:
            assert(node.nodes.size() == 2);
            return visit_node_recursively(*this, node.nodes[0]) *
                   visit_node_recursively(*this, node.nodes[1]);
        case ub_operator::divide:
            assert(node.nodes.size() == 2);
            return visit_node_recursively(*this, node.nodes[0]) /
                   visit_node_recursively(*this, node.nodes[1]);
        default:
            return std::numeric_limits<double>::quiet_NaN();
        }
    }
};

namespace client {
    ///////////////////////////////////////////////////////////////////////////////
    //  Semantic actions
    ////////////////////////////////////////////////////////1///////////////////////
    namespace {

    std::deque<vnode> vn_stack;

    auto do_int = [](auto & ctx) {
        std::cout << "push " << bp::_attr(ctx) << std::endl;
        vnode i = _attr(ctx);
        vn_stack.push_front(i); // ast
    };
    auto const do_add = [](auto & ctx) {
        std::cout << "add" << std::endl;
        // stack effect notation ( a, b -- c ) where c = a + b
        {   // ast
            assert(vn_stack.size() > 1);
            auto && second = vn_stack[0];
            auto && first = vn_stack[1];
            auto result = std::optional(impl::node(ub_operator::add, first, second));
            vn_stack[1] = result;
            vn_stack.pop_front();
        }
    };
    auto const do_subt = [](auto & ctx) {
        std::cout << "subtract" << std::endl;
        // stack effect notation ( a, b -- c ) where c = a - b
        {   // ast
            assert(vn_stack.size() > 1);
            auto && second = vn_stack[0];
            auto && first = vn_stack[1];
            auto result = std::optional(impl::node(ub_operator::subtract, first, second));
            vn_stack[1] = result;
            vn_stack.pop_front();
        }
    };
    auto const do_mult = [](auto & ctx) {
        std::cout << "mult" << std::endl;
        // stack effect notation ( a, b -- c ) where c = a * b
        {   // ast
            assert(vn_stack.size() > 1);
            auto && second = vn_stack[0];
            auto && first = vn_stack[1];
            auto result = std::optional(impl::node(ub_operator::multiply, first, second));
            vn_stack[1] = result;
            vn_stack.pop_front();
        }
    };
    auto const do_div = [](auto & ctx) {
        std::cout << "divide" << std::endl;
        // stack effect notation ( a, b -- c ) where c = a / b
        {   // ast
            assert(vn_stack.size() > 1);
            auto && second = vn_stack[0];
            auto && first = vn_stack[1];
            auto result = std::optional(impl::node(ub_operator::divide, first, second));
            vn_stack[1] = result;
            vn_stack.pop_front();
        }
    };
    auto const do_neg = [](auto & ctx) {
        std::cout << "negate" << std::endl;
        // stack effect notation ( a -- -a )
        {   // ast
            assert(vn_stack.size() > 0);
            auto && arg = vn_stack[0];
            auto result = std::optional(impl::node(ub_operator::uminus, arg));
            vn_stack[0] = result;
        }
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
int main()
{
    //test_recursive_node_visitor();
    std::cout << "/////////////////////////////////////////////////////////"
    << std::endl << std::endl;
    std::cout << "Expression parser..." << std::endl << std::endl;
    std::cout << "/////////////////////////////////////////////////////////"
    << std::endl << std::endl;
    std::cout << "Type an expression...or [q or Q] to quit"
    << std::endl << std::endl;

    typedef std::string::const_iterator iterator_type;

    std::string str;
    while (std::getline(std::cin, str)) {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        auto & calc = client::calculator; // Our grammar

        auto r = bp::parse(str, calc, bp::ws);

        if (r) {
            std::cout << "-------------------------" << std::endl;
            std::cout << "Parsing succeeded" << std::endl;
            assert(client::vn_stack.size() == 1);
            std::cout << str << " ==> "
            << std::visit(node_visitor(), client::vn_stack[0])
            << " (AST eval)"
            << std::endl;
            client::vn_stack.pop_front();
            std::cout << "-------------------------" << std::endl;
        } else {
            iterator_type iter = str.begin();
            iterator_type end = str.end();
            std::cout << "-------------------------" << std::endl;
            std::cout << "Parsing failed" << std::endl;
            bp::prefix_parse(iter, end, calc, bp::ws);
            std::string rest(iter, end);
            std::cout << "stopped at: \"" << rest << "\"" << std::endl;
            client::vn_stack.clear();
            std::cout << "-------------------------" << std::endl;
        }
        std::cout << "Type an expression...or [q or Q] to quit"
        << std::endl << std::endl;
    }

    std::cout << "Bye... :-)" << std::endl << std::endl;
    return 0;
}
