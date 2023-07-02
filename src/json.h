
enum e_json_type
{
	e_json_type_invalid,
	e_json_type_json,
	e_json_type_number,
	e_json_type_string,
	e_json_type_array,
	e_json_type_bool,
	e_json_type_null,
	e_json_type_count,
};

struct s_len_str
{
	int len;
	char* str;
};

struct s_json
{
	e_json_type type;
	s_str<64>* keys;
	s_json* values;

	s64 number;
	s_len_str str;
	b8 nbool;

	int key_count;
	int key_capacity;
	int value_count;
	int value_capacity;

	s_json* get(char* in_key)
	{
		for(int key_i = 0; key_i < key_count; key_i++)
		{
			auto key = keys[key_i];
			if(key.equals(in_key)) { return &values[key_i]; }
		}
		return null;
	}

	s_json* get_deep(char* in_key)
	{
		for(int key_i = 0; key_i < key_count; key_i++)
		{
			auto key = keys[key_i];
			auto value = &values[key_i];
			if(key.equals(in_key)) { return value; }
			if(value->type == e_json_type_json)
			{
				s_json* possible_result = value->get_deep(in_key);
				if(possible_result) { return possible_result; }
			}

		}
		return null;
	}

	void add_key(s_str<64> key)
	{
		if(key_capacity == 0)
		{
			key_capacity = 16;
			keys = (s_str<64>*)malloc(sizeof(s_str<64>) * key_capacity);
		}
		else if(key_count == key_capacity)
		{
			assert(keys);
			key_capacity *= 2;
			keys = (s_str<64>*)realloc(keys, sizeof(s_str<64>) * key_capacity);
		}
		keys[key_count++] = key;
	}

	void add_value(s_json value)
	{
		if(value_capacity == 0)
		{
			value_capacity = 16;
			values = (s_json*)malloc(sizeof(s_json) * value_capacity);
		}
		else if(value_count == value_capacity)
		{
			assert(values);
			value_capacity *= 2;
			values = (s_json*)realloc(values, sizeof(s_json) * value_capacity);
		}
		values[value_count++] = value;
	}
};

struct s_json_parse
{
	b8 success;
	s_str<64> key;
	s_json json;
	s_tokenizer tokenizer;
};

struct s_json_parse_status
{
	b8 error;
	s_str<64> error_msg;

	void do_error(char* str)
	{
		if(error) { return; }
		error = true;
		error_msg.from_cstr(str);
	}
};

func s_json_parse parse_json_key(s_tokenizer tokenizer, s_json_parse_status* status);
func s_json_parse parse_json_value(s_tokenizer tokenizer, s_json_parse_status* status);