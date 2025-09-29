
#include "device_driver.h"
#include <stdlib.h>
#include <math.h>

#define MAX_BULLETS    (6)
#define BULLET_STEP    (5)
#define BULLET_SIZE_X  (2)
#define BULLET_SIZE_Y  (2)
#define BULLET_COLOR   (1)

#define LCDW			(320)
#define LCDH			(240)
#define X_MIN	 		(0)
#define X_MAX	 		(LCDW - 1)
#define Y_MIN	 		(0)
#define Y_MAX	 		(LCDH - 1)

#define TIMER_PERIOD	(10)
#define RIGHT			(1)
#define LEFT			(-1)
#define HOME			(0)
#define SCHOOL			(1)

#define MOB1_MAX		(5)
#define MOB_STEP		(1)
#define MOB_SIZE_X		(12)
#define MOB_SIZE_Y		(12)
#define MOB2_MAX		(3)
#define MOB2_STEP		(8)
#define MOB3_MAX		(4)
#define MAX_MOB3_BULLETS (4)
#define MOB_MAX			(MOB1_MAX + MOB2_MAX)

#define MAX_ITEM		(3)
#define ITEM_SIZE_X		(8)
#define ITEM_SIZE_Y		(8)
#define FREEZE_TIME		(40)


#define HERO_STEP		(15)
#define HERO_SIZE_X		(12)
#define HERO_SIZE_Y		(12)
#define HERO_LIFE		(10)

#define BACK_COLOR		(5)
#define MOB_COLOR		(4)
#define MOB2_COLOR		(0)
#define HERO_COLOR		(2)

#define GAME_OVER		(1)



typedef struct
{
	int x, y;
	int w, h;
	int ci;
	int dir1;
	int dir2;
	int life;
} QUERY_DRAW;

typedef struct {
	int active;  
	int x, y;
	int dir;     // 왼쪽:2, 위쪽:0, 오른쪽:3, 아래:1  
} BULLET;

typedef struct {
	int active;  
	int x, y;
	int w, h;
	int ci;
} ITEM;

typedef struct {
	int x, y;
	float dx, dy;  // 정수 방향 벡터
	int active;
} MOB_BULLET;

//===========구조체 배열 선언============
static BULLET bullets[MAX_BULLETS];  // 총알 배열
static MOB_BULLET mob_bullets[MAX_MOB3_BULLETS];  // 총알 배열

static QUERY_DRAW mobs[MOB1_MAX + MOB2_MAX];           // 적 mob_max명
static QUERY_DRAW mobs3[MOB3_MAX];
static QUERY_DRAW hero;

static ITEM items[MAX_ITEM];

static int score, stage = 1;
static unsigned short color[] = {RED, YELLOW, GREEN, BLUE, WHITE, BLACK};
static int mob_frozen = 0;
static int mob_freeze_timer = 0;


//===========================hero bullets============================
static void Bullet_Fire(int direction, int x, int y)
{
	int i;
	for(i = 0; i < MAX_BULLETS; i++) {
		if(!bullets[i].active) {
			bullets[i].active = 1;
			bullets[i].dir = direction;
			bullets[i].x = x + HERO_SIZE_X / 2 - BULLET_SIZE_X / 2;
			bullets[i].y = y + HERO_SIZE_Y / 2 - BULLET_SIZE_Y / 2;
			break;
		}
	}
}

static void Draw_Object(QUERY_DRAW * obj)
{
	Lcd_Draw_Box(obj->x, obj->y, obj->w, obj->h, color[obj->ci]);
}


