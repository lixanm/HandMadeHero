/* win32_handmade.cpp —— Win32 平台层
   详细说明见项目里的 explain\ 目录(按天的工作日志 + 按集的视频讲解)
   本工程字符集是 MultiByte,字符串直接写 "..." 即可,不需要 L"..." */

#include <windows.h>
#include <stdint.h>

#define  local_persist static
#define  global_variable static
#define  internal static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;


/* ============================================================================
   后备缓冲(back buffer)
   我们**自己**分配一块内存当位图,往里写像素,再用 StretchDIBits 贴到窗口。
   渲染归我们自己,Windows 只负责显示。
   ⚠️ Day 4 更正:用 StretchDIBits 就**不需要 DC、也不需要位图句柄**了
      (它直接收像素指针),所以 HBITMAP / HDC 这两个东西已经删掉。
   ============================================================================ */
global_variable bool Running;              // 是否在跑    
global_variable BITMAPINFO BitmapInfo;     // 描述这块内存长什么样(给 StretchDIBits 看的说明书)
global_variable void *BitmapMemory;        // ★ 我们自己分配的像素内存
global_variable int BitmapWidth;           // 缓冲尺寸(= 客户区尺寸)
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;     //每像素 4 字节(32bpp)

/**/
internal void
RanderWeirdGradient(int Xoffset,int Yoffset)
{
    int Width = BitmapWidth;
    int Height = BitmapHeight;

    int Pitch = Width * BytesPerPixel;
    uint8* Row = (uint8*)BitmapMemory;
    for (int Y = 0; Y < BitmapHeight; ++Y)
    {
        uint8* Pixel = (uint8*)Row;
        for (int X = 0; X < BitmapWidth; ++X)
        {
            /* ⚠️ 内存里的实际顺序是「蓝 绿 红 填充」(B,G,R,A),**不是** R,G,B,A!
               原因:小端机器上"内存按 R,G,B 排"读进寄存器会变成 B,G,R,而 Windows 作者
               希望寄存器里看是 R,G,B,于是干脆把内存顺序定义成反的(见 Day 004 文档第五节)。
               顺着这条规律:第 1 个字节 = 蓝、第 2 个 = 绿、第 3 个 = 红。 */
            *Pixel++ = (uint8)(X + Xoffset);                    // 蓝
            *Pixel++ = (uint8)(Y + Yoffset);                    // 绿
            *Pixel++ = 0;                           // 红 
            *Pixel++ = 0;                           // 填充

        }
        Row += Pitch;
    }
}


internal void
Win32ResizeDIBSection(int Width, int Height)
{
    /* 窗口每改一次大小都会进来,所以先把上一块内存还给系统,否则拖一次窗口就泄漏一块内存。
       VirtualFree 的 MEM_RELEASE = 整块释放;大小传 0,它自己记得当初分配了多少。 */
    if (BitmapMemory)
    {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
        BitmapMemory = 0;
    }

    BitmapWidth  = Width;
    BitmapHeight = Height;

    /* 填 BITMAPINFOHEADER:描述这块位图长什么样 */
    BitmapInfo.bmiHeader.biSize        = sizeof(BitmapInfo.bmiHeader); // 只算"头"的大小,不含颜色表
    BitmapInfo.bmiHeader.biWidth       = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight      = -BitmapHeight;  // ★ 负数 = 自上而下的 DIB(内存第 0 行就是画面最上面那行)
                                                         //   为什么要加这个负号:实测发现正数(自下而上)时,
                                                         //   内存第 0 行会被画在窗口最下面,渲染循环里的 Y 和屏幕的 Y 就反了
                                                         //   (渐变会上下颠倒)。取负之后两边一致。
    BitmapInfo.bmiHeader.biPlanes      = 1;              // 必须是 1(位平面是老概念)
    BitmapInfo.bmiHeader.biBitCount    = 32;             // 24 位够用,要 32 是为了 DWORD 对齐
    BitmapInfo.bmiHeader.biCompression = BI_RGB;         // 不压缩:我们要写得快、贴得也快
    /* 其余字段(biSizeImage / biXPelsPerMeter / biYPelsPerMeter / biClrUsed / biClrImportant)
       是全局变量,默认就是 0,不用管 */

    /* ★ 自己分配内存:每像素 4 字节(32bpp),所以需要 宽 * 高 * 4 字节。
       VirtualAlloc 是按页(4096 字节)给的,所以实际拿到的会比要的多一点,无所谓。
       MEM_COMMIT = 真的要这块内存(而不是只占地址范围);PAGE_READWRITE = 可读可写。 */
    int BitmapMemorySize = BitmapWidth * BitmapHeight * BytesPerPixel;
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    RanderWeirdGradient(0, 0);

}

/* 把后备缓冲贴到窗口上。
   名字里的 win32 不能省:UpdateWindow 是 Windows 自己的 API,重名会编译不过。 */
