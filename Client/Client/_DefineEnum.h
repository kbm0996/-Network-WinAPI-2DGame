#ifndef __SPRITE_DEFINE__
#define __SPRITE_DEFINE__

#define dfMAX_USER 30

//-----------------------------------------------------------------
// System Value
//-----------------------------------------------------------------
#define dfSCREEN_WIDTH	640
#define dfSCREEN_HEIGHT 480
#define dfSCREEN_BIT	32


//-----------------------------------------------------------------
// Animation Frame Delay
//-----------------------------------------------------------------
#define dfDELAY_STAND	5
#define dfDELAY_MOVE	4
#define dfDELAY_ATTACK1	3
#define dfDELAY_ATTACK2	4
#define dfDELAY_ATTACK3	4
#define dfDELAY_EFFECT	3

/*---------------------------------------------------------------

Sprite Index

---------------------------------------------------------------*/
enum e_SPRITE
{
	eMAP = 0,

	// ĳ���� ���ֱ� ����
	ePLAYER_STAND_L01,
	ePLAYER_STAND_L02,
	ePLAYER_STAND_L03,
	ePLAYER_STAND_L04,
	ePLAYER_STAND_L05,
	ePLAYER_STAND_L_MAX = ePLAYER_STAND_L05,

	// ĳ���� ���ֱ� ������
	ePLAYER_STAND_R01,
	ePLAYER_STAND_R02,
	ePLAYER_STAND_R03,
	ePLAYER_STAND_R04,
	ePLAYER_STAND_R05,
	ePLAYER_STAND_R_MAX = ePLAYER_STAND_R05,



	// ĳ���� �̵� ����
	ePLAYER_MOVE_L01,
	ePLAYER_MOVE_L02,
	ePLAYER_MOVE_L03,
	ePLAYER_MOVE_L04,
	ePLAYER_MOVE_L05,
	ePLAYER_MOVE_L06,
	ePLAYER_MOVE_L07,
	ePLAYER_MOVE_L08,
	ePLAYER_MOVE_L09,
	ePLAYER_MOVE_L10,
	ePLAYER_MOVE_L11,
	ePLAYER_MOVE_L12,
	ePLAYER_MOVE_L_MAX = ePLAYER_MOVE_L12,

	// ĳ���� �̵� ������
	ePLAYER_MOVE_R01,
	ePLAYER_MOVE_R02,
	ePLAYER_MOVE_R03,
	ePLAYER_MOVE_R04,
	ePLAYER_MOVE_R05,
	ePLAYER_MOVE_R06,
	ePLAYER_MOVE_R07,
	ePLAYER_MOVE_R08,
	ePLAYER_MOVE_R09,
	ePLAYER_MOVE_R10,
	ePLAYER_MOVE_R11,
	ePLAYER_MOVE_R12,
	ePLAYER_MOVE_R_MAX = ePLAYER_MOVE_R12,




	// ĳ���� ����1 ����
	ePLAYER_ATTACK1_L01,
	ePLAYER_ATTACK1_L02,
	ePLAYER_ATTACK1_L03,
	ePLAYER_ATTACK1_L04,
	ePLAYER_ATTACK1_L_MAX = ePLAYER_ATTACK1_L04,

	// ĳ���� ����1 ������
	ePLAYER_ATTACK1_R01,
	ePLAYER_ATTACK1_R02,
	ePLAYER_ATTACK1_R03,
	ePLAYER_ATTACK1_R04,
	ePLAYER_ATTACK1_R_MAX = ePLAYER_ATTACK1_R04,



	// ĳ���� ����2 ����
	ePLAYER_ATTACK2_L01,
	ePLAYER_ATTACK2_L02,
	ePLAYER_ATTACK2_L03,
	ePLAYER_ATTACK2_L04,
	ePLAYER_ATTACK2_L_MAX = ePLAYER_ATTACK2_L04,

	// ĳ���� ����2 ������
	ePLAYER_ATTACK2_R01,
	ePLAYER_ATTACK2_R02,
	ePLAYER_ATTACK2_R03,
	ePLAYER_ATTACK2_R04,
	ePLAYER_ATTACK2_R_MAX = ePLAYER_ATTACK2_R04,



	// ĳ���� ����3 ����
	ePLAYER_ATTACK3_L01,
	ePLAYER_ATTACK3_L02,
	ePLAYER_ATTACK3_L03,
	ePLAYER_ATTACK3_L04,
	ePLAYER_ATTACK3_L05,
	ePLAYER_ATTACK3_L06,
	ePLAYER_ATTACK3_L_MAX = ePLAYER_ATTACK3_L06,

	// ĳ���� ����3 ������
	ePLAYER_ATTACK3_R01,
	ePLAYER_ATTACK3_R02,
	ePLAYER_ATTACK3_R03,
	ePLAYER_ATTACK3_R04,
	ePLAYER_ATTACK3_R05,
	ePLAYER_ATTACK3_R06,
	ePLAYER_ATTACK3_R_MAX = ePLAYER_ATTACK3_R06,


	// Ÿ�� ����Ʈ 
	eEFFECT_SPARK_01,
	eEFFECT_SPARK_02,
	eEFFECT_SPARK_03,
	eEFFECT_SPARK_04,
	eEFFECT_SPARK_MAX = eEFFECT_SPARK_04,

	eGUAGE_HP,
	eSHADOW,

	eSPRITE_MAX

};


/*---------------------------------------------------------------

������Ʈ Ÿ��

---------------------------------------------------------------*/
enum e_OBJECT_TYPE
{
	eTYPE_PLAYER = 0,
	eTYPE_EFFECT
};



//-----------------------------------------------------------------
// Character Animation Define
//-----------------------------------------------------------------
#define dfACTION_MOVE_LL		0
#define dfACTION_MOVE_LU		1
#define dfACTION_MOVE_UU		2
#define dfACTION_MOVE_RU		3
#define dfACTION_MOVE_RR		4
#define dfACTION_MOVE_RD		5
#define dfACTION_MOVE_DD		6
#define dfACTION_MOVE_LD		7

#define dfACTION_JUMP_LL		8
#define dfACTION_JUMP_RR		9
#define dfACTION_JUMP_CC		10

#define dfACTION_ATTACK1		11
#define dfACTION_ATTACK2		12
#define dfACTION_ATTACK3		13

#define dfACTION_PUSH			14
#define dfACTION_STAND			255


//-----------------------------------------------------------------
// Character Speed
//-----------------------------------------------------------------
#define dfSPEED_PLAYER_X	3
#define dfSPEED_PLAYER_Y	2

//-----------------------------------------------------------------
// Character Direction
//-----------------------------------------------------------------
#define dfDIR_LEFT			0
#define dfDIR_RIGHT			4
// ��Ʈ��ũ ���� ���Ǵ� ��,�� ���Ⱚ�� ���� ����ϱ� ���ؼ�
// 0, 4 �� ����Ѵ�.


//-----------------------------------------------------------------
// Screen Move Range
//-----------------------------------------------------------------
#define dfRANGE_MOVE_TOP	50
#define dfRANGE_MOVE_LEFT	10
#define dfRANGE_MOVE_RIGHT	630
#define dfRANGE_MOVE_BOTTOM	470

#endif