static void Bullet_Update(void)
{
	int i, j;
	for(i = 0; i < MAX_BULLETS; i++) {
		if(bullets[i].active) {
			// 지운다
			Lcd_Draw_Box(bullets[i].x, bullets[i].y, BULLET_SIZE_X, BULLET_SIZE_Y, color[BACK_COLOR]);

			switch(bullets[i].dir) {
				case 0: bullets[i].y -= BULLET_STEP; break;  // 위쪽
				case 1: bullets[i].y +=	BULLET_STEP; break;  // 아래쪽
				case 2: bullets[i].x -=	BULLET_STEP; break;  // 왼쪽
				case 3: bullets[i].x += BULLET_STEP; break;  // 오른쪽
			}

			// 화면 밖이면 비활성화
			if(bullets[i].x < X_MIN || bullets[i].x > X_MAX ||
			   bullets[i].y < Y_MIN || bullets[i].y > Y_MAX) {
				bullets[i].active = 0;
			} else {
				// 다시 그린다
				Lcd_Draw_Box(bullets[i].x, bullets[i].y, BULLET_SIZE_X, BULLET_SIZE_Y, color[BULLET_COLOR]);
			}

			// 적과 충돌 확인
			for(j=0;j<MOB_MAX;j++){
				if(mobs[j].life>0){
				if(bullets[i].x < mobs[j].x + mobs[j].w && bullets[i].x + BULLET_SIZE_X > mobs[j].x &&
				bullets[i].y < mobs[j].y + mobs[j].h && bullets[i].y + BULLET_SIZE_Y > mobs[j].y) {
					bullets[i].active = 0;
					Lcd_Draw_Box(bullets[i].x, bullets[i].y, BULLET_SIZE_X, BULLET_SIZE_Y, color[BACK_COLOR]);
					mobs[j].life--;
					Uart_Printf("Enemy hit!\n");

					if (mobs[j].life <= 0) {
						mobs[j].ci = BACK_COLOR;
						Draw_Object(&mobs[j]);
					
						if (!mob_frozen) {
							// 바로 재생성
							mobs[j].ci = MOB_COLOR;
							mobs[j].x = rand() % (LCDH - MOB_SIZE_Y); 
							mobs[j].y = rand() % (LCDH - MOB_SIZE_Y);
							mobs[j].life = 1;
						}
						score++;
						Lcd_Printf(0, 0, RED, BLACK, 1, 1, "Score:%d", score);
					}
					}
				}
			}
		}
	}
}


//==========================mob3 function==============================
int mob3_positions[4][2] = {
    {0, 20},
    {LCDW - MOB_SIZE_X, 20},
    {0, LCDH - MOB_SIZE_Y},
    {LCDW - MOB_SIZE_X, LCDH - MOB_SIZE_Y}
};



void Mob3_Fire_All(void) {
    int i;
    for (i = 0; i < MOB3_MAX; i++) {
        if (mob_bullets[i].active == 0) {  // 총알이 비활성화 상태인 경우만 발사
            // 첫 번째로 발사되는 총알
            mob_bullets[i].active = 1;
            mob_bullets[i].x = mobs3[i].x + MOB_SIZE_X / 2;
            mob_bullets[i].y = mobs3[i].y + MOB_SIZE_Y / 2;

            // 방향 벡터 설정 (hero 위치 기준)
            int dx = hero.x + HERO_SIZE_X / 2 - mob_bullets[i].x;
            int dy = hero.y + HERO_SIZE_Y / 2 - mob_bullets[i].y;
            float len = sqrtf(dx * dx + dy * dy);
            mob_bullets[i].dx = (float)dx / len * BULLET_STEP;
            mob_bullets[i].dy = (float)dy / len * BULLET_STEP;
        }
    }
}



void Mob3_Bullet_Update(void) {
	int i;
	for (i = 0; i < MAX_MOB3_BULLETS; i++) {
		if (mob_bullets[i].active) {
			// 지우기
			Lcd_Draw_Box(mob_bullets[i].x, mob_bullets[i].y, BULLET_SIZE_X, BULLET_SIZE_Y, color[BACK_COLOR]);

			// 이동
			mob_bullets[i].x += (int)mob_bullets[i].dx;
			mob_bullets[i].y += (int)mob_bullets[i].dy;



			// 화면 밖
			if (mob_bullets[i].x < X_MIN || mob_bullets[i].x > X_MAX ||
				mob_bullets[i].y < Y_MIN || mob_bullets[i].y > Y_MAX) {
				mob_bullets[i].active = 0;
				continue;
			}

			// 다시 그리기
			Lcd_Draw_Box(mob_bullets[i].x, mob_bullets[i].y, BULLET_SIZE_X, BULLET_SIZE_Y, color[4]); 
		}
	}
}



