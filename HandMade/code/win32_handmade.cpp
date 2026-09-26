/* win32_handmade.cpp —— Win32 平台层
   详细说明见项目里的 explain\ 目录(按天的工作日志 + 按集的视频讲解)
   本工程字符集是 MultiByte,字符串直接写 "..." 即可,不需要 L"..." */

#include <windows.h>
#include <stdint.h>
#include <xinput.h>//Xinput.h` 只有类型和声明,没有实现
#include <dsound.h>//声音

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


struct win32_offscreen_buffer
{
	BITMAPINFO Info;     // 描述这块内存长什么样(给 StretchDIBits 看的说明书)
	void* Memory;        // ★ 我们自己分配的像素内存
	int Width;           // 缓冲尺寸(= 客户区尺寸)
	int Height;
    int Pitch;          //每行像素的字节
};

//想要返回两个值的时候，直接创建一个结构体就行
struct win32_window_dimension
{
	int Width;           // 缓冲尺寸(= 窗口尺寸)
	int Height;
};

/*动态加载库*///**  Run-Time Dynamic Linking  **  运行时动态链接
/*输入*/
//NOTE: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(_In_  DWORD  dwUserIndex,_Out_ XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
//typedef DWORD WINAPI x_input_get_state(_In_  DWORD  dwUserIndex,_Out_ XINPUT_STATE* pState)
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);//设备未连接
}//DWORD __stdcall XInputGetStateStub(DWORD dwUserIndex, XINPUT_STATE* pState) { return ERROR_DEVICE_NOT_CONNECTED; }
global_variable x_input_get_state* XInputGetState_=XInputGetStateStub;//_:动态加载
#define XInputGetState XInputGetState_

//NOTE: XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(_In_ DWORD   dwUserIndex, _In_ XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state* XInputSetState_=XInputSetStateStub;
#define XInputSetState XInputSetState_