internal void
Win32UpdateWindow(HDC DeviceContext, RECT *WindowRect, int X, int Y, int Width, int Height)
{
    if (!BitmapMemory) return;   // 还没分配好(WM_SIZE 还没来过),先不画

    int WindowWidth = WindowRect->right - WindowRect->left;
    int WindowHeight = WindowRect->bottom - WindowRect->top;

    /* 把缓冲里的 (X,Y,Width,Height) 贴到窗口的 (X,Y,Width,Height)。
       两边是同一套坐标(缓冲和客户区一样大、原点都在左上角),所以数值相同。
       以后要"固定分辨率渲染 + 放大到窗口",只要改这两个矩形 —— 这就是当初选
       StretchDIBits 而不是 BitBlt 的原因。 */
    /* ⚠️ StretchDIBits 的参数顺序是「先目标、后源」:
         第 2~5 个 = 目标(窗口上要画的那块)= 整个客户区
         第 6~9 个 = 源(我们缓冲里对应的那块)= 整个后备缓冲
       两边现在尺寸相同(缓冲就是按客户区建的),但**别写反** ——
       以后要"固定分辨率渲染 + 放大到窗口"时,写反就会画错。 */
    StretchDIBits(DeviceContext,
                  0, 0, WindowWidth, WindowHeight,   // 目标:整个客户区
                  0, 0, BitmapWidth, BitmapHeight,   // 源:整个后备缓冲
                  BitmapMemory, &BitmapInfo,
                  DIB_RGB_COLORS,                    // 要和创建时一致
                  SRCCOPY);                          // 直接覆盖,不做位运算
}

/* 窗口过程:系统把发给本窗口的每条消息都送进这里。
   CALLBACK = __stdcall —— 是"系统调用你",调用约定必须和系统一致。 */
LRESULT CALLBACK
WindowProc(HWND Window,
           UINT Message,
           WPARAM WParam,
           LPARAM LParam)
{
    LRESULT Result = 0;   // 返回值:含义随消息变;只有 default 分支会给它赋值

    /* 每个 case 都用 {} 包住:否则 case 里声明的变量属于整个 switch 的作用域,
       从别的 case 跳进来会跳过它们的初始化(编译器会警告 initialization is skipped) */
    switch (Message)
    {
        
        case WM_SIZE:   // 窗口尺寸变了(Day 3 起在这里重建后备缓冲)
        {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);

			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;

            Win32ResizeDIBSection(Width, Height);   // 按新的客户区尺寸重建后备缓冲
            OutputDebugStringA("WM_SiZE\n");
        } break;

        /* 用户点了 X / Alt+F4。
           ⚠️ 空的 case 会把"销毁窗口"这件默认处理的活拦下来(消息不再落到 default),
           所以这里必须自己 PostQuitMessage(0) 或 DestroyWindow,否则点 X 关不掉窗口 */
        case WM_CLOSE:
        {
            Running = false;
        } break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;
        
        case WM_DESTROY:  // 窗口正在销毁:发 WM_QUIT,让消息循环结束
        {
            Running = false;
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

            /* 把后备缓冲贴到窗口上(取代了之前的 PatBlt 涂白涂黑)。
               Win32UpdateWindow 现在还多要一个"客户区矩形",用来知道窗口有多大 —— 现取一个给它。 */
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
			EndPaint(Window, &Paint);
        }break;

        default:  // 没处理的消息必须交回系统,否则窗口拖不动、重画乱套
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

/* 程序入口(Windows 子系统找的是 WinMain,不是 main) */
int WINAPI
WinMain(HINSTANCE hInstance,       // 本程序的实例句柄:注册窗口类、创建窗口都要用它
        HINSTANCE hPrevInstance,   // Win32 里永远是 NULL(Win16 时代的遗留物)
        LPSTR lpCmdLine,           // 命令行参数(不含程序名),类型是 ANSI 字符串
        int nShowCmd)              // 窗口该怎么显示(我们用了 WS_VISIBLE,这里用不到)
{
    /* ① 窗口类登记表。= {0} 不能省:局部变量不清零就是栈上的垃圾值 */
    WNDCLASS WindowClass = {0};

    
    //WindowClass.style = /*CS_HREDRAW | CS_VREDRAW |*/ CS_OWNDC;  //样式也去掉(以后要长期持有一块 DC 时,CS_OWNDC 可能要加回来)
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
		if (Window)   // 创建失败会返回 0;别带着一个不存在的窗口进消息循环
		{
            Running = true;
            while (Running) // 无限循环(用 for(;;) 而不是 while(1):后者会触发"条件表达式不是常量"的警告)
            {
				BOOL PeekMessageA(
					[out]          LPMSG lpMsg,
					[in, optional] HWND  hWnd,
					[in]           UINT  wMsgFilterMin,
					[in]           UINT  wMsgFilterMax,
					[in]           UINT  wRemoveMsg
				);
                MSG Message;
                /* GetMessage:从本线程的消息队列取一条;队列空时会阻塞(让出 CPU,不烧 CPU)。
                   返回值 >0 = 取到消息 / 0 = 收到 WM_QUIT(该退出了) / -1 = 出错 */
                BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
                if (MessageResult > 0) {
                    TranslateMessage(&Message);   // WM_KEYDOWN / WM_KEYUP 翻译成 WM_CHAR
                    DispatchMessage(&Message);    // 投给 WindowProc —— 系统在这一步回调你
                }
                else {
                    break;                        // WM_QUIT 或出错:跳出循环,程序结束
                }
            }
		}
		else
		{
            // TODO: 失败时应该记一条日志(GetLastError)
			OutputDebugStringA("CreateWindowEx failed\n");
		}
    }
    else 
    {
        // TODO: 失败时应该记一条日志(GetLastError)
    }
    

    return 0;
}
