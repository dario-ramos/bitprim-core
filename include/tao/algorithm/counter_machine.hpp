//! \file tao/algorithm/counter_machine.hpp
// Tao.Algorithm
//
// Copyright Fernando Pelliccioni 2016
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_ALGORITHM_COUNTER_MACHINE_HPP
#define TAO_ALGORITHM_COUNTER_MACHINE_HPP

#include <iterator>

#include <tao/algorithm/concepts.hpp>
#include <tao/algorithm/integers.hpp>
#include <tao/algorithm/type_attributes.hpp>

namespace tao { namespace algorithm {

// // Concepts
// #define Integer typename
// #define Container typename
// #define Iterator typename
// #define ForwardIterator typename
// #define BinaryOperation typename
// #define Semiregular typename
// #define UnaryFunction typename

// #define requires(...) 

//TODO: 
	// Mutable<I>
	// Domain<Op>



template <ForwardIterator I, BinaryOperation Op>
// 	TODO: requires Op is associative
	requires(Mutable<I> && ValueType<I> == Domain<Op>) 
ValueType<I> add_to_counter(I f, I l, Op op, ValueType<I> x, ValueType<I> const& e) {
	// precondition: x != e
	while (f != l) {
		if (*f == e) {
			*f = x;
			return e;
		}
		x = op(*f, x);
		*f = e;
		++f;
	}
	return x;
}

template <Semiregular T, BinaryOperation Op, std::size_t Size = 64>
// 	requires Op is associative //TODO
	requires(Domain<Op> == T) //TODO...
class counter_machine {
public:	
	counter_machine(Op op, T const& e)
		: op_{op}, e_{e}, l{f}
	{}

	counter_machine(counter_machine const&) = delete;
	counter_machine& operator=(counter_machine const&) = delete;
	
	//Move Constructor and Move Assignment Operator are deleted too
    //See http://stackoverflow.com/questions/37092864/should-i-delete-the-move-constructor-and-the-move-assignment-of-a-smart-pointer/38820178#38820178
    // and http://talesofcpp.fusionfenix.com/post-24/episode-eleven-to-kill-a-move-constructor

	void add_to(T x, T* to) {
		// precondition: TODO
		x = add_to_counter(to, l, op_, x, e_);
		if (x != e_) {
			*l = x;
			++l;
		}
	}

	void add(T x) {
		// precondition: must not be called more than 2^Size - 1 times
        add_to(x, f);
	}

private:
	const Op op_;
	const T e_;
public:
	T f[Size];
	T* l;
};

}} /*tao::algorithm*/

#endif /*TAO_ALGORITHM_COUNTER_MACHINE_HPP*/
