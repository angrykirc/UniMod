#include "stdafx.h"
#include <map>

int UnimodVersion=0x000402;/// 00.04.02 

void (__cdecl *netClientSend) (int PlrN,int Dir,//1 - �������
								void *Buf,int BufSize);
// ���������� ��� Bool - ������� ���� ������� ���� �������

extern sprite_s *(__cdecl *spriteLoadAdd) (int thingType,int coordX,int coordY);
extern void (__cdecl *spriteDeleteStatic)(sprite_s *Sprite);

sprite_s *(__cdecl *netSpriteByCodeHi)(int netCode);// ��� �����������
sprite_s *(__cdecl *netSpriteByCodeLo)(int netCode);// ��� ������������

void (__cdecl *netSendPointFx)(int PacketIdx,noxPoint* Pt);
void (__cdecl *netSendRayFx)(int PacketIdx,noxRectInt* Rc);
void (__cdecl *netPriMsg)(void *PlayerUnit,char *String,int Flag);

DWORD (__cdecl *netGetUnitCodeServ)(void *Unit);
DWORD (__cdecl *netGetUnitCodeCli)(void *Sprite);

void *(__cdecl *netGetUnitByExtent)(DWORD NetCode);/// ������ ��� ������������ (�� �������)playerGoObserver

void (__cdecl *netSendShieldFx)(void *Unit,noxPoint* From);

extern "C" void conSendToServer(const char *Cmd);
extern "C" int conDoCmd(char *Cmd,bool &PrintNil);

extern void netOnTileChanged(BYTE *Buf,BYTE *End);
extern void netOnUpdateUnitDef(BYTE *Buf,BYTE *BufEnd);

int (__cdecl *netSendBySock)(int Player,void *Data,int Size, int Type);

byte authorisedArray[0x1F];
void (__cdecl *playerGoObserver)(void* playerPtr, byte unk1, byte unk2);

bigUnitStruct *netUnitByCodeServ(DWORD NetCode)
{
	if (NetCode & 0x8000)
		return (bigUnitStruct *)netGetUnitByExtent(NetCode&0x7FFF);
	return 0;
}
void netSendServ(void *Buf,int BufSize)
{
	netClientSend(0x1F,0,Buf,BufSize);// ������ ��� ���, �� �� ������
}
void netSendNotOne(void *Buf,int BufSize,void *One) /// �������� ���� ��������  ����� ������
{
	for(bigUnitStruct* Plr=playerFirstUnit();Plr!=0;Plr=playerNextUnit(Plr))
	{
		BYTE *P2=(BYTE*)Plr->unitController;
		if (P2==One)
			continue;
		P2+=0x114;P2=*((BYTE **)P2);
		P2+=0x810;
		netClientSend(*P2,1,Buf,BufSize);
	}
}
void netSendAll(void *Buf,int BufSize)
{
	for(bigUnitStruct* Plr=playerFirstUnit();Plr!=0;Plr=playerNextUnit(Plr))
	{
		BYTE *P2=(BYTE*)Plr->unitController;
		P2+=0x114;P2=*((BYTE **)P2);
		P2+=0x810;
		netClientSend(*P2,1,Buf,BufSize);
	}
	
}
void conSendToServer(const char *Src)
{
	BYTE Buf[255],*P=Buf;
	size_t Size=strlen(Src)+1;
	
	if (Size>0 && Size<255)
	{
		netUniPacket(upLuaRq,P,Size);
		memcpy(P,Src,Size);
		netSendServ(Buf,Size+P-Buf);
	}

}
namespace {

	BYTE *fakePlayerInputPacket(BYTE* BufStart)
	{
		BYTE *P=BufStart;
		if(P[1]<10)
		{
			BufStart+=P[1]+2;
			return BufStart;
		}
		else
		{
			const BYTE replace[12]={0x3f, 0x0a, 0x01, 0x00, 0x00, 0x00, 0x81, 0x01, 0x00, 0x00, 0x00, 0x81};
			if(P[1]>10)
				BufStart+=(P[1]+2)-12;
			memcpy(BufStart, replace, 12);
			return BufStart;
		}
	}

