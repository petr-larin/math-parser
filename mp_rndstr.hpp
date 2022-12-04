#pragma once

#include <array>
#include <random>
#include <vector>
#include <string>

using std::array;
using std::random_device;
using std::mt19937;
using std::discrete_distribution;
using std::vector;
using std::wstring;

class mp_rndstr {

public:

	constexpr static size_t main_lex_qty{ 9 };
	using main_weights = array<double, main_lex_qty>; // weights for +, -, *, /, ^, (), f(), var, val
	using main_weights_seq = vector<main_weights>;
	constexpr static size_t rnd_var_qty{ 5 };

	mp_rndstr() = default;

	// Generate a random string based on the weights in the sequence.
	// If the sequence is empty, the default sequence is used.

	wstring mp_random_string(const main_weights_seq& sequence);
	
	// Select rnd_var_qty var IDs as candidates for use in the random string.
	// They are not necessarily different.
	// var_case < 0 - all ids are uppercase
	// var_case == 0 - each id is random case
	// var_case > 0 - all ids are lowercase

	void shuffle_variables(int var_case);

	// Return the current var ID candidates

	wchar_t current_vars(size_t index)
	{
		return rnd_variable.items[index][0];
	}

	// Return the var IDs actually used in the last call to mp_random_string.
	// (A subset of current_vars.)
	// 0 <= ind < vars_just_used_qty()

	wchar_t vars_just_used(size_t ind)
	{
		return _vars_just_used[ind];
	}

	// Size of the vars_just_used array

	size_t vars_just_used_qty()
	{
		return _vars_just_used_qty;
	}

private:

	template<typename T, size_t N>
	class randomizer {

	friend class mp_rndstr;

	public:

		randomizer() = default;

		randomizer(const array<T, N>& items, const array<double, N>& weights)
			: items{ items }, distr{ weights.cbegin(), weights.cend() } {};

		T next(mt19937& gen)
		{
			return items[distr(gen)];
		}

	private:

		array<T, N> items;
		discrete_distribution<> distr;
	};

	random_device rd{};
	mt19937 gen{ rd() };

	void single_pass(wstring& rnd_str, const main_weights& weights);

	randomizer<wstring, 3> rnd_uni_sign{ { L"+", L"-", L"" }, { 1,2,7 } };
	randomizer<wstring, rnd_var_qty> rnd_variable{ 
		{ L"a", L"t", L"x", L"y", L"z" }, 
		{ 1,1,1,1,1 } 
	};
	randomizer<wstring, 11> rnd_constant{ 
		{ L"pi", L"e", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"10", L"sqrt(2)"},
		{ 2,1,2,2,2,2,2,1,1,1,1 } 
	};
	randomizer<wstring, 10> rnd_function{
		{ L"sqrt(", L"sin(", L"cos(", L"tg(", L"ctg(", L"arcsin(", L"arctg(", L"exp(", L"ln(", L"abs("},
		{ 1,1,1,1,1,1,1,1,1,1 }
	};
	randomizer<wstring, 11> shuffle_vars{
		{ L"a", L"b", L"c", L"s", L"t", L"u", L"v", L"w", L"x", L"y", L"z"}, 
		{ 1,1,1,1,1,1,1,1,1,1,1 }
	};
	randomizer<bool, 2> true_false{ { false, true }, { 1,1 } };

	array<wchar_t, rnd_var_qty> _vars_just_used{};
	size_t _vars_just_used_qty{};
};