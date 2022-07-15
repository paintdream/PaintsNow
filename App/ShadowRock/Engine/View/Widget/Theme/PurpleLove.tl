local globalFont = "/Packed/Font/Cantarell-Regular"
local globalImage = {
	path = "/Packed/Texture/DarkUI",
	width = 512,
	height = 512
}

local globalScalePattern = {
	repeatable = true,
	edge = { left = 8, top = 8, right = 8, bottom = 8 }
}

return {
	width = 512,
	height = 512,
	styles =
	{
		title =
		{
			font = globalFont,
			fontSize = 24,
			size = { 0, 28, 0, 28 },
			margin = { 5, 5, 5, 5 },
			padding = { 5, 5, 5, 5 },
			fitContent = false,
			alignment = "center"
		},
		plain =
		{
			font = globalFont,
			fontSize = 18,
			size = { 0, 20, 0, 20 },
			margin = { 5, 5, 5, 5 },
			padding = { 5, 5, 5, 5 },
			fitContent = false,
			alignment = "center"
		},
		button =
		{
			font = globalFont,
			fontSize = 18,
			size = { 0, 20, 0, 20 },
			margin = { 5, 5, 5, 5 },
			padding = { 5, 5, 5, 5 },
			fitContent = true,
			alignment = "center"
		},
		tag =
		{
			font = globalFont,
			fontSize = 18,
			size = { 0, 20, 0, 20 },
			margin = { 5, 5, 5, 5 },
			padding = { 5, 5, 5, 5 },
			fitContent = true,
			alignment = "center"
		}
	},
	slices =
	{
		add_hover =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 452, y = 145, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		add_normal =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 452, y = 177, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		add_pressed =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 452, y = 209, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		arrow_left_hover =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 452, y = 241, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		arrow_left_normal =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 452, y = 273, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		arrow_left_pressed =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 452, y = 305, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		arrow_right_hover =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 440, y = 337, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		arrow_right_normal =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 472, y = 337, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		arrow_right_pressed =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 440, y = 369, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		button_hover =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 168, y = 479, w = 105, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 105, h = 30 },
			sourceSize = { w = 105, h = 30 }
		},
		button_normal =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 218, y = 76, w = 105, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 105, h = 30 },
			sourceSize = { w = 105, h = 30 }
		},
		button_pressed =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 325, y = 86, w = 105, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 105, h = 30 },
			sourceSize = { w = 105, h = 30 }
		},
		checkbox_checked =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 484, y = 154, w = 24, h = 24 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 24, h = 24 },
			sourceSize = { w = 24, h = 24 }
		},
		checkbox_unchecked =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 484, y = 180, w = 24, h = 24 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 24, h = 24 },
			sourceSize = { w = 24, h = 24 }
		},
		field =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 2, y = 134, w = 211, h = 36 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 211, h = 36 },
			sourceSize = { w = 211, h = 36 }
		},
		field_hover =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 215, y = 139, w = 211, h = 36 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 211, h = 36 },
			sourceSize = { w = 211, h = 36 }
		},
		field_round =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 2, y = 172, w = 211, h = 36 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 211, h = 36 },
			sourceSize = { w = 211, h = 36 }
		},
		field_round_hover =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 215, y = 177, w = 211, h = 36 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 211, h = 36 },
			sourceSize = { w = 211, h = 36 }
		},
		icon_confirm =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 452, y = 79, w = 36, h = 31 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 36, h = 31 },
			sourceSize = { w = 36, h = 31 }
		},
		icon_question =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 452, y = 41, w = 36, h = 36 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 36, h = 36 },
			sourceSize = { w = 36, h = 36 }
		},
		icon_warning =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 452, y = 2, w = 36, h = 37 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 36, h = 37 },
			sourceSize = { w = 36, h = 37 }
		},
		loading_fill =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 356, y = 2, w = 82, h = 82 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 82, h = 82 },
			sourceSize = { w = 82, h = 82 }
		},
		loading_frame =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 356, y = 335, w = 82, h = 82 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 82, h = 82 },
			sourceSize = { w = 82, h = 82 }
		},
		message_background =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 2, y = 2, w = 352, h = 72 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 352, h = 72 },
			sourceSize = { w = 352, h = 72 }
		},
		message_frame =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 2, y = 335, w = 352, h = 72 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 352, h = 72 },
			sourceSize = { w = 352, h = 72 }
		},
		minus_hover =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 472, y = 369, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		minus_normal =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 440, y = 401, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		minus_pressed =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 472, y = 401, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		mult_hover =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 435, y = 473, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		mult_normal =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 467, y = 473, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		mult_pressed =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 275, y = 479, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		playcontrol =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 2, y = 76, w = 214, h = 56 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 214, h = 56 },
			sourceSize = { w = 214, h = 56 }
		},
		progress_control =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 490, y = 32, w = 20, h = 20 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 20, h = 20 },
			sourceSize = { w = 20, h = 20 }
		},
		progress_fill =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 2, y = 253, w = 267, h = 44 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 267, h = 44 },
			sourceSize = { w = 267, h = 44 }
		},
		progress_frame =
		{
			image = globalImage,
			pattern = globalScalePattern,
			edge = { x = 15, y = 5 },
			frame = { x = 2, y = 433, w = 356, h = 44 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 356, h = 44 },
			sourceSize = { w = 356, h = 44 }
		},
		progress_hint =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 360, y = 419, w = 73, h = 40 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 73, h = 40 },
			sourceSize = { w = 73, h = 40 }
		},
		progress_slot =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 2, y = 419, w = 331, h = 10 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 331, h = 10 },
			sourceSize = { w = 331, h = 10 }
		},
		radio_checked =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 484, y = 206, w = 24, h = 24 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 24, h = 24 },
			sourceSize = { w = 24, h = 24 }
		},
		radio_unchecked =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 484, y = 232, w = 24, h = 24 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 24, h = 24 },
			sourceSize = { w = 24, h = 24 }
		},
		search =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 2, y = 210, w = 211, h = 36 },
			edge = { x = 40 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 211, h = 36 },
			sourceSize = { w = 211, h = 36 }
		},
		search_hover =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 215, y = 215, w = 211, h = 36 },
			edge = { x = 60 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 211, h = 36 },
			sourceSize = { w = 211, h = 36 }
		},
		selector =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 490, y = 2, w = 20, h = 28 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 20, h = 28 },
			sourceSize = { w = 20, h = 28 }
		},
		slider_control =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 307, y = 479, w = 40, h = 22 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 40, h = 22 },
			sourceSize = { w = 40, h = 22 }
		},
		slider_control_vertical =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 486, y = 112, w = 22, h = 40 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 22, h = 40 },
			sourceSize = { w = 22, h = 40 }
		},
		slider_line =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 2, y = 419, w = 331, h = 10 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 331, h = 10 },
			sourceSize = { w = 331, h = 10 }
		},
		slider_line_vertical =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 440, y = 2, w = 10, h = 331 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 10, h = 331 },
			sourceSize = { w = 10, h = 331 }
		},
		star_empty =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 452, y = 112, w = 32, h = 31 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 32, h = 31 },
			sourceSize = { w = 32, h = 31 }
		},
		star_full =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 218, y = 108, w = 30, h = 29 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 29 },
			sourceSize = { w = 30, h = 29 }
		},
		switch_off =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 435, y = 433, w = 73, h = 38 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 73, h = 38 },
			sourceSize = { w = 73, h = 38 }
		},
		switch_on =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 360, y = 461, w = 73, h = 38 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 73, h = 38 },
			sourceSize = { w = 73, h = 38 }
		},
		tag =
		{
			image = globalImage,
			pattern = globalScalePattern,
			frame = { x = 2, y = 479, w = 164, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 164, h = 30 },
			sourceSize = { w = 164, h = 30 }
		}
	}
}