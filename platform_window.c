#include "matrix.h"
#include "platform_window.h"
#include "vector.h"

#if PLATFORM_WINDOWS

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
enum key                win32_vk_to_key(WORD vk);

enum error platform_window_create(int width, int height, const char* s_title, struct platform_window* p_platform_window, void(*p_callback_on_created)(struct platform_window*, void*), void* p_callback_on_created_user_ptr) {    
    HINSTANCE hinstance = GetModuleHandle(NULL);

	const WNDCLASS wndclass = {
		.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
		.lpfnWndProc = wnd_proc,
		.cbWndExtra = sizeof(struct platform_window*),
		.hInstance = hinstance,
		.hCursor = LoadCursor(0, IDC_ARROW),
		.lpszClassName = "GLWnd",
	};

    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE;
    const DWORD styleex = WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE;

	static int b_wndclass_registered = 0;

	if (!b_wndclass_registered) {
		if (!RegisterClass(&wndclass)) {
            return ERROR_WIN32_REGISTER_CLASS;
        }
		b_wndclass_registered = 1;
	}
	
	RECT rect = {
		.right = width ?width : 800,
		.bottom = height ? height : 600,
	};
	AdjustWindowRectEx(&rect, style, FALSE, styleex);

    *p_platform_window = (struct platform_window) {
        ._p_callback_on_created = p_callback_on_created,
        ._p_callback_on_created_user_ptr = p_callback_on_created_user_ptr
    };

	HWND hwnd = CreateWindowEx(
		styleex,
		wndclass.lpszClassName,
		s_title,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		0,
		0,
		hinstance,
		(LPVOID)p_platform_window
    );

	if (!hwnd) {
        return ERROR_WIN32_CREATE_WINDOW;
    }

    p_platform_window->_hwnd = hwnd;

	return ERROR_OK;
}

void platform_window_destroy(struct platform_window* p_platform_window) {
    ReleaseDC(p_platform_window->_hwnd, p_platform_window->_hdc);
    DestroyWindow(p_platform_window->_hwnd);
    *p_platform_window = (struct platform_window){0};
}

void platform_window_poll(struct platform_window* p_platform_window) {
    MSG msg;
	while (PeekMessage(&msg, p_platform_window->_hwnd, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
    p_platform_window->_mouse_pos_old[0] = p_platform_window->_mouse_pos[0];
    p_platform_window->_mouse_pos_old[1] = p_platform_window->_mouse_pos[1];
}

int platform_window_is_open(const struct platform_window* p_platform_window) {
    return IsWindow(p_platform_window->_hwnd) == TRUE;
}

void platform_window_size(const struct platform_window* p_platform_window, unsigned int* p_width, unsigned int* p_height) {
    RECT client_rect;
    GetClientRect(p_platform_window->_hwnd, &client_rect);
    *p_width = client_rect.right;
    *p_height = client_rect.bottom;
}

void platform_window_get_mouse_client_coords(const struct platform_window* p_platform_window, int* p_x, int* p_y) {
    *p_x = p_platform_window->_mouse_pos[0];
    *p_y = p_platform_window->_mouse_pos[1];
}

void platform_window_normalize_client_coords(const struct platform_window* p_platform_window, int client_x, int client_y, float* p_x, float* p_y) {
    unsigned int width, height;
    platform_window_size(p_platform_window, &width, &height);
    *p_x = (float)client_x / width;
    *p_y = (float)client_y / height;
}

void platform_window_client_coords_to_ndc(const struct platform_window* p_platform_window, int client_x, int client_y, float* p_ndc_x, float* p_ndc_y) {
    platform_window_normalize_client_coords(p_platform_window, client_x, client_y, p_ndc_x, p_ndc_y);
    *p_ndc_x =  (*p_ndc_x - 0.5) * 2;
    *p_ndc_y = -(*p_ndc_y - 0.5) * 2;
}

void platform_window_client_to_world_ray(const struct platform_window* p_platform_window, const float* p_camera_matrix, int client_x, int client_y, float* p_result) {
    float inv[16];
    matrix_inverse(4, p_camera_matrix, inv);
    
    float ndc[2];
    platform_window_client_coords_to_ndc(p_platform_window, client_x, client_y, &ndc[0], &ndc[1]);

    float n[4] = { ndc[0], ndc[1], -1, 1 };
    matrix_multiply_vec4(inv, n, n);
    vec_div_s(4, n, n[3], n);
    
    float f[4] = { ndc[0], ndc[1], 1, 1 };
    matrix_multiply_vec4(inv, f, f);
    vec_div_s(4, f, f[3], f);

    vec3_sub(f, n, p_result);
    vec3_norm(p_result, p_result);
}

void platform_window_set_on_close_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*), void* p_user_ptr) {
    p_platform_window->_p_callback_on_close = p_callback;
    p_platform_window->_p_callback_on_close_user_ptr = p_user_ptr;
}