/*声音*/
//NOTE:DirectSoundCreate
//extern _Check_return_ HRESULT WINAPI DirectSoundCreate(_In_opt_ LPCGUID pcGuidDevice, _Outptr_ LPDIRECTSOUND* ppDS, _Pre_null_ LPUNKNOWN pUnkOuter);
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(_In_opt_ LPCGUID pcGuidDevice, _Outptr_ LPDIRECTSOUND* ppDS, _Pre_null_ LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

/* 次缓冲:游戏主循环每帧都要往它里面写音频样本 ⇒ 必须是【全局】的。
   它和 DirectSound 对象的区别:对象只在初始化里用,缓冲要在初始化【之外】被用到,
   所以"谁活得更久"这件事是由"谁在后面还要用"决定的(同 XInputGetState_ 是全局的理由)。 */
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;


global_variable int    BytesPerSample = sizeof(int16) * 2;  /* 16 位 × 2 声道 = 4 字节/采样帧 */
global_variable uint32 RunningSampleIndex = 0;              /* ★ 必须全局:永远递增,跨帧不归零 */
global_variable int16  ToneVolume = 3000;                   /* 音量(相对 32767 很小,不刺耳) */
global_variable int    SecondaryBufferSize;                 /* ★ 必须 == 创建次缓冲时的 dwBufferBytes */


global_variable int BytesPerPixel = 4;     //每像素 4 字节(32bpp)
global_variable bool Running;              // 是否在跑    
global_variable win32_offscreen_buffer GlobalBackBuffer;

/*输入*/
internal void 
Win32LoadXinput()
{
    //HMODULE 就是 DLL 的基地址
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary) { 
        //TODO:还要诊断一下，至少知道用的啥系统
        XInputLibrary = LoadLibraryA("xinput1_3.dll"); 
    }
    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary,"XInputGetState");
        if (!XInputGetState) { XInputGetState = XInputGetStateStub; }

        XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary,"XInputSetState");
        if (!XInputSetState) { XInputSetState = XInputSetStateStub; }
    }
    else {
        //TODO:日志记录错误
    }
}
/*声音*/
internal void
Win32InitSound(HWND Window,int32 BufferSize)
{
    /*加载 dsound.dll(缺了只是没声音;故意不 FreeLibrary)。 */
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");//DLL文件基地址
    if (DSoundLibrary)
    {
        //从指定的动态链接库 (DLL) 中检索导出的函数（也称为过程）或变量的地址。
        direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

           //返回 HRESULT ⇒ 用 SUCCEEDED;&& 保证空指针时不会调用。 */
        LPDIRECTSOUND DirectSound = 0;//指向响应此函数而创建的 DirectSound 对象的指针的地址。
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
            //当我们拿回指向 DirectSound 的句柄时，DirectSound 拥有一个指向 DirectSound DLL内部虚表的指针
        {


			/* 格式与缓冲长度:放函数作用域,主缓冲和次缓冲都要用。 */
			uint32 SamplesPerSecond = 48000;
			uint32 SecondsToWrite = 2;       /* 2 秒 = 抗卡顿的减震器 */


			/* ⚠️ 先算 nBlockAlign 再算 nAvgBytesPerSec(结构体里顺序是反的,否则 E_INVALIDARG)。 */
			//WAVEFORMATEX 结构定义了波形音频数据的格式。
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;   /* = 1 */ //波形音频格式类型。
			WaveFormat.nChannels = 2; //波形音频数据中的声道数。单声道数据使用一个声道，立体声数据使用两个声道。
			WaveFormat.nSamplesPerSec = SamplesPerSecond;//每秒采样数
			WaveFormat.wBitsPerSample = 16;//wFormatTag 格式类型的每个样本的位数。
			//块对齐，以字节为单位。块对齐是 wFormatTag 格式类型的最小数据原子单元。
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;   /* = 4 */
			//格式标签所需的平均数据传输速率（以字节 / 秒为单位）。
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;       /* = 192000 */
			WaveFormat.cbSize = 0;


            //HRESULT 是一种用于 Windows 编程的 32 位整数数据类型，用于报告函数或方法的完成状态或错误代码
            HRESULT Error = DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY);
             //SetCooperativeLevel 是 DirectSound 内部虚表的一个虚函数
            if (SUCCEEDED(Error))//返回DD_OK
            {


                //**  主缓冲  只是拿到"声卡"的句柄  **  调 `SetFormat` 告诉 Windows"按这个格式播"
                DSBUFFERDESC BufferDescription = {};//DS缓冲区
                BufferDescription.dwSize = sizeof(BufferDescription);//结构体大小
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;//区分主次缓冲区
                BufferDescription.dwBufferBytes = 0;//字节大小
                BufferDescription.lpwfxFormat = 0;
                //指定缓冲区波形格式的 WAVEFORMATEX 或 WAVEFORMATEXTENSIBLE 结构的地址。对于主缓冲区，此值必须为 NULL。

				LPDIRECTSOUNDBUFFER PrimaryBuffer = 0;//接收新缓冲区对象 IDirectSoundBuffer 接口的变量地址
                Error = DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0);
				if (SUCCEEDED(Error))
				{

					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))//设置流中的格式信息
					{

					}
                    else {
						OutputDebugStringA("DirectSound: primary buffer SetFormat failed\n");
                    }
				}
				else{
					OutputDebugStringA("DirectSound: primary buffer creation failed\n");
				}

            }


            /* 次缓冲 —— 真正写样本的内存
                           ⚠️ 输出到全局:每帧的 Lock/Unlock 在别的函数里。 */
            DSBUFFERDESC SecondaryDesc = {};
            SecondaryDesc.dwSize = sizeof(SecondaryDesc);
            SecondaryDesc.dwFlags = 0;
            SecondaryDesc.dwBufferBytes = BufferSize;
            SecondaryDesc.lpwfxFormat = &WaveFormat;

            Error = DirectSound->CreateSoundBuffer(&SecondaryDesc, &GlobalSecondaryBuffer, 0);
            if (SUCCEEDED(Error))
            {
                //次缓冲不用设置格式信息，直接传入缓冲区

            }
            else {
                OutputDebugStringA("DirectSound: secondary buffer creation failed\n");
            }
        }
        else
        {
            OutputDebugStringA("DirectSound: DirectSoundCreate failed (no sound device?)\n");
        }
    }
    else
    {
        OutputDebugStringA("DirectSound: dsound.dll not loaded - running without sound\n");
    }
}


internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);

	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}


/* 把整个后备缓冲渲染成"怪渐变":蓝通道跟着 X 变、绿通道跟着 Y 变,再加上偏移量。
   偏移量每帧 +1,于是画面看起来在滚动 —— 这就是"动画"的全部秘密。
   (Casey 后来说它其实该叫 BlueOffset / GreenOffset 更准确:我们偏移的是**颜色值**,不是坐标。
    另外每个通道只有 8 位,所以每 256 个像素颜色会回绕一次 → 屏幕上一道一道的彩条。) */
internal void
RenderWeirdGradient(win32_offscreen_buffer *Buffer,int Xoffset, int Yoffset)
{
    if (!Buffer->Memory) return;
    uint8* Row = (uint8*)Buffer->Memory;
    for (int Y = 0; Y < Buffer->Height; ++Y)
    {
        uint32* Pixel = (uint32*)Row;
        for (int X = 0; X < Buffer->Width; ++X)
        {
            uint8 Blue  = (uint8)(X + Xoffset);
            uint8 Green = (uint8)(Y + Yoffset);

            /*
                内存：  BB GG RR xx  “小端机器”
                寄存器：xx RR GG BB
                (见 Day 004 文档第五节)
               —— 这里用"一次写 32 位",而不是逐字节写 4 次 ——
               一个像素 = 一个 32 位值。小端机器上"32 位值的最低 8 位"正好落在内存的第一个字节,所以:
                   蓝   → 最低 8 位        绿 → 左移 8 位        红 → 左移 16 位(还没用)   填充 → 最高 8 位
               为什么值得这么写:每像素从"4 次字节写 + 4 次指针自增"变成"1 次 32 位写 + 1 次自增"。
               实测 1280x655 每帧:逐字节 2.49ms → 一次写 1.06ms(不优化,快 2.35 倍);
                                   开优化后也还快 1.22 倍。详见 Day 004 文档第五节。 */
            *Pixel++ = ((uint32)Green << 8) | (uint32)Blue;
        }
        Row += Buffer->Pitch;
    }
}



internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer,int Width, int Height)//想修改Buffer的话还是要传指针
{
    /* 窗口每改一次大小都会进来,所以先把上一块内存还给系统,否则拖一次窗口就泄漏一块内存。
       VirtualFree 的 MEM_RELEASE = 整块释放;大小传 0,它自己记得当初分配了多少。 */
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
        Buffer->Memory = 0;
    }

    Buffer->Width  = Width;
    Buffer->Height = Height;

	Buffer->Pitch = Width * BytesPerPixel;


    /* 填 BITMAPINFOHEADER:描述这块位图长什么样 */
    Buffer->Info.bmiHeader.biSize        = sizeof(Buffer->Info.bmiHeader); // 只算"头"的大小,不含颜色表
    Buffer->Info.bmiHeader.biWidth       = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight      = -Buffer->Height;  // ★ 负数 = 自上而下的 DIB(内存第 0 行就是画面最上面那行)
                                                         //   为什么要加这个负号:实测发现正数(自下而上)时,
                                                         //   内存第 0 行会被画在窗口最下面,渲染循环里的 Y 和屏幕的 Y 就反了
                                                         //   (渐变会上下颠倒)。取负之后两边一致。
    Buffer->Info.bmiHeader.biPlanes      = 1;              // 必须是 1(位平面是老概念)
    Buffer->Info.bmiHeader.biBitCount    = 32;             // 24 位够用,要 32 是为了 DWORD 对齐
    Buffer->Info.bmiHeader.biCompression = BI_RGB;         // 不压缩:我们要写得快、贴得也快
    /* 其余字段(biSizeImage / biXPelsPerMeter / biYPelsPerMeter / biClrUsed / biClrImportant)
       是全局结构体,默认就是 0,不用管 */

    /* ★ 自己分配内存:每像素 4 字节(32bpp),所以需要 宽 * 高 * 4 字节。
       VirtualAlloc 是按页(4096 字节)给的,所以实际拿到的会比要的多一点,无所谓。
       MEM_COMMIT = 真的要这块内存(而不是只占地址范围); 
       MEM_RESERVE:保留进程虚拟地址空间的一部分，而不在内存或磁盘上的分页文件中分配任何实际的物理存储空间。
       PAGE_READWRITE = 可读可写。 */
    int BitmapMemorySize = Buffer->Width * Buffer->Height * BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);//明确需要保留和提交内存，而不是仅仅保留


    //TODO:


}