	int sendToServer(lua_State *L)
	{
		const char *S=lua_tostring(L,1);
		if (S)
			conSendToServer(S);
		return 0;
	}
	int netGetCodeServL(lua_State *L)
	{
		if (lua_type(L,1)!=LUA_TLIGHTUSERDATA)
		{
			lua_pushstring(L,"wrong args!");
			lua_error_(L);
		}
		void *P=lua_touserdata(L,1);
		if (P==0)
		{
			lua_pushnil(L);return 1;
		}
		lua_pushinteger(L,netGetUnitCodeServ(P));
		return 1;
	}
	int netPointFx(lua_State *L)
	{
		lua_settop(L,3);
		if (
			(lua_type(L,-3)!=LUA_TNUMBER)||
			(lua_type(L,-2)!=LUA_TNUMBER)||
			(lua_type(L,-1)!=LUA_TNUMBER)||
			0
			)
		{
			lua_pushstring(L,"wrong args!");
			lua_error_(L);
		}
		noxPoint P(lua_tonumber(L,-2),lua_tonumber(L,-1));
		int Code=lua_tointeger(L,-3);
		// �������� - ���� ����� ����� �� netMsgNames ������ ������ ���������� ���������
		// ������ ������� � ������ +0x27
		// ��������� ������: 0-10,21,25,31,34 - � ����� �������� 0x84
		// ����������� ������� ����
		// 0xA0-�����,0xA3  - ��������
		netSendPointFx(Code,&P);
		return 0;
	}
	int netRayFx(lua_State *L)
	{
		lua_settop(L,5);

		if (
			(lua_type(L,-5)!=LUA_TNUMBER)||
			(lua_type(L,-4)!=LUA_TNUMBER)||
			(lua_type(L,-3)!=LUA_TNUMBER)||
			(lua_type(L,-2)!=LUA_TNUMBER)||
			(lua_type(L,-1)!=LUA_TNUMBER)||
			0
			)
		{
			lua_pushstring(L,"wrong args!");
			lua_error_(L);
		}
		noxRectInt R(lua_tointeger(L,-4),lua_tointeger(L,-3),
			lua_tointeger(L,-2),lua_tointeger(L,-1));
		int Code=lua_tointeger(L,-5);
		// �������� - ���� ����� ����� �� netMsgNames ������ ������ ���������� ���������
		// ������ ������� � ������ +0x27
		// ��������� ������:11,14,15,16,19,20 - � ����� �������� 0x9F
		// ����������� ������� ����
		// 
		netSendRayFx(Code,&R);
		return 0;
	}
	int netShieldFx(lua_State *L)
	{
		lua_settop(L,3);
		if (lua_type(L,-3)!=LUA_TLIGHTUSERDATA)
		{
			lua_pushstring(L,"wrong args!");
			lua_error_(L);
		}
		
		noxPoint Pt(lua_tonumber(L,-2),lua_tonumber(L,-1));
		netSendShieldFx(lua_touserdata(L,-3),&Pt);
		return 0;
	}
	int netFake(lua_State *L) 
			/// �������� ��������� � ������, �� �� ����� �������� ���� 
	{
		if (lua_type(L,1)!=LUA_TLIGHTUSERDATA)
		{
			lua_pushstring(L,"wrong args!");
			lua_error_(L);
		}
	
		struct {
			BYTE Pckt;
			USHORT Code;
		} B={0xE8,netGetUnitCodeServ(lua_touserdata(L,-1))};

		netSendAll(&B,sizeof(B));

		return 0;
	}
	int netOnRespL(lua_State *L)
	{
		bigUnitStruct *Plr=(bigUnitStruct *)lua_touserdata(L,1);
		char Buf[280];
		sprintf(Buf,"cli %p %s%s",Plr,(lua_tointeger(L,2)==0)?"error:":"ok:",lua_tostring(L,3) );
		conPrintI(Buf);	
		return 0;
	}
	int netDoReq(lua_State *L)/// ���������� �������� ���������
	{
		BYTE Buf[255],*P=Buf;
		size_t Size;
		const char *Src=lua_tolstring(L,1,&Size);
		if (Size>0 && Size<255)
		{
			Size++;
			netUniPacket(upLuaRq,P,Size);
			memcpy(P,Src,Size-1);
			netSendAll(Buf,Size+P-Buf);
		}
		else
		{
			lua_pushstring(L,"wrong: string too long (max 255)!");
			lua_error_(L);
		}
		return 0;
	}
	int netDoDelayed(lua_State *L)
	{
		int n=lua_gettop(L);
		lua_pushvalue(L,lua_upvalueindex(1));///�������� ��� �������
			lua_pushvalue(L,lua_upvalueindex(2));// fn
			lua_pushinteger(L,0); // 0 - time
			lua_newtable(L);
				for (int i=1;i<=n;i++)
				{
					lua_pushvalue(L,i);
					lua_rawseti(L,-2,i); //{"string"}
				}
		lua_call(L,3,0);
		return 0;
	}

