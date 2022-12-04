#include <cwctype>
#include "mp_rndstr.hpp"

wstring mp_rndstr::mp_random_string(const main_weights_seq& sequence)
{
	_vars_just_used_qty = 0;

	auto seq_len{ sequence.size() };

	main_weights_seq default_sequence{};

	bool use_default_sequence{ false };

	if (seq_len == 0) {

		use_default_sequence = true;

		// weights for +, -, *, /, ^, (), f(), var, val

		default_sequence.push_back({ 4,4,2,1,1,4,4,0,0 });
		default_sequence.push_back({ 4,4,3,2,1,3,4,1,1 });
		default_sequence.push_back({ 4,3,3,1,1,3,4,3,3 });
		default_sequence.push_back({ 3,3,3,2,1,1,3,3,3 });
		default_sequence.push_back({ 3,3,2,1,1,1,2,3,3 });
		default_sequence.push_back({ 3,2,2,1,1,0,1,3,3 });

		seq_len = default_sequence.size();
	}

	wstring rnd_str{ L"@" };

	rnd_str.insert(0, rnd_uni_sign.next(gen));

	for (size_t it = 0; it < seq_len; ++it)
		single_pass(rnd_str, use_default_sequence ? default_sequence[it] : sequence[it]);

	single_pass(rnd_str,{ 0,0,0,0,0,0,0,1,1 });

	return rnd_str;
}

void mp_rndstr::shuffle_variables(int var_case)
{
	for (size_t it = 0; it < rnd_var_qty; ++it)
	{
		const auto var_id = shuffle_vars.next(gen);

		const auto var_id_lwr = var_id[0];
		const auto var_id_upr = towupper(var_id_lwr);

		if (var_case > 0)
			rnd_variable.items[it] = var_id_lwr;
		else
			if (var_case < 0)
				rnd_variable.items[it] = var_id_upr;
			else
				rnd_variable.items[it] = true_false.next(gen) ? var_id_lwr : var_id_upr;
	}
}

void mp_rndstr::single_pass(wstring& rnd_str, const main_weights& weights)
{
	const auto str_len{ rnd_str.size() };

	if (str_len == 0) return;

	randomizer<char, main_lex_qty> rnd({ '+','-','*','/','^','(','f','x','5' }, weights);

	for (auto it = str_len - 1; ;)
	{
		if (rnd_str[it] != '@')
			if (it == 0)
				break;
			else
			{
				--it;
				continue;
			}

		switch (rnd.next(gen))
		{
			case '+':
			{
				rnd_str.insert(it, L"@ + ");
				break;
			}

			case '-':
			{
				rnd_str.insert(it, L"@ - ");
				break;
			}

			case '*':
			{
				rnd_str.insert(it, L"@*");
				break;
			}

			case '/':
			{
				rnd_str.insert(it, L"@/");
				break;
			}

			case '^':
			{
				rnd_str.insert(it, L"@^");
				break;
			}

			case '(':
			{
				rnd_str.insert(it + 1, L")");
				rnd_str.insert(it, rnd_uni_sign.next(gen));
				rnd_str.insert(it, L"(");
				break;
			}

			case 'f':
			{
				rnd_str.insert(it + 1, L")");
				rnd_str.insert(it, rnd_uni_sign.next(gen));
				rnd_str.insert(it, rnd_function.next(gen));

				break;
			}

			case 'x':
			{
				rnd_str.erase(it, 1);
				const auto rnd_var = rnd_variable.next(gen);
				rnd_str.insert(it, rnd_var);

				size_t ind = 0;
				for (; ind < _vars_just_used_qty; ++ind)
					if (rnd_var[0] == _vars_just_used[ind]) break;

				if (ind == _vars_just_used_qty)
					_vars_just_used[_vars_just_used_qty++] = rnd_var[0];

				break;
			}

			case '5':
			{
				rnd_str.erase(it, 1);
				rnd_str.insert(it, rnd_constant.next(gen));
				break;
			}
		}

		if (it == 0) break;
		--it;
	}
}
