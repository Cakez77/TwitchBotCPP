
enum e_font
{
	e_font_small,
	e_font_medium,
	e_font_big,
	e_font_count,
};

enum e_layer
{
	e_layer_base,
};

func void draw_rect(s_v2 pos, int layer, s_v2 size, s_v4 color, s_transform t = zero);
func void draw_text(char* text, s_v2 in_pos, int layer, s_v4 color, e_font font_id, b8 centered, s_transform t = zero);
func void draw_texture(s_v2 pos, int layer, s_v2 size, s_v4 color, s_transform t);