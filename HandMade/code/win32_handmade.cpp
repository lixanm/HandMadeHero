/* win32_handmade.cpp —— Win32 平台层
   详细说明(每个字段、每条消息、踩过的坑)见 D:\HandMade\explain\代码说明.md
   本工程字符集是 MultiByte,字符串直接写 "..." 即可,不需要 L"..." */

#include <windows.h>

/* 窗口过程:系统把发给本窗口的每条消息都送进这里。
   CALLBACK = __stdcall —— 是"系统调用你",调用约定必须和系统一致。 */
LRESULT CALLBACK
WindowProc(HWND Window,
           UINT Message,
           WPARAM WParam,
           LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        
        case WM_SIZE:   // 窗口尺寸变了(Day 3 起在这里重建后备缓冲)
        {
            OutputDebugStringA("WM_SiZE\n");
        } break;

        case WM_CLOSE:  // 用户点了 X,交给默认处理去销毁窗口
        {
			OutputDebugStringA("WM_CLOSE\n");
        } break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;
        
        case WM_DESTROY:  // 窗口正在销毁:发 WM_QUIT,让消息循环结束
        {
			OutputDebugStringA("WM_DESTROY\n");
            PostQuitMessage(0);
        } break;


        case WM_PAINT:                  //绘制
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);

            /* rcPaint = 系统告诉你的"脏区域",只需要重画这一块 */
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width  = Paint.rcPaint.right  - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            /* static:只初始化一次,下次进来还记得上次用的是黑还是白。
               WHITENESS/BLACKNESS 是光栅操作码:直接把目标涂成白/黑,不需要选画刷 */
            static DWORD Operation = WHITENESS;
            PatBlt(DeviceContext, X, Y, Width, Height, Operation);
            if (Operation == WHITENESS)
            {
                Operation = BLACKNESS;
            }
            else
            {
                Operation = WHITENESS;
            }
			EndPaint(Window, &Paint);
        }break;

        default:  // 没处理的消息必须交回系统,否则窗口拖不动、重画乱套
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

int WINAPI
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nShowCmd)
{
    /* ① 窗口类登记表。= {0} 不能省:局部变量不清零就是栈上的垃圾值 */
    WNDCLASS WindowClass = {0};

    
    //WindowClass.style = /*CS_HREDRAW | CS_VREDRAW |*/ CS_OWNDC;  //样式也去掉
    WindowClass.lpfnWndProc = WindowProc;   // 消息处理函数(最关键的一格)
    WindowClass.hInstance = hInstance;
    //WindowClass.hIcon = LoadIcon(0, IDI_APPLICATION);//图标
    //WindowClass.hCursor = LoadCursor(0, IDC_ARROW);//光标，不需要
    WindowClass.hbrBackground = 0;          // 0 = 不让系统擦背景,画面自己画满
    WindowClass.lpszMenuName = 0;           // 没有菜单
    /* cbClsExtra / cbWndExtra 保持 0,用不到 */

    /* 类名存进变量:注册和创建必须用同一个字符串,写两遍容易差一个字母 */
    const char *WindowClassName = "HandmadeHeroWindowClass";
    WindowClass.lpszClassName = WindowClassName;

    /* ② 登记(返回 0 = 失败,可用 GetLastError 查原因) */
    if (RegisterClass(&WindowClass))
    {
		/* ③ 照表造窗口(12 个参数的含义见文档) */
		HWND Window = CreateWindowEx(0,
			WindowClassName,
			"Handmade Hero",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			hInstance,
			0);
		if (Window)
		{
            MSG Message;
            for (;;)
            {
                BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
                if (MessageResult > 0) {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                else {
                    break;
                }
            }
		}
		else
		{
            //todo
			OutputDebugStringA("CreateWindowEx failed\n");
		}
    }
    else 
    {
        //todo
    }
    

    return 0;
}