static int Check_Collision(void)
{
	int i;
		int hit = 0; // 충돌 여부 저장
	
		// 몹과 충돌 체크
		for (i = 0; i < MOB_MAX; i++) {
			if (mobs[i].life <= 0) continue;
	
			if (hero.x < mobs[i].x + MOB_STEP &&
				hero.x + hero.w > mobs[i].x &&
				hero.y < mobs[i].y + MOB_STEP &&
				hero.y + hero.h > mobs[i].y) {
				hit = 1; // 충돌 발생
			}
		}
	
		// 총알 충돌 체크
		for (i = 0; i < MAX_MOB3_BULLETS; i++) {
			if (mob_bullets[i].active == 0) continue;  // 비활성화된 총알은 제외
	
			if (hero.x < mob_bullets[i].x + BULLET_SIZE_X &&
				hero.x + hero.w > mob_bullets[i].x &&
				hero.y < mob_bullets[i].y + BULLET_SIZE_Y &&
				hero.y + hero.h > mob_bullets[i].y) {
				
				Uart_Printf("Hit by MOB3 bullet!\n");
				mob_bullets[i].active = 0; // 총알 비활성화
				Lcd_Draw_Box(mob_bullets[i].x, mob_bullets[i].y, BULLET_SIZE_X, BULLET_SIZE_Y, color[5]);//총알 지우기
				hit = 1; // 충돌 발생
			}
		}
	
		return hit; // 한 프레임 내 모든 충돌 검사 후 결과 리턴
}




static int Car_Move(void)
{
	if (mob_frozen) return 0;  // 몬스터 정지 중이면 이동하지 않음

	int i, end = 0;
	for (i=0;i<MOB1_MAX;i++){
		if (mobs[i].x > hero.x){
			mobs[i].x -= MOB_STEP * mobs[i].dir1;
		}
		else if(mobs[i].x < hero.x){
			mobs[i].x += MOB_STEP * mobs[i].dir1;
		}

		if (mobs[i].y > hero.y){
			mobs[i].y -= MOB_STEP * mobs[i].dir2;
		}
		else if(mobs[i].y < hero.y){
			mobs[i].y += MOB_STEP * mobs[i].dir2;
		}
	}

	for (i=MOB1_MAX;i<MOB_MAX;i++){

		if(i%2 == 1){
			mobs[i].x -= MOB2_STEP * mobs[i].dir1;
			mobs[i].y -= MOB2_STEP * mobs[i].dir2;
		}
		else if(i%2 == 0){
			mobs[i].x += MOB2_STEP * mobs[i].dir1;
			mobs[i].y += MOB2_STEP * mobs[i].dir2;
		}
		if((mobs[i].x + mobs[i].w > X_MAX)||(mobs[i].x<=X_MIN)){
			mobs[i].dir1 *= -1;
		}
		if((mobs[i].y + mobs[i].h > Y_MAX)||(mobs[i].y<=Y_MIN)){
			mobs[i].dir2 *= -1;
		}
	}

	return Check_Collision();
}


static void k0(void)
{
	if(hero.y > Y_MIN) hero.y -= HERO_STEP;
}

static void k1(void)
{
	if(hero.y + hero.h < Y_MAX) hero.y += HERO_STEP;
}

static void k2(void)
{
	if(hero.x > X_MIN) hero.x -= HERO_STEP;
}

static void k3(void)
{
	if(hero.x + hero.w < X_MAX) hero.x += HERO_STEP;
}


