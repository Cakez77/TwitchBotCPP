

/* @Note(tkap, 23/04/2023): This probably breaks with non-ascii characters
Usage:
char* str = R"(
{
		"foo":
		{
			"bar": 7,
			"baz": "66",
		}
	}
)";
s_json* json = parse_json(str);
if(json)
{
	s_json* baz_data = json->get("foo")->get("baz");
	// s_json* baz_data = json->get_deep("baz"); // this also works
	assert(baz_data->type == e_json_type_string);
	printf("%s\n", baz_data->str.str); // prints 66
}
*/

func s_json* parse_json(char* str)
{
	s_tokenizer tokenizer = zero;
	tokenizer.at = str;
	s_json_parse_status status;
	s_json* result = null;
	s_json_parse jp = parse_json_value(tokenizer, &status);
	if(jp.success)
	{
		result = (s_json*)malloc(sizeof(s_json));
		*result = jp.json;
		return result;
	}

	// @Note(tkap, 23/04/2023): For debugging purposes, can be removed
	if(status.error)
	{
		printf("Error parsing json: %s\n", status.error_msg.data);
	}

	return null;
}

func s_json_parse parse_json_key(s_tokenizer tokenizer, s_json_parse_status* status)
{
	s_json_parse result = zero;
	s_token token = zero;
	breakable_block
	{
		if(!consume_token(e_token_string, &tokenizer, &token)) { break; }
		// @Note(tkap, 23/04/2023): +1 and -2 because we want foo, not "foo"
		result.key.from_data(token.at + 1, token.length - 2);
		if(!consume_token(":", &tokenizer))
		{
			status->do_error("Expected ':' after key");
			break;
		}
		result.success = true;
		result.tokenizer = tokenizer;
	}
	return result;
}

func s_json_parse parse_json_value(s_tokenizer tokenizer, s_json_parse_status* status)
{
	s_json_parse result = zero;
	s_json_parse jp = zero;
	s_json json = zero;
	s_token token = zero;
	if(consume_token("{", &tokenizer))
	{
		json.type = e_json_type_json;
		b8 found_comma = true;
		while(true)
		{
			if(consume_token("}", &tokenizer)) { break; }

			// @Note(tkap, 23/04/2023): This handles the following mistake:
			// "foo": 1 <--- MISSING COMMA HERE!
			// "bar": 2
			if(!found_comma)
			{
				status->do_error("Missing comma after TODO");
				goto end;
			}

			jp = parse_json_key(tokenizer, status);
			if(!jp.success)
			{
				status->do_error("Expected key");
				goto end;
			}
			tokenizer = jp.tokenizer;
			json.add_key(jp.key);

			jp = parse_json_value(tokenizer, status);
			if(!jp.success)
			{
				status->do_error("Expected value after key");
				goto end;
			}
			tokenizer = jp.tokenizer;
			json.add_value(jp.json);

			found_comma = false;
			if(consume_token(",", &tokenizer)) { found_comma = true; }
		}
	}
	else if(consume_token(e_token_number, &tokenizer, &token))
	{
		json.type = e_json_type_number;
		json.number = token_to_int(token);
	}
	else if(consume_token(e_token_string, &tokenizer, &token))
	{
		json.type = e_json_type_string;
		json.str.len = token.length - 2;
		json.str.str = (char*)malloc(json.str.len + 1);
		memcpy(json.str.str, token.at + 1, json.str.len);
		json.str.str[json.str.len] = 0;
	}
	else if(consume_token("[", &tokenizer))
	{
		json.type = e_json_type_array;
		b8 found_comma = true;
		while(true)
		{
			if(consume_token("]", &tokenizer)) { break; }
			if(!found_comma)
			{
				status->do_error("Missing ','");
				goto end;
			}

			jp = parse_json_value(tokenizer, status);
			if(!jp.success)
			{
				status->do_error("Expected a value");
				goto end;
			}
			tokenizer = jp.tokenizer;
			json.add_value(jp.json);

			found_comma = false;
			if(consume_token(",", &tokenizer)) { found_comma = true; }
		}
	}
	else if(consume_token("true", &tokenizer))
	{
		json.type = e_json_type_bool;
		json.nbool = true;
	}
	else if(consume_token("false", &tokenizer))
	{
		json.type = e_json_type_bool;
		json.nbool = false;
	}
	else if(consume_token("null", &tokenizer))
	{
		json.type = e_json_type_null;
	}
	else
	{
		status->do_error("Unrecognized value");
		goto end;
	}

	result.success = true;
	result.json = json;
	result.tokenizer = tokenizer;

end:
	return result;

}