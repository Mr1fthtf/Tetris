#include "control.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32 //-----Windows------

#include <windows.h>
#define random() rand()

#else //--------------Linux-------

#include <signal.h>
#include <linux/input.h>
#include <sys/time.h>

#endif //WIN32----------------

static int judge_shape(int n,int m,int x,int y);
static void store_shape();
// static void init_shape();
static void new_shape();
static void destroy_line();
static int is_over();
static void move_shape_down();
static void fall_down();
static void move_shape_left();
static void move_shape_right();
static void change_shape();

int tm = 800000;

int p_x = 60,p_y = 15;
int matrix[24][28] = {0};
int score,level = 1;


#ifdef WIN32 //----------------Windows----------------------

#define TIMER_ID 1
void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	catch_signal((int)dwTime);
	sleep(1);
}

DWORD CALLBACK ThreadProc(PVOID pvoid)
{
	//强制系统为线程简历消息队列
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	SetTimer(0, TIMER_ID, 800, TimerProc);

	//获取并分发消息
	while(GetMessage(&msg, NULL, 0, 0))
	{
		if(msg.message == WM_TIMER)
		{
			TranslateMessage(&msg);    // 翻译消息
			DispatchMessage(&msg);     // 分发消息
		}
	}

	KillTimer(NULL, 10);
	printf("thread end here\n");
	return 0;
}

void alarm_us(int t)
{
	//KillTimer(NULL, TIMER_ID);
	//SetTimer(NULL, TIMER_ID, t / 1000, TimerProc);
}
void close_alarm()
{
	//KillTimer(NULL, TIMER_ID);
}

#else //-------------------------Linux-----------------------

//寰瀹氭椂鍣�,瀹氭椂鍣ㄤ竴鏃﹀惎鍔紝浼氭瘡闅斾竴娈垫椂闂村彂閫丼IGALRM淇″彿
void alarm_us(int t)
{
	struct itimerval value;
	//瀹氭椂鍣ㄥ惎鍔ㄧ殑鍒濆鍊�
	value.it_value.tv_sec = 0;
	value.it_value.tv_usec = t;

	//瀹氭椂鍣ㄥ惎鍔ㄥ悗鐨勯棿闅旀椂闂村��
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = t;

	setitimer(ITIMER_REAL,&value,NULL);
}

//鍏抽棴瀹氭椂鍣�
void close_alarm()
{
	struct itimerval value;
	//瀹氭椂鍣ㄥ惎鍔ㄧ殑鍒濆鍊�
	value.it_value.tv_sec = 0;
	value.it_value.tv_usec = 0;

	//瀹氭椂鍣ㄥ惎鍔ㄥ悗鐨勯棿闅旀椂闂村��
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL,&value,NULL);
}

struct termios tm_old;
//鑾峰彇涓�涓瓧绗﹁�� 涓嶅洖鏄�
int getch()
{
	struct termios tm;
	tcgetattr(0, &tm_old);
	cfmakeraw(&tm);
	tcsetattr(0, 0, &tm);
	int ch = getchar();
	tcsetattr(0, 0, &tm_old);

	return ch;
}

#endif //WIN32-------------------------------------------

//淇″彿娉ㄥ唽鍑芥暟
void catch_signal(int signo)
{
	//鍚戜笅绉诲姩鍥惧舰锛屼竴鐩村埌搴曢儴
	move_shape_down(num,mode,color);
	// signal(SIGALRM,catch_signal);
}

static void xyconsoletobox(int *a, int *b)
{
	*a = (*a - 12) / 2;
	*b = *b - 6;
}