///==============item function================
int Item_Check_Collision() {
	int i, col = 0;
    for (i = 0; i < MAX_ITEM; i++) {
        if (items[i].active == 1){
			if((items[i].x >= hero.x) && ((hero.x + HERO_STEP) >= items[i].x)) col |= 1<<0;
			else if((items[i].x < hero.x) && ((items[i].x + (items[i].w/2)) >= hero.x)) col |= 1<<0;
		
			if((items[i].y >= hero.y) && ((hero.y + HERO_STEP) >= items[i].y)) col |= 1<<1;
			else if((items[i].y < hero.y) && ((items[i].y + (items[i].w/2)) >= hero.y)) col |= 1<<1;
		}
        
		if(col == 3){
			items[i].active = 0; // 아이템 먹힘 처리

            if (i == 0) {
                hero.life++;  // 체력 회복
				score++;
            } 
			else if (i == 1) {
                hero.life++;  // 체력 회복
				score++;
            }
			else if (i == 2) {
				// 몬스터 정지 효과 시작
				mob_frozen = 1;
				mob_freeze_timer = FREEZE_TIME;  // 3초 (TIM4 주기 100ms 기준, 3초면 30카운트)
				score++;
			}
			Uart_Printf("item collision\n");
			Lcd_Draw_Box(items[i].x, items[i].y, items[i].w, items[i].h, color[BACK_COLOR]);
			return 1;
		}
		else {
			col = 0;
		}
	}
	return 0;
}



void Spawn_Random_Item() {
	srand(TIM4->CNT);
	int i = rand() % MAX_ITEM;
	
    if (items[i].active == 0) {
        items[i].x = (rand() % X_MAX);
        items[i].y = (rand() % Y_MAX);
		Lcd_Draw_Box(items[i].x, items[i].y, items[i].w, items[i].h, color[items[i].ci]);
        items[i].active = 1;
    }
}





static int Frog_Move(int k)
{
	// UP(0), DOWN(1), LEFT(2), RIGHT(3)

	static void (*key_func[])(void) = {k0, k1, k2, k3};
	if(k <= 3) key_func[k]();
	return Check_Collision();
}

static void Game_Init(void)
{
	int i;
	score = 0;
	Lcd_Clr_Screen();
	hero.x = 150; hero.y = 220; hero.w = HERO_SIZE_X; hero.h = HERO_SIZE_Y; hero.ci = HERO_COLOR; hero.life = HERO_LIFE;
	for(i=0;i<MOB1_MAX;i++){
		mobs[i].x = (rand()%X_MAX); mobs[i].y = (rand()%Y_MAX); mobs[i].w = MOB_SIZE_X; mobs[i].h = MOB_SIZE_Y; mobs[i].ci = MOB_COLOR;
		mobs[i].dir1 = RIGHT; mobs[i].dir2 = RIGHT; mobs[i].life=1;
	}

	

	Lcd_Draw_Box(hero.x, hero.y, hero.w, hero.h, color[hero.ci]);

	for(i=0;i<MOB1_MAX;i++){
		Lcd_Draw_Box(mobs[i].x, mobs[i].y, mobs[i].w, mobs[i].h, color[mobs[i].ci]);
	}
}

extern volatile int TIM4_expired;
extern volatile int USART1_rx_ready;
extern volatile int USART1_rx_data;
extern volatile int Jog_key_in;
extern volatile int Jog_key;

void System_Init(void)
{
	Clock_Init();
	LED_Init();
	Key_Poll_Init();
	Uart1_Init(115200);

	SCB->VTOR = 0x08003000;
	SCB->SHCSR = 7<<16;
}

#define DIPLAY_MODE		3




//======================================sound function===========================================
extern volatile int TIM2_Expired;

#define BASE  (500) //msec

static void Buzzer_Beep(unsigned char tone, int duration)
{
	const static unsigned short tone_value[] = {261,277,293,311,329,349,369,391,415,440,466,493,523,554,587,622,659,698,739,783,830,880,932,987};

	TIM3_Out_Freq_Generation(tone_value[tone]);
	TIM2_Repeat_Interrupt_Enable(1, duration);
}