	void netLuaRq(BYTE *P,BYTE *End)
	{
		BYTE Buf[255],*Pt=Buf;
		size_t Size;
		const char *Src;
		bool Ok=true;
		int From = lua_gettop(L); /// ������ ������� �����
		End[-1]=0;
		sprintf((char*)Buf,"cmd: %s",P);
		conPrintI((char*)Buf);	

		if (0!=luaL_loadstring(L,(char*)P))
		{ 
			Ok=false;
		}
		if (Ok && (0!=lua_pcall(L,0,1,0)) )
		{
			Ok=false;
		}
		Src=lua_tolstring(L,-1,&Size);
		if (Size<255)
		{
			netUniPacket(upLuaResp,Pt,Size+1);
			*(Pt++)=Ok?1:0;
			if (Src==NULL)
				*Pt=0;
			else
				memcpy(Pt,Src,Size);
			netSendServ(Buf,Size+Pt-Buf);
		}
		lua_settop(L,From);

	}
	void netDelStatic(BYTE *Start,BYTE *End)
	{
		int Code=*((int*)(Start+0));
		if (0==(Code&0x8000)) // �� �����������
			return;
		sprite_s *S=netSpriteByCodeHi(Code);
		BYTE* P=(BYTE *)S;
		*((int *)(P+0x80))|=0x8000;
		if (S)
			spriteDeleteStatic(S);
	}
	void netMoveStatic(BYTE *Start,BYTE *End)
	{
		int Code=*((int*)(Start+0));
		//TODO:
	}

	void netNewStatic(BYTE *Start,BYTE *End)
	{
		BYTE *S=
			(BYTE *)spriteLoadAdd( *((int*)(Start+0)),
				(*((float*)(Start+4))),
				(*((float*)(Start+8))));
		DWORD *Code=(DWORD *)(S+0x80);
		if (*Code!=*((DWORD*)(Start+0x0C)) )
			Code++;
	}
	void netOnVersionRq(BYTE *Start,BYTE *End,bigUnitStruct* Plr)
	{
		if (End-Start<4)
			return;// �����-�� ������
		BYTE *P2=(BYTE*)Plr->unitController;
		P2+=0x114;P2=*((BYTE **)P2);
		P2+=0x810;
		int Top= lua_gettop(L);
		int OtherVersion=*((int*)Start);
		if (UnimodVersion!=OtherVersion)
		{
			getServerVar("serverOnClientVersionWrong");
			if (lua_type(L,-1)==LUA_TFUNCTION)
			{
				lua_pushinteger(L,OtherVersion);
				lua_pushinteger(L,UnimodVersion);
				lua_pushinteger(L,*P2);
				lua_pushlightuserdata(L,Plr);
				lua_pcall(L,4,0,0);
			}
		}
		const char *Src;
		size_t Size=0;
		if (End-Start>4)
		{
			do
			{
				if (0!=luaL_loadbuffer(L,(char*)Start+4,End-Start-4,"netOnVersion"))
					break;
				lua_getfield(L,LUA_REGISTRYINDEX,"server");			
				lua_setfenv(L,-2);
				lua_pcall(L,0,1,0);
			} while(0);
			Src=lua_tolstring(L,-1,&Size);
			if (Size>200)
				Size=200;
			if (Src==NULL)
				Size=0;
		}
		BYTE Buf[255],*Pt=Buf;
		netUniPacket(upVersionResp,Pt,Size+4);
		*((int*)Pt)=UnimodVersion;
		Pt+=4;
		if (Size>0)
		{
			memcpy(Pt,Src,Size);
			Pt+=Size;
		}
		netClientSend(*P2,1,Buf,Pt-Buf);
		lua_settop(L,Top);
	}
	void netOnVersionResp(BYTE *Start,BYTE *End)
	{
		if (End-Start<4)
			return;// �����-�� ������
		int Top= lua_gettop(L);
		int OtherVersion=*((int*)Start);

		getClientVar("clientOnServerVersion");
		if (lua_type(L,-1)==LUA_TFUNCTION)
		{
			lua_pushinteger(L,OtherVersion);
			lua_pushinteger(L,UnimodVersion);
			if (End-Start<4)
				lua_pushnil(L);
			else
			{
				lua_pushlstring(L,(char*)Start+4,End-Start-4);
			}
			lua_pcall(L,3,0,0);
		}
		lua_settop(L,Top);
	}
	int netVersionRq(lua_State*L)
	{
		lua_settop(L,1);
		size_t Size;
		const char *Src=lua_tolstring(L,1,&Size);
		if (Size>200)
			Size=200;
		if (Src==NULL)
			Size=0;
		BYTE Buf[255],*Pt=Buf;
		netUniPacket(upVersionRq,Pt,Size+4);
		*((int*)Pt)=UnimodVersion;
		Pt+=4;
		if (Size>0)
		{
			memcpy(Pt,Src,Size);
			Pt+=Size;
		}
		netSendServ(Buf,Pt-Buf);
		return 0;
	}
	int netGetVersion(lua_State*L)
	{
		lua_pushinteger(L,UnimodVersion);
		return 1;
	}
/* ������ ����� ����������� */
	typedef std::map <BYTE,netClientFn_s> ClientMap_s;
	typedef std::map <BYTE,netServFn_s> ServerMap_s;
	ClientMap_s ClientRegMap;
	ServerMap_s ServerRegMap;
};
void netRegClientPacket(uniPacket_e Event,netClientFn_s Fn)
{
	ClientRegMap.insert(std::make_pair(Event,Fn));
}
void netRegServPacket(uniPacket_e Event,netServFn_s Fn)
{
	ServerRegMap.insert(std::make_pair(Event,Fn));
}
extern "C" void __cdecl onNetPacket(BYTE *&BufStart,BYTE *E);