//纰版挒妫�娴�,妫�娴嬫柟鍧楁槸鍚︾鎾炲埌杈圭晫鎴栧叾浠栨柟鍧�
static int judge_shape(int n,int m,int a,int b)
{
	int xx, yy;
	int *sp = shape[n][m];

	xyconsoletobox(&a, &b);

	if(a < 0) return 1; //宸﹁竟鐣�
	if(a > 28 / 2 - (4 - sp[16])) return 1; //鍙宠竟鐣�
	if(b > 24 - (4 - sp[17])) return 1; //涓嬭竟鐣�

	/* 琛� 鍑弒p[17]鏄负浜嗛伩鍏嶅浘褰㈠埌杈捐竟鐣屽垽鏂璵atrix鏁扮粍瓒婄晫
	 *
	 *      [][]#
	 *      [][]#
	 *
	 * 鍋囪濡傚浘#浠ｈ〃澧欏锛岀敯瀛楁柟鍧楀埌杈惧彸杈圭晫锛�
	 * 浣嗘槸鐢板瓧鏂瑰潡鍙宠竟鐨勬柟鍧椾笉闇�瑕佽�冭檻涔熶笉鍙互鑰冭檻
	 * 杩欐椂matrix[b][a]涓í鍧愭爣(a + 3) * 2浼氳秺鐣�
	 */
	for(yy = 0; yy < 4 - sp[17]; yy++)
	{
		for(xx = 0; xx < 4 - sp[16]; xx++)  //鍒�
		{
			if((sp[yy * 4 + xx] & matrix[b + yy][(a + xx) * 2]))
			{
				return 1;
			}
		}
	}
	return 0;
}



//淇濆瓨鏂瑰潡妫嬬洏淇℃伅锛屾柟鍧楄惤鍦板悗锛岄渶灏嗘暣涓柟鍧楀浘褰㈣繘琛屼繚瀛�
static void store_shape()
{
	int i,  j;
	int a, b;
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			if (shape[num][mode][4 * i + j] == 1)
			{
				a = x + 2 * j;
				b = y + i;
				xyconsoletobox(&a, &b);
				matrix[b][a * 2] = 1;
				matrix[b][a * 2 + 1] = color;
			}
		}
	}
}



// //鍒濆鍖栨鐩�
// static void init_shape()
// {
// 	//褰撴柟鍧椾笅钀藉埌搴曢儴鏃讹紝鎺у埗鍖轰骇鐢熸柊鐨勬柟鍧�
// 	//鎵撳嵃涓嬬Щ鏂瑰潡
// }

//鐢熸垚鏂板浘褰�
static void new_shape()
{
	x = i_x;
	y = i_y;
	num = n_num;
	mode = n_mode;
	color = n_color;
	n_num = random()%(6-0+1)+0;
	n_mode = random()%(3-0+1)+0;
	n_color = random()%(46-41+1)+41;
}



//娑堣妫�娴嬶紝褰撲竴琛屾弧鏃讹紝杩涜娑堣鎿嶄綔
static void destroy_line()
{
	int i, j, k;
	int neederase;
	int lines = 0;
	for (i = 23; i >= 0; )
	{
		neederase = 1;
		for (j = 0; j < 28; j += 2)
		{
			if (matrix[i][j] == 0)
			{
				neederase = 0;
				break;
			}
		}
		if (neederase == 1)
		{
			lines++;
			for (j = i; j > 0; j--)
			{
				for (k = 0; k < 28; k++)
				{
					matrix[j][k] = matrix[j - 1][k];
				}
			}
		}
		else
			i--;
	}
	score += 10 * lines;
}



//鍒ゆ柇娓告垙鏄惁缁撴潫
static int is_over()
{
	int i;
	for (i = 5; i < 9; i++)
	{
		if (matrix[0][i * 2] == 1)
			return 1;
	}
	return 0;
}



//鍥惧舰涓嬭惤鍑芥暟
static void move_shape_down()
{
	//棣栧厛鍒ゆ柇锛屽浘褰㈡湁娌℃湁瑙﹀簳
	if (judge_shape(num, mode, x, y + 1) == 1)		//瑙﹀簳
	{
		//淇濆瓨鐜版湁鍥惧舰
		store_shape();
		//娑堣
		destroy_line();
		//閲嶆柊鎵撳嵃鍦板浘銆佸垎鏁�
		print_matrix();
		print_score_level();
		//鍒ゆ柇娓告垙鏄惁缁撴潫
		if (is_over() == 1)
		{
			game_over();
			close_alarm();
		}
		//鐢熸垚鏂板浘褰�
		eraser_shape(n_num, n_mode, n_x, n_y);
		new_shape();
		print_mode_shape(num, mode, x, y, color);
		print_mode_shape(n_num, n_mode, n_x, n_y, n_color);
		// close_alarm();
	}
	else									//绉诲姩鍚庝笉浼氳Е搴�
	{
		//鍏堟竻鐞嗗師鏈夊浘褰�
		eraser_shape(num, mode, x, y);
		y++;
		print_mode_shape(num, mode, x, y, color);
	}
}



