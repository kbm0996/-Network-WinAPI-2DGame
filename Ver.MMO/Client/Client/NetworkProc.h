#ifndef __NETWORK_PROC__
#define __NETWORK_PROC__


///////////////////////////////////////////////////////
// Network
//
///////////////////////////////////////////////////////
int NetworkInit(WCHAR* szIP);
bool NetworkProc(WPARAM wParam, LPARAM lParam);
void Network_Close();

///////////////////////////////////////////////////////
// Send
//
///////////////////////////////////////////////////////
bool SendEvent();

bool SendPacket(st_NETWORK_PACKET_HEADER *pHeader, mylib::CSerialBuffer *pPacket);

void mpMoveStart(st_NETWORK_PACKET_HEADER *pHeader, mylib::CSerialBuffer *pPacket, BYTE iDir, WORD iX, WORD iY);
void mpMoveStop(st_NETWORK_PACKET_HEADER *pHeader, mylib::CSerialBuffer *pPacket, BYTE iDir, WORD iX, WORD iY);
void mpAttack1(st_NETWORK_PACKET_HEADER *pHeader, mylib::CSerialBuffer *pPacket, BYTE iDir, WORD iX, WORD iY);
void mpAttack2(st_NETWORK_PACKET_HEADER *pHeader, mylib::CSerialBuffer *pPacket, BYTE iDir, WORD iX, WORD iY);
void mpAttack3(st_NETWORK_PACKET_HEADER *pHeader, mylib::CSerialBuffer *pPacket, BYTE iDir, WORD iX, WORD iY);

///////////////////////////////////////////////////////
// Recv
//
///////////////////////////////////////////////////////
bool RecvEvent();
int RecvComplete();

bool OnRecv(BYTE byType, mylib::CSerialBuffer* pPacket);

void netPacketProc_CreateMyCharacter(mylib::CSerialBuffer* pPacket);
void netPacketProc_CreateOtherCharacter(mylib::CSerialBuffer* pPacket);
void netPacketProc_DeleteCharacter(mylib::CSerialBuffer* pPacket);
void netPacketProc_MoveStart(mylib::CSerialBuffer* pPacket);
void netPacketProc_MoveStop(mylib::CSerialBuffer* pPacket);
void netPacketProc_Attack1(mylib::CSerialBuffer* pPacket);
void netPacketProc_Attack2(mylib::CSerialBuffer* pPacket);
void netPacketProc_Attack3(mylib::CSerialBuffer* pPacket);
void netPacketProc_Damage(mylib::CSerialBuffer* pPacket);
void netPacketProc_Sync(mylib::CSerialBuffer *pPacket);



#endif