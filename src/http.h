

struct s_connection
{
	HINTERNET handle;
	s_str<64> url;
};

struct s_http
{
	HINTERNET session_handle;
	s_sarray<s_connection, 16> connection_arr;
};

struct s_http_get
{
	char* text;
	int status_code;
	int size_in_bytes;
};

func s_http_get http_request(char* url, char* header, s_lin_arena* arena, wchar_t* method = L"GET", char* data = 0);
func s_http_get http_get(char* url, char* header, s_lin_arena* arena);
func s_http_get http_post(char* url, char* header, s_lin_arena* arena);