static void fall_down()
{
	//鐩存帴钀藉湴
	int step = 1;
	for (step; judge_shape(num, mode, x, y + step) != 1; step++) ;
	step--;
	eraser_shape(num, mode, x, y);
	y += step;
	print_mode_shape(num, mode, x, y, color);
}



//鏂瑰潡宸︾Щ
static void move_shape_left()
{
	//濡傛灉瓒婄晫锛岃繑鍥�
	//妫�娴嬬鎾�
	if (judge_shape(num, mode, x - 2, y) == 1)		//鎾炲
	{
		return ;
	}
	//娑堥櫎褰撳墠鍥惧舰 宸︾Щ 鎵撳嵃
	else
	{
		//鍏堟竻鐞嗗師鏈夊浘褰�
		eraser_shape(num, mode, x, y);
		x -= 2;
		print_mode_shape(num, mode, x, y, color);
	}
}



//鏂瑰潡鍙崇Щ
static void move_shape_right()
{
	//濡傛灉瓒婄晫锛岃繑鍥�
	//妫�娴嬬鎾�
	if (judge_shape(num, mode, x + 2, y) == 1)		//鎾炲
	{
		return ;
	}
	//娑堥櫎褰撳墠鍥惧舰 宸︾Щ 鎵撳嵃
	else
	{
		//鍏堟竻鐞嗗師鏈夊浘褰�
		eraser_shape(num, mode, x, y);
		x += 2;
		print_mode_shape(num, mode, x, y, color);
	}
}



//鏂瑰潡鍙樺舰
static void change_shape()
{
	//鑾峰彇涓嬩竴涓彲鍙樻崲鐨勭姸鎬�
	int tm = mode < 3 ? mode + 1 : 0;
	if(judge_shape(num, tm, x, y) == 1)
	{
		//鍙樻崲浜х敓纰版挒
	}
	else
	{
		//鎿﹂櫎鍘熸潵鐨勫舰鐘�
		eraser_shape(num, mode, x, y);
		//鎵撳嵃鏂板舰鐘�
		print_mode_shape(num, tm, x, y, color);
		//淇敼褰撳墠褰㈢姸涓烘柊褰㈢姸
		mode = tm;
	}
}

void key_control()
{
	static int count;
	int ch;
	//q閫�鍑�
	//鍥炶溅鐩存帴鍒板簳
	//绌烘牸鏆傚仠
	//涓� 鏂瑰潡鏃嬭浆
	//涓� 鏂瑰潡涓嬬Щ
	//宸� 鏂瑰潡宸︾Щ
	//鍙� 鏂瑰潡鍙崇Щ

#ifdef WIN32 //---------------Windows------------
	DWORD threadId;
	HANDLE tHandle = CreateThread(NULL, 0, ThreadProc, 0, 0, &threadId);
	
	while (1)
	{
		ch = getch();
		switch (ch)
		{
			case 72://KEY_UP:
				change_shape();
				break;
			case 80://KEY_DOWN:
				move_shape_down();
				break;
			case 75://KEY_LEFT:
				move_shape_left();
				break;
			case 77://KEY_RIGHT:
				move_shape_right();
				break;
			case 13://KEY_ENTER:
				fall_down();
				break;
			case 32://KEY_SPACE:
				// close_alarm();
				break;
			case 113://KEY_Q:
				game_over();
				clear();
				printf("\r\n");
				return;
				break;
			default:
				break;
		}
	}

#else

	while (1)
	{
		ch = getch();
		switch (ch)
		{
			case 65://KEY_UP:
				change_shape();
				break;
			case 66://KEY_DOWN:
				move_shape_down();
				break;
			case 68://KEY_LEFT:
				move_shape_left();
				break;
			case 67://KEY_RIGHT:
				move_shape_right();
				break;
			case 13://KEY_ENTER:
				fall_down();
				break;
			case 32://KEY_SPACE:
				// close_alarm();
				break;
			case 113://KEY_Q:
				game_over();
				clear();
				printf("\r\n");
				return;
				break;
			default:
				break;
		}
	}

#endif //WIN32-----------------------------------


}