void netUniPacket(uniPacket_e Code,BYTE *&Data,int Size)/// ��������� ��� �����
{
	*(Data++)=0xF8;//��������
	*(Data++)=Size+1;
	*(Data++)=(BYTE)Code;
}
extern void netOnWallChanged(wallRec *newData);
extern void netOnSendArchive(int Size,char *Name,char *NameE);
extern void netOnAbortDownload();
void __cdecl onNetPacket(BYTE *&BufStart,BYTE *E)/// ��������� ��������
{
	BYTE *P=BufStart;
	if (*P==186)
		netOnAbortDownload();
	else if (*P==0xF8)/// ��� ����� ������ ������-����� {F8,<�����>, ������}
	{
		P++;
		BufStart+=2+*(P++);// ��������������� ��� �����
		
/*		char Buf[60];
		sprintf(Buf,"Unipacket cli %x",*P);
		conPrintI(Buf);*/

		switch (*(P++))
		{
		case upWallChanged:
			netOnWallChanged((wallRec*)P);
			break;
		case upLuaRq:
			netLuaRq(P,BufStart);
			break;
		case upNewStatic:
			netNewStatic(P,BufStart);
			break;
		case upDelStatic:
			netDelStatic(P,BufStart);
			break;
		case upMoveStatic:
			netMoveStatic(P,BufStart);
			break;
		case upUpdateDef:
			netOnUpdateUnitDef(P-UNIPACKET_HEAD,BufStart);
			break;
		case upSendArchive:
			netOnSendArchive( *((int*)P),(char*)P+sizeof(int),(char*)BufStart);
			break;
		case upChangeTile:
			netOnTileChanged(P,BufStart);
			break;
		case upVersionResp:
			netOnVersionResp(P,BufStart);
			break;
		default:
			{
				ClientMap_s::const_iterator I=ClientRegMap.find(P[-1]);
				if (I!=ClientRegMap.end())
					I->second(P);
			}
		};
	}
}
extern void spellServDoCustom(int SpellArr[5],bool OnSelf,BYTE *MyPlayer,BYTE *MyUc);

