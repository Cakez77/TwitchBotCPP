

global s_http http;

func s_http_get http_request(char* url, char* header, s_lin_arena* arena,
														 wchar_t* method, char* data)
{
	g_http_mutex.lock();

	s_http_get result = zero;
	int url_len = (int)strlen(url);
	char* base = (char*)arena->get(url_len + 1);
	char* query = (char*)arena->get(url_len + 1);
	query[0] = 0;
	my_strcpy(base, url_len + 1, url);
	str_remove_from_left(base, "https://");
	str_remove_from_left(base, "http://");
	str_remove_from_left(base, "www.");
	int slash = str_find_from_left(base, "/");
	if(slash != -1)
	{
		my_strcpy(query, url_len + 1, base + slash + 1);
		base[slash] = 0;
	}

	if(!http.session_handle)
	{
		http.session_handle = WinHttpOpen(
			L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/112.0.0.0 Safari/537.36",
			WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			WINHTTP_FLAG_ASYNC
		);
		assert(http.session_handle);
	}

	s_connection* connection = null;
	foreach(connection_i, c, http.connection_arr)
	{
		if(c->url.equals(base))
		{
			connection = c;
			break;
		}
	}
	if(!connection)
	{
		s_connection new_connection = zero;
		new_connection.url.from_cstr(base);
		new_connection.handle = WinHttpConnect(
			http.session_handle,
			str_to_wide(base, arena),
			INTERNET_DEFAULT_PORT,
			0
		);
		assert(new_connection.handle);
		http.connection_arr.add(new_connection);
		connection = http.connection_arr.get_last_ptr();
	}
	assert(connection->handle);

	wchar_t* wide_query = str_to_wide(query, arena);
	HINTERNET request_handle = WinHttpOpenRequest(
		connection->handle,
		method,
		wide_query,
		null,
		WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		WINHTTP_FLAG_REFRESH // ??
	);
	assert(request_handle);

	wchar_t* wide_header = header? str_to_wide(header, arena) : 0;
	DWORD header_length = header? (DWORD)wcslen(wide_header) : 0;
	// DWORD data_length = data? (DWORD)wcslen(data) : 0;
	BOOL send_result = WinHttpSendRequest(
		request_handle,
		wide_header? wide_header: WINHTTP_NO_ADDITIONAL_HEADERS,
		wide_header? header_length : 0,
		data? data : WINHTTP_NO_REQUEST_DATA,
		data? (DWORD)strlen(data) : 0,
		data? (DWORD)strlen(data) : 0,
		null
	);
	assert(send_result);

	BOOL receive_result = WinHttpReceiveResponse(request_handle, null);
	assert(receive_result);

	// DWORD buffer_len = sizeof(int);
	// BOOL query_header_result = WinHttpQueryHeaders(
	// 	request_handle,
	// 	WINHTTP_QUERY_STATUS_CODE,
	// 	null,
	// 	&data.status_code,
	// 	&buffer_len,
	// 	WINHTTP_NO_HEADER_INDEX
	// );
	// if(!query_header_result)
	// {
	// 	int code = GetLastError();
	// 	switch(code)
	// 	{
	// 		case ERROR_WINHTTP_HEADER_NOT_FOUND:
	// 		{
	// 			printf("header\n");
	// 		} break;
	// 		case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:
	// 		{
	// 			printf("handle state\n");
	// 		} break;
	// 		case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
	// 		{
	// 			printf("handle type\n");
	// 		} break;
	// 	}
	// 	assert(false);
	// }
	// printf("%i\n", data.status_code);

	result.text = (char*)arena->get_clear(1 * c_mb);
	char* cursor = result.text;

	while(true)
	{
		DWORD bytes_to_read = 0;
		WinHttpQueryDataAvailable(
			request_handle,

			// @TODO(tkap, 20/04/2023): https://learn.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpquerydataavailable
			// "When WinHTTP is used in asynchronous mode, always set this parameter to NULL and retrieve data in the callback function; not doing so can cause a memory fault."
			&bytes_to_read
		);
		if(bytes_to_read <= 0) { break; }

		DWORD bytes_read = 0;
		BOOL read_result = WinHttpReadData(
			request_handle,
			cursor,
			bytes_to_read,

			// @TODO(tkap, 20/04/2023): "When using WinHTTP asynchronously, always set this parameter to NULL and retrieve the information in the callback function; not doing so can cause a memory fault."
			&bytes_read
		);
		assert(read_result);
		assert(bytes_read == bytes_to_read);
		cursor += bytes_read;
		result.size_in_bytes += bytes_read;
	}

	BOOL close_result = WinHttpCloseHandle(request_handle);
	assert(close_result);

	g_http_mutex.unlock();

	return result;
}

func s_http_get http_get(char* url, char* header, s_lin_arena* arena)
{
	return http_request(url, header, arena, L"GET");
}

func s_http_get http_post(char* url, char* header, s_lin_arena* arena)
{
	return http_request(url, header, arena, L"POST");
}