void platform_window_set_on_focus_change_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*, int), void* p_user_ptr) {
    p_platform_window->_p_callback_on_focus_change = p_callback;
    p_platform_window->_p_callback_on_focus_change_user_ptr = p_user_ptr;
}

void platform_window_set_on_resize_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*, int, int), void* p_user_ptr) {
    p_platform_window->_p_callback_on_resize = p_callback;
    p_platform_window->_p_callback_on_resize_user_ptr = p_user_ptr;
}

void platform_window_set_on_key_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*, enum key, int), void* p_user_ptr) {
    p_platform_window->_p_callback_on_key = p_callback;
    p_platform_window->_p_callback_on_key_user_ptr = p_user_ptr;
}

void platform_window_set_on_mouse_button_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*, enum mouse_button, int), void* p_user_ptr) {
    p_platform_window->_p_callback_on_mouse_button = p_callback;
    p_platform_window->_p_callback_on_mouse_button_user_ptr = p_user_ptr;
}

void platform_window_set_on_mouse_move_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*, int, int), void* p_user_ptr) {
    p_platform_window->_p_callback_on_mouse_move = p_callback;
    p_platform_window->_p_callback_on_mouse_move_user_ptr = p_user_ptr;
}

void platform_window_set_on_scroll_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*, int), void* p_user_ptr) {
    p_platform_window->_p_callback_on_scroll = p_callback;
    p_platform_window->_p_callback_on_scroll_user_ptr = p_user_ptr;
}