/* 把后备缓冲贴到窗口上。
   名字里的 win32 不能省:UpdateWindow 是 Windows 自己的 API,重名会编译不过。 */
internal void
Win32DisplayBufferInWindow(HDC DeviceContext,     // 往哪儿画(窗口的 DC,"作画工具包")
            int WindowWidth, int WindowHeight,      // 窗口有多大
                  win32_offscreen_buffer *Buffer)
{
    if (!Buffer->Memory) return;   // 还没分配好(WM_SIZE 还没来过),先不画


    //TODO:宽高比校正

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
                  0, 0, Buffer->Width, Buffer->Height,   // 源:整个后备缓冲
                  Buffer->Memory, &Buffer->Info,         // 像素数据首地址 + 格式说明书
                  DIB_RGB_COLORS,                    // 要和创建时一致
                  SRCCOPY);                          // 直接覆盖,不做位运算
}

/* 窗口过程:系统把发给本窗口的每条消息都送进这里。
   CALLBACK = __stdcall —— 是"系统调用你",调用约定必须和系统一致。 */
LRESULT CALLBACK
WindowMainWindowCallback(HWND Window,      // 消息是发给哪个窗口的
           UINT Message,     // 消息编号(WM_xxx)
           WPARAM WParam,    // 消息附带的第 1 个参数(含义随消息变)
           LPARAM LParam)    // 消息附带的第 2 个参数(含义随消息变)
{
    LRESULT Result = 0;   // 返回值:含义随消息变;只有 default 分支会给它赋值

    /* 每个 case 都用 {} 包住:否则 case 里声明的变量属于整个 switch 的作用域,
       从别的 case 跳进来会跳过它们的初始化(编译器会警告 initialization is skipped) */
    switch (Message)
    {
        
        case WM_SIZE:   // 窗口尺寸变了
        {
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
		} break;
        
        case WM_DESTROY:  // 窗口正在销毁:发 WM_QUIT,让消息循环结束
        {
            Running = false;
        } break;
        /*
        case WM_SETCURSOR://鼠标消失
        {
            SetCursor(0);
        }break;
        */

        /*键盘控制*/
        case WM_SYSKEYDOWN://按下ALT再按下某键
        case WM_SYSKEYUP:
        case WM_KEYDOWN://按下某键
        case WM_KEYUP://松开
        {
            uint32 VKCode = (uint32)WParam;
            bool WasDown = ((LParam & (1 << 30)) != 0);//在这次按键事件中，该键原本是抬起还是按下
            bool IsDown = ((LParam & (1 << 31)) == 0);//现在呢

            /* ★ 过滤"按键重复":按住不放会反复收到 WM_KEYDOWN,那时 WasDown == IsDown == true;
               只有"状态真的变了"才是真正的一次按下 / 一次抬起。 */
            if (WasDown != IsDown)
            {
                if (VKCode == 'W')
                {
                }
                else if (VKCode == 'A')
                {
                }
                else if (VKCode == 'S')
                {
                }
                else if (VKCode == 'D')
                {
                }
                else if (VKCode == 'Q')
                {
                }
                else if (VKCode == 'E')
                {
                }
                else if (VKCode == VK_UP)
                {
                }
                else if (VKCode == VK_LEFT)
                {
                }
                else if (VKCode == VK_DOWN)
                {
                }
                else if (VKCode == VK_RIGHT)
                {
                }
                else if (VKCode == VK_ESCAPE)
                {
                    if (IsDown)
                    {
                    }
                    if (WasDown)
                    {
                    }
                }
                else if (VKCode == VK_SPACE)
                {
                }
            }
			bool AltKeyWasDown = ((LParam & (1 << 29)) != 0);
			if ((VKCode == VK_F4) && AltKeyWasDown)//ALT+F4关闭窗口
			{
				Running = false;
			}
        }break;

        case WM_PAINT:                  //绘制
        {
            /* 现在主循环每帧都在 blit,这里的贴图有点重复 —— 但 WM_PAINT 仍然**必须处理**:
               BeginPaint 会清掉"这块脏了"的标记,不处理的话系统会一直给你发 WM_PAINT(窗口疯狂重画)。
               (Paint.rcPaint 是系统给的"脏区域";我们现在整窗重画,所以用不上它。) */
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);


            /* 把后备缓冲贴到窗口上(取代了之前的 PatBlt 涂白涂黑)。
               Win32UpdateWindow 现在还多要一个"客户区矩形",用来知道窗口有多大 —— 现取一个给它。 */
            
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(DeviceContext, Dimension.Width,Dimension.Height, &GlobalBackBuffer);


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
WinMain(HINSTANCE hInstance,       
        HINSTANCE hPrevInstance,   
        LPSTR lpCmdLine,           
        int nShowCmd)             
{


    /*连接输入*/
    Win32LoadXinput();



    /* ① 窗口类登记表。= {0} 不能省:局部变量不清零就是栈上的垃圾值 */
    WNDCLASS WindowClass = {0};

    /*在这里修改位图大小，而不是在回调函数中？？？*/
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);   // 按新的客户区尺寸重建后备缓冲

    
    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;  //样式
    WindowClass.lpfnWndProc = WindowMainWindowCallback;   // 消息处理函数(最关键的一格)
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
        HWND Window = CreateWindowEx(0,                        // 扩展样式:不用
            WindowClassName,                                   // 用哪个窗口类(必须和注册时一致)
            "Handmade Hero",                                   // 标题栏上的文字
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,                  // 普通可缩放窗口 + 立刻显示出来
            CW_USEDEFAULT,                                     // X:让系统决定放哪儿
            CW_USEDEFAULT,                                     // Y:让系统决定放哪儿
            CW_USEDEFAULT,                                     // 宽:让系统决定
            CW_USEDEFAULT,                                     // 高:让系统决定
            0,                                                 // 父窗口:没有
            0,                                                 // 菜单:没有
            hInstance,                                         // 所属程序实例
            0);                                                // 额外的创建参数:不用
        if (Window)   // 创建失败会返回 0;别带着一个不存在的窗口进消息循环
        {

            /* 动画的偏移量:⚠️ 必须声明在循环**外面**!
               写在循环体里 = 每一帧都被重新初始化成 0,画面永远不动
               —— 这正是 Casey 在 Day 4 现场犯的错,记在这儿免得再踩。
               (Y 方向现在一直传 0;想让它也动起来,照 X 的样子 ++ 就行。) */
            int Xoffset = 0;
            int Yoffset = 0;



            int SamplesPerSecond = 48000;
            int Hz = 256;
			//int SquareWaveCount = 0;//方波计数器
			int SquareWavePeriod = SamplesPerSecond / Hz;        /* 187 */
			int HalfSquareWavePeriod = SquareWavePeriod / 2;     /* 93 ← 新增,写方波要用 */

			SecondaryBufferSize = SamplesPerSecond * BytesPerSample * 2;   /* 2 秒 = 384000 字节 */
			Win32InitSound(Window, SecondaryBufferSize);                   /* ← 传的是同一个值 */




			MSG Message;                              // 装一条消息的盒子
            Running = true;
            while (Running)   // 条件是个全局标志,WM_CLOSE / WM_DESTROY 里会把它设成 false
            {
                /* —— ① 先把消息队列**排空** ——
                   内层 while 会一直取到队列空为止。用 PeekMessage 而不是 GetMessage 的原因
                   见上面那段说明:它**不阻塞**,所以队列空了以后我们还能接着往下跑。 */

                while (PeekMessageA(&Message,             // 取一条消息填进这个盒子
                                    0, 0, 0,              // 三个 0 = 不过滤,什么消息都要
                                    PM_REMOVE))           // 取走它(而不是只看一眼留着)
                {
                    if (Message.message == WM_QUIT)
                    {
                        Running = false;      // PeekMessage 不会用"返回 0"告诉我们该退出,得自己查
                    }

					TranslateMessage(&Message);   // WM_KEYDOWN / WM_KEYUP 翻译成 WM_CHAR
					DispatchMessage(&Message);    // 投给 WindowProc —— 系统在这一步回调你
                }


                //键盘手柄
				DWORD dwResult;
				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
				{
					XINPUT_STATE ControllerState;
					ZeroMemory(&ControllerState, sizeof(XINPUT_STATE));

					// Simply get the state of the controller from XInput.
					dwResult = XInputGetState(ControllerIndex, &ControllerState);

					if (dwResult == ERROR_SUCCESS)
					{
						// Controller is connected
                        XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

                        bool Up = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        bool Down = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        bool Left = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        bool Right = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

                        bool Start = Pad->wButtons & XINPUT_GAMEPAD_START;
                        bool Back = Pad->wButtons & XINPUT_GAMEPAD_BACK;

                        //按下摇杆
                        //bool LeftThumb = Pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
                        //bool RightThumb = Pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
                        
                        //肩键
                        bool LeftShoulder = Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        bool RightShoulder = Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;

                        bool AButton = Pad->wButtons & XINPUT_GAMEPAD_A;
                        bool BButton = Pad->wButtons & XINPUT_GAMEPAD_B;
                        bool XButton = Pad->wButtons & XINPUT_GAMEPAD_X;
                        bool YButton = Pad->wButtons & XINPUT_GAMEPAD_Y;

                        //扳机。触发器模拟控件的当前值。 该值介于 0 和 255 之间。
						//BYTE  bLeftTrigger;
						//BYTE  bRightTrigger;
                        
                        //左操纵杆 x/y 轴值
                        SHORT StickX = Pad->sThumbLX;
						SHORT StickY = Pad->sThumbLY;

                        //右操纵杆 x/y 轴值
						//SHORT sThumbRX;
						//SHORT sThumbRY;


                        /*操控*/
                        Xoffset += StickX >>12;
                        Yoffset += StickY >>12;

                        if (AButton)
                        {
                            Yoffset += 2;
                        }

					}
					else
					{
                        //TODO:日志系统记录错误
						// Controller is not connected
					}
				}

                //手柄震动
                /*
                XINPUT_VIBRATION Vibration;
                Vibration.wLeftMotorSpeed = 65535;   // WORD,0~65535
                Vibration.wRightMotorSpeed = 65535;
                XInputSetState(0, &Vibration);*/


                /* —— ② 渲染:往我们自己的那块内存里写像素(此时屏幕上还看不到) —— */
                RenderWeirdGradient(&GlobalBackBuffer,Xoffset, Yoffset);



                //音效输出
				/* Lock 之前,先把这几样准备好 */
				DWORD PlayCursor;                     // GetCurrentPosition 的输出①
				DWORD WriteCursor;                    // 输出②(我们其实不用,但 API 要求给个地方放)
				DWORD ByteToLock;                     // ← 第 1 个参数
				DWORD BytesToWrite;                   // ← 第 2 个参数
				VOID* Region1;  DWORD Region1Size;    // ← 第 3、4 个参数
				VOID* Region2;  DWORD Region2Size;    // ← 第 5、6 个参数

				HRESULT LockResult = GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
				if (SUCCEEDED(LockResult))
				{
					/* ByteToLock / BytesToWrite 的算法 */
                    /* —— (1) 这次从哪个字节开始写 ——
   RunningSampleIndex 是【永远递增】的采样帧序号(全局,跨帧不归零),
   乘以"每个采样帧多少字节" = "已经写过多少字节",再 % 缓冲大小 ⇒ 折回环形缓冲的位置。 */
                    ByteToLock = (RunningSampleIndex * BytesPerSample) % SecondaryBufferSize;

                    /* —— (2) 这次要写多少字节:一直写到 play cursor 之前为止 —— */
                    if (ByteToLock > PlayCursor)
                    {
                        /* 我落在播放头【后面】:先写到缓冲末尾,再绕回头写到播放头 */
                        BytesToWrite = (SecondaryBufferSize - ByteToLock);
                        BytesToWrite += PlayCursor;
                    }
                    else if (ByteToLock == PlayCursor)
                    {
                        /* 特例:只有刚启动时两个游标才会相等(都是 0),
                           这时上面两条算出来都是 0 = 什么都不写 ⇒ 直接填满整个缓冲 */
                        BytesToWrite = SecondaryBufferSize;
                    }
                    else
                    {
                        /* 播放头在我前面:一段直写,不跨末尾 */
                        BytesToWrite = PlayCursor - ByteToLock;
                    }

						if (SUCCEEDED(GlobalSecondaryBuffer -> Lock(ByteToLock, BytesToWrite,
							                                        &Region1, &Region1Size,
							                                        &Region2, &Region2Size,
							                                        0)))          // ← dwFlags = 0
						{
							/* 往 Region1(+Region2)里写方波样本 */
                            /* —— (3) 写第一段 —— */
                            int16* SampleOut = (int16*)Region1;
                            DWORD Region1SampleCount = Region1Size / BytesPerSample;
                            for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
                            {
                                /* 方波:每 HalfSquareWavePeriod 个采样翻转一次符号 */
                                int16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2)
                                    ? ToneVolume : -ToneVolume;
                                *SampleOut++ = SampleValue;   /* 左声道 */
                                *SampleOut++ = SampleValue;   /* 右声道 —— 同值 = 单声道听感 */
                            }

                            /* —— (4) 写第二段:只有"跨过缓冲末尾"时才有第二块;
                                   没跨时 Region2 是 NULL / Region2Size 是 0,这个循环体一次都不会进 —— */
                            int16* SampleOut2 = (int16*)Region2;
                            DWORD Region2SampleCount = Region2Size / BytesPerSample;
                            for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
                            {
                                int16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2)
                                    ? ToneVolume : -ToneVolume;
                                *SampleOut2++ = SampleValue;  /* 左声道 */
                                *SampleOut2++ = SampleValue;  /* 右声道 */
                            }


							GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
						}
				}


                /* —— ③ 贴图:⚠️ 这一步不能少!只渲染不 blit,屏幕上什么都不会变
                       —— 这也是 Casey 在 Day 4 现场犯的第二个错。
                       GetDC 是"借"、ReleaseDC 是"还",必须成对出现。 —— */
                HDC DeviceContext = GetDC(Window);

                win32_window_dimension Dim = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(DeviceContext, Dim.Width, Dim.Height, &GlobalBackBuffer);

                ReleaseDC(Window, DeviceContext);


            }
		}
		else
		{
            // TODO: 失败时应该记一条日志(GetLastError)
		}
    }
    else 
    {
        // TODO: 失败时应该记一条日志(GetLastError)
    }
    

    return 0;
}