extern void netOnClientTryUse(BYTE *Start,BYTE *End,BYTE *MyUc,void *Player);
extern "C" void __cdecl onNetPacket2(BYTE *&BufStart,BYTE *E,
		BYTE *MyPlayer, /// bigUnitStruct
		BYTE *MyUc)/// ��������� ��������
{
	BYTE *P=BufStart;
	bool specialAuthorisation=false; //���������� �������������� �����������
	if (*P==0x3F && specialAuthorisation==true)
	{
		void **PP=(void **)(((char*)MyPlayer)+0x2EC);
		PP=(void**)(((char*)*PP)+0x114);
		byte *P=(byte*)(*PP);
		byte playerIdx = *((byte*)(P+0x810));
		if(playerIdx!=0x1F) // ��� ���� ���������� �����
			switch(authorisedArray[playerIdx])
			{
				case 0:
					BufStart=fakePlayerInputPacket(BufStart);
					playerGoObserver(P, 1, 1);
					// �������� ���� � ������
					authorisedArray[playerIdx]++;
					break;
				case 1:
					BufStart=fakePlayerInputPacket(BufStart);
					// � ���� ��������� ����� ����� �� ����� ���� �� �����������
					break;
				case 2:
					// � � ���� - ����������� ��� ��������
					break;
			}
	}
	else if(*P==0xA8 && specialAuthorisation==true)
	{
		
	}
	else if (*P==0xF8)/// ��� ����� ������ ������-����� {F8,<�����>, ������}
	{
		P++;
		int BufSize=*(P++);
		BufStart+=2+BufSize;// ��������������� ��� �����
		BufSize--;// ������ ������������� ���� �� � ����

/*		char Buf[60];
		sprintf(Buf,"Unipacket serv %x",*P);
		conPrintI(Buf);*/

		switch (*(P++))
		{
		case upLuaResp: //TODO: ��������������� ������� �� ������ �������
			lua_getglobal(L,"netOnResp");
			if (lua_type(L,-1)==LUA_TFUNCTION)
			{
				lua_pushlightuserdata(L,MyPlayer);
				lua_pushinteger(L,*(P++));
				lua_pushlstring(L,(char*)P,BufSize-1);
				lua_pcall(L,3,0,0);
			}
			break;
		case upLuaRq:
			char Buf[200];bool Unused;
			strncpy(Buf,(char *)P,199);Buf[199]=0;
			conDoCmd(Buf,Unused);
			sprintf(Buf,"cmd %s",P);
			conPrintI(Buf);
			break;
		case upTryUnitUse:
			netOnClientTryUse(P,BufStart,MyUc,MyPlayer);
		case upVersionRq:
			netOnVersionRq(P,BufStart,(bigUnitStruct*)MyPlayer);
			break;
		default:
			{
				ServerMap_s::const_iterator I=ServerRegMap.find(P[-1]);
				if (I!=ServerRegMap.end())
					I->second(P,MyPlayer,MyUc);
			}
		}
		return;
	} else if (*P==0x79) // TRY_SPELL
	{
		if ( *((DWORD*)(P+1)) > 0x100 ) /// ���� ��� "����" ����� - ���� ����� ������ ���� � ���� ������
		{
			spellServDoCustom((int*)(P+1),(P[15])!=0,MyPlayer,MyUc);
			P+=0x16;
		}
	}
/*	else if (*P==0x72) // ������� �������� �������
	{ // 7 ���� {BYTE pkt, USHORT Obj,X,Y;}
		char Buf[80];
		USHORT V=toShort(P+1);
		netUnitByCodeServ(V);
		sprintf(Buf,"72 %x (%d,%d)",V,toShort(P+3),toShort(P+5) );
		conPrintI(Buf);	

	}*/
}
extern void InjectJumpTo(DWORD Addr,void *Fn);
extern void InjectOffs(DWORD Addr,void *Fn);
void netInit()
{
	ASSIGN(netSpriteByCodeHi,0x0045A720);
	ASSIGN(netSpriteByCodeLo,0x0045A6F0);

	ASSIGN(netClientSend,0x0040EBC0);
	ASSIGN(netGetUnitCodeServ,0x00578AC0);
	ASSIGN(netGetUnitByExtent,0x4ED020);
	ASSIGN(netGetUnitCodeCli,0x00578B00);
	ASSIGN(netSendBySock,0x00552640);
	ASSIGN(netPriMsg,0x004DA2C0);




	ASSIGN(playerGoObserver,0x004E6860);

	
	
	registerserver("servNetCode",&netGetCodeServL);

	/// ����������� �����������
	ASSIGN(netSendPointFx,0x522FF0);
	ASSIGN(netSendRayFx,0x5232F0);
	ASSIGN(netSendShieldFx,0x00523670);

	registerserver("netOnResp",&netOnRespL); /// ������� ������� �� ����� �������
	registerserver("netPointFx",&netPointFx);
	registerserver("netRayFx",&netRayFx);
	registerserver("netShieldFx",&netShieldFx);
	registerserver("netReq",&netDoReq);

	registerserver("netFake",&netFake);
	registerclient("netGetVersion",netGetVersion);
	registerclient("netVersionRq",&netVersionRq); /// ������� �������� �������� ������ �������
	registerclient("netToServer",&sendToServer);
}