void platform_window_set_on_char_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*, int), void* p_user_ptr) {
    p_platform_window->_p_callback_on_char = p_callback;
    p_platform_window->_p_callback_on_char_user_ptr = p_user_ptr;
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;

	struct platform_window* p_platform_window = (LPVOID)GetWindowLongPtr(hwnd, 0);

    switch (msg) {
        case WM_CREATE: {
			const CREATESTRUCT* createstruct = (const CREATESTRUCT*)lparam;
			p_platform_window = createstruct->lpCreateParams;

            HDC hdc = GetDC(hwnd);
            
            RECT rect = {0};
            SetRectEmpty(&rect);
            AdjustWindowRectEx(&rect, createstruct->style, FALSE, createstruct->dwExStyle);

            p_platform_window->_hwnd = hwnd;
            p_platform_window->_hdc = hdc;

			SetWindowLongPtr(hwnd, 0, (LONG_PTR)p_platform_window);

            if (p_platform_window->_p_callback_on_created) {
                p_platform_window->_p_callback_on_created(p_platform_window, p_platform_window->_p_callback_on_created_user_ptr);
            }

            break;
        }

        case WM_CLOSE: {
            platform_window_destroy(p_platform_window);
            
            if (p_platform_window->_p_callback_on_close) {
                p_platform_window->_p_callback_on_close(p_platform_window, p_platform_window->_p_callback_on_close_user_ptr);
            }
            break;
        }

        case WM_SETFOCUS: {
            if (p_platform_window->_p_callback_on_focus_change) {
                p_platform_window->_p_callback_on_focus_change(p_platform_window, p_platform_window->_p_callback_on_focus_change_user_ptr, 1);
            }
            break;
        }

        case WM_KILLFOCUS: {
            if (p_platform_window->_p_callback_on_focus_change) {
                p_platform_window->_p_callback_on_focus_change(p_platform_window, p_platform_window->_p_callback_on_focus_change_user_ptr, 0);
            }
            break;
        }

        case WM_SIZE: {
            if (p_platform_window->_p_callback_on_resize) {
                p_platform_window->_p_callback_on_resize(p_platform_window, p_platform_window->_p_callback_on_resize_user_ptr, LOWORD(lparam), HIWORD(lparam));
            }
            break;
        }

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP: {
            if (!p_platform_window->_p_callback_on_key) {
                break;
            }
                
			WORD vk = LOWORD(wparam);

			WORD repeat_count = LOWORD(lparam);

			WORD flags = HIWORD(lparam);
			
			WORD scancode = LOBYTE(flags);
			if ((flags & KF_EXTENDED) == KF_EXTENDED) {
				scancode = MAKEWORD(scancode, 0xE0);
            }

			BOOL b_is_up = (flags & KF_UP) == KF_UP;

			if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) {
				vk = LOWORD(MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK_EX));
            }

            const enum key key = win32_vk_to_key(vk);

			for (WORD i = 0; i < repeat_count; ++i) {
                p_platform_window->_p_callback_on_key(p_platform_window, p_platform_window->_p_callback_on_key_user_ptr, key, !b_is_up);
            }

			break;
		}

        case WM_LBUTTONDOWN: {
            if (p_platform_window->_p_callback_on_mouse_button) {
                p_platform_window->_p_callback_on_mouse_button(p_platform_window, p_platform_window->_p_callback_on_mouse_button_user_ptr, MOUSE_BUTTON_left, 1);
            }
            break;
        }

        case WM_LBUTTONUP: {
            if (p_platform_window->_p_callback_on_mouse_button) {
                p_platform_window->_p_callback_on_mouse_button(p_platform_window, p_platform_window->_p_callback_on_mouse_button_user_ptr, MOUSE_BUTTON_left, 0);
            }
            break;
        }

        case WM_RBUTTONDOWN: {
            if (p_platform_window->_p_callback_on_mouse_button) {
                p_platform_window->_p_callback_on_mouse_button(p_platform_window, p_platform_window->_p_callback_on_mouse_button_user_ptr, MOUSE_BUTTON_right, 1);
            }
            break;
        }

        case WM_RBUTTONUP: {
            if (p_platform_window->_p_callback_on_mouse_button) {
                p_platform_window->_p_callback_on_mouse_button(p_platform_window, p_platform_window->_p_callback_on_mouse_button_user_ptr, MOUSE_BUTTON_right, 0);
            }
            break;
        }

        case WM_MBUTTONDOWN: {
            if (p_platform_window->_p_callback_on_mouse_button) {
                p_platform_window->_p_callback_on_mouse_button(p_platform_window, p_platform_window->_p_callback_on_mouse_button_user_ptr, MOUSE_BUTTON_middle, 1);
            }
            break;
        }

        case WM_MBUTTONUP: {
            if (p_platform_window->_p_callback_on_mouse_button) {
                p_platform_window->_p_callback_on_mouse_button(p_platform_window, p_platform_window->_p_callback_on_mouse_button_user_ptr, MOUSE_BUTTON_middle, 0);
            }
            break;
        }

        case WM_MOUSEMOVE: {
            p_platform_window->_mouse_pos[0] = (SHORT)LOWORD(lparam);
            p_platform_window->_mouse_pos[1] = (SHORT)HIWORD(lparam);
            if (p_platform_window->_p_callback_on_mouse_move) {
                const int delta_x = p_platform_window->_mouse_pos[0] - p_platform_window->_mouse_pos_old[0];
                const int delta_y = p_platform_window->_mouse_pos[1] - p_platform_window->_mouse_pos_old[1];
                p_platform_window->_p_callback_on_mouse_move(p_platform_window, p_platform_window->_p_callback_on_mouse_move_user_ptr, delta_x, delta_y);
            }
            break;
        }

        case WM_SETCURSOR: {
            DefWindowProc(hwnd, msg, wparam, lparam);
            break;
        }

        default: {
            result = DefWindowProc(hwnd, msg, wparam, lparam);
            break;
        }
    }

    return result;
}

enum key win32_vk_to_key(WORD vk) {
    if (vk >= '0' && vk <= '9') {
        return KEY_0 + (vk - '0');
    }

    if (vk >= 'A' && vk <= 'Z') {
        return KEY_a + (vk - 'A');
    }

    switch (vk) {
        case VK_SPACE:    return KEY_space;
        case VK_LCONTROL: return KEY_ctrl_left;
        case VK_RCONTROL: return KEY_ctrl_right;
        case VK_LSHIFT:   return KEY_shift_left;
        case VK_RSHIFT:   return KEY_shift_right;
        case VK_RETURN:   return KEY_enter;
        case VK_BACK:     return KEY_backspace;
        case VK_LEFT:     return KEY_left;
        case VK_RIGHT:    return KEY_right;
        case VK_UP:       return KEY_up;
        case VK_DOWN:     return KEY_down;
        case VK_DELETE:   return KEY_delete;
    }

    return KEY_unknown;
}

#endif