//=======================================   main    ============================================
void Main(void)
{
	System_Init();
	Uart_Printf("Street Froggy\n");
	TIM3_Out_Init();

	Lcd_Init(DIPLAY_MODE);

	Jog_Poll_Init();
	Jog_ISR_Enable(1);
	Uart1_RX_Interrupt_Enable(1);
	int dir, cnt_for_tim4=0, game_over = 0 , cnt_for_mob3 = 0, music_run = 0, m = 0, goto_start = 0;
	int collis = 0, lock = 0, cnt_for_systic = 0, lock_for_stage = 0, lock_for_item = 0, lock_for_bullet = 0;
	enum key{C1, C1_, D1, D1_, E1, F1, F1_, G1, G1_, A1, A1_, B1, C2, C2_, D2, D2_, E2, F2, F2_, G2, G2_, A2, A2_, B2};
	enum note{N16=BASE/4, N8=BASE/2, N4=BASE, N2=BASE*2, N1=BASE*4};
	extern tImage blood_hand1;
	extern tImage blood_hand2;



	//============================== BGM ===================================
	const int vampire_bgm[][2] = {
		{A1, N4}, {C2, N4}, {B1, N2},  // 느린 음계 진행
		{D2, N4}, {F2, N4}, {E2, N2},
	
		{C2, N4}, {A1, N4}, {G1_, N2}, // 어두운 음색 강조
		{F1_, N4}, {D2_, N4}, {C2, N2},
	
		{A1, N4}, {C2, N4}, {B1, N2},  // 앞 패턴 반복
		{E2, N2}, {D2, N4}, {C2, N4}
	};


	

for(;;){
	int i;
	stage = 1, lock_for_stage = 0;
	for(i=MOB1_MAX;i<MOB_MAX;i++){
		mobs[i].life=0;
	}
	Lcd_Clr_Screen();

	for(;;){
		draw_image(10, 80, &blood_hand1);
		draw_image(250, 110, &blood_hand2);
		Lcd_Printf(50,50,RED,BLACK,2,2,"Vampire Hunter");
		Lcd_Printf(70,150,RED,BLACK,1,1,"Push SW1 to start game");
		if(Jog_key_in&&Jog_key==5){
			Jog_key_in=0;
			break;
		}
	}



	for(;;)
	{

		Game_Init();
		TIM4_Repeat_Interrupt_Enable(1, TIMER_PERIOD*2);
		//TIM2_Repeat_Interrupt_Enable(1, TIMER_PERIOD*2); //음악 재생용?
		Lcd_Printf(0,0,RED,BLACK,1,1,"Score:%d", score);
		Lcd_Printf(250,0,RED,BLACK,1,1,"Stage%d", stage);

		for(;;)
		{

			if((TIM2_Expired == 0 )&&( music_run == 0)){
				Buzzer_Beep(vampire_bgm[m][0], vampire_bgm[m][1]);
				music_run = 1;
			}
	
			if((TIM2_Expired == 1 )&&( music_run == 1)){
				m++;
				music_run = 0;
				TIM3_Out_Stop();
				TIM2_Expired = 0;
			}
		
			if(m>=(sizeof(vampire_bgm)/sizeof(vampire_bgm[0]))){
				m=0;
			}


			if(Jog_key_in) 
			{
				if(Jog_key == 4){
					if(dir == 3) {  // 오른쪽
						Bullet_Fire(dir, hero.x, hero.y); 
					}
					else if(dir == 0) {  // 위쪽
						Bullet_Fire(dir, hero.x, hero.y);  
					}
					else if(dir == 1) {  // 아래쪽
						Bullet_Fire(dir, hero.x, hero.y);  
					}
					else if(dir == 2) {  // 왼쪽
						Bullet_Fire(dir, hero.x, hero.y);  
					}
				}
				else{
					hero.ci = BACK_COLOR;
					Draw_Object(&hero);
					collis = Frog_Move(Jog_key);
					hero.ci = HERO_COLOR;
					Draw_Object(&hero);
					dir = Jog_key;
					if(Item_Check_Collision()){
						lock_for_item = 0;
					}
				}
				Jog_key_in = 0;
			}


			//===========mob moving============
			if(TIM4_expired&&cnt_for_tim4>=4) 
			{
				int i;
				// === freeze 시간 감소 ===
				if (mob_frozen) {
					mob_freeze_timer--;
					if (mob_freeze_timer <= 0) {
						mob_frozen = 0;
						
						for (i = 0; i < MOB1_MAX; i++) {
							if (mobs[i].life <= 0) {
								// 기존 죽은 위치를 지운다 (검은색)
								Lcd_Draw_Box(mobs[i].x, mobs[i].y, mobs[i].w, mobs[i].h, color[BACK_COLOR]);
								//  새 위치로 재배치
								mobs[i].x = rand() % (LCDH - MOB_SIZE_Y); 
								mobs[i].y = rand() % (LCDH - MOB_SIZE_Y);
								mobs[i].life = 1;

								// 색상 설정 및 그리기
								mobs[i].ci = (i < MOB1_MAX) ? MOB_COLOR : MOB2_COLOR;
								Draw_Object(&mobs[i]);
							}
						}
						for (i = MOB1_MAX; i < MOB_MAX; i++) {
							if (mobs[i].life <= 0 && stage>=2) {
								// 기존 죽은 위치를 지운다 (검은색)
								Lcd_Draw_Box(mobs[i].x, mobs[i].y, mobs[i].w, mobs[i].h, color[BACK_COLOR]);
								//  새 위치로 재배치
								mobs[i].x = rand() % (LCDH - MOB_SIZE_Y); 
								mobs[i].y = rand() % (LCDH - MOB_SIZE_Y);
								mobs[i].life = 1;

								// 색상 설정 및 그리기
								mobs[i].ci = (i < MOB1_MAX) ? MOB_COLOR : MOB2_COLOR;
								Draw_Object(&mobs[i]);
							}
						}
					}
				}
				
				if (!mob_frozen){
					for(i=0;i<MOB_MAX;i++){
						mobs[i].ci = BACK_COLOR;
						Draw_Object(&mobs[i]);
					}

					Car_Move();

					for(i=0;i<MOB1_MAX;i++){
						mobs[i].ci = MOB_COLOR;
						Draw_Object(&mobs[i]);
					}

					if(stage >= 2){
						for(i=MOB1_MAX;i<MOB_MAX;i++){
							mobs[i].ci = MOB2_COLOR;
							Draw_Object(&mobs[i]);
						}
					}
					if(hero.life<10){
						Lcd_Printf(120,0,WHITE,BLACK,1,1,"LIFE: %d   ", hero.life);
					}
					else Lcd_Printf(120,0,WHITE,BLACK,1,1,"LIFE: %d", hero.life);

					if (stage >= 3 && lock_for_bullet==0) { // 2초마다 (4 = 0.5초 단위)
						Mob3_Fire_All();
						for(i=0;i<MOB3_MAX;i++){
							mobs3[i].ci = 1;
							Draw_Object(&mobs3[i]);
						}
						lock_for_bullet = 1;
					}
					else if(stage>=3 && lock_for_bullet == 1){
						for(i=0;i<MOB3_MAX;i++){
							mobs3[i].ci = 1;
							Draw_Object(&mobs3[i]);
						}
						cnt_for_mob3++;
					}

					if(cnt_for_mob3 >=13){
						lock_for_bullet = 0;
						cnt_for_mob3 = 0;
					}
				}

				Bullet_Update();
				TIM4_expired = 0;
				cnt_for_tim4 = 0;
				Lcd_Printf(250,0,RED,BLACK,1,1,"Stage%d", stage);
				Lcd_Printf(0,0,RED,BLACK,1,1,"Score:%d", score);
				for(i=0; i<MAX_ITEM; i++){
					if(items[i].active == 1){
					Lcd_Draw_Box(items[i].x, items[i].y, items[i].w, items[i].h, color[items[i].ci]);
					}
				}
			}
			else if(TIM4_expired&&cnt_for_tim4<4){
				collis = Check_Collision();
				Mob3_Bullet_Update();
				Bullet_Update();
				cnt_for_tim4++;
				TIM4_expired=0;
			}
			



			//==================무적시간===================
			if(collis && lock == 0){
				hero.life--;
				Uart_Printf("Collision! Left life is %d\n", hero.life);//부딪히면 몇 초 동안은 충돌 인정 안 되도록 해야.
				lock=1;
				SysTick_Run(100);
			}
			else if(lock && (SysTick_Check_Timeout())){
				if(cnt_for_systic<10){
					cnt_for_systic++;
					hero.ci = 0;
					Draw_Object(&hero);
				}
				else if(cnt_for_systic>=10){
					cnt_for_systic = 0;
					lock = 0;
					hero.ci = HERO_COLOR;
					Draw_Object(&hero);
					SysTick_Stop();
				}
			}



			//===============change to 2 stage=================
			if(score>=30 && lock_for_stage == 0){
				Lcd_Clr_Screen();
				TIM3_Out_Stop();
				for(;;){
					Lcd_Printf(100,50,RED,BLACK,2,2,"Stage 2");
					Lcd_Printf(60,150,RED,BLACK,1,1,"Push SW1 to start stage");
					if(Jog_key_in&&Jog_key==5){
						Jog_key_in=0;
						break;
					}
				}
				Lcd_Clr_Screen();
				int i;
				stage++;
				score = 0;
				lock_for_stage = 1;
				for(i=MOB1_MAX;i<MOB_MAX;i++){
					mobs[i].x = (rand()%X_MAX); mobs[i].y = (rand()%Y_MAX); mobs[i].w = MOB_SIZE_X; mobs[i].h = MOB_SIZE_Y; mobs[i].ci = MOB2_COLOR;
					mobs[i].dir1 = RIGHT; mobs[i].dir2 = RIGHT; mobs[i].life=1;
				}
				for(i=0;i<2;i++){
					items[i].w = ITEM_SIZE_X; items[i].h = ITEM_SIZE_Y; items[i].ci = 2;
				}
					items[2].w = ITEM_SIZE_X; items[2].h = ITEM_SIZE_Y; items[2].ci = 3;
			}


			//===========change to 3 stage======================
			if(score>=30 && lock_for_stage == 1){
				Lcd_Clr_Screen();
				TIM3_Out_Stop();
				for(;;){
					Lcd_Printf(100,50,RED,BLACK,2,2,"Stage 3");
					Lcd_Printf(60,150,RED,BLACK,1,1,"Push SW1 to start stage");
					if(Jog_key_in&&Jog_key==5){
						Jog_key_in=0;
						break;
					}
				}
				lock_for_item = 0;
				Lcd_Clr_Screen();
				int i;
				stage++;
				score = 0;
				lock_for_stage = 2;
				for (i = 0; i < MOB3_MAX; i++) {
					mobs3[i].x = mob3_positions[i][0];
					mobs3[i].y = mob3_positions[i][1];
					mobs3[i].w = MOB_SIZE_X;
					mobs3[i].h = MOB_SIZE_Y;
					mobs3[i].ci = 1; // 새로운 색
					Draw_Object(&mobs3[i]);
				}
			}


			//==================Game Clear=====================
			if(score>=30 && lock_for_stage == 2){
				Lcd_Clr_Screen();
				TIM3_Out_Stop();
				for(;;){
					Lcd_Printf(100,50,GREEN,BLACK,2,2,"Clear!!");
					Lcd_Printf(50,150,YELLOW,BLACK,1,1,"Push Down KEY for restart");
					if(Jog_key_in&&Jog_key==1){
						Jog_key_in=0;
						goto START;
					}
				}
			}



			//====================item spawn==================
			if(((stage >=2 && score % 5 == 0 )&& score > 1) && lock_for_item==0){
				Spawn_Random_Item();
				lock_for_item = 1;
				int i;
				for(i=0; i<2; i++){
					if(items[i].active ==1)
					Uart_Printf("item %d is x: %d   y: %d\n", i, items[i].x, items[i].y);
				}
			}





			//===========game over==============
			if(hero.life <= 0)
			{
				TIM4_Repeat_Interrupt_Enable(0, 0);
				Lcd_Clr_Screen();
				TIM3_Out_Stop();
				for(;;){
					Lcd_Printf(80,50,RED,BLACK,2,2,"Game Over");
					Lcd_Printf(50,110,WHITE,BLACK,1,1,"Push Down KEY for restart");
					Lcd_Printf(80,150,WHITE,BLACK,1,1,"Push SW1 to resume");
					if(Jog_key_in&&Jog_key==5){
						Jog_key_in=0;
						break;
					}
					else if(Jog_key_in&&Jog_key==1){
						Jog_key_in=0;
						goto START;
					}
				}
				Lcd_Clr_Screen();
				break;
			}
		}
	}
	START:;
}
